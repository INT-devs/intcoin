// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

// Define mock CBlock and CBlockIndex BEFORE including headers
namespace intcoin {
    class CBlock {
    public:
        CBlock() = default;
        CBlock(const CBlock&) = default;
        CBlock(CBlock&&) = default;
        CBlock& operator=(const CBlock&) = default;
        CBlock& operator=(CBlock&&) = default;
        ~CBlock() = default;
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
using intcoin::CBlock;
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
    CBlock block;
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
        CBlock block;
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
        CBlock block;
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
        CBlock block;
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
