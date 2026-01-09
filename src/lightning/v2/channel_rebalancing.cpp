// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/channel_rebalancing.h>
#include <intcoin/lightning/v2/routing.h>
#include <intcoin/lightning/v2/network_explorer.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <set>
#include <functional>
#include <chrono>
#include <cmath>
#include <random>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetRebalanceStrategyName(RebalanceStrategy strategy) {
    switch (strategy) {
        case RebalanceStrategy::MANUAL: return "MANUAL";
        case RebalanceStrategy::AUTO_BALANCED: return "AUTO_BALANCED";
        case RebalanceStrategy::AUTO_OPTIMIZED: return "AUTO_OPTIMIZED";
        case RebalanceStrategy::LIQUIDITY_PROVIDER: return "LIQUIDITY_PROVIDER";
        case RebalanceStrategy::ROUTING_NODE: return "ROUTING_NODE";
        case RebalanceStrategy::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

RebalanceStrategy ParseRebalanceStrategy(const std::string& str) {
    if (str == "MANUAL") return RebalanceStrategy::MANUAL;
    if (str == "AUTO_BALANCED") return RebalanceStrategy::AUTO_BALANCED;
    if (str == "AUTO_OPTIMIZED") return RebalanceStrategy::AUTO_OPTIMIZED;
    if (str == "LIQUIDITY_PROVIDER") return RebalanceStrategy::LIQUIDITY_PROVIDER;
    if (str == "ROUTING_NODE") return RebalanceStrategy::ROUTING_NODE;
    if (str == "CUSTOM") return RebalanceStrategy::CUSTOM;
    return RebalanceStrategy::MANUAL;
}

std::string GetRebalanceMethodName(RebalanceMethod method) {
    switch (method) {
        case RebalanceMethod::CIRCULAR: return "CIRCULAR";
        case RebalanceMethod::SWAP: return "SWAP";
        case RebalanceMethod::DUAL_FUNDING: return "DUAL_FUNDING";
        case RebalanceMethod::SPLICE: return "SPLICE";
        default: return "UNKNOWN";
    }
}

RebalanceMethod ParseRebalanceMethod(const std::string& str) {
    if (str == "CIRCULAR") return RebalanceMethod::CIRCULAR;
    if (str == "SWAP") return RebalanceMethod::SWAP;
    if (str == "DUAL_FUNDING") return RebalanceMethod::DUAL_FUNDING;
    if (str == "SPLICE") return RebalanceMethod::SPLICE;
    return RebalanceMethod::CIRCULAR;
}

std::string GetRebalanceStatusName(RebalanceStatus status) {
    switch (status) {
        case RebalanceStatus::PENDING: return "PENDING";
        case RebalanceStatus::IN_PROGRESS: return "IN_PROGRESS";
        case RebalanceStatus::COMPLETED: return "COMPLETED";
        case RebalanceStatus::FAILED: return "FAILED";
        case RebalanceStatus::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

RebalanceStatus ParseRebalanceStatus(const std::string& str) {
    if (str == "PENDING") return RebalanceStatus::PENDING;
    if (str == "IN_PROGRESS") return RebalanceStatus::IN_PROGRESS;
    if (str == "COMPLETED") return RebalanceStatus::COMPLETED;
    if (str == "FAILED") return RebalanceStatus::FAILED;
    if (str == "CANCELLED") return RebalanceStatus::CANCELLED;
    return RebalanceStatus::PENDING;
}

// Pimpl implementation
class ChannelRebalancingManager::Impl {
public:
    Config config_;
    std::vector<RebalanceOperation> operations_;
    std::vector<ChannelBalance> balances_;
    std::map<std::string, RebalanceTarget> targets_;
    Statistics stats_;
    std::unique_ptr<RoutingManager> routing_manager_;
    std::unique_ptr<NetworkExplorer> network_explorer_;

    Impl() : config_(),
             routing_manager_(std::make_unique<RoutingManager>()),
             network_explorer_(std::make_unique<NetworkExplorer>()) {}

    explicit Impl(const Config& config) : config_(config),
             routing_manager_(std::make_unique<RoutingManager>()),
             network_explorer_(std::make_unique<NetworkExplorer>()) {}

    // Helper methods
    std::string GetPeerPubkey(const std::string& channel_id) const;
    std::vector<std::string> GetChannelNeighbors(const std::string& node_pubkey) const;
    bool IsViableRoute(const std::vector<std::string>& path, uint64_t amount) const;
    double CalculateImbalanceScore(const ChannelBalance& balance, double target_ratio) const;
    RebalanceRecommendation CreateRecommendation(
        const std::string& source,
        const std::string& dest,
        uint64_t amount,
        const std::string& reason
    ) const;
};

ChannelRebalancingManager::ChannelRebalancingManager()
    : pimpl_(std::make_unique<Impl>()) {}

ChannelRebalancingManager::ChannelRebalancingManager(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

ChannelRebalancingManager::~ChannelRebalancingManager() = default;

std::vector<ChannelBalance> ChannelRebalancingManager::GetChannelBalances() const {
    return pimpl_->balances_;
}

ChannelBalance ChannelRebalancingManager::GetChannelBalance(const std::string& channel_id) const {
    auto it = std::find_if(pimpl_->balances_.begin(), pimpl_->balances_.end(),
        [&channel_id](const ChannelBalance& b) { return b.channel_id == channel_id; });
    if (it != pimpl_->balances_.end()) {
        return *it;
    }
    return ChannelBalance{};
}

std::string ChannelRebalancingManager::RebalanceChannel(
    const std::string& source_channel,
    const std::string& dest_channel,
    uint64_t amount,
    uint64_t max_fee
) {
    RebalanceOperation op;
    op.rebalance_id = "rebalance_" + std::to_string(pimpl_->operations_.size());
    op.source_channel = source_channel;
    op.dest_channel = dest_channel;
    op.amount = amount;
    op.max_fee = max_fee > 0 ? max_fee : pimpl_->config_.max_fee_per_rebalance;
    op.method = pimpl_->config_.preferred_method;
    op.status = RebalanceStatus::PENDING;

    pimpl_->operations_.push_back(op);
    pimpl_->stats_.total_rebalances++;

    return op.rebalance_id;
}

uint32_t ChannelRebalancingManager::AutoRebalance() {
    if (!pimpl_->config_.auto_rebalance) {
        return 0; // Auto-rebalancing disabled
    }

    // Check if enough time has passed since last rebalance
    uint64_t now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    uint64_t interval_seconds = pimpl_->config_.rebalance_interval_hours * 3600;

    if (pimpl_->stats_.last_rebalance_time > 0 &&
        (now - pimpl_->stats_.last_rebalance_time) < interval_seconds) {
        return 0; // Too soon since last rebalance
    }

    // Get rebalancing recommendations
    uint32_t max_concurrent = 5; // Maximum concurrent rebalances
    auto recommendations = GetRecommendations(max_concurrent);

    if (recommendations.empty()) {
        return 0; // No rebalancing needed
    }

    uint32_t initiated = 0;

    // Execute rebalancing based on strategy
    switch (pimpl_->config_.strategy) {
        case RebalanceStrategy::MANUAL:
            // Manual mode - don't auto-execute
            return 0;

        case RebalanceStrategy::AUTO_BALANCED:
            // Execute all recommendations to reach 50/50 balance
            for (const auto& rec : recommendations) {
                std::string op_id = RebalanceChannel(
                    rec.source_channel,
                    rec.dest_channel,
                    rec.recommended_amount,
                    rec.estimated_fee + (rec.estimated_fee / 10) // +10% fee buffer
                );
                if (!op_id.empty()) {
                    initiated++;
                }
            }
            break;

        case RebalanceStrategy::AUTO_OPTIMIZED:
            // Execute only high-priority recommendations
            for (const auto& rec : recommendations) {
                // Only execute if priority score > 0.6
                if (rec.priority_score > 0.6) {
                    std::string op_id = RebalanceChannel(
                        rec.source_channel,
                        rec.dest_channel,
                        rec.recommended_amount,
                        rec.estimated_fee + (rec.estimated_fee / 10)
                    );
                    if (!op_id.empty()) {
                        initiated++;
                    }
                }
            }
            break;

        case RebalanceStrategy::LIQUIDITY_PROVIDER:
            // Prioritize inbound liquidity (low local balance)
            for (const auto& rec : recommendations) {
                auto dest_balance = GetChannelBalance(rec.dest_channel);
                // Only rebalance if dest channel has low local balance (<30%)
                if (dest_balance.local_ratio < 0.3) {
                    std::string op_id = RebalanceChannel(
                        rec.source_channel,
                        rec.dest_channel,
                        rec.recommended_amount,
                        rec.estimated_fee + (rec.estimated_fee / 10)
                    );
                    if (!op_id.empty()) {
                        initiated++;
                    }
                }
            }
            break;

        case RebalanceStrategy::ROUTING_NODE:
            // Maintain balanced channels for optimal routing
            for (const auto& rec : recommendations) {
                auto source_balance = GetChannelBalance(rec.source_channel);
                auto dest_balance = GetChannelBalance(rec.dest_channel);

                // Only rebalance if both channels are significantly imbalanced
                double source_dev = std::abs(source_balance.local_ratio - 0.5);
                double dest_dev = std::abs(dest_balance.local_ratio - 0.5);

                if (source_dev > 0.2 && dest_dev > 0.2) {
                    std::string op_id = RebalanceChannel(
                        rec.source_channel,
                        rec.dest_channel,
                        rec.recommended_amount,
                        rec.estimated_fee + (rec.estimated_fee / 10)
                    );
                    if (!op_id.empty()) {
                        initiated++;
                    }
                }
            }
            break;

        case RebalanceStrategy::CUSTOM:
            // Execute based on custom channel targets
            for (const auto& rec : recommendations) {
                // Check if either channel has a custom target
                bool has_custom_target =
                    (pimpl_->targets_.find(rec.source_channel) != pimpl_->targets_.end()) ||
                    (pimpl_->targets_.find(rec.dest_channel) != pimpl_->targets_.end());

                if (has_custom_target) {
                    std::string op_id = RebalanceChannel(
                        rec.source_channel,
                        rec.dest_channel,
                        rec.recommended_amount,
                        rec.estimated_fee + (rec.estimated_fee / 10)
                    );
                    if (!op_id.empty()) {
                        initiated++;
                    }
                }
            }
            break;
    }

    // Update last rebalance time
    pimpl_->stats_.last_rebalance_time = now;

    return initiated;
}

std::vector<RebalanceRecommendation> ChannelRebalancingManager::GetRecommendations(
    uint32_t limit
) const {
    std::vector<RebalanceRecommendation> recommendations;

    // Analyze all channel pairs to find rebalancing opportunities
    const auto& balances = pimpl_->balances_;

    // Find channels that need rebalancing
    std::vector<std::pair<std::string, double>> over_balanced;  // channel_id, excess_ratio
    std::vector<std::pair<std::string, double>> under_balanced; // channel_id, deficit_ratio

    for (const auto& balance : balances) {
        if (!balance.active) continue;

        // Get target ratio for this channel
        double target_ratio = pimpl_->config_.target_local_ratio;
        auto target_it = pimpl_->targets_.find(balance.channel_id);
        if (target_it != pimpl_->targets_.end()) {
            target_ratio = target_it->second.target_local_ratio;
        }

        // Calculate deviation from target
        double deviation = balance.local_ratio - target_ratio;
        double abs_deviation = std::abs(deviation);

        // Significant imbalance threshold (>10% deviation)
        if (abs_deviation > 0.1) {
            if (deviation > 0) {
                // Too much local balance (over-balanced)
                over_balanced.push_back({balance.channel_id, abs_deviation});
            } else {
                // Too little local balance (under-balanced)
                under_balanced.push_back({balance.channel_id, abs_deviation});
            }
        }
    }

    // Sort by severity (highest deviation first)
    std::sort(over_balanced.begin(), over_balanced.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    std::sort(under_balanced.begin(), under_balanced.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Create recommendations by pairing over-balanced with under-balanced channels
    for (const auto& [source_id, source_deviation] : over_balanced) {
        for (const auto& [dest_id, dest_deviation] : under_balanced) {
            if (recommendations.size() >= limit) {
                break;
            }

            // Calculate optimal rebalance amount
            uint64_t amount = CalculateOptimalAmount(source_id, dest_id);

            // Skip if amount is too small
            if (amount < pimpl_->config_.min_rebalance_amount) {
                continue;
            }

            // Check if circular route exists
            auto route = FindCircularRoute(source_id, dest_id, amount);
            if (route.empty()) {
                continue; // No viable route found
            }

            // Create recommendation
            std::ostringstream reason;
            reason << "Channel " << source_id.substr(0, 8) << "... is "
                   << std::fixed << std::setprecision(1)
                   << (source_deviation * 100) << "% over-balanced, "
                   << "channel " << dest_id.substr(0, 8) << "... is "
                   << (dest_deviation * 100) << "% under-balanced";

            auto rec = pimpl_->CreateRecommendation(
                source_id, dest_id, amount, reason.str());

            recommendations.push_back(rec);
        }

        if (recommendations.size() >= limit) {
            break;
        }
    }

    // Sort recommendations by priority score (highest first)
    std::sort(recommendations.begin(), recommendations.end(),
        [](const auto& a, const auto& b) { return a.priority_score > b.priority_score; });

    // Limit results
    if (recommendations.size() > limit) {
        recommendations.resize(limit);
    }

    return recommendations;
}

void ChannelRebalancingManager::SetChannelTarget(
    const std::string& channel_id,
    const RebalanceTarget& target
) {
    pimpl_->targets_[channel_id] = target;
}

RebalanceTarget ChannelRebalancingManager::GetChannelTarget(const std::string& channel_id) const {
    auto it = pimpl_->targets_.find(channel_id);
    if (it != pimpl_->targets_.end()) {
        return it->second;
    }
    return RebalanceTarget{};
}

void ChannelRebalancingManager::RemoveChannelTarget(const std::string& channel_id) {
    pimpl_->targets_.erase(channel_id);
}

std::vector<RebalanceOperation> ChannelRebalancingManager::GetActiveOperations() const {
    std::vector<RebalanceOperation> active;
    for (const auto& op : pimpl_->operations_) {
        if (op.status == RebalanceStatus::PENDING || op.status == RebalanceStatus::IN_PROGRESS) {
            active.push_back(op);
        }
    }
    return active;
}

RebalanceOperation ChannelRebalancingManager::GetOperation(const std::string& rebalance_id) const {
    auto it = std::find_if(pimpl_->operations_.begin(), pimpl_->operations_.end(),
        [&rebalance_id](const RebalanceOperation& op) { return op.rebalance_id == rebalance_id; });
    if (it != pimpl_->operations_.end()) {
        return *it;
    }
    return RebalanceOperation{};
}

bool ChannelRebalancingManager::CancelOperation(const std::string& rebalance_id) {
    auto it = std::find_if(pimpl_->operations_.begin(), pimpl_->operations_.end(),
        [&rebalance_id](const RebalanceOperation& op) { return op.rebalance_id == rebalance_id; });
    if (it != pimpl_->operations_.end() && it->status == RebalanceStatus::PENDING) {
        it->status = RebalanceStatus::CANCELLED;
        return true;
    }
    return false;
}

std::vector<RebalanceOperation> ChannelRebalancingManager::GetHistory(uint32_t limit) const {
    std::vector<RebalanceOperation> history;
    uint32_t count = 0;
    for (auto it = pimpl_->operations_.rbegin();
         it != pimpl_->operations_.rend() && count < limit;
         ++it, ++count) {
        history.push_back(*it);
    }
    return history;
}

void ChannelRebalancingManager::ClearHistory() {
    pimpl_->operations_.clear();
}

void ChannelRebalancingManager::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

ChannelRebalancingManager::Config ChannelRebalancingManager::GetConfig() const {
    return pimpl_->config_;
}

void ChannelRebalancingManager::SetAutoRebalance(bool enabled) {
    pimpl_->config_.auto_rebalance = enabled;
}

bool ChannelRebalancingManager::IsAutoRebalanceEnabled() const {
    return pimpl_->config_.auto_rebalance;
}

uint64_t ChannelRebalancingManager::EstimateFee(
    const std::string& source_channel,
    const std::string& dest_channel,
    uint64_t amount,
    RebalanceMethod method
) const {
    (void)source_channel;
    (void)dest_channel;
    (void)method;
    return amount / 1000;  // 0.1% fee estimate
}

std::vector<std::string> ChannelRebalancingManager::FindCircularRoute(
    const std::string& source_channel,
    const std::string& dest_channel,
    uint64_t amount
) const {
    // Get the peer nodes for source and dest channels
    std::string source_peer = pimpl_->GetPeerPubkey(source_channel);
    std::string dest_peer = pimpl_->GetPeerPubkey(dest_channel);

    if (source_peer.empty() || dest_peer.empty()) {
        return {};
    }

    // Find circular route: our_node -> source_peer -> ... -> dest_peer -> our_node
    std::vector<std::string> best_route;
    uint64_t best_fee = UINT64_MAX;
    const uint32_t max_hops = 10;

    // DFS to find circular routes
    std::function<void(const std::string&, std::vector<std::string>&, std::set<std::string>&, uint32_t)> dfs;

    dfs = [&](const std::string& current_node,
              std::vector<std::string>& path,
              std::set<std::string>& visited,
              uint32_t remaining_hops) {
        // If we reached dest_peer and have enough hops, check if valid
        if (current_node == dest_peer && path.size() >= 3) {
            // Complete the circle back to our node
            std::vector<std::string> complete_path = path;
            // Note: In real implementation, "our_node" would come from node config
            // For now, we return the path through the network

            // Verify route viability
            if (pimpl_->IsViableRoute(complete_path, amount)) {
                // Calculate fee for this route
                try {
                    auto route = pimpl_->routing_manager_->BuildRoute(complete_path, amount);
                    if (route.total_fee_msat < best_fee) {
                        best_fee = route.total_fee_msat;
                        best_route = complete_path;
                    }
                } catch (...) {
                    // Route building failed, skip this route
                }
            }
            return;
        }

        // Explore neighbors if we have hops remaining
        if (remaining_hops == 0) return;

        auto neighbors = pimpl_->GetChannelNeighbors(current_node);
        for (const auto& neighbor : neighbors) {
            // Avoid revisiting nodes (except destination for circular routes)
            if (visited.find(neighbor) != visited.end()) {
                continue;
            }

            // Add to path and explore
            path.push_back(neighbor);
            visited.insert(neighbor);

            dfs(neighbor, path, visited, remaining_hops - 1);

            // Backtrack
            path.pop_back();
            visited.erase(neighbor);
        }
    };

    // Start DFS from source peer
    std::vector<std::string> initial_path = {source_peer};
    std::set<std::string> visited = {source_peer};
    dfs(source_peer, initial_path, visited, max_hops);

    return best_route;
}

uint64_t ChannelRebalancingManager::CalculateOptimalAmount(
    const std::string& source_channel,
    const std::string& dest_channel
) const {
    // Get channel balances
    auto source_balance = GetChannelBalance(source_channel);
    auto dest_balance = GetChannelBalance(dest_channel);

    if (source_balance.capacity == 0 || dest_balance.capacity == 0) {
        return pimpl_->config_.min_rebalance_amount;
    }

    // Get target ratios (use custom targets if set, otherwise use config)
    double source_target = pimpl_->config_.target_local_ratio;
    double dest_target = pimpl_->config_.target_local_ratio;

    auto source_target_it = pimpl_->targets_.find(source_channel);
    if (source_target_it != pimpl_->targets_.end()) {
        source_target = source_target_it->second.target_local_ratio;
    }

    auto dest_target_it = pimpl_->targets_.find(dest_channel);
    if (dest_target_it != pimpl_->targets_.end()) {
        dest_target = dest_target_it->second.target_local_ratio;
    }

    // Calculate how much to move from source to reach target
    uint64_t source_target_balance = static_cast<uint64_t>(
        source_balance.capacity * source_target);
    int64_t source_excess = static_cast<int64_t>(source_balance.local_balance) -
                            static_cast<int64_t>(source_target_balance);

    // Calculate how much dest needs to reach target
    uint64_t dest_target_balance = static_cast<uint64_t>(
        dest_balance.capacity * dest_target);
    int64_t dest_deficit = static_cast<int64_t>(dest_target_balance) -
                           static_cast<int64_t>(dest_balance.local_balance);

    // Optimal amount is minimum of source excess and dest deficit
    // (can't move more than source has, or more than dest needs)
    uint64_t optimal_amount = 0;

    if (source_excess > 0 && dest_deficit > 0) {
        optimal_amount = std::min(
            static_cast<uint64_t>(source_excess),
            static_cast<uint64_t>(dest_deficit)
        );
    } else {
        // One channel doesn't need rebalancing, use minimum
        return pimpl_->config_.min_rebalance_amount;
    }

    // Clamp to configured limits
    optimal_amount = std::max(optimal_amount, pimpl_->config_.min_rebalance_amount);
    optimal_amount = std::min(optimal_amount, pimpl_->config_.max_rebalance_amount);

    // Ensure we don't exceed source channel's local balance
    optimal_amount = std::min(optimal_amount, source_balance.local_balance);

    // Ensure we don't exceed dest channel's remote balance
    optimal_amount = std::min(optimal_amount, dest_balance.remote_balance);

    return optimal_amount;
}

ChannelRebalancingManager::Statistics ChannelRebalancingManager::GetStatistics() const {
    return pimpl_->stats_;
}

// Helper method implementations
std::string ChannelRebalancingManager::Impl::GetPeerPubkey(const std::string& channel_id) const {
    // Find channel and return peer pubkey
    auto it = std::find_if(balances_.begin(), balances_.end(),
        [&channel_id](const ChannelBalance& b) { return b.channel_id == channel_id; });
    if (it != balances_.end()) {
        return it->peer_pubkey;
    }
    return "";
}

std::vector<std::string> ChannelRebalancingManager::Impl::GetChannelNeighbors(
    const std::string& node_pubkey
) const {
    std::vector<std::string> neighbors;
    // Query network explorer for node's channels
    try {
        auto node_info = network_explorer_->GetNode(node_pubkey);
        auto channels = network_explorer_->GetNodeChannels(node_pubkey);

        for (const auto& channel : channels) {
            // Add the peer node (the other end of the channel)
            if (channel.node1_pubkey == node_pubkey && !channel.node2_disabled) {
                neighbors.push_back(channel.node2_pubkey);
            } else if (channel.node2_pubkey == node_pubkey && !channel.node1_disabled) {
                neighbors.push_back(channel.node1_pubkey);
            }
        }
    } catch (...) {
        // Network explorer not available, use local channels only
        for (const auto& balance : balances_) {
            if (balance.active) {
                neighbors.push_back(balance.peer_pubkey);
            }
        }
    }
    return neighbors;
}

bool ChannelRebalancingManager::Impl::IsViableRoute(
    const std::vector<std::string>& path,
    uint64_t amount
) const {
    if (path.size() < 2) return false;

    // Check if route can handle the amount
    try {
        auto route = routing_manager_->BuildRoute(path, amount);

        // Verify route meets constraints
        uint64_t max_fee = static_cast<uint64_t>(amount * config_.max_fee_ratio);
        if (route.total_fee_msat > max_fee) {
            return false;
        }

        // Check success probability
        if (route.success_probability < 0.5) {
            return false;
        }

        return true;
    } catch (...) {
        return false;
    }
}

double ChannelRebalancingManager::Impl::CalculateImbalanceScore(
    const ChannelBalance& balance,
    double target_ratio
) const {
    // Calculate how far from target (0.0 = perfect, 1.0 = maximally imbalanced)
    double current_ratio = balance.local_ratio;
    double deviation = std::abs(current_ratio - target_ratio);

    // Weight by channel capacity (larger channels matter more)
    double capacity_weight = static_cast<double>(balance.capacity) /
                            std::max(1.0, static_cast<double>(balance.capacity));

    return deviation * capacity_weight;
}

RebalanceRecommendation ChannelRebalancingManager::Impl::CreateRecommendation(
    const std::string& source,
    const std::string& dest,
    uint64_t amount,
    const std::string& reason
) const {
    RebalanceRecommendation rec;
    rec.source_channel = source;
    rec.dest_channel = dest;
    rec.recommended_amount = amount;
    rec.reason = reason;

    // Estimate fee
    try {
        auto source_peer = GetPeerPubkey(source);
        auto dest_peer = GetPeerPubkey(dest);

        if (!source_peer.empty() && !dest_peer.empty()) {
            RouteConstraints constraints;
            constraints.max_fee_ratio = config_.max_fee_ratio;

            auto route = routing_manager_->FindRoute(
                source_peer, dest_peer, amount, constraints);
            rec.estimated_fee = route.total_fee_msat;
        } else {
            rec.estimated_fee = amount / 1000; // 0.1% estimate
        }
    } catch (...) {
        rec.estimated_fee = amount / 1000;
    }

    // Calculate priority score based on imbalance severity
    auto source_balance = std::find_if(balances_.begin(), balances_.end(),
        [&source](const ChannelBalance& b) { return b.channel_id == source; });
    auto dest_balance = std::find_if(balances_.begin(), balances_.end(),
        [&dest](const ChannelBalance& b) { return b.channel_id == dest; });

    if (source_balance != balances_.end() && dest_balance != balances_.end()) {
        double source_score = CalculateImbalanceScore(*source_balance, config_.target_local_ratio);
        double dest_score = CalculateImbalanceScore(*dest_balance, config_.target_local_ratio);
        rec.priority_score = (source_score + dest_score) / 2.0;
    } else {
        rec.priority_score = 0.5;
    }

    return rec;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
