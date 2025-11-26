/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Quantum-Resistant Cryptography (Dilithium3 + Kyber768)
 */

#ifndef INTCOIN_CRYPTO_H
#define INTCOIN_CRYPTO_H

#include "types.h"
#include <array>
#include <vector>
#include <optional>

namespace intcoin {

// ============================================================================
// Constants (liboqs PQC algorithm sizes)
// ============================================================================

// Dilithium3 (NIST Security Level 3)
constexpr size_t DILITHIUM3_PUBLICKEYBYTES = 1952;
constexpr size_t DILITHIUM3_SECRETKEYBYTES = 4000;
constexpr size_t DILITHIUM3_BYTES = 3293;

// Kyber768 (NIST Security Level 3)
constexpr size_t KYBER768_PUBLICKEYBYTES = 1184;
constexpr size_t KYBER768_SECRETKEYBYTES = 2400;
constexpr size_t KYBER768_CIPHERTEXTBYTES = 1088;
constexpr size_t KYBER768_SSBYTES = 32;

// SHA3-256
constexpr size_t SHA3_256_DIGEST_SIZE = 32;

// ============================================================================
// Dilithium3 Cryptography (Digital Signatures)
// ============================================================================

class DilithiumCrypto {
public:
    /// Dilithium key pair
    struct KeyPair {
        PublicKey public_key;
        SecretKey secret_key;
    };

    /// Generate new Dilithium3 key pair
    static Result<KeyPair> GenerateKeyPair();

    /// Sign message with Dilithium3
    static Result<Signature> Sign(const std::vector<uint8_t>& message,
                                  const SecretKey& secret_key);

    /// Sign hash with Dilithium3
    static Result<Signature> SignHash(const uint256& hash,
                                      const SecretKey& secret_key);

    /// Verify Dilithium3 signature
    static Result<void> Verify(const std::vector<uint8_t>& message,
                               const Signature& signature,
                               const PublicKey& public_key);

    /// Verify signature on hash
    static Result<void> VerifyHash(const uint256& hash,
                                   const Signature& signature,
                                   const PublicKey& public_key);

    /// Import public key from bytes
    static Result<PublicKey> ImportPublicKey(const std::vector<uint8_t>& bytes);

    /// Import secret key from bytes
    static Result<SecretKey> ImportSecretKey(const std::vector<uint8_t>& bytes);

    /// Export public key to bytes
    static std::vector<uint8_t> ExportPublicKey(const PublicKey& key);

    /// Export secret key to bytes
    static std::vector<uint8_t> ExportSecretKey(const SecretKey& key);
};

// ============================================================================
// Kyber768 Cryptography (Key Encapsulation)
// ============================================================================

class KyberCrypto {
public:
    /// Kyber key pair
    struct KeyPair {
        std::array<uint8_t, KYBER768_PUBLICKEYBYTES> public_key;
        std::array<uint8_t, KYBER768_SECRETKEYBYTES> secret_key;
    };

    /// Kyber ciphertext
    using Ciphertext = std::array<uint8_t, KYBER768_CIPHERTEXTBYTES>;

    /// Shared secret
    using SharedSecret = std::array<uint8_t, KYBER768_SSBYTES>;

    /// Generate new Kyber768 key pair
    static Result<KeyPair> GenerateKeyPair();

    /// Encapsulate shared secret (encrypt)
    static Result<std::pair<SharedSecret, Ciphertext>> Encapsulate(
        const std::array<uint8_t, KYBER768_PUBLICKEYBYTES>& public_key);

    /// Decapsulate shared secret (decrypt)
    static Result<SharedSecret> Decapsulate(
        const Ciphertext& ciphertext,
        const std::array<uint8_t, KYBER768_SECRETKEYBYTES>& secret_key);

    /// Import public key
    static Result<std::array<uint8_t, KYBER768_PUBLICKEYBYTES>> ImportPublicKey(
        const std::vector<uint8_t>& bytes);

    /// Export public key
    static std::vector<uint8_t> ExportPublicKey(
        const std::array<uint8_t, KYBER768_PUBLICKEYBYTES>& key);
};

// ============================================================================
// SHA3-256 Hashing
// ============================================================================

class SHA3 {
public:
    /// Hash single buffer
    static uint256 Hash(const std::vector<uint8_t>& data);

    /// Hash two buffers (double hash)
    static uint256 DoubleHash(const std::vector<uint8_t>& data);

    /// Hash buffer with specific size
    static uint256 Hash(const uint8_t* data, size_t len);

    /// HMAC-SHA3-256
    static uint256 HMAC(const std::vector<uint8_t>& key,
                        const std::vector<uint8_t>& message);
};

// ============================================================================
// Address Generation (Bech32 with 'int1' prefix)
// ============================================================================

class AddressEncoder {
public:
    /// Encode public key hash to Bech32 address
    static Result<std::string> EncodeAddress(const uint256& pubkey_hash);

    /// Decode Bech32 address to public key hash
    static Result<uint256> DecodeAddress(const std::string& address);

    /// Validate address format
    static bool ValidateAddress(const std::string& address);

    /// Get address version
    static std::optional<uint8_t> GetAddressVersion(const std::string& address);
};

// ============================================================================
// Key Derivation (BIP32-like for quantum-resistant keys)
// ============================================================================

class KeyDerivation {
public:
    /// Extended key (hierarchical deterministic wallet)
    struct ExtendedKey {
        uint8_t depth;
        uint32_t child_index;
        std::array<uint8_t, 32> chain_code;
        SecretKey secret_key;
        PublicKey public_key;
    };

    /// Derive master key from seed
    static Result<ExtendedKey> DeriveMasterKey(const std::vector<uint8_t>& seed);

    /// Derive child key
    static Result<ExtendedKey> DeriveChildKey(const ExtendedKey& parent,
                                              uint32_t index,
                                              bool hardened = false);

    /// Derive key from path (e.g., "m/44'/0'/0'/0/0")
    static Result<ExtendedKey> DerivePath(const ExtendedKey& master,
                                          const std::string& path);
};

// ============================================================================
// Random Number Generation (CSPRNG)
// ============================================================================

class RandomGenerator {
public:
    /// Generate random bytes
    static std::vector<uint8_t> GetRandomBytes(size_t count);

    /// Generate random uint256
    static uint256 GetRandomUint256();

    /// Generate random uint64
    static uint64_t GetRandomUint64();

    /// Seed the RNG (for testing only - production uses system entropy)
    static void SeedForTesting(uint64_t seed);
};

// ============================================================================
// Utility Functions
// ============================================================================

/// Constant-time comparison (prevent timing attacks)
bool ConstantTimeCompare(const std::vector<uint8_t>& a,
                        const std::vector<uint8_t>& b);

/// Secure memory wipe
void SecureWipe(void* ptr, size_t len);

/// Convert public key to public key hash
uint256 PublicKeyToHash(const PublicKey& pubkey);

/// Convert public key hash to address
std::string PublicKeyHashToAddress(const uint256& pubkey_hash);

/// Convert public key to address (convenience)
std::string PublicKeyToAddress(const PublicKey& pubkey);

} // namespace intcoin

#endif // INTCOIN_CRYPTO_H
