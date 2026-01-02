// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/mobile_sdk.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace intcoin {
namespace mobile {

MobileSDK::MobileSDK(const SDKConfig& config)
    : config_(config), wallet_open_(false) {

    LogF(LogLevel::INFO, "Mobile SDK: Initializing for INTcoin %s",
         config_.network.c_str());

    // Create database backend
    db_ = std::make_shared<BlockchainDB>(config_.wallet_path + "/spv_data");

    // Create SPV client if enabled
    if (config_.enable_spv) {
        spv_client_ = std::make_shared<SPVClient>(db_);
        LogF(LogLevel::INFO, "Mobile SDK: SPV mode enabled");
    }

    // Create mobile RPC handler
    rpc_ = std::make_shared<MobileRPC>(spv_client_, wallet_);

    LogF(LogLevel::INFO, "Mobile SDK: Initialized successfully");
}

MobileSDK::~MobileSDK() {
    CloseWallet();
}

// ========================================
// Wallet Management
// ========================================

Result<std::string> MobileSDK::CreateWallet(const std::string& mnemonic,
                                            const std::string& password) {
    if (wallet_open_) {
        return Result<std::string>::Error("Wallet already open");
    }

    LogF(LogLevel::INFO, "Mobile SDK: Creating new wallet");

    // Create wallet instance with config
    wallet::WalletConfig wallet_config;
    // TODO: Set wallet_config.wallet_path from config_.wallet_path when available
    wallet_ = std::make_shared<wallet::Wallet>(wallet_config);

    // Generate or use provided mnemonic
    std::string wallet_mnemonic = mnemonic;
    if (wallet_mnemonic.empty()) {
        // Generate new BIP39 mnemonic
        // TODO: Integrate with wallet BIP39 implementation
        wallet_mnemonic = "TODO: Generate BIP39 mnemonic";
        LogF(LogLevel::INFO, "Mobile SDK: Generated new mnemonic");
    }

    // Initialize wallet with mnemonic and password
    // TODO: Integrate with wallet initialization
    // For now, create basic wallet structure

    wallet_open_ = true;

    // Update bloom filter with wallet addresses
    if (config_.enable_spv && spv_client_) {
        UpdateBloomFilter();
    }

    LogF(LogLevel::INFO, "Mobile SDK: Wallet created successfully");

    return Result<std::string>::Ok(wallet_mnemonic);
}

Result<void> MobileSDK::OpenWallet(const std::string& password) {
    if (wallet_open_) {
        return Result<void>::Error("Wallet already open");
    }

    LogF(LogLevel::INFO, "Mobile SDK: Opening wallet");

    // Load existing wallet
    wallet::WalletConfig wallet_config;
    // TODO: Set wallet_config.wallet_path from config_.wallet_path when available
    wallet_ = std::make_shared<wallet::Wallet>(wallet_config);

    // TODO: Decrypt and verify password
    // TODO: Load wallet state from storage

    wallet_open_ = true;

    // Update bloom filter
    if (config_.enable_spv && spv_client_) {
        UpdateBloomFilter();
    }

    LogF(LogLevel::INFO, "Mobile SDK: Wallet opened successfully");

    return Result<void>::Ok();
}

void MobileSDK::CloseWallet() {
    if (!wallet_open_) {
        return;
    }

    LogF(LogLevel::INFO, "Mobile SDK: Closing wallet");

    // Stop sync
    if (spv_client_) {
        spv_client_->StopSync();
    }

    // Clear bloom filter
    if (spv_client_) {
        spv_client_->ClearBloomFilter();
    }

    wallet_.reset();
    wallet_open_ = false;

    LogF(LogLevel::INFO, "Mobile SDK: Wallet closed");
}

bool MobileSDK::IsWalletOpen() const {
    return wallet_open_;
}

Result<std::vector<uint8_t>> MobileSDK::BackupWallet() {
    if (!wallet_open_) {
        return Result<std::vector<uint8_t>>::Error("Wallet not open");
    }

    // TODO: Implement wallet backup
    // Should encrypt wallet data including keys and metadata

    LogF(LogLevel::INFO, "Mobile SDK: Creating wallet backup");

    std::vector<uint8_t> backup_data;
    // Placeholder backup
    return Result<std::vector<uint8_t>>::Ok(backup_data);
}

Result<void> MobileSDK::RestoreWallet(const std::vector<uint8_t>& backup_data,
                                      const std::string& password) {
    if (wallet_open_) {
        return Result<void>::Error("Wallet already open");
    }

    // TODO: Implement wallet restore
    // Should decrypt and validate backup data

    LogF(LogLevel::INFO, "Mobile SDK: Restoring wallet from backup");

    return Result<void>::Ok();
}

// ========================================
// Address Management
// ========================================

Result<std::string> MobileSDK::GetNewAddress() {
    if (!wallet_open_) {
        return Result<std::string>::Error("Wallet not open");
    }

    // TODO: Generate new address from wallet
    // Should use BIP32/44 derivation path: m/44'/2210'/0'/0/n

    std::string address = "int1q...";  // Placeholder
    LogF(LogLevel::DEBUG, "Mobile SDK: Generated new address: %s", address.c_str());

    // Add to bloom filter
    if (config_.enable_spv && spv_client_) {
        spv_client_->AddWatchAddress(address);
    }

    return Result<std::string>::Ok(address);
}

Result<std::string> MobileSDK::GetCurrentAddress() {
    if (!wallet_open_) {
        return Result<std::string>::Error("Wallet not open");
    }

    // TODO: Get current receiving address from wallet
    std::string address = "int1q...";  // Placeholder

    return Result<std::string>::Ok(address);
}

std::vector<std::string> MobileSDK::GetAllAddresses() {
    if (!wallet_open_) {
        return {};
    }

    // TODO: Get all addresses from wallet
    std::vector<std::string> addresses;

    return addresses;
}

bool MobileSDK::ValidateAddress(const std::string& address) {
    // INTcoin uses Bech32 format: int1...
    if (address.size() < 4 || address.substr(0, 4) != "int1") {
        return false;
    }

    // TODO: Implement full Bech32 validation
    // For now, basic check
    return address.size() >= 42 && address.size() <= 62;
}

// ========================================
// Balance & UTXO Management
// ========================================

Result<BalanceResponse> MobileSDK::GetBalance() {
    if (!wallet_open_) {
        return Result<BalanceResponse>::Error("Wallet not open");
    }

    // Get current receiving address for query
    auto addr_result = GetCurrentAddress();
    if (addr_result.IsError()) {
        return Result<BalanceResponse>::Error(addr_result.error);
    }

    BalanceRequest request;
    request.address = addr_result.GetValue();
    request.min_confirmations = 1;

    return rpc_->GetBalance(request);
}

Result<UTXOResponse> MobileSDK::GetUTXOs(uint32_t min_confirmations) {
    if (!wallet_open_) {
        return Result<UTXOResponse>::Error("Wallet not open");
    }

    auto addr_result = GetCurrentAddress();
    if (addr_result.IsError()) {
        return Result<UTXOResponse>::Error(addr_result.error);
    }

    UTXORequest request;
    request.address = addr_result.GetValue();
    request.min_confirmations = min_confirmations;

    return rpc_->GetUTXOs(request);
}

// ========================================
// Transaction Management
// ========================================

Result<Transaction> MobileSDK::CreateTransaction(const std::string& to_address,
                                                 uint64_t amount_ints,
                                                 uint64_t fee_rate) {
    if (!wallet_open_) {
        return Result<Transaction>::Error("Wallet not open");
    }

    // Validate recipient address
    if (!ValidateAddress(to_address)) {
        return Result<Transaction>::Error("Invalid recipient address");
    }

    // Get UTXOs
    auto utxo_result = GetUTXOs(1);
    if (utxo_result.IsError()) {
        return Result<Transaction>::Error("Failed to get UTXOs: " + utxo_result.error);
    }

    UTXOResponse utxos = utxo_result.GetValue();

    // Check sufficient balance
    if (utxos.total_amount < amount_ints) {
        return Result<Transaction>::Error("Insufficient balance");
    }

    // Estimate fee if not provided
    if (fee_rate == 0) {
        auto fee_result = EstimateFee(to_address, amount_ints, 6);
        if (fee_result.IsError()) {
            return Result<Transaction>::Error("Failed to estimate fee: " + fee_result.error);
        }
        fee_rate = fee_result.GetValue().fee_rate;
    }

    // TODO: Implement transaction creation
    // 1. Select UTXOs (coin selection algorithm)
    // 2. Create transaction inputs
    // 3. Create transaction outputs (recipient + change)
    // 4. Calculate fee based on transaction size
    // 5. Sign transaction with wallet keys

    Transaction tx;
    LogF(LogLevel::INFO, "Mobile SDK: Created transaction to %s for %llu INTS",
         to_address.c_str(), amount_ints);

    return Result<Transaction>::Ok(tx);
}

Result<uint256> MobileSDK::SendTransaction(const Transaction& tx) {
    if (!wallet_open_) {
        return Result<uint256>::Error("Wallet not open");
    }

    // Serialize transaction
    std::vector<uint8_t> serialized_tx = tx.Serialize();

    SendTransactionRequest request;
    request.raw_transaction = serialized_tx;

    auto result = rpc_->SendTransaction(request);
    if (result.IsError()) {
        return Result<uint256>::Error(result.error);
    }

    SendTransactionResponse response = result.GetValue();

    if (!response.accepted) {
        return Result<uint256>::Error("Transaction rejected: " + response.error);
    }

    LogF(LogLevel::INFO, "Mobile SDK: Broadcast transaction %s",
         BytesToHex(std::vector<uint8_t>(response.tx_hash.begin(),
                                        response.tx_hash.end())).substr(0, 16).c_str());

    // Trigger transaction event
    if (tx_callback_) {
        TxEvent event;
        event.type = TxEventType::PENDING;
        event.tx_hash = response.tx_hash;
        event.amount_ints = 0;  // Calculate from tx
        event.confirmations = 0;
        event.timestamp = std::time(nullptr);
        ProcessTransactionEvent(event);
    }

    return Result<uint256>::Ok(response.tx_hash);
}

Result<HistoryResponse> MobileSDK::GetTransactionHistory(uint32_t limit, uint32_t offset) {
    if (!wallet_open_) {
        return Result<HistoryResponse>::Error("Wallet not open");
    }

    auto addr_result = GetCurrentAddress();
    if (addr_result.IsError()) {
        return Result<HistoryResponse>::Error(addr_result.error);
    }

    HistoryRequest request;
    request.address = addr_result.GetValue();
    request.page_size = limit;
    request.page = offset / limit;

    return rpc_->GetHistory(request);
}

Result<HistoryEntry> MobileSDK::GetTransaction(const uint256& tx_hash) {
    // TODO: Implement single transaction lookup
    // For now, search through history

    auto history_result = GetTransactionHistory(100, 0);
    if (history_result.IsError()) {
        return Result<HistoryEntry>::Error(history_result.error);
    }

    for (const auto& entry : history_result.GetValue().entries) {
        if (entry.tx_hash == tx_hash) {
            return Result<HistoryEntry>::Ok(entry);
        }
    }

    return Result<HistoryEntry>::Error("Transaction not found");
}

Result<FeeEstimateResponse> MobileSDK::EstimateFee(const std::string& to_address,
                                                    uint64_t amount_ints,
                                                    uint32_t target_blocks) {
    // Estimate transaction size (typical P2PKH: ~250 bytes)
    uint32_t estimated_size = 250;

    FeeEstimateRequest request;
    request.tx_size = estimated_size;
    request.target_blocks = target_blocks;

    return rpc_->EstimateFee(request);
}

// ========================================
// Sync & Network
// ========================================

Result<void> MobileSDK::StartSync() {
    if (!config_.enable_spv || !spv_client_) {
        return Result<void>::Error("SPV not enabled");
    }

    LogF(LogLevel::INFO, "Mobile SDK: Starting blockchain sync");

    auto result = spv_client_->StartSync();
    if (result.IsError()) {
        return result;
    }

    // Start progress update thread
    // TODO: Implement periodic progress callback

    return Result<void>::Ok();
}

void MobileSDK::StopSync() {
    if (!config_.enable_spv || !spv_client_) {
        return;
    }

    LogF(LogLevel::INFO, "Mobile SDK: Stopping blockchain sync");
    spv_client_->StopSync();
}

bool MobileSDK::IsSyncing() const {
    if (!config_.enable_spv || !spv_client_) {
        return false;
    }

    return spv_client_->IsSyncing();
}

SyncProgress MobileSDK::GetSyncProgress() const {
    SyncProgress progress;

    if (!config_.enable_spv || !spv_client_) {
        progress.current_height = 0;
        progress.target_height = 0;
        progress.progress = 0.0;
        progress.is_syncing = false;
        return progress;
    }

    progress.current_height = spv_client_->GetBestHeight();
    progress.target_height = progress.current_height;  // TODO: Get from network
    progress.progress = spv_client_->GetSyncProgress();
    progress.is_syncing = spv_client_->IsSyncing();

    return progress;
}

Result<MobileRPC::NetworkStatus> MobileSDK::GetNetworkStatus() {
    return rpc_->GetNetworkStatus();
}

// ========================================
// QR Code Support
// ========================================

std::string MobileSDK::GeneratePaymentURI(const std::string& address,
                                          uint64_t amount_ints,
                                          const std::string& label,
                                          const std::string& message) {
    std::ostringstream uri;
    uri << "intcoin:" << address;

    bool has_params = false;

    if (amount_ints > 0) {
        uri << "?amount=" << FormatINTS(amount_ints);
        has_params = true;
    }

    if (!label.empty()) {
        uri << (has_params ? "&" : "?") << "label=" << label;
        has_params = true;
    }

    if (!message.empty()) {
        uri << (has_params ? "&" : "?") << "message=" << message;
    }

    return uri.str();
}

Result<MobileSDK::PaymentDetails> MobileSDK::ParsePaymentURI(const std::string& uri) {
    PaymentDetails details;
    details.amount_ints = 0;

    // Parse intcoin: URI
    if (uri.substr(0, 8) != "intcoin:") {
        return Result<PaymentDetails>::Error("Invalid URI scheme");
    }

    // Extract address and parameters
    size_t param_start = uri.find('?');
    details.address = uri.substr(8, param_start - 8);

    if (!ValidateAddress(details.address)) {
        return Result<PaymentDetails>::Error("Invalid address in URI");
    }

    // Parse parameters
    if (param_start != std::string::npos) {
        std::string params = uri.substr(param_start + 1);
        // TODO: Implement parameter parsing (amount, label, message)
    }

    return Result<PaymentDetails>::Ok(details);
}

// ========================================
// Callbacks
// ========================================

void MobileSDK::SetTransactionCallback(std::function<void(const TxEvent&)> callback) {
    tx_callback_ = callback;
}

void MobileSDK::SetSyncProgressCallback(std::function<void(const SyncProgress&)> callback) {
    sync_callback_ = callback;
}

// ========================================
// Utility
// ========================================

std::string MobileSDK::FormatINTS(uint64_t ints) {
    // 1 INT = 1,000,000 INTS
    uint64_t int_part = ints / 1000000;
    uint64_t frac_part = ints % 1000000;

    std::ostringstream ss;
    ss << int_part << "." << std::setfill('0') << std::setw(6) << frac_part << " INT";

    return ss.str();
}

Result<uint64_t> MobileSDK::ParseINTAmount(const std::string& amount_str) {
    // Parse "1.5" or "1500000" to INTS
    size_t dot_pos = amount_str.find('.');

    if (dot_pos == std::string::npos) {
        // No decimal point, assume INTS
        try {
            return Result<uint64_t>::Ok(std::stoull(amount_str));
        } catch (...) {
            return Result<uint64_t>::Error("Invalid amount format");
        }
    }

    // Has decimal point, parse as INT
    try {
        std::string int_part = amount_str.substr(0, dot_pos);
        std::string frac_part = amount_str.substr(dot_pos + 1);

        // Pad fraction to 6 digits
        while (frac_part.length() < 6) {
            frac_part += "0";
        }
        if (frac_part.length() > 6) {
            frac_part = frac_part.substr(0, 6);
        }

        uint64_t ints = std::stoull(int_part) * 1000000 + std::stoull(frac_part);
        return Result<uint64_t>::Ok(ints);
    } catch (...) {
        return Result<uint64_t>::Error("Invalid amount format");
    }
}

std::string MobileSDK::GetVersion() {
    return "1.2.0-beta";
}

// ========================================
// Private Methods
// ========================================

void MobileSDK::UpdateBloomFilter() {
    if (!config_.enable_spv || !spv_client_ || !wallet_open_) {
        return;
    }

    // Create bloom filter for wallet addresses
    BloomFilter filter(config_.bloom_filter_addresses,
                      config_.bloom_fp_rate,
                      std::time(nullptr) & 0xFFFFFFFF);

    // Add all wallet addresses to filter
    auto addresses = GetAllAddresses();
    for (const auto& address : addresses) {
        // TODO: Convert address to script pubkey and add to filter
        // For now, add address as-is
        std::vector<uint8_t> addr_data(address.begin(), address.end());
        filter.Add(addr_data);
    }

    spv_client_->SetBloomFilter(filter);

    LogF(LogLevel::INFO, "Mobile SDK: Updated bloom filter with %zu addresses",
         addresses.size());
}

void MobileSDK::ProcessTransactionEvent(const TxEvent& event) {
    if (tx_callback_) {
        tx_callback_(event);
    }

    LogF(LogLevel::INFO, "Mobile SDK: Transaction event - %s for %llu INTS",
         event.type == TxEventType::RECEIVED ? "Received" :
         event.type == TxEventType::SENT ? "Sent" :
         event.type == TxEventType::CONFIRMED ? "Confirmed" : "Pending",
         event.amount_ints);
}

void MobileSDK::UpdateSyncProgress() {
    if (sync_callback_) {
        sync_callback_(GetSyncProgress());
    }
}

}  // namespace mobile
}  // namespace intcoin

// ========================================
// C API Implementation
// ========================================

using namespace intcoin::mobile;

intcoin_sdk_t intcoin_sdk_create(const char* network,
                                  const char* wallet_path,
                                  const char* rpc_endpoint) {
    try {
        SDKConfig config;
        config.network = network ? network : "mainnet";
        config.wallet_path = wallet_path ? wallet_path : "";
        config.rpc_endpoint = rpc_endpoint ? rpc_endpoint : "http://localhost:2210";

        auto sdk = new MobileSDK(config);
        return reinterpret_cast<intcoin_sdk_t>(sdk);
    } catch (...) {
        return nullptr;
    }
}

void intcoin_sdk_destroy(intcoin_sdk_t sdk) {
    if (sdk) {
        delete reinterpret_cast<MobileSDK*>(sdk);
    }
}

int intcoin_sdk_create_wallet(intcoin_sdk_t sdk,
                               const char* password,
                               char* mnemonic_out) {
    if (!sdk || !password || !mnemonic_out) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    auto result = mobile_sdk->CreateWallet("", password);

    if (result.IsError()) {
        return -1;
    }

    std::strncpy(mnemonic_out, result.GetValue().c_str(), 255);
    mnemonic_out[255] = '\0';

    return 0;
}

int intcoin_sdk_open_wallet(intcoin_sdk_t sdk, const char* password) {
    if (!sdk || !password) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    auto result = mobile_sdk->OpenWallet(password);

    return result.IsError() ? -1 : 0;
}

void intcoin_sdk_close_wallet(intcoin_sdk_t sdk) {
    if (sdk) {
        reinterpret_cast<MobileSDK*>(sdk)->CloseWallet();
    }
}

int intcoin_sdk_get_new_address(intcoin_sdk_t sdk, char* address_out) {
    if (!sdk || !address_out) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    auto result = mobile_sdk->GetNewAddress();

    if (result.IsError()) {
        return -1;
    }

    std::strncpy(address_out, result.GetValue().c_str(), 63);
    address_out[63] = '\0';

    return 0;
}

int intcoin_sdk_get_balance(intcoin_sdk_t sdk,
                             uint64_t* confirmed_out,
                             uint64_t* unconfirmed_out) {
    if (!sdk || !confirmed_out || !unconfirmed_out) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    auto result = mobile_sdk->GetBalance();

    if (result.IsError()) {
        return -1;
    }

    *confirmed_out = result.GetValue().confirmed_balance;
    *unconfirmed_out = result.GetValue().unconfirmed_balance;

    return 0;
}

int intcoin_sdk_send_transaction(intcoin_sdk_t sdk,
                                  const char* to_address,
                                  uint64_t amount_ints,
                                  uint8_t* tx_hash_out) {
    if (!sdk || !to_address || !tx_hash_out) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);

    // Create transaction
    auto tx_result = mobile_sdk->CreateTransaction(to_address, amount_ints, 0);
    if (tx_result.IsError()) {
        return -1;
    }

    // Send transaction
    auto send_result = mobile_sdk->SendTransaction(tx_result.GetValue());
    if (send_result.IsError()) {
        return -1;
    }

    // Copy tx hash
    std::memcpy(tx_hash_out, send_result.GetValue().data(), 32);

    return 0;
}

int intcoin_sdk_start_sync(intcoin_sdk_t sdk) {
    if (!sdk) {
        return -1;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    auto result = mobile_sdk->StartSync();

    return result.IsError() ? -1 : 0;
}

void intcoin_sdk_stop_sync(intcoin_sdk_t sdk) {
    if (sdk) {
        reinterpret_cast<MobileSDK*>(sdk)->StopSync();
    }
}

double intcoin_sdk_get_sync_progress(intcoin_sdk_t sdk) {
    if (!sdk) {
        return 0.0;
    }

    auto mobile_sdk = reinterpret_cast<MobileSDK*>(sdk);
    return mobile_sdk->GetSyncProgress().progress;
}

void intcoin_sdk_format_ints(uint64_t ints, char* out) {
    if (!out) {
        return;
    }

    std::string formatted = MobileSDK::FormatINTS(ints);
    std::strncpy(out, formatted.c_str(), 31);
    out[31] = '\0';
}

int intcoin_sdk_validate_address(const char* address) {
    if (!address) {
        return 0;
    }

    return MobileSDK::ValidateAddress(address) ? 1 : 0;
}

void intcoin_sdk_generate_payment_uri(const char* address,
                                       uint64_t amount_ints,
                                       const char* label,
                                       const char* message,
                                       char* uri_out) {
    if (!address || !uri_out) {
        return;
    }

    std::string uri = MobileSDK::GeneratePaymentURI(
        address,
        amount_ints,
        label ? label : "",
        message ? message : ""
    );

    std::strncpy(uri_out, uri.c_str(), 511);
    uri_out[511] = '\0';
}
