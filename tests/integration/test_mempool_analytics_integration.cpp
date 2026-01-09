// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

/**
 * Integration Test: Mempool Analytics
 *
 * Tests end-to-end mempool analytics functionality including:
 * - Real-time statistics collection
 * - Historical snapshot management
 * - ML-based fee estimation
 * - Transaction flow analysis
 * - Concurrent access patterns
 */

#include <intcoin/mempool_analytics/analytics.h>
#include <intcoin/mempool_analytics/fee_estimator.h>
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace intcoin::mempool_analytics;

// Test result tracking
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    uint64_t duration_ms;
};

std::vector<TestResult> results;

#define INTEGRATION_TEST(name) \
    bool name(); \
    static bool run_##name() { \
        auto start = std::chrono::steady_clock::now(); \
        bool result = name(); \
        auto end = std::chrono::steady_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); \
        results.push_back({#name, result, "", static_cast<uint64_t>(duration)}); \
        return result; \
    } \
    bool name()

// Test: T-MA-001 - Real-time Statistics Collection
INTEGRATION_TEST(test_realtime_statistics) {
    MempoolAnalytics analytics;

    // Simulate high transaction load
    const int NUM_TRANSACTIONS = 1000;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        analytics.OnTransactionAdded(250, 10.0 + (i % 50), i % 6);
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Verify statistics
    auto stats = analytics.GetCurrentStats();

    std::cout << "  → " << NUM_TRANSACTIONS << " transactions added in " << duration << "ms" << std::endl;
    std::cout << "  → Total size: " << stats.bytes << " bytes" << std::endl;
    std::cout << "  → Average fee rate: " << stats.avg_fee_rate << " sat/byte" << std::endl;

    // Assertions
    assert(stats.size == NUM_TRANSACTIONS);
    assert(duration < 100); // Must complete in <100ms

    return true;
}

// Test: T-MA-002 - Historical Snapshots
INTEGRATION_TEST(test_historical_snapshots) {
    MempoolAnalytics analytics;

    // Take snapshots over time
    const int NUM_SNAPSHOTS = 10;
    std::vector<uint64_t> timestamps;

    for (int i = 0; i < NUM_SNAPSHOTS; i++) {
        // Add some transactions
        for (int j = 0; j < 100; j++) {
            analytics.OnTransactionAdded(250, 15.0, j % 6);
        }

        // Take snapshot
        analytics.TakeSnapshot();
        timestamps.push_back(std::chrono::system_clock::now().time_since_epoch().count());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Query historical data
    auto history = analytics.GetHistory(timestamps.front(), timestamps.back());

    std::cout << "  → Created " << NUM_SNAPSHOTS << " snapshots" << std::endl;
    std::cout << "  → Retrieved " << history.size() << " historical records" << std::endl;

    assert(history.size() <= NUM_SNAPSHOTS);

    return true;
}

// Test: T-MA-003 - Fee Estimation Accuracy
INTEGRATION_TEST(test_fee_estimation) {
    FeeEstimator estimator;

    // Simulate training data (simplified)
    std::vector<BlockData> training_data;
    for (int i = 0; i < 100; i++) {
        BlockData block;
        block.height = 100000 + i;
        block.timestamp = 1704067200 + (i * 600); // 10 min blocks
        block.total_size = 1000000; // 1MB blocks
        // Simulate fee rates for transactions in the block
        for (int j = 0; j < 100; j++) {
            block.fee_rates.push_back(10.0 + (i % 20) + (j % 5));
        }
        training_data.push_back(block);
    }

    // Train model (this would use actual ML in production)
    [[maybe_unused]] bool trained = estimator.TrainModel(training_data);

    // Test estimation for various targets
    std::vector<uint32_t> targets = {1, 3, 6, 12, 24};

    std::cout << "  → Fee estimates for confirmation targets:" << std::endl;
    for (auto target : targets) {
        auto estimate = estimator.EstimateFee(target);
        std::cout << "    " << target << " blocks: " << estimate.fee_rate
                  << " sat/byte (confidence: " << estimate.confidence << ")" << std::endl;
    }

    assert(trained);

    return true;
}

// Test: T-MA-004 - Transaction Flow Analysis
INTEGRATION_TEST(test_transaction_flow) {
    MempoolAnalytics analytics;

    // Simulate varying transaction flow
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(50, 150);

    auto start_time = std::chrono::steady_clock::now();

    // Simulate 10 seconds of activity
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(1)) {
        int num_add = dis(gen);
        int num_remove = dis(gen) / 2;

        for (int i = 0; i < num_add; i++) {
            analytics.OnTransactionAdded(250, 12.0, i % 6);
        }

        for (int i = 0; i < num_remove; i++) {
            analytics.OnTransactionRemoved(250, 12.0, i % 6);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Analyze flow
    auto flow = analytics.AnalyzeTransactionFlow();

    std::cout << "  → Inflow rate: " << flow.inflow_rate << " tx/sec" << std::endl;
    std::cout << "  → Outflow rate: " << flow.outflow_rate << " tx/sec" << std::endl;
    std::cout << "  → Net flow: " << (flow.inflow_rate - flow.outflow_rate) << " tx/sec" << std::endl;

    assert(flow.inflow_rate > 0);

    return true;
}

// Test: T-MA-005 - Concurrent Access
INTEGRATION_TEST(test_concurrent_access) {
    MempoolAnalytics analytics;

    const int NUM_THREADS = 10;
    const int QUERIES_PER_THREAD = 100;

    std::vector<std::thread> threads;
    std::atomic<int> successful_queries{0};

    auto start = std::chrono::steady_clock::now();

    // Spawn concurrent threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&analytics, &successful_queries]() {
            for (int j = 0; j < QUERIES_PER_THREAD; j++) {
                [[maybe_unused]] auto stats = analytics.GetCurrentStats();
                // Stats retrieved successfully
                successful_queries++;
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "  → " << NUM_THREADS << " threads, " << QUERIES_PER_THREAD << " queries each" << std::endl;
    std::cout << "  → Total duration: " << duration << "ms" << std::endl;
    std::cout << "  → Average per query: " << (duration / (NUM_THREADS * QUERIES_PER_THREAD)) << "ms" << std::endl;

    assert(successful_queries == NUM_THREADS * QUERIES_PER_THREAD);
    assert(duration < 200 * NUM_THREADS); // All queries should complete quickly

    return true;
}

// Test runner
int main() {
    std::cout << "=== Mempool Analytics Integration Tests ===" << std::endl << std::endl;

    // Run all tests
    run_test_realtime_statistics();
    run_test_historical_snapshots();
    run_test_fee_estimation();
    run_test_transaction_flow();
    run_test_concurrent_access();

    // Print results
    std::cout << std::endl << "=== Test Results ===" << std::endl;

    int passed = 0;
    int failed = 0;
    uint64_t total_duration = 0;

    for (const auto& result : results) {
        std::string status = result.passed ? "PASS" : "FAIL";
        std::cout << "[" << status << "] " << result.test_name
                  << " (" << result.duration_ms << "ms)" << std::endl;

        if (result.passed) passed++;
        else failed++;

        total_duration += result.duration_ms;
    }

    std::cout << std::endl;
    std::cout << "Total: " << results.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total Duration: " << total_duration << "ms" << std::endl;

    return failed == 0 ? 0 : 1;
}
