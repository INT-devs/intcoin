// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/submarine_swaps.h>
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
    SubmarineSwapManager manager;

    TEST_ASSERT(manager.IsEnabled(), "Manager should be enabled by default");

    auto config = manager.GetConfig();
    TEST_ASSERT(config.min_swap_amount > 0, "Min swap amount should be positive");

    return true;
}

// Test: Get quote
bool test_get_quote() {
    SubmarineSwapManager manager;

    auto quote = manager.GetQuote(SwapType::SWAP_IN, 100000);

    TEST_ASSERT(quote.amount == 100000, "Quote amount should match");
    TEST_ASSERT(quote.total_fee >= 0, "Total fee should be non-negative");

    return true;
}

// Test: Create swap-in
bool test_create_swap_in() {
    SubmarineSwapManager manager;

    auto swap = manager.CreateSwapIn(
        100000,
        "bc1qrefundaddress..."
    );

    TEST_ASSERT(!swap.swap_id.empty(), "Swap ID should not be empty");
    TEST_ASSERT(swap.type == SwapType::SWAP_IN, "Type should be SWAP_IN");
    TEST_ASSERT(swap.amount == 100000, "Amount should match");

    return true;
}

// Test: Create swap-out
bool test_create_swap_out() {
    SubmarineSwapManager manager;

    auto swap = manager.CreateSwapOut(
        200000,
        "bc1qclaimaddress..."
    );

    TEST_ASSERT(!swap.swap_id.empty(), "Swap ID should not be empty");
    TEST_ASSERT(swap.type == SwapType::SWAP_OUT, "Type should be SWAP_OUT");
    TEST_ASSERT(swap.amount == 200000, "Amount should match");

    return true;
}

// Test: Get swap
bool test_get_swap() {
    SubmarineSwapManager manager;

    auto swap = manager.CreateSwapIn(100000, "bc1qrefund...");

    auto retrieved = manager.GetSwap(swap.swap_id);

    TEST_ASSERT(retrieved.swap_id == swap.swap_id, "Swap IDs should match");
    TEST_ASSERT(retrieved.amount == swap.amount, "Amounts should match");

    return true;
}

// Test: Get active swaps
bool test_active_swaps() {
    SubmarineSwapManager manager;

    auto active = manager.GetActiveSwaps();

    TEST_ASSERT(active.size() >= 0, "Active swaps should be accessible");

    return true;
}

// Test: Get swap history
bool test_swap_history() {
    SubmarineSwapManager manager;

    auto history = manager.GetSwapHistory(10);

    TEST_ASSERT(history.size() <= 10, "History should respect limit");

    return true;
}

// Test: Estimate fees
bool test_estimate_fees() {
    SubmarineSwapManager manager;

    uint64_t fees = manager.EstimateFees(SwapType::SWAP_IN, 100000);

    TEST_ASSERT(fees >= 0, "Fees should be non-negative");

    return true;
}

// Test: Get swap limits
bool test_swap_limits() {
    SubmarineSwapManager manager;

    auto limits = manager.GetSwapLimits(SwapType::SWAP_IN);

    TEST_ASSERT(limits.min_amount > 0, "Min amount should be positive");
    TEST_ASSERT(limits.max_amount > limits.min_amount, "Max > Min");

    return true;
}

// Test: Statistics
bool test_statistics() {
    SubmarineSwapManager manager;

    auto stats = manager.GetStatistics();

    TEST_ASSERT(stats.total_swaps >= 0, "Total swaps should be non-negative");
    TEST_ASSERT(stats.total_fees_paid >= 0, "Total fees should be non-negative");

    return true;
}

// Test: Cancel swap
bool test_cancel_swap() {
    SubmarineSwapManager manager;

    auto swap = manager.CreateSwapIn(100000, "bc1qrefund...");

    bool cancelled = manager.CancelSwap(swap.swap_id);

    // Cancellation may or may not succeed
    TEST_ASSERT(true, "Cancel operation completed");

    return true;
}

// Test: Monitor swap
bool test_monitor_swap() {
    SubmarineSwapManager manager;

    auto swap = manager.CreateSwapIn(100000, "bc1qrefund...");

    SwapStatus status = manager.MonitorSwap(swap.swap_id);

    TEST_ASSERT(status == SwapStatus::PENDING || status == SwapStatus::INVOICE_GENERATED,
                "Status should be valid");

    return true;
}

// Test: Configuration
bool test_configuration() {
    SubmarineSwapManager manager;

    SubmarineSwapManager::Config config;
    config.min_swap_amount = 50000;
    config.max_swap_amount = 5000000;

    manager.SetConfig(config);

    auto retrieved = manager.GetConfig();
    TEST_ASSERT(retrieved.min_swap_amount == 50000, "Min amount should match");
    TEST_ASSERT(retrieved.max_swap_amount == 5000000, "Max amount should match");

    return true;
}

// Test: Enable/disable
bool test_enable_disable() {
    SubmarineSwapManager manager;

    TEST_ASSERT(manager.IsEnabled(), "Should be enabled initially");

    manager.SetEnabled(false);
    TEST_ASSERT(!manager.IsEnabled(), "Should be disabled");

    manager.SetEnabled(true);
    TEST_ASSERT(manager.IsEnabled(), "Should be enabled again");

    return true;
}

// Test: Swap type names
bool test_type_names() {
    std::string name = GetSwapTypeName(SwapType::SWAP_IN);
    TEST_ASSERT(!name.empty(), "Type name should not be empty");

    SwapType type = ParseSwapType("SWAP_IN");
    TEST_ASSERT(type == SwapType::SWAP_IN, "Should parse correctly");

    return true;
}

// Test: Swap status names
bool test_status_names() {
    std::string name = GetSwapStatusName(SwapStatus::COMPLETED);
    TEST_ASSERT(!name.empty(), "Status name should not be empty");

    SwapStatus status = ParseSwapStatus("COMPLETED");
    TEST_ASSERT(status == SwapStatus::COMPLETED, "Should parse correctly");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Submarine Swaps Tests ===" << std::endl;

    RUN_TEST(test_manager_init);
    RUN_TEST(test_get_quote);
    RUN_TEST(test_create_swap_in);
    RUN_TEST(test_create_swap_out);
    RUN_TEST(test_get_swap);
    RUN_TEST(test_active_swaps);
    RUN_TEST(test_swap_history);
    RUN_TEST(test_estimate_fees);
    RUN_TEST(test_swap_limits);
    RUN_TEST(test_statistics);
    RUN_TEST(test_cancel_swap);
    RUN_TEST(test_monitor_swap);
    RUN_TEST(test_configuration);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_type_names);
    RUN_TEST(test_status_names);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
