/*
 * Simple signing test to debug crash
 */

#include "intcoin/crypto.h"
#include <iostream>
#include <string>

using namespace intcoin;

int main() {
    std::cout << "Generating keypair..." << std::endl;
    auto keygen_result = DilithiumCrypto::GenerateKeyPair();
    if (keygen_result.IsError()) {
        std::cout << "Key generation failed: " << keygen_result.error << std::endl;
        return 1;
    }
    std::cout << "Keypair generated successfully" << std::endl;

    auto keypair = *keygen_result.value;

    std::cout << "Creating message..." << std::endl;
    std::string message_str = "Test message";
    std::vector<uint8_t> message(message_str.begin(), message_str.end());

    std::cout << "Signing message..." << std::endl;
    auto sign_result = DilithiumCrypto::Sign(message, keypair.secret_key);
    if (sign_result.IsError()) {
        std::cout << "Signing failed: " << sign_result.error << std::endl;
        return 1;
    }
    std::cout << "Signing succeeded!" << std::endl;

    return 0;
}
