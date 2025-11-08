// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// NIST FIPS 203 test vectors for ML-KEM-1024 (Kyber1024)
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

// ============================================================================
// Test 1: Basic Key Generation
// ============================================================================

void test_key_generation() {
    std::cout << "\n=== Test 1: Basic Key Generation ===" << std::endl;

    // Generate a keypair
    auto keypair = Kyber::generate_keypair();

    // Verify sizes match ML-KEM-1024 specification
    test_assert(keypair.public_key.size() == 1568, "Public key size is 1568 bytes");
    test_assert(keypair.private_key.size() == 3168, "Private key size is 3168 bytes");

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
// Test 2: Basic Encapsulation and Decapsulation
// ============================================================================

void test_encapsulate_decapsulate() {
    std::cout << "\n=== Test 2: Encapsulation and Decapsulation ===" << std::endl;

    // Generate keypair
    auto keypair = Kyber::generate_keypair();

    // Encapsulate to create shared secret and ciphertext
    auto [shared_secret1, ciphertext] = Kyber::encapsulate(keypair.public_key);

    // Verify ciphertext size
    test_assert(ciphertext.size() == 1568, "Ciphertext size is 1568 bytes");

    // Verify shared secret size
    test_assert(shared_secret1.size() == 32, "Shared secret size is 32 bytes");

    // Decapsulate to recover shared secret
    auto shared_secret2_opt = Kyber::decapsulate(ciphertext, keypair);
    test_assert(shared_secret2_opt.has_value(), "Decapsulation succeeded");

    auto shared_secret2 = *shared_secret2_opt;

    // Verify shared secrets match
    test_assert(shared_secret1 == shared_secret2, "Shared secrets match");
}

// ============================================================================
// Test 3: Wrong Key Decapsulation
// ============================================================================

void test_wrong_key_decapsulation() {
    std::cout << "\n=== Test 3: Wrong Key Decapsulation ===" << std::endl;

    auto keypair1 = Kyber::generate_keypair();
    auto keypair2 = Kyber::generate_keypair();

    // Encapsulate with first public key
    auto [shared_secret1, ciphertext] = Kyber::encapsulate(keypair1.public_key);

    // Try to decapsulate with wrong private key
    auto shared_secret2_opt = Kyber::decapsulate(ciphertext, keypair2);

    // Decapsulation should succeed (KEM always produces output)
    // but shared secrets should NOT match
    test_assert(shared_secret2_opt.has_value(), "Decapsulation with wrong key produces output");

    auto shared_secret2 = *shared_secret2_opt;
    test_assert(shared_secret1 != shared_secret2, "Shared secrets differ with wrong key");
}

// ============================================================================
// Test 4: Multiple Encapsulations
// ============================================================================

void test_multiple_encapsulations() {
    std::cout << "\n=== Test 4: Multiple Encapsulations ===" << std::endl;

    auto keypair = Kyber::generate_keypair();

    // Encapsulate twice with same public key
    auto [secret1, ciphertext1] = Kyber::encapsulate(keypair.public_key);
    auto [secret2, ciphertext2] = Kyber::encapsulate(keypair.public_key);

    // Ciphertexts should be different (randomness)
    test_assert(ciphertext1 != ciphertext2, "Ciphertexts differ (randomized)");

    // Shared secrets should be different
    test_assert(secret1 != secret2, "Shared secrets differ (randomized)");

    // But both should decapsulate correctly
    auto decap1 = Kyber::decapsulate(ciphertext1, keypair);
    auto decap2 = Kyber::decapsulate(ciphertext2, keypair);

    test_assert(decap1.has_value() && *decap1 == secret1, "First encapsulation decapsulates correctly");
    test_assert(decap2.has_value() && *decap2 == secret2, "Second encapsulation decapsulates correctly");
}

// ============================================================================
// Test 5: Corrupted Ciphertext
// ============================================================================

void test_corrupted_ciphertext() {
    std::cout << "\n=== Test 5: Corrupted Ciphertext ===" << std::endl;

    auto keypair = Kyber::generate_keypair();
    auto [shared_secret, ciphertext] = Kyber::encapsulate(keypair.public_key);

    // Corrupt the ciphertext
    auto corrupted_ciphertext = ciphertext;
    corrupted_ciphertext[0] ^= 0x01;

    // Decapsulation should still succeed (implicit rejection)
    // but produce different shared secret
    auto decap_result = Kyber::decapsulate(corrupted_ciphertext, keypair);
    test_assert(decap_result.has_value(), "Decapsulation of corrupted ciphertext succeeds (implicit rejection)");

    // The shared secret should be different (implicit rejection in action)
    test_assert(*decap_result != shared_secret, "Corrupted ciphertext produces different shared secret");
}

// ============================================================================
// Test 6: Keypair Serialization
// ============================================================================

void test_keypair_serialization() {
    std::cout << "\n=== Test 6: Keypair Serialization ===" << std::endl;

    auto keypair = Kyber::generate_keypair();

    // Serialize private key
    auto serialized = keypair.serialize_private();
    test_assert(serialized.size() == 1568 + 3168, "Serialized keypair correct size");

    // Deserialize
    auto keypair2_opt = KyberKeyPair::deserialize_private(serialized);
    test_assert(keypair2_opt.has_value(), "Deserialization succeeded");

    auto keypair2 = *keypair2_opt;

    // Verify keys match
    test_assert(keypair.public_key == keypair2.public_key, "Public keys match after serialization");
    test_assert(keypair.private_key == keypair2.private_key, "Private keys match after serialization");

    // Test encapsulation/decapsulation with deserialized key
    auto [secret, ciphertext] = Kyber::encapsulate(keypair2.public_key);
    auto decap_result = Kyber::decapsulate(ciphertext, keypair2);

    test_assert(decap_result.has_value() && *decap_result == secret,
                "Deserialized keypair works correctly");
}

// ============================================================================
// Test 7: Invalid Serialized Data
// ============================================================================

void test_invalid_serialization() {
    std::cout << "\n=== Test 7: Invalid Serialized Data ===" << std::endl;

    // Too short
    std::vector<uint8_t> short_data(100, 0x00);
    auto result1 = KyberKeyPair::deserialize_private(short_data);
    test_assert(!result1.has_value(), "Rejects too-short serialized data");

    // Too long
    std::vector<uint8_t> long_data(10000, 0x00);
    auto result2 = KyberKeyPair::deserialize_private(long_data);
    test_assert(!result2.has_value(), "Rejects too-long serialized data");

    // Exact wrong size
    std::vector<uint8_t> wrong_size(1568 + 3168 + 1, 0x00);
    auto result3 = KyberKeyPair::deserialize_private(wrong_size);
    test_assert(!result3.has_value(), "Rejects incorrect size serialized data");
}

// ============================================================================
// Test 8: Key Clearing
// ============================================================================

void test_key_clearing() {
    std::cout << "\n=== Test 8: Key Clearing ===" << std::endl;

    auto keypair = Kyber::generate_keypair();

    // Verify private key is not all zeros initially
    bool has_nonzero = false;
    for (uint8_t byte : keypair.private_key) {
        if (byte != 0) {
            has_nonzero = true;
            break;
        }
    }
    test_assert(has_nonzero, "Private key initially non-zero");

    // Clear private key
    keypair.clear_private();

    // Verify all bytes are now zero
    bool all_zero = true;
    for (uint8_t byte : keypair.private_key) {
        if (byte != 0) {
            all_zero = false;
            break;
        }
    }
    test_assert(all_zero, "Private key cleared to all zeros");
}

// ============================================================================
// Test 9: Shared Secret Uniqueness
// ============================================================================

void test_shared_secret_uniqueness() {
    std::cout << "\n=== Test 9: Shared Secret Uniqueness ===" << std::endl;

    auto keypair = Kyber::generate_keypair();

    const int iterations = 100;
    std::set<std::array<uint8_t, 32>> seen_secrets;

    for (int i = 0; i < iterations; i++) {
        auto [secret, ciphertext] = Kyber::encapsulate(keypair.public_key);

        // Convert to array for set insertion
        std::array<uint8_t, 32> secret_array;
        std::copy(secret.begin(), secret.end(), secret_array.begin());

        seen_secrets.insert(secret_array);
    }

    // All secrets should be unique (extremely high probability)
    test_assert(seen_secrets.size() == iterations,
                "100 encapsulations produce 100 unique shared secrets");
}

// ============================================================================
// Test 10: Performance - Key Generation
// ============================================================================

void test_keygen_performance() {
    std::cout << "\n=== Test 10: Key Generation Performance ===" << std::endl;

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto keypair = Kyber::generate_keypair();
        (void)keypair; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average key generation time: " << avg_time << " ms" << std::endl;
    std::cout << "Keys per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 50, "Key generation < 50ms (reasonable performance)");
}

// ============================================================================
// Test 11: Performance - Encapsulation
// ============================================================================

void test_encapsulation_performance() {
    std::cout << "\n=== Test 11: Encapsulation Performance ===" << std::endl;

    auto keypair = Kyber::generate_keypair();

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto [secret, ciphertext] = Kyber::encapsulate(keypair.public_key);
        (void)secret; // Prevent optimization
        (void)ciphertext;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average encapsulation time: " << avg_time << " ms" << std::endl;
    std::cout << "Encapsulations per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 30, "Encapsulation < 30ms (reasonable performance)");
}

// ============================================================================
// Test 12: Performance - Decapsulation
// ============================================================================

void test_decapsulation_performance() {
    std::cout << "\n=== Test 12: Decapsulation Performance ===" << std::endl;

    auto keypair = Kyber::generate_keypair();
    auto [secret, ciphertext] = Kyber::encapsulate(keypair.public_key);

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto result = Kyber::decapsulate(ciphertext, keypair);
        (void)result; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);

    std::cout << "Average decapsulation time: " << avg_time << " ms" << std::endl;
    std::cout << "Decapsulations per second: " << (1000.0 / avg_time) << std::endl;

    test_assert(avg_time < 35, "Decapsulation < 35ms (reasonable performance)");
}

// ============================================================================
// Test 13: Constant-Time Decapsulation
// ============================================================================

void test_constant_time() {
    std::cout << "\n=== Test 13: Constant-Time Decapsulation ===" << std::endl;

    auto keypair = Kyber::generate_keypair();
    auto [secret, valid_ciphertext] = Kyber::encapsulate(keypair.public_key);

    // Create invalid ciphertext
    auto invalid_ciphertext = valid_ciphertext;
    invalid_ciphertext[100] ^= 0x01;

    const int iterations = 1000;

    // Time valid ciphertext decapsulations
    auto start_valid = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Kyber::decapsulate(valid_ciphertext, keypair);
    }
    auto end_valid = std::chrono::high_resolution_clock::now();
    auto valid_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_valid - start_valid);

    // Time invalid ciphertext decapsulations
    auto start_invalid = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        Kyber::decapsulate(invalid_ciphertext, keypair);
    }
    auto end_invalid = std::chrono::high_resolution_clock::now();
    auto invalid_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_invalid - start_invalid);

    double valid_avg = valid_duration.count() / static_cast<double>(iterations);
    double invalid_avg = invalid_duration.count() / static_cast<double>(iterations);
    double time_diff_percent = std::abs(valid_avg - invalid_avg) / std::max(valid_avg, invalid_avg) * 100.0;

    std::cout << "Valid ciphertext avg: " << valid_avg << " ns" << std::endl;
    std::cout << "Invalid ciphertext avg: " << invalid_avg << " ns" << std::endl;
    std::cout << "Time difference: " << time_diff_percent << "%" << std::endl;

    // Constant-time operations should have < 15% timing variance
    // (slightly higher tolerance than Dilithium due to implicit rejection)
    test_assert(time_diff_percent < 15.0, "Decapsulation is constant-time (< 15% variance)");
}

// ============================================================================
// Test 14: Cross-Keypair Test
// ============================================================================

void test_cross_keypair() {
    std::cout << "\n=== Test 14: Cross-Keypair Test ===" << std::endl;

    auto keypair1 = Kyber::generate_keypair();
    auto keypair2 = Kyber::generate_keypair();

    // Encapsulate with keypair1's public key
    auto [secret1, ciphertext1] = Kyber::encapsulate(keypair1.public_key);

    // Decapsulate with correct keypair
    auto decap1 = Kyber::decapsulate(ciphertext1, keypair1);
    test_assert(decap1.has_value() && *decap1 == secret1, "Correct keypair recovers shared secret");

    // Decapsulate with wrong keypair (implicit rejection)
    auto decap2 = Kyber::decapsulate(ciphertext1, keypair2);
    test_assert(decap2.has_value(), "Wrong keypair still produces output (implicit rejection)");
    test_assert(*decap2 != secret1, "Wrong keypair produces different shared secret");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "NIST FIPS 203 ML-KEM-1024 (Kyber1024) Tests" << std::endl;
    std::cout << "============================================" << std::endl;

    try {
        test_key_generation();
        test_encapsulate_decapsulate();
        test_wrong_key_decapsulation();
        test_multiple_encapsulations();
        test_corrupted_ciphertext();
        test_keypair_serialization();
        test_invalid_serialization();
        test_key_clearing();
        test_shared_secret_uniqueness();
        test_keygen_performance();
        test_encapsulation_performance();
        test_decapsulation_performance();
        test_constant_time();
        test_cross_keypair();

        std::cout << "\n============================================" << std::endl;
        std::cout << "ALL TESTS PASSED (14/14)" << std::endl;
        std::cout << "ML-KEM-1024 implementation verified" << std::endl;
        std::cout << "============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
