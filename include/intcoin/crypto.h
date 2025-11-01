// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Quantum-resistant cryptography (CRYSTALS-Dilithium, Kyber, SHA3).

#ifndef INTCOIN_CRYPTO_H
#define INTCOIN_CRYPTO_H

#include "primitives.h"
#include <vector>
#include <array>
#include <string>
#include <optional>
#include <memory>

namespace intcoin {
namespace crypto {

/**
 * SHA3-256 hash function (FIPS 202)
 * Used for general hashing: transactions, merkle trees, addresses
 */
class SHA3_256 {
public:
    SHA3_256();
    ~SHA3_256();

    // Update hash with data
    void update(const uint8_t* data, size_t len);
    void update(const std::vector<uint8_t>& data);

    // Finalize and get hash
    Hash256 finalize();

    // Reset to initial state
    void reset();

    // One-shot hash
    static Hash256 hash(const uint8_t* data, size_t len);
    static Hash256 hash(const std::vector<uint8_t>& data);

    // Double hash (hash of hash)
    static Hash256 double_hash(const uint8_t* data, size_t len);
    static Hash256 double_hash(const std::vector<uint8_t>& data);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * SHA-256 hash function (FIPS 180-4)
 * Used specifically for Proof of Work mining
 *
 * Why SHA-256 for PoW?
 * - Becomes ASIC-resistant in quantum era
 * - Well-tested, proven algorithm
 * - Simple to verify
 * - CPU-friendly mining
 */
class SHA256_PoW {
public:
    // One-shot hash
    static Hash256 hash(const uint8_t* data, size_t len);
    static Hash256 hash(const std::vector<uint8_t>& data);

    // Double hash (for block hashing)
    static Hash256 double_hash(const uint8_t* data, size_t len);
    static Hash256 double_hash(const std::vector<uint8_t>& data);
};

/**
 * CRYSTALS-Dilithium keypair
 * Post-quantum digital signature (NIST FIPS 204)
 */
struct DilithiumKeyPair {
    DilithiumPubKey public_key;
    std::array<uint8_t, 4864> private_key;  // Dilithium5 private key

    DilithiumKeyPair() : public_key{}, private_key{} {}

    // Serialize private key for secure storage
    std::vector<uint8_t> serialize_private() const;

    // Deserialize private key
    static std::optional<DilithiumKeyPair> deserialize_private(
        const std::vector<uint8_t>& data
    );

    // Clear sensitive data
    void clear_private();
};

/**
 * CRYSTALS-Dilithium digital signature
 * Quantum-resistant signatures (NIST FIPS 204)
 */
class Dilithium {
public:
    /**
     * Generate new keypair
     */
    static DilithiumKeyPair generate_keypair();

    /**
     * Sign message with private key
     */
    static DilithiumSignature sign(
        const std::vector<uint8_t>& message,
        const DilithiumKeyPair& keypair
    );

    /**
     * Verify signature
     */
    static bool verify(
        const std::vector<uint8_t>& message,
        const DilithiumSignature& signature,
        const DilithiumPubKey& public_key
    );

    /**
     * Get algorithm name
     */
    static std::string algorithm_name() {
        return "CRYSTALS-Dilithium5";
    }

    /**
     * Get security level (NIST level)
     */
    static int security_level() {
        return 5;  // NIST Level 5 (highest)
    }
};

/**
 * Kyber keypair
 * Post-quantum key encapsulation (NIST FIPS 203)
 */
struct KyberKeyPair {
    KyberPubKey public_key;
    std::array<uint8_t, 3168> private_key;  // Kyber1024 private key

    KyberKeyPair() : public_key{}, private_key{} {}

    // Serialize private key for secure storage
    std::vector<uint8_t> serialize_private() const;

    // Deserialize private key
    static std::optional<KyberKeyPair> deserialize_private(
        const std::vector<uint8_t>& data
    );

    // Clear sensitive data
    void clear_private();
};

/**
 * Kyber key encapsulation mechanism
 * Quantum-resistant key exchange (NIST FIPS 203)
 */
class Kyber {
public:
    /**
     * Generate new keypair
     */
    static KyberKeyPair generate_keypair();

    /**
     * Encapsulate: generate shared secret and ciphertext
     */
    static std::pair<SharedSecret, KyberCiphertext> encapsulate(
        const KyberPubKey& public_key
    );

    /**
     * Decapsulate: recover shared secret from ciphertext
     */
    static std::optional<SharedSecret> decapsulate(
        const KyberCiphertext& ciphertext,
        const KyberKeyPair& keypair
    );

    /**
     * Get algorithm name
     */
    static std::string algorithm_name() {
        return "CRYSTALS-Kyber1024";
    }

    /**
     * Get security level (NIST level)
     */
    static int security_level() {
        return 5;  // NIST Level 5 (highest)
    }
};

/**
 * Address generation
 * Creates addresses from public keys
 */
class Address {
public:
    /**
     * Generate address from Dilithium public key
     * Format: Base58Check encoding of hash(pubkey)
     */
    static std::string from_public_key(const DilithiumPubKey& pubkey);

    /**
     * Validate address format
     */
    static bool validate(const std::string& address);

    /**
     * Extract public key hash from address
     */
    static std::optional<Hash256> decode(const std::string& address);

    /**
     * Get address version byte (mainnet/testnet)
     */
    enum class Network {
        MAINNET = 0x3C,  // 'I' in Base58
        TESTNET = 0x6F   // 'T' in Base58
    };

    /**
     * Generate address for specific network
     */
    static std::string from_public_key(
        const DilithiumPubKey& pubkey,
        Network network
    );
};

/**
 * Random number generation
 * Cryptographically secure RNG
 */
class SecureRandom {
public:
    /**
     * Generate random bytes
     */
    static void generate(uint8_t* buffer, size_t length);

    /**
     * Generate random bytes (vector)
     */
    static std::vector<uint8_t> generate(size_t length);

    /**
     * Generate random 32-bit integer
     */
    static uint32_t generate_uint32();

    /**
     * Generate random 64-bit integer
     */
    static uint64_t generate_uint64();
};

/**
 * HMAC-based Key Derivation Function (HKDF)
 * For deriving keys from master secrets
 */
class HKDF {
public:
    /**
     * Derive key from master secret
     */
    static std::vector<uint8_t> derive(
        const std::vector<uint8_t>& master_secret,
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& info,
        size_t output_length
    );

    /**
     * Derive deterministic child key
     */
    static std::vector<uint8_t> derive_child(
        const std::vector<uint8_t>& parent_key,
        uint32_t index
    );
};

/**
 * BIP39-style mnemonic phrase generation
 * For wallet seed backup
 */
class Mnemonic {
public:
    /**
     * Generate mnemonic from entropy
     * Word count: 12, 15, 18, 21, or 24 words
     */
    static std::string generate(size_t word_count = 24);

    /**
     * Convert mnemonic to seed
     */
    static std::vector<uint8_t> to_seed(
        const std::string& mnemonic,
        const std::string& passphrase = ""
    );

    /**
     * Validate mnemonic phrase
     */
    static bool validate(const std::string& mnemonic);

    /**
     * Get wordlist size
     */
    static size_t wordlist_size();
};

} // namespace crypto
} // namespace intcoin

#endif // INTCOIN_CRYPTO_H
