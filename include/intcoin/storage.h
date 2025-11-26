/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Storage Layer (RocksDB Backend)
 */

#ifndef INTCOIN_STORAGE_H
#define INTCOIN_STORAGE_H

#include "types.h"
#include "block.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace intcoin {

// ============================================================================
// Database Key Prefixes
// ============================================================================

namespace db {

constexpr char PREFIX_BLOCK = 'b';           // block_hash -> Block
constexpr char PREFIX_BLOCK_HEIGHT = 'h';    // height -> block_hash
constexpr char PREFIX_TX = 't';              // tx_hash -> Transaction
constexpr char PREFIX_UTXO = 'u';            // outpoint -> TxOut
constexpr char PREFIX_ADDRESS_INDEX = 'i';   // address -> [tx_hashes]
constexpr char PREFIX_CHAINSTATE = 'c';      // chainstate metadata
constexpr char PREFIX_PEER = 'p';            // peer_id -> PeerInfo
constexpr char PREFIX_BLOCK_INDEX = 'x';     // block_hash -> BlockIndex

} // namespace db

// ============================================================================
// Chain State
// ============================================================================

struct ChainState {
    /// Best block hash
    uint256 best_block_hash;

    /// Best block height
    uint64_t best_height;

    /// Total chain work
    uint256 chain_work;

    /// Total transactions
    uint64_t total_transactions;

    /// UTXO set size
    uint64_t utxo_count;

    /// Total supply
    uint64_t total_supply;

    /// Serialize
    std::vector<uint8_t> Serialize() const;

    /// Deserialize
    static Result<ChainState> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Block Index (metadata for each block)
// ============================================================================

struct BlockIndex {
    /// Block hash
    uint256 hash;

    /// Block height
    uint64_t height;

    /// Previous block hash
    uint256 prev_hash;

    /// Block timestamp
    uint64_t timestamp;

    /// Difficulty bits
    uint32_t bits;

    /// Chain work (cumulative)
    uint256 chain_work;

    /// Number of transactions
    uint32_t tx_count;

    /// Block size
    uint32_t size;

    /// File position (for pruning)
    uint64_t file_pos;

    /// Serialize
    std::vector<uint8_t> Serialize() const;

    /// Deserialize
    static Result<BlockIndex> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Blockchain Database
// ============================================================================

class BlockchainDB {
public:
    /// Constructor
    explicit BlockchainDB(const std::string& data_dir);

    /// Destructor
    ~BlockchainDB();

    /// Open database
    Result<void> Open();

    /// Close database
    void Close();

    /// Check if database is open
    bool IsOpen() const;

    // ------------------------------------------------------------------------
    // Block Operations
    // ------------------------------------------------------------------------

    /// Store block
    Result<void> StoreBlock(const Block& block);

    /// Get block by hash
    Result<Block> GetBlock(const uint256& hash) const;

    /// Get block by height
    Result<Block> GetBlockByHeight(uint64_t height) const;

    /// Check if block exists
    bool HasBlock(const uint256& hash) const;

    /// Delete block (for reorg)
    Result<void> DeleteBlock(const uint256& hash);

    // ------------------------------------------------------------------------
    // Block Index Operations
    // ------------------------------------------------------------------------

    /// Store block index
    Result<void> StoreBlockIndex(const BlockIndex& index);

    /// Get block index
    Result<BlockIndex> GetBlockIndex(const uint256& hash) const;

    /// Get block hash by height
    Result<uint256> GetBlockHash(uint64_t height) const;

    /// Store height -> hash mapping
    Result<void> StoreBlockHeight(uint64_t height, const uint256& hash);

    // ------------------------------------------------------------------------
    // Transaction Operations
    // ------------------------------------------------------------------------

    /// Store transaction
    Result<void> StoreTransaction(const Transaction& tx);

    /// Get transaction by hash
    Result<Transaction> GetTransaction(const uint256& hash) const;

    /// Check if transaction exists
    bool HasTransaction(const uint256& hash) const;

    /// Delete transaction
    Result<void> DeleteTransaction(const uint256& hash);

    // ------------------------------------------------------------------------
    // UTXO Operations
    // ------------------------------------------------------------------------

    /// Store UTXO
    Result<void> StoreUTXO(const OutPoint& outpoint, const TxOut& output);

    /// Get UTXO
    Result<TxOut> GetUTXO(const OutPoint& outpoint) const;

    /// Check if UTXO exists
    bool HasUTXO(const OutPoint& outpoint) const;

    /// Delete UTXO (spent)
    Result<void> DeleteUTXO(const OutPoint& outpoint);

    /// Get all UTXOs for address
    Result<std::vector<std::pair<OutPoint, TxOut>>> GetUTXOsForAddress(
        const std::string& address) const;

    // ------------------------------------------------------------------------
    // Chain State Operations
    // ------------------------------------------------------------------------

    /// Store chain state
    Result<void> StoreChainState(const ChainState& state);

    /// Get chain state
    Result<ChainState> GetChainState() const;

    /// Update best block
    Result<void> UpdateBestBlock(const uint256& hash, uint64_t height);

    // ------------------------------------------------------------------------
    // Address Index Operations
    // ------------------------------------------------------------------------

    /// Add transaction to address index
    Result<void> IndexTransaction(const Transaction& tx);

    /// Get transactions for address
    Result<std::vector<uint256>> GetTransactionsForAddress(
        const std::string& address) const;

    // ------------------------------------------------------------------------
    // Batch Operations
    // ------------------------------------------------------------------------

    /// Begin batch write
    void BeginBatch();

    /// Commit batch write
    Result<void> CommitBatch();

    /// Abort batch write
    void AbortBatch();

    // ------------------------------------------------------------------------
    // Pruning
    // ------------------------------------------------------------------------

    /// Enable pruning
    void EnablePruning(uint64_t target_size_gb);

    /// Prune old blocks
    Result<void> PruneBlocks(uint64_t keep_blocks);

    /// Check if pruning is enabled
    bool IsPruningEnabled() const;

    // ------------------------------------------------------------------------
    // Database Stats
    // ------------------------------------------------------------------------

    /// Get database size
    uint64_t GetDatabaseSize() const;

    /// Get block count
    uint64_t GetBlockCount() const;

    /// Get transaction count
    uint64_t GetTransactionCount() const;

    /// Get UTXO count
    uint64_t GetUTXOCount() const;

    // ------------------------------------------------------------------------
    // Maintenance
    // ------------------------------------------------------------------------

    /// Compact database
    Result<void> Compact();

    /// Verify database integrity
    Result<void> Verify();

    /// Backup database
    Result<void> Backup(const std::string& backup_dir);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// UTXO Set (In-Memory Cache)
// ============================================================================

class UTXOSet {
public:
    /// Constructor
    explicit UTXOSet(std::shared_ptr<BlockchainDB> db);

    /// Load UTXO set from database
    Result<void> Load();

    /// Add UTXO
    Result<void> AddUTXO(const OutPoint& outpoint, const TxOut& output);

    /// Spend UTXO
    Result<void> SpendUTXO(const OutPoint& outpoint);

    /// Get UTXO
    std::optional<TxOut> GetUTXO(const OutPoint& outpoint) const;

    /// Check if UTXO exists
    bool HasUTXO(const OutPoint& outpoint) const;

    /// Get total value
    uint64_t GetTotalValue() const;

    /// Get UTXO count
    size_t GetCount() const;

    /// Apply block (add outputs, spend inputs)
    Result<void> ApplyBlock(const Block& block);

    /// Revert block (undo changes)
    Result<void> RevertBlock(const Block& block);

    /// Flush to database
    Result<void> Flush();

    /// Get all UTXOs for address
    std::vector<std::pair<OutPoint, TxOut>> GetUTXOsForAddress(
        const std::string& address) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Mempool (Transaction Pool)
// ============================================================================

class Mempool {
public:
    /// Constructor
    Mempool();

    /// Destructor
    ~Mempool();

    /// Add transaction
    Result<void> AddTransaction(const Transaction& tx);

    /// Remove transaction
    void RemoveTransaction(const uint256& tx_hash);

    /// Get transaction
    std::optional<Transaction> GetTransaction(const uint256& tx_hash) const;

    /// Check if transaction exists
    bool HasTransaction(const uint256& tx_hash) const;

    /// Get all transactions
    std::vector<Transaction> GetAllTransactions() const;

    /// Get transactions for mining (sorted by fee)
    std::vector<Transaction> GetTransactionsForMining(size_t max_count) const;

    /// Remove transactions in block
    void RemoveBlockTransactions(const Block& block);

    /// Get mempool size
    size_t GetSize() const;

    /// Get total fees
    uint64_t GetTotalFees() const;

    /// Clear mempool
    void Clear();

    /// Limit mempool size (evict low-fee txs)
    void LimitSize(size_t max_size);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Wallet Database
// ============================================================================

class WalletDB {
public:
    /// Constructor
    explicit WalletDB(const std::string& wallet_file);

    /// Destructor
    ~WalletDB();

    /// Open wallet
    Result<void> Open(const std::string& passphrase = "");

    /// Close wallet
    void Close();

    /// Create new wallet
    static Result<void> Create(const std::string& wallet_file,
                              const std::string& passphrase = "");

    /// Store key pair
    Result<void> StoreKeyPair(const std::string& label,
                             const PublicKey& pubkey,
                             const SecretKey& seckey);

    /// Get secret key
    Result<SecretKey> GetSecretKey(const PublicKey& pubkey) const;

    /// Get all public keys
    Result<std::vector<PublicKey>> GetAllPublicKeys() const;

    /// Store transaction
    Result<void> StoreWalletTransaction(const Transaction& tx);

    /// Get wallet transactions
    Result<std::vector<Transaction>> GetWalletTransactions() const;

    /// Encrypt wallet
    Result<void> Encrypt(const std::string& passphrase);

    /// Decrypt wallet
    Result<void> Decrypt(const std::string& passphrase);

    /// Check if encrypted
    bool IsEncrypted() const;

    /// Backup wallet
    Result<void> Backup(const std::string& backup_file) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace intcoin

#endif // INTCOIN_STORAGE_H
