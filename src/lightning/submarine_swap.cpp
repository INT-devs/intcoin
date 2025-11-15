// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/submarine_swap.h"
#include <random>
#include <chrono>
#include <algorithm>

namespace intcoin {
namespace submarine {

//=============================================================================
// SubmarineSwap Serialization
//=============================================================================

std::vector<uint8_t> SubmarineSwap::serialize() const {
    std::vector<uint8_t> result;

    // Version
    uint32_t version = SUBMARINE_SWAP_VERSION;
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&version),
                 reinterpret_cast<const uint8_t*>(&version) + sizeof(version));

    // Swap ID
    result.insert(result.end(), swap_id.data.begin(), swap_id.data.end());

    // Payment hash
    result.insert(result.end(), payment_hash.data.begin(), payment_hash.data.end());

    // Preimage
    result.insert(result.end(), preimage.data.begin(), preimage.data.end());

    // Direction and state
    result.push_back(static_cast<uint8_t>(direction));
    result.push_back(static_cast<uint8_t>(state));

    // Amounts
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&fee_sat),
                 reinterpret_cast<const uint8_t*>(&fee_sat) + sizeof(fee_sat));

    // Heights
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_height),
                 reinterpret_cast<const uint8_t*>(&timeout_height) + sizeof(timeout_height));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&created_at),
                 reinterpret_cast<const uint8_t*>(&created_at) + sizeof(created_at));

    // Funding transaction
    auto funding_bytes = funding_tx.serialize();
    uint32_t funding_size = funding_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&funding_size),
                 reinterpret_cast<const uint8_t*>(&funding_size) + sizeof(funding_size));
    result.insert(result.end(), funding_bytes.begin(), funding_bytes.end());

    // Addresses
    auto claim_bytes = claim_address.serialize();
    uint32_t claim_size = claim_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&claim_size),
                 reinterpret_cast<const uint8_t*>(&claim_size) + sizeof(claim_size));
    result.insert(result.end(), claim_bytes.begin(), claim_bytes.end());

    auto refund_bytes = refund_address.serialize();
    uint32_t refund_size = refund_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&refund_size),
                 reinterpret_cast<const uint8_t*>(&refund_size) + sizeof(refund_size));
    result.insert(result.end(), refund_bytes.begin(), refund_bytes.end());

    // Optional channel_id
    if (channel_id.has_value()) {
        result.push_back(1);
        result.insert(result.end(),
                     channel_id->data.begin(),
                     channel_id->data.end());
    } else {
        result.push_back(0);
    }

    // Optional htlc_id
    if (htlc_id.has_value()) {
        result.push_back(1);
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&(*htlc_id)),
                     reinterpret_cast<const uint8_t*>(&(*htlc_id)) + sizeof(*htlc_id));
    } else {
        result.push_back(0);
    }

    return result;
}

SubmarineSwap SubmarineSwap::deserialize(const std::vector<uint8_t>& data) {
    SubmarineSwap swap;
    size_t offset = 0;

    // Version
    uint32_t version;
    std::memcpy(&version, &data[offset], sizeof(version));
    offset += sizeof(version);

    // Swap ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.swap_id.data.begin());
    offset += 32;

    // Payment hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.payment_hash.data.begin());
    offset += 32;

    // Preimage
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.preimage.data.begin());
    offset += 32;

    // Direction and state
    swap.direction = static_cast<SwapDirection>(data[offset++]);
    swap.state = static_cast<SwapState>(data[offset++]);

    // Amounts
    std::memcpy(&swap.amount_sat, &data[offset], sizeof(swap.amount_sat));
    offset += sizeof(swap.amount_sat);
    std::memcpy(&swap.fee_sat, &data[offset], sizeof(swap.fee_sat));
    offset += sizeof(swap.fee_sat);

    // Heights
    std::memcpy(&swap.timeout_height, &data[offset], sizeof(swap.timeout_height));
    offset += sizeof(swap.timeout_height);
    std::memcpy(&swap.created_at, &data[offset], sizeof(swap.created_at));
    offset += sizeof(swap.created_at);

    // Funding transaction
    uint32_t funding_size;
    std::memcpy(&funding_size, &data[offset], sizeof(funding_size));
    offset += sizeof(funding_size);
    std::vector<uint8_t> funding_bytes(data.begin() + offset, data.begin() + offset + funding_size);
    swap.funding_tx = Transaction::deserialize(funding_bytes);
    offset += funding_size;

    // Addresses
    uint32_t claim_size;
    std::memcpy(&claim_size, &data[offset], sizeof(claim_size));
    offset += sizeof(claim_size);
    std::vector<uint8_t> claim_bytes(data.begin() + offset, data.begin() + offset + claim_size);
    swap.claim_address = Address::deserialize(claim_bytes);
    offset += claim_size;

    uint32_t refund_size;
    std::memcpy(&refund_size, &data[offset], sizeof(refund_size));
    offset += sizeof(refund_size);
    std::vector<uint8_t> refund_bytes(data.begin() + offset, data.begin() + offset + refund_size);
    swap.refund_address = Address::deserialize(refund_bytes);
    offset += refund_size;

    // Optional channel_id
    if (data[offset++] == 1) {
        Hash256 cid;
        std::copy(data.begin() + offset, data.begin() + offset + 32, cid.data.begin());
        swap.channel_id = cid;
        offset += 32;
    }

    // Optional htlc_id
    if (data[offset++] == 1) {
        uint64_t hid;
        std::memcpy(&hid, &data[offset], sizeof(hid));
        swap.htlc_id = hid;
        offset += sizeof(hid);
    }

    return swap;
}

//=============================================================================
// SwapQuote Serialization
//=============================================================================

std::vector<uint8_t> SwapQuote::serialize() const {
    std::vector<uint8_t> result;

    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&service_fee_sat),
                 reinterpret_cast<const uint8_t*>(&service_fee_sat) + sizeof(service_fee_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&network_fee_sat),
                 reinterpret_cast<const uint8_t*>(&network_fee_sat) + sizeof(network_fee_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&total_cost_sat),
                 reinterpret_cast<const uint8_t*>(&total_cost_sat) + sizeof(total_cost_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_blocks),
                 reinterpret_cast<const uint8_t*>(&timeout_blocks) + sizeof(timeout_blocks));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&exchange_rate),
                 reinterpret_cast<const uint8_t*>(&exchange_rate) + sizeof(exchange_rate));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&expires_at),
                 reinterpret_cast<const uint8_t*>(&expires_at) + sizeof(expires_at));

    return result;
}

SwapQuote SwapQuote::deserialize(const std::vector<uint8_t>& data) {
    SwapQuote quote;
    size_t offset = 0;

    std::memcpy(&quote.amount_sat, &data[offset], sizeof(quote.amount_sat));
    offset += sizeof(quote.amount_sat);
    std::memcpy(&quote.service_fee_sat, &data[offset], sizeof(quote.service_fee_sat));
    offset += sizeof(quote.service_fee_sat);
    std::memcpy(&quote.network_fee_sat, &data[offset], sizeof(quote.network_fee_sat));
    offset += sizeof(quote.network_fee_sat);
    std::memcpy(&quote.total_cost_sat, &data[offset], sizeof(quote.total_cost_sat));
    offset += sizeof(quote.total_cost_sat);
    std::memcpy(&quote.timeout_blocks, &data[offset], sizeof(quote.timeout_blocks));
    offset += sizeof(quote.timeout_blocks);
    std::memcpy(&quote.exchange_rate, &data[offset], sizeof(quote.exchange_rate));
    offset += sizeof(quote.exchange_rate);
    std::memcpy(&quote.expires_at, &data[offset], sizeof(quote.expires_at));
    offset += sizeof(quote.expires_at);

    return quote;
}

//=============================================================================
// SubmarineSwapManager Implementation
//=============================================================================

SubmarineSwapManager::SubmarineSwapManager()
    : base_fee_pct_(0.01),      // 1% default fee
      min_fee_sat_(1000),        // 1000 sats minimum
      max_fee_sat_(100000),      // 100k sats maximum
      current_height_(0) {
}

std::optional<SubmarineSwap> SubmarineSwapManager::initiate_on_to_off_swap(
    uint64_t amount_sat,
    const std::string& lightning_invoice,
    const Address& refund_address,
    uint32_t timeout_blocks) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Validate timeout
    if (timeout_blocks < MIN_SWAP_TIMEOUT || timeout_blocks > MAX_SWAP_TIMEOUT) {
        return std::nullopt;
    }

    // Create swap
    SubmarineSwap swap;
    swap.swap_id = generate_swap_id();
    swap.preimage = generate_preimage();
    swap.payment_hash = compute_payment_hash(swap.preimage);
    swap.direction = SwapDirection::ON_TO_OFF;
    swap.state = SwapState::PENDING;
    swap.amount_sat = amount_sat;
    swap.fee_sat = calculate_service_fee(amount_sat);
    swap.timeout_height = current_height_ + timeout_blocks;
    swap.created_at = current_height_;
    swap.refund_address = refund_address;

    // Store swap
    swaps_[swap.swap_id] = swap;

    return swap;
}

bool SubmarineSwapManager::fund_swap(const Hash256& swap_id, const Transaction& funding_tx) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return false;
    }

    SubmarineSwap& swap = it->second;

    if (swap.state != SwapState::PENDING) {
        return false;
    }

    swap.funding_tx = funding_tx;
    swap.state = SwapState::FUNDED;

    return true;
}

bool SubmarineSwapManager::claim_lightning_payment(const Hash256& swap_id, const Hash256& preimage) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return false;
    }

    SubmarineSwap& swap = it->second;

    if (swap.state != SwapState::FUNDED) {
        return false;
    }

    if (!verify_preimage(swap.payment_hash, preimage)) {
        return false;
    }

    swap.state = SwapState::REDEEMED;

    return true;
}

std::optional<SubmarineSwap> SubmarineSwapManager::initiate_off_to_on_swap(
    uint64_t amount_sat,
    const Address& claim_address,
    uint32_t timeout_blocks) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Validate timeout
    if (timeout_blocks < MIN_SWAP_TIMEOUT || timeout_blocks > MAX_SWAP_TIMEOUT) {
        return std::nullopt;
    }

    // Create swap
    SubmarineSwap swap;
    swap.swap_id = generate_swap_id();
    swap.preimage = generate_preimage();
    swap.payment_hash = compute_payment_hash(swap.preimage);
    swap.direction = SwapDirection::OFF_TO_ON;
    swap.state = SwapState::PENDING;
    swap.amount_sat = amount_sat;
    swap.fee_sat = calculate_service_fee(amount_sat);
    swap.timeout_height = current_height_ + timeout_blocks;
    swap.created_at = current_height_;
    swap.claim_address = claim_address;

    // Store swap
    swaps_[swap.swap_id] = swap;

    return swap;
}

bool SubmarineSwapManager::create_lightning_payment(
    const Hash256& swap_id,
    const Hash256& channel_id,
    uint64_t htlc_id) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return false;
    }

    SubmarineSwap& swap = it->second;

    if (swap.state != SwapState::PENDING) {
        return false;
    }

    swap.channel_id = channel_id;
    swap.htlc_id = htlc_id;
    swap.state = SwapState::FUNDED;

    return true;
}

bool SubmarineSwapManager::claim_onchain_funds(const Hash256& swap_id, const Hash256& preimage) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return false;
    }

    SubmarineSwap& swap = it->second;

    if (swap.state != SwapState::FUNDED) {
        return false;
    }

    if (!verify_preimage(swap.payment_hash, preimage)) {
        return false;
    }

    // Create claim transaction
    Transaction claim_tx = create_htlc_claim_tx(swap, preimage);

    // TODO: Broadcast claim_tx to network

    swap.state = SwapState::REDEEMED;

    return true;
}

std::optional<SubmarineSwap> SubmarineSwapManager::get_swap(const Hash256& swap_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<SubmarineSwap> SubmarineSwapManager::list_swaps() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<SubmarineSwap> result;
    result.reserve(swaps_.size());

    for (const auto& [id, swap] : swaps_) {
        result.push_back(swap);
    }

    return result;
}

std::vector<SubmarineSwap> SubmarineSwapManager::list_swaps_by_state(SwapState state) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<SubmarineSwap> result;

    for (const auto& [id, swap] : swaps_) {
        if (swap.state == state) {
            result.push_back(swap);
        }
    }

    return result;
}

bool SubmarineSwapManager::refund_swap(const Hash256& swap_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = swaps_.find(swap_id);
    if (it == swaps_.end()) {
        return false;
    }

    SubmarineSwap& swap = it->second;

    // Can only refund if timeout has passed
    if (current_height_ < swap.timeout_height) {
        return false;
    }

    if (swap.state != SwapState::FUNDED) {
        return false;
    }

    // Create refund transaction
    Transaction refund_tx = create_htlc_refund_tx(swap);

    // TODO: Broadcast refund_tx to network

    swap.state = SwapState::REFUNDED;

    return true;
}

void SubmarineSwapManager::monitor_swaps(uint32_t current_block_height) {
    std::lock_guard<std::mutex> lock(mutex_);

    current_height_ = current_block_height;

    // Check for timed-out swaps and auto-refund
    for (auto& [id, swap] : swaps_) {
        if (swap.state == SwapState::FUNDED &&
            current_height_ >= swap.timeout_height) {

            // Auto-refund
            Transaction refund_tx = create_htlc_refund_tx(swap);
            // TODO: Broadcast refund_tx to network

            swap.state = SwapState::REFUNDED;
        }
    }
}

SwapQuote SubmarineSwapManager::get_swap_quote(SwapDirection direction, uint64_t amount_sat) const {
    std::lock_guard<std::mutex> lock(mutex_);

    SwapQuote quote;
    quote.amount_sat = amount_sat;
    quote.service_fee_sat = calculate_service_fee(amount_sat);
    quote.network_fee_sat = 5000;  // Estimated on-chain fee (TODO: dynamic estimation)
    quote.total_cost_sat = amount_sat + quote.service_fee_sat + quote.network_fee_sat;
    quote.timeout_blocks = DEFAULT_SWAP_TIMEOUT;
    quote.exchange_rate = 1.0;

    // Quote expires in 5 minutes
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::minutes(5);
    quote.expires_at = std::chrono::system_clock::to_time_t(expires);

    return quote;
}

uint64_t SubmarineSwapManager::calculate_service_fee(uint64_t amount_sat) const {
    uint64_t fee = static_cast<uint64_t>(amount_sat * base_fee_pct_);

    // Clamp to min/max
    if (fee < min_fee_sat_) {
        fee = min_fee_sat_;
    }
    if (fee > max_fee_sat_) {
        fee = max_fee_sat_;
    }

    return fee;
}

void SubmarineSwapManager::set_fee_params(double base_fee_pct,
                                         uint64_t min_fee_sat,
                                         uint64_t max_fee_sat) {
    std::lock_guard<std::mutex> lock(mutex_);

    base_fee_pct_ = base_fee_pct;
    min_fee_sat_ = min_fee_sat;
    max_fee_sat_ = max_fee_sat;
}

SubmarineSwapManager::SwapStats SubmarineSwapManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    SwapStats stats{};
    stats.total_swaps = swaps_.size();

    for (const auto& [id, swap] : swaps_) {
        switch (swap.state) {
            case SwapState::REDEEMED:
                stats.successful_swaps++;
                stats.total_volume_sat += swap.amount_sat;
                stats.total_fees_sat += swap.fee_sat;
                break;
            case SwapState::FAILED:
            case SwapState::REFUNDED:
                stats.failed_swaps++;
                break;
            case SwapState::PENDING:
            case SwapState::FUNDED:
                stats.pending_swaps++;
                break;
        }
    }

    return stats;
}

//=============================================================================
// Private Helper Methods
//=============================================================================

Hash256 SubmarineSwapManager::generate_swap_id() const {
    // Generate random swap ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    Hash256 id;
    for (size_t i = 0; i < 4; i++) {
        uint64_t val = dis(gen);
        std::memcpy(&id.data[i * 8], &val, 8);
    }

    return id;
}

Hash256 SubmarineSwapManager::generate_preimage() const {
    // Generate random preimage
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    Hash256 preimage;
    for (size_t i = 0; i < 4; i++) {
        uint64_t val = dis(gen);
        std::memcpy(&preimage.data[i * 8], &val, 8);
    }

    return preimage;
}

Hash256 SubmarineSwapManager::compute_payment_hash(const Hash256& preimage) const {
    // Compute SHA3-256 hash of preimage
    return sha3_256(preimage.data);
}

Transaction SubmarineSwapManager::create_htlc_funding_tx(const SubmarineSwap& swap) const {
    // Create HTLC funding transaction
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // TODO: Create proper HTLC output script:
    // OP_IF
    //   OP_SHA256 <payment_hash> OP_EQUALVERIFY <claim_pubkey> OP_CHECKSIG
    // OP_ELSE
    //   <timeout_height> OP_CHECKLOCKTIMEVERIFY OP_DROP <refund_pubkey> OP_CHECKSIG
    // OP_ENDIF

    return tx;
}

Transaction SubmarineSwapManager::create_htlc_claim_tx(
    const SubmarineSwap& swap,
    const Hash256& preimage) const {

    // Create claim transaction that spends HTLC with preimage
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // TODO: Create input spending HTLC funding tx
    // TODO: Add witness with preimage and signature

    return tx;
}

Transaction SubmarineSwapManager::create_htlc_refund_tx(const SubmarineSwap& swap) const {
    // Create refund transaction that spends HTLC after timeout
    Transaction tx;
    tx.version = 1;
    tx.locktime = swap.timeout_height;

    // TODO: Create input spending HTLC funding tx
    // TODO: Add witness with signature (using timeout path)

    return tx;
}

bool SubmarineSwapManager::verify_preimage(const Hash256& payment_hash,
                                          const Hash256& preimage) const {
    Hash256 computed_hash = compute_payment_hash(preimage);
    return computed_hash == payment_hash;
}

void SubmarineSwapManager::update_swap_state(const Hash256& swap_id, SwapState new_state) {
    auto it = swaps_.find(swap_id);
    if (it != swaps_.end()) {
        it->second.state = new_state;
    }
}

//=============================================================================
// SubmarineSwapService Implementation
//=============================================================================

SubmarineSwapService::SubmarineSwapService(uint16_t listen_port)
    : listen_port_(listen_port),
      running_(false) {
}

bool SubmarineSwapService::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return false;
    }

    // TODO: Start network listener
    // TODO: Start blockchain monitor

    running_ = true;
    return true;
}

void SubmarineSwapService::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    // TODO: Stop network listener
    // TODO: Stop blockchain monitor

    running_ = false;
}

std::optional<SwapQuote> SubmarineSwapService::handle_quote_request(
    SwapDirection direction,
    uint64_t amount_sat) {

    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return std::nullopt;
    }

    return swap_manager_.get_swap_quote(direction, amount_sat);
}

bool SubmarineSwapService::handle_swap_request(const SubmarineSwap& swap) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return false;
    }

    // TODO: Validate swap request
    // TODO: Create corresponding swap on service side
    // TODO: Fund Lightning payment or create on-chain HTLC

    return true;
}

SubmarineSwapService::ServiceStats SubmarineSwapService::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto mgr_stats = swap_manager_.get_stats();

    ServiceStats stats{};
    stats.active_swaps = mgr_stats.pending_swaps;
    stats.completed_swaps = mgr_stats.successful_swaps;
    stats.total_volume_sat = mgr_stats.total_volume_sat;
    stats.total_fees_earned_sat = mgr_stats.total_fees_sat;

    return stats;
}

} // namespace submarine
} // namespace intcoin
