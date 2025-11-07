// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/p2p.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace intcoin::p2p;

// Fuzz target for P2P message deserialization
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Avoid extremely large inputs
    if (size > 32 * 1024 * 1024) {  // Match MAX_MESSAGE_SIZE
        return 0;
    }

    std::vector<uint8_t> buffer(data, data + size);

    try {
        // Test message header deserialization
        if (size >= 44) {  // Minimum header size
            MessageHeader header = MessageHeader::deserialize(buffer);

            // Test full message deserialization
            Message msg = Message::deserialize(buffer);

            // Test serialization round-trip
            std::vector<uint8_t> serialized = msg.serialize();
        }

        // Test inventory vector
        if (size >= 36) {  // Min size for inv vector
            InvVector inv = InvVector::deserialize(buffer);
            std::vector<uint8_t> inv_serialized = inv.serialize();
        }

    } catch (...) {
        // Expected for invalid inputs
    }

    return 0;
}
