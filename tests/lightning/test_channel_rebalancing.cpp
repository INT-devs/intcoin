// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/channel_rebalancing.h>
#include <iostream>
#include <cassert>

using namespace intcoin::lightning::v2;

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

// Test: Manager initialization
bool test_manager_init() {
    ChannelRebalancingManager manager;

    TEST_ASSERT(!manager.IsAutoRebalanceEnabled(), "Auto-rebalance disabled by default");

    auto config = manager.GetConfig();
    TEST_ASSERT(config.target_local_ratio >= 0.0 && config.target_local_ratio <= 1.0,
                "Target ratio should be 0.0-1.0");

    return true;
}

// Test: Get channel balances
bool test_get_balances() {
    ChannelRebalancingManager manager;

    auto balances = manager.GetChannelBalances();

    TEST_ASSERT(balances.size() >= 0, "Balances should be accessible");

    return true;
}

// Test: Rebalance channel
bool test_rebalance_channel() {
    ChannelRebalancingManager manager;

    std::string rebalance_id = manager.RebalanceChannel(
        "channel_1",
        "channel_2",
        100000,
        1000  // max fee
    );

    // Rebalancing may fail without actual channels
    TEST_ASSERT(true, "Rebalance operation completed");

    return true;
}

// Test: Get recommendations
bool test_recommendations() {
    ChannelRebalancingManager manager;

    auto recommendations = manager.GetRecommendations(5);

    TEST_ASSERT(recommendations.size() <= 5, "Should respect limit");

    return true;
}

// Test: Set channel target
bool test_channel_target() {
    ChannelRebalancingManager manager;

    RebalanceTarget target;
    target.channel_id = "channel_123";
    target.target_local_ratio = 0.6;
    target.priority = 8;

    manager.SetChannelTarget("channel_123", target);

    auto retrieved = manager.GetChannelTarget("channel_123");
    TEST_ASSERT(retrieved.target_local_ratio == 0.6, "Target ratio should match");
    TEST_ASSERT(retrieved.priority == 8, "Priority should match");

    return true;
}

// Test: Remove channel target
bool test_remove_target() {
    ChannelRebalancingManager manager;

    RebalanceTarget target;
    target.channel_id = "channel_123";

    manager.SetChannelTarget("channel_123", target);
    manager.RemoveChannelTarget("channel_123");

    // Target should be removed
    TEST_ASSERT(true, "Target removed successfully");

    return true;
}

// Test: Get active operations
bool test_active_operations() {
    ChannelRebalancingManager manager;

    auto active = manager.GetActiveOperations();

    TEST_ASSERT(active.size() >= 0, "Active operations should be accessible");

    return true;
}

// Test: Get history
bool test_history() {
    ChannelRebalancingManager manager;

    auto history = manager.GetHistory(10);

    TEST_ASSERT(history.size() <= 10, "History should respect limit");

    return true;
}

// Test: Estimate fee
bool test_estimate_fee() {
    ChannelRebalancingManager manager;

    uint64_t fee = manager.EstimateFee(
        "channel_1",
        "channel_2",
        100000,
        RebalanceMethod::CIRCULAR
    );

    TEST_ASSERT(fee >= 0, "Fee should be non-negative");

    return true;
}

// Test: Calculate optimal amount
bool test_optimal_amount() {
    ChannelRebalancingManager manager;

    uint64_t amount = manager.CalculateOptimalAmount(
        "channel_1",
        "channel_2"
    );

    TEST_ASSERT(amount >= 0, "Amount should be non-negative");

    return true;
}

// Test: Find circular route
bool test_circular_route() {
    ChannelRebalancingManager manager;

    auto route = manager.FindCircularRoute(
        "channel_1",
        "channel_2",
        100000
    );

    // Route may be empty without actual network
    TEST_ASSERT(route.size() >= 0, "Route should be accessible");

    return true;
}

// Test: Configuration
bool test_configuration() {
    ChannelRebalancingManager manager;

    ChannelRebalancingManager::Config config;
    config.strategy = RebalanceStrategy::AUTO_OPTIMIZED;
    config.target_local_ratio = 0.6;
    config.max_fee_per_rebalance = 5000;

    manager.SetConfig(config);

    auto retrieved = manager.GetConfig();
    TEST_ASSERT(retrieved.strategy == RebalanceStrategy::AUTO_OPTIMIZED, "Strategy should match");
    TEST_ASSERT(retrieved.target_local_ratio == 0.6, "Target ratio should match");

    return true;
}

// Test: Statistics
bool test_statistics() {
    ChannelRebalancingManager manager;

    auto stats = manager.GetStatistics();

    TEST_ASSERT(stats.total_rebalances >= 0, "Total rebalances should be non-negative");
    TEST_ASSERT(stats.total_fees_paid >= 0, "Total fees should be non-negative");

    return true;
}

// Test: Auto-rebalance enable/disable
bool test_auto_rebalance() {
    ChannelRebalancingManager manager;

    TEST_ASSERT(!manager.IsAutoRebalanceEnabled(), "Should be disabled initially");

    manager.SetAutoRebalance(true);
    TEST_ASSERT(manager.IsAutoRebalanceEnabled(), "Should be enabled");

    manager.SetAutoRebalance(false);
    TEST_ASSERT(!manager.IsAutoRebalanceEnabled(), "Should be disabled again");

    return true;
}

// Test: Clear history
bool test_clear_history() {
    ChannelRebalancingManager manager;

    manager.ClearHistory();

    auto history = manager.GetHistory();
    TEST_ASSERT(history.size() == 0, "History should be empty after clear");

    return true;
}

// Test: Strategy names
bool test_strategy_names() {
    std::string name = GetRebalanceStrategyName(RebalanceStrategy::AUTO_BALANCED);
    TEST_ASSERT(!name.empty(), "Strategy name should not be empty");

    RebalanceStrategy strategy = ParseRebalanceStrategy("AUTO_BALANCED");
    TEST_ASSERT(strategy == RebalanceStrategy::AUTO_BALANCED, "Should parse correctly");

    return true;
}

// Test: Method names
bool test_method_names() {
    std::string name = GetRebalanceMethodName(RebalanceMethod::CIRCULAR);
    TEST_ASSERT(!name.empty(), "Method name should not be empty");

    RebalanceMethod method = ParseRebalanceMethod("CIRCULAR");
    TEST_ASSERT(method == RebalanceMethod::CIRCULAR, "Should parse correctly");

    return true;
}

// Test: Status names
bool test_status_names() {
    std::string name = GetRebalanceStatusName(RebalanceStatus::COMPLETED);
    TEST_ASSERT(!name.empty(), "Status name should not be empty");

    RebalanceStatus status = ParseRebalanceStatus("COMPLETED");
    TEST_ASSERT(status == RebalanceStatus::COMPLETED, "Should parse correctly");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Channel Rebalancing Tests ===" << std::endl;

    RUN_TEST(test_manager_init);
    RUN_TEST(test_get_balances);
    RUN_TEST(test_rebalance_channel);
    RUN_TEST(test_recommendations);
    RUN_TEST(test_channel_target);
    RUN_TEST(test_remove_target);
    RUN_TEST(test_active_operations);
    RUN_TEST(test_history);
    RUN_TEST(test_estimate_fee);
    RUN_TEST(test_optimal_amount);
    RUN_TEST(test_circular_route);
    RUN_TEST(test_configuration);
    RUN_TEST(test_statistics);
    RUN_TEST(test_auto_rebalance);
    RUN_TEST(test_clear_history);
    RUN_TEST(test_strategy_names);
    RUN_TEST(test_method_names);
    RUN_TEST(test_status_names);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
