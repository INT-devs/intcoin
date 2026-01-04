// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_PRIVACY_RING_SIGNATURES_H
#define INTCOIN_PRIVACY_RING_SIGNATURES_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <array>

namespace intcoin {
namespace privacy {

/**
 * @brief Ring Signatures (Borromean/MLSAG) for Transaction Privacy
 *
 * Implements ring signatures to hide the true sender among a group of decoys.
 * Based on Monero's MLSAG (Multilayered Linkable Spontaneous Anonymous Group) signatures.
 */

// 32-byte public key
using PublicKey = std::array<uint8_t, 32>;

// 32-byte private key
using PrivateKey = std::array<uint8_t, 32>;

// 32-byte key image (prevents double-spending)
using KeyImage = std::array<uint8_t, 32>;

// 64-byte signature component
using SignatureComponent = std::array<uint8_t, 64>;

// Ring signature structure
struct RingSignature {
    std::vector<SignatureComponent> c;  // Challenge values
    std::vector<SignatureComponent> r;  // Response values
    KeyImage key_image;                 // Key image for double-spend prevention
    std::vector<PublicKey> ring;        // Public keys in the ring (including real + decoys)
};

// Transaction output for ring signature
struct TxOutput {
    PublicKey public_key;
    uint64_t amount;                    // Encrypted in production
    std::string output_id;
};

/**
 * @class RingSignatureManager
 * @brief Manages ring signature generation and verification
 */
class RingSignatureManager {
public:
    RingSignatureManager();
    ~RingSignatureManager();

    /**
     * Generate a new key pair
     */
    struct KeyPair {
        PublicKey public_key;
        PrivateKey private_key;
    };

    KeyPair GenerateKeyPair();

    /**
     * Generate ring signature
     *
     * @param message Message to sign (typically transaction hash)
     * @param real_output The real output being spent
     * @param private_key Private key of real output
     * @param decoy_outputs Decoy outputs to mix with (ring members)
     * @param ring_size Total ring size (real + decoys)
     * @return Ring signature
     */
    RingSignature Sign(
        const std::vector<uint8_t>& message,
        const TxOutput& real_output,
        const PrivateKey& private_key,
        const std::vector<TxOutput>& decoy_outputs,
        size_t ring_size = 11  // Default ring size of 11 (1 real + 10 decoys)
    );

    /**
     * Verify ring signature
     *
     * @param message Message that was signed
     * @param signature Ring signature to verify
     * @return true if signature is valid
     */
    bool Verify(
        const std::vector<uint8_t>& message,
        const RingSignature& signature
    );

    /**
     * Check if key image has been used (double-spend detection)
     */
    bool IsKeyImageSpent(const KeyImage& key_image) const;

    /**
     * Mark key image as spent
     */
    void MarkKeyImageSpent(const KeyImage& key_image);

    /**
     * Select decoy outputs for ring
     *
     * Uses gamma distribution for realistic selection based on output age.
     */
    std::vector<TxOutput> SelectDecoys(
        const std::vector<TxOutput>& available_outputs,
        size_t num_decoys,
        uint64_t min_confirmations = 10
    );

    /**
     * Serialize/deserialize ring signature
     */
    std::vector<uint8_t> SerializeSignature(const RingSignature& signature) const;
    RingSignature DeserializeSignature(const std::vector<uint8_t>& data) const;

    // Statistics
    struct RingStats {
        uint64_t total_signatures_created{0};
        uint64_t total_signatures_verified{0};
        uint64_t verification_failures{0};
        double avg_ring_size{0.0};
        uint64_t num_spent_key_images{0};
    };

    RingStats GetStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace privacy
} // namespace intcoin

#endif // INTCOIN_PRIVACY_RING_SIGNATURES_H
