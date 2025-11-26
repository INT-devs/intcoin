/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Blockchain State and Operations
 */

#ifndef INTCOIN_BLOCKCHAIN_H
#define INTCOIN_BLOCKCHAIN_H

#include "types.h"
#include "block.h"
#include "transaction.h"
#include "storage.h"
#include <memory>
#include <vector>
#include <optional>
#include <functional>

namespace intcoin {

// ============================================================================
// Blockchain
// ============================================================================

class Blockchain {
public:
    /// Constructor
    explicit Blockchain(std::shared_ptr<BlockchainDB> db);

    /// Destructor
    ~Blockchain();

    /// Initialize blockchain (create genesis if needed)
    Result<void> Initialize();

    // ------------------------------------------------------------------------
    // Block Operations
    // ------------------------------------------------------------------------

    /// Add block to chain
    Result<void> AddBlock(const Block& block);

    /// Get block by hash
    Result<Block> GetBlock(const uint256& hash) const;

    /// Get block by height
    Result<Block> GetBlockByHeight(uint64_t height) const;

    /// Get block header by hash
    Result<BlockHeader> GetBlockHeader(const uint256& hash) const;

    /// Get block header by height
    Result<BlockHeader> GetBlockHeaderByHeight(uint64_t height) const;

    /// Check if block exists
    bool HasBlock(const uint256& hash) const;

    /// Get best block
    Result<Block> GetBestBlock() const;

    /// Get best block hash
    uint256 GetBestBlockHash() const;

    /// Get best block height
    uint64_t GetBestHeight() const;

    // ------------------------------------------------------------------------
    // Chain State
    // ------------------------------------------------------------------------

    /// Get chain work
    uint256 GetChainWork() const;

    /// Get total transactions
    uint64_t GetTotalTransactions() const;

    /// Get total supply
    uint64_t GetTotalSupply() const;

    /// Get difficulty
    double GetDifficulty() const;

    /// Get network hash rate
    double GetNetworkHashRate() const;

    // ------------------------------------------------------------------------
    // Block Validation
    // ------------------------------------------------------------------------

    /// Validate and accept block
    Result<void> ValidateBlock(const Block& block) const;

    /// Check if block is on main chain
    bool IsOnMainChain(const uint256& block_hash) const;

    /// Get block confirmations
    uint64_t GetBlockConfirmations(const uint256& block_hash) const;

    // ------------------------------------------------------------------------
    // Chain Reorganization
    // ------------------------------------------------------------------------

    /// Handle chain reorganization
    Result<void> Reorganize(const std::vector<Block>& new_chain);

    /// Find fork point between two chains
    Result<uint256> FindForkPoint(const uint256& hash1,
                                  const uint256& hash2) const;

    /// Get blocks from height
    Result<std::vector<Block>> GetBlocksFromHeight(uint64_t start_height,
                                                   size_t count) const;

    // ------------------------------------------------------------------------
    // Transaction Queries
    // ------------------------------------------------------------------------

    /// Get transaction by hash
    Result<Transaction> GetTransaction(const uint256& tx_hash) const;

    /// Check if transaction exists
    bool HasTransaction(const uint256& tx_hash) const;

    /// Get transaction confirmations
    uint64_t GetTransactionConfirmations(const uint256& tx_hash) const;

    /// Get transaction block
    Result<Block> GetTransactionBlock(const uint256& tx_hash) const;

    // ------------------------------------------------------------------------
    // UTXO Queries
    // ------------------------------------------------------------------------

    /// Get UTXO
    std::optional<TxOut> GetUTXO(const OutPoint& outpoint) const;

    /// Check if UTXO exists
    bool HasUTXO(const OutPoint& outpoint) const;

    /// Get UTXO set
    const UTXOSet& GetUTXOSet() const;

    /// Get UTXOs for address
    std::vector<std::pair<OutPoint, TxOut>> GetUTXOsForAddress(
        const std::string& address) const;

    /// Get address balance
    uint64_t GetAddressBalance(const std::string& address) const;

    // ------------------------------------------------------------------------
    // Block Mining Support
    // ------------------------------------------------------------------------

    /// Get block template for mining
    Result<Block> GetBlockTemplate(const PublicKey& miner_pubkey) const;

    /// Submit mined block
    Result<void> SubmitBlock(const Block& block);

    // ------------------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------------------

    /// Get blockchain info
    struct BlockchainInfo {
        uint64_t height;
        uint256 best_block_hash;
        uint256 chain_work;
        double difficulty;
        uint64_t total_transactions;
        uint64_t total_supply;
        uint64_t utxo_count;
        double verification_progress;
        bool pruned;
    };

    BlockchainInfo GetInfo() const;

    /// Get block stats
    struct BlockStats {
        uint64_t height;
        uint256 hash;
        uint64_t timestamp;
        uint32_t tx_count;
        uint64_t total_fees;
        uint64_t block_reward;
        uint32_t size;
        uint32_t weight;
        double difficulty;
    };

    Result<BlockStats> GetBlockStats(const uint256& block_hash) const;
    Result<BlockStats> GetBlockStatsByHeight(uint64_t height) const;

    // ------------------------------------------------------------------------
    // Mempool Access
    // ------------------------------------------------------------------------

    /// Get mempool
    Mempool& GetMempool();
    const Mempool& GetMempool() const;

    /// Add transaction to mempool
    Result<void> AddToMempool(const Transaction& tx);

    // ------------------------------------------------------------------------
    // Listeners/Callbacks
    // ------------------------------------------------------------------------

    /// Callback for new blocks
    using BlockCallback = std::function<void(const Block&)>;

    /// Callback for new transactions
    using TransactionCallback = std::function<void(const Transaction&)>;

    /// Register block callback
    void RegisterBlockCallback(BlockCallback callback);

    /// Register transaction callback
    void RegisterTransactionCallback(TransactionCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Block Validator (used by Blockchain)
// ============================================================================

class BlockValidator {
public:
    /// Constructor
    explicit BlockValidator(const Blockchain& chain);

    /// Validate block completely
    Result<void> Validate(const Block& block) const;

    /// Validate block header only
    Result<void> ValidateHeader(const BlockHeader& header) const;

    /// Validate block transactions
    Result<void> ValidateTransactions(const Block& block) const;

    /// Validate merkle root
    Result<void> ValidateMerkleRoot(const Block& block) const;

    /// Validate proof-of-work
    Result<void> ValidateProofOfWork(const BlockHeader& header) const;

    /// Validate timestamp
    Result<void> ValidateTimestamp(const BlockHeader& header) const;

    /// Validate difficulty
    Result<void> ValidateDifficulty(const BlockHeader& header) const;

private:
    const Blockchain& chain_;
};

// ============================================================================
// Transaction Validator (used by Blockchain and Mempool)
// ============================================================================

class TxValidator {
public:
    /// Constructor
    explicit TxValidator(const Blockchain& chain);

    /// Validate transaction completely
    Result<void> Validate(const Transaction& tx) const;

    /// Validate transaction structure
    Result<void> ValidateStructure(const Transaction& tx) const;

    /// Validate inputs
    Result<void> ValidateInputs(const Transaction& tx) const;

    /// Validate outputs
    Result<void> ValidateOutputs(const Transaction& tx) const;

    /// Validate signature
    Result<void> ValidateSignature(const Transaction& tx) const;

    /// Validate fees
    Result<void> ValidateFees(const Transaction& tx) const;

    /// Check double spend
    Result<void> CheckDoubleSpend(const Transaction& tx) const;

private:
    const Blockchain& chain_;
};

// ============================================================================
// Blockchain Iterator
// ============================================================================

class BlockchainIterator {
public:
    /// Constructor (start from best block)
    explicit BlockchainIterator(const Blockchain& chain);

    /// Constructor (start from specific block)
    BlockchainIterator(const Blockchain& chain, const uint256& start_hash);

    /// Get current block
    Result<Block> GetBlock() const;

    /// Move to previous block
    bool MovePrev();

    /// Move to next block
    bool MoveNext();

    /// Check if at genesis
    bool IsAtGenesis() const;

    /// Check if at best block
    bool IsAtBestBlock() const;

    /// Get current height
    uint64_t GetHeight() const;

private:
    const Blockchain& chain_;
    uint256 current_hash_;
    uint64_t current_height_;
};

// ============================================================================
// Utility Functions
// ============================================================================

/// Calculate block subsidy (reward without fees)
uint64_t CalculateBlockSubsidy(uint64_t height);

/// Calculate total supply at height
uint64_t CalculateTotalSupply(uint64_t height);

/// Check if height is halving height
bool IsHalvingHeight(uint64_t height);

/// Get next halving height
uint64_t GetNextHalvingHeight(uint64_t current_height);

/// Estimate blocks until halving
uint64_t EstimateBlocksUntilHalving(uint64_t current_height);

} // namespace intcoin

#endif // INTCOIN_BLOCKCHAIN_H
