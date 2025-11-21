// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// SPV (Simplified Payment Verification) Proof Implementation

#include "intcoin/bridge/spv_proof.h"
#include "intcoin/crypto/sha256.h"
#include <algorithm>
#include <cstring>

namespace intcoin {
namespace bridge {

// ============================================================================
// MerkleProof Implementation
// ============================================================================

bool MerkleProof::verify() const {
    return calculate_root() == merkle_root;
}

Hash256 MerkleProof::calculate_root() const {
    if (merkle_path.size() != path_direction.size()) {
        return Hash256{};
    }

    Hash256 current = tx_hash;

    for (size_t i = 0; i < merkle_path.size(); ++i) {
        SHA256 hasher;

        if (path_direction[i]) {
            // Current is on the right
            hasher.update(merkle_path[i].data(), merkle_path[i].size());
            hasher.update(current.data(), current.size());
        } else {
            // Current is on the left
            hasher.update(current.data(), current.size());
            hasher.update(merkle_path[i].data(), merkle_path[i].size());
        }

        hasher.finalize(current.data());
    }

    return current;
}

// ============================================================================
// SPVBlockHeader Implementation
// ============================================================================

Hash256 SPVBlockHeader::get_hash() const {
    SHA256 hasher;

    // Hash block header fields
    hasher.update(reinterpret_cast<const uint8_t*>(&version), sizeof(version));
    hasher.update(prev_block.data(), prev_block.size());
    hasher.update(merkle_root.data(), merkle_root.size());
    hasher.update(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    hasher.update(reinterpret_cast<const uint8_t*>(&bits), sizeof(bits));
    hasher.update(reinterpret_cast<const uint8_t*>(&nonce), sizeof(nonce));

    Hash256 hash;
    hasher.finalize(hash.data());

    // Double SHA-256 for block hash
    SHA256 hasher2;
    hasher2.update(hash.data(), hash.size());
    hasher2.finalize(hash.data());

    return hash;
}

bool SPVBlockHeader::verify_pow() const {
    Hash256 hash = get_hash();

    // Convert bits to target
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00FFFFFF;

    // Check if hash is below target
    // For simplicity, we check the first few bytes
    uint32_t hash_value = 0;
    std::memcpy(&hash_value, hash.data(), sizeof(uint32_t));

    // Target check (simplified)
    if (exponent >= 32) {
        return true; // Very high difficulty
    }

    uint32_t target = mantissa << (8 * (exponent - 3));
    return hash_value <= target;
}

// ============================================================================
// SPVProof Implementation
// ============================================================================

SPVProof::SPVProof() : verified_(false) {}

SPVProof::~SPVProof() = default;

SPVProof SPVProof::create(const Transaction& tx,
                         const Block& block,
                         uint32_t tx_index) {
    SPVProof proof;
    proof.transaction_ = tx;

    // Create block header
    proof.block_header_.version = block.version;
    proof.block_header_.prev_block = block.prev_block;
    proof.block_header_.merkle_root = block.merkle_root;
    proof.block_header_.timestamp = block.timestamp;
    proof.block_header_.bits = block.bits;
    proof.block_header_.nonce = block.nonce;
    proof.block_header_.height = block.height;

    // Create merkle proof
    proof.merkle_proof_.tx_hash = tx.get_hash();
    proof.merkle_proof_.tx_index = tx_index;
    proof.merkle_proof_.merkle_root = block.merkle_root;

    // Build merkle path
    std::vector<Hash256> tx_hashes;
    for (const auto& transaction : block.transactions) {
        tx_hashes.push_back(transaction.get_hash());
    }

    uint32_t index = tx_index;
    while (tx_hashes.size() > 1) {
        std::vector<Hash256> next_level;

        for (size_t i = 0; i < tx_hashes.size(); i += 2) {
            if (i == tx_hashes.size() - 1) {
                // Odd number of hashes, duplicate last one
                next_level.push_back(tx_hashes[i]);
                if (i == index) {
                    index = next_level.size() - 1;
                }
            } else {
                // Store sibling in merkle path
                if (i == index || i + 1 == index) {
                    if (i == index) {
                        proof.merkle_proof_.merkle_path.push_back(tx_hashes[i + 1]);
                        proof.merkle_proof_.path_direction.push_back(false);
                    } else {
                        proof.merkle_proof_.merkle_path.push_back(tx_hashes[i]);
                        proof.merkle_proof_.path_direction.push_back(true);
                    }
                }

                // Combine hashes
                SHA256 hasher;
                hasher.update(tx_hashes[i].data(), tx_hashes[i].size());
                hasher.update(tx_hashes[i + 1].data(), tx_hashes[i + 1].size());
                Hash256 combined;
                hasher.finalize(combined.data());
                next_level.push_back(combined);

                if (i == index || i + 1 == index) {
                    index = next_level.size() - 1;
                }
            }
        }

        tx_hashes = std::move(next_level);
    }

    return proof;
}

SPVProof SPVProof::create_from_header(const Transaction& tx,
                                     const SPVBlockHeader& header,
                                     const MerkleProof& merkle_proof) {
    SPVProof proof;
    proof.transaction_ = tx;
    proof.block_header_ = header;
    proof.merkle_proof_ = merkle_proof;
    return proof;
}

bool SPVProof::verify(const Hash256& expected_block_hash) const {
    // Verify block header
    if (!verify_block_header()) {
        return false;
    }

    // Check block hash matches
    if (block_header_.get_hash() != expected_block_hash) {
        return false;
    }

    // Verify merkle proof
    return verify_merkle_proof();
}

bool SPVProof::verify_confirmations(uint32_t required_confirmations,
                                   uint32_t current_height) const {
    uint32_t confirmations = get_confirmations(current_height);
    return confirmations >= required_confirmations;
}

uint32_t SPVProof::get_confirmations(uint32_t current_height) const {
    if (current_height < block_header_.height) {
        return 0;
    }
    return current_height - block_header_.height + 1;
}

std::vector<uint8_t> SPVProof::serialize() const {
    std::vector<uint8_t> data;

    // Serialize block header
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&block_header_.version),
                reinterpret_cast<const uint8_t*>(&block_header_.version) + sizeof(uint32_t));
    data.insert(data.end(), block_header_.prev_block.begin(), block_header_.prev_block.end());
    data.insert(data.end(), block_header_.merkle_root.begin(), block_header_.merkle_root.end());
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&block_header_.timestamp),
                reinterpret_cast<const uint8_t*>(&block_header_.timestamp) + sizeof(uint64_t));
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&block_header_.bits),
                reinterpret_cast<const uint8_t*>(&block_header_.bits) + sizeof(uint32_t));
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&block_header_.nonce),
                reinterpret_cast<const uint8_t*>(&block_header_.nonce) + sizeof(uint64_t));
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&block_header_.height),
                reinterpret_cast<const uint8_t*>(&block_header_.height) + sizeof(uint32_t));

    // Serialize merkle proof
    data.insert(data.end(), merkle_proof_.tx_hash.begin(), merkle_proof_.tx_hash.end());
    data.insert(data.end(), merkle_proof_.merkle_root.begin(), merkle_proof_.merkle_root.end());
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&merkle_proof_.tx_index),
                reinterpret_cast<const uint8_t*>(&merkle_proof_.tx_index) + sizeof(uint32_t));

    uint32_t path_size = merkle_proof_.merkle_path.size();
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&path_size),
                reinterpret_cast<const uint8_t*>(&path_size) + sizeof(uint32_t));

    for (const auto& hash : merkle_proof_.merkle_path) {
        data.insert(data.end(), hash.begin(), hash.end());
    }

    for (bool dir : merkle_proof_.path_direction) {
        data.push_back(dir ? 1 : 0);
    }

    // Serialize transaction
    auto tx_data = transaction_.serialize();
    uint32_t tx_size = tx_data.size();
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&tx_size),
                reinterpret_cast<const uint8_t*>(&tx_size) + sizeof(uint32_t));
    data.insert(data.end(), tx_data.begin(), tx_data.end());

    return data;
}

std::optional<SPVProof> SPVProof::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 100) {  // Minimum size check
        return std::nullopt;
    }

    SPVProof proof;
    size_t offset = 0;

    // Deserialize block header
    std::memcpy(&proof.block_header_.version, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(proof.block_header_.prev_block.data(), &data[offset], 32);
    offset += 32;

    std::memcpy(proof.block_header_.merkle_root.data(), &data[offset], 32);
    offset += 32;

    std::memcpy(&proof.block_header_.timestamp, &data[offset], sizeof(uint64_t));
    offset += sizeof(uint64_t);

    std::memcpy(&proof.block_header_.bits, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::memcpy(&proof.block_header_.nonce, &data[offset], sizeof(uint64_t));
    offset += sizeof(uint64_t);

    std::memcpy(&proof.block_header_.height, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Deserialize merkle proof
    std::memcpy(proof.merkle_proof_.tx_hash.data(), &data[offset], 32);
    offset += 32;

    std::memcpy(proof.merkle_proof_.merkle_root.data(), &data[offset], 32);
    offset += 32;

    std::memcpy(&proof.merkle_proof_.tx_index, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    uint32_t path_size;
    std::memcpy(&path_size, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < path_size; ++i) {
        Hash256 hash;
        std::memcpy(hash.data(), &data[offset], 32);
        offset += 32;
        proof.merkle_proof_.merkle_path.push_back(hash);
    }

    for (uint32_t i = 0; i < path_size; ++i) {
        proof.merkle_proof_.path_direction.push_back(data[offset++] != 0);
    }

    // Deserialize transaction
    uint32_t tx_size;
    std::memcpy(&tx_size, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::vector<uint8_t> tx_data(data.begin() + offset, data.begin() + offset + tx_size);
    auto tx_opt = Transaction::deserialize(tx_data);
    if (!tx_opt) {
        return std::nullopt;
    }
    proof.transaction_ = *tx_opt;

    return proof;
}

bool SPVProof::verify_merkle_proof() const {
    // Check tx hash matches
    if (transaction_.get_hash() != merkle_proof_.tx_hash) {
        return false;
    }

    // Verify merkle proof
    if (!merkle_proof_.verify()) {
        return false;
    }

    // Check merkle root matches block header
    return merkle_proof_.merkle_root == block_header_.merkle_root;
}

bool SPVProof::verify_block_header() const {
    // Verify proof of work
    return block_header_.verify_pow();
}

// ============================================================================
// SPVChainVerifier Implementation
// ============================================================================

SPVChainVerifier::SPVChainVerifier(ChainType chain_type)
    : chain_type_(chain_type)
    , tip_height_(0) {}

SPVChainVerifier::~SPVChainVerifier() = default;

bool SPVChainVerifier::add_header(const SPVBlockHeader& header) {
    std::lock_guard<std::mutex> lock(headers_mutex_);

    // Verify header connects to existing chain
    if (header.height > 0 && headers_.count(header.height - 1) == 0) {
        return false;  // Missing previous header
    }

    // Verify previous block hash
    if (header.height > 0) {
        const auto& prev = headers_[header.height - 1];
        if (prev.get_hash() != header.prev_block) {
            return false;  // Invalid chain
        }
    }

    // Verify proof of work
    if (!header.verify_pow()) {
        return false;
    }

    // Verify difficulty if needed
    if (header.height > 0 && !verify_difficulty(header)) {
        return false;
    }

    // Add header
    headers_[header.height] = header;
    hash_to_height_[header.get_hash()] = header.height;

    // Update tip
    if (header.height >= tip_height_) {
        tip_height_ = header.height;
        tip_hash_ = header.get_hash();
    }

    return true;
}

bool SPVChainVerifier::add_headers(const std::vector<SPVBlockHeader>& headers) {
    for (const auto& header : headers) {
        if (!add_header(header)) {
            return false;
        }
    }
    return true;
}

bool SPVChainVerifier::verify_chain() const {
    std::lock_guard<std::mutex> lock(headers_mutex_);

    if (headers_.empty()) {
        return true;
    }

    // Check all headers form a valid chain
    for (uint32_t height = 1; height <= tip_height_; ++height) {
        if (headers_.count(height) == 0 || headers_.count(height - 1) == 0) {
            return false;
        }

        const auto& current = headers_.at(height);
        const auto& prev = headers_.at(height - 1);

        if (current.prev_block != prev.get_hash()) {
            return false;
        }

        if (!current.verify_pow()) {
            return false;
        }
    }

    return true;
}

bool SPVChainVerifier::verify_header_chain(const std::vector<SPVBlockHeader>& headers) const {
    if (headers.empty()) {
        return true;
    }

    for (size_t i = 1; i < headers.size(); ++i) {
        if (headers[i].prev_block != headers[i - 1].get_hash()) {
            return false;
        }
        if (!headers[i].verify_pow()) {
            return false;
        }
    }

    return true;
}

std::optional<SPVBlockHeader> SPVChainVerifier::get_header(const Hash256& block_hash) const {
    std::lock_guard<std::mutex> lock(headers_mutex_);

    auto it = hash_to_height_.find(block_hash);
    if (it == hash_to_height_.end()) {
        return std::nullopt;
    }

    return headers_.at(it->second);
}

std::optional<SPVBlockHeader> SPVChainVerifier::get_header_at_height(uint32_t height) const {
    std::lock_guard<std::mutex> lock(headers_mutex_);

    auto it = headers_.find(height);
    if (it == headers_.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool SPVChainVerifier::verify_difficulty(const SPVBlockHeader& header) const {
    // Simplified difficulty verification
    // In production, this would check difficulty adjustment rules
    uint32_t interval = get_difficulty_adjustment_interval();

    if (header.height % interval != 0) {
        // Not a difficulty adjustment block, check it matches previous
        auto prev_opt = get_header_at_height(header.height - 1);
        if (!prev_opt) {
            return false;
        }
        return header.bits == prev_opt->bits;
    }

    return true;  // Accept difficulty adjustment blocks
}

bool SPVChainVerifier::verify_difficulty_transition(const SPVBlockHeader& prev,
                                                    const SPVBlockHeader& next) const {
    // Simplified - in production, check difficulty adjustment algorithm
    return true;
}

uint32_t SPVChainVerifier::get_confirmations(const Hash256& block_hash) const {
    std::lock_guard<std::mutex> lock(headers_mutex_);

    auto it = hash_to_height_.find(block_hash);
    if (it == hash_to_height_.end()) {
        return 0;
    }

    return tip_height_ - it->second + 1;
}

uint32_t SPVChainVerifier::get_difficulty_adjustment_interval() const {
    switch (chain_type_) {
        case ChainType::BITCOIN:
        case ChainType::LITECOIN:
            return 2016;  // 2 weeks
        case ChainType::ETHEREUM:
            return 1;     // Each block
        default:
            return 1440;  // 1 day
    }
}

uint32_t SPVChainVerifier::get_target_spacing() const {
    switch (chain_type_) {
        case ChainType::BITCOIN:
            return 600;   // 10 minutes
        case ChainType::ETHEREUM:
            return 12;    // 12 seconds
        case ChainType::LITECOIN:
            return 150;   // 2.5 minutes
        default:
            return 60;    // 1 minute
    }
}

// ============================================================================
// CrossChainProof Implementation
// ============================================================================

CrossChainProof::CrossChainProof()
    : source_chain_(ChainType::BITCOIN)
    , verified_(false) {}

CrossChainProof::~CrossChainProof() = default;

CrossChainProof CrossChainProof::create(ChainType source_chain,
                                       const Transaction& tx,
                                       const SPVProof& spv_proof) {
    CrossChainProof proof;
    proof.source_chain_ = source_chain;
    proof.spv_proof_ = spv_proof;
    return proof;
}

bool CrossChainProof::verify(uint32_t required_confirmations) const {
    // Verify SPV proof
    Hash256 block_hash = spv_proof_.get_block_header().get_hash();
    if (!spv_proof_.verify(block_hash)) {
        return false;
    }

    // Check confirmations (if we have confirmation headers)
    if (!confirmation_headers_.empty()) {
        uint32_t current_height = spv_proof_.get_block_header().height +
                                 confirmation_headers_.size();
        return spv_proof_.verify_confirmations(required_confirmations, current_height);
    }

    return true;
}

bool CrossChainProof::verify_against_chain(const SPVChainVerifier& verifier) const {
    Hash256 block_hash = spv_proof_.get_block_header().get_hash();

    // Check block exists in verifier's chain
    auto header_opt = verifier.get_header(block_hash);
    if (!header_opt) {
        return false;
    }

    // Verify SPV proof
    return spv_proof_.verify(block_hash);
}

std::vector<uint8_t> CrossChainProof::serialize() const {
    std::vector<uint8_t> data;

    // Serialize source chain
    data.push_back(static_cast<uint8_t>(source_chain_));

    // Serialize SPV proof
    auto spv_data = spv_proof_.serialize();
    uint32_t spv_size = spv_data.size();
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&spv_size),
                reinterpret_cast<const uint8_t*>(&spv_size) + sizeof(uint32_t));
    data.insert(data.end(), spv_data.begin(), spv_data.end());

    // Serialize confirmation headers count
    uint32_t conf_count = confirmation_headers_.size();
    data.insert(data.end(),
                reinterpret_cast<const uint8_t*>(&conf_count),
                reinterpret_cast<const uint8_t*>(&conf_count) + sizeof(uint32_t));

    // TODO: Serialize confirmation headers

    return data;
}

std::optional<CrossChainProof> CrossChainProof::deserialize(
    const std::vector<uint8_t>& data) {
    if (data.size() < 10) {
        return std::nullopt;
    }

    CrossChainProof proof;
    size_t offset = 0;

    // Deserialize source chain
    proof.source_chain_ = static_cast<ChainType>(data[offset++]);

    // Deserialize SPV proof
    uint32_t spv_size;
    std::memcpy(&spv_size, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::vector<uint8_t> spv_data(data.begin() + offset,
                                  data.begin() + offset + spv_size);
    auto spv_opt = SPVProof::deserialize(spv_data);
    if (!spv_opt) {
        return std::nullopt;
    }
    proof.spv_proof_ = *spv_opt;

    return proof;
}

// ============================================================================
// BridgeRelay Implementation
// ============================================================================

BridgeRelay::BridgeRelay() = default;

BridgeRelay::~BridgeRelay() = default;

void BridgeRelay::add_chain_verifier(ChainType chain,
                                    std::shared_ptr<SPVChainVerifier> verifier) {
    std::lock_guard<std::mutex> lock(relay_mutex_);
    verifiers_[chain] = verifier;
}

std::shared_ptr<SPVChainVerifier> BridgeRelay::get_chain_verifier(ChainType chain) {
    std::lock_guard<std::mutex> lock(relay_mutex_);

    auto it = verifiers_.find(chain);
    if (it == verifiers_.end()) {
        return nullptr;
    }

    return it->second;
}

bool BridgeRelay::verify_proof(const CrossChainProof& proof,
                              uint32_t required_confirmations) {
    auto verifier = get_chain_verifier(proof.get_source_chain());
    if (!verifier) {
        return false;
    }

    // Verify proof against chain
    if (!proof.verify_against_chain(*verifier)) {
        return false;
    }

    // Check confirmations
    Hash256 block_hash = proof.get_spv_proof().get_block_header().get_hash();
    uint32_t confirmations = verifier->get_confirmations(block_hash);

    return confirmations >= required_confirmations;
}

bool BridgeRelay::verify_swap_locked(const AtomicSwap& swap,
                                    ChainType chain,
                                    const CrossChainProof& proof) {
    // Verify proof
    if (!verify_proof(proof, 6)) {
        return false;
    }

    // Verify transaction matches swap parameters
    const auto& tx = proof.get_transaction();

    // TODO: Check transaction locks funds to correct HTLC address
    // TODO: Verify amount matches swap
    // TODO: Verify hash lock matches

    return true;
}

bool BridgeRelay::sync_headers(ChainType chain,
                              const std::vector<SPVBlockHeader>& headers) {
    auto verifier = get_chain_verifier(chain);
    if (!verifier) {
        return false;
    }

    return verifier->add_headers(headers);
}

uint32_t BridgeRelay::get_chain_height(ChainType chain) const {
    std::lock_guard<std::mutex> lock(relay_mutex_);

    auto it = verifiers_.find(chain);
    if (it == verifiers_.end()) {
        return 0;
    }

    return it->second->get_tip_height();
}

void BridgeRelay::monitor_swap(const Hash256& swap_id,
                              ChainType chain,
                              const Hash256& expected_txid) {
    std::lock_guard<std::mutex> lock(relay_mutex_);

    // TODO: Implement swap monitoring
    // Store swap monitoring info for later verification
}

std::vector<CrossChainProof> BridgeRelay::get_pending_proofs() const {
    std::lock_guard<std::mutex> lock(relay_mutex_);

    std::vector<CrossChainProof> proofs;
    for (const auto& [id, proof] : pending_proofs_) {
        proofs.push_back(proof);
    }

    return proofs;
}

} // namespace bridge
} // namespace intcoin
