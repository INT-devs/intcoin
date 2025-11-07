// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/transaction.h"
#include "intcoin/crypto.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace intcoin;

// Fuzz target for transaction deserialization
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Avoid extremely large inputs
    if (size > 1024 * 1024) {  // 1 MB limit
        return 0;
    }

    std::vector<uint8_t> buffer(data, data + size);

    try {
        // Test transaction deserialization
        Transaction tx = Transaction::deserialize(buffer);

        // If successful, try to serialize it back
        std::vector<uint8_t> serialized = tx.serialize();

        // Calculate hash (tests hashing logic)
        Hash256 hash = tx.get_hash();

        // Test validation
        tx.is_coinbase();

    } catch (...) {
        // Expected for invalid inputs
    }

    return 0;
}
