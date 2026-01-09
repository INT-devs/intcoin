// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_NETWORK_EXPLORER_H
#define INTCOIN_LIGHTNING_V2_NETWORK_EXPLORER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Node features
 */
struct NodeFeatures {
    bool wumbo_channels{false};      // Large channels support
    bool anchor_outputs{false};
    bool static_remote_key{false};
    bool payment_addr{false};
    bool mpp_optional{false};
    bool mpp_required{false};
    bool amp_optional{false};
    bool amp_required{false};
    bool taproot{false};
};

/**
 * Lightning node information
 */
struct NodeInfo {
    std::string pubkey;
    std::string alias;
    std::string color;               // RGB hex color
    std::vector<std::string> addresses; // Network addresses
    uint32_t last_update{0};
    NodeFeatures features;

    // Statistics
    uint32_t num_channels{0};
    uint64_t total_capacity{0};
    double avg_channel_size{0.0};
};

/**
 * Channel information
 */
struct ChannelInfo {
    std::string channel_id;
    std::string short_channel_id;    // Block:Tx:Output format
    std::string node1_pubkey;
    std::string node2_pubkey;
    uint64_t capacity{0};            // Capacity in ints

    // Node 1 policy
    uint64_t node1_fee_base_msat{0};
    uint32_t node1_fee_rate{0};      // Proportional fee (ppm)
    uint32_t node1_cltv_delta{0};
    uint64_t node1_htlc_min_msat{0};
    uint64_t node1_htlc_max_msat{0};
    bool node1_disabled{false};

    // Node 2 policy
    uint64_t node2_fee_base_msat{0};
    uint32_t node2_fee_rate{0};
    uint32_t node2_cltv_delta{0};
    uint64_t node2_htlc_min_msat{0};
    uint64_t node2_htlc_max_msat{0};
    bool node2_disabled{false};

    uint32_t last_update{0};
};

/**
 * Network statistics
 */
struct NetworkStats {
    uint32_t num_nodes{0};
    uint32_t num_channels{0};
    uint64_t total_capacity{0};
    uint64_t avg_channel_size{0};
    uint64_t median_channel_size{0};
    uint32_t avg_node_degree{0};      // Avg channels per node
    uint32_t max_node_degree{0};
    double network_density{0.0};
    uint32_t num_zombie_channels{0};  // Inactive channels
    uint64_t zombie_capacity{0};
};

/**
 * Node ranking criteria
 */
enum class NodeRanking {
    BY_CAPACITY,                      // Total channel capacity
    BY_CHANNELS,                      // Number of channels
    BY_CENTRALITY,                    // Network centrality
    BY_UPTIME,                        // Historical uptime
    BY_AGE                            // Node age
};

/**
 * Channel filter
 */
struct ChannelFilter {
    uint64_t min_capacity{0};
    uint64_t max_capacity{0};
    uint32_t max_fee_rate{0};
    bool exclude_disabled{true};
    std::vector<std::string> node_pubkeys; // Filter by nodes
};

/**
 * Path visualization
 */
struct PathVisualization {
    std::vector<NodeInfo> nodes;
    std::vector<ChannelInfo> channels;
    std::vector<std::string> node_sequence;
    uint64_t total_capacity{0};
    uint64_t total_fee{0};
};

/**
 * Network Explorer
 *
 * Provides tools to explore and visualize the Lightning Network graph,
 * query node and channel information, and analyze network topology.
 */
class NetworkExplorer {
public:
    NetworkExplorer();
    ~NetworkExplorer();

    /**
     * Get network statistics
     */
    NetworkStats GetNetworkStats() const;

    /**
     * Get node information
     *
     * @param pubkey Node public key
     * @return Node information
     */
    NodeInfo GetNode(const std::string& pubkey) const;

    /**
     * Get channel information
     *
     * @param channel_id Channel ID or short channel ID
     * @return Channel information
     */
    ChannelInfo GetChannel(const std::string& channel_id) const;

    /**
     * Get all nodes
     *
     * @return Vector of all nodes
     */
    std::vector<NodeInfo> GetAllNodes() const;

    /**
     * Get all channels
     *
     * @param filter Optional channel filter
     * @return Vector of channels
     */
    std::vector<ChannelInfo> GetAllChannels(
        const ChannelFilter& filter = ChannelFilter()
    ) const;

    /**
     * Get node channels
     *
     * @param pubkey Node public key
     * @return Vector of node's channels
     */
    std::vector<ChannelInfo> GetNodeChannels(const std::string& pubkey) const;

    /**
     * Get node peers
     *
     * @param pubkey Node public key
     * @return Vector of peer node pubkeys
     */
    std::vector<std::string> GetNodePeers(const std::string& pubkey) const;

    /**
     * Search nodes
     *
     * Search by alias or pubkey
     *
     * @param query Search query
     * @param limit Maximum results
     * @return Vector of matching nodes
     */
    std::vector<NodeInfo> SearchNodes(
        const std::string& query,
        uint32_t limit = 20
    ) const;

    /**
     * Get top nodes
     *
     * @param ranking Ranking criteria
     * @param limit Number of nodes
     * @return Vector of top nodes
     */
    std::vector<NodeInfo> GetTopNodes(
        NodeRanking ranking,
        uint32_t limit = 100
    ) const;

    /**
     * Get largest channels
     *
     * @param limit Number of channels
     * @return Vector of largest channels
     */
    std::vector<ChannelInfo> GetLargestChannels(uint32_t limit = 100) const;

    /**
     * Find path between nodes
     *
     * @param source Source node pubkey
     * @param destination Destination node pubkey
     * @return Path visualization
     */
    PathVisualization FindPath(
        const std::string& source,
        const std::string& destination
    ) const;

    /**
     * Get node neighbors
     *
     * Get nodes within N hops
     *
     * @param pubkey Node public key
     * @param max_hops Maximum hops (default 1)
     * @return Vector of neighbor nodes
     */
    std::vector<NodeInfo> GetNodeNeighbors(
        const std::string& pubkey,
        uint32_t max_hops = 1
    ) const;

    /**
     * Calculate node centrality
     *
     * Measure of node importance in network
     *
     * @param pubkey Node public key
     * @return Centrality score (0.0-1.0)
     */
    double CalculateNodeCentrality(const std::string& pubkey) const;

    /**
     * Get network topology
     *
     * Export network graph
     *
     * @param format Format (json, graphml, dot)
     * @return Network topology data
     */
    std::string GetNetworkTopology(const std::string& format = "json") const;

    /**
     * Refresh network graph
     *
     * Update from latest gossip
     *
     * @return Number of updates processed
     */
    uint32_t RefreshNetworkGraph();

    /**
     * Get zombie channels
     *
     * Channels with no updates for extended period
     *
     * @param max_age_days Maximum age in days
     * @return Vector of zombie channels
     */
    std::vector<ChannelInfo> GetZombieChannels(uint32_t max_age_days = 14) const;

    /**
     * Analyze routing position
     *
     * Analyze node's position for routing
     *
     * @param pubkey Node public key
     * @return Analysis results
     */
    struct RoutingAnalysis {
        double centrality_score{0.0};
        uint32_t potential_routes{0};
        double avg_route_length{0.0};
        uint64_t reachable_capacity{0};
    };

    RoutingAnalysis AnalyzeRoutingPosition(const std::string& pubkey) const;

    /**
     * Get channel updates
     *
     * Recent channel policy updates
     *
     * @param since_timestamp Unix timestamp
     * @param limit Maximum updates
     * @return Vector of updated channels
     */
    std::vector<ChannelInfo> GetChannelUpdates(
        uint64_t since_timestamp,
        uint32_t limit = 100
    ) const;

    /**
     * Get new nodes
     *
     * Recently announced nodes
     *
     * @param since_timestamp Unix timestamp
     * @param limit Maximum nodes
     * @return Vector of new nodes
     */
    std::vector<NodeInfo> GetNewNodes(
        uint64_t since_timestamp,
        uint32_t limit = 100
    ) const;

    /**
     * Export node data
     *
     * @param pubkey Node public key
     * @param format Format (json, csv)
     * @return Exported data
     */
    std::string ExportNodeData(
        const std::string& pubkey,
        const std::string& format = "json"
    ) const;

    /**
     * Export channel data
     *
     * @param channel_id Channel ID
     * @param format Format (json, csv)
     * @return Exported data
     */
    std::string ExportChannelData(
        const std::string& channel_id,
        const std::string& format = "json"
    ) const;

    /**
     * Get graph snapshot
     *
     * @return Timestamp of current graph
     */
    uint64_t GetGraphTimestamp() const;

    /**
     * Clear graph cache
     */
    void ClearCache();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get node ranking name
 */
std::string GetNodeRankingName(NodeRanking ranking);

/**
 * Parse node ranking from string
 */
NodeRanking ParseNodeRanking(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_NETWORK_EXPLORER_H
