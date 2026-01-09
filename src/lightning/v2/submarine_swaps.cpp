// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/submarine_swaps.h>
#include <sstream>
#include <algorithm>
#include <map>
#include <random>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetSwapTypeName(SwapType type) {
    switch (type) {
        case SwapType::SWAP_IN: return "SWAP_IN";
        case SwapType::SWAP_OUT: return "SWAP_OUT";
        case SwapType::LOOP_IN: return "LOOP_IN";
        case SwapType::LOOP_OUT: return "LOOP_OUT";
        default: return "UNKNOWN";
    }
}

SwapType ParseSwapType(const std::string& name) {
    if (name == "SWAP_IN" || name == "LOOP_IN") return SwapType::SWAP_IN;
    if (name == "SWAP_OUT" || name == "LOOP_OUT") return SwapType::SWAP_OUT;
    return SwapType::SWAP_IN;
}

std::string GetSwapStatusName(SwapStatus status) {
    switch (status) {
        case SwapStatus::PENDING: return "PENDING";
        case SwapStatus::INVOICE_GENERATED: return "INVOICE_GENERATED";
        case SwapStatus::LOCKUP_TX_BROADCAST: return "LOCKUP_TX_BROADCAST";
        case SwapStatus::LOCKUP_TX_CONFIRMED: return "LOCKUP_TX_CONFIRMED";
        case SwapStatus::CLAIM_TX_BROADCAST: return "CLAIM_TX_BROADCAST";
        case SwapStatus::CLAIM_TX_CONFIRMED: return "CLAIM_TX_CONFIRMED";
        case SwapStatus::COMPLETED: return "COMPLETED";
        case SwapStatus::REFUNDED: return "REFUNDED";
        case SwapStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

SwapStatus ParseSwapStatus(const std::string& name) {
    if (name == "PENDING") return SwapStatus::PENDING;
    if (name == "INVOICE_GENERATED") return SwapStatus::INVOICE_GENERATED;
    if (name == "LOCKUP_TX_BROADCAST") return SwapStatus::LOCKUP_TX_BROADCAST;
    if (name == "LOCKUP_TX_CONFIRMED") return SwapStatus::LOCKUP_TX_CONFIRMED;
    if (name == "CLAIM_TX_BROADCAST") return SwapStatus::CLAIM_TX_BROADCAST;
    if (name == "CLAIM_TX_CONFIRMED") return SwapStatus::CLAIM_TX_CONFIRMED;
    if (name == "COMPLETED") return SwapStatus::COMPLETED;
    if (name == "REFUNDED") return SwapStatus::REFUNDED;
    if (name == "FAILED") return SwapStatus::FAILED;
    return SwapStatus::PENDING;
}

// Pimpl implementation
class SubmarineSwapManager::Impl {
public:
    Config config_;
    std::map<std::string, SubmarineSwap> swaps_;
    Statistics stats_;
    bool enabled_{true};

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}

    // HTLC script generation helpers
    std::vector<uint8_t> GeneratePreimage();
    std::vector<uint8_t> GeneratePaymentHash(const std::vector<uint8_t>& preimage);
    std::vector<uint8_t> CreateHTLCLockupScript(
        const std::string& sender_pubkey,
        const std::string& receiver_pubkey,
        const std::vector<uint8_t>& payment_hash,
        uint32_t timeout_height
    );
    std::vector<uint8_t> CreateHTLCRedeemScript(
        const std::vector<uint8_t>& htlc_script,
        const std::vector<uint8_t>& preimage,
        const std::vector<uint8_t>& receiver_sig
    );
    std::vector<uint8_t> CreateHTLCRefundScript(
        const std::vector<uint8_t>& htlc_script,
        const std::vector<uint8_t>& sender_sig
    );
    std::string GenerateLockupAddress(const std::vector<uint8_t>& htlc_script);
    std::string BytesToHex(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> HexToBytes(const std::string& hex);

    // Blockchain monitoring helpers
    uint32_t GetCurrentBlockHeight();
    bool CheckTransactionConfirmed(const std::string& txid, uint32_t required_confirmations);
    std::string FindLockupTransaction(const std::string& lockup_address);
    bool CheckTransactionHasPreimage(const std::string& txid, const std::vector<uint8_t>& payment_hash);
    bool IsTimeoutExpired(uint32_t timeout_height);
    void UpdateSwapStatus(SubmarineSwap& swap);
};

SubmarineSwapManager::SubmarineSwapManager()
    : pimpl_(std::make_unique<Impl>()) {}

SubmarineSwapManager::SubmarineSwapManager(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

SubmarineSwapManager::~SubmarineSwapManager() = default;

SwapQuote SubmarineSwapManager::GetQuote(SwapType type, uint64_t amount) const {
    SwapQuote quote;
    quote.type = type;
    quote.amount = amount;
    quote.service_fee = amount / 100;  // 1% service fee
    quote.onchain_fee = 5000;          // ~5000 ints miner fee
    quote.total_fee = quote.service_fee + quote.onchain_fee;
    quote.fee_percentage = 1.0 + (static_cast<double>(quote.onchain_fee) / amount * 100.0);
    quote.timeout_blocks = pimpl_->config_.default_timeout;
    quote.min_amount = pimpl_->config_.min_swap_amount;
    quote.max_amount = pimpl_->config_.max_swap_amount;

    return quote;
}

SubmarineSwap SubmarineSwapManager::CreateSwapIn(
    uint64_t amount,
    const std::string& refund_address
) {
    SubmarineSwap swap;
    swap.swap_id = "swap_in_" + std::to_string(pimpl_->swaps_.size());
    swap.type = SwapType::SWAP_IN;
    swap.status = SwapStatus::PENDING;
    swap.amount = amount;
    swap.refund_address = refund_address;

    auto quote = GetQuote(SwapType::SWAP_IN, amount);
    swap.fee = quote.total_fee;

    // Generate cryptographic material
    auto preimage = pimpl_->GeneratePreimage();
    auto payment_hash = pimpl_->GeneratePaymentHash(preimage);

    swap.preimage = pimpl_->BytesToHex(preimage);
    swap.payment_hash = pimpl_->BytesToHex(payment_hash);

    // Generate HTLC lockup script for on-chain deposit
    // User sends to lockup address, service provides Lightning payment
    std::string user_pubkey = "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798";  // Placeholder
    std::string service_pubkey = pimpl_->config_.server_pubkey.empty()
        ? "02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"  // Placeholder
        : pimpl_->config_.server_pubkey;

    uint32_t current_height = 800000;  // Would get from blockchain in production
    uint32_t timeout_height = current_height + quote.timeout_blocks;
    swap.timeout_height = timeout_height;

    auto htlc_script = pimpl_->CreateHTLCLockupScript(
        user_pubkey,      // User can refund after timeout
        service_pubkey,   // Service can claim with preimage
        payment_hash,
        timeout_height
    );

    // Generate lockup address
    swap.lockup_address = pimpl_->GenerateLockupAddress(htlc_script);

    // Set timestamps
    swap.created_at = static_cast<uint64_t>(std::time(nullptr));
    swap.expires_at = swap.created_at + (quote.timeout_blocks * 600);  // ~10 min per block

    pimpl_->swaps_[swap.swap_id] = swap;
    pimpl_->stats_.total_swaps++;

    return swap;
}

SubmarineSwap SubmarineSwapManager::CreateSwapOut(
    uint64_t amount,
    const std::string& claim_address
) {
    SubmarineSwap swap;
    swap.swap_id = "swap_out_" + std::to_string(pimpl_->swaps_.size());
    swap.type = SwapType::SWAP_OUT;
    swap.status = SwapStatus::PENDING;
    swap.amount = amount;
    swap.claim_address = claim_address;

    auto quote = GetQuote(SwapType::SWAP_OUT, amount);
    swap.fee = quote.total_fee;

    // Generate cryptographic material
    auto preimage = pimpl_->GeneratePreimage();
    auto payment_hash = pimpl_->GeneratePaymentHash(preimage);

    swap.preimage = pimpl_->BytesToHex(preimage);
    swap.payment_hash = pimpl_->BytesToHex(payment_hash);

    // Generate HTLC lockup script for swap-out
    // Service locks on-chain funds, user pays Lightning invoice with payment hash
    std::string service_pubkey = pimpl_->config_.server_pubkey.empty()
        ? "02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"  // Placeholder
        : pimpl_->config_.server_pubkey;
    std::string user_pubkey = "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798";  // Placeholder

    uint32_t current_height = 800000;  // Would get from blockchain in production
    uint32_t timeout_height = current_height + quote.timeout_blocks;
    swap.timeout_height = timeout_height;

    auto htlc_script = pimpl_->CreateHTLCLockupScript(
        service_pubkey,   // Service can refund after timeout
        user_pubkey,      // User can claim with preimage (from Lightning payment)
        payment_hash,
        timeout_height
    );

    // Generate lockup address where service will send on-chain funds
    swap.lockup_address = pimpl_->GenerateLockupAddress(htlc_script);

    // Generate Lightning invoice for user to pay (contains payment_hash)
    // Format: lnbc<amount><payment_hash>
    swap.invoice = "lnbc" + std::to_string(amount) + "n1" + swap.payment_hash.substr(0, 20);
    swap.payment_request = swap.invoice;
    swap.status = SwapStatus::INVOICE_GENERATED;

    // Set timestamps
    swap.created_at = static_cast<uint64_t>(std::time(nullptr));
    swap.expires_at = swap.created_at + (quote.timeout_blocks * 600);  // ~10 min per block

    pimpl_->swaps_[swap.swap_id] = swap;
    pimpl_->stats_.total_swaps++;

    return swap;
}

bool SubmarineSwapManager::CompleteSwapIn(
    const std::string& swap_id,
    const std::string& lockup_txid
) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.lockup_txid = lockup_txid;
        it->second.status = SwapStatus::COMPLETED;
        pimpl_->stats_.completed_swaps++;
        pimpl_->stats_.total_swapped_in += it->second.amount;
        return true;
    }
    return false;
}

std::string SubmarineSwapManager::CompleteSwapOut(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.status = SwapStatus::COMPLETED;
        pimpl_->stats_.completed_swaps++;
        pimpl_->stats_.total_swapped_out += it->second.amount;
        return "preimage_" + swap_id;
    }
    return "";
}

std::string SubmarineSwapManager::RefundSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.status = SwapStatus::REFUNDED;
        it->second.refund_txid = "refund_tx_" + swap_id;
        pimpl_->stats_.refunded_swaps++;
        return it->second.refund_txid;
    }
    return "";
}

SubmarineSwap SubmarineSwapManager::GetSwap(const std::string& swap_id) const {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        return it->second;
    }
    return SubmarineSwap{};
}

std::vector<SubmarineSwap> SubmarineSwapManager::GetActiveSwaps() const {
    std::vector<SubmarineSwap> active;

    for (auto& [id, swap] : pimpl_->swaps_) {
        // Update each swap's status before checking
        pimpl_->UpdateSwapStatus(swap);

        if (swap.status != SwapStatus::COMPLETED &&
            swap.status != SwapStatus::FAILED &&
            swap.status != SwapStatus::REFUNDED) {
            active.push_back(swap);
        }
    }

    return active;
}

std::vector<SubmarineSwap> SubmarineSwapManager::GetSwapHistory(uint32_t limit) const {
    std::vector<SubmarineSwap> history;
    uint32_t count = 0;

    for (auto it = pimpl_->swaps_.rbegin();
         it != pimpl_->swaps_.rend() && count < limit;
         ++it, ++count) {
        history.push_back(it->second);
    }

    return history;
}

SwapStatus SubmarineSwapManager::MonitorSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        // Update swap status based on blockchain state
        pimpl_->UpdateSwapStatus(it->second);
        return it->second.status;
    }
    return SwapStatus::FAILED;
}

bool SubmarineSwapManager::CancelSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end() && it->second.status == SwapStatus::PENDING) {
        it->second.status = SwapStatus::FAILED;
        pimpl_->stats_.failed_swaps++;
        return true;
    }
    return false;
}

SubmarineSwapManager::SwapLimits SubmarineSwapManager::GetSwapLimits(SwapType type) const {
    (void)type;

    SwapLimits limits;
    limits.min_amount = pimpl_->config_.min_swap_amount;
    limits.max_amount = pimpl_->config_.max_swap_amount;
    return limits;
}

uint64_t SubmarineSwapManager::EstimateFees(SwapType type, uint64_t amount) const {
    auto quote = GetQuote(type, amount);
    return quote.total_fee;
}

void SubmarineSwapManager::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

SubmarineSwapManager::Config SubmarineSwapManager::GetConfig() const {
    return pimpl_->config_;
}

SubmarineSwapManager::Statistics SubmarineSwapManager::GetStatistics() const {
    return pimpl_->stats_;
}

void SubmarineSwapManager::SetEnabled(bool enabled) {
    pimpl_->enabled_ = enabled;
}

bool SubmarineSwapManager::IsEnabled() const {
    return pimpl_->enabled_;
}

// ============================================================================
// HTLC Script Generation Implementation
// ============================================================================

/**
 * Generate random 32-byte preimage
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::GeneratePreimage() {
    std::vector<uint8_t> preimage(32);
    if (RAND_bytes(preimage.data(), 32) != 1) {
        // Fallback to C++ random if OpenSSL fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : preimage) {
            byte = static_cast<uint8_t>(dis(gen));
        }
    }
    return preimage;
}

/**
 * Generate payment hash (SHA256 of preimage)
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::GeneratePaymentHash(
    const std::vector<uint8_t>& preimage
) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(preimage.data(), preimage.size(), hash.data());
    return hash;
}

/**
 * Create HTLC lockup script
 *
 * Script structure (P2WSH):
 * OP_IF
 *   OP_SHA256 <payment_hash> OP_EQUALVERIFY
 *   <receiver_pubkey> OP_CHECKSIG
 * OP_ELSE
 *   <timeout_height> OP_CHECKLOCKTIMEVERIFY OP_DROP
 *   <sender_pubkey> OP_CHECKSIG
 * OP_ENDIF
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::CreateHTLCLockupScript(
    const std::string& sender_pubkey,
    const std::string& receiver_pubkey,
    const std::vector<uint8_t>& payment_hash,
    uint32_t timeout_height
) {
    std::vector<uint8_t> script;

    // OP_IF (0x63)
    script.push_back(0x63);

    // Hash lock branch (receiver can claim with preimage)
    // OP_SHA256 (0xa8)
    script.push_back(0xa8);

    // Push payment hash (32 bytes)
    script.push_back(0x20);  // PUSH 32 bytes
    script.insert(script.end(), payment_hash.begin(), payment_hash.end());

    // OP_EQUALVERIFY (0x88)
    script.push_back(0x88);

    // Push receiver pubkey (33 bytes for compressed key)
    auto receiver_bytes = HexToBytes(receiver_pubkey);
    script.push_back(static_cast<uint8_t>(receiver_bytes.size()));
    script.insert(script.end(), receiver_bytes.begin(), receiver_bytes.end());

    // OP_CHECKSIG (0xac)
    script.push_back(0xac);

    // OP_ELSE (0x67)
    script.push_back(0x67);

    // Time lock branch (sender can refund after timeout)
    // Push timeout height (4 bytes, little-endian)
    script.push_back(0x04);  // PUSH 4 bytes
    script.push_back(static_cast<uint8_t>(timeout_height & 0xFF));
    script.push_back(static_cast<uint8_t>((timeout_height >> 8) & 0xFF));
    script.push_back(static_cast<uint8_t>((timeout_height >> 16) & 0xFF));
    script.push_back(static_cast<uint8_t>((timeout_height >> 24) & 0xFF));

    // OP_CHECKLOCKTIMEVERIFY (0xb1)
    script.push_back(0xb1);

    // OP_DROP (0x75)
    script.push_back(0x75);

    // Push sender pubkey (33 bytes for compressed key)
    auto sender_bytes = HexToBytes(sender_pubkey);
    script.push_back(static_cast<uint8_t>(sender_bytes.size()));
    script.insert(script.end(), sender_bytes.begin(), sender_bytes.end());

    // OP_CHECKSIG (0xac)
    script.push_back(0xac);

    // OP_ENDIF (0x68)
    script.push_back(0x68);

    return script;
}

/**
 * Create HTLC redeem witness script (claim with preimage)
 *
 * Witness structure:
 * <receiver_signature> <preimage> <1> <htlc_script>
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::CreateHTLCRedeemScript(
    const std::vector<uint8_t>& htlc_script,
    const std::vector<uint8_t>& preimage,
    const std::vector<uint8_t>& receiver_sig
) {
    std::vector<uint8_t> witness;

    // Push receiver signature
    witness.push_back(static_cast<uint8_t>(receiver_sig.size()));
    witness.insert(witness.end(), receiver_sig.begin(), receiver_sig.end());

    // Push preimage
    witness.push_back(static_cast<uint8_t>(preimage.size()));
    witness.insert(witness.end(), preimage.begin(), preimage.end());

    // Push OP_TRUE (0x51) to take IF branch
    witness.push_back(0x51);

    // Push HTLC script
    witness.push_back(static_cast<uint8_t>(htlc_script.size()));
    witness.insert(witness.end(), htlc_script.begin(), htlc_script.end());

    return witness;
}

/**
 * Create HTLC refund witness script (timeout refund)
 *
 * Witness structure:
 * <sender_signature> <0> <htlc_script>
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::CreateHTLCRefundScript(
    const std::vector<uint8_t>& htlc_script,
    const std::vector<uint8_t>& sender_sig
) {
    std::vector<uint8_t> witness;

    // Push sender signature
    witness.push_back(static_cast<uint8_t>(sender_sig.size()));
    witness.insert(witness.end(), sender_sig.begin(), sender_sig.end());

    // Push OP_FALSE (0x00) to take ELSE branch
    witness.push_back(0x00);

    // Push HTLC script
    witness.push_back(static_cast<uint8_t>(htlc_script.size()));
    witness.insert(witness.end(), htlc_script.begin(), htlc_script.end());

    return witness;
}

/**
 * Generate P2WSH lockup address from HTLC script
 */
std::string SubmarineSwapManager::Impl::GenerateLockupAddress(
    const std::vector<uint8_t>& htlc_script
) {
    // Hash the script with SHA256 for P2WSH
    std::vector<uint8_t> script_hash(SHA256_DIGEST_LENGTH);
    SHA256(htlc_script.data(), htlc_script.size(), script_hash.data());

    // For now, return hex-encoded script hash
    // In production, this should be Bech32-encoded with proper witness version
    return "bc1q" + BytesToHex(script_hash).substr(0, 58);  // Simplified P2WSH address
}

/**
 * Convert bytes to hex string
 */
std::string SubmarineSwapManager::Impl::BytesToHex(
    const std::vector<uint8_t>& bytes
) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

/**
 * Convert hex string to bytes
 */
std::vector<uint8_t> SubmarineSwapManager::Impl::HexToBytes(
    const std::string& hex
) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// ============================================================================
// Blockchain Monitoring Implementation
// ============================================================================

/**
 * Get current blockchain height
 */
uint32_t SubmarineSwapManager::Impl::GetCurrentBlockHeight() {
    // In production, this would query the blockchain via RPC
    // For now, return a simulated block height
    return 800000 + static_cast<uint32_t>(std::time(nullptr) % 10000);
}

/**
 * Check if transaction has required confirmations
 */
bool SubmarineSwapManager::Impl::CheckTransactionConfirmed(
    const std::string& txid,
    uint32_t required_confirmations
) {
    if (txid.empty()) {
        return false;
    }

    // In production, this would:
    // 1. Query blockchain for transaction
    // 2. Get block height where tx was included
    // 3. Calculate confirmations = current_height - tx_height + 1
    // 4. Return confirmations >= required_confirmations

    // For now, simulate confirmation based on txid pattern
    // (transactions with "lockup" in ID are considered confirmed)
    if (txid.find("lockup") != std::string::npos) {
        return required_confirmations <= 6;  // Simulate 6 confirmations
    }

    return false;
}

/**
 * Find lockup transaction to a specific address
 */
std::string SubmarineSwapManager::Impl::FindLockupTransaction(
    const std::string& lockup_address
) {
    if (lockup_address.empty()) {
        return "";
    }

    // In production, this would:
    // 1. Query blockchain for transactions to lockup_address
    // 2. Verify transaction matches expected amount
    // 3. Return transaction ID

    // For now, simulate finding a lockup transaction
    // Return empty string to indicate no transaction found yet
    // This would be populated when user actually sends funds
    return "";
}

/**
 * Check if transaction witness contains the preimage
 */
bool SubmarineSwapManager::Impl::CheckTransactionHasPreimage(
    const std::string& txid,
    const std::vector<uint8_t>& payment_hash
) {
    if (txid.empty() || payment_hash.empty()) {
        return false;
    }

    // In production, this would:
    // 1. Fetch transaction from blockchain
    // 2. Parse witness data
    // 3. Check if any witness element hashes to payment_hash
    // 4. Return true if preimage found

    // For now, simulate check
    if (txid.find("claim") != std::string::npos) {
        return true;  // Simulate successful claim with preimage
    }

    return false;
}

/**
 * Check if swap timeout has expired
 */
bool SubmarineSwapManager::Impl::IsTimeoutExpired(uint32_t timeout_height) {
    uint32_t current_height = GetCurrentBlockHeight();
    return current_height >= timeout_height;
}

/**
 * Update swap status based on blockchain state
 *
 * State machine:
 * PENDING → LOCKUP_TX_BROADCAST → LOCKUP_TX_CONFIRMED →
 * CLAIM_TX_BROADCAST → CLAIM_TX_CONFIRMED → COMPLETED
 *
 * or
 *
 * PENDING → ... → REFUNDED (if timeout expired)
 */
void SubmarineSwapManager::Impl::UpdateSwapStatus(SubmarineSwap& swap) {
    // Check for timeout expiration first
    if (swap.timeout_height > 0 && IsTimeoutExpired(swap.timeout_height)) {
        if (swap.status != SwapStatus::COMPLETED &&
            swap.status != SwapStatus::REFUNDED) {
            swap.status = SwapStatus::FAILED;
            swap.error_message = "Swap expired due to timeout";
            return;
        }
    }

    // Update status based on current state
    switch (swap.status) {
        case SwapStatus::PENDING:
        case SwapStatus::INVOICE_GENERATED: {
            // Check if lockup transaction exists
            if (swap.lockup_txid.empty()) {
                std::string lockup_txid = FindLockupTransaction(swap.lockup_address);
                if (!lockup_txid.empty()) {
                    swap.lockup_txid = lockup_txid;
                    swap.status = SwapStatus::LOCKUP_TX_BROADCAST;
                }
            }
            break;
        }

        case SwapStatus::LOCKUP_TX_BROADCAST: {
            // Check if lockup transaction is confirmed
            if (!swap.lockup_txid.empty()) {
                if (CheckTransactionConfirmed(swap.lockup_txid, 3)) {
                    swap.status = SwapStatus::LOCKUP_TX_CONFIRMED;
                }
            }
            break;
        }

        case SwapStatus::LOCKUP_TX_CONFIRMED: {
            // For swap-in: Service should now pay Lightning invoice
            // For swap-out: User should now claim on-chain with preimage
            // Check if claim transaction exists
            if (swap.claim_txid.empty()) {
                // In production, monitor for spending of lockup UTXO
                // For now, this would be set when claim is initiated
            } else {
                swap.status = SwapStatus::CLAIM_TX_BROADCAST;
            }
            break;
        }

        case SwapStatus::CLAIM_TX_BROADCAST: {
            // Check if claim transaction is confirmed
            if (!swap.claim_txid.empty()) {
                if (CheckTransactionConfirmed(swap.claim_txid, 6)) {
                    swap.status = SwapStatus::CLAIM_TX_CONFIRMED;
                }
            }
            break;
        }

        case SwapStatus::CLAIM_TX_CONFIRMED: {
            // Verify preimage was revealed
            auto payment_hash = HexToBytes(swap.payment_hash);
            if (CheckTransactionHasPreimage(swap.claim_txid, payment_hash)) {
                swap.status = SwapStatus::COMPLETED;
                swap.completed_at = static_cast<uint64_t>(std::time(nullptr));
            }
            break;
        }

        case SwapStatus::COMPLETED:
        case SwapStatus::REFUNDED:
        case SwapStatus::FAILED:
            // Terminal states - no further updates
            break;
    }
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
