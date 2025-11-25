#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/mempool.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test mempool transaction insertion with various inputs
        std::vector<uint8_t> tx_data(data, data + size);
        
        // Test transaction parsing for mempool
        if (size > 4) {
            intcoin::Transaction tx;
            auto parse_result = tx.Deserialize(tx_data);
            
            // Test fee calculation
            if (parse_result) {
                uint64_t fee = tx.CalculateFee();
                (void)fee;  // Use the result
            }
        }
        
        // Test priority calculation
        if (size >= 32) {
            intcoin::MempoolEntry entry;
            auto priority = entry.CalculatePriority(size);
            (void)priority;
        }
        
        // Test ancestor/descendant tracking
        if (size > 0) {
            std::vector<uint8_t> tx_hash(data, data + std::min(size, size_t(32)));
            auto validation = intcoin::ValidateMempoolTransaction(tx_hash);
        }
        
        // Test eviction from mempool
        if (size > 20) {
            std::vector<uint8_t> evict_data(data, data + std::min(size, size_t(100)));
            auto eviction_check = intcoin::CheckMempoolEviction(evict_data);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
