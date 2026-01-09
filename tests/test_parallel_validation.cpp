// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <cstdint>
#include <cstddef>
#include <vector>

// Define mock Block and CBlockIndex BEFORE including headers
namespace intcoin {
    // Forward declare uint256 from types.h
    using uint256 = std::array<uint8_t, 32>;
    class Block {
    public:
        Block() = default;
        Block(const Block&) = default;
        Block(Block&&) = default;
        Block& operator=(const Block&) = default;
        Block& operator=(Block&&) = default;
        ~Block() = default;

        // Mock methods required by parallel_validation
        uint256 GetHash() const { return uint256{}; }
        uint256 CalculateMerkleRoot() const { return uint256{}; }
        size_t GetSerializedSize() const { return 1000; }

        struct Header {
            uint32_t version{1};
            uint64_t timestamp{0};
            uint256 randomx_hash{};
            uint32_t bits{0x1e0ffff0};
            uint256 merkle_root{};
        } header;

        struct Transaction {
            bool IsCoinbase() const { return true; }
            size_t GetSerializedSize() const { return 250; }
            std::vector<int> inputs;
            std::vector<int> outputs{1}; // At least one output
        };

        std::vector<Transaction> transactions{{}}; // Start with one coinbase tx
    };

    class CBlockIndex {
    public:
        CBlockIndex() = default;
        CBlockIndex(const CBlockIndex&) = default;
        CBlockIndex(CBlockIndex&&) = default;
        CBlockIndex& operator=(const CBlockIndex&) = default;
        CBlockIndex& operator=(CBlockIndex&&) = default;
        ~CBlockIndex() = default;
    };
}

#include <intcoin/ibd/parallel_validation.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace intcoin::ibd;
using intcoin::Block;
using intcoin::CBlockIndex;

// Test helpers
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASS" << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAIL" << std::endl; \
            failed++; \
        } \
        total++; \
    } while(0)

// Test: Thread pool initialization
bool test_threadpool_init() {
    ThreadPool pool(4);
    TEST_ASSERT(pool.GetThreadCount() == 4, "Thread count should be 4");
    return true;
}

// Test: Processor initialization
bool test_processor_init() {
    ParallelBlockProcessor processor;
    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_validated == 0, "Initial blocks validated should be 0");
    return true;
}

// Test: Single block submission
bool test_single_block_submission() {
    ParallelBlockProcessor processor;
    Block block;
    CBlockIndex index;

    auto future = processor.SubmitBlock(block, &index);
    TEST_ASSERT(future.valid(), "Future should be valid");

    auto result = future.get();
    TEST_ASSERT(result.valid, "Block should be valid");

    return true;
}

// Test: Multiple blocks submission
bool test_multiple_blocks_submission() {
    ParallelBlockProcessor processor;
    std::vector<ValidationFuture> futures;

    for (int i = 0; i < 10; i++) {
        Block block;
        CBlockIndex index;
        futures.push_back(processor.SubmitBlock(block, &index));
    }

    for (auto& future : futures) {
        auto result = future.get();
        TEST_ASSERT(result.valid, "All blocks should be valid");
    }

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_validated == 10, "Should have validated 10 blocks");

    return true;
}

// Test: Validation statistics
bool test_validation_statistics() {
    ParallelBlockProcessor processor;

    for (int i = 0; i < 10; i++) {
        Block block;
        CBlockIndex index;
        auto future = processor.SubmitBlock(block, &index);
        future.get();
    }

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_validated == 10, "Should have 10 validated blocks");
    TEST_ASSERT(stats.total_validation_time_ms >= 0, "Total time should be non-negative");

    return true;
}

// Test: Concurrent block processing
bool test_concurrent_processing() {
    ParallelBlockProcessor processor;
    std::vector<ValidationFuture> futures;

    // Submit 100 blocks
    for (int i = 0; i < 100; i++) {
        Block block;
        CBlockIndex index;
        futures.push_back(processor.SubmitBlock(block, &index));
    }

    // Wait for all to complete
    int valid_count = 0;
    for (auto& future : futures) {
        auto result = future.get();
        if (result.valid) valid_count++;
    }

    TEST_ASSERT(valid_count == 100, "All 100 blocks should be valid");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;
    int total = 0;

    std::cout << "=== Parallel Validation Tests ===" << std::endl;
    std::cout << std::endl;

    RUN_TEST(test_threadpool_init);
    RUN_TEST(test_processor_init);
    RUN_TEST(test_single_block_submission);
    RUN_TEST(test_multiple_blocks_submission);
    RUN_TEST(test_validation_statistics);
    RUN_TEST(test_concurrent_processing);

    std::cout << std::endl;
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Total: " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << std::endl;

    return (failed == 0) ? 0 : 1;
}
