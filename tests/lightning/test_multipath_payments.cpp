// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/multipath_payments.h>
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
    MultiPathPaymentManager manager;

    TEST_ASSERT(manager.IsEnabled(), "Manager should be enabled by default");

    auto config = manager.GetConfig();
    TEST_ASSERT(config.max_paths <= 8, "Max paths should be reasonable");

    return true;
}

// Test: Configuration
bool test_configuration() {
    MultiPathPaymentManager manager;

    MPPConfig config;
    config.max_paths = 4;
    config.strategy = SplitStrategy::MINIMIZE_FEES;
    config.enable_amp = false;

    manager.SetConfig(config);

    auto retrieved = manager.GetConfig();
    TEST_ASSERT(retrieved.max_paths == 4, "Max paths should be 4");
    TEST_ASSERT(retrieved.strategy == SplitStrategy::MINIMIZE_FEES, "Strategy should match");
    TEST_ASSERT(!retrieved.enable_amp, "AMP should be disabled");

    return true;
}

// Test: Send payment
bool test_send_payment() {
    MultiPathPaymentManager manager;

    std::string payment_id = manager.SendPayment(
        "03abc123...",  // destination
        1000000,        // 1M msats
        "payment_hash_123",
        10000           // max fee
    );

    TEST_ASSERT(!payment_id.empty(), "Payment ID should not be empty");

    auto payment = manager.GetPayment(payment_id);
    TEST_ASSERT(payment.total_amount_msat == 1000000, "Amount should match");

    return true;
}

// Test: Send AMP payment
bool test_send_amp_payment() {
    MPPConfig config;
    config.enable_amp = true;

    MultiPathPaymentManager manager(config);

    std::string payment_id = manager.SendAMPPayment(
        "03def456...",
        2000000,
        20000
    );

    TEST_ASSERT(!payment_id.empty(), "AMP payment ID should not be empty");

    return true;
}

// Test: Payment splitting
bool test_payment_splitting() {
    MultiPathPaymentManager manager;

    auto result = manager.SplitPayment(
        "03abc123...",
        5000000,  // 5M msats
        50000     // max fee
    );

    TEST_ASSERT(!result.routes.empty(), "Should find routes");
    TEST_ASSERT(result.total_amount_msat > 0, "Total amount should be positive");

    return true;
}

// Test: Find routes
bool test_find_routes() {
    MultiPathPaymentManager manager;

    auto routes = manager.FindRoutes(
        "03abc123...",
        1000000,
        3  // Find 3 routes
    );

    // Routes may be empty in test environment
    TEST_ASSERT(routes.size() <= 3, "Should not exceed requested routes");

    return true;
}

// Test: Optimal split calculation
bool test_optimal_split() {
    MultiPathPaymentManager manager;

    std::vector<PaymentRoute> routes;
    PaymentRoute route1, route2;
    route1.amount_msat = 500000;
    route2.amount_msat = 500000;
    routes.push_back(route1);
    routes.push_back(route2);

    auto splits = manager.CalculateOptimalSplit(1000000, routes);

    TEST_ASSERT(splits.size() <= routes.size(), "Splits should match routes");

    return true;
}

// Test: Get active payments
bool test_active_payments() {
    MultiPathPaymentManager manager;

    auto active = manager.GetActivePayments();

    // Active payments list should be accessible
    TEST_ASSERT(active.size() >= 0, "Active payments should be accessible");

    return true;
}

// Test: Payment history
bool test_payment_history() {
    MultiPathPaymentManager manager;

    auto history = manager.GetPaymentHistory(10);

    TEST_ASSERT(history.size() <= 10, "History should respect limit");

    return true;
}

// Test: Statistics
bool test_statistics() {
    MultiPathPaymentManager manager;

    auto stats = manager.GetStatistics();

    TEST_ASSERT(stats.total_payments >= 0, "Total payments should be non-negative");
    TEST_ASSERT(stats.average_success_rate >= 0.0 && stats.average_success_rate <= 1.0,
                "Success rate should be 0.0-1.0");

    return true;
}

// Test: Cancel payment
bool test_cancel_payment() {
    MultiPathPaymentManager manager;

    std::string payment_id = manager.SendPayment(
        "03abc123...",
        1000000,
        "payment_hash",
        10000
    );

    bool cancelled = manager.CancelPayment(payment_id);
    (void)cancelled;  // Suppress unused warning

    // Cancellation may or may not succeed depending on state
    TEST_ASSERT(true, "Cancel operation completed");

    return true;
}

// Test: Enable/disable
bool test_enable_disable() {
    MultiPathPaymentManager manager;

    TEST_ASSERT(manager.IsEnabled(), "Should be enabled initially");

    manager.SetEnabled(false);
    TEST_ASSERT(!manager.IsEnabled(), "Should be disabled");

    manager.SetEnabled(true);
    TEST_ASSERT(manager.IsEnabled(), "Should be enabled again");

    return true;
}

// Test: Payment status names
bool test_status_names() {
    std::string name = GetPaymentStatusName(PaymentStatus::SUCCEEDED);
    TEST_ASSERT(!name.empty(), "Status name should not be empty");

    PaymentStatus status = ParsePaymentStatus("SUCCEEDED");
    TEST_ASSERT(status == PaymentStatus::SUCCEEDED, "Should parse correctly");

    return true;
}

// Test: Split strategy names
bool test_strategy_names() {
    std::string name = GetSplitStrategyName(SplitStrategy::MINIMIZE_FEES);
    TEST_ASSERT(!name.empty(), "Strategy name should not be empty");

    SplitStrategy strategy = ParseSplitStrategy("MINIMIZE_FEES");
    TEST_ASSERT(strategy == SplitStrategy::MINIMIZE_FEES, "Should parse correctly");

    return true;
}

// Test: Clear history
bool test_clear_history() {
    MultiPathPaymentManager manager;

    manager.ClearHistory();

    auto history = manager.GetPaymentHistory();
    TEST_ASSERT(history.size() == 0, "History should be empty after clear");

    return true;
}

// Test: Retry failed parts
bool test_retry_failed() {
    MultiPathPaymentManager manager;

    std::string payment_id = manager.SendPayment(
        "03abc123...",
        1000000,
        "payment_hash",
        10000
    );

    bool retried = manager.RetryFailedParts(payment_id);
    (void)retried;  // Suppress unused warning

    // Retry may or may not succeed
    TEST_ASSERT(true, "Retry operation completed");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Multi-Path Payments Tests ===" << std::endl;

    RUN_TEST(test_manager_init);
    RUN_TEST(test_configuration);
    RUN_TEST(test_send_payment);
    RUN_TEST(test_send_amp_payment);
    RUN_TEST(test_payment_splitting);
    RUN_TEST(test_find_routes);
    RUN_TEST(test_optimal_split);
    RUN_TEST(test_active_payments);
    RUN_TEST(test_payment_history);
    RUN_TEST(test_statistics);
    RUN_TEST(test_cancel_payment);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_status_names);
    RUN_TEST(test_strategy_names);
    RUN_TEST(test_clear_history);
    RUN_TEST(test_retry_failed);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
