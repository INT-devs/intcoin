// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// HMAC-based Key Derivation Function (HKDF) using SHA3-256.

#include "intcoin/crypto.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/hmac.h>
#include <stdexcept>
#include <cstring>

namespace intcoin {
namespace crypto {

namespace {
    // HMAC-SHA3-256
    std::vector<uint8_t> hmac_sha3_256(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data
    ) {
        std::vector<uint8_t> result(32);
        unsigned int len = 32;

        if (!HMAC(
                EVP_sha3_256(),
                key.data(), key.size(),
                data.data(), data.size(),
                result.data(), &len
            )) {
            throw std::runtime_error("HMAC-SHA3-256 failed");
        }

        return result;
    }

    // HKDF Extract
    std::vector<uint8_t> hkdf_extract(
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& ikm
    ) {
        return hmac_sha3_256(salt, ikm);
    }

    // HKDF Expand
    std::vector<uint8_t> hkdf_expand(
        const std::vector<uint8_t>& prk,
        const std::vector<uint8_t>& info,
        size_t length
    ) {
        const size_t hash_len = 32;  // SHA3-256
        size_t n = (length + hash_len - 1) / hash_len;

        if (n > 255) {
            throw std::runtime_error("HKDF output too long");
        }

        std::vector<uint8_t> t;
        std::vector<uint8_t> result;

        for (size_t i = 1; i <= n; i++) {
            std::vector<uint8_t> data = t;
            data.insert(data.end(), info.begin(), info.end());
            data.push_back(static_cast<uint8_t>(i));

            t = hmac_sha3_256(prk, data);
            result.insert(result.end(), t.begin(), t.end());
        }

        result.resize(length);
        return result;
    }
}

std::vector<uint8_t> HKDF::derive(
    const std::vector<uint8_t>& master_secret,
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& info,
    size_t output_length
) {
    // HKDF-Extract
    auto prk = hkdf_extract(salt, master_secret);

    // HKDF-Expand
    return hkdf_expand(prk, info, output_length);
}

std::vector<uint8_t> HKDF::derive_child(
    const std::vector<uint8_t>& parent_key,
    uint32_t index
) {
    // Create info from index
    std::vector<uint8_t> info(4);
    info[0] = (index >> 24) & 0xFF;
    info[1] = (index >> 16) & 0xFF;
    info[2] = (index >> 8) & 0xFF;
    info[3] = index & 0xFF;

    // Use parent key as both master secret and salt
    return derive(parent_key, parent_key, info, 32);
}

} // namespace crypto
} // namespace intcoin
