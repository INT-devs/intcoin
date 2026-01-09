// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/ibd/parallel_validation.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

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

// Test: Thread pool task submission
bool test_threadpool_submit_task() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    // Submit 10 tasks that increment a counter
    for (int i = 0; i < 10; i++) {
        pool.SubmitTask([&counter]() {
            counter++;
        });
    }

    // Give tasks time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TEST_ASSERT(counter.load() == 10, "All 10 tasks should have completed");
    return true;
}

// Test: Thread pool queue size
bool test_threadpool_queue_size() {
    ThreadPool pool(1);  // Single thread to ensure queue builds up

    // Initial queue should be empty
    TEST_ASSERT(pool.GetQueueSize() == 0, "Initial queue should be empty");

    return true;
}

// Test: Processor enable/disable
bool test_processor_enable_disable() {
    ParallelBlockProcessor processor;

    // Should be enabled by default
    TEST_ASSERT(processor.IsEnabled(), "Processor should be enabled by default");

    // Disable
    processor.SetEnabled(false);
    TEST_ASSERT(!processor.IsEnabled(), "Processor should be disabled");

    // Re-enable
    processor.SetEnabled(true);
    TEST_ASSERT(processor.IsEnabled(), "Processor should be re-enabled");

    return true;
}

// Test: Processor thread count
bool test_processor_thread_count() {
    ParallelBlockProcessor processor;

    // Set custom thread count
    processor.SetThreadCount(8);
    auto stats = processor.GetStats();
    TEST_ASSERT(stats.active_threads == 8, "Should have 8 active threads");

    // Set thread count to auto (0)
    processor.SetThreadCount(0);
    stats = processor.GetStats();
    TEST_ASSERT(stats.active_threads > 0, "Auto thread count should be > 0");

    return true;
}

// Test: Concurrent thread pool operations
bool test_concurrent_threadpool() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    // Submit 100 tasks from multiple threads
    std::vector<std::thread> submitters;
    for (int t = 0; t < 4; t++) {
        submitters.emplace_back([&pool, &counter]() {
            for (int i = 0; i < 25; i++) {
                pool.SubmitTask([&counter]() {
                    counter++;
                });
            }
        });
    }

    // Wait for all submitters
    for (auto& t : submitters) {
        t.join();
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    TEST_ASSERT(counter.load() == 100, "All 100 concurrent tasks should complete");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;
    int total = 0;

    std::cout << "=== Parallel Validation Tests ===" << std::endl;
    std::cout << std::endl;

    // Core thread pool tests
    RUN_TEST(test_threadpool_init);
    RUN_TEST(test_threadpool_submit_task);
    RUN_TEST(test_threadpool_queue_size);
    RUN_TEST(test_concurrent_threadpool);

    // Processor configuration tests
    RUN_TEST(test_processor_init);
    RUN_TEST(test_processor_enable_disable);
    RUN_TEST(test_processor_thread_count);

    // Note: Block submission tests require integration testing with real Block objects
    // and are covered by test_ibd_integration.cpp

    std::cout << std::endl;
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Total: " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << std::endl;

    return (failed == 0) ? 0 : 1;
}
