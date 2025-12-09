/*
 * Bech32 Address Encoding Tests
 * Tests Bech32 encoding, decoding, and validation
 */

#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

void test_bech32_encode_decode() {
    std::cout << "\n=== Test 1: Bech32 Encode/Decode Round-Trip ===" << std::endl;

    // Create a test pubkey hash (all zeros)
    uint256 pubkey_hash{};
    for (size_t i = 0; i < 32; ++i) {
        pubkey_hash[i] = 0;
    }

    // Encode to Bech32 address
    auto encode_result = AddressEncoder::EncodeAddress(pubkey_hash);
    if (encode_result.IsError()) {
        std::cerr << "Encoding failed: " << encode_result.error << std::endl;
    }
    assert(encode_result.IsOk());
    std::string address = *encode_result.value;
    std::cout << "✓ Encoded address: " << address << std::endl;

    // Verify it starts with 'int1'
    assert(address.substr(0, 4) == "int1");
    std::cout << "✓ Address has correct prefix 'int1'" << std::endl;

    // Decode the address back
    auto decode_result = AddressEncoder::DecodeAddress(address);
    assert(decode_result.IsOk());
    uint256 decoded_hash = *decode_result.value;

    // Verify we got the same hash back
    assert(pubkey_hash == decoded_hash);
    (void)decoded_hash;  // Suppress unused warning
    std::cout << "✓ Round-trip encode/decode successful" << std::endl;
}

void test_bech32_different_hashes() {
    std::cout << "\n=== Test 2: Different Hashes Produce Different Addresses ===" << std::endl;

    // Create two different pubkey hashes
    uint256 hash1{};
    uint256 hash2{};
    for (size_t i = 0; i < 32; ++i) {
        hash1[i] = i;
        hash2[i] = 31 - i;
    }

    // Encode both
    auto addr1_result = AddressEncoder::EncodeAddress(hash1);
    auto addr2_result = AddressEncoder::EncodeAddress(hash2);
    assert(addr1_result.IsOk());
    assert(addr2_result.IsOk());

    std::string addr1 = *addr1_result.value;
    std::string addr2 = *addr2_result.value;

    // Addresses should be different
    assert(addr1 != addr2);
    std::cout << "✓ Different hashes produce different addresses" << std::endl;
    std::cout << "  Address 1: " << addr1 << std::endl;
    std::cout << "  Address 2: " << addr2 << std::endl;

    // Decode and verify
    auto decode1_result = AddressEncoder::DecodeAddress(addr1);
    auto decode2_result = AddressEncoder::DecodeAddress(addr2);
    assert(decode1_result.IsOk());
    assert(decode2_result.IsOk());

    assert(*decode1_result.value == hash1);
    assert(*decode2_result.value == hash2);
    std::cout << "✓ Both addresses decode correctly" << std::endl;
}

void test_bech32_validation() {
    std::cout << "\n=== Test 3: Address Validation ===" << std::endl;

    // Create valid address
    uint256 hash{};
    for (size_t i = 0; i < 32; ++i) {
        hash[i] = 0xFF;
    }

    auto encode_result = AddressEncoder::EncodeAddress(hash);
    assert(encode_result.IsOk());
    std::string valid_address = *encode_result.value;

    // Test valid address
    assert(AddressEncoder::ValidateAddress(valid_address) == true);
    std::cout << "✓ Valid address passes validation" << std::endl;

    // Test invalid addresses
    assert(AddressEncoder::ValidateAddress("") == false);
    std::cout << "✓ Empty string fails validation" << std::endl;

    assert(AddressEncoder::ValidateAddress("int1") == false);
    std::cout << "✓ Address with only prefix fails validation" << std::endl;

    assert(AddressEncoder::ValidateAddress("btc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh") == false);
    std::cout << "✓ Wrong HRP fails validation" << std::endl;

    // Test corrupted checksum (change last character)
    std::string corrupted = valid_address;
    corrupted[corrupted.size() - 1] = (corrupted[corrupted.size() - 1] == 'q') ? 'p' : 'q';
    assert(AddressEncoder::ValidateAddress(corrupted) == false);
    std::cout << "✓ Corrupted checksum fails validation" << std::endl;
}

void test_bech32_case_insensitivity() {
    std::cout << "\n=== Test 4: Case Insensitivity ===" << std::endl;

    // Create test address
    uint256 hash{};
    for (size_t i = 0; i < 32; ++i) {
        hash[i] = i * 8;
    }

    auto encode_result = AddressEncoder::EncodeAddress(hash);
    assert(encode_result.IsOk());
    std::string lowercase_addr = *encode_result.value;

    // Convert to uppercase
    std::string uppercase_addr = lowercase_addr;
    for (char& c : uppercase_addr) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }

    // Both should decode to the same hash
    auto decode_lower = AddressEncoder::DecodeAddress(lowercase_addr);
    auto decode_upper = AddressEncoder::DecodeAddress(uppercase_addr);
    assert(decode_lower.IsOk());
    assert(decode_upper.IsOk());
    assert(*decode_lower.value == *decode_upper.value);
    std::cout << "✓ Lowercase and uppercase addresses decode to same hash" << std::endl;

    // Mixed case should fail
    std::string mixed_case = lowercase_addr;
    if (mixed_case.size() > 10) {
        mixed_case[10] = 'Q'; // Make one character uppercase
    }
    auto decode_mixed = AddressEncoder::DecodeAddress(mixed_case);
    assert(decode_mixed.IsError());
    std::cout << "✓ Mixed case address correctly rejected" << std::endl;
}

void test_bech32_checksum_detection() {
    std::cout << "\n=== Test 5: Checksum Error Detection ===" << std::endl;

    // Create valid address
    uint256 hash{};
    for (size_t i = 0; i < 32; ++i) {
        hash[i] = (i * 7) % 256;
    }

    auto encode_result = AddressEncoder::EncodeAddress(hash);
    assert(encode_result.IsOk());
    std::string valid_address = *encode_result.value;

    std::cout << "Valid address: " << valid_address << std::endl;

    // Test single character errors at different positions
    int error_count = 0;
    for (size_t i = 5; i < valid_address.size(); ++i) { // Skip HRP
        std::string corrupted = valid_address;
        // Change character to a different one
        char original = corrupted[i];
        corrupted[i] = (original == 'q') ? 'p' : 'q';

        if (!AddressEncoder::ValidateAddress(corrupted)) {
            error_count++;
        }
    }

    std::cout << "✓ Detected " << error_count << " single-character errors" << std::endl;
    assert(error_count > 0);
}

void test_pubkey_to_address() {
    std::cout << "\n=== Test 6: Public Key to Address Conversion ===" << std::endl;

    // Generate a Dilithium keypair
    auto keypair_result = DilithiumCrypto::GenerateKeyPair();
    assert(keypair_result.IsOk());
    PublicKey pubkey = keypair_result.value->public_key;

    // Convert pubkey to hash
    uint256 pubkey_hash = PublicKeyToHash(pubkey);
    std::cout << "✓ Public key hashed successfully" << std::endl;
    std::cout << "  Hash: " << ToHex(pubkey_hash) << std::endl;

    // Convert hash to address
    std::string address = PublicKeyHashToAddress(pubkey_hash);
    assert(!address.empty());
    std::cout << "✓ Generated address: " << address << std::endl;

    // Verify address is valid
    assert(AddressEncoder::ValidateAddress(address));
    std::cout << "✓ Address validates successfully" << std::endl;

    // Decode address back to hash
    auto decode_result = AddressEncoder::DecodeAddress(address);
    assert(decode_result.IsOk());
    assert(*decode_result.value == pubkey_hash);
    std::cout << "✓ Address decodes back to original hash" << std::endl;

    // Test convenience function
    std::string address2 = PublicKeyToAddress(pubkey);
    assert(address == address2);
    std::cout << "✓ PublicKeyToAddress convenience function works" << std::endl;
}

void test_bech32_edge_cases() {
    std::cout << "\n=== Test 7: Edge Cases ===" << std::endl;

    // Test all zeros
    uint256 zeros{};
    auto addr_zeros = AddressEncoder::EncodeAddress(zeros);
    assert(addr_zeros.IsOk());
    std::cout << "✓ All-zeros hash encodes successfully" << std::endl;

    // Test all 0xFF
    uint256 ones{};
    for (size_t i = 0; i < 32; ++i) {
        ones[i] = 0xFF;
    }
    auto addr_ones = AddressEncoder::EncodeAddress(ones);
    assert(addr_ones.IsOk());
    std::cout << "✓ All-ones hash encodes successfully" << std::endl;

    // Verify they're different
    assert(*addr_zeros.value != *addr_ones.value);
    std::cout << "✓ Different edge case hashes produce different addresses" << std::endl;

    // Verify both decode correctly
    auto decode_zeros = AddressEncoder::DecodeAddress(*addr_zeros.value);
    auto decode_ones = AddressEncoder::DecodeAddress(*addr_ones.value);
    assert(decode_zeros.IsOk());
    assert(decode_ones.IsOk());
    assert(*decode_zeros.value == zeros);
    assert(*decode_ones.value == ones);
    std::cout << "✓ Edge case addresses decode correctly" << std::endl;
}

void test_bech32_invalid_inputs() {
    std::cout << "\n=== Test 8: Invalid Input Handling ===" << std::endl;

    // Test various invalid addresses
    std::vector<std::string> invalid_addresses = {
        "",                                              // Empty
        "int1",                                          // Too short
        "1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",    // Missing HRP
        "int2qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh", // Wrong HRP
        "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlhO", // Invalid character (O)
        "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlhI", // Invalid character (I)
        "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlhb", // Invalid character (b)
        "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh1", // Invalid character (1)
    };

    for (const auto& addr : invalid_addresses) {
        assert(!AddressEncoder::ValidateAddress(addr));
        (void)addr;  // Suppress unused warning in release builds
    }
    std::cout << "✓ All invalid addresses correctly rejected" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Bech32 Address Encoding Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_bech32_encode_decode();
        test_bech32_different_hashes();
        test_bech32_validation();
        test_bech32_case_insensitivity();
        test_bech32_checksum_detection();
        test_pubkey_to_address();
        test_bech32_edge_cases();
        test_bech32_invalid_inputs();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✓ All Bech32 tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
