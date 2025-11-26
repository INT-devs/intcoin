/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Cryptography Test Suite (Dilithium3 + Kyber768 + SHA3)
 */

#include "intcoin/crypto.h"
#include "intcoin/types.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace intcoin;

void print_test_header(const std::string& test_name) {
    std::cout << "\n========================================\n";
    std::cout << test_name << "\n";
    std::cout << "========================================\n";
}

void print_result(const std::string& test, bool passed) {
    std::cout << test << ": " << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
}

// Test 1: SHA3-256 Hashing
bool test_sha3() {
    print_test_header("Test 1: SHA3-256 Hashing");

    // Test empty string
    std::vector<uint8_t> empty_data;
    uint256 hash1 = SHA3::Hash(empty_data);
    std::string expected1 = "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a";
    std::string result1 = ToHex(hash1);
    bool pass1 = (result1 == expected1);
    print_result("SHA3-256 empty string", pass1);

    // Test "abc"
    std::string input2 = "abc";
    std::vector<uint8_t> data2(input2.begin(), input2.end());
    uint256 hash2 = SHA3::Hash(data2);
    std::string expected2 = "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532";
    std::string result2 = ToHex(hash2);
    bool pass2 = (result2 == expected2);
    print_result("SHA3-256 'abc'", pass2);

    return pass1 && pass2;
}

// Test 2: Dilithium3 Key Generation
bool test_dilithium_keygen() {
    print_test_header("Test 2: Dilithium3 (ML-DSA-65) Key Generation");

    auto result = DilithiumCrypto::GenerateKeyPair();

    if (result.IsError()) {
        std::cout << "Error: " << result.error << std::endl;
        print_result("Dilithium3 key generation", false);
        return false;
    }

    auto keypair = *result.value;

    // Check that keys are not all zeros
    bool pk_nonzero = false;
    bool sk_nonzero = false;

    for (size_t i = 0; i < keypair.public_key.size(); ++i) {
        if (keypair.public_key[i] != 0) {
            pk_nonzero = true;
            break;
        }
    }

    for (size_t i = 0; i < keypair.secret_key.size(); ++i) {
        if (keypair.secret_key[i] != 0) {
            sk_nonzero = true;
            break;
        }
    }

    bool pass = pk_nonzero && sk_nonzero;
    print_result("Dilithium3 keys non-zero", pass);

    std::cout << "Public key size: " << keypair.public_key.size() << " bytes" << std::endl;
    std::cout << "Secret key size: " << keypair.secret_key.size() << " bytes" << std::endl;

    return pass;
}

// Test 3: Dilithium3 Sign and Verify
bool test_dilithium_sign_verify() {
    print_test_header("Test 3: Dilithium3 (ML-DSA-65) Sign & Verify");

    // Generate keypair
    auto keygen_result = DilithiumCrypto::GenerateKeyPair();
    if (keygen_result.IsError()) {
        std::cout << "Key generation failed: " << keygen_result.error << std::endl;
        return false;
    }
    auto keypair = *keygen_result.value;

    // Create test message
    std::string message_str = "INTcoin: Quantum-resistant cryptocurrency";
    std::vector<uint8_t> message(message_str.begin(), message_str.end());

    // Sign message
    auto sign_result = DilithiumCrypto::Sign(message, keypair.secret_key);
    if (sign_result.IsError()) {
        std::cout << "Signing failed: " << sign_result.error << std::endl;
        print_result("Dilithium3 signing", false);
        return false;
    }
    auto signature = *sign_result.value;
    print_result("Dilithium3 signing", true);

    std::cout << "Signature size: " << signature.size() << " bytes" << std::endl;

    // Verify signature
    auto verify_result = DilithiumCrypto::Verify(message, signature, keypair.public_key);
    bool verify_pass = !verify_result.IsError();
    print_result("Dilithium3 verification (valid)", verify_pass);

    // Test invalid signature (modify one byte)
    Signature bad_signature = signature;
    bad_signature[0] ^= 0xFF;
    auto verify_bad = DilithiumCrypto::Verify(message, bad_signature, keypair.public_key);
    bool verify_bad_fails = verify_bad.IsError();
    print_result("Dilithium3 verification (invalid)", verify_bad_fails);

    return verify_pass && verify_bad_fails;
}

// Test 4: Kyber768 Key Generation
bool test_kyber_keygen() {
    print_test_header("Test 4: Kyber768 (ML-KEM-768) Key Generation");

    auto result = KyberCrypto::GenerateKeyPair();

    if (result.IsError()) {
        std::cout << "Error: " << result.error << std::endl;
        print_result("Kyber768 key generation", false);
        return false;
    }

    auto keypair = *result.value;

    // Check that keys are not all zeros
    bool pk_nonzero = false;
    bool sk_nonzero = false;

    for (size_t i = 0; i < keypair.public_key.size(); ++i) {
        if (keypair.public_key[i] != 0) {
            pk_nonzero = true;
            break;
        }
    }

    for (size_t i = 0; i < keypair.secret_key.size(); ++i) {
        if (keypair.secret_key[i] != 0) {
            sk_nonzero = true;
            break;
        }
    }

    bool pass = pk_nonzero && sk_nonzero;
    print_result("Kyber768 keys non-zero", pass);

    std::cout << "Public key size: " << keypair.public_key.size() << " bytes" << std::endl;
    std::cout << "Secret key size: " << keypair.secret_key.size() << " bytes" << std::endl;

    return pass;
}

// Test 5: Kyber768 Encapsulate and Decapsulate
bool test_kyber_encap_decap() {
    print_test_header("Test 5: Kyber768 (ML-KEM-768) Encap & Decap");

    // Generate keypair
    auto keygen_result = KyberCrypto::GenerateKeyPair();
    if (keygen_result.IsError()) {
        std::cout << "Key generation failed: " << keygen_result.error << std::endl;
        return false;
    }
    auto keypair = *keygen_result.value;

    // Encapsulate
    auto encap_result = KyberCrypto::Encapsulate(keypair.public_key);
    if (encap_result.IsError()) {
        std::cout << "Encapsulation failed: " << encap_result.error << std::endl;
        print_result("Kyber768 encapsulation", false);
        return false;
    }
    auto [shared_secret_1, ciphertext] = *encap_result.value;
    print_result("Kyber768 encapsulation", true);

    std::cout << "Shared secret size: " << shared_secret_1.size() << " bytes" << std::endl;
    std::cout << "Ciphertext size: " << ciphertext.size() << " bytes" << std::endl;

    // Decapsulate
    auto decap_result = KyberCrypto::Decapsulate(ciphertext, keypair.secret_key);
    if (decap_result.IsError()) {
        std::cout << "Decapsulation failed: " << decap_result.error << std::endl;
        print_result("Kyber768 decapsulation", false);
        return false;
    }
    auto shared_secret_2 = *decap_result.value;
    print_result("Kyber768 decapsulation", true);

    // Verify shared secrets match
    bool secrets_match = (shared_secret_1 == shared_secret_2);
    print_result("Kyber768 shared secrets match", secrets_match);

    // Verify secrets are not all zeros
    bool secret_nonzero = false;
    for (size_t i = 0; i < shared_secret_1.size(); ++i) {
        if (shared_secret_1[i] != 0) {
            secret_nonzero = true;
            break;
        }
    }
    print_result("Kyber768 shared secret non-zero", secret_nonzero);

    return secrets_match && secret_nonzero;
}

int main() {
    std::cout << "INTcoin Cryptography Test Suite\n";
    std::cout << "Testing: SHA3-256, Dilithium3 (ML-DSA-65), Kyber768 (ML-KEM-768)\n";

    int passed = 0;
    int total = 5;

    if (test_sha3()) passed++;
    if (test_dilithium_keygen()) passed++;
    if (test_dilithium_sign_verify()) passed++;
    if (test_kyber_keygen()) passed++;
    if (test_kyber_encap_decap()) passed++;

    std::cout << "\n========================================\n";
    std::cout << "FINAL RESULTS: " << passed << "/" << total << " tests passed\n";
    std::cout << "========================================\n";

    if (passed == total) {
        std::cout << "✅ ALL TESTS PASSED!\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}
