// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_DB_H
#define INTCOIN_DB_H

#include "primitives.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>

namespace intcoin {

/**
 * Database abstraction layer for RocksDB
 *
 * Provides a simple key-value interface with batch operations
 * for blockchain storage, UTXO set, transaction index, etc.
 */
class Database {
public:
    /**
     * Open or create a database at the specified path
     * @param path Directory path for the database
     * @param create_if_missing Create the database if it doesn't exist
     * @return true if successful, false otherwise
     */
    bool open(const std::string& path, bool create_if_missing = true);

    /**
     * Close the database
     */
    void close();

    /**
     * Check if database is open
     */
    bool is_open() const { return db_ != nullptr; }

    /**
     * Write a key-value pair
     * @param key The key to write
     * @param value The value to write
     * @return true if successful
     */
    bool write(const std::string& key, const std::string& value);

    /**
     * Write a key-value pair with binary data
     */
    bool write(const std::string& key, const std::vector<uint8_t>& value);

    /**
     * Read a value by key
     * @param key The key to read
     * @return The value if found, std::nullopt otherwise
     */
    std::optional<std::string> read(const std::string& key) const;

    /**
     * Read binary data by key
     */
    std::optional<std::vector<uint8_t>> read_bytes(const std::string& key) const;

    /**
     * Delete a key
     * @param key The key to delete
     * @return true if successful
     */
    bool erase(const std::string& key);

    /**
     * Check if a key exists
     * @param key The key to check
     * @return true if the key exists
     */
    bool exists(const std::string& key) const;

    /**
     * Batch write operations for atomic updates
     */
    class Batch {
    public:
        Batch() = default;

        void write(const std::string& key, const std::string& value);
        void write(const std::string& key, const std::vector<uint8_t>& value);
        void erase(const std::string& key);
        void clear();

    private:
        friend class Database;
        rocksdb::WriteBatch batch_;
    };

    /**
     * Apply a batch of operations atomically
     * @param batch The batch to apply
     * @return true if successful
     */
    bool write_batch(const Batch& batch);

    /**
     * Compact the database to optimize storage
     */
    void compact();

    /**
     * Get database statistics
     */
    struct Stats {
        size_t num_keys;
        size_t total_size;
        std::string db_path;
    };
    Stats get_stats() const;

private:
    std::unique_ptr<rocksdb::DB> db_;
    std::string db_path_;
};

/**
 * Block index database
 * Maps block hash -> block data and height -> block hash
 */
class BlockIndexDB {
public:
    BlockIndexDB() = default;

    bool open(const std::string& data_dir);
    void close();

    // Write block data
    bool write_block(const Hash256& hash, uint32_t height, const std::vector<uint8_t>& block_data);

    // Read block data by hash
    std::optional<std::vector<uint8_t>> read_block(const Hash256& hash) const;

    // Get block hash by height
    std::optional<Hash256> get_block_hash(uint32_t height) const;

    // Get block height by hash
    std::optional<uint32_t> get_block_height(const Hash256& hash) const;

    // Check if block exists
    bool has_block(const Hash256& hash) const;

    // Get best block height
    std::optional<uint32_t> get_best_height() const;

    // Update best block
    bool set_best_block(const Hash256& hash, uint32_t height);

private:
    Database db_;
};

/**
 * UTXO set database
 * Maps OutPoint -> TxOutput for all unspent outputs
 */
class UTXODatabase {
public:
    UTXODatabase() = default;

    bool open(const std::string& data_dir);
    void close();

    // Write UTXO
    bool write_utxo(const OutPoint& outpoint, const TxOutput& output, uint32_t height);

    // Read UTXO
    std::optional<std::pair<TxOutput, uint32_t>> read_utxo(const OutPoint& outpoint) const;

    // Delete UTXO (when spent)
    bool erase_utxo(const OutPoint& outpoint);

    // Check if UTXO exists
    bool has_utxo(const OutPoint& outpoint) const;

    // Batch operations for block processing
    class Batch {
    public:
        void add_utxo(const OutPoint& outpoint, const TxOutput& output, uint32_t height);
        void spend_utxo(const OutPoint& outpoint);

    private:
        friend class UTXODatabase;
        Database::Batch batch_;
    };

    bool apply_batch(const Batch& batch);

    // Get UTXO count
    size_t get_utxo_count() const;

private:
    Database db_;
};

/**
 * Transaction index database
 * Maps transaction hash -> block hash and transaction data
 */
class TransactionIndexDB {
public:
    TransactionIndexDB() = default;

    bool open(const std::string& data_dir);
    void close();

    // Write transaction
    bool write_transaction(const Hash256& tx_hash, const Hash256& block_hash,
                          uint32_t height, const std::vector<uint8_t>& tx_data);

    // Read transaction data
    std::optional<std::vector<uint8_t>> read_transaction(const Hash256& tx_hash) const;

    // Get block containing transaction
    std::optional<Hash256> get_transaction_block(const Hash256& tx_hash) const;

    // Get transaction height
    std::optional<uint32_t> get_transaction_height(const Hash256& tx_hash) const;

    // Check if transaction exists
    bool has_transaction(const Hash256& tx_hash) const;

private:
    Database db_;
};

} // namespace intcoin

#endif // INTCOIN_DB_H
