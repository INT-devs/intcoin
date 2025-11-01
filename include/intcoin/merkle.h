// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Merkle tree implementation for efficient transaction verification.

#ifndef INTCOIN_MERKLE_H
#define INTCOIN_MERKLE_H

#include "primitives.h"
#include "transaction.h"
#include <vector>
#include <optional>
#include <memory>

namespace intcoin {

// Forward declaration
struct MerkleNode;

/**
 * Merkle tree node
 */
struct MerkleNode {
    Hash256 hash;
    std::shared_ptr<MerkleNode> left;
    std::shared_ptr<MerkleNode> right;

    MerkleNode() : hash{}, left(nullptr), right(nullptr) {}

    MerkleNode(const Hash256& h) : hash(h), left(nullptr), right(nullptr) {}

    bool is_leaf() const {
        return left == nullptr && right == nullptr;
    }
};

/**
 * Merkle proof (for SPV clients)
 * Proves that a transaction is included in a block
 */
struct MerkleProof {
    Hash256 tx_hash;                    // Transaction being proven
    std::vector<Hash256> proof_hashes;  // Hashes needed for verification
    std::vector<bool> proof_flags;      // Left (false) or right (true) sibling
    Hash256 root;                       // Expected merkle root

    MerkleProof() : tx_hash{}, proof_hashes(), proof_flags(), root{} {}

    // Verify that the proof is valid
    bool verify() const;

    // Serialize for transmission
    std::vector<uint8_t> serialize() const;

    // Deserialize
    static MerkleProof deserialize(const std::vector<uint8_t>& data);
};

/**
 * Merkle tree implementation
 * Efficiently organizes and verifies transactions
 */
class MerkleTree {
public:
    MerkleTree() : root_(nullptr) {}

    /**
     * Build tree from transaction hashes
     */
    void build(const std::vector<Hash256>& tx_hashes);

    /**
     * Build tree from transactions
     */
    void build(const std::vector<Transaction>& transactions);

    /**
     * Get merkle root
     */
    Hash256 get_root() const;

    /**
     * Generate proof for transaction at index
     */
    std::optional<MerkleProof> generate_proof(size_t tx_index) const;

    /**
     * Verify a merkle proof
     */
    static bool verify_proof(const MerkleProof& proof);

    /**
     * Calculate merkle root directly from hashes (static utility)
     */
    static Hash256 calculate_root(const std::vector<Hash256>& hashes);

    /**
     * Get tree depth
     */
    size_t get_depth() const;

    /**
     * Get total number of leaves
     */
    size_t get_leaf_count() const {
        return leaf_count_;
    }

    /**
     * Clear the tree
     */
    void clear() {
        root_ = nullptr;
        leaves_.clear();
        leaf_count_ = 0;
    }

private:
    std::shared_ptr<MerkleNode> root_;
    std::vector<std::shared_ptr<MerkleNode>> leaves_;
    size_t leaf_count_ = 0;

    /**
     * Build tree recursively
     */
    std::shared_ptr<MerkleNode> build_recursive(
        const std::vector<std::shared_ptr<MerkleNode>>& nodes
    );

    /**
     * Hash two child nodes together
     */
    static Hash256 hash_pair(const Hash256& left, const Hash256& right);

    /**
     * Generate proof recursively
     */
    void generate_proof_recursive(
        const std::shared_ptr<MerkleNode>& node,
        size_t index,
        size_t range_start,
        size_t range_end,
        std::vector<Hash256>& proof_hashes,
        std::vector<bool>& proof_flags
    ) const;

    /**
     * Calculate tree depth recursively
     */
    size_t calculate_depth(const std::shared_ptr<MerkleNode>& node) const;
};

/**
 * Merkle mountain range (for efficient append-only trees)
 * Used in some advanced blockchain designs
 */
class MerkleMountainRange {
public:
    MerkleMountainRange() : peaks_(), size_(0) {}

    /**
     * Append a new element
     */
    void append(const Hash256& hash);

    /**
     * Get the root (bag of peaks)
     */
    Hash256 get_root() const;

    /**
     * Get current size
     */
    size_t size() const {
        return size_;
    }

    /**
     * Generate proof for element at index
     */
    std::optional<MerkleProof> generate_proof(size_t index) const;

    /**
     * Clear the MMR
     */
    void clear() {
        peaks_.clear();
        size_ = 0;
    }

private:
    std::vector<Hash256> peaks_;
    size_t size_;

    /**
     * Bag the peaks into a single hash
     */
    Hash256 bag_peaks() const;
};

} // namespace intcoin

#endif // INTCOIN_MERKLE_H
