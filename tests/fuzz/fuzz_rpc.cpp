// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// Fuzz target for RPC JSON parsing
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Avoid extremely large inputs
    if (size > 1024 * 1024) {  // 1 MB limit
        return 0;
    }

    try {
        // Convert to string
        std::string json_str(reinterpret_cast<const char*>(data), size);

        // Test JSON parsing
        // In real implementation, this would parse RPC requests
        // For now, just test that we don't crash on arbitrary input

        // Test various JSON-like patterns
        if (json_str.find("{") != std::string::npos) {
            // Could be JSON object
        }

        if (json_str.find("jsonrpc") != std::string::npos) {
            // Could be valid RPC request
        }

        // Test parameter extraction
        if (json_str.find("params") != std::string::npos) {
            // Has parameters
        }

    } catch (...) {
        // Expected for invalid JSON
    }

    return 0;
}
