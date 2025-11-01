// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Cryptographically secure random number generation.

#include "intcoin/crypto.h"
#include <openssl/rand.h>
#include <stdexcept>

namespace intcoin {
namespace crypto {

void SecureRandom::generate(uint8_t* buffer, size_t length) {
    if (RAND_bytes(buffer, static_cast<int>(length)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
}

std::vector<uint8_t> SecureRandom::generate(size_t length) {
    std::vector<uint8_t> result(length);
    generate(result.data(), length);
    return result;
}

uint32_t SecureRandom::generate_uint32() {
    uint32_t value;
    generate(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

uint64_t SecureRandom::generate_uint64() {
    uint64_t value;
    generate(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

} // namespace crypto
} // namespace intcoin
