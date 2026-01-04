// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/mempool_analytics/analytics.h>
#include <intcoin/mempool_analytics/fee_estimator.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace intcoin::mempool_analytics;

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

// Test: Basic analytics initialization
bool test_analytics_init() {
    MempoolAnalytics analytics;
    auto stats = analytics.GetCurrentStats();

    TEST_ASSERT(stats.size == 0, "Initial size should be 0");
    TEST_ASSERT(stats.bytes == 0, "Initial bytes should be 0");
    TEST_ASSERT(stats.priority_dist.GetTotalCount() == 0, "Initial priority count should be 0");

    return true;
}

// Test: Transaction addition
bool test_transaction_addition() {
    MempoolAnalytics analytics;

    // Add LOW priority transaction
    analytics.OnTransactionAdded(250, 10.0, 0);

    auto stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.size == 1, "Size should be 1 after adding transaction");
    TEST_ASSERT(stats.bytes == 250, "Bytes should match transaction size");
    TEST_ASSERT(stats.priority_dist.low_count == 1, "LOW priority count should be 1");

    // Add HIGH priority transaction
    analytics.OnTransactionAdded(500, 25.0, 2);

    stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.size == 2, "Size should be 2");
    TEST_ASSERT(stats.bytes == 750, "Total bytes should be 750");
    TEST_ASSERT(stats.priority_dist.high_count == 1, "HIGH priority count should be 1");

    return true;
}

// Test: Transaction removal
bool test_transaction_removal() {
    MempoolAnalytics analytics;

    analytics.OnTransactionAdded(250, 10.0, 0);
    analytics.OnTransactionAdded(500, 25.0, 2);

    analytics.OnTransactionRemoved(250, 10.0, 0);

    auto stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.size == 1, "Size should be 1 after removal");
    TEST_ASSERT(stats.bytes == 500, "Bytes should be 500");
    TEST_ASSERT(stats.priority_dist.low_count == 0, "LOW priority count should be 0");
    TEST_ASSERT(stats.priority_dist.high_count == 1, "HIGH priority count should still be 1");

    return true;
}

// Test: Priority distribution
bool test_priority_distribution() {
    MempoolAnalytics analytics;

    // Add transactions of each priority level
    analytics.OnTransactionAdded(100, 5.0, 0);   // LOW
    analytics.OnTransactionAdded(100, 10.0, 1);  // NORMAL
    analytics.OnTransactionAdded(100, 20.0, 2);  // HIGH
    analytics.OnTransactionAdded(100, 30.0, 3);  // HTLC
    analytics.OnTransactionAdded(100, 40.0, 4);  // BRIDGE
    analytics.OnTransactionAdded(100, 50.0, 5);  // CRITICAL

    auto stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.priority_dist.low_count == 1, "LOW count should be 1");
    TEST_ASSERT(stats.priority_dist.normal_count == 1, "NORMAL count should be 1");
    TEST_ASSERT(stats.priority_dist.high_count == 1, "HIGH count should be 1");
    TEST_ASSERT(stats.priority_dist.htlc_count == 1, "HTLC count should be 1");
    TEST_ASSERT(stats.priority_dist.bridge_count == 1, "BRIDGE count should be 1");
    TEST_ASSERT(stats.priority_dist.critical_count == 1, "CRITICAL count should be 1");
    TEST_ASSERT(stats.priority_dist.GetTotalCount() == 6, "Total count should be 6");

    return true;
}

// Test: Snapshot functionality
bool test_snapshots() {
    MempoolAnalytics analytics;

    analytics.OnTransactionAdded(250, 10.0, 1);
    analytics.TakeSnapshot();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    analytics.OnTransactionAdded(300, 15.0, 2);
    analytics.TakeSnapshot();

    auto history = analytics.GetHistory(0, UINT64_MAX);
    TEST_ASSERT(history.size() == 2, "Should have 2 snapshots");
    TEST_ASSERT(history[0].stats.size == 1, "First snapshot should have 1 tx");
    TEST_ASSERT(history[1].stats.size == 2, "Second snapshot should have 2 txs");

    return true;
}

// Test: Flow metrics
bool test_flow_metrics() {
    MempoolAnalytics analytics;

    analytics.OnTransactionAdded(250, 10.0, 1);
    analytics.OnTransactionAdded(300, 15.0, 2);
    analytics.OnTransactionRemoved(250, 10.0, 1);

    auto metrics = analytics.AnalyzeTransactionFlow();
    TEST_ASSERT(metrics.inflow_rate >= 0, "Inflow rate should be non-negative");
    TEST_ASSERT(metrics.outflow_rate >= 0, "Outflow rate should be non-negative");

    return true;
}

// Test: JSON export
bool test_json_export() {
    MempoolAnalytics analytics;

    analytics.OnTransactionAdded(250, 10.0, 0);
    analytics.OnTransactionAdded(500, 25.0, 2);

    std::string json = analytics.ExportToJSON();
    TEST_ASSERT(!json.empty(), "JSON export should not be empty");
    TEST_ASSERT(json.find("current_stats") != std::string::npos,
                "JSON should contain current_stats");
    TEST_ASSERT(json.find("priority_distribution") != std::string::npos,
                "JSON should contain priority_distribution");

    return true;
}

// Test: Fee estimator initialization
bool test_fee_estimator_init() {
    FeeEstimator estimator;

    auto estimate = estimator.EstimateFee(1);
    TEST_ASSERT(estimate.fee_rate > 0, "Fee estimate should be positive");
    TEST_ASSERT(estimate.target_blocks == 1, "Target blocks should match");

    return true;
}

// Test: Fee estimation for different target blocks
bool test_fee_estimation_targets() {
    FeeEstimator estimator;

    auto estimate1 = estimator.EstimateFee(1);
    auto estimate6 = estimator.EstimateFee(6);

    TEST_ASSERT(estimate1.fee_rate >= estimate6.fee_rate,
                "Fee for 1 block should be >= fee for 6 blocks");

    return true;
}

// Test: Fee range calculation
bool test_fee_range() {
    FeeEstimator estimator;

    auto range = estimator.GetFeeRange(3, 0.95);
    TEST_ASSERT(range.min_fee_rate <= range.optimal_fee_rate,
                "Min fee should be <= optimal");
    TEST_ASSERT(range.optimal_fee_rate <= range.max_fee_rate,
                "Optimal fee should be <= max");
    TEST_ASSERT(range.confidence == 0.95, "Confidence should match");

    return true;
}

// Test: Model training
bool test_model_training() {
    FeeEstimator estimator;

    std::vector<BlockData> blocks;
    for (uint32_t i = 0; i < 10; ++i) {
        BlockData block;
        block.height = 1000 + i;
        block.timestamp = 1640000000 + (i * 600);
        block.fee_rates = {10.0, 15.0, 20.0, 25.0};
        block.total_size = 1000000;
        blocks.push_back(block);
    }

    bool trained = estimator.TrainModel(blocks);
    TEST_ASSERT(trained, "Model training should succeed");

    auto estimate = estimator.EstimateFee(3);
    TEST_ASSERT(estimate.fee_rate > 0, "Estimate should be positive after training");

    return true;
}

// Test: Model update
bool test_model_update() {
    FeeEstimator estimator;

    BlockData block;
    block.height = 2000;
    block.timestamp = 1650000000;
    block.fee_rates = {12.0, 18.0, 22.0};
    block.total_size = 800000;

    estimator.UpdateModel(block);

    auto estimate = estimator.EstimateFee(1);
    TEST_ASSERT(estimate.fee_rate > 0, "Estimate should work after update");

    return true;
}

// Test: History pruning
bool test_history_pruning() {
    MempoolAnalytics analytics;

    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    analytics.TakeSnapshot();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    analytics.TakeSnapshot();

    auto history_before = analytics.GetHistory(0, UINT64_MAX);
    size_t count_before = history_before.size();

    // Prune everything before now + 1
    analytics.PruneHistory(now + 1);

    auto history_after = analytics.GetHistory(0, UINT64_MAX);
    TEST_ASSERT(history_after.size() < count_before || count_before == 0,
                "History should be pruned");

    return true;
}

// Test: Concurrent access
bool test_concurrent_access() {
    MempoolAnalytics analytics;

    std::thread t1([&analytics]() {
        for (int i = 0; i < 100; ++i) {
            analytics.OnTransactionAdded(250, 10.0, 0);
        }
    });

    std::thread t2([&analytics]() {
        for (int i = 0; i < 100; ++i) {
            auto stats = analytics.GetCurrentStats();
            (void)stats; // Suppress unused warning
        }
    });

    t1.join();
    t2.join();

    auto stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.size == 100, "Final size should be 100");

    return true;
}

// Test: Large transaction volume
bool test_large_volume() {
    MempoolAnalytics analytics;

    for (int i = 0; i < 10000; ++i) {
        analytics.OnTransactionAdded(250, 10.0 + (i % 50), i % 6);
    }

    auto stats = analytics.GetCurrentStats();
    TEST_ASSERT(stats.size == 10000, "Size should be 10000");
    TEST_ASSERT(stats.bytes == 2500000, "Bytes should be 2.5 MB");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Mempool Analytics Tests ===" << std::endl;

    RUN_TEST(test_analytics_init);
    RUN_TEST(test_transaction_addition);
    RUN_TEST(test_transaction_removal);
    RUN_TEST(test_priority_distribution);
    RUN_TEST(test_snapshots);
    RUN_TEST(test_flow_metrics);
    RUN_TEST(test_json_export);
    RUN_TEST(test_fee_estimator_init);
    RUN_TEST(test_fee_estimation_targets);
    RUN_TEST(test_fee_range);
    RUN_TEST(test_model_training);
    RUN_TEST(test_model_update);
    RUN_TEST(test_history_pruning);
    RUN_TEST(test_concurrent_access);
    RUN_TEST(test_large_volume);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
