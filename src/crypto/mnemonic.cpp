// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// BIP39-style mnemonic phrase generation for wallet seeds.

#include "intcoin/crypto.h"
#include <openssl/evp.h>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace intcoin {
namespace crypto {

namespace {
    // BIP39 English wordlist (2048 words)
    // This is a simplified version - in production, use the full BIP39 wordlist
    const std::vector<std::string> WORDLIST = {
        "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract",
        "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid",
        // ... (In production, include all 2048 BIP39 words)
        // For now, this is a stub showing the concept
        "zero", "zone", "zoo"
    };

    // Calculate checksum bits
    uint8_t calculate_checksum(const std::vector<uint8_t>& entropy) {
        auto hash = SHA3_256::hash(entropy);
        return hash[0];
    }

    // Convert entropy to word indices
    std::vector<size_t> entropy_to_indices(
        const std::vector<uint8_t>& entropy,
        size_t word_count
    ) {
        (void)word_count;  // TODO: Use for proper mnemonic encoding

        // Append checksum
        std::vector<uint8_t> data = entropy;
        uint8_t checksum = calculate_checksum(entropy);
        data.push_back(checksum);

        // Extract 11-bit indices
        std::vector<size_t> indices;
        size_t bit_pos = 0;

        for (size_t i = 0; i < word_count; i++) {
            size_t index = 0;

            for (size_t j = 0; j < 11; j++) {
                size_t byte_pos = bit_pos / 8;
                size_t bit_in_byte = bit_pos % 8;

                if (byte_pos < data.size()) {
                    bool bit = (data[byte_pos] >> (7 - bit_in_byte)) & 1;
                    index = (index << 1) | (bit ? 1 : 0);
                }

                bit_pos++;
            }

            indices.push_back(index % 2048);  // Ensure valid index
        }

        return indices;
    }

    // PBKDF2-HMAC-SHA512 for seed derivation
    std::vector<uint8_t> pbkdf2_hmac_sha512(
        const std::string& password,
        const std::string& salt,
        int iterations
    ) {
        std::vector<uint8_t> result(64);

        if (!PKCS5_PBKDF2_HMAC(
                password.c_str(), password.size(),
                reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                iterations,
                EVP_sha512(),
                64,
                result.data()
            )) {
            throw std::runtime_error("PBKDF2 failed");
        }

        return result;
    }
}

std::string Mnemonic::generate(size_t word_count) {
    // Validate word count (12, 15, 18, 21, 24)
    if (word_count != 12 && word_count != 15 && word_count != 18 &&
        word_count != 21 && word_count != 24) {
        throw std::invalid_argument("Invalid word count (must be 12, 15, 18, 21, or 24)");
    }

    // Calculate entropy size (in bytes)
    // 12 words = 128 bits = 16 bytes
    // 24 words = 256 bits = 32 bytes
    size_t entropy_bits = (word_count * 11) - (word_count / 3);
    size_t entropy_bytes = entropy_bits / 8;

    // Generate random entropy
    auto entropy = SecureRandom::generate(entropy_bytes);

    // Convert to word indices
    auto indices = entropy_to_indices(entropy, word_count);

    // Build mnemonic string
    std::ostringstream oss;
    for (size_t i = 0; i < indices.size(); i++) {
        if (i > 0) oss << " ";
        // In production, use actual BIP39 wordlist
        oss << "word" << indices[i];  // Stub
    }

    return oss.str();
}

std::vector<uint8_t> Mnemonic::to_seed(
    const std::string& mnemonic,
    const std::string& passphrase
) {
    // BIP39: salt = "mnemonic" + passphrase
    std::string salt = "mnemonic" + passphrase;

    // PBKDF2-HMAC-SHA512 with 2048 iterations
    return pbkdf2_hmac_sha512(mnemonic, salt, 2048);
}

bool Mnemonic::validate(const std::string& mnemonic) {
    // Split into words
    std::istringstream iss(mnemonic);
    std::vector<std::string> words;
    std::string word;

    while (iss >> word) {
        words.push_back(word);
    }

    // Check word count
    size_t word_count = words.size();
    if (word_count != 12 && word_count != 15 && word_count != 18 &&
        word_count != 21 && word_count != 24) {
        return false;
    }

    // In production: verify each word is in BIP39 wordlist
    // In production: verify checksum

    return true;  // Simplified validation
}

size_t Mnemonic::wordlist_size() {
    return 2048;  // BIP39 standard
}

} // namespace crypto
} // namespace intcoin
