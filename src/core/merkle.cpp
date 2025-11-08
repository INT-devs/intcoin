// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/merkle.h"
#include "intcoin/crypto.h"
#include <stdexcept>

namespace intcoin {

// MerkleProof implementation

bool MerkleProof::verify() const {
    Hash256 current = tx_hash;

    for (size_t i = 0; i < proof_hashes.size(); ++i) {
        std::vector<uint8_t> combined;

        if (i < proof_flags.size() && proof_flags[i]) {
            // Sibling is on the right
            combined.insert(combined.end(), current.begin(), current.end());
            combined.insert(combined.end(), proof_hashes[i].begin(), proof_hashes[i].end());
        } else {
            // Sibling is on the left
            combined.insert(combined.end(), proof_hashes[i].begin(), proof_hashes[i].end());
            combined.insert(combined.end(), current.begin(), current.end());
        }

        current = crypto::SHA3_256::hash(combined.data(), combined.size());
    }

    return current == root;
}

std::vector<uint8_t> MerkleProof::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize tx_hash (32 bytes)
    buffer.insert(buffer.end(), tx_hash.begin(), tx_hash.end());

    // Serialize merkle root (32 bytes)
    buffer.insert(buffer.end(), root.begin(), root.end());

    // Serialize number of proof hashes (4 bytes)
    uint32_t hash_count = static_cast<uint32_t>(proof_hashes.size());
    buffer.push_back(static_cast<uint8_t>(hash_count & 0xFF));
    buffer.push_back(static_cast<uint8_t>((hash_count >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((hash_count >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((hash_count >> 24) & 0xFF));

    // Serialize all proof hashes (32 bytes each)
    for (const auto& hash : proof_hashes) {
        buffer.insert(buffer.end(), hash.begin(), hash.end());
    }

    // Serialize flags (1 byte per flag)
    uint32_t flag_count = static_cast<uint32_t>(proof_flags.size());
    buffer.push_back(static_cast<uint8_t>(flag_count & 0xFF));
    buffer.push_back(static_cast<uint8_t>((flag_count >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((flag_count >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((flag_count >> 24) & 0xFF));

    for (bool flag : proof_flags) {
        buffer.push_back(flag ? 1 : 0);
    }

    return buffer;
}

MerkleProof MerkleProof::deserialize(const std::vector<uint8_t>& data) {
    MerkleProof proof;

    if (data.size() < 68) return proof;  // Minimum size: 32 (tx_hash) + 32 (root) + 4 (count)

    size_t offset = 0;

    // Deserialize tx_hash (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, proof.tx_hash.begin());
    offset += 32;

    // Deserialize root (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, proof.root.begin());
    offset += 32;

    // Deserialize hash count (4 bytes)
    uint32_t hash_count = static_cast<uint32_t>(data[offset]) |
                          (static_cast<uint32_t>(data[offset + 1]) << 8) |
                          (static_cast<uint32_t>(data[offset + 2]) << 16) |
                          (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Deserialize proof hashes (32 bytes each)
    if (data.size() < offset + (hash_count * 32)) return proof;  // Size check

    for (uint32_t i = 0; i < hash_count; ++i) {
        Hash256 hash;
        std::copy(data.begin() + offset, data.begin() + offset + 32, hash.begin());
        proof.proof_hashes.push_back(hash);
        offset += 32;
    }

    // Deserialize flag count (4 bytes)
    if (data.size() < offset + 4) return proof;
    uint32_t flag_count = static_cast<uint32_t>(data[offset]) |
                          (static_cast<uint32_t>(data[offset + 1]) << 8) |
                          (static_cast<uint32_t>(data[offset + 2]) << 16) |
                          (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Deserialize flags
    if (data.size() < offset + flag_count) return proof;
    for (uint32_t i = 0; i < flag_count; ++i) {
        proof.proof_flags.push_back(data[offset + i] != 0);
    }

    return proof;
}

// MerkleTree implementation

void MerkleTree::build(const std::vector<Hash256>& tx_hashes) {
    clear();

    if (tx_hashes.empty()) {
        return;
    }

    leaf_count_ = tx_hashes.size();

    // Create leaf nodes
    for (const auto& hash : tx_hashes) {
        leaves_.push_back(std::make_shared<MerkleNode>(hash));
    }

    // Build tree recursively
    root_ = build_recursive(leaves_);
}

void MerkleTree::build(const std::vector<Transaction>& transactions) {
    std::vector<Hash256> hashes;
    hashes.reserve(transactions.size());

    for (const auto& tx : transactions) {
        hashes.push_back(tx.get_hash());
    }

    build(hashes);
}

Hash256 MerkleTree::get_root() const {
    if (!root_) {
        return Hash256{};
    }
    return root_->hash;
}

std::optional<MerkleProof> MerkleTree::generate_proof(size_t tx_index) const {
    if (!root_ || tx_index >= leaf_count_) {
        return std::nullopt;
    }

    MerkleProof proof;
    proof.tx_hash = leaves_[tx_index]->hash;
    proof.root = root_->hash;

    generate_proof_recursive(root_, tx_index, 0, leaf_count_,
                            proof.proof_hashes, proof.proof_flags);

    return proof;
}

bool MerkleTree::verify_proof(const MerkleProof& proof) {
    return proof.verify();
}

Hash256 MerkleTree::calculate_root(const std::vector<Hash256>& hashes) {
    if (hashes.empty()) {
        return Hash256{};
    }

    if (hashes.size() == 1) {
        return hashes[0];
    }

    std::vector<Hash256> current_level = hashes;

    while (current_level.size() > 1) {
        std::vector<Hash256> next_level;

        for (size_t i = 0; i < current_level.size(); i += 2) {
            Hash256 left = current_level[i];
            Hash256 right = (i + 1 < current_level.size()) ? current_level[i + 1] : current_level[i];

            next_level.push_back(hash_pair(left, right));
        }

        current_level = std::move(next_level);
    }

    return current_level[0];
}

size_t MerkleTree::get_depth() const {
    if (!root_) {
        return 0;
    }
    return calculate_depth(root_);
}

std::shared_ptr<MerkleNode> MerkleTree::build_recursive(
    const std::vector<std::shared_ptr<MerkleNode>>& nodes
) {
    if (nodes.empty()) {
        return nullptr;
    }

    if (nodes.size() == 1) {
        return nodes[0];
    }

    std::vector<std::shared_ptr<MerkleNode>> parents;

    for (size_t i = 0; i < nodes.size(); i += 2) {
        auto left = nodes[i];
        auto right = (i + 1 < nodes.size()) ? nodes[i + 1] : nodes[i];

        auto parent = std::make_shared<MerkleNode>();
        parent->hash = hash_pair(left->hash, right->hash);
        parent->left = left;
        parent->right = right;

        parents.push_back(parent);
    }

    return build_recursive(parents);
}

Hash256 MerkleTree::hash_pair(const Hash256& left, const Hash256& right) {
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    return crypto::SHA3_256::hash(combined.data(), combined.size());
}

void MerkleTree::generate_proof_recursive(
    const std::shared_ptr<MerkleNode>& node,
    size_t index,
    size_t range_start,
    size_t range_end,
    std::vector<Hash256>& proof_hashes,
    std::vector<bool>& proof_flags
) const {
    if (!node || range_start >= range_end || node->is_leaf()) {
        return;
    }

    size_t mid = (range_start + range_end) / 2;

    if (index < mid) {
        // Target is in left subtree
        if (node->right) {
            proof_hashes.push_back(node->right->hash);
            proof_flags.push_back(true);  // Sibling is on right
        }
        generate_proof_recursive(node->left, index, range_start, mid,
                                proof_hashes, proof_flags);
    } else {
        // Target is in right subtree
        if (node->left) {
            proof_hashes.push_back(node->left->hash);
            proof_flags.push_back(false);  // Sibling is on left
        }
        generate_proof_recursive(node->right, index, mid, range_end,
                                proof_hashes, proof_flags);
    }
}

size_t MerkleTree::calculate_depth(const std::shared_ptr<MerkleNode>& node) const {
    if (!node || node->is_leaf()) {
        return 1;
    }

    size_t left_depth = node->left ? calculate_depth(node->left) : 0;
    size_t right_depth = node->right ? calculate_depth(node->right) : 0;

    return 1 + std::max(left_depth, right_depth);
}

// MerkleMountainRange implementation

void MerkleMountainRange::append(const Hash256& hash) {
    peaks_.push_back(hash);
    size_++;

    // Merge peaks if needed
    while (peaks_.size() > 1) {
        size_t last = peaks_.size() - 1;
        size_t second_last = peaks_.size() - 2;

        // Simplified merging logic
        Hash256 merged;
        std::vector<uint8_t> combined;
        combined.insert(combined.end(), peaks_[second_last].begin(), peaks_[second_last].end());
        combined.insert(combined.end(), peaks_[last].begin(), peaks_[last].end());
        merged = crypto::SHA3_256::hash(combined.data(), combined.size());

        peaks_.pop_back();
        peaks_.pop_back();
        peaks_.push_back(merged);
    }
}

Hash256 MerkleMountainRange::get_root() const {
    return bag_peaks();
}

std::optional<MerkleProof> MerkleMountainRange::generate_proof(size_t tx_index) const {
    // MMR proof generation
    if (tx_index >= size_) {
        return std::nullopt;  // Index out of range
    }

    MerkleProof proof;

    // MMR proofs require walking up the tree and collecting sibling hashes
    // For a complete implementation, we would need to store the full MMR structure
    // Since we only store peaks, we can provide a simplified proof using peaks

    // For now, provide the bagged peaks as the proof
    // A full implementation would traverse the MMR tree structure
    for (const auto& peak : peaks_) {
        proof.proof_hashes.push_back(peak);
        proof.proof_flags.push_back(false);  // Simplified: all peaks on left
    }

    // The root is the bagged peaks
    proof.root = bag_peaks();

    return proof;
}

Hash256 MerkleMountainRange::bag_peaks() const {
    if (peaks_.empty()) {
        return Hash256{};
    }

    if (peaks_.size() == 1) {
        return peaks_[0];
    }

    Hash256 result = peaks_[0];
    for (size_t i = 1; i < peaks_.size(); ++i) {
        std::vector<uint8_t> combined;
        combined.insert(combined.end(), result.begin(), result.end());
        combined.insert(combined.end(), peaks_[i].begin(), peaks_[i].end());
        result = crypto::SHA3_256::hash(combined.data(), combined.size());
    }

    return result;
}

} // namespace intcoin
