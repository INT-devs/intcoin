// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/channel_rebalancing.h>
#include <sstream>
#include <algorithm>
#include <map>

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

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}
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
    return 0;  // Stub
}

std::vector<RebalanceRecommendation> ChannelRebalancingManager::GetRecommendations(
    uint32_t limit
) const {
    (void)limit;
    return {};  // Stub
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
    (void)amount;
    return {source_channel, dest_channel};  // Simple stub
}

uint64_t ChannelRebalancingManager::CalculateOptimalAmount(
    const std::string& source_channel,
    const std::string& dest_channel
) const {
    (void)source_channel;
    (void)dest_channel;
    return pimpl_->config_.min_rebalance_amount;
}

ChannelRebalancingManager::Statistics ChannelRebalancingManager::GetStatistics() const {
    return pimpl_->stats_;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
