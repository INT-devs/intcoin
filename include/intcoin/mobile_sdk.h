// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_MOBILE_SDK_H
#define INTCOIN_MOBILE_SDK_H

#include <intcoin/bloom.h>
#include <intcoin/mobile_rpc.h>
#include <intcoin/spv.h>
#include <intcoin/transaction.h>
#include <intcoin/types.h>
#include <intcoin/wallet.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace intcoin {
namespace mobile {

/// SDK configuration options
struct SDKConfig {
    /// Network type (mainnet, testnet)
    std::string network = "mainnet";

    /// Full node RPC endpoint
    std::string rpc_endpoint = "http://localhost:2210";

    /// Path to wallet storage
    std::string wallet_path;

    /// Enable SPV mode (lightweight sync)
    bool enable_spv = true;

    /// Bloom filter false positive rate (0.0001 = 0.01%)
    double bloom_fp_rate = 0.0001;

    /// Number of addresses to watch in bloom filter
    uint32_t bloom_filter_addresses = 100;
};

/// Transaction event types
enum class TxEventType {
    RECEIVED,   // Funds received
    SENT,       // Funds sent
    CONFIRMED,  // Transaction confirmed
    PENDING     // Transaction pending
};

/// Transaction event callback
struct TxEvent {
    TxEventType type;
    uint256 tx_hash;
    std::string address;
    uint64_t amount_ints;  // Amount in INTS
    uint32_t confirmations;
    uint64_t timestamp;
};

/// Sync progress callback
struct SyncProgress {
    uint64_t current_height;
    uint64_t target_height;
    double progress;  // 0.0 to 1.0
    bool is_syncing;
};

/// Mobile SDK for INTcoin lightweight wallet clients
/// Provides high-level API for mobile wallet applications
class MobileSDK {
public:
    /// Constructor
    /// @param config SDK configuration
    MobileSDK(const SDKConfig& config);

    /// Destructor
    ~MobileSDK();

    // ========================================
    // Wallet Management
    // ========================================

    /// Create new wallet with mnemonic seed
    /// @param mnemonic BIP39 mnemonic phrase (leave empty to generate)
    /// @param password Wallet encryption password
    /// @return Result with mnemonic phrase if successful
    Result<std::string> CreateWallet(const std::string& mnemonic, const std::string& password);

    /// Open existing wallet
    /// @param password Wallet encryption password
    /// @return Success/failure result
    Result<void> OpenWallet(const std::string& password);

    /// Close wallet and cleanup
    void CloseWallet();

    /// Check if wallet is open
    /// @return True if wallet is open
    bool IsWalletOpen() const;

    /// Backup wallet to encrypted data
    /// @return Encrypted wallet backup data
    Result<std::vector<uint8_t>> BackupWallet();

    /// Restore wallet from backup
    /// @param backup_data Encrypted wallet backup
    /// @param password Wallet password
    /// @return Success/failure result
    Result<void> RestoreWallet(const std::vector<uint8_t>& backup_data, const std::string& password);

    // ========================================
    // Address Management
    // ========================================

    /// Generate new receiving address
    /// @return INTcoin Bech32 address
    Result<std::string> GetNewAddress();

    /// Get current receiving address
    /// @return INTcoin Bech32 address
    Result<std::string> GetCurrentAddress();

    /// Get all wallet addresses
    /// @return Vector of addresses
    std::vector<std::string> GetAllAddresses();

    /// Validate INTcoin address format
    /// @param address Address to validate
    /// @return True if valid
    static bool ValidateAddress(const std::string& address);

    // ========================================
    // Balance & UTXO Management
    // ========================================

    /// Get wallet balance
    /// @return Balance in INTS (1 INT = 1,000,000 INTS)
    Result<BalanceResponse> GetBalance();

    /// Get UTXOs for wallet
    /// @param min_confirmations Minimum confirmations required
    /// @return UTXOs available for spending
    Result<UTXOResponse> GetUTXOs(uint32_t min_confirmations = 1);

    // ========================================
    // Transaction Management
    // ========================================

    /// Create and sign transaction
    /// @param to_address Recipient address
    /// @param amount_ints Amount in INTS
    /// @param fee_rate Fee rate in INTS per KB (0 = auto estimate)
    /// @return Signed transaction ready to broadcast
    Result<Transaction> CreateTransaction(const std::string& to_address,
                                          uint64_t amount_ints,
                                          uint64_t fee_rate = 0);

    /// Broadcast transaction to network
    /// @param tx Transaction to broadcast
    /// @return Result with transaction hash
    Result<uint256> SendTransaction(const Transaction& tx);

    /// Get transaction history
    /// @param limit Maximum number of transactions
    /// @param offset Offset for pagination
    /// @return Transaction history
    Result<HistoryResponse> GetTransactionHistory(uint32_t limit = 50, uint32_t offset = 0);

    /// Get transaction by hash
    /// @param tx_hash Transaction hash
    /// @return Transaction details
    Result<HistoryEntry> GetTransaction(const uint256& tx_hash);

    /// Estimate transaction fee
    /// @param to_address Recipient address
    /// @param amount_ints Amount in INTS
    /// @param target_blocks Confirmation target
    /// @return Fee estimate
    Result<FeeEstimateResponse> EstimateFee(const std::string& to_address,
                                            uint64_t amount_ints,
                                            uint32_t target_blocks = 6);

    // ========================================
    // Sync & Network
    // ========================================

    /// Start blockchain sync
    /// @return Success/failure result
    Result<void> StartSync();

    /// Stop blockchain sync
    void StopSync();

    /// Check if syncing
    /// @return True if sync in progress
    bool IsSyncing() const;

    /// Get sync progress
    /// @return Current sync status
    SyncProgress GetSyncProgress() const;

    /// Get network status
    /// @return Network information
    Result<MobileRPC::NetworkStatus> GetNetworkStatus();

    // ========================================
    // QR Code Support
    // ========================================

    /// Generate payment URI for QR code
    /// @param address Receiving address
    /// @param amount_ints Amount in INTS (optional)
    /// @param label Payment label (optional)
    /// @param message Payment message (optional)
    /// @return intcoin: URI for QR code
    static std::string GeneratePaymentURI(const std::string& address,
                                         uint64_t amount_ints = 0,
                                         const std::string& label = "",
                                         const std::string& message = "");

    /// Parse payment URI from QR code
    /// @param uri Payment URI
    /// @return Parsed payment details
    struct PaymentDetails {
        std::string address;
        uint64_t amount_ints;
        std::string label;
        std::string message;
    };
    static Result<PaymentDetails> ParsePaymentURI(const std::string& uri);

    // ========================================
    // Callbacks
    // ========================================

    /// Set transaction event callback
    /// @param callback Function to call on transaction events
    void SetTransactionCallback(std::function<void(const TxEvent&)> callback);

    /// Set sync progress callback
    /// @param callback Function to call on sync progress updates
    void SetSyncProgressCallback(std::function<void(const SyncProgress&)> callback);

    // ========================================
    // Utility
    // ========================================

    /// Convert INTS to INT (human-readable)
    /// @param ints Amount in INTS
    /// @return Formatted string (e.g., "1.234567 INT")
    static std::string FormatINTS(uint64_t ints);

    /// Parse INT amount to INTS
    /// @param amount_str Amount string (e.g., "1.5" or "1500000")
    /// @return Amount in INTS
    static Result<uint64_t> ParseINTAmount(const std::string& amount_str);

    /// Get SDK version
    /// @return SDK version string
    static std::string GetVersion();

private:
    /// SDK configuration
    SDKConfig config_;

    /// Wallet instance
    std::shared_ptr<wallet::Wallet> wallet_;

    /// SPV client for lightweight sync
    std::shared_ptr<SPVClient> spv_client_;

    /// Mobile RPC handler
    std::shared_ptr<MobileRPC> rpc_;

    /// Database backend
    std::shared_ptr<BlockchainDB> db_;

    /// Transaction event callback
    std::function<void(const TxEvent&)> tx_callback_;

    /// Sync progress callback
    std::function<void(const SyncProgress&)> sync_callback_;

    /// Wallet open state
    bool wallet_open_;

    /// Update bloom filter with wallet addresses
    void UpdateBloomFilter();

    /// Process transaction event
    void ProcessTransactionEvent(const TxEvent& event);

    /// Update sync progress
    void UpdateSyncProgress();
};

}  // namespace mobile
}  // namespace intcoin

// ========================================
// C API for Platform Bindings
// ========================================

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque handle to SDK instance
typedef void* intcoin_sdk_t;

/// Create SDK instance
/// @param network "mainnet" or "testnet"
/// @param wallet_path Path to wallet storage
/// @param rpc_endpoint RPC endpoint URL
/// @return SDK handle or NULL on error
intcoin_sdk_t intcoin_sdk_create(const char* network,
                                  const char* wallet_path,
                                  const char* rpc_endpoint);

/// Destroy SDK instance
/// @param sdk SDK handle
void intcoin_sdk_destroy(intcoin_sdk_t sdk);

/// Create new wallet
/// @param sdk SDK handle
/// @param password Wallet password
/// @param mnemonic_out Output buffer for mnemonic (min 256 bytes)
/// @return 0 on success, error code otherwise
int intcoin_sdk_create_wallet(intcoin_sdk_t sdk,
                               const char* password,
                               char* mnemonic_out);

/// Open existing wallet
/// @param sdk SDK handle
/// @param password Wallet password
/// @return 0 on success, error code otherwise
int intcoin_sdk_open_wallet(intcoin_sdk_t sdk, const char* password);

/// Close wallet
/// @param sdk SDK handle
void intcoin_sdk_close_wallet(intcoin_sdk_t sdk);

/// Get new address
/// @param sdk SDK handle
/// @param address_out Output buffer for address (min 64 bytes)
/// @return 0 on success, error code otherwise
int intcoin_sdk_get_new_address(intcoin_sdk_t sdk, char* address_out);

/// Get balance in INTS
/// @param sdk SDK handle
/// @param confirmed_out Confirmed balance output
/// @param unconfirmed_out Unconfirmed balance output
/// @return 0 on success, error code otherwise
int intcoin_sdk_get_balance(intcoin_sdk_t sdk,
                             uint64_t* confirmed_out,
                             uint64_t* unconfirmed_out);

/// Send transaction
/// @param sdk SDK handle
/// @param to_address Recipient address
/// @param amount_ints Amount in INTS
/// @param tx_hash_out Output buffer for tx hash (32 bytes)
/// @return 0 on success, error code otherwise
int intcoin_sdk_send_transaction(intcoin_sdk_t sdk,
                                  const char* to_address,
                                  uint64_t amount_ints,
                                  uint8_t* tx_hash_out);

/// Start sync
/// @param sdk SDK handle
/// @return 0 on success, error code otherwise
int intcoin_sdk_start_sync(intcoin_sdk_t sdk);

/// Stop sync
/// @param sdk SDK handle
void intcoin_sdk_stop_sync(intcoin_sdk_t sdk);

/// Get sync progress (0.0 to 1.0)
/// @param sdk SDK handle
/// @return Sync progress
double intcoin_sdk_get_sync_progress(intcoin_sdk_t sdk);

/// Format INTS to human-readable string
/// @param ints Amount in INTS
/// @param out Output buffer (min 32 bytes)
void intcoin_sdk_format_ints(uint64_t ints, char* out);

/// Validate address
/// @param address Address to validate
/// @return 1 if valid, 0 otherwise
int intcoin_sdk_validate_address(const char* address);

/// Generate payment URI
/// @param address Receiving address
/// @param amount_ints Amount (0 for no amount)
/// @param label Label (NULL for none)
/// @param message Message (NULL for none)
/// @param uri_out Output buffer (min 512 bytes)
void intcoin_sdk_generate_payment_uri(const char* address,
                                       uint64_t amount_ints,
                                       const char* label,
                                       const char* message,
                                       char* uri_out);

#ifdef __cplusplus
}
#endif

#endif  // INTCOIN_MOBILE_SDK_H
