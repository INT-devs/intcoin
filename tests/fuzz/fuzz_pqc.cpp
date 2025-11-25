#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/pqc/dilithium.h"
#include "intcoin/pqc/kyber.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test Dilithium5 signature verification with malformed inputs
        if (size >= 4595) {
            // Dilithium5 signature is 4595 bytes
            std::vector<uint8_t> signature(data, data + 4595);
            std::vector<uint8_t> message(data + 4595, data + size);
            
            auto dil_validation = intcoin::ValidateDilithiumSignature(signature, message);
        }
        
        // Test Kyber1024 key encapsulation with invalid data
        if (size >= 1568) {
            // Kyber1024 ciphertext is 1568 bytes
            std::vector<uint8_t> ciphertext(data, data + 1568);
            auto kyber_validation = intcoin::ValidateKyberCiphertext(ciphertext);
        }
        
        // Test public key validation
        if (size >= 1312) {
            // Kyber1024 public key is 1312 bytes
            std::vector<uint8_t> pubkey(data, data + 1312);
            auto pubkey_validation = intcoin::ValidateQuantumPubkey(pubkey);
        }
        
        // Test PQC parameter validation
        if (size > 0) {
            uint8_t param_id = data[0];
            auto param_validation = intcoin::ValidatePQCParameter(param_id);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
