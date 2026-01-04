// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_PRIVACY_CONFIDENTIAL_TRANSACTIONS_H
#define INTCOIN_PRIVACY_CONFIDENTIAL_TRANSACTIONS_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <array>

namespace intcoin {
namespace privacy {

/**
 * @brief Confidential Transactions (Pedersen Commitments + Range Proofs)
 *
 * Implements confidential transactions to hide transaction amounts while
 * ensuring no inflation. Uses Pedersen commitments with Bulletproofs
 * for efficient range proofs.
 *
 * Based on Elements/Liquid implementation of confidential transactions.
 */

// 32-byte commitment
using Commitment = std::array<uint8_t, 32>;

// Blinding factor for commitment
using BlindingFactor = std::array<uint8_t, 32>;

// Pedersen commitment: C = aG + bH where a=amount, b=blinding factor
struct PedersenCommitment {
    Commitment commitment;
    BlindingFactor blinding_factor;  // Secret
    uint64_t amount;                 // Secret
};

// Range proof ensures amount is in valid range [0, 2^64)
struct RangeProof {
    std::vector<uint8_t> proof_data;
    uint32_t proof_size{0};
    uint64_t min_value{0};
    uint64_t max_value{0};
};

// Confidential transaction output
struct ConfidentialOutput {
    Commitment amount_commitment;     // Pedersen commitment to amount
    RangeProof range_proof;          // Bulletproof proving amount is valid
    std::vector<uint8_t> encrypted_amount; // Amount encrypted for recipient
};

// Confidential transaction
struct ConfidentialTransaction {
    std::vector<ConfidentialOutput> outputs;
    std::vector<Commitment> input_commitments;
    Commitment fee_commitment;        // Fee is public but committed
    std::vector<uint8_t> signature;   // Transaction signature
};

/**
 * @class ConfidentialTransactionManager
 * @brief Manages confidential transaction creation and verification
 */
class ConfidentialTransactionManager {
public:
    ConfidentialTransactionManager();
    ~ConfidentialTransactionManager();

    /**
     * Create Pedersen commitment to amount
     *
     * @param amount Amount to commit to
     * @param blinding_factor Blinding factor (random if nullptr)
     * @return Pedersen commitment
     */
    PedersenCommitment CreateCommitment(
        uint64_t amount,
        const BlindingFactor* blinding_factor = nullptr
    );

    /**
     * Verify commitment is valid
     */
    bool VerifyCommitment(const PedersenCommitment& commitment);

    /**
     * Generate blinding factor
     */
    BlindingFactor GenerateBlindingFactor();

    /**
     * Create range proof (Bulletproofs)
     *
     * @param amount Amount to prove is in range
     * @param blinding_factor Blinding factor used in commitment
     * @param min_value Minimum value (default 0)
     * @param max_value Maximum value (default 2^64-1)
     * @return Range proof
     */
    RangeProof CreateRangeProof(
        uint64_t amount,
        const BlindingFactor& blinding_factor,
        uint64_t min_value = 0,
        uint64_t max_value = UINT64_MAX
    );

    /**
     * Verify range proof
     *
     * @param commitment Commitment to amount
     * @param range_proof Range proof to verify
     * @return true if proof is valid
     */
    bool VerifyRangeProof(
        const Commitment& commitment,
        const RangeProof& range_proof
    );

    /**
     * Create confidential output
     *
     * @param amount Output amount
     * @param recipient_public_key Recipient's public key (for encrypting amount)
     * @return Confidential output
     */
    ConfidentialOutput CreateOutput(
        uint64_t amount,
        const std::array<uint8_t, 32>& recipient_public_key
    );

    /**
     * Decrypt confidential output (for recipient)
     *
     * @param output Confidential output
     * @param private_key Recipient's private key
     * @return Decrypted amount
     */
    uint64_t DecryptOutput(
        const ConfidentialOutput& output,
        const std::array<uint8_t, 32>& private_key
    );

    /**
     * Verify confidential transaction
     *
     * Checks that:
     * 1. Sum of input commitments = sum of output commitments + fee
     * 2. All range proofs are valid
     * 3. No negative amounts (proven by range proofs)
     */
    bool VerifyTransaction(const ConfidentialTransaction& tx);

    /**
     * Create confidential transaction
     *
     * @param input_amounts Input amounts
     * @param input_blinding_factors Input blinding factors
     * @param output_amounts Output amounts
     * @param output_recipients Output recipient public keys
     * @param fee Transaction fee (public)
     * @return Confidential transaction
     */
    ConfidentialTransaction CreateTransaction(
        const std::vector<uint64_t>& input_amounts,
        const std::vector<BlindingFactor>& input_blinding_factors,
        const std::vector<uint64_t>& output_amounts,
        const std::vector<std::array<uint8_t, 32>>& output_recipients,
        uint64_t fee
    );

    /**
     * Aggregate commitments (for verification)
     *
     * Sum multiple commitments: C1 + C2 + ... + Cn
     */
    Commitment AggregateCommitments(const std::vector<Commitment>& commitments);

    /**
     * Subtract commitments: C1 - C2
     */
    Commitment SubtractCommitments(const Commitment& c1, const Commitment& c2);

    /**
     * Check if commitment equation balances
     *
     * Verifies: sum(inputs) - sum(outputs) - fee = 0
     */
    bool VerifyCommitmentBalance(
        const std::vector<Commitment>& input_commitments,
        const std::vector<Commitment>& output_commitments,
        const Commitment& fee_commitment
    );

    /**
     * Batch verify multiple range proofs (more efficient)
     */
    bool BatchVerifyRangeProofs(
        const std::vector<Commitment>& commitments,
        const std::vector<RangeProof>& proofs
    );

    // Bulletproofs settings
    struct BulletproofsConfig {
        uint32_t max_proof_size{674};  // Bytes for 64-bit range proof
        uint32_t aggregation_size{1};  // Number of proofs to aggregate
        bool use_batch_verification{true};
    };

    void SetBulletproofsConfig(const BulletproofsConfig& config);
    BulletproofsConfig GetBulletproofsConfig() const;

    // Statistics
    struct ConfidentialStats {
        uint64_t total_commitments_created{0};
        uint64_t total_range_proofs_created{0};
        uint64_t total_range_proofs_verified{0};
        uint64_t total_transactions_verified{0};
        uint64_t verification_failures{0};
        double avg_proof_size{0.0};
    };

    ConfidentialStats GetStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace privacy
} // namespace intcoin

#endif // INTCOIN_PRIVACY_CONFIDENTIAL_TRANSACTIONS_H
