// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_MEMPOOL_H
#define INTCOIN_MEMPOOL_H

#include <intcoin/blockchain.h>
#include <intcoin/util.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <cstdint>
#include <ctime>

namespace intcoin {

/// Transaction priority class
enum class TxPriority : uint8_t {
    LOW = 0,        // Standard transactions
    NORMAL = 1,     // Normal priority
    HIGH = 2,       // High fee transactions
    HTLC = 3,       // HTLC transactions (atomic swaps)
    BRIDGE = 4,     // Bridge transactions (deposits/withdrawals)
    CRITICAL = 5    // Critical system transactions
};

/// Mempool entry
struct MempoolEntry {
    Transaction tx;
    uint256 tx_hash;
    TxPriority priority;
    uint64_t fee;
    uint64_t fee_per_byte;
    uint64_t size_bytes;
    std::time_t added_time;
    uint32_t height_added;

    // Dependencies
    std::set<uint256> depends_on;  // Parent transactions this depends on
    std::set<uint256> depended_by; // Child transactions depending on this

    // Statistics
    uint32_t broadcast_count;
    std::time_t last_broadcast;
};

/// Mempool statistics
struct MempoolStats {
    uint64_t total_transactions;
    uint64_t total_size_bytes;
    uint64_t total_fees;

    // By priority
    std::map<TxPriority, uint64_t> count_by_priority;
    std::map<TxPriority, uint64_t> size_by_priority;

    // Rates
    double avg_fee_per_byte;
    uint64_t min_fee;
    uint64_t max_fee;

    // Resource usage
    uint64_t memory_usage_bytes;
    uint32_t orphan_count;
};

/// Mempool configuration
struct MempoolConfig {
    uint64_t max_size_mb = 300;                    // Max mempool size in MB
    uint64_t min_relay_fee_per_kb = 1000;          // Min fee to relay (ints/KB)
    uint64_t max_orphan_tx = 100;                  // Max orphan transactions
    uint32_t expiry_hours = 72;                    // Expire transactions after 72h
    bool persist_on_shutdown = true;               // Save mempool to disk
    std::string persist_file = "mempool.dat";      // Persistence file path

    // Priority limits (max transactions per priority level)
    std::map<TxPriority, uint32_t> priority_limits = {
        {TxPriority::LOW, 10000},
        {TxPriority::NORMAL, 20000},
        {TxPriority::HIGH, 5000},
        {TxPriority::HTLC, 2000},
        {TxPriority::BRIDGE, 1000},
        {TxPriority::CRITICAL, 500}
    };
};

/// Enhanced mempool interface (avoids conflict with basic Mempool in storage.h)
class IMempoolInterface {
public:
    virtual ~IMempoolInterface() = default;

    /// Initialize mempool
    virtual Result<void> Initialize(const MempoolConfig& config) = 0;

    /// Shutdown mempool (persists if configured)
    virtual Result<void> Shutdown() = 0;

    /// Add transaction to mempool
    virtual Result<void> AddTransaction(
        const Transaction& tx,
        TxPriority priority = TxPriority::NORMAL
    ) = 0;

    /// Remove transaction from mempool
    virtual Result<void> RemoveTransaction(const uint256& tx_hash) = 0;

    /// Check if transaction exists in mempool
    virtual bool HasTransaction(const uint256& tx_hash) const = 0;

    /// Get transaction from mempool
    virtual Result<Transaction> GetTransaction(const uint256& tx_hash) const = 0;

    /// Get mempool entry (with metadata)
    virtual Result<MempoolEntry> GetEntry(const uint256& tx_hash) const = 0;

    /// Get all transactions (ordered by priority and fee)
    virtual std::vector<MempoolEntry> GetAllTransactions() const = 0;

    /// Get transactions for block template (prioritized)
    virtual std::vector<Transaction> GetBlockTemplate(
        uint64_t max_size_bytes,
        uint64_t max_count = 0
    ) const = 0;

    /// Remove transactions that are now confirmed in blocks
    virtual Result<uint32_t> RemoveConfirmedTransactions(
        const std::vector<uint256>& tx_hashes
    ) = 0;

    /// Remove expired transactions
    virtual Result<uint32_t> RemoveExpired() = 0;

    /// Get mempool statistics
    virtual MempoolStats GetStats() const = 0;

    /// Estimate fee for priority level
    virtual Result<uint64_t> EstimateFee(
        TxPriority priority,
        uint64_t size_bytes
    ) const = 0;

    /// Persist mempool to disk
    virtual Result<void> Persist() const = 0;

    /// Restore mempool from disk
    virtual Result<void> Restore() = 0;

    /// Clear all transactions
    virtual Result<void> Clear() = 0;
};

/// INTcoin enhanced mempool implementation with priority queues
class INTcoinMempool : public IMempoolInterface {
public:
    INTcoinMempool();
    ~INTcoinMempool() override;

    Result<void> Initialize(const MempoolConfig& config) override;
    Result<void> Shutdown() override;
    Result<void> AddTransaction(const Transaction& tx, TxPriority priority) override;
    Result<void> RemoveTransaction(const uint256& tx_hash) override;
    bool HasTransaction(const uint256& tx_hash) const override;
    Result<Transaction> GetTransaction(const uint256& tx_hash) const override;
    Result<MempoolEntry> GetEntry(const uint256& tx_hash) const override;
    std::vector<MempoolEntry> GetAllTransactions() const override;
    std::vector<Transaction> GetBlockTemplate(uint64_t max_size_bytes, uint64_t max_count) const override;
    Result<uint32_t> RemoveConfirmedTransactions(const std::vector<uint256>& tx_hashes) override;
    Result<uint32_t> RemoveExpired() override;
    MempoolStats GetStats() const override;
    Result<uint64_t> EstimateFee(TxPriority priority, uint64_t size_bytes) const override;
    Result<void> Persist() const override;
    Result<void> Restore() override;
    Result<void> Clear() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Internal helpers
    TxPriority DeterminePriority(const Transaction& tx, uint64_t fee_per_byte) const;
    bool ValidateTransaction(const Transaction& tx) const;
    void EvictLowPriority();
    uint64_t CalculateTxSize(const Transaction& tx) const;
    std::vector<MempoolEntry> GetAllTransactionsInternal() const;  // Internal helper, caller must hold mutex
    Result<void> RemoveTransactionInternal(const uint256& tx_hash);  // Internal helper, caller must hold mutex
    Result<void> AddContractTransaction(const Transaction& tx, TxPriority priority);  // Internal helper for contract txs
};

/// Helper functions
std::string TxPriorityToString(TxPriority priority);
TxPriority StringToTxPriority(const std::string& str);

} // namespace intcoin

#endif // INTCOIN_MEMPOOL_H
