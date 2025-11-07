// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/crypto.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace intcoin::crypto;

// Fuzz target for cryptographic operations
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Test various crypto operations with fuzzy input

    try {
        // Test SHA3-256
        if (size > 0 && size < 10000) {
            Hash256 hash = SHA3_256::hash(data, size);

            // Test double hash
            Hash256 double_hash = SHA3_256::double_hash(data, size);
        }

        // Test signature verification with random data
        if (size >= 64) {  // Minimum size for some crypto operations
            std::vector<uint8_t> message(data, data + std::min(size, size_t(1000)));

            // This should safely handle invalid signatures
            try {
                // Generate a keypair (always valid)
                auto keypair = Dilithium::generate_keypair();

                // Try to verify a potentially invalid signature
                if (size >= 4595) {  // Dilithium signature size
                    DilithiumSignature sig;
                    std::copy_n(data, std::min(size, sig.size()), sig.begin());

                    // This should handle invalid signatures gracefully
                    bool valid = Dilithium::verify(message, sig, keypair.public_key);
                }
            } catch (...) {
                // Expected for invalid crypto data
            }
        }

    } catch (...) {
        // Expected for invalid inputs
    }

    return 0;
}
