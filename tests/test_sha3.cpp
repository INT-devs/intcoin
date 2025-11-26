/*
 * Simple SHA3-256 Test
 * Tests SHA3-256 implementation with known test vectors
 */

#include "include/intcoin/crypto.h"
#include "include/intcoin/util.h"
#include <iostream>
#include <string>
#include <vector>

using namespace intcoin;

void test_sha3(const std::string& input, const std::string& expected_hex) {
    std::vector<uint8_t> data(input.begin(), input.end());
    uint256 hash = SHA3::Hash(data);
    std::string result_hex = ToHex(hash);

    bool passed = (result_hex == expected_hex);

    std::cout << "Input: \"" << input << "\"" << std::endl;
    std::cout << "Expected: " << expected_hex << std::endl;
    std::cout << "Got:      " << result_hex << std::endl;
    std::cout << "Status:   " << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "=== SHA3-256 Test Vectors ===" << std::endl << std::endl;

    // Test vector 1: Empty string
    // echo -n "" | openssl dgst -sha3-256
    test_sha3("",
              "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");

    // Test vector 2: "abc"
    // echo -n "abc" | openssl dgst -sha3-256
    test_sha3("abc",
              "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532");

    // Test vector 3: Longer string
    // echo -n "The quick brown fox jumps over the lazy dog" | openssl dgst -sha3-256
    test_sha3("The quick brown fox jumps over the lazy dog",
              "69070dda01975c8c120c3aada1b282394e7f032fa9cf32f4cb2259a0897dfc04");

    // Test vector 4: "INTcoin"
    // echo -n "INTcoin" | openssl dgst -sha3-256
    test_sha3("INTcoin",
              "b04816fa4706015b6774bfd42ecc6c2711cf680f3b0b4b772dc3610b308a1283");

    // Test DoubleHash
    std::cout << "=== Testing Double SHA3-256 ===" << std::endl;
    std::vector<uint8_t> data{'a', 'b', 'c'};
    uint256 double_hash = SHA3::DoubleHash(data);
    std::cout << "DoubleHash('abc'): " << ToHex(double_hash) << std::endl;
    std::cout << std::endl;

    // Test FromHex / ToHex round-trip
    std::cout << "=== Testing Hex Conversion Round-Trip ===" << std::endl;
    std::string test_hex = "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a";
    auto parsed = FromHex(test_hex);
    if (parsed.has_value()) {
        std::string round_trip = ToHex(*parsed);
        bool round_trip_ok = (round_trip == test_hex);
        std::cout << "Original: " << test_hex << std::endl;
        std::cout << "Round-trip: " << round_trip << std::endl;
        std::cout << "Status: " << (round_trip_ok ? "✅ PASS" : "❌ FAIL") << std::endl;
    } else {
        std::cout << "❌ Failed to parse hex string" << std::endl;
    }

    std::cout << std::endl << "=== All Tests Complete ===" << std::endl;

    return 0;
}
