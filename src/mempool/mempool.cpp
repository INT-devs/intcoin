// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/mempool.h>
#include <intcoin/blockchain.h>
#include <intcoin/util.h>
#include <intcoin/contracts/transaction.h>
#include <intcoin/contracts/validator.h>
#include <intcoin/crypto.h>

#include <unordered_map>
#include <set>
#include <queue>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <ctime>

namespace intcoin {

// Helper functions
std::string TxPriorityToString(TxPriority priority) {
    switch (priority) {
        case TxPriority::LOW:      return "LOW";
        case TxPriority::NORMAL:   return "NORMAL";
        case TxPriority::HIGH:     return "HIGH";
        case TxPriority::HTLC:     return "HTLC";
        case TxPriority::BRIDGE:   return "BRIDGE";
        case TxPriority::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

TxPriority StringToTxPriority(const std::string& str) {
    if (str == "LOW") return TxPriority::LOW;
    if (str == "NORMAL") return TxPriority::NORMAL;
    if (str == "HIGH") return TxPriority::HIGH;
    if (str == "HTLC") return TxPriority::HTLC;
    if (str == "BRIDGE") return TxPriority::BRIDGE;
    if (str == "CRITICAL") return TxPriority::CRITICAL;
    return TxPriority::NORMAL;
}

// Implementation details
struct INTcoinMempool::Impl {
    MempoolConfig config;
    bool is_initialized;
    mutable std::mutex mutex;

    // Transaction storage
    std::unordered_map<std::string, MempoolEntry> entries;

    // Priority queues (sorted by fee_per_byte within each priority)
    std::map<TxPriority, std::multimap<uint64_t, std::string>> priority_queues;

    // Orphan transactions (waiting for parent)
    std::unordered_map<std::string, MempoolEntry> orphan_txs;

    // Contract transaction tracking
    std::unordered_map<std::string, uint64_t> address_nonces;  // address -> next expected nonce
    std::unordered_map<std::string, std::string> nonce_to_tx;  // "address:nonce" -> tx_hash
    uint64_t total_gas_in_mempool;  // Total gas for all contract txs in mempool

    Impl() : is_initialized(false), total_gas_in_mempool(0) {}

    // Helper: Convert uint256 to hex string
    std::string Uint256ToHex(const uint256& hash) const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t byte : hash) {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }

    // Helper: Calculate total mempool size
    uint64_t GetTotalSize() const {
        uint64_t total = 0;
        for (const auto& pair : entries) {
            total += pair.second.size_bytes;
        }
        return total;
    }

    // Helper: Get count for priority level
    uint32_t GetCountForPriority(TxPriority priority) const {
        auto it = priority_queues.find(priority);
        if (it == priority_queues.end()) return 0;
        return it->second.size();
    }
};

INTcoinMempool::INTcoinMempool()
    : impl_(std::make_unique<Impl>()) {
}

INTcoinMempool::~INTcoinMempool() {
    if (impl_->is_initialized) {
        Shutdown();
    }
}

Result<void> INTcoinMempool::Initialize(const MempoolConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->is_initialized) {
        return Result<void>::Error("Mempool already initialized");
    }

    impl_->config = config;
    impl_->is_initialized = true;

    // Initialize priority queues
    for (int i = 0; i <= static_cast<int>(TxPriority::CRITICAL); ++i) {
        impl_->priority_queues[static_cast<TxPriority>(i)] = {};
    }

    // Try to restore from disk if configured
    if (config.persist_on_shutdown) {
        auto restore_result = Restore();
        if (restore_result.IsOk()) {
            LogF(LogLevel::INFO, "Mempool: Restored from %s", config.persist_file.c_str());
        }
    }

    LogF(LogLevel::INFO, "Mempool: Initialized (max size: %lu MB)", config.max_size_mb);
    return Result<void>::Ok();
}

Result<void> INTcoinMempool::Shutdown() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Mempool not initialized");
    }

    // Persist if configured
    if (impl_->config.persist_on_shutdown) {
        auto persist_result = Persist();
        if (!persist_result.IsOk()) {
            LogF(LogLevel::WARNING, "Mempool: Failed to persist on shutdown");
        }
    }

    impl_->entries.clear();
    impl_->priority_queues.clear();
    impl_->orphan_txs.clear();
    impl_->is_initialized = false;

    LogF(LogLevel::INFO, "Mempool: Shutdown complete");
    return Result<void>::Ok();
}

Result<void> INTcoinMempool::AddTransaction(const Transaction& tx, TxPriority priority) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Mempool not initialized");
    }

    // Calculate transaction hash
    uint256 tx_hash = tx.GetHash();
    std::string tx_key = impl_->Uint256ToHex(tx_hash);

    // Check if already in mempool
    if (impl_->entries.find(tx_key) != impl_->entries.end()) {
        return Result<void>::Error("Transaction already in mempool");
    }

    // Validate transaction
    if (!ValidateTransaction(tx)) {
        return Result<void>::Error("Transaction validation failed");
    }

    // Handle contract transactions differently
    if (tx.IsContractTransaction()) {
        return AddContractTransaction(tx, priority);
    }

    // Standard UTXO transaction handling
    // Calculate tx size and fee
    uint64_t tx_size = CalculateTxSize(tx);
    // Placeholder fee calculation: use minimum relay fee * size
    // In production, would calculate from (sum of inputs - sum of outputs) using UTXO set
    uint64_t fee = (impl_->config.min_relay_fee_per_kb * tx_size) / 1000;
    uint64_t fee_per_byte = fee / std::max<uint64_t>(tx_size, 1);

    // Determine final priority (may upgrade based on fee)
    TxPriority final_priority = DeterminePriority(tx, fee_per_byte);
    if (final_priority > priority) {
        priority = final_priority;
    }

    // Check priority limit
    if (impl_->GetCountForPriority(priority) >= impl_->config.priority_limits[priority]) {
        // Try to evict lower priority transactions
        EvictLowPriority();

        // Check again
        if (impl_->GetCountForPriority(priority) >= impl_->config.priority_limits[priority]) {
            return Result<void>::Error("Mempool full for this priority level");
        }
    }

    // Check total mempool size
    uint64_t max_size_bytes = impl_->config.max_size_mb * 1024 * 1024;
    if (impl_->GetTotalSize() + tx_size > max_size_bytes) {
        EvictLowPriority();

        if (impl_->GetTotalSize() + tx_size > max_size_bytes) {
            return Result<void>::Error("Mempool full");
        }
    }

    // Create mempool entry
    MempoolEntry entry;
    entry.tx = tx;
    entry.tx_hash = tx_hash;
    entry.priority = priority;
    entry.fee = fee;
    entry.fee_per_byte = fee_per_byte;
    entry.size_bytes = tx_size;
    entry.added_time = std::time(nullptr);
    entry.height_added = 0;  // Would get from blockchain
    entry.broadcast_count = 0;
    entry.last_broadcast = 0;

    // Add to storage
    impl_->entries[tx_key] = entry;

    // Add to priority queue
    impl_->priority_queues[priority].emplace(fee_per_byte, tx_key);

    LogF(LogLevel::INFO, "Mempool: Added tx %s (priority: %s, fee: %lu ints)",
         tx_key.substr(0, 16).c_str(), TxPriorityToString(priority).c_str(), fee);

    return Result<void>::Ok();
}

Result<void> INTcoinMempool::AddContractTransaction(const Transaction& tx, TxPriority priority) {
    // Note: Caller must hold mutex lock

    uint256 tx_hash = tx.GetHash();
    std::string tx_key = impl_->Uint256ToHex(tx_hash);

    // Parse contract transaction
    std::string from_address;
    uint64_t nonce = 0;
    uint64_t gas_limit = 0;
    uint64_t gas_price = 0;

    if (tx.IsContractDeployment()) {
        auto deploy_result = contracts::ContractDeploymentTx::Deserialize(tx.contract_data);
        if (!deploy_result.has_value()) {
            return Result<void>::Error("Failed to deserialize contract deployment");
        }

        const auto& deploy_tx = deploy_result.value();
        from_address = PublicKeyToAddress(deploy_tx.from);
        nonce = deploy_tx.nonce;
        gas_limit = deploy_tx.gas_limit;
        gas_price = deploy_tx.gas_price;

    } else if (tx.IsContractCall()) {
        auto call_result = contracts::ContractCallTx::Deserialize(tx.contract_data);
        if (!call_result.has_value()) {
            return Result<void>::Error("Failed to deserialize contract call");
        }

        const auto& call_tx = call_result.value();
        from_address = PublicKeyToAddress(call_tx.from);
        nonce = call_tx.nonce;
        gas_limit = call_tx.gas_limit;
        gas_price = call_tx.gas_price;
    }

    // Check nonce
    std::string nonce_key = from_address + ":" + std::to_string(nonce);

    // Check if we already have a transaction with this nonce
    auto nonce_it = impl_->nonce_to_tx.find(nonce_key);
    if (nonce_it != impl_->nonce_to_tx.end()) {
        // Transaction with same nonce exists - check for RBF (Replace-By-Fee)
        std::string existing_tx_key = nonce_it->second;
        auto existing_entry_it = impl_->entries.find(existing_tx_key);

        if (existing_entry_it != impl_->entries.end()) {
            const MempoolEntry& existing_entry = existing_entry_it->second;

            // Parse existing transaction to get its gas price
            uint64_t existing_gas_price = 0;
            if (existing_entry.tx.IsContractDeployment()) {
                auto existing_deploy = contracts::ContractDeploymentTx::Deserialize(existing_entry.tx.contract_data);
                if (existing_deploy.has_value()) {
                    existing_gas_price = existing_deploy.value().gas_price;
                }
            } else if (existing_entry.tx.IsContractCall()) {
                auto existing_call = contracts::ContractCallTx::Deserialize(existing_entry.tx.contract_data);
                if (existing_call.has_value()) {
                    existing_gas_price = existing_call.value().gas_price;
                }
            }

            // RBF: New transaction must have gas price at least 10% higher
            uint64_t min_replacement_gas_price = existing_gas_price + (existing_gas_price / 10);
            if (gas_price < min_replacement_gas_price) {
                return Result<void>::Error("Gas price too low for transaction replacement (need 10% increase)");
            }

            // Remove the old transaction
            LogF(LogLevel::INFO, "Mempool: Replacing tx %s with higher gas price (%lu -> %lu)",
                 existing_tx_key.substr(0, 16).c_str(), existing_gas_price, gas_price);

            // Update gas tracking
            if (existing_entry.tx.IsContractDeployment()) {
                auto old_deploy = contracts::ContractDeploymentTx::Deserialize(existing_entry.tx.contract_data);
                if (old_deploy.has_value()) {
                    impl_->total_gas_in_mempool -= old_deploy.value().gas_limit;
                }
            } else if (existing_entry.tx.IsContractCall()) {
                auto old_call = contracts::ContractCallTx::Deserialize(existing_entry.tx.contract_data);
                if (old_call.has_value()) {
                    impl_->total_gas_in_mempool -= old_call.value().gas_limit;
                }
            }

            // Remove from priority queue
            auto& pq = impl_->priority_queues[existing_entry.priority];
            auto range = pq.equal_range(existing_entry.fee_per_byte);
            for (auto pq_it = range.first; pq_it != range.second; ) {
                if (pq_it->second == existing_tx_key) {
                    pq_it = pq.erase(pq_it);
                    break;
                } else {
                    ++pq_it;
                }
            }

            // Remove from storage
            impl_->entries.erase(existing_entry_it);
        }
    }

    // Check if nonce is valid (should be next expected nonce for this address)
    auto address_nonce_it = impl_->address_nonces.find(from_address);
    if (address_nonce_it != impl_->address_nonces.end()) {
        uint64_t expected_nonce = address_nonce_it->second;
        if (nonce < expected_nonce) {
            return Result<void>::Error("Nonce too low (already used)");
        }
        // Allow future nonces (they'll be held until prerequisites are met)
    }

    // Calculate tx size and fee (gas-based for contracts)
    uint64_t tx_size = tx.GetSerializedSize();
    uint64_t fee = gas_limit * gas_price;  // Total fee = gas_limit * gas_price
    uint64_t fee_per_byte = fee / std::max<uint64_t>(tx_size, 1);

    // Contract transactions have higher priority based on gas price
    if (gas_price >= 100) priority = TxPriority::HIGH;
    else if (gas_price >= 10) priority = TxPriority::NORMAL;

    // Check block gas limit (30M gas target per block)
    constexpr uint64_t BLOCK_GAS_LIMIT = 30'000'000;
    if (impl_->total_gas_in_mempool + gas_limit > BLOCK_GAS_LIMIT * 2) {
        // Mempool can hold 2 blocks worth of gas
        return Result<void>::Error("Mempool gas limit exceeded");
    }

    // Check priority limit
    if (impl_->GetCountForPriority(priority) >= impl_->config.priority_limits[priority]) {
        EvictLowPriority();

        if (impl_->GetCountForPriority(priority) >= impl_->config.priority_limits[priority]) {
            return Result<void>::Error("Mempool full for this priority level");
        }
    }

    // Create mempool entry
    MempoolEntry entry;
    entry.tx = tx;
    entry.tx_hash = tx_hash;
    entry.priority = priority;
    entry.fee = fee;
    entry.fee_per_byte = fee_per_byte;
    entry.size_bytes = tx_size;
    entry.added_time = std::time(nullptr);
    entry.height_added = 0;
    entry.broadcast_count = 0;
    entry.last_broadcast = 0;

    // Add to storage
    impl_->entries[tx_key] = entry;

    // Add to priority queue (sorted by gas price for contracts)
    impl_->priority_queues[priority].emplace(gas_price, tx_key);

    // Update contract tracking
    impl_->nonce_to_tx[nonce_key] = tx_key;
    impl_->address_nonces[from_address] = nonce + 1;  // Update expected nonce
    impl_->total_gas_in_mempool += gas_limit;

    LogF(LogLevel::INFO, "Mempool: Added contract tx %s (nonce: %lu, gas: %lu, gas_price: %lu)",
         tx_key.substr(0, 16).c_str(), nonce, gas_limit, gas_price);

    return Result<void>::Ok();
}

Result<void> INTcoinMempool::RemoveTransactionInternal(const uint256& tx_hash) {
    // Note: Caller must hold mutex lock
    if (!impl_->is_initialized) {
        return Result<void>::Error("Mempool not initialized");
    }

    std::string tx_key = impl_->Uint256ToHex(tx_hash);

    auto it = impl_->entries.find(tx_key);
    if (it == impl_->entries.end()) {
        return Result<void>::Error("Transaction not found");
    }

    const MempoolEntry& entry = it->second;

    // If contract transaction, clean up tracking data
    if (entry.tx.IsContractTransaction()) {
        std::string from_address;
        uint64_t nonce = 0;
        uint64_t gas_limit = 0;

        if (entry.tx.IsContractDeployment()) {
            auto deploy_result = contracts::ContractDeploymentTx::Deserialize(entry.tx.contract_data);
            if (deploy_result.has_value()) {
                from_address = PublicKeyToAddress(deploy_result.value().from);
                nonce = deploy_result.value().nonce;
                gas_limit = deploy_result.value().gas_limit;
            }
        } else if (entry.tx.IsContractCall()) {
            auto call_result = contracts::ContractCallTx::Deserialize(entry.tx.contract_data);
            if (call_result.has_value()) {
                from_address = PublicKeyToAddress(call_result.value().from);
                nonce = call_result.value().nonce;
                gas_limit = call_result.value().gas_limit;
            }
        }

        if (!from_address.empty()) {
            std::string nonce_key = from_address + ":" + std::to_string(nonce);
            impl_->nonce_to_tx.erase(nonce_key);
            impl_->total_gas_in_mempool -= gas_limit;
        }
    }

    // Remove from priority queue
    auto& pq = impl_->priority_queues[entry.priority];
    auto range = pq.equal_range(entry.fee_per_byte);
    for (auto pq_it = range.first; pq_it != range.second; ) {
        if (pq_it->second == tx_key) {
            pq_it = pq.erase(pq_it);
            break;
        } else {
            ++pq_it;
        }
    }

    // Remove from storage
    impl_->entries.erase(it);

    LogF(LogLevel::INFO, "Mempool: Removed tx %s", tx_key.substr(0, 16).c_str());
    return Result<void>::Ok();
}

Result<void> INTcoinMempool::RemoveTransaction(const uint256& tx_hash) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return RemoveTransactionInternal(tx_hash);
}

bool INTcoinMempool::HasTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) return false;

    std::string tx_key = impl_->Uint256ToHex(tx_hash);
    return impl_->entries.find(tx_key) != impl_->entries.end();
}

Result<Transaction> INTcoinMempool::GetTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<Transaction>::Error("Mempool not initialized");
    }

    std::string tx_key = impl_->Uint256ToHex(tx_hash);
    auto it = impl_->entries.find(tx_key);

    if (it == impl_->entries.end()) {
        return Result<Transaction>::Error("Transaction not found");
    }

    return Result<Transaction>::Ok(it->second.tx);
}

Result<MempoolEntry> INTcoinMempool::GetEntry(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<MempoolEntry>::Error("Mempool not initialized");
    }

    std::string tx_key = impl_->Uint256ToHex(tx_hash);
    auto it = impl_->entries.find(tx_key);

    if (it == impl_->entries.end()) {
        return Result<MempoolEntry>::Error("Transaction not found");
    }

    return Result<MempoolEntry>::Ok(it->second);
}

std::vector<MempoolEntry> INTcoinMempool::GetAllTransactionsInternal() const {
    // Note: Caller must hold mutex lock
    std::vector<MempoolEntry> result;
    result.reserve(impl_->entries.size());

    for (const auto& pair : impl_->entries) {
        result.push_back(pair.second);
    }

    // Sort by priority (descending), then fee_per_byte (descending)
    std::sort(result.begin(), result.end(), [](const MempoolEntry& a, const MempoolEntry& b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.fee_per_byte > b.fee_per_byte;
    });

    return result;
}

std::vector<MempoolEntry> INTcoinMempool::GetAllTransactions() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return GetAllTransactionsInternal();
}

std::vector<Transaction> INTcoinMempool::GetBlockTemplate(
    uint64_t max_size_bytes,
    uint64_t max_count
) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<Transaction> result;
    uint64_t total_size = 0;
    uint64_t total_gas = 0;
    constexpr uint64_t BLOCK_GAS_LIMIT = 30'000'000;  // 30M gas per block

    // Get transactions ordered by priority and fee
    auto all_entries = GetAllTransactionsInternal();

    for (const auto& entry : all_entries) {
        if (max_count > 0 && result.size() >= max_count) break;
        if (total_size + entry.size_bytes > max_size_bytes) break;

        // Check gas limit for contract transactions
        if (entry.tx.IsContractTransaction()) {
            uint64_t tx_gas_limit = 0;

            if (entry.tx.IsContractDeployment()) {
                auto deploy_result = contracts::ContractDeploymentTx::Deserialize(entry.tx.contract_data);
                if (deploy_result.has_value()) {
                    tx_gas_limit = deploy_result.value().gas_limit;
                }
            } else if (entry.tx.IsContractCall()) {
                auto call_result = contracts::ContractCallTx::Deserialize(entry.tx.contract_data);
                if (call_result.has_value()) {
                    tx_gas_limit = call_result.value().gas_limit;
                }
            }

            // Skip if adding this tx would exceed block gas limit
            if (total_gas + tx_gas_limit > BLOCK_GAS_LIMIT) {
                continue;
            }

            total_gas += tx_gas_limit;
        }

        result.push_back(entry.tx);
        total_size += entry.size_bytes;
    }

    return result;
}

Result<uint32_t> INTcoinMempool::RemoveConfirmedTransactions(
    const std::vector<uint256>& tx_hashes
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint32_t>::Error("Mempool not initialized");
    }

    uint32_t removed_count = 0;

    for (const auto& tx_hash : tx_hashes) {
        auto remove_result = RemoveTransactionInternal(tx_hash);
        if (remove_result.IsOk()) {
            removed_count++;
        }
    }

    if (removed_count > 0) {
        LogF(LogLevel::INFO, "Mempool: Removed %u confirmed transactions", removed_count);
    }

    return Result<uint32_t>::Ok(removed_count);
}

Result<uint32_t> INTcoinMempool::RemoveExpired() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint32_t>::Error("Mempool not initialized");
    }

    std::time_t now = std::time(nullptr);
    std::time_t expiry_threshold = now - (impl_->config.expiry_hours * 3600);

    std::vector<std::string> to_remove;

    for (const auto& pair : impl_->entries) {
        if (pair.second.added_time < expiry_threshold) {
            to_remove.push_back(pair.first);
        }
    }

    uint32_t removed_count = 0;

    for (const auto& tx_key : to_remove) {
        // Find and remove the entry
        auto it = impl_->entries.find(tx_key);
        if (it != impl_->entries.end()) {
            const MempoolEntry& entry = it->second;

            // Remove from priority queue
            auto& pq = impl_->priority_queues[entry.priority];
            auto range = pq.equal_range(entry.fee_per_byte);
            for (auto pq_it = range.first; pq_it != range.second; ) {
                if (pq_it->second == tx_key) {
                    pq_it = pq.erase(pq_it);
                    break;
                } else {
                    ++pq_it;
                }
            }

            // Remove from storage
            impl_->entries.erase(it);
            removed_count++;
        }
    }

    if (removed_count > 0) {
        LogF(LogLevel::INFO, "Mempool: Removed %u expired transactions", removed_count);
    }

    return Result<uint32_t>::Ok(removed_count);
}

MempoolStats INTcoinMempool::GetStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    MempoolStats stats = {};

    stats.total_transactions = impl_->entries.size();
    stats.total_size_bytes = impl_->GetTotalSize();
    stats.total_fees = 0;
    stats.memory_usage_bytes = impl_->entries.size() * sizeof(MempoolEntry);
    stats.orphan_count = impl_->orphan_txs.size();

    uint64_t min_fee = UINT64_MAX;
    uint64_t max_fee = 0;
    uint64_t total_fee_per_byte = 0;

    // Calculate stats by priority
    for (const auto& pair : impl_->entries) {
        const MempoolEntry& entry = pair.second;

        stats.total_fees += entry.fee;
        stats.count_by_priority[entry.priority]++;
        stats.size_by_priority[entry.priority] += entry.size_bytes;

        total_fee_per_byte += entry.fee_per_byte;

        if (entry.fee < min_fee) min_fee = entry.fee;
        if (entry.fee > max_fee) max_fee = entry.fee;
    }

    stats.min_fee = (min_fee == UINT64_MAX) ? 0 : min_fee;
    stats.max_fee = max_fee;
    stats.avg_fee_per_byte = stats.total_transactions > 0 ?
        static_cast<double>(total_fee_per_byte) / stats.total_transactions : 0.0;

    return stats;
}

Result<uint64_t> INTcoinMempool::EstimateFee(
    TxPriority priority,
    uint64_t size_bytes
) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint64_t>::Error("Mempool not initialized");
    }

    // Get transactions at this priority level
    auto it = impl_->priority_queues.find(priority);
    if (it == impl_->priority_queues.end() || it->second.empty()) {
        // No transactions at this priority, use min relay fee
        uint64_t fee = (size_bytes * impl_->config.min_relay_fee_per_kb) / 1024;
        return Result<uint64_t>::Ok(fee);
    }

    // Use median fee_per_byte from this priority level
    const auto& pq = it->second;
    auto mid_it = pq.begin();
    std::advance(mid_it, pq.size() / 2);
    uint64_t median_fee_per_byte = mid_it->first;

    uint64_t estimated_fee = median_fee_per_byte * size_bytes;
    return Result<uint64_t>::Ok(estimated_fee);
}

Result<void> INTcoinMempool::Persist() const {
    // Note: Caller must hold mutex lock
    if (!impl_->is_initialized) {
        return Result<void>::Error("Mempool not initialized");
    }

    std::ofstream file(impl_->config.persist_file, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Error("Failed to open mempool file for writing");
    }

    // Write header
    uint32_t version = 1;
    uint32_t count = impl_->entries.size();
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write entries (simplified - would need proper serialization)
    // This is a placeholder implementation
    for (const auto& pair : impl_->entries) {
        (void)pair;  // Suppress unused warning - placeholder implementation
        // Would serialize MempoolEntry here
    }

    file.close();

    LogF(LogLevel::INFO, "Mempool: Persisted %u transactions to %s",
         count, impl_->config.persist_file.c_str());

    return Result<void>::Ok();
}

Result<void> INTcoinMempool::Restore() {
    // Note: Caller must hold mutex lock
    std::ifstream file(impl_->config.persist_file, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Error("Mempool file not found");
    }

    // Read header
    uint32_t version = 0;
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    if (version != 1) {
        return Result<void>::Error("Unsupported mempool version");
    }

    // Read entries (simplified - would need proper deserialization)
    // This is a placeholder implementation
    uint32_t restored = 0;
    for (uint32_t i = 0; i < count; ++i) {
        // Would deserialize MempoolEntry here
        // Then call AddTransaction()
        restored++;
    }

    file.close();

    LogF(LogLevel::INFO, "Mempool: Restored %u transactions from %s",
         restored, impl_->config.persist_file.c_str());

    return Result<void>::Ok();
}

Result<void> INTcoinMempool::Clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Mempool not initialized");
    }

    impl_->entries.clear();
    impl_->priority_queues.clear();
    impl_->orphan_txs.clear();

    // Re-initialize priority queues
    for (int i = 0; i <= static_cast<int>(TxPriority::CRITICAL); ++i) {
        impl_->priority_queues[static_cast<TxPriority>(i)] = {};
    }

    LogF(LogLevel::INFO, "Mempool: Cleared all transactions");
    return Result<void>::Ok();
}

// Private methods

TxPriority INTcoinMempool::DeterminePriority(
    const Transaction& tx,
    uint64_t fee_per_byte
) const {
    // Check if it's an HTLC transaction
    // Would check tx structure for HTLC patterns
    // For now, use fee-based priority

    if (fee_per_byte >= 100) return TxPriority::HIGH;
    if (fee_per_byte >= 10) return TxPriority::NORMAL;
    return TxPriority::LOW;
}

bool INTcoinMempool::ValidateTransaction(const Transaction& tx) const {
    // Basic validation
    if (tx.inputs.empty()) return false;
    if (tx.outputs.empty()) return false;

    // Would perform full validation:
    // - Check signatures
    // - Verify inputs exist and are unspent
    // - Check amounts
    // - Verify scripts
    // - Check for double spends

    return true;
}

void INTcoinMempool::EvictLowPriority() {
    // Evict lowest priority, lowest fee transactions first
    for (int priority = 0; priority <= static_cast<int>(TxPriority::NORMAL); ++priority) {
        auto& pq = impl_->priority_queues[static_cast<TxPriority>(priority)];

        if (!pq.empty()) {
            // Remove lowest fee transaction
            auto it = pq.begin();
            std::string tx_key = it->second;
            pq.erase(it);
            impl_->entries.erase(tx_key);

            LogF(LogLevel::INFO, "Mempool: Evicted low priority tx %s",
                 tx_key.substr(0, 16).c_str());
            return;
        }
    }
}

uint64_t INTcoinMempool::CalculateTxSize(const Transaction& tx) const {
    // Simplified size calculation
    // Would calculate actual serialized size
    return 250;  // Placeholder: average transaction size
}

} // namespace intcoin
