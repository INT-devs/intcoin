// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// NIST test vectors for SHA3-256 (FIPS 202) and SHA-256 (FIPS 180-4)
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
std::string bytes_to_hex(const std::array<uint8_t, 32>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// ============================================================================
// SHA3-256 NIST FIPS 202 Test Vectors
// ============================================================================

void test_sha3_256_empty() {
    std::cout << "\n=== SHA3-256: Empty String ===" << std::endl;

    // NIST test vector: SHA3-256("")
    std::vector<uint8_t> input;
    std::string expected = "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a";

    auto result = SHA3_256::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA3-256 empty string matches NIST vector");
}

void test_sha3_256_abc() {
    std::cout << "\n=== SHA3-256: \"abc\" ===" << std::endl;

    // NIST test vector: SHA3-256("abc")
    std::string input_str = "abc";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532";

    auto result = SHA3_256::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA3-256(\"abc\") matches NIST vector");
}

void test_sha3_256_448bits() {
    std::cout << "\n=== SHA3-256: 448-bit Message ===" << std::endl;

    // NIST test vector: SHA3-256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
    std::string input_str = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376";

    auto result = SHA3_256::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA3-256 448-bit message matches NIST vector");
}

void test_sha3_256_896bits() {
    std::cout << "\n=== SHA3-256: 896-bit Message ===" << std::endl;

    // NIST test vector: SHA3-256("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn...")
    std::string input_str = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "916f6061fe879741ca6469b43971dfdb28b1a32dc36cb3254e812be27aad1d18";

    auto result = SHA3_256::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA3-256 896-bit message matches NIST vector");
}

void test_sha3_256_million_a() {
    std::cout << "\n=== SHA3-256: One Million 'a' ===" << std::endl;

    // NIST test vector: SHA3-256(1,000,000 x 'a')
    std::vector<uint8_t> input(1000000, 'a');
    std::string expected = "5c8875ae474a3634ba4fd55ec85bffd661f32aca75c6d699d0cdcb6c115891c1";

    auto result = SHA3_256::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA3-256 one million 'a' matches NIST vector");
}

// ============================================================================
// SHA-256 NIST FIPS 180-4 Test Vectors
// ============================================================================

void test_sha256_empty() {
    std::cout << "\n=== SHA-256: Empty String ===" << std::endl;

    // NIST test vector: SHA-256("")
    std::vector<uint8_t> input;
    std::string expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

    auto result = SHA256_PoW::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA-256 empty string matches NIST vector");
}

void test_sha256_abc() {
    std::cout << "\n=== SHA-256: \"abc\" ===" << std::endl;

    // NIST test vector: SHA-256("abc")
    std::string input_str = "abc";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

    auto result = SHA256_PoW::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA-256(\"abc\") matches NIST vector");
}

void test_sha256_448bits() {
    std::cout << "\n=== SHA-256: 448-bit Message ===" << std::endl;

    // NIST test vector: SHA-256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
    std::string input_str = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

    auto result = SHA256_PoW::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA-256 448-bit message matches NIST vector");
}

void test_sha256_896bits() {
    std::cout << "\n=== SHA-256: 896-bit Message ===" << std::endl;

    // NIST test vector
    std::string input_str = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());
    std::string expected = "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1";

    auto result = SHA256_PoW::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA-256 896-bit message matches NIST vector");
}

void test_sha256_million_a() {
    std::cout << "\n=== SHA-256: One Million 'a' ===" << std::endl;

    // NIST test vector: SHA-256(1,000,000 x 'a')
    std::vector<uint8_t> input(1000000, 'a');
    std::string expected = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";

    auto result = SHA256_PoW::hash(input.data(), input.size());
    std::string result_hex = bytes_to_hex(result);

    test_assert(result_hex == expected, "SHA-256 one million 'a' matches NIST vector");
}

// ============================================================================
// SHA-256 Double Hash Tests (Bitcoin-style)
// ============================================================================

void test_sha256_double_hash() {
    std::cout << "\n=== SHA-256: Double Hash ===" << std::endl;

    std::string input_str = "hello";
    std::vector<uint8_t> input(input_str.begin(), input_str.end());

    // First hash
    auto hash1 = SHA256_PoW::hash(input);
    // Second hash
    auto hash2 = SHA256_PoW::hash(hash1.data(), hash1.size());

    // Using double_hash function
    auto double_hash_result = SHA256_PoW::double_hash(input);

    test_assert(hash2 == double_hash_result, "Double hash matches manual double hashing");
}

// ============================================================================
// SHA3-256 Incremental Update Tests
// ============================================================================

void test_sha3_256_incremental() {
    std::cout << "\n=== SHA3-256: Incremental Update ===" << std::endl;

    // Single update
    std::string full_str = "The quick brown fox jumps over the lazy dog";
    std::vector<uint8_t> full_input(full_str.begin(), full_str.end());
    auto single_result = SHA3_256::hash(full_input.data(), full_input.size());

    // Incremental update
    SHA3_256 hasher;
    hasher.update(reinterpret_cast<const uint8_t*>("The quick brown fox "), 20);
    hasher.update(reinterpret_cast<const uint8_t*>("jumps over the lazy dog"), 23);
    auto incremental_result = hasher.finalize();

    test_assert(single_result == incremental_result, "Incremental SHA3-256 matches single update");
}

// ============================================================================
// Performance Tests
// ============================================================================

void test_sha3_performance() {
    std::cout << "\n=== SHA3-256: Performance ===" << std::endl;

    std::vector<uint8_t> data(1024 * 1024); // 1 MB
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto hash = SHA3_256::hash(data.data(), data.size());
        (void)hash; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);
    double throughput = (1024.0 * 1024.0) / (avg_time / 1000.0) / (1024.0 * 1024.0); // MB/s

    std::cout << "SHA3-256 average time (1MB): " << avg_time << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " MB/s" << std::endl;

    test_assert(avg_time < 50, "SHA3-256 performance reasonable (< 50ms for 1MB)");
}

void test_sha256_performance() {
    std::cout << "\n=== SHA-256: Performance ===" << std::endl;

    std::vector<uint8_t> data(1024 * 1024); // 1 MB
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    const int iterations = 100;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        auto hash = SHA256_PoW::hash(data.data(), data.size());
        (void)hash; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);
    double throughput = (1024.0 * 1024.0) / (avg_time / 1000.0) / (1024.0 * 1024.0); // MB/s

    std::cout << "SHA-256 average time (1MB): " << avg_time << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " MB/s" << std::endl;

    test_assert(avg_time < 30, "SHA-256 performance reasonable (< 30ms for 1MB)");
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_zero_bytes() {
    std::cout << "\n=== Edge Case: All Zero Bytes ===" << std::endl;

    std::vector<uint8_t> zeros(1000, 0x00);

    auto sha3_result = SHA3_256::hash(zeros.data(), zeros.size());
    auto sha256_result = SHA256_PoW::hash(zeros.data(), zeros.size());

    // Results should be deterministic (same input = same output)
    auto sha3_result2 = SHA3_256::hash(zeros.data(), zeros.size());
    auto sha256_result2 = SHA256_PoW::hash(zeros.data(), zeros.size());

    test_assert(sha3_result == sha3_result2, "SHA3-256 deterministic for zero bytes");
    test_assert(sha256_result == sha256_result2, "SHA-256 deterministic for zero bytes");
}

void test_all_ff_bytes() {
    std::cout << "\n=== Edge Case: All 0xFF Bytes ===" << std::endl;

    std::vector<uint8_t> ffs(1000, 0xFF);

    auto sha3_result = SHA3_256::hash(ffs.data(), ffs.size());
    auto sha256_result = SHA256_PoW::hash(ffs.data(), ffs.size());

    // Results should be deterministic
    auto sha3_result2 = SHA3_256::hash(ffs.data(), ffs.size());
    auto sha256_result2 = SHA256_PoW::hash(ffs.data(), ffs.size());

    test_assert(sha3_result == sha3_result2, "SHA3-256 deterministic for 0xFF bytes");
    test_assert(sha256_result == sha256_result2, "SHA-256 deterministic for 0xFF bytes");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "NIST Hash Function Verification Tests" << std::endl;
    std::cout << "SHA3-256 (FIPS 202) & SHA-256 (FIPS 180-4)" << std::endl;
    std::cout << "============================================" << std::endl;

    try {
        // SHA3-256 NIST FIPS 202 tests
        test_sha3_256_empty();
        test_sha3_256_abc();
        test_sha3_256_448bits();
        test_sha3_256_896bits();
        test_sha3_256_million_a();

        // SHA-256 NIST FIPS 180-4 tests
        test_sha256_empty();
        test_sha256_abc();
        test_sha256_448bits();
        test_sha256_896bits();
        test_sha256_million_a();

        // Additional tests
        test_sha256_double_hash();
        test_sha3_256_incremental();

        // Performance tests
        test_sha3_performance();
        test_sha256_performance();

        // Edge cases
        test_zero_bytes();
        test_all_ff_bytes();

        std::cout << "\n============================================" << std::endl;
        std::cout << "ALL TESTS PASSED (17/17)" << std::endl;
        std::cout << "Hash implementations verified against NIST" << std::endl;
        std::cout << "SHA3-256 (FIPS 202): ✅" << std::endl;
        std::cout << "SHA-256 (FIPS 180-4): ✅" << std::endl;
        std::cout << "============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
