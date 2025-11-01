// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Address generation and validation (Base58Check encoding).

#include "intcoin/crypto.h"
#include <algorithm>
#include <cstring>

namespace intcoin {
namespace crypto {

namespace {
    // Base58 alphabet (Bitcoin-style)
    constexpr const char* BASE58_ALPHABET =
        "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    // Encode data to Base58
    std::string encode_base58(const std::vector<uint8_t>& data) {
        // Count leading zeros
        size_t leading_zeros = 0;
        for (auto byte : data) {
            if (byte != 0) break;
            leading_zeros++;
        }

        // Allocate enough space
        std::vector<uint8_t> b58((data.size() - leading_zeros) * 138 / 100 + 1);
        size_t length = 0;

        // Process the bytes
        for (size_t i = leading_zeros; i < data.size(); i++) {
            int carry = data[i];
            for (size_t j = 0; j < length; j++) {
                carry += 256 * b58[j];
                b58[j] = carry % 58;
                carry /= 58;
            }
            while (carry > 0) {
                b58[length++] = carry % 58;
                carry /= 58;
            }
        }

        // Convert to string
        std::string result(leading_zeros, '1');  // Leading 1s for leading zeros
        for (size_t i = 0; i < length; i++) {
            result.push_back(BASE58_ALPHABET[b58[length - 1 - i]]);
        }

        return result;
    }

    // Decode Base58 to data
    std::optional<std::vector<uint8_t>> decode_base58(const std::string& str) {
        // Count leading 1s
        size_t leading_ones = 0;
        for (auto ch : str) {
            if (ch != '1') break;
            leading_ones++;
        }

        // Allocate enough space
        std::vector<uint8_t> b256((str.size() - leading_ones) * 733 / 1000 + 1);
        size_t length = 0;

        // Process the characters
        for (size_t i = leading_ones; i < str.size(); i++) {
            const char* p = strchr(BASE58_ALPHABET, str[i]);
            if (!p) {
                return std::nullopt;  // Invalid character
            }

            int carry = p - BASE58_ALPHABET;
            for (size_t j = 0; j < length; j++) {
                carry += 58 * b256[j];
                b256[j] = carry % 256;
                carry /= 256;
            }
            while (carry > 0) {
                b256[length++] = carry % 256;
                carry /= 256;
            }
        }

        // Convert to result
        std::vector<uint8_t> result(leading_ones, 0);
        for (size_t i = 0; i < length; i++) {
            result.push_back(b256[length - 1 - i]);
        }

        return result;
    }

    // Add checksum to data
    std::vector<uint8_t> add_checksum(const std::vector<uint8_t>& data) {
        // Calculate checksum (first 4 bytes of double SHA3-256)
        auto hash = SHA3_256::double_hash(data);

        std::vector<uint8_t> result = data;
        result.insert(result.end(), hash.begin(), hash.begin() + 4);

        return result;
    }

    // Verify and remove checksum
    std::optional<std::vector<uint8_t>> verify_checksum(const std::vector<uint8_t>& data) {
        if (data.size() < 5) {
            return std::nullopt;
        }

        // Split data and checksum
        std::vector<uint8_t> payload(data.begin(), data.end() - 4);
        std::vector<uint8_t> checksum(data.end() - 4, data.end());

        // Calculate expected checksum
        auto hash = SHA3_256::double_hash(payload);

        // Verify checksum
        if (!std::equal(checksum.begin(), checksum.end(), hash.begin())) {
            return std::nullopt;
        }

        return payload;
    }
}

std::string Address::from_public_key(const DilithiumPubKey& pubkey) {
    return from_public_key(pubkey, Network::MAINNET);
}

std::string Address::from_public_key(
    const DilithiumPubKey& pubkey,
    Network network
) {
    // Hash the public key
    std::vector<uint8_t> pubkey_vec(pubkey.begin(), pubkey.end());
    auto hash = SHA3_256::hash(pubkey_vec);

    // Add version byte
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(network));
    payload.insert(payload.end(), hash.begin(), hash.end());

    // Add checksum and encode
    auto with_checksum = add_checksum(payload);
    return encode_base58(with_checksum);
}

bool Address::validate(const std::string& address) {
    // Decode
    auto decoded = decode_base58(address);
    if (!decoded) {
        return false;
    }

    // Verify checksum
    auto payload = verify_checksum(*decoded);
    if (!payload) {
        return false;
    }

    // Check version byte
    if (payload->size() != 33) {  // 1 byte version + 32 byte hash
        return false;
    }

    uint8_t version = (*payload)[0];
    return version == static_cast<uint8_t>(Network::MAINNET) ||
           version == static_cast<uint8_t>(Network::TESTNET);
}

std::optional<Hash256> Address::decode(const std::string& address) {
    // Decode
    auto decoded = decode_base58(address);
    if (!decoded) {
        return std::nullopt;
    }

    // Verify checksum
    auto payload = verify_checksum(*decoded);
    if (!payload) {
        return std::nullopt;
    }

    // Check size
    if (payload->size() != 33) {
        return std::nullopt;
    }

    // Extract hash (skip version byte)
    Hash256 hash;
    std::copy(payload->begin() + 1, payload->end(), hash.begin());

    return hash;
}

} // namespace crypto
} // namespace intcoin
