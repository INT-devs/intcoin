// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Transaction memory pool (mempool) for unconfirmed transactions.

#ifndef INTCOIN_MEMPOOL_H
#define INTCOIN_MEMPOOL_H

#include "transaction.h"
#include "primitives.h"
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <optional>
#include <chrono>

namespace intcoin {

/**
 * Transaction entry in mempool
 */
struct MempoolEntry {
    Transaction tx;
    uint64_t fee;
    uint64_t fee_rate;      // Satoshis per byte
    uint64_t time_added;
    uint32_t height;        // Height when added
    size_t size;

    MempoolEntry() : fee(0), fee_rate(0), time_added(0), height(0), size(0) {}

    MempoolEntry(const Transaction& transaction, uint64_t tx_fee, uint32_t block_height)
        : tx(transaction)
        , fee(tx_fee)
        , time_added(std::chrono::system_clock::now().time_since_epoch().count())
        , height(block_height)
        , size(transaction.get_size())
    {
        fee_rate = size > 0 ? fee / size : 0;
    }

    bool operator<(const MempoolEntry& other) const {
        // Higher fee rate = higher priority
        return fee_rate > other.fee_rate;
    }
};

/**
 * Memory pool for unconfirmed transactions
 */
class Mempool {
public:
    Mempool();

    // Add transaction to mempool
    bool add_transaction(const Transaction& tx, uint32_t current_height);

    // Remove transaction from mempool
    void remove_transaction(const Hash256& tx_hash);

    // Remove transactions that were included in a block
    void remove_block_transactions(const Block& block);

    // Get transaction from mempool
    std::optional<Transaction> get_transaction(const Hash256& tx_hash) const;

    // Check if transaction exists in mempool
    bool has_transaction(const Hash256& tx_hash) const;

    // Get transactions for block mining (sorted by fee rate)
    std::vector<Transaction> get_transactions_for_mining(size_t max_count, size_t max_size) const;

    // Get all transactions
    std::vector<Transaction> get_all_transactions() const;

    // Statistics
    size_t size() const { return transactions_.size(); }
    size_t total_size_bytes() const;
    uint64_t total_fees() const;

    // Cleanup
    void remove_expired_transactions(uint64_t max_age_seconds);
    void clear();

    // Validation
    bool validate_transaction(const Transaction& tx) const;

    // Dependencies
    std::vector<Hash256> get_transaction_dependencies(const Hash256& tx_hash) const;

private:
    // Map: tx_hash -> entry
    std::unordered_map<Hash256, MempoolEntry> transactions_;

    // Priority queue for mining (sorted by fee rate)
    std::multiset<MempoolEntry> priority_queue_;

    // Map: spent_outpoint -> tx_hash (for detecting conflicts)
    std::unordered_map<OutPoint, Hash256> spent_outputs_;

    // Configuration
    static constexpr size_t MAX_MEMPOOL_SIZE = 300 * 1024 * 1024;  // 300 MB
    static constexpr size_t MAX_TRANSACTION_SIZE = 100 * 1024;      // 100 KB
    static constexpr uint64_t MIN_RELAY_FEE_RATE = 1;               // 1 sat/byte minimum

    // Internal helpers
    bool check_conflicts(const Transaction& tx) const;
    void add_spent_outputs(const Transaction& tx);
    void remove_spent_outputs(const Transaction& tx);
};

/**
 * Mempool statistics
 */
struct MempoolStats {
    size_t transaction_count;
    size_t total_bytes;
    uint64_t total_fees;
    uint64_t min_fee_rate;
    uint64_t median_fee_rate;
    uint64_t max_fee_rate;

    MempoolStats()
        : transaction_count(0)
        , total_bytes(0)
        , total_fees(0)
        , min_fee_rate(0)
        , median_fee_rate(0)
        , max_fee_rate(0)
    {}
};

} // namespace intcoin

#endif // INTCOIN_MEMPOOL_H
