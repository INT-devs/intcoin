// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/network_explorer.h>
#include <sstream>
#include <algorithm>
#include <map>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetNodeRankingName(NodeRanking ranking) {
    switch (ranking) {
        case NodeRanking::BY_CAPACITY: return "BY_CAPACITY";
        case NodeRanking::BY_CHANNELS: return "BY_CHANNELS";
        case NodeRanking::BY_CENTRALITY: return "BY_CENTRALITY";
        case NodeRanking::BY_UPTIME: return "BY_UPTIME";
        case NodeRanking::BY_AGE: return "BY_AGE";
        default: return "UNKNOWN";
    }
}

NodeRanking ParseNodeRanking(const std::string& name) {
    if (name == "BY_CAPACITY") return NodeRanking::BY_CAPACITY;
    if (name == "BY_CHANNELS") return NodeRanking::BY_CHANNELS;
    if (name == "BY_CENTRALITY") return NodeRanking::BY_CENTRALITY;
    if (name == "BY_UPTIME") return NodeRanking::BY_UPTIME;
    if (name == "BY_AGE") return NodeRanking::BY_AGE;
    return NodeRanking::BY_CAPACITY;
}

// Pimpl implementation
class NetworkExplorer::Impl {
public:
    std::map<std::string, NodeInfo> nodes_;
    std::map<std::string, ChannelInfo> channels_;
    uint64_t graph_timestamp_{0};

    Impl() = default;
};

NetworkExplorer::NetworkExplorer()
    : pimpl_(std::make_unique<Impl>()) {}

NetworkExplorer::~NetworkExplorer() = default;

NetworkStats NetworkExplorer::GetNetworkStats() const {
    NetworkStats stats;
    stats.num_nodes = static_cast<uint32_t>(pimpl_->nodes_.size());
    stats.num_channels = static_cast<uint32_t>(pimpl_->channels_.size());

    // Calculate capacity
    uint64_t total = 0;
    for (const auto& [id, channel] : pimpl_->channels_) {
        total += channel.capacity;
    }
    stats.total_capacity = total;

    if (stats.num_channels > 0) {
        stats.avg_channel_size = total / stats.num_channels;
        stats.median_channel_size = stats.avg_channel_size;  // Stub
    }

    return stats;
}

NodeInfo NetworkExplorer::GetNode(const std::string& pubkey) const {
    auto it = pimpl_->nodes_.find(pubkey);
    if (it != pimpl_->nodes_.end()) {
        return it->second;
    }
    return NodeInfo{};
}

ChannelInfo NetworkExplorer::GetChannel(const std::string& channel_id) const {
    auto it = pimpl_->channels_.find(channel_id);
    if (it != pimpl_->channels_.end()) {
        return it->second;
    }
    return ChannelInfo{};
}

std::vector<NodeInfo> NetworkExplorer::GetAllNodes() const {
    std::vector<NodeInfo> nodes;
    for (const auto& [pubkey, node] : pimpl_->nodes_) {
        nodes.push_back(node);
    }
    return nodes;
}

std::vector<ChannelInfo> NetworkExplorer::GetAllChannels(
    const ChannelFilter& filter
) const {
    std::vector<ChannelInfo> channels;

    for (const auto& [id, channel] : pimpl_->channels_) {
        // Apply filters
        if (filter.min_capacity > 0 && channel.capacity < filter.min_capacity) {
            continue;
        }
        if (filter.max_capacity > 0 && channel.capacity > filter.max_capacity) {
            continue;
        }
        if (filter.exclude_disabled && (channel.node1_disabled || channel.node2_disabled)) {
            continue;
        }

        channels.push_back(channel);
    }

    return channels;
}

std::vector<ChannelInfo> NetworkExplorer::GetNodeChannels(const std::string& pubkey) const {
    std::vector<ChannelInfo> channels;

    for (const auto& [id, channel] : pimpl_->channels_) {
        if (channel.node1_pubkey == pubkey || channel.node2_pubkey == pubkey) {
            channels.push_back(channel);
        }
    }

    return channels;
}

std::vector<std::string> NetworkExplorer::GetNodePeers(const std::string& pubkey) const {
    std::vector<std::string> peers;

    for (const auto& [id, channel] : pimpl_->channels_) {
        if (channel.node1_pubkey == pubkey) {
            peers.push_back(channel.node2_pubkey);
        } else if (channel.node2_pubkey == pubkey) {
            peers.push_back(channel.node1_pubkey);
        }
    }

    return peers;
}

std::vector<NodeInfo> NetworkExplorer::SearchNodes(
    const std::string& query,
    uint32_t limit
) const {
    std::vector<NodeInfo> results;
    uint32_t count = 0;

    for (const auto& [pubkey, node] : pimpl_->nodes_) {
        if (count >= limit) break;

        if (node.alias.find(query) != std::string::npos ||
            node.pubkey.find(query) != std::string::npos) {
            results.push_back(node);
            count++;
        }
    }

    return results;
}

std::vector<NodeInfo> NetworkExplorer::GetTopNodes(
    NodeRanking ranking,
    uint32_t limit
) const {
    (void)ranking;

    std::vector<NodeInfo> nodes = GetAllNodes();

    // Sort by capacity (stub - should sort by ranking criteria)
    std::sort(nodes.begin(), nodes.end(),
        [](const NodeInfo& a, const NodeInfo& b) {
            return a.total_capacity > b.total_capacity;
        });

    if (nodes.size() > limit) {
        nodes.resize(limit);
    }

    return nodes;
}

std::vector<ChannelInfo> NetworkExplorer::GetLargestChannels(uint32_t limit) const {
    std::vector<ChannelInfo> channels = GetAllChannels();

    std::sort(channels.begin(), channels.end(),
        [](const ChannelInfo& a, const ChannelInfo& b) {
            return a.capacity > b.capacity;
        });

    if (channels.size() > limit) {
        channels.resize(limit);
    }

    return channels;
}

PathVisualization NetworkExplorer::FindPath(
    const std::string& source,
    const std::string& destination
) const {
    PathVisualization path;

    // Simple stub: direct path
    path.node_sequence = {source, destination};

    auto src_node = GetNode(source);
    auto dst_node = GetNode(destination);

    if (!src_node.pubkey.empty()) {
        path.nodes.push_back(src_node);
    }
    if (!dst_node.pubkey.empty()) {
        path.nodes.push_back(dst_node);
    }

    return path;
}

std::vector<NodeInfo> NetworkExplorer::GetNodeNeighbors(
    const std::string& pubkey,
    uint32_t max_hops
) const {
    (void)max_hops;

    std::vector<NodeInfo> neighbors;
    auto peers = GetNodePeers(pubkey);

    for (const auto& peer : peers) {
        auto node = GetNode(peer);
        if (!node.pubkey.empty()) {
            neighbors.push_back(node);
        }
    }

    return neighbors;
}

double NetworkExplorer::CalculateNodeCentrality(const std::string& pubkey) const {
    auto channels = GetNodeChannels(pubkey);
    auto total_nodes = pimpl_->nodes_.size();

    if (total_nodes <= 1) {
        return 0.0;
    }

    // Simple centrality: ratio of connections to total nodes
    return static_cast<double>(channels.size()) / static_cast<double>(total_nodes);
}

std::string NetworkExplorer::GetNetworkTopology(const std::string& format) const {
    (void)format;
    return "{}";  // Stub: empty JSON
}

uint32_t NetworkExplorer::RefreshNetworkGraph() {
    return 0;  // Stub
}

std::vector<ChannelInfo> NetworkExplorer::GetZombieChannels(uint32_t max_age_days) const {
    (void)max_age_days;
    return {};  // Stub
}

NetworkExplorer::RoutingAnalysis NetworkExplorer::AnalyzeRoutingPosition(
    const std::string& pubkey
) const {
    RoutingAnalysis analysis;
    analysis.centrality_score = CalculateNodeCentrality(pubkey);

    auto channels = GetNodeChannels(pubkey);
    analysis.potential_routes = static_cast<uint32_t>(channels.size());

    uint64_t total_capacity = 0;
    for (const auto& channel : channels) {
        total_capacity += channel.capacity;
    }
    analysis.reachable_capacity = total_capacity;

    return analysis;
}

std::vector<ChannelInfo> NetworkExplorer::GetChannelUpdates(
    uint64_t since_timestamp,
    uint32_t limit
) const {
    std::vector<ChannelInfo> updates;
    uint32_t count = 0;

    for (const auto& [id, channel] : pimpl_->channels_) {
        if (count >= limit) break;

        if (channel.last_update >= since_timestamp) {
            updates.push_back(channel);
            count++;
        }
    }

    return updates;
}

std::vector<NodeInfo> NetworkExplorer::GetNewNodes(
    uint64_t since_timestamp,
    uint32_t limit
) const {
    std::vector<NodeInfo> new_nodes;
    uint32_t count = 0;

    for (const auto& [pubkey, node] : pimpl_->nodes_) {
        if (count >= limit) break;

        if (node.last_update >= since_timestamp) {
            new_nodes.push_back(node);
            count++;
        }
    }

    return new_nodes;
}

std::string NetworkExplorer::ExportNodeData(
    const std::string& pubkey,
    const std::string& format
) const {
    (void)pubkey;
    (void)format;
    return "{}";  // Stub: empty JSON
}

std::string NetworkExplorer::ExportChannelData(
    const std::string& channel_id,
    const std::string& format
) const {
    (void)channel_id;
    (void)format;
    return "{}";  // Stub: empty JSON
}

uint64_t NetworkExplorer::GetGraphTimestamp() const {
    return pimpl_->graph_timestamp_;
}

void NetworkExplorer::ClearCache() {
    pimpl_->nodes_.clear();
    pimpl_->channels_.clear();
    pimpl_->graph_timestamp_ = 0;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
