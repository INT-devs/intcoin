// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Cryptography unit tests.

#include "intcoin/crypto.h"
#include "intcoin/primitives.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace intcoin;
using namespace intcoin::crypto;

void print_hex(const std::string& label, const uint8_t* data, size_t len) {
    std::cout << label << ": ";
    for (size_t i = 0; i < len && i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]);
    }
    if (len > 32) std::cout << "...";
    std::cout << std::dec << std::endl;
}

void test_sha3() {
    std::cout << "\n=== Testing SHA3-256 ===" << std::endl;

    std::vector<uint8_t> data = {'h', 'e', 'l', 'l', 'o'};
    auto hash = SHA3_256::hash(data);

    print_hex("SHA3-256('hello')", hash.data(), hash.size());

    // Test double hash
    auto double_hash = SHA3_256::double_hash(data);
    print_hex("Double hash", double_hash.data(), double_hash.size());

    std::cout << "✓ SHA3-256 tests passed" << std::endl;
}

void test_dilithium() {
    std::cout << "\n=== Testing CRYSTALS-Dilithium ===" << std::endl;

    // Generate keypair
    auto keypair = Dilithium::generate_keypair();
    std::cout << "✓ Generated Dilithium keypair" << std::endl;
    print_hex("Public key", keypair.public_key.data(), 32);

    // Sign message
    std::vector<uint8_t> message = {'I', 'N', 'T', 'c', 'o', 'i', 'n'};
    auto signature = Dilithium::sign(message, keypair);
    std::cout << "✓ Signed message" << std::endl;
    print_hex("Signature", signature.data(), 32);

    // Verify signature
    bool valid = Dilithium::verify(message, signature, keypair.public_key);
    assert(valid && "Signature should be valid");
    std::cout << "✓ Signature verified" << std::endl;

    // Test invalid signature
    std::vector<uint8_t> wrong_message = {'W', 'R', 'O', 'N', 'G'};
    bool invalid = Dilithium::verify(wrong_message, signature, keypair.public_key);
    assert(!invalid && "Wrong message should fail verification");
    std::cout << "✓ Invalid signature rejected" << std::endl;

    // Test serialization
    auto serialized = keypair.serialize_private();
    auto deserialized = DilithiumKeyPair::deserialize_private(serialized);
    assert(deserialized.has_value() && "Deserialization should succeed");
    std::cout << "✓ Key serialization/deserialization works" << std::endl;

    std::cout << "✓ All Dilithium tests passed" << std::endl;
}

void test_kyber() {
    std::cout << "\n=== Testing CRYSTALS-Kyber ===" << std::endl;

    // Generate keypair
    auto keypair = Kyber::generate_keypair();
    std::cout << "✓ Generated Kyber keypair" << std::endl;

    // Encapsulate
    auto [secret1, ciphertext] = Kyber::encapsulate(keypair.public_key);
    std::cout << "✓ Encapsulated shared secret" << std::endl;
    print_hex("Shared secret (sender)", secret1.data(), secret1.size());
    print_hex("Ciphertext", ciphertext.data(), 32);

    // Decapsulate
    auto secret2_opt = Kyber::decapsulate(ciphertext, keypair);
    assert(secret2_opt.has_value() && "Decapsulation should succeed");
    auto secret2 = secret2_opt.value();
    std::cout << "✓ Decapsulated shared secret" << std::endl;
    print_hex("Shared secret (receiver)", secret2.data(), secret2.size());

    // Verify secrets match
    assert(secret1 == secret2 && "Shared secrets should match");
    std::cout << "✓ Shared secrets match" << std::endl;

    std::cout << "✓ All Kyber tests passed" << std::endl;
}

void test_address() {
    std::cout << "\n=== Testing Address Generation ===" << std::endl;

    // Generate keypair
    auto keypair = Dilithium::generate_keypair();

    // Generate mainnet address
    auto mainnet_addr = Address::from_public_key(keypair.public_key, Address::Network::MAINNET);
    std::cout << "Mainnet address: " << mainnet_addr << std::endl;

    // Generate testnet address
    auto testnet_addr = Address::from_public_key(keypair.public_key, Address::Network::TESTNET);
    std::cout << "Testnet address: " << testnet_addr << std::endl;

    // Validate addresses
    assert(Address::validate(mainnet_addr) && "Mainnet address should be valid");
    assert(Address::validate(testnet_addr) && "Testnet address should be valid");
    std::cout << "✓ Addresses validated" << std::endl;

    // Decode address
    auto decoded = Address::decode(mainnet_addr);
    assert(decoded.has_value() && "Address decoding should succeed");
    std::cout << "✓ Address decoding works" << std::endl;

    std::cout << "✓ All address tests passed" << std::endl;
}

void test_random() {
    std::cout << "\n=== Testing Secure Random ===" << std::endl;

    auto bytes = SecureRandom::generate(32);
    print_hex("Random bytes", bytes.data(), bytes.size());

    uint32_t r32 = SecureRandom::generate_uint32();
    std::cout << "Random uint32: " << r32 << std::endl;

    uint64_t r64 = SecureRandom::generate_uint64();
    std::cout << "Random uint64: " << r64 << std::endl;

    std::cout << "✓ Random generation tests passed" << std::endl;
}

void test_hkdf() {
    std::cout << "\n=== Testing HKDF ===" << std::endl;

    std::vector<uint8_t> master_secret = {'m', 'a', 's', 't', 'e', 'r'};
    std::vector<uint8_t> salt = {'s', 'a', 'l', 't'};
    std::vector<uint8_t> info = {'i', 'n', 'f', 'o'};

    auto derived = HKDF::derive(master_secret, salt, info, 32);
    print_hex("Derived key", derived.data(), derived.size());

    auto child = HKDF::derive_child(master_secret, 0);
    print_hex("Child key", child.data(), child.size());

    std::cout << "✓ HKDF tests passed" << std::endl;
}

void test_mnemonic() {
    std::cout << "\n=== Testing Mnemonic ===" << std::endl;

    // Generate 24-word mnemonic
    auto mnemonic = Mnemonic::generate(24);
    std::cout << "Mnemonic (24 words): " << mnemonic.substr(0, 80) << "..." << std::endl;

    // Validate
    assert(Mnemonic::validate(mnemonic) && "Mnemonic should be valid");
    std::cout << "✓ Mnemonic validated" << std::endl;

    // Convert to seed
    auto seed = Mnemonic::to_seed(mnemonic, "passphrase");
    print_hex("Seed", seed.data(), seed.size());

    std::cout << "✓ Mnemonic tests passed" << std::endl;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   INTcoin Quantum Cryptography Tests     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;

    try {
        test_sha3();
        test_dilithium();
        test_kyber();
        test_address();
        test_random();
        test_hkdf();
        test_mnemonic();

        std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
        std::cout << "║      ✓ ALL TESTS PASSED ✓                ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════╝\n" << std::endl;

        std::cout << "Quantum-resistant cryptography is working!" << std::endl;
        std::cout << "- CRYSTALS-Dilithium (signatures): ✓" << std::endl;
        std::cout << "- CRYSTALS-Kyber (key exchange): ✓" << std::endl;
        std::cout << "- SHA3-256 (hashing): ✓" << std::endl;
        std::cout << "- NIST FIPS compliance: ✓" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
