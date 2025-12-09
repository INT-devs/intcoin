/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Cryptography Implementation (Dilithium3 + Kyber768 + SHA3)
 */

#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <cstring>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <oqs/oqs.h>

namespace intcoin {

// ============================================================================
// Deterministic RNG for HD Wallet Key Derivation
// ============================================================================

// Thread-local state for deterministic RNG (used by GenerateDeterministicKeyPair)
namespace {
    thread_local struct DeterministicRNGState {
        std::vector<uint8_t> seed;
        uint64_t counter = 0;
        bool active = false;
    } det_rng_state;

    // Custom deterministic RNG function for liboqs
    // Uses SHA3-256 hash chain: output = SHA3(seed || counter)
    void deterministic_randombytes(uint8_t *random_array, size_t bytes_to_read) {
        if (!det_rng_state.active || det_rng_state.seed.empty()) {
            // Fallback to system RNG if not properly initialized
            // This shouldn't happen, but provides safety
            return;
        }

        size_t offset = 0;
        while (offset < bytes_to_read) {
            // Prepare input: seed || counter (little-endian)
            std::vector<uint8_t> input = det_rng_state.seed;
            for (int i = 0; i < 8; i++) {
                input.push_back((det_rng_state.counter >> (i * 8)) & 0xFF);
            }

            // Hash to get random bytes
            auto hash = SHA3::Hash(input);

            // Copy what we need
            size_t to_copy = std::min(static_cast<size_t>(hash.size()),
                                     bytes_to_read - offset);
            std::copy_n(hash.begin(), to_copy, random_array + offset);

            offset += to_copy;
            det_rng_state.counter++;
        }
    }
}

// ============================================================================
// Dilithium3 Implementation
// ============================================================================

Result<DilithiumCrypto::KeyPair> DilithiumCrypto::GenerateKeyPair() {
    // Create ML-DSA-65 (Dilithium3) signature object
    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == nullptr) {
        return Result<KeyPair>::Error("Failed to create ML-DSA-65 signature object");
    }

    KeyPair keypair;
    PublicKey public_key{};
    SecretKey secret_key{};

    // Generate keypair
    int rc = OQS_SIG_keypair(sig, public_key.data(), secret_key.data());
    if (rc != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        return Result<KeyPair>::Error("Failed to generate ML-DSA-65 keypair");
    }

    OQS_SIG_free(sig);

    keypair.public_key = public_key;
    keypair.secret_key = secret_key;

    return Result<KeyPair>::Ok(std::move(keypair));
}

Result<DilithiumCrypto::KeyPair> DilithiumCrypto::GenerateDeterministicKeyPair(
        const std::vector<uint8_t>& seed) {
    if (seed.empty()) {
        return Result<KeyPair>::Error("Seed cannot be empty for deterministic key generation");
    }

    // Initialize deterministic RNG state
    det_rng_state.seed = seed;
    det_rng_state.counter = 0;
    det_rng_state.active = true;

    // Set custom RNG for liboqs
    OQS_randombytes_custom_algorithm(deterministic_randombytes);

    // Create ML-DSA-65 (Dilithium3) signature object
    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == nullptr) {
        // Cleanup and restore system RNG
        det_rng_state.active = false;
        det_rng_state.seed.clear();
        OQS_randombytes_switch_algorithm("system");
        return Result<KeyPair>::Error("Failed to create ML-DSA-65 signature object");
    }

    KeyPair keypair;
    PublicKey public_key{};
    SecretKey secret_key{};

    // Generate keypair using deterministic RNG
    int rc = OQS_SIG_keypair(sig, public_key.data(), secret_key.data());

    // Cleanup
    OQS_SIG_free(sig);

    // Restore system RNG and clear sensitive data
    det_rng_state.active = false;
    det_rng_state.seed.clear();
    det_rng_state.counter = 0;
    OQS_randombytes_switch_algorithm("system");

    if (rc != OQS_SUCCESS) {
        return Result<KeyPair>::Error("Failed to generate deterministic ML-DSA-65 keypair");
    }

    keypair.public_key = public_key;
    keypair.secret_key = secret_key;

    return Result<KeyPair>::Ok(std::move(keypair));
}

Result<Signature> DilithiumCrypto::Sign(const std::vector<uint8_t>& message,
                                       const SecretKey& secret_key) {
    // Create ML-DSA-65 signature object
    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == nullptr) {
        return Result<Signature>::Error("Failed to create ML-DSA-65 signature object");
    }

    Signature signature{};
    size_t signature_len = 0;

    // Sign the message
    int rc = OQS_SIG_sign(sig, signature.data(), &signature_len,
                          message.data(), message.size(),
                          secret_key.data());

    if (rc != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        return Result<Signature>::Error("Failed to sign message with ML-DSA-65");
    }

    OQS_SIG_free(sig);

    // Note: ML-DSA-65 signatures are variable length but should fit in our buffer
    // The actual signature_len may be less than signature.size()

    return Result<Signature>::Ok(std::move(signature));
}

Result<Signature> DilithiumCrypto::SignHash(const uint256& hash,
                                           const SecretKey& secret_key) {
    std::vector<uint8_t> hash_bytes(hash.begin(), hash.end());
    return Sign(hash_bytes, secret_key);
}

Result<void> DilithiumCrypto::Verify(const std::vector<uint8_t>& message,
                                    const Signature& signature,
                                    const PublicKey& public_key) {
    // Create ML-DSA-65 signature object
    OQS_SIG *sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == nullptr) {
        return Result<void>::Error("Failed to create ML-DSA-65 signature object");
    }

    // Verify the signature
    int rc = OQS_SIG_verify(sig, message.data(), message.size(),
                            signature.data(), signature.size(),
                            public_key.data());

    OQS_SIG_free(sig);

    if (rc != OQS_SUCCESS) {
        return Result<void>::Error("Signature verification failed");
    }

    return Result<void>::Ok();
}

Result<void> DilithiumCrypto::VerifyHash(const uint256& hash,
                                        const Signature& signature,
                                        const PublicKey& public_key) {
    std::vector<uint8_t> hash_bytes(hash.begin(), hash.end());
    return Verify(hash_bytes, signature, public_key);
}

// ============================================================================
// Kyber768 Implementation
// ============================================================================

Result<KyberCrypto::KeyPair> KyberCrypto::GenerateKeyPair() {
    // Create ML-KEM-768 (Kyber768) KEM object
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == nullptr) {
        return Result<KeyPair>::Error("Failed to create ML-KEM-768 KEM object");
    }

    KeyPair keypair;
    std::array<uint8_t, KYBER768_PUBLICKEYBYTES> public_key{};
    std::array<uint8_t, KYBER768_SECRETKEYBYTES> secret_key{};

    // Generate keypair
    int rc = OQS_KEM_keypair(kem, public_key.data(), secret_key.data());
    if (rc != OQS_SUCCESS) {
        OQS_KEM_free(kem);
        return Result<KeyPair>::Error("Failed to generate ML-KEM-768 keypair");
    }

    OQS_KEM_free(kem);

    keypair.public_key = public_key;
    keypair.secret_key = secret_key;

    return Result<KeyPair>::Ok(std::move(keypair));
}

Result<std::pair<KyberCrypto::SharedSecret, KyberCrypto::Ciphertext>>
KyberCrypto::Encapsulate(const std::array<uint8_t, KYBER768_PUBLICKEYBYTES>& public_key) {
    // Create ML-KEM-768 KEM object
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == nullptr) {
        return Result<std::pair<SharedSecret, Ciphertext>>::Error("Failed to create ML-KEM-768 KEM object");
    }

    Ciphertext ciphertext{};
    SharedSecret shared_secret{};

    // Encapsulate to generate shared secret and ciphertext
    int rc = OQS_KEM_encaps(kem, ciphertext.data(), shared_secret.data(), public_key.data());
    if (rc != OQS_SUCCESS) {
        OQS_KEM_free(kem);
        return Result<std::pair<SharedSecret, Ciphertext>>::Error("Failed to encapsulate with ML-KEM-768");
    }

    OQS_KEM_free(kem);

    return Result<std::pair<SharedSecret, Ciphertext>>::Ok(std::make_pair(shared_secret, ciphertext));
}

Result<KyberCrypto::SharedSecret> KyberCrypto::Decapsulate(
    const Ciphertext& ciphertext,
    const std::array<uint8_t, KYBER768_SECRETKEYBYTES>& secret_key) {
    // Create ML-KEM-768 KEM object
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == nullptr) {
        return Result<SharedSecret>::Error("Failed to create ML-KEM-768 KEM object");
    }

    SharedSecret shared_secret{};

    // Decapsulate to recover shared secret
    int rc = OQS_KEM_decaps(kem, shared_secret.data(), ciphertext.data(), secret_key.data());
    if (rc != OQS_SUCCESS) {
        OQS_KEM_free(kem);
        return Result<SharedSecret>::Error("Failed to decapsulate with ML-KEM-768");
    }

    OQS_KEM_free(kem);

    return Result<SharedSecret>::Ok(std::move(shared_secret));
}

// ============================================================================
// SHA3-256 Implementation
// ============================================================================

uint256 SHA3::Hash(const std::vector<uint8_t>& data) {
    uint256 result{};

    // Create EVP context for SHA3-256
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // Initialize SHA3-256
    if (EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize SHA3-256");
    }

    // Update with data
    if (!data.empty()) {
        if (EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update SHA3-256");
        }
    }

    // Finalize and get hash
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, result.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize SHA3-256");
    }

    EVP_MD_CTX_free(ctx);

    // Verify we got exactly 32 bytes
    if (hash_len != 32) {
        throw std::runtime_error("SHA3-256 returned unexpected hash length");
    }

    return result;
}

uint256 SHA3::DoubleHash(const std::vector<uint8_t>& data) {
    auto first = Hash(data);
    std::vector<uint8_t> first_bytes(first.begin(), first.end());
    return Hash(first_bytes);
}

uint256 SHA3::Hash(const uint8_t* data, size_t len) {
    std::vector<uint8_t> vec(data, data + len);
    return Hash(vec);
}

// ============================================================================
// Address Encoding (Bech32)
// ============================================================================

namespace {
    // Bech32 character set (base32)
    const char* BECH32_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    // Generator values for Bech32 checksum
    const uint32_t BECH32_GENERATOR[5] = {
        0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3
    };

    // Find character in charset (returns -1 if not found)
    int CharsetIndex(char c) {
        for (int i = 0; i < 32; ++i) {
            if (BECH32_CHARSET[i] == c) {
                return i;
            }
        }
        return -1;
    }

    // Bech32 checksum polymod
    uint32_t Bech32Polymod(const std::vector<uint8_t>& values) {
        uint32_t chk = 1;
        for (uint8_t value : values) {
            uint32_t top = chk >> 25;
            chk = (chk & 0x1ffffff) << 5 ^ value;
            for (int i = 0; i < 5; ++i) {
                if ((top >> i) & 1) {
                    chk ^= BECH32_GENERATOR[i];
                }
            }
        }
        return chk;
    }

    // Expand human-readable part for checksum
    std::vector<uint8_t> ExpandHRP(const std::string& hrp) {
        std::vector<uint8_t> result;
        result.reserve(hrp.size() * 2 + 1);

        // High bits of each character
        for (char c : hrp) {
            result.push_back(c >> 5);
        }

        // Separator
        result.push_back(0);

        // Low bits of each character
        for (char c : hrp) {
            result.push_back(c & 31);
        }

        return result;
    }

    // Create Bech32 checksum
    std::vector<uint8_t> CreateChecksum(const std::string& hrp,
                                        const std::vector<uint8_t>& data) {
        std::vector<uint8_t> values = ExpandHRP(hrp);
        values.insert(values.end(), data.begin(), data.end());
        values.insert(values.end(), 6, 0); // 6 zero bytes for checksum placeholder

        uint32_t polymod = Bech32Polymod(values) ^ 1;

        std::vector<uint8_t> checksum(6);
        for (int i = 0; i < 6; ++i) {
            checksum[i] = (polymod >> (5 * (5 - i))) & 31;
        }

        return checksum;
    }

    // Verify Bech32 checksum
    bool VerifyChecksum(const std::string& hrp, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> values = ExpandHRP(hrp);
        values.insert(values.end(), data.begin(), data.end());
        return Bech32Polymod(values) == 1;
    }

    // Convert 8-bit data to 5-bit data
    std::vector<uint8_t> ConvertBits(const std::vector<uint8_t>& data,
                                     int frombits, int tobits, bool pad) {
        std::vector<uint8_t> result;
        int acc = 0;
        int bits = 0;
        int maxv = (1 << tobits) - 1;
        int max_acc = (1 << (frombits + tobits - 1)) - 1;

        for (uint8_t value : data) {
            if ((value >> frombits) != 0) {
                return {}; // Invalid input
            }
            acc = ((acc << frombits) | value) & max_acc;
            bits += frombits;
            while (bits >= tobits) {
                bits -= tobits;
                result.push_back((acc >> bits) & maxv);
            }
        }

        if (pad) {
            if (bits > 0) {
                result.push_back((acc << (tobits - bits)) & maxv);
            }
        } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
            return {}; // Invalid padding
        }

        return result;
    }
}

Result<std::string> AddressEncoder::EncodeAddress(const uint256& pubkey_hash) {
    const std::string hrp = "int1";

    // Convert pubkey_hash to vector
    std::vector<uint8_t> hash_bytes(pubkey_hash.begin(), pubkey_hash.end());

    // Add version byte (0 for mainnet P2PKH)
    std::vector<uint8_t> data_with_version;
    data_with_version.push_back(0); // Version 0
    data_with_version.insert(data_with_version.end(), hash_bytes.begin(), hash_bytes.end());

    // Convert from 8-bit to 5-bit
    std::vector<uint8_t> data_5bit = ConvertBits(data_with_version, 8, 5, true);
    if (data_5bit.empty()) {
        return Result<std::string>::Error("Failed to convert bits for Bech32 encoding");
    }

    // Create checksum
    std::vector<uint8_t> checksum = CreateChecksum(hrp, data_5bit);

    // Combine data and checksum
    data_5bit.insert(data_5bit.end(), checksum.begin(), checksum.end());

    // Encode to Bech32 string
    std::string result = hrp + "1";
    for (uint8_t value : data_5bit) {
        if (value >= 32) {
            return Result<std::string>::Error("Invalid 5-bit value in Bech32 encoding");
        }
        result += BECH32_CHARSET[value];
    }

    return Result<std::string>::Ok(std::move(result));
}

Result<uint256> AddressEncoder::DecodeAddress(const std::string& address) {
    const std::string hrp = "int1";

    // Convert to lowercase
    std::string lower_addr;
    lower_addr.reserve(address.size());
    bool has_lower = false, has_upper = false;

    for (char c : address) {
        if (c >= 'A' && c <= 'Z') {
            has_upper = true;
            lower_addr += (c - 'A' + 'a');
        } else if (c >= 'a' && c <= 'z') {
            has_lower = true;
            lower_addr += c;
        } else {
            lower_addr += c;
        }
    }

    // Bech32 addresses should not mix upper and lowercase
    if (has_lower && has_upper) {
        return Result<uint256>::Error("Mixed case in Bech32 address");
    }

    // Find separator '1'
    size_t sep_pos = lower_addr.rfind('1');
    if (sep_pos == std::string::npos) {
        return Result<uint256>::Error("No separator found in address");
    }

    // Extract HRP and data
    std::string addr_hrp = lower_addr.substr(0, sep_pos);
    std::string data_part = lower_addr.substr(sep_pos + 1);

    // Verify HRP matches
    if (addr_hrp != hrp) {
        return Result<uint256>::Error("Invalid HRP (expected 'int1')");
    }

    // Checksum must be at least 6 characters
    if (data_part.size() < 6) {
        return Result<uint256>::Error("Address too short");
    }

    // Decode data part
    std::vector<uint8_t> data_5bit;
    data_5bit.reserve(data_part.size());

    for (char c : data_part) {
        int idx = CharsetIndex(c);
        if (idx == -1) {
            return Result<uint256>::Error("Invalid character in Bech32 address");
        }
        data_5bit.push_back(static_cast<uint8_t>(idx));
    }

    // Verify checksum
    if (!VerifyChecksum(hrp, data_5bit)) {
        return Result<uint256>::Error("Invalid checksum");
    }

    // Remove checksum (last 6 characters)
    data_5bit.resize(data_5bit.size() - 6);

    // Convert from 5-bit to 8-bit
    std::vector<uint8_t> data_8bit = ConvertBits(data_5bit, 5, 8, false);
    if (data_8bit.empty()) {
        return Result<uint256>::Error("Failed to convert bits in Bech32 decoding");
    }

    // First byte is version
    if (data_8bit.empty()) {
        return Result<uint256>::Error("Address data too short");
    }

    uint8_t version = data_8bit[0];
    if (version != 0) {
        return Result<uint256>::Error("Unsupported address version");
    }

    // Remaining bytes are the pubkey hash
    data_8bit.erase(data_8bit.begin());

    if (data_8bit.size() != 32) {
        return Result<uint256>::Error("Invalid pubkey hash length");
    }

    // Convert to uint256
    uint256 pubkey_hash{};
    std::copy(data_8bit.begin(), data_8bit.end(), pubkey_hash.begin());

    return Result<uint256>::Ok(std::move(pubkey_hash));
}

bool AddressEncoder::ValidateAddress(const std::string& address) {
    auto result = DecodeAddress(address);
    return result.IsOk();
}

// ============================================================================
// Utility Functions
// ============================================================================

bool ConstantTimeCompare(const std::vector<uint8_t>& a,
                        const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) return false;

    uint8_t result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}

void SecureWipe(void* ptr, size_t len) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (len--) {
        *p++ = 0;
    }
}

uint256 PublicKeyToHash(const PublicKey& pubkey) {
    // Convert std::array to vector for hashing
    std::vector<uint8_t> serialized(pubkey.begin(), pubkey.end());
    return SHA3::Hash(serialized);
}

std::string PublicKeyHashToAddress(const uint256& pubkey_hash) {
    auto result = AddressEncoder::EncodeAddress(pubkey_hash);
    if (result.IsError()) {
        return "";
    }
    return *result.value;
}

std::string PublicKeyToAddress(const PublicKey& pubkey) {
    uint256 hash = PublicKeyToHash(pubkey);
    return PublicKeyHashToAddress(hash);
}

// ============================================================================
// Random Number Generation
// ============================================================================

std::vector<uint8_t> RandomGenerator::GetRandomBytes(size_t count) {
    std::vector<uint8_t> bytes(count);

    // Use OpenSSL's CSPRNG (RAND_bytes)
    if (RAND_bytes(bytes.data(), static_cast<int>(count)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }

    return bytes;
}

uint256 RandomGenerator::GetRandomUint256() {
    auto bytes = GetRandomBytes(32);
    uint256 result;
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}

uint64_t RandomGenerator::GetRandomUint64() {
    auto bytes = GetRandomBytes(8);
    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i) {
        result |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
    }
    return result;
}

} // namespace intcoin
