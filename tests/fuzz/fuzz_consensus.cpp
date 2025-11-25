#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/pow.h"
#include "intcoin/consensus.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test PoW validation with various block headers
        std::vector<uint8_t> block_data(data, data + size);
        
        // Test block header parsing
        if (size >= 80) {
            intcoin::BlockHeader header;
            auto parse_result = header.Deserialize(block_data);
            
            // Test PoW verification
            if (parse_result) {
                auto is_valid = intcoin::ValidateProofOfWork(header);
                (void)is_valid;
            }
        }
        
        // Test difficulty adjustment
        if (size >= 4) {
            uint32_t target = *(uint32_t*)data;
            auto adjusted = intcoin::AdjustDifficulty(target, 10080);  // 1 week
            (void)adjusted;
        }
        
        // Test nonce validation
        if (size >= 4) {
            uint32_t nonce = *(uint32_t*)data;
            std::vector<uint8_t> block_hash(size >= 32 ? data : nullptr, 
                                           size >= 32 ? data + 32 : data);
            auto nonce_valid = intcoin::ValidateNonce(nonce, block_hash);
            (void)nonce_valid;
        }
        
        // Test consensus rules
        if (size > 0) {
            std::vector<uint8_t> consensus_data(data, data + std::min(size, size_t(256)));
            auto consensus_check = intcoin::CheckConsensusRules(consensus_data);
        }
        
        // Test block validation chain
        if (size > 80) {
            intcoin::Block block;
            auto block_parse = block.Deserialize(block_data);
            if (block_parse) {
                auto chain_validation = intcoin::ValidateBlockChain(block);
                (void)chain_validation;
            }
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
