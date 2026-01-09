// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_DATABASE_H
#define INTCOIN_CONTRACTS_DATABASE_H

#include "intcoin/types.h"
#include "intcoin/contracts/vm.h"
#include "intcoin/contracts/transaction.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace intcoin {
namespace contracts {

/**
 * Contract Account Info
 *
 * Stores metadata and state for a deployed contract
 */
struct ContractAccount {
    std::string address;                    // Bech32 address (int1q...)
    uint64_t balance;                       // Contract balance in satINT
    uint64_t nonce;                         // Transaction counter
    std::vector<uint8_t> bytecode;          // Contract bytecode
    uint256 code_hash;                      // SHA3-256 hash of bytecode
    uint256 storage_root;                   // Merkle root of storage trie
    std::string creator;                    // Creator's address
    uint256 creation_tx;                    // Creation transaction hash
    uint64_t block_created;                 // Block number when created
    uint64_t block_updated;                 // Last update block number
};

/**
 * Event Log Entry
 *
 * Stores emitted events from contract execution
 */
struct EventLogEntry {
    std::string contract_address;          // Contract that emitted the log
    std::vector<uint256> topics;           // Indexed topics (event signature + params)
    std::vector<uint8_t> data;             // Non-indexed data
    uint64_t block_number;                 // Block number
    uint256 transaction_hash;              // Transaction hash
    uint32_t log_index;                    // Index within transaction
};

/**
 * Transaction Receipt
 *
 * Stores execution result of a contract transaction
 */
struct TransactionReceipt {
    uint256 txid;                          // Transaction hash
    std::string contract_address;          // Contract address (for deployment)
    std::string from;                      // Sender address
    std::string to;                        // Recipient address (for calls)
    uint64_t gas_used;                     // Actual gas consumed
    uint64_t gas_price;                    // Gas price paid
    uint64_t total_fee;                    // Total fee = gas_used * gas_price
    ExecutionResult status;                // Execution status
    std::vector<uint8_t> return_data;      // Returned data
    std::vector<EventLogEntry> logs;       // Emitted event logs
    uint64_t block_number;                 // Block number
    uint64_t block_timestamp;              // Block timestamp
    uint32_t tx_index;                     // Index within block
};

/**
 * Contract Database
 *
 * Persistent storage layer for smart contract state, receipts, and logs
 */
class ContractDatabase {
public:
    ContractDatabase();
    explicit ContractDatabase(const std::string& db_path);
    ~ContractDatabase();

    // Disable copy, allow move
    ContractDatabase(const ContractDatabase&) = delete;
    ContractDatabase& operator=(const ContractDatabase&) = delete;
    ContractDatabase(ContractDatabase&&) noexcept;
    ContractDatabase& operator=(ContractDatabase&&) noexcept;

    /**
     * Open/close database
     */
    Result<void> Open(const std::string& db_path);
    void Close();
    bool IsOpen() const;

    /**
     * Contract Account Operations
     */

    /// Store contract account
    Result<void> PutContractAccount(const ContractAccount& account);

    /// Retrieve contract account by address
    Result<ContractAccount> GetContractAccount(const std::string& address);

    /// Check if contract exists
    bool ContractExists(const std::string& address);

    /// Delete contract account (for testing/cleanup)
    Result<void> DeleteContractAccount(const std::string& address);

    /// List all contract addresses (with pagination)
    Result<std::vector<std::string>> ListContractAddresses(
        uint32_t limit = 100,
        uint32_t offset = 0
    );

    /**
     * Contract Storage Operations
     */

    /// Store contract storage key-value pair
    Result<void> PutContractStorage(
        const std::string& contract_address,
        const Word256& key,
        const Word256& value
    );

    /// Retrieve storage value by key
    Result<Word256> GetContractStorage(
        const std::string& contract_address,
        const Word256& key
    );

    /// Check if storage key exists
    bool StorageKeyExists(
        const std::string& contract_address,
        const Word256& key
    );

    /// Delete storage key (sets to zero)
    Result<void> DeleteContractStorage(
        const std::string& contract_address,
        const Word256& key
    );

    /**
     * Transaction Receipt Operations
     */

    /// Store transaction receipt
    Result<void> PutReceipt(const TransactionReceipt& receipt);

    /// Retrieve receipt by transaction hash
    Result<TransactionReceipt> GetReceipt(const uint256& txid);

    /// Check if receipt exists
    bool ReceiptExists(const uint256& txid);

    /**
     * Event Log Operations
     */

    /// Store event log
    Result<void> PutEventLog(const EventLogEntry& log);

    /// Query event logs with filters
    /// @param contract_address Filter by contract (empty = all contracts)
    /// @param from_block Starting block number
    /// @param to_block Ending block number
    /// @param topics Topic filters (empty = no filter)
    Result<std::vector<EventLogEntry>> QueryEventLogs(
        const std::string& contract_address = "",
        uint64_t from_block = 0,
        uint64_t to_block = UINT64_MAX,
        const std::vector<uint256>& topics = {}
    );

    /**
     * Batch Operations
     */

    /// Begin a write batch
    void BeginBatch();

    /// Commit the current batch
    Result<void> CommitBatch();

    /// Discard the current batch
    void DiscardBatch();

    /**
     * Database Statistics
     */

    struct Stats {
        uint64_t total_contracts;          // Total deployed contracts
        uint64_t total_receipts;           // Total receipts stored
        uint64_t total_logs;               // Total event logs
        uint64_t db_size_bytes;            // Database size on disk
    };

    Result<Stats> GetStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_DATABASE_H
