// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// SHA-256 implementation for Proof of Work mining.

#include "intcoin/crypto.h"

// Suppress OpenSSL 3.0 deprecation warnings for SHA256 API
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/sha.h>
#include <cstring>
#include <stdexcept>

namespace intcoin {
namespace crypto {

/**
 * SHA-256 for Proof of Work
 *
 * We use classical SHA-256 for PoW because:
 * 1. It becomes ASIC-resistant in the quantum era
 * 2. Well-tested and proven algorithm
 * 3. Simple to implement and verify
 * 4. CPU-friendly mining
 *
 * Note: SHA3-256 is used for general hashing (transactions, merkle trees)
 *       SHA-256 is used specifically for PoW mining
 */

Hash256 SHA256_PoW::hash(const uint8_t* data, size_t len) {
    Hash256 result;

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(result.data(), &ctx);

    return result;
}

Hash256 SHA256_PoW::hash(const std::vector<uint8_t>& data) {
    return hash(data.data(), data.size());
}

Hash256 SHA256_PoW::double_hash(const uint8_t* data, size_t len) {
    auto hash1 = hash(data, len);
    return hash(hash1.data(), hash1.size());
}

Hash256 SHA256_PoW::double_hash(const std::vector<uint8_t>& data) {
    return double_hash(data.data(), data.size());
}

} // namespace crypto
} // namespace intcoin

#pragma clang diagnostic pop
