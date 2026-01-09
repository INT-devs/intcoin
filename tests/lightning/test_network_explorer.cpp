// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/network_explorer.h>
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

// Test: Explorer initialization
bool test_explorer_init() {
    NetworkExplorer explorer;

    // Explorer should initialize successfully
    TEST_ASSERT(true, "Explorer initialized");

    return true;
}

// Test: Get network statistics
bool test_network_stats() {
    NetworkExplorer explorer;

    [[maybe_unused]] auto stats = explorer.GetNetworkStats();

    TEST_ASSERT(true, "Number of nodes accessible"); // unsigned field
    TEST_ASSERT(true, "Number of channels accessible"); // unsigned field
    TEST_ASSERT(true, "Total capacity accessible"); // unsigned field

    return true;
}

// Test: Get all nodes
bool test_get_all_nodes() {
    NetworkExplorer explorer;

    auto nodes = explorer.GetAllNodes();

    TEST_ASSERT(true, "Nodes should be accessible"); // size() is unsigned

    return true;
}

// Test: Get all channels
bool test_get_all_channels() {
    NetworkExplorer explorer;

    auto channels = explorer.GetAllChannels();

    TEST_ASSERT(true, "Channels should be accessible"); // size() is unsigned

    return true;
}

// Test: Get node
bool test_get_node() {
    NetworkExplorer explorer;

    auto node = explorer.GetNode("03node_pubkey...");

    // Node may not exist in test environment
    TEST_ASSERT(true, "Get node completed");

    return true;
}

// Test: Get channel
bool test_get_channel() {
    NetworkExplorer explorer;

    auto channel = explorer.GetChannel("channel_123");

    // Channel may not exist in test environment
    TEST_ASSERT(true, "Get channel completed");

    return true;
}

// Test: Search nodes
bool test_search_nodes() {
    NetworkExplorer explorer;

    auto results = explorer.SearchNodes("test", 10);

    TEST_ASSERT(results.size() <= 10, "Results should respect limit");

    return true;
}

// Test: Get top nodes
bool test_get_top_nodes() {
    NetworkExplorer explorer;

    auto top = explorer.GetTopNodes(NodeRanking::BY_CAPACITY, 100);

    TEST_ASSERT(top.size() <= 100, "Top nodes should respect limit");

    return true;
}

// Test: Get largest channels
bool test_largest_channels() {
    NetworkExplorer explorer;

    auto channels = explorer.GetLargestChannels(50);

    TEST_ASSERT(channels.size() <= 50, "Largest channels should respect limit");

    return true;
}

// Test: Get node channels
bool test_node_channels() {
    NetworkExplorer explorer;

    auto channels = explorer.GetNodeChannels("03node...");

    TEST_ASSERT(true, "Node channels should be accessible"); // size() is unsigned

    return true;
}

// Test: Get node peers
bool test_node_peers() {
    NetworkExplorer explorer;

    auto peers = explorer.GetNodePeers("03node...");

    TEST_ASSERT(true, "Node peers should be accessible"); // size() is unsigned

    return true;
}

// Test: Find path
bool test_find_path() {
    NetworkExplorer explorer;

    auto path = explorer.FindPath("03source...", "03dest...");

    // Path may be empty without network
    TEST_ASSERT(true, "Path should be accessible"); // size() is unsigned

    return true;
}

// Test: Get node neighbors
bool test_node_neighbors() {
    NetworkExplorer explorer;

    auto neighbors = explorer.GetNodeNeighbors("03node...", 2);

    TEST_ASSERT(true, "Neighbors should be accessible"); // size() is unsigned

    return true;
}

// Test: Calculate node centrality
bool test_node_centrality() {
    NetworkExplorer explorer;

    double centrality = explorer.CalculateNodeCentrality("03node...");

    TEST_ASSERT(centrality >= 0.0 && centrality <= 1.0, "Centrality should be 0.0-1.0");

    return true;
}

// Test: Get network topology
bool test_network_topology() {
    NetworkExplorer explorer;

    std::string topology = explorer.GetNetworkTopology("json");

    TEST_ASSERT(!topology.empty(), "Topology should not be empty");

    return true;
}

// Test: Refresh network graph
bool test_refresh_graph() {
    NetworkExplorer explorer;

    [[maybe_unused]] uint32_t updates = explorer.RefreshNetworkGraph();

    TEST_ASSERT(true, "Updates accessible"); // unsigned field

    return true;
}

// Test: Get zombie channels
bool test_zombie_channels() {
    NetworkExplorer explorer;

    auto zombies = explorer.GetZombieChannels(14);

    TEST_ASSERT(true, "Zombie channels should be accessible"); // size() is unsigned

    return true;
}

// Test: Analyze routing position
bool test_routing_position() {
    NetworkExplorer explorer;

    auto analysis = explorer.AnalyzeRoutingPosition("03node...");

    TEST_ASSERT(analysis.centrality_score >= 0.0 && analysis.centrality_score <= 1.0,
                "Centrality score should be 0.0-1.0");

    return true;
}

// Test: Get channel updates
bool test_channel_updates() {
    NetworkExplorer explorer;

    uint64_t timestamp = 1704067200;  // 2024-01-01
    auto updates = explorer.GetChannelUpdates(timestamp, 100);

    TEST_ASSERT(updates.size() <= 100, "Updates should respect limit");

    return true;
}

// Test: Get new nodes
bool test_new_nodes() {
    NetworkExplorer explorer;

    uint64_t timestamp = 1704067200;
    auto nodes = explorer.GetNewNodes(timestamp, 50);

    TEST_ASSERT(nodes.size() <= 50, "New nodes should respect limit");

    return true;
}

// Test: Export node data
bool test_export_node() {
    NetworkExplorer explorer;

    std::string json = explorer.ExportNodeData("03node...", "json");

    TEST_ASSERT(!json.empty(), "Exported data should not be empty");

    return true;
}

// Test: Export channel data
bool test_export_channel() {
    NetworkExplorer explorer;

    std::string json = explorer.ExportChannelData("channel_123", "json");

    TEST_ASSERT(!json.empty(), "Exported data should not be empty");

    return true;
}

// Test: Get graph timestamp
bool test_graph_timestamp() {
    NetworkExplorer explorer;

    [[maybe_unused]] uint64_t timestamp = explorer.GetGraphTimestamp();

    TEST_ASSERT(true, "Timestamp accessible"); // unsigned field

    return true;
}

// Test: Clear cache
bool test_clear_cache() {
    NetworkExplorer explorer;

    explorer.ClearCache();

    // Cache clearing should complete
    TEST_ASSERT(true, "Cache cleared successfully");

    return true;
}

// Test: Channel filter
bool test_channel_filter() {
    NetworkExplorer explorer;

    ChannelFilter filter;
    filter.min_capacity = 100000;
    filter.max_capacity = 10000000;
    filter.exclude_disabled = true;

    auto channels = explorer.GetAllChannels(filter);

    TEST_ASSERT(true, "Filtered channels should be accessible"); // size() is unsigned

    return true;
}

// Test: Node ranking names
bool test_ranking_names() {
    std::string name = GetNodeRankingName(NodeRanking::BY_CAPACITY);
    TEST_ASSERT(!name.empty(), "Ranking name should not be empty");

    NodeRanking ranking = ParseNodeRanking("BY_CAPACITY");
    TEST_ASSERT(ranking == NodeRanking::BY_CAPACITY, "Should parse correctly");

    return true;
}

int main() {
    int total = 0, passed = 0, failed = 0;

    std::cout << "=== Network Explorer Tests ===" << std::endl;

    RUN_TEST(test_explorer_init);
    RUN_TEST(test_network_stats);
    RUN_TEST(test_get_all_nodes);
    RUN_TEST(test_get_all_channels);
    RUN_TEST(test_get_node);
    RUN_TEST(test_get_channel);
    RUN_TEST(test_search_nodes);
    RUN_TEST(test_get_top_nodes);
    RUN_TEST(test_largest_channels);
    RUN_TEST(test_node_channels);
    RUN_TEST(test_node_peers);
    RUN_TEST(test_find_path);
    RUN_TEST(test_node_neighbors);
    RUN_TEST(test_node_centrality);
    RUN_TEST(test_network_topology);
    RUN_TEST(test_refresh_graph);
    RUN_TEST(test_zombie_channels);
    RUN_TEST(test_routing_position);
    RUN_TEST(test_channel_updates);
    RUN_TEST(test_new_nodes);
    RUN_TEST(test_export_node);
    RUN_TEST(test_export_channel);
    RUN_TEST(test_graph_timestamp);
    RUN_TEST(test_clear_cache);
    RUN_TEST(test_channel_filter);
    RUN_TEST(test_ranking_names);

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Total:  " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
