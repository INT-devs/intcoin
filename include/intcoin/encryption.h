// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_ENCRYPTION_H
#define INTCOIN_ENCRYPTION_H

#include <vector>
#include <cstdint>
#include <string>
#include <optional>
#include <memory>

namespace intcoin {
namespace crypto {

/**
 * AES-256-GCM encryption for wallet data
 *
 * Features:
 * - 256-bit key derived from password using PBKDF2
 * - Galois/Counter Mode for authenticated encryption
 * - Random IV for each encryption
 * - Authentication tag to prevent tampering
 */
class AES256_GCM {
public:
    static constexpr size_t KEY_SIZE = 32;      // 256 bits
    static constexpr size_t IV_SIZE = 12;       // 96 bits (recommended for GCM)
    static constexpr size_t TAG_SIZE = 16;      // 128 bits
    static constexpr size_t SALT_SIZE = 32;     // 256 bits
    static constexpr int PBKDF2_ITERATIONS = 100000; // OWASP recommendation

    /**
     * Encrypted data container
     */
    struct EncryptedData {
        std::vector<uint8_t> salt;          // Salt for PBKDF2
        std::vector<uint8_t> iv;            // Initialization vector
        std::vector<uint8_t> ciphertext;    // Encrypted data
        std::vector<uint8_t> tag;           // Authentication tag

        // Serialize to single vector for storage
        std::vector<uint8_t> serialize() const;

        // Deserialize from storage
        static std::optional<EncryptedData> deserialize(const std::vector<uint8_t>& data);
    };

    /**
     * Derive encryption key from password using PBKDF2-SHA256
     *
     * @param password User password
     * @param salt Random salt
     * @param iterations Number of PBKDF2 iterations
     * @return 256-bit key
     */
    static std::vector<uint8_t> derive_key(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        int iterations = PBKDF2_ITERATIONS
    );

    /**
     * Encrypt data with password
     *
     * @param plaintext Data to encrypt
     * @param password User password
     * @param additional_data Optional additional authenticated data (AAD)
     * @return Encrypted data with salt, IV, ciphertext, and tag
     */
    static std::optional<EncryptedData> encrypt(
        const std::vector<uint8_t>& plaintext,
        const std::string& password,
        const std::vector<uint8_t>& additional_data = {}
    );

    /**
     * Decrypt data with password
     *
     * @param encrypted Encrypted data
     * @param password User password
     * @param additional_data Optional additional authenticated data (AAD) - must match encryption
     * @return Decrypted plaintext, or nullopt if password wrong or data corrupted
     */
    static std::optional<std::vector<uint8_t>> decrypt(
        const EncryptedData& encrypted,
        const std::string& password,
        const std::vector<uint8_t>& additional_data = {}
    );

    /**
     * Verify password without decrypting
     *
     * @param encrypted Encrypted data
     * @param password Password to verify
     * @return true if password is correct
     */
    static bool verify_password(
        const EncryptedData& encrypted,
        const std::string& password
    );

private:
    // Internal encryption with derived key
    static std::optional<EncryptedData> encrypt_with_key(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& additional_data
    );

    // Internal decryption with derived key
    static std::optional<std::vector<uint8_t>> decrypt_with_key(
        const EncryptedData& encrypted,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& additional_data
    );
};

/**
 * Secure memory operations
 */
class SecureMemory {
public:
    /**
     * Securely zero memory
     * Uses volatile to prevent compiler optimization
     */
    static void secure_zero(void* ptr, size_t size);

    /**
     * Securely zero vector
     */
    static void secure_zero(std::vector<uint8_t>& vec);

    /**
     * Securely zero string
     */
    static void secure_zero(std::string& str);

    /**
     * Compare memory in constant time (prevents timing attacks)
     */
    static bool constant_time_compare(
        const void* a,
        const void* b,
        size_t size
    );

    /**
     * Compare vectors in constant time
     */
    static bool constant_time_compare(
        const std::vector<uint8_t>& a,
        const std::vector<uint8_t>& b
    );
};

/**
 * RAII wrapper for secure memory cleanup
 */
template<typename T>
class SecureVector {
private:
    std::vector<T> data_;

public:
    SecureVector() = default;

    explicit SecureVector(size_t size) : data_(size) {}

    SecureVector(const std::vector<T>& other) : data_(other) {}

    SecureVector(std::vector<T>&& other) : data_(std::move(other)) {}

    ~SecureVector() {
        if constexpr (std::is_same_v<T, uint8_t>) {
            SecureMemory::secure_zero(data_);
        }
    }

    // No copy
    SecureVector(const SecureVector&) = delete;
    SecureVector& operator=(const SecureVector&) = delete;

    // Move only
    SecureVector(SecureVector&& other) noexcept : data_(std::move(other.data_)) {}

    SecureVector& operator=(SecureVector&& other) noexcept {
        if (this != &other) {
            if constexpr (std::is_same_v<T, uint8_t>) {
                SecureMemory::secure_zero(data_);
            }
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // Access
    std::vector<T>& get() { return data_; }
    const std::vector<T>& get() const { return data_; }

    // Convenience
    size_t size() const { return data_.size(); }
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }

    typename std::vector<T>::iterator begin() { return data_.begin(); }
    typename std::vector<T>::iterator end() { return data_.end(); }
    typename std::vector<T>::const_iterator begin() const { return data_.begin(); }
    typename std::vector<T>::const_iterator end() const { return data_.end(); }
};

using SecureBytes = SecureVector<uint8_t>;

} // namespace crypto
} // namespace intcoin

#endif // INTCOIN_ENCRYPTION_H
