// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/crypto.h"
#include <openssl/sha.h>
#include <cstring>

namespace intcoin {
namespace crypto {

// SHA-256 for Proof of Work mining
Hash256 SHA256_PoW::hash(const uint8_t* data, size_t len) {
    Hash256 result;
    SHA256(data, len, result.data());
    return result;
}

Hash256 SHA256_PoW::hash(const std::vector<uint8_t>& data) {
    return hash(data.data(), data.size());
}

Hash256 SHA256_PoW::double_hash(const uint8_t* data, size_t len) {
    Hash256 first;
    SHA256(data, len, first.data());

    Hash256 result;
    SHA256(first.data(), first.size(), result.data());
    return result;
}

Hash256 SHA256_PoW::double_hash(const std::vector<uint8_t>& data) {
    return double_hash(data.data(), data.size());
}

} // namespace crypto
} // namespace intcoin
