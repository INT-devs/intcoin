/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Block Structure and Operations
 */

#ifndef INTCOIN_BLOCK_H
#define INTCOIN_BLOCK_H

#include "types.h"
#include "transaction.h"
#include <vector>
#include <memory>

namespace intcoin {

// ============================================================================
// Block Header
// ============================================================================

struct BlockHeader {
    /// Block version
    uint32_t version;

    /// Previous block hash
    uint256 prev_block_hash;

    /// Merkle root of transactions
    uint256 merkle_root;

    /// Block timestamp (Unix time)
    uint64_t timestamp;

    /// Difficulty target (compact format)
    uint32_t bits;

    /// RandomX nonce
    uint64_t nonce;

    /// RandomX hash (PoW result)
    uint256 randomx_hash;

    /// RandomX key
    uint256 randomx_key;

    /// Calculate block hash
    uint256 GetHash() const;

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<BlockHeader> Deserialize(const std::vector<uint8_t>& data);

    /// Get serialized size
    size_t GetSerializedSize() const;
};

// ============================================================================
// Block
// ============================================================================

class Block {
public:
    /// Block header
    BlockHeader header;

    /// Transactions in this block
    std::vector<Transaction> transactions;

    /// Default constructor
    Block() = default;

    /// Constructor with header and transactions
    Block(BlockHeader hdr, std::vector<Transaction> txs);

    /// Get block hash
    uint256 GetHash() const;

    /// Get block height (must query blockchain)
    uint64_t GetHeight() const;

    /// Calculate merkle root from transactions
    uint256 CalculateMerkleRoot() const;

    /// Verify block structure and PoW
    Result<void> Verify() const;

    /// Verify all transactions
    Result<void> VerifyTransactions() const;

    /// Get total transaction fees
    uint64_t GetTotalFees() const;

    /// Get coinbase transaction
    const Transaction& GetCoinbase() const;

    /// Check if this is genesis block
    bool IsGenesis() const;

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<Block> Deserialize(const std::vector<uint8_t>& data);

    /// Get serialized size
    size_t GetSerializedSize() const;

private:
    mutable std::optional<uint256> cached_hash_;
};

// ============================================================================
// Genesis Block
// ============================================================================

/// Create genesis block
Block CreateGenesisBlock();

/// Genesis block hash (computed once)
const uint256& GetGenesisBlockHash();

// ============================================================================
// Block Validation
// ============================================================================

/// Validate block header
Result<void> ValidateBlockHeader(const BlockHeader& header);

/// Validate block structure
Result<void> ValidateBlockStructure(const Block& block);

/// Validate block transactions
Result<void> ValidateBlockTransactions(const Block& block);

/// Validate block PoW
Result<void> ValidateProofOfWork(const BlockHeader& header);

// ============================================================================
// Merkle Tree
// ============================================================================

/// Calculate merkle root from transaction hashes
uint256 CalculateMerkleRoot(const std::vector<uint256>& tx_hashes);

/// Build merkle tree
std::vector<uint256> BuildMerkleTree(const std::vector<uint256>& tx_hashes);

/// Get merkle branch for transaction at index
std::vector<uint256> GetMerkleBranch(const std::vector<uint256>& tx_hashes, size_t index);

/// Verify merkle proof
bool VerifyMerkleProof(const uint256& tx_hash, const uint256& merkle_root,
                       const std::vector<uint256>& branch, size_t index);

} // namespace intcoin

#endif // INTCOIN_BLOCK_H
