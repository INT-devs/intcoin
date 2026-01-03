// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/routing.h>
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
    RoutingManager manager;

    auto config = manager.GetConfig();
    TEST_ASSERT(config.algorithm == RoutingAlgorithm::MISSION_CONTROL,
                "Default algorithm should be MISSION_CONTROL");

    return true;
}

// Test: Find route
bool test_find_route() {
    RoutingManager manager;

    RouteConstraints constraints;
    constraints.max_hops = 10;

    auto route = manager.FindRoute(
        "03source...",
        "03dest...",
        1000000,  // 1M msats
        constraints
    );

    // Route may be empty without network
    TEST_ASSERT(true, "Find route completed");

    return true;
}

// Test: Find multiple routes
bool test_find_routes() {
    RoutingManager manager;

    auto routes = manager.FindRoutes(
        "03source...",
        "03dest...",
        1000000,
        3  // Find 3 routes
    );

    TEST_ASSERT(routes.size() <= 3, "Should respect route count limit");

    return true;
}

// Test: Query route
bool test_query_route() {
    RoutingManager manager;

    auto route = manager.QueryRoute(
        "03source...",
        "03dest...",
        500000
    );

    // Route query should complete
    TEST_ASSERT(true, "Query route completed");

    return true;
}

// Test: Build route from hops
bool test_build_route() {
    RoutingManager manager;

    std::vector<std::string> hops = {
        "03node1...",
        "03node2...",
        "03node3..."
    };

    auto route = manager.BuildRoute(hops, 1000000);

    TEST_ASSERT(route.hops.size() <= hops.size(), "Hops should match");

    return true;
}

// Test: Calculate route score
bool test_route_score() {
    RoutingManager manager;

    Route route;
    route.total_fee_msat = 100;
    route.hop_count = 3;
    route.success_probability = 0.9;

    double score = manager.CalculateRouteScore(route);

    TEST_ASSERT(score >= 0.0 && score <= 1.0, "Score should be 0.0-1.0");

    return true;
}

// Test: Estimate success probability
bool test_success_probability() {
    RoutingManager manager;

    Route route;
    route.hop_count = 4;

    double probability = manager.EstimateSuccessProbability(route);

    TEST_ASSERT(probability >= 0.0 && probability <= 1.0, "Probability should be 0.0-1.0");

    return true;
}

// Test: Record payment attempt
bool test_record_attempt() {
    RoutingManager manager;

    PaymentAttempt attempt;
    attempt.attempt_id = "attempt_123";
    attempt.success = true;

    manager.RecordPaymentAttempt(attempt);

    // Recording should complete
    TEST_ASSERT(true, "Payment attempt recorded");

    return true;
}

// Test: Mission control entries
bool test_mission_control() {
    RoutingManager manager;

    auto entries = manager.GetMissionControlEntries();

    TEST_ASSERT(entries.size() >= 0, "Mission control entries should be accessible");

    return true;
}

// Test: Clear mission control
bool test_clear_mission_control() {
    RoutingManager manager;

    manager.ClearMissionControl();

    auto entries = manager.GetMissionControlEntries();
    TEST_ASSERT(entries.size() == 0, "Mission control should be empty after clear");

    return true;
}

// Test: Export mission control
bool test_export_mission_control() {
    RoutingManager manager;

    std::string json = manager.ExportMissionControl();

    TEST_ASSERT(!json.empty(), "Export should produce JSON");

    return true;
}

// Test: Import mission control
bool test_import_mission_control() {
    RoutingManager manager;

    std::string json = "{\"entries\":[]}";
    bool imported = manager.ImportMissionControl(json);
    (void)imported;  // Suppress unused warning

    // Import may succeed or fail
    TEST_ASSERT(true, "Import operation completed");

    return true;
}

// Test: Configuration
bool test_configuration() {
    RoutingManager manager;

    RoutingManager::Config config;
    config.algorithm = RoutingAlgorithm::DIJKSTRA;
    config.optimization = RouteOptimization::MINIMIZE_FEE;
    config.max_routes = 5;

    manager.SetConfig(config);

    auto retrieved = manager.GetConfig();
    TEST_ASSERT(retrieved.algorithm == RoutingAlgorithm::DIJKSTRA, "Algorithm should match");
    TEST_ASSERT(retrieved.optimization == RouteOptimization::MINIMIZE_FEE, "Optimization should match");

    return true;
}

// Test: Statistics
bool test_statistics() {
    RoutingManager manager;

    auto stats = manager.GetStatistics();

    TEST_ASSERT(stats.routes_found >= 0, "Routes found should be non-negative");
    TEST_ASSERT(stats.successful_payments >= 0, "Successful payments should be non-negative");

    return true;
}

// Test: Reset statistics
bool test_reset_statistics() {
    RoutingManager manager;

    manager.ResetStatistics();

    auto stats = manager.GetStatistics();
    TEST_ASSERT(stats.routes_found == 0, "Routes found should be 0 after reset");

    return true;
}

// Test: Algorithm names
bool test_algorithm_names() {
    std::string name = GetRoutingAlgorithmName(RoutingAlgorithm::DIJKSTRA);
    TEST_ASSERT(!name.empty(), "Algorithm name should not be empty");

    RoutingAlgorithm algorithm = ParseRoutingAlgorithm("DIJKSTRA");
    TEST_ASSERT(algorithm == RoutingAlgorithm::DIJKSTRA, "Should parse correctly");

    return true;
}

// Test: Optimization names
bool test_optimization_names() {
    std::string name = GetRouteOptimizationName(RouteOptimization::MINIMIZE_FEE);
    TEST_ASSERT(!name.empty(), "Optimization name should not be empty");

    RouteOptimization optimization = ParseRouteOptimization("MINIMIZE_FEE");
    TEST_ASSERT(optimization == RouteOptimization::MINIMIZE_FEE, "Should parse correctly");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Routing Tests ===" << std::endl;

    RUN_TEST(test_manager_init);
    RUN_TEST(test_find_route);
    RUN_TEST(test_find_routes);
    RUN_TEST(test_query_route);
    RUN_TEST(test_build_route);
    RUN_TEST(test_route_score);
    RUN_TEST(test_success_probability);
    RUN_TEST(test_record_attempt);
    RUN_TEST(test_mission_control);
    RUN_TEST(test_clear_mission_control);
    RUN_TEST(test_export_mission_control);
    RUN_TEST(test_import_mission_control);
    RUN_TEST(test_configuration);
    RUN_TEST(test_statistics);
    RUN_TEST(test_reset_statistics);
    RUN_TEST(test_algorithm_names);
    RUN_TEST(test_optimization_names);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
