// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/encryption.h"
#include "intcoin/crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <cstring>

namespace intcoin {
namespace crypto {

// EncryptedData serialization
std::vector<uint8_t> AES256_GCM::EncryptedData::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(4 + salt.size() + iv.size() + ciphertext.size() + tag.size());

    // Write sizes (4 bytes each)
    auto write_uint32 = [&result](uint32_t value) {
        result.push_back(static_cast<uint8_t>(value & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };

    write_uint32(static_cast<uint32_t>(salt.size()));
    write_uint32(static_cast<uint32_t>(iv.size()));
    write_uint32(static_cast<uint32_t>(ciphertext.size()));
    write_uint32(static_cast<uint32_t>(tag.size()));

    // Write data
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());

    return result;
}

std::optional<AES256_GCM::EncryptedData> AES256_GCM::EncryptedData::deserialize(
    const std::vector<uint8_t>& data
) {
    if (data.size() < 16) {  // Need at least 4 size fields
        return std::nullopt;
    }

    size_t offset = 0;
    auto read_uint32 = [&data, &offset]() -> uint32_t {
        uint32_t value = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        return value;
    };

    uint32_t salt_size = read_uint32();
    uint32_t iv_size = read_uint32();
    uint32_t ciphertext_size = read_uint32();
    uint32_t tag_size = read_uint32();

    // Validate sizes
    if (offset + salt_size + iv_size + ciphertext_size + tag_size != data.size()) {
        return std::nullopt;
    }

    EncryptedData result;

    result.salt.assign(data.begin() + offset, data.begin() + offset + salt_size);
    offset += salt_size;

    result.iv.assign(data.begin() + offset, data.begin() + offset + iv_size);
    offset += iv_size;

    result.ciphertext.assign(data.begin() + offset, data.begin() + offset + ciphertext_size);
    offset += ciphertext_size;

    result.tag.assign(data.begin() + offset, data.begin() + offset + tag_size);

    return result;
}

// Derive key using PBKDF2-SHA256
std::vector<uint8_t> AES256_GCM::derive_key(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    int iterations
) {
    std::vector<uint8_t> key(KEY_SIZE);

    PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(KEY_SIZE),
        key.data()
    );

    return key;
}

// Encrypt with password
std::optional<AES256_GCM::EncryptedData> AES256_GCM::encrypt(
    const std::vector<uint8_t>& plaintext,
    const std::string& password,
    const std::vector<uint8_t>& additional_data
) {
    // Generate random salt
    std::vector<uint8_t> salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
        return std::nullopt;
    }

    // Derive key from password
    auto key = derive_key(password, salt, PBKDF2_ITERATIONS);

    return encrypt_with_key(plaintext, key, salt, additional_data);
}

// Internal encryption with derived key
std::optional<AES256_GCM::EncryptedData> AES256_GCM::encrypt_with_key(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& additional_data
) {
    EncryptedData result;
    result.salt = salt;

    // Generate random IV
    result.iv.resize(IV_SIZE);
    if (RAND_bytes(result.iv.data(), IV_SIZE) != 1) {
        return std::nullopt;
    }

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::nullopt;
    }

    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), result.iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }

    // Add additional authenticated data (AAD) if provided
    int len;
    if (!additional_data.empty()) {
        if (EVP_EncryptUpdate(ctx, nullptr, &len, additional_data.data(),
                             static_cast<int>(additional_data.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::nullopt;
        }
    }

    // Encrypt plaintext
    result.ciphertext.resize(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    if (EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len,
                         plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }
    int ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }
    ciphertext_len += len;
    result.ciphertext.resize(ciphertext_len);

    // Get authentication tag
    result.tag.resize(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, result.tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }

    EVP_CIPHER_CTX_free(ctx);
    return result;
}

// Decrypt with password
std::optional<std::vector<uint8_t>> AES256_GCM::decrypt(
    const EncryptedData& encrypted,
    const std::string& password,
    const std::vector<uint8_t>& additional_data
) {
    // Derive key from password
    auto key = derive_key(password, encrypted.salt, PBKDF2_ITERATIONS);

    return decrypt_with_key(encrypted, key, additional_data);
}

// Internal decryption with derived key
std::optional<std::vector<uint8_t>> AES256_GCM::decrypt_with_key(
    const EncryptedData& encrypted,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& additional_data
) {
    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::nullopt;
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), encrypted.iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }

    // Add additional authenticated data (AAD) if provided
    int len;
    if (!additional_data.empty()) {
        if (EVP_DecryptUpdate(ctx, nullptr, &len, additional_data.data(),
                             static_cast<int>(additional_data.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::nullopt;
        }
    }

    // Decrypt ciphertext
    std::vector<uint8_t> plaintext(encrypted.ciphertext.size());
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                         encrypted.ciphertext.data(),
                         static_cast<int>(encrypted.ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }
    int plaintext_len = len;

    // Set expected tag value
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                           const_cast<uint8_t*>(encrypted.tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::nullopt;
    }

    // Finalize decryption (verifies authentication tag)
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        // Authentication failed or decryption error
        return std::nullopt;
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    return plaintext;
}

// Verify password without decrypting
bool AES256_GCM::verify_password(
    const EncryptedData& encrypted,
    const std::string& password
) {
    // Derive key and attempt decrypt
    // If decryption succeeds, password is correct
    auto result = decrypt(encrypted, password);
    return result.has_value();
}

// Secure memory operations

void SecureMemory::secure_zero(void* ptr, size_t size) {
    if (!ptr || size == 0) return;

    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (size--) {
        *p++ = 0;
    }
}

void SecureMemory::secure_zero(std::vector<uint8_t>& vec) {
    secure_zero(vec.data(), vec.size());
}

void SecureMemory::secure_zero(std::string& str) {
    secure_zero(const_cast<char*>(str.data()), str.size());
}

bool SecureMemory::constant_time_compare(const void* a, const void* b, size_t size) {
    const uint8_t* pa = static_cast<const uint8_t*>(a);
    const uint8_t* pb = static_cast<const uint8_t*>(b);

    uint8_t result = 0;
    for (size_t i = 0; i < size; ++i) {
        result |= pa[i] ^ pb[i];
    }

    return result == 0;
}

bool SecureMemory::constant_time_compare(
    const std::vector<uint8_t>& a,
    const std::vector<uint8_t>& b
) {
    if (a.size() != b.size()) {
        return false;
    }
    return constant_time_compare(a.data(), b.data(), a.size());
}

} // namespace crypto
} // namespace intcoin
