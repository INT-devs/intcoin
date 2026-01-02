// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_MOBILE_RPC_H
#define INTCOIN_MOBILE_RPC_H

#include <intcoin/bloom.h>
#include <intcoin/spv.h>
#include <intcoin/transaction.h>
#include <intcoin/types.h>
#include <intcoin/wallet.h>

#include <cstdint>
#include <string>
#include <vector>

namespace intcoin {
namespace mobile {

/// Mobile-optimized RPC methods for lightweight INTcoin wallet clients
/// Designed for minimal bandwidth usage and battery efficiency

// ============================================================================
// Request/Response Structures
// ============================================================================

/// Mobile sync request
struct SyncRequest {
    /// Last known block hash (empty for initial sync)
    uint256 last_block_hash;

    /// Bloom filter for transaction filtering
    BloomFilter filter;

    /// Maximum number of headers to return
    uint32_t max_headers;

    /// Default constructor
    SyncRequest() : max_headers(2000) {}
};

/// Mobile sync response
struct SyncResponse {
    /// Block headers (up to max_headers)
    std::vector<BlockHeader> headers;

    /// Filtered transactions matching bloom filter
    std::vector<Transaction> transactions;

    /// Merkle proofs for filtered transactions
    std::vector<std::pair<uint256, std::vector<uint256>>> merkle_proofs;

    /// Current best block height
    uint64_t best_height;

    /// Current best block hash
    uint256 best_hash;

    /// Estimated network fee rate (INTS per kilobyte)
    uint64_t fee_rate;
};

/// Balance request
struct BalanceRequest {
    /// Address to check balance for (INTcoin Bech32 format)
    std::string address;

    /// Minimum confirmations required
    uint32_t min_confirmations;

    /// Default constructor
    BalanceRequest() : min_confirmations(1) {}
};

/// Balance response
struct BalanceResponse {
    /// Confirmed balance in INTS (1 INT = 1,000,000 INTS)
    uint64_t confirmed_balance;

    /// Unconfirmed balance in INTS
    uint64_t unconfirmed_balance;

    /// Total balance (confirmed + unconfirmed) in INTS
    uint64_t total_balance;

    /// Number of UTXOs
    uint32_t utxo_count;
};

/// Transaction history request
struct HistoryRequest {
    /// Address to get history for
    std::string address;

    /// Page number (0-indexed)
    uint32_t page;

    /// Items per page
    uint32_t page_size;

    /// Include unconfirmed transactions
    bool include_unconfirmed;

    /// Default constructor
    HistoryRequest() : page(0), page_size(20), include_unconfirmed(true) {}
};

/// Transaction history entry
struct HistoryEntry {
    /// Transaction hash
    uint256 tx_hash;

    /// Block height (0 if unconfirmed)
    uint64_t block_height;

    /// Block hash (empty if unconfirmed)
    uint256 block_hash;

    /// Timestamp (Unix time)
    uint64_t timestamp;

    /// Amount in INTS (positive for received, negative for sent)
    int64_t amount;

    /// Fee in INTS (for sent transactions)
    uint64_t fee;

    /// Number of confirmations
    uint32_t confirmations;

    /// Transaction type ("received", "sent", "self")
    std::string type;
};

/// Transaction history response
struct HistoryResponse {
    /// Transaction history entries
    std::vector<HistoryEntry> entries;

    /// Total number of transactions (for pagination)
    uint32_t total_count;

    /// Current page
    uint32_t page;

    /// Total pages
    uint32_t total_pages;
};

/// Send transaction request
struct SendTransactionRequest {
    /// Signed transaction (serialized)
    std::vector<uint8_t> raw_transaction;

    /// Optional: return merkle proof after confirmation
    bool return_merkle_proof;

    /// Default constructor
    SendTransactionRequest() : return_merkle_proof(false) {}
};

/// Send transaction response
struct SendTransactionResponse {
    /// Transaction hash
    uint256 tx_hash;

    /// Whether transaction was accepted to mempool
    bool accepted;

    /// Error message (if not accepted)
    std::string error;

    /// Estimated confirmation time (seconds)
    uint32_t estimated_confirmation;
};

/// UTXO request
struct UTXORequest {
    /// Address to get UTXOs for
    std::string address;

    /// Minimum confirmations
    uint32_t min_confirmations;

    /// Maximum number of UTXOs to return
    uint32_t max_utxos;

    /// Default constructor
    UTXORequest() : min_confirmations(1), max_utxos(100) {}
};

/// UTXO entry
struct UTXOEntry {
    /// Transaction hash
    uint256 tx_hash;

    /// Output index
    uint32_t output_index;

    /// Amount in INTS
    uint64_t amount;

    /// Script pubkey
    Script script_pubkey;

    /// Block height
    uint64_t block_height;

    /// Confirmations
    uint32_t confirmations;
};

/// UTXO response
struct UTXOResponse {
    /// UTXOs
    std::vector<UTXOEntry> utxos;

    /// Total balance of returned UTXOs
    uint64_t total_amount;
};

/// Fee estimation request
struct FeeEstimateRequest {
    /// Target confirmation blocks
    uint32_t target_blocks;

    /// Transaction size in bytes (for estimation)
    uint32_t tx_size;

    /// Default constructor
    FeeEstimateRequest() : target_blocks(6), tx_size(250) {}
};

/// Fee estimation response
struct FeeEstimateResponse {
    /// Fee rate in INTS per kilobyte
    uint64_t fee_rate;

    /// Estimated fee for transaction in INTS
    uint64_t estimated_fee;

    /// Confidence level (0.0 - 1.0)
    double confidence;
};

// ============================================================================
// Mobile RPC Handler
// ============================================================================

/// Mobile-optimized RPC handler for INTcoin lightweight clients
class MobileRPC {
public:
    /// Constructor
    /// @param spv_client SPV client instance
    /// @param wallet Wallet instance (optional)
    MobileRPC(std::shared_ptr<SPVClient> spv_client,
              std::shared_ptr<wallet::Wallet> wallet = nullptr);

    /// Sync headers and get filtered transactions
    /// @param request Sync request with bloom filter
    /// @return Sync response with headers and transactions
    Result<SyncResponse> Sync(const SyncRequest& request);

    /// Get balance for address
    /// @param request Balance request
    /// @return Balance response
    Result<BalanceResponse> GetBalance(const BalanceRequest& request);

    /// Get transaction history
    /// @param request History request
    /// @return History response with paginated entries
    Result<HistoryResponse> GetHistory(const HistoryRequest& request);

    /// Send transaction to network
    /// @param request Send transaction request
    /// @return Send transaction response
    Result<SendTransactionResponse> SendTransaction(const SendTransactionRequest& request);

    /// Get UTXOs for address
    /// @param request UTXO request
    /// @return UTXO response
    Result<UTXOResponse> GetUTXOs(const UTXORequest& request);

    /// Estimate transaction fee
    /// @param request Fee estimate request
    /// @return Fee estimate response
    Result<FeeEstimateResponse> EstimateFee(const FeeEstimateRequest& request);

    /// Get current network status
    /// @return Network status (height, hash, peer count)
    struct NetworkStatus {
        uint64_t block_height;
        uint256 block_hash;
        uint32_t peer_count;
        bool is_syncing;
        double sync_progress;
    };
    Result<NetworkStatus> GetNetworkStatus();

private:
    /// SPV client
    std::shared_ptr<SPVClient> spv_client_;

    /// Wallet (optional)
    std::shared_ptr<wallet::Wallet> wallet_;

    /// Calculate transaction fee
    uint64_t CalculateTransactionFee(const Transaction& tx);

    /// Get confirmations for block height
    uint32_t GetConfirmations(uint64_t block_height);
};

}  // namespace mobile
}  // namespace intcoin

#endif  // INTCOIN_MOBILE_RPC_H
