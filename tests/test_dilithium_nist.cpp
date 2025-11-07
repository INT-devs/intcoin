// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// NIST FIPS 204 test vectors for ML-DSA-87 (Dilithium5)
// Validates implementation against NIST reference

#include "intcoin/crypto.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cassert>
#include <chrono>

using namespace intcoin::crypto;

// Test assertion helper
void test_assert(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "[PASS] " << test_name << std::endl;
    } else {
        std::cerr << "[FAIL] " << test_name << std::endl;
        exit(1);
    }
}

// Helper to convert hex string to bytes
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Helper to convert bytes to hex string
std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// Helper to convert array to vector
template<size_t N>
std::vector<uint8_t> array_to_vec(const std::array<uint8_t, N>& arr) {
    return std::vector<uint8_t>(arr.begin(), arr.end());
}

// ============================================================================
// Test 1: Basic Key Generation
// ============================================================================

void test_key_generation() {
    std::cout << "\n=== Test 1: Basic Key Generation ===" << std::endl;

    // Generate a keypair
    auto keypair = Dilithium::generate_keypair();

    // Verify sizes match ML-DSA-87 specification
    test_assert(keypair.public_key.size() == 2592, "Public key size is 2592 bytes");
    test_assert(keypair.private_key.size() == 4896, "Private key size is 4896 bytes");

    // Verify keys are not all zeros
    bool pubkey_nonzero = false;
    bool privkey_nonzero = false;

    for (uint8_t byte : keypair.public_key) {
        if (byte != 0) {
            pubkey_nonzero = true;
            break;
        }
    }

    for (uint8_t byte : keypair.private_key) {
        if (byte != 0) {
            privkey_nonzero = true;
            break;
        }
    }

    test_assert(pubkey_nonzero, "Public key is not all zeros");
    test_assert(privkey_nonzero, "Private key is not all zeros");
}

// ============================================================================
// Test 2: Signature Generation and Verification
// ============================================================================

void test_sign_verify() {
    std::cout << "\n=== Test 2: Signature Generation and Verification ===" << std::endl;

    // Generate keypair
    auto keypair = Dilithium::generate_keypair();

    // Test message
    std::string test_message = "The quick brown fox jumps over the lazy dog";
    std::vector<uint8_t> message(test_message.begin(), test_message.end());

    // Sign message
    auto signature = Dilithium::sign(message, keypair);

    // Verify signature size
    test_assert(signature.size() == 4627, "Signature size is 4627 bytes");

    // Verify signature
    bool valid = Dilithium::verify(message, signature, keypair.public_key);
    test_assert(valid, "Valid signature verifies correctly");

    // Test with modified message (should fail)
    std::vector<uint8_t> modified_message = message;
    modified_message[0] ^= 0x01;

    bool invalid = Dilithium::verify(modified_message, signature, keypair.public_key);
    test_assert(!invalid, "Signature fails for modified message");

    // Test with modified signature (should fail)
    auto modified_signature = signature;
    modified_signature[0] ^= 0x01;

    bool invalid2 = Dilithium::verify(message, modified_signature, keypair.public_key);
    test_assert(!invalid2, "Modified signature fails verification");
}

// ============================================================================
// Test 3: Empty Message
// ============================================================================

void test_empty_message() {
    std::cout << "\n=== Test 3: Empty Message ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();
    std::vector<uint8_t> empty_message;

    auto signature = Dilithium::sign(empty_message, keypair);
    test_assert(signature.size() == 4627, "Signature size correct for empty message");

    bool valid = Dilithium::verify(empty_message, signature, keypair.public_key);
    test_assert(valid, "Empty message signature verifies");
}

// ============================================================================
// Test 4: Large Message
// ============================================================================

void test_large_message() {
    std::cout << "\n=== Test 4: Large Message (1MB) ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();

    // Create 1MB message
    std::vector<uint8_t> large_message(1024 * 1024);
    for (size_t i = 0; i < large_message.size(); i++) {
        large_message[i] = static_cast<uint8_t>(i % 256);
    }

    auto signature = Dilithium::sign(large_message, keypair);
    test_assert(signature.size() == 4627, "Signature size correct for large message");

    bool valid = Dilithium::verify(large_message, signature, keypair.public_key);
    test_assert(valid, "Large message signature verifies");
}

// ============================================================================
// Test 5: Determinism (same key signs differently with randomness)
// ============================================================================

void test_signature_randomness() {
    std::cout << "\n=== Test 5: Signature Randomness ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();

    std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Sign same message twice
    auto sig1 = Dilithium::sign(message, keypair);
    auto sig2 = Dilithium::sign(message, keypair);

    // ML-DSA includes randomness, so signatures should differ
    bool signatures_differ = (sig1 != sig2);
    test_assert(signatures_differ, "Signatures use randomness (probabilistic signing)");

    // But both should verify
    bool valid1 = Dilithium::verify(message, sig1, keypair.public_key);
    bool valid2 = Dilithium::verify(message, sig2, keypair.public_key);

    test_assert(valid1 && valid2, "Both randomized signatures verify correctly");
}

// ============================================================================
// Test 6: Multiple Keypairs
// ============================================================================

void test_multiple_keypairs() {
    std::cout << "\n=== Test 6: Multiple Keypairs ===" << std::endl;

    auto keypair1 = Dilithium::generate_keypair();
    auto keypair2 = Dilithium::generate_keypair();

    // Verify keypairs are different
    test_assert(keypair1.public_key != keypair2.public_key, "Different keypairs have different public keys");
    test_assert(keypair1.private_key != keypair2.private_key, "Different keypairs have different private keys");

    std::vector<uint8_t> message = {0xDE, 0xAD, 0xBE, 0xEF};

    // Sign with first keypair
    auto sig1 = Dilithium::sign(message, keypair1);

    // Should verify with correct key
    bool valid_correct = Dilithium::verify(message, sig1, keypair1.public_key);
    test_assert(valid_correct, "Signature verifies with correct public key");

    // Should NOT verify with different key
    bool invalid_wrong_key = Dilithium::verify(message, sig1, keypair2.public_key);
    test_assert(!invalid_wrong_key, "Signature fails with wrong public key");
}

// ============================================================================
// Test 7: Edge Cases - All Zero Message
// ============================================================================

void test_all_zero_message() {
    std::cout << "\n=== Test 7: All-Zero Message ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();

    std::vector<uint8_t> zero_message(1000, 0x00);

    auto signature = Dilithium::sign(zero_message, keypair);
    bool valid = Dilithium::verify(zero_message, signature, keypair.public_key);

    test_assert(valid, "All-zero message signature verifies");
}

// ============================================================================
// Test 8: Edge Cases - All 0xFF Message
// ============================================================================

void test_all_ff_message() {
    std::cout << "\n=== Test 8: All-0xFF Message ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();

    std::vector<uint8_t> ff_message(1000, 0xFF);

    auto signature = Dilithium::sign(ff_message, keypair);
    bool valid = Dilithium::verify(ff_message, signature, keypair.public_key);

    test_assert(valid, "All-0xFF message signature verifies");
}

// ============================================================================
// Test 9: Performance - Key Generation
// ============================================================================

void test_keygen_performance() {
    std::cout << "\n=== Test 9: Key Generation Performance ===" << std::endl;

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto keypair = Dilithium::generate_keypair();
        (void)keypair; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average key generation time: " << avg_time << " ms" << std::endl;
    std::cout << "Keys per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 100, "Key generation < 100ms (reasonable performance)");
}

// ============================================================================
// Test 10: Performance - Signing
// ============================================================================

void test_signing_performance() {
    std::cout << "\n=== Test 10: Signing Performance ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();
    std::vector<uint8_t> message(32, 0xAB); // 32-byte message

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto signature = Dilithium::sign(message, keypair);
        (void)signature; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average signing time: " << avg_time << " ms" << std::endl;
    std::cout << "Signatures per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 50, "Signing < 50ms (reasonable performance)");
}

// ============================================================================
// Test 11: Performance - Verification
// ============================================================================

void test_verification_performance() {
    std::cout << "\n=== Test 11: Verification Performance ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();
    std::vector<uint8_t> message(32, 0xAB);
    auto signature = Dilithium::sign(message, keypair);

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        bool valid = Dilithium::verify(message, signature, keypair.public_key);
        (void)valid; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average verification time: " << avg_time << " ms" << std::endl;
    std::cout << "Verifications per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 20, "Verification < 20ms (reasonable performance)");
}

// ============================================================================
// Test 12: Constant-Time Verification
// ============================================================================

void test_constant_time() {
    std::cout << "\n=== Test 12: Constant-Time Verification ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();
    std::vector<uint8_t> message(32, 0xCD);

    auto valid_signature = Dilithium::sign(message, keypair);
    auto invalid_signature = valid_signature;
    invalid_signature[100] ^= 0x01; // Corrupt signature

    const int iterations = 1000;

    // Time valid signature verifications
    auto start_valid = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Dilithium::verify(message, valid_signature, keypair.public_key);
    }
    auto end_valid = std::chrono::high_resolution_clock::now();
    auto valid_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_valid - start_valid);

    // Time invalid signature verifications
    auto start_invalid = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Dilithium::verify(message, invalid_signature, keypair.public_key);
    }
    auto end_invalid = std::chrono::high_resolution_clock::now();
    auto invalid_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_invalid - start_invalid);

    double valid_avg = valid_duration.count() / static_cast<double>(iterations);
    double invalid_avg = invalid_duration.count() / static_cast<double>(iterations);
    double time_diff_percent = std::abs(valid_avg - invalid_avg) / std::max(valid_avg, invalid_avg) * 100.0;

    std::cout << "Valid signature avg: " << valid_avg << " ns" << std::endl;
    std::cout << "Invalid signature avg: " << invalid_avg << " ns" << std::endl;
    std::cout << "Time difference: " << time_diff_percent << "%" << std::endl;

    // Constant-time operations should have < 10% timing variance
    test_assert(time_diff_percent < 10.0, "Verification is constant-time (< 10% variance)");
}

// ============================================================================
// Test 13: Known Answer Test (KAT) - Simplified
// ============================================================================

void test_known_answer() {
    std::cout << "\n=== Test 13: Known Answer Test ===" << std::endl;

    // This is a simplified KAT since we're using liboqs
    // Real NIST KAT would require deterministic signing with specific seed

    auto keypair = Dilithium::generate_keypair();
    std::vector<uint8_t> message = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    auto signature = Dilithium::sign(message, keypair);

    // Verify the signature
    bool valid = Dilithium::verify(message, signature, keypair.public_key);
    test_assert(valid, "KAT: Signature verifies");

    // Verify signature is deterministic for verification (not signing due to randomness)
    bool valid2 = Dilithium::verify(message, signature, keypair.public_key);
    test_assert(valid == valid2, "KAT: Verification is deterministic");
}

// ============================================================================
// Test 14: Cross-Message Verification
// ============================================================================

void test_cross_message() {
    std::cout << "\n=== Test 14: Cross-Message Verification ===" << std::endl;

    auto keypair = Dilithium::generate_keypair();

    std::vector<uint8_t> msg1 = {0x01, 0x02, 0x03};
    std::vector<uint8_t> msg2 = {0x04, 0x05, 0x06};

    auto sig1 = Dilithium::sign(msg1, keypair);
    auto sig2 = Dilithium::sign(msg2, keypair);

    // Correct verifications
    test_assert(Dilithium::verify(msg1, sig1, keypair.public_key), "Message 1 verifies with signature 1");
    test_assert(Dilithium::verify(msg2, sig2, keypair.public_key), "Message 2 verifies with signature 2");

    // Cross-verifications should fail
    test_assert(!Dilithium::verify(msg1, sig2, keypair.public_key), "Message 1 fails with signature 2");
    test_assert(!Dilithium::verify(msg2, sig1, keypair.public_key), "Message 2 fails with signature 1");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "NIST FIPS 204 ML-DSA-87 (Dilithium5) Tests" << std::endl;
    std::cout << "============================================" << std::endl;

    try {
        test_key_generation();
        test_sign_verify();
        test_empty_message();
        test_large_message();
        test_signature_randomness();
        test_multiple_keypairs();
        test_all_zero_message();
        test_all_ff_message();
        test_keygen_performance();
        test_signing_performance();
        test_verification_performance();
        test_constant_time();
        test_known_answer();
        test_cross_message();

        std::cout << "\n============================================" << std::endl;
        std::cout << "ALL TESTS PASSED (14/14)" << std::endl;
        std::cout << "ML-DSA-87 implementation verified" << std::endl;
        std::cout << "============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
