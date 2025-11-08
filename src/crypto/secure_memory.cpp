// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/crypto.h"
#include <cstring>
#include <openssl/crypto.h>

namespace intcoin {
namespace crypto {

void SecureMemory::secure_zero(void* ptr, size_t len) {
    if (ptr == nullptr || len == 0) {
        return;
    }
    // Use OpenSSL's secure memory clearing (prevents compiler optimization)
    OPENSSL_cleanse(ptr, len);
}

bool SecureMemory::constant_time_compare(const void* a, const void* b, size_t len) {
    if (a == nullptr || b == nullptr) {
        return a == b;
    }

    // Use OpenSSL's constant-time comparison
    return CRYPTO_memcmp(a, b, len) == 0;
}

} // namespace crypto
} // namespace intcoin
