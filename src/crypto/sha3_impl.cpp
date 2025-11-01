// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/crypto.h"
#include <openssl/evp.h>
#include <cstring>
#include <stdexcept>

namespace intcoin {
namespace crypto {

// SHA3-256 implementation using OpenSSL EVP interface
struct SHA3_256::Impl {
    EVP_MD_CTX* ctx;
    bool finalized;

    Impl() : ctx(EVP_MD_CTX_new()), finalized(false) {
        if (!ctx) {
            throw std::runtime_error("Failed to create SHA3-256 context");
        }
        if (EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize SHA3-256");
        }
    }

    ~Impl() {
        if (ctx) {
            EVP_MD_CTX_free(ctx);
        }
    }
};

SHA3_256::SHA3_256() : impl_(std::make_unique<Impl>()) {}

SHA3_256::~SHA3_256() = default;

void SHA3_256::update(const uint8_t* data, size_t len) {
    if (impl_->finalized) {
        throw std::runtime_error("Cannot update after finalize");
    }
    if (EVP_DigestUpdate(impl_->ctx, data, len) != 1) {
        throw std::runtime_error("SHA3-256 update failed");
    }
}

void SHA3_256::update(const std::vector<uint8_t>& data) {
    update(data.data(), data.size());
}

Hash256 SHA3_256::finalize() {
    if (impl_->finalized) {
        throw std::runtime_error("Already finalized");
    }

    Hash256 result;
    unsigned int len = 0;
    if (EVP_DigestFinal_ex(impl_->ctx, result.data(), &len) != 1) {
        throw std::runtime_error("SHA3-256 finalize failed");
    }

    if (len != 32) {
        throw std::runtime_error("SHA3-256 output length mismatch");
    }

    impl_->finalized = true;
    return result;
}

void SHA3_256::reset() {
    if (EVP_DigestInit_ex(impl_->ctx, EVP_sha3_256(), nullptr) != 1) {
        throw std::runtime_error("Failed to reset SHA3-256");
    }
    impl_->finalized = false;
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
    auto first = hash(data, len);
    return hash(first.data(), first.size());
}

Hash256 SHA3_256::double_hash(const std::vector<uint8_t>& data) {
    return double_hash(data.data(), data.size());
}

} // namespace crypto
} // namespace intcoin
