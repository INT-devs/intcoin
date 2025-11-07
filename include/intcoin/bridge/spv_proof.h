// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BRIDGE_SPV_PROOF_H
#define INTCOIN_BRIDGE_SPV_PROOF_H

#include "../primitives.h"
#include "../block.h"
#include "../transaction.h"
#include "atomic_swap.h"
#include <vector>
#include <optional>

namespace intcoin {
namespace bridge {

/**
 * Merkle proof for SPV (Simplified Payment Verification)
 *
 * Proves a transaction exists in a block without downloading entire block
 */
struct MerkleProof {
    Hash256 tx_hash;                    // Transaction hash
    std::vector<Hash256> merkle_path;   // Path to merkle root
    std::vector<bool> path_direction;   // Left (false) or right (true) at each level
    Hash256 merkle_root;                // Block's merkle root
    uint32_t tx_index;                  // Position in block

    MerkleProof() : tx_index(0) {}

    // Verify proof
    bool verify() const;

    // Calculate root from proof
    Hash256 calculate_root() const;
};

/**
 * Block header for SPV
 */
struct SPVBlockHeader {
    uint32_t version;
    Hash256 prev_block;
    Hash256 merkle_root;
    uint64_t timestamp;
    uint32_t bits;
    uint64_t nonce;
    uint32_t height;

    SPVBlockHeader() : version(0), timestamp(0), bits(0), nonce(0), height(0) {}

    // Calculate block hash
    Hash256 get_hash() const;

    // Verify proof of work
    bool verify_pow() const;
};

/**
 * SPV proof that a transaction is confirmed
 *
 * Includes block header, merkle proof, and confirmation count
 */
class SPVProof {
public:
    SPVProof();
    ~SPVProof();

    // Proof creation
    static SPVProof create(const Transaction& tx,
                          const Block& block,
                          uint32_t tx_index);

    static SPVProof create_from_header(const Transaction& tx,
                                       const SPVBlockHeader& header,
                                       const MerkleProof& merkle_proof);

    // Verification
    bool verify(const Hash256& expected_block_hash) const;
    bool verify_confirmations(uint32_t required_confirmations,
                             uint32_t current_height) const;

    // Proof data
    SPVBlockHeader get_block_header() const { return block_header_; }
    MerkleProof get_merkle_proof() const { return merkle_proof_; }
    Transaction get_transaction() const { return transaction_; }
    uint32_t get_confirmations(uint32_t current_height) const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<SPVProof> deserialize(const std::vector<uint8_t>& data);

private:
    SPVBlockHeader block_header_;
    MerkleProof merkle_proof_;
    Transaction transaction_;
    bool verified_;

    bool verify_merkle_proof() const;
    bool verify_block_header() const;
};

/**
 * SPV chain verifier
 *
 * Verifies a chain of block headers for SPV
 */
class SPVChainVerifier {
public:
    SPVChainVerifier(ChainType chain_type);
    ~SPVChainVerifier();

    // Add block headers
    bool add_header(const SPVBlockHeader& header);
    bool add_headers(const std::vector<SPVBlockHeader>& headers);

    // Verify chain
    bool verify_chain() const;
    bool verify_header_chain(const std::vector<SPVBlockHeader>& headers) const;

    // Get headers
    std::optional<SPVBlockHeader> get_header(const Hash256& block_hash) const;
    std::optional<SPVBlockHeader> get_header_at_height(uint32_t height) const;
    uint32_t get_tip_height() const { return tip_height_; }
    Hash256 get_tip_hash() const { return tip_hash_; }

    // Difficulty verification
    bool verify_difficulty(const SPVBlockHeader& header) const;
    bool verify_difficulty_transition(const SPVBlockHeader& prev,
                                      const SPVBlockHeader& next) const;

    // Confirmations
    uint32_t get_confirmations(const Hash256& block_hash) const;

private:
    ChainType chain_type_;
    std::map<uint32_t, SPVBlockHeader> headers_;  // height -> header
    std::unordered_map<Hash256, uint32_t> hash_to_height_;
    uint32_t tip_height_;
    Hash256 tip_hash_;
    mutable std::mutex headers_mutex_;

    // Chain parameters
    uint32_t get_difficulty_adjustment_interval() const;
    uint32_t get_target_spacing() const;
};

/**
 * Cross-chain transaction proof
 *
 * Proves a transaction occurred on another blockchain
 */
class CrossChainProof {
public:
    CrossChainProof();
    ~CrossChainProof();

    // Proof creation
    static CrossChainProof create(ChainType source_chain,
                                  const Transaction& tx,
                                  const SPVProof& spv_proof);

    // Verification
    bool verify(uint32_t required_confirmations = 6) const;
    bool verify_against_chain(const SPVChainVerifier& verifier) const;

    // Proof data
    ChainType get_source_chain() const { return source_chain_; }
    Transaction get_transaction() const { return spv_proof_.get_transaction(); }
    SPVProof get_spv_proof() const { return spv_proof_; }
    uint32_t get_block_height() const {
        return spv_proof_.get_block_header().height;
    }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<CrossChainProof> deserialize(
        const std::vector<uint8_t>& data);

private:
    ChainType source_chain_;
    SPVProof spv_proof_;
    std::vector<SPVBlockHeader> confirmation_headers_;
    bool verified_;
};

/**
 * Bridge relay for verifying cross-chain transactions
 */
class BridgeRelay {
public:
    BridgeRelay();
    ~BridgeRelay();

    // Add chain verifiers
    void add_chain_verifier(ChainType chain, std::shared_ptr<SPVChainVerifier> verifier);
    std::shared_ptr<SPVChainVerifier> get_chain_verifier(ChainType chain);

    // Verify proofs
    bool verify_proof(const CrossChainProof& proof,
                     uint32_t required_confirmations = 6);

    bool verify_swap_locked(const AtomicSwap& swap,
                           ChainType chain,
                           const CrossChainProof& proof);

    // Sync chain headers
    bool sync_headers(ChainType chain, const std::vector<SPVBlockHeader>& headers);
    uint32_t get_chain_height(ChainType chain) const;

    // Monitor transactions
    void monitor_swap(const Hash256& swap_id,
                     ChainType chain,
                     const Hash256& expected_txid);

    std::vector<CrossChainProof> get_pending_proofs() const;

private:
    std::unordered_map<ChainType, std::shared_ptr<SPVChainVerifier>> verifiers_;
    std::unordered_map<Hash256, CrossChainProof> pending_proofs_;
    mutable std::mutex relay_mutex_;

    // Callbacks
    std::function<void(const Hash256&, const CrossChainProof&)> proof_verified_callback_;
};

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_SPV_PROOF_H
