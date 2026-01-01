/**
 * @file test_bloom.cpp
 * @brief Bloom filter test suite for INTcoin
 * @author INTcoin Core Developers
 * @date 2026-01-01
 * @version 1.2.0-beta
 */

#include "intcoin/bloom.h"
#include "intcoin/transaction.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace intcoin;

// Test 1: Bloom filter creation and basic operations
void TestBloomFilterCreation() {
    std::cout << "Test 1: Bloom Filter Creation..." << std::endl;

    // Create bloom filter with 10 elements, 0.01 false positive rate
    BloomFilter filter(10, 0.01, 12345, BLOOM_UPDATE_ALL);

    assert(filter.IsEmpty());
    assert(!filter.IsFull());
    assert(filter.IsValid());
    assert(filter.GetSize() > 0);
    assert(filter.GetNumHashFuncs() > 0);
    assert(filter.GetTweak() == 12345);
    assert(filter.GetFlags() == BLOOM_UPDATE_ALL);

    std::cout << "✓ Bloom filter creation successful" << std::endl;
    std::cout << "  - Filter size: " << filter.GetSize() << " bytes" << std::endl;
    std::cout << "  - Hash functions: " << filter.GetNumHashFuncs() << std::endl;
}

// Test 2: Adding and checking elements
void TestBloomFilterAdd() {
    std::cout << "\nTest 2: Adding Elements..." << std::endl;

    BloomFilter filter(100, 0.01);

    std::vector<uint8_t> data1 = {0x01, 0x02, 0x03, 0x04};
    std::vector<uint8_t> data2 = {0x05, 0x06, 0x07, 0x08};
    std::vector<uint8_t> data3 = {0x09, 0x0a, 0x0b, 0x0c};

    // Initially empty
    assert(!filter.Contains(data1));
    assert(!filter.Contains(data2));

    // Add data1
    filter.Add(data1);
    assert(filter.Contains(data1));
    assert(!filter.Contains(data2));
    assert(!filter.IsEmpty());

    // Add data2
    filter.Add(data2);
    assert(filter.Contains(data1));
    assert(filter.Contains(data2));
    assert(!filter.Contains(data3));

    std::cout << "✓ Adding and checking elements working correctly" << std::endl;
}

// Test 3: OutPoint filtering
void TestBloomFilterOutPoint() {
    std::cout << "\nTest 3: OutPoint Filtering..." << std::endl;

    BloomFilter filter(50, 0.001);

    // Create test outpoints
    OutPoint outpoint1;
    for (size_t i = 0; i < 32; i++) {
        outpoint1.tx_hash[i] = static_cast<uint8_t>(i);
    }
    outpoint1.index = 0;

    OutPoint outpoint2;
    for (size_t i = 0; i < 32; i++) {
        outpoint2.tx_hash[i] = static_cast<uint8_t>(i + 32);
    }
    outpoint2.index = 1;

    // Test filtering
    assert(!filter.ContainsOutPoint(outpoint1));
    filter.AddOutPoint(outpoint1);
    assert(filter.ContainsOutPoint(outpoint1));
    assert(!filter.ContainsOutPoint(outpoint2));

    std::cout << "✓ OutPoint filtering working correctly" << std::endl;
}

// Test 4: Transaction matching
void TestBloomFilterTransaction() {
    std::cout << "\nTest 4: Transaction Matching..." << std::endl;

    BloomFilter filter(100, 0.01);

    // Create test transaction
    Transaction tx;
    tx.version = 1;

    // Add input
    TxIn input;
    for (size_t i = 0; i < 32; i++) {
        input.prev_tx_hash[i] = static_cast<uint8_t>(i);
    }
    input.prev_tx_index = 0;
    input.script_sig.bytes = {0x48, 0x30, 0x45};  // Dummy signature
    tx.inputs.push_back(input);

    // Add output
    TxOut output;
    output.value = 50000000;
    output.script_pubkey.bytes = {0x76, 0xa9, 0x14};  // Dummy P2PKH
    tx.outputs.push_back(output);

    // Transaction shouldn't match empty filter
    assert(!filter.MatchesTransaction(tx));

    // Add outpoint to filter
    OutPoint outpoint;
    outpoint.tx_hash = input.prev_tx_hash;
    outpoint.index = input.prev_tx_index;
    filter.AddOutPoint(outpoint);

    // Now transaction should match
    assert(filter.MatchesTransaction(tx));

    std::cout << "✓ Transaction matching working correctly" << std::endl;
}

// Test 5: Serialization and deserialization
void TestBloomFilterSerialization() {
    std::cout << "\nTest 5: Serialization..." << std::endl;

    // Create and populate filter
    BloomFilter filter1(50, 0.01, 98765, BLOOM_UPDATE_P2PUBKEY_ONLY);
    std::vector<uint8_t> data = {0xde, 0xad, 0xbe, 0xef};
    filter1.Add(data);

    // Serialize
    std::vector<uint8_t> serialized = filter1.Serialize();
    assert(!serialized.empty());

    // Deserialize
    auto result = BloomFilter::Deserialize(serialized);
    assert(result.IsOk());

    BloomFilter filter2 = result.GetValue();

    // Verify properties match
    assert(filter2.GetSize() == filter1.GetSize());
    assert(filter2.GetNumHashFuncs() == filter1.GetNumHashFuncs());
    assert(filter2.GetTweak() == filter1.GetTweak());
    assert(filter2.GetFlags() == filter1.GetFlags());
    assert(filter2.Contains(data));

    std::cout << "✓ Serialization and deserialization working correctly" << std::endl;
}

// Test 6: False positive rate
void TestBloomFilterFalsePositiveRate() {
    std::cout << "\nTest 6: False Positive Rate..." << std::endl;

    // Create filter with known parameters
    const uint32_t num_elements = 100;
    const double target_fp_rate = 0.01;  // 1%
    BloomFilter filter(num_elements, target_fp_rate);

    // Add elements
    for (uint32_t i = 0; i < num_elements; i++) {
        std::vector<uint8_t> data = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF)
        };
        filter.Add(data);
    }

    // Test for false positives with non-added elements
    uint32_t false_positives = 0;
    const uint32_t test_count = 10000;

    for (uint32_t i = num_elements; i < num_elements + test_count; i++) {
        std::vector<uint8_t> data = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF)
        };
        if (filter.Contains(data)) {
            false_positives++;
        }
    }

    double measured_fp_rate = static_cast<double>(false_positives) / test_count;

    std::cout << "✓ False positive rate test complete" << std::endl;
    std::cout << "  - Target FP rate: " << (target_fp_rate * 100) << "%" << std::endl;
    std::cout << "  - Measured FP rate: " << (measured_fp_rate * 100) << "%" << std::endl;
    std::cout << "  - False positives: " << false_positives << " / " << test_count << std::endl;

    // Allow some tolerance (measured rate should be within 5x of target)
    assert(measured_fp_rate < target_fp_rate * 5.0);
}

// Test 7: Clear filter
void TestBloomFilterClear() {
    std::cout << "\nTest 7: Clear Filter..." << std::endl;

    BloomFilter filter(50, 0.01);

    // Add some data
    std::vector<uint8_t> data1 = {0x11, 0x22, 0x33};
    std::vector<uint8_t> data2 = {0x44, 0x55, 0x66};

    filter.Add(data1);
    filter.Add(data2);

    assert(filter.Contains(data1));
    assert(filter.Contains(data2));
    assert(!filter.IsEmpty());

    // Clear filter
    filter.Clear();

    assert(!filter.Contains(data1));
    assert(!filter.Contains(data2));
    assert(filter.IsEmpty());

    std::cout << "✓ Clear filter working correctly" << std::endl;
}

// Test 8: Edge cases
void TestBloomFilterEdgeCases() {
    std::cout << "\nTest 8: Edge Cases..." << std::endl;

    // Empty data
    BloomFilter filter(10, 0.01);
    std::vector<uint8_t> empty_data;
    filter.Add(empty_data);  // Should not crash
    assert(!filter.Contains(empty_data));

    // Very small filter
    BloomFilter small_filter(1, 0.5);
    assert(small_filter.IsValid());

    // Maximum size filter
    BloomFilter large_filter(10000, 0.0001);
    assert(large_filter.IsValid());
    assert(large_filter.GetSize() <= BloomFilter::MAX_BLOOM_FILTER_SIZE);

    std::cout << "✓ Edge cases handled correctly" << std::endl;
}

// Main test runner
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "INTcoin Bloom Filter Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        TestBloomFilterCreation();
        TestBloomFilterAdd();
        TestBloomFilterOutPoint();
        TestBloomFilterTransaction();
        TestBloomFilterSerialization();
        TestBloomFilterFalsePositiveRate();
        TestBloomFilterClear();
        TestBloomFilterEdgeCases();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All bloom filter tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
