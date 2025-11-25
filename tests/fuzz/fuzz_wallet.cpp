#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/wallet.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test wallet address parsing
        std::string address(reinterpret_cast<const char*>(data), size);
        auto validation = intcoin::ValidateAddress(address);
        
        // Test private key import (if valid hex)
        if (size == 64 || size == 65) {
            std::string hex_key(reinterpret_cast<const char*>(data), size);
            auto key_validation = intcoin::ValidatePrivateKey(hex_key);
        }
        
        // Test mnemonic parsing
        if (size > 20 && size < 500) {
            std::string mnemonic(reinterpret_cast<const char*>(data), size);
            auto mnemonic_validation = intcoin::ValidateMnemonic(mnemonic);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
