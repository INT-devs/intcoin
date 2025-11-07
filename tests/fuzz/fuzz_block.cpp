// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace intcoin;

// Fuzz target for block deserialization
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Avoid extremely large inputs
    if (size > 10 * 1024 * 1024) {  // 10 MB limit
        return 0;
    }

    std::vector<uint8_t> buffer(data, data + size);

    try {
        // Test block deserialization
        Block block = Block::deserialize(buffer);

        // If successful, try to serialize it back
        std::vector<uint8_t> serialized = block.serialize();

        // Calculate block hash
        Hash256 hash = block.header.get_hash();

        // Calculate merkle root
        Hash256 merkle_root = block.calculate_merkle_root();

    } catch (...) {
        // Expected for invalid inputs
    }

    return 0;
}
