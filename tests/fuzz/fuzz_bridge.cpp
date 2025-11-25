#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/bridge/bridge.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test cross-chain bridge message parsing
        std::vector<uint8_t> bridge_data(data, data + size);
        
        // Test atomic swap parsing
        if (size >= 32) {
            intcoin::AtomicSwap swap;
            auto parse_result = swap.Deserialize(bridge_data);
        }
        
        // Test SPV proof validation
        if (size > 80) {
            std::vector<uint8_t> proof_data(data, data + std::min(size, size_t(4096)));
            auto proof_validation = intcoin::ValidateSPVProof(proof_data);
        }
        
        // Test cross-chain address validation
        if (size > 10 && size < 500) {
            std::string chain_addr(reinterpret_cast<const char*>(data), size);
            auto addr_validation = intcoin::ValidateCrossChainAddress(chain_addr);
        }
        
        // Test bridge transaction parsing
        if (size > 4) {
            intcoin::BridgeTransaction bridge_tx;
            bridge_tx.Deserialize(bridge_data);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
