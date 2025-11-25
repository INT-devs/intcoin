#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/contracts/vm.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test smart contract bytecode parsing
        std::vector<uint8_t> bytecode(data, data + size);
        
        // Test bytecode validation
        auto validation = intcoin::ValidateBytecode(bytecode);
        
        // Test opcode parsing
        if (size >= 1) {
            uint8_t opcode = data[0];
            auto opcode_info = intcoin::GetOpcodeInfo(opcode);
        }
        
        // Test contract deserialization
        if (size > 4) {
            intcoin::SmartContract contract;
            contract.DeserializeFromBinary(bytecode);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
