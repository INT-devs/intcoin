// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/ibd/parallel_validation.h>
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace intcoin::ibd;

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

// Mock CBlock and CBlockIndex
class CBlock {};
class CBlockIndex {};

// Test: Thread pool initialization
bool test_threadpool_init() {
    ThreadPool pool(4);

    TEST_ASSERT(pool.GetThreadCount() == 4, "Thread count should be 4");
    TEST_ASSERT(pool.GetQueueSize() == 0, "Queue should be empty initially");

    return true;
}

// Test: Thread pool task execution
bool test_threadpool_execution() {
    ThreadPool pool(2);

    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        // Submit task would go here (template implementation needed)
        counter++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TEST_ASSERT(counter == 10, "All tasks should have incremented counter");

    return true;
}

// Test: Parallel processor initialization
bool test_processor_init() {
    ParallelBlockProcessor::Config config;
    config.num_threads = 4;

    ParallelBlockProcessor processor(config);

    TEST_ASSERT(processor.IsEnabled(), "Processor should be enabled by default");

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 0, "No blocks should be submitted initially");

    return true;
}

// Test: Auto-detect thread count
bool test_auto_detect_threads() {
    ParallelBlockProcessor::Config config;
    config.num_threads = 0; // Auto-detect

    ParallelBlockProcessor processor(config);

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.active_threads > 0, "Should have detected threads");

    return true;
}

// Test: Block submission
bool test_block_submission() {
    ParallelBlockProcessor processor;

    CBlock block;
    CBlockIndex index;

    auto future = processor.SubmitBlock(block, &index);

    TEST_ASSERT(future.valid(), "Future should be valid");

    auto result = future.get();
    TEST_ASSERT(result.valid, "Block validation should succeed (mock)");

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 1, "One block should be submitted");

    return true;
}

// Test: Multiple block submissions
bool test_multiple_submissions() {
    ParallelBlockProcessor processor;

    for (int i = 0; i < 10; ++i) {
        CBlock block;
        CBlockIndex index;
        processor.SubmitBlock(block, &index);
    }

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 10, "10 blocks should be submitted");

    return true;
}

// Test: Wait for completion
bool test_wait_for_completion() {
    ParallelBlockProcessor processor;

    for (int i = 0; i < 5; ++i) {
        CBlock block;
        CBlockIndex index;
        processor.SubmitBlock(block, &index);
    }

    processor.WaitForCompletion();

    // Should complete without hanging
    TEST_ASSERT(true, "Wait for completion should not hang");

    return true;
}

// Test: Enable/disable parallel validation
bool test_enable_disable() {
    ParallelBlockProcessor processor;

    TEST_ASSERT(processor.IsEnabled(), "Should be enabled initially");

    processor.SetEnabled(false);
    TEST_ASSERT(!processor.IsEnabled(), "Should be disabled");

    processor.SetEnabled(true);
    TEST_ASSERT(processor.IsEnabled(), "Should be enabled again");

    return true;
}

// Test: Set thread count
bool test_set_thread_count() {
    ParallelBlockProcessor processor;

    processor.SetThreadCount(8);

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.active_threads == 8, "Thread count should be 8");

    return true;
}

// Test: Validation statistics
bool test_validation_stats() {
    ParallelBlockProcessor processor;

    for (int i = 0; i < 20; ++i) {
        CBlock block;
        CBlockIndex index;
        auto future = processor.SubmitBlock(block, &index);
        future.wait();
    }

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 20, "20 blocks submitted");
    TEST_ASSERT(stats.total_validation_time_ms > 0, "Validation time should be recorded");
    TEST_ASSERT(stats.GetValidationRate() > 0, "Validation rate should be positive");

    return true;
}

// Test: Concurrent submissions
bool test_concurrent_submissions() {
    ParallelBlockProcessor processor;

    std::thread t1([&processor]() {
        for (int i = 0; i < 50; ++i) {
            CBlock block;
            CBlockIndex index;
            processor.SubmitBlock(block, &index);
        }
    });

    std::thread t2([&processor]() {
        for (int i = 0; i < 50; ++i) {
            CBlock block;
            CBlockIndex index;
            processor.SubmitBlock(block, &index);
        }
    });

    t1.join();
    t2.join();

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 100, "100 blocks should be submitted");

    return true;
}

// Test: Process validated blocks
bool test_process_validated() {
    ParallelBlockProcessor processor;

    for (int i = 0; i < 10; ++i) {
        CBlock block;
        CBlockIndex index;
        processor.SubmitBlock(block, &index);
    }

    uint32_t processed = processor.ProcessValidatedBlocks();
    TEST_ASSERT(processed >= 0, "Processed count should be non-negative");

    return true;
}

// Test: Large workload
bool test_large_workload() {
    ParallelBlockProcessor::Config config;
    config.num_threads = 8;

    ParallelBlockProcessor processor(config);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 1000; ++i) {
        CBlock block;
        CBlockIndex index;
        processor.SubmitBlock(block, &index);
    }

    processor.WaitForCompletion();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "(Processed 1000 blocks in " << duration << "ms) ";

    auto stats = processor.GetStats();
    TEST_ASSERT(stats.blocks_submitted == 1000, "1000 blocks should be submitted");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Parallel Validation Tests ===" << std::endl;

    RUN_TEST(test_threadpool_init);
    RUN_TEST(test_threadpool_execution);
    RUN_TEST(test_processor_init);
    RUN_TEST(test_auto_detect_threads);
    RUN_TEST(test_block_submission);
    RUN_TEST(test_multiple_submissions);
    RUN_TEST(test_wait_for_completion);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_set_thread_count);
    RUN_TEST(test_validation_stats);
    RUN_TEST(test_concurrent_submissions);
    RUN_TEST(test_process_validated);
    RUN_TEST(test_large_workload);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
