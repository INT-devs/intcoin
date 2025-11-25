#include <cstdint>
#include <cstring>
#include <vector>
#include "intcoin/lightning/channel.h"
#include "intcoin/validation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return 0;
    
    try {
        // Test Lightning channel message parsing
        std::vector<uint8_t> message_data(data, data + size);
        
        // Test channel state update parsing
        if (size >= 4) {
            intcoin::LightningChannel channel;
            auto parse_result = channel.ParseMessage(message_data);
        }
        
        // Test HTLC validation
        if (size >= 32) {
            std::vector<uint8_t> htlc_data(data, data + std::min(size, size_t(256)));
            auto htlc_validation = intcoin::ValidateHTLC(htlc_data);
        }
        
        // Test invoice parsing
        if (size > 20 && size < 10000) {
            std::string invoice(reinterpret_cast<const char*>(data), size);
            auto invoice_validation = intcoin::ValidateLightningInvoice(invoice);
        }
        
    } catch (...) {
        // Catch all exceptions during fuzzing
    }
    
    return 0;
}
