// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/mempool.h"
#include "intcoin/block.h"
#include <algorithm>
#include <chrono>

namespace intcoin {

Mempool::Mempool() {
}

bool Mempool::add_transaction(const Transaction& tx, uint32_t current_height) {
    Hash256 tx_hash = tx.get_hash();

    // Check if already in mempool
    if (has_transaction(tx_hash)) {
        return false;
    }

    // Validate transaction
    if (!validate_transaction(tx)) {
        return false;
    }

    // Check for conflicts
    if (check_conflicts(tx)) {
        return false;
    }

    // Check size
    if (tx.get_size() > MAX_TRANSACTION_SIZE) {
        return false;
    }

    // Calculate fee
    uint64_t fee = tx.get_fee();
    uint64_t fee_rate = tx.get_size() > 0 ? fee / tx.get_size() : 0;

    // Check minimum fee
    if (fee_rate < MIN_RELAY_FEE_RATE) {
        return false;
    }

    // Check mempool size limit
    size_t tx_size = tx.get_size();
    if (total_size_bytes() + tx_size > MAX_MEMPOOL_SIZE) {
        // Evict lower fee transactions to make room
        uint64_t fee_rate = fee * 1000 / tx_size;  // satoshis per kilobyte

        // Find transactions with lower fee rate
        std::vector<Hash256> to_evict;
        size_t bytes_to_free = (total_size_bytes() + tx_size) - MAX_MEMPOOL_SIZE;
        size_t bytes_freed = 0;

        for (const auto& [existing_hash, existing_entry] : transactions_) {
            uint64_t existing_fee_rate = existing_entry.fee * 1000 / existing_entry.tx.get_size();

            // Only evict if existing transaction has lower fee rate
            if (existing_fee_rate < fee_rate) {
                to_evict.push_back(existing_hash);
                bytes_freed += existing_entry.tx.get_size();

                if (bytes_freed >= bytes_to_free) {
                    break;
                }
            }
        }

        // If we can't free enough space, reject the new transaction
        if (bytes_freed < bytes_to_free) {
            return false;
        }

        // Evict the lower-fee transactions
        for (const auto& hash : to_evict) {
            remove_transaction(hash);
        }
    }

    // Create entry
    MempoolEntry entry(tx, fee, current_height);

    // Add to mempool
    transactions_[tx_hash] = entry;
    priority_queue_.insert(entry);
    add_spent_outputs(tx);

    // Update cached size
    cached_total_size_ += entry.size;

    return true;
}

void Mempool::remove_transaction(const Hash256& tx_hash) {
    auto it = transactions_.find(tx_hash);
    if (it == transactions_.end()) {
        return;
    }

    // Update cached size before removal
    cached_total_size_ -= it->second.size;

    // Remove from priority queue
    priority_queue_.erase(it->second);

    // Remove spent outputs
    remove_spent_outputs(it->second.tx);

    // Remove from main map
    transactions_.erase(it);
}

void Mempool::remove_block_transactions(const Block& block) {
    for (const auto& tx : block.transactions) {
        remove_transaction(tx.get_hash());
    }
}

std::optional<Transaction> Mempool::get_transaction(const Hash256& tx_hash) const {
    auto it = transactions_.find(tx_hash);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    return it->second.tx;
}

bool Mempool::has_transaction(const Hash256& tx_hash) const {
    return transactions_.find(tx_hash) != transactions_.end();
}

std::vector<Transaction> Mempool::get_transactions_for_mining(size_t max_count, size_t max_size) const {
    std::vector<Transaction> txs;
    size_t total_size = 0;

    for (const auto& entry : priority_queue_) {
        if (txs.size() >= max_count) break;
        if (total_size + entry.size > max_size) break;

        txs.push_back(entry.tx);
        total_size += entry.size;
    }

    return txs;
}

std::vector<Transaction> Mempool::get_all_transactions() const {
    std::vector<Transaction> txs;
    txs.reserve(transactions_.size());

    for (const auto& [hash, entry] : transactions_) {
        txs.push_back(entry.tx);
    }

    return txs;
}

size_t Mempool::total_size_bytes() const {
    // Return cached size - O(1) instead of O(n)
    return cached_total_size_;
}

uint64_t Mempool::total_fees() const {
    uint64_t total = 0;
    for (const auto& [hash, entry] : transactions_) {
        total += entry.fee;
    }
    return total;
}

void Mempool::remove_expired_transactions(uint64_t max_age_seconds) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    std::vector<Hash256> to_remove;
    for (const auto& [hash, entry] : transactions_) {
        uint64_t age = (now - entry.time_added) / 1000000000;  // Convert to seconds
        if (age > max_age_seconds) {
            to_remove.push_back(hash);
        }
    }

    for (const auto& hash : to_remove) {
        remove_transaction(hash);
    }
}

void Mempool::clear() {
    transactions_.clear();
    priority_queue_.clear();
    spent_outputs_.clear();
    cached_total_size_ = 0;  // Reset cached size
}

bool Mempool::validate_transaction(const Transaction& tx) const {
    // Basic structure validation
    if (!tx.validate_structure()) {
        return false;
    }

    // Coinbase transactions can't be in mempool
    if (tx.is_coinbase()) {
        return false;
    }

    // Check outputs are not dust
    for (const auto& output : tx.outputs) {
        if (output.is_dust()) {
            return false;
        }
    }

    return true;
}

std::vector<Hash256> Mempool::get_transaction_dependencies(const Hash256& tx_hash) const {
    std::vector<Hash256> deps;

    auto it = transactions_.find(tx_hash);
    if (it == transactions_.end()) {
        return deps;
    }

    // Find transactions that spend outputs from this transaction
    for (const auto& [hash, entry] : transactions_) {
        for (const auto& input : entry.tx.inputs) {
            if (input.previous_output.tx_hash == tx_hash) {
                deps.push_back(hash);
                break;
            }
        }
    }

    return deps;
}

bool Mempool::check_conflicts(const Transaction& tx) const {
    // Check if any inputs are already spent in mempool
    for (const auto& input : tx.inputs) {
        if (spent_outputs_.find(input.previous_output) != spent_outputs_.end()) {
            return true;  // Conflict found
        }
    }
    return false;
}

void Mempool::add_spent_outputs(const Transaction& tx) {
    Hash256 tx_hash = tx.get_hash();
    for (const auto& input : tx.inputs) {
        spent_outputs_[input.previous_output] = tx_hash;
    }
}

void Mempool::remove_spent_outputs(const Transaction& tx) {
    for (const auto& input : tx.inputs) {
        spent_outputs_.erase(input.previous_output);
    }
}

} // namespace intcoin
