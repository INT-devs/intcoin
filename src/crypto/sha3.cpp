// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// SHA3-256 implementation using OpenSSL.

#include "intcoin/crypto.h"
#include <openssl/evp.h>
#include <cstring>
#include <stdexcept>

namespace intcoin {
namespace crypto {

struct SHA3_256::Impl {
    EVP_MD_CTX* ctx;

    Impl() : ctx(EVP_MD_CTX_new()) {
        if (!ctx) {
            throw std::runtime_error("Failed to create SHA3-256 context");
        }
        reset();
    }

    ~Impl() {
        if (ctx) {
            EVP_MD_CTX_free(ctx);
        }
    }

    void reset() {
        if (EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr) != 1) {
            throw std::runtime_error("Failed to initialize SHA3-256");
        }
    }
};

SHA3_256::SHA3_256() : impl_(std::make_unique<Impl>()) {}

SHA3_256::~SHA3_256() = default;

void SHA3_256::update(const uint8_t* data, size_t len) {
    if (EVP_DigestUpdate(impl_->ctx, data, len) != 1) {
        throw std::runtime_error("Failed to update SHA3-256");
    }
}

void SHA3_256::update(const std::vector<uint8_t>& data) {
    update(data.data(), data.size());
}

Hash256 SHA3_256::finalize() {
    Hash256 hash;
    unsigned int len = HASH_SIZE;

    if (EVP_DigestFinal_ex(impl_->ctx, hash.data(), &len) != 1) {
        throw std::runtime_error("Failed to finalize SHA3-256");
    }

    if (len != HASH_SIZE) {
        throw std::runtime_error("SHA3-256 produced incorrect hash size");
    }

    // Reset for reuse
    impl_->reset();

    return hash;
}

void SHA3_256::reset() {
    impl_->reset();
}

Hash256 SHA3_256::hash(const uint8_t* data, size_t len) {
    SHA3_256 hasher;
    hasher.update(data, len);
    return hasher.finalize();
}

Hash256 SHA3_256::hash(const std::vector<uint8_t>& data) {
    return hash(data.data(), data.size());
}

Hash256 SHA3_256::double_hash(const uint8_t* data, size_t len) {
    auto hash1 = hash(data, len);
    return hash(hash1.data(), hash1.size());
}

Hash256 SHA3_256::double_hash(const std::vector<uint8_t>& data) {
    return double_hash(data.data(), data.size());
}

} // namespace crypto
} // namespace intcoin
