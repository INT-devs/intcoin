// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/mempool_analytics/analytics.h>
#include <mutex>
#include <deque>
#include <chrono>
#include <sstream>

namespace intcoin {
namespace mempool_analytics {

class MempoolAnalytics::Impl {
public:
    mutable std::mutex mutex_;
    MempoolStats current_stats_;
    std::deque<MempoolSnapshot> history_;

    // Flow tracking
    uint64_t transactions_added_{0};
    uint64_t transactions_removed_{0};
    uint64_t transactions_rejected_{0};
    uint64_t transactions_evicted_{0};
    uint64_t last_flow_update_{0};

    // Flow window counters (reset periodically)
    uint64_t transactions_added_window_{0};
    uint64_t transactions_removed_window_{0};
    double current_flow_inflow_{0.0};
    double current_flow_outflow_{0.0};

    static constexpr size_t MAX_HISTORY_SIZE = 10000;
    static constexpr uint64_t SNAPSHOT_INTERVAL_SEC = 60;

    Impl() {
        current_stats_.timestamp = GetCurrentTimestamp();
        last_flow_update_ = current_stats_.timestamp;
    }

    static uint64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    void UpdateFlowMetrics() {
        uint64_t now = GetCurrentTimestamp();
        uint64_t elapsed = now - last_flow_update_;

        if (elapsed > 0) {
            // Calculate rates before resetting
            current_flow_inflow_ = static_cast<double>(transactions_added_window_) / std::max(elapsed, 1ULL);
            current_flow_outflow_ = static_cast<double>(transactions_removed_window_) / std::max(elapsed, 1ULL);

            // Reset window counters
            transactions_added_window_ = 0;
            transactions_removed_window_ = 0;
            last_flow_update_ = now;
        }
    }
};

MempoolAnalytics::MempoolAnalytics()
    : pimpl_(std::make_unique<Impl>()) {}

MempoolAnalytics::~MempoolAnalytics() = default;

MempoolStats MempoolAnalytics::GetCurrentStats() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->current_stats_;
}

std::vector<MempoolSnapshot> MempoolAnalytics::GetHistory(
    uint64_t start_time,
    uint64_t end_time
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::vector<MempoolSnapshot> result;

    for (const auto& snapshot : pimpl_->history_) {
        if (snapshot.timestamp >= start_time && snapshot.timestamp <= end_time) {
            result.push_back(snapshot);
        }
    }

    return result;
}

FlowMetrics MempoolAnalytics::AnalyzeTransactionFlow() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    FlowMetrics metrics;

    // Use the current flow rates calculated by UpdateFlowMetrics
    // If we have window activity, use that for more recent rates
    uint64_t now = Impl::GetCurrentTimestamp();
    uint64_t elapsed = now - pimpl_->last_flow_update_;

    if (pimpl_->transactions_added_window_ > 0 || pimpl_->transactions_removed_window_ > 0) {
        // Use recent window data
        metrics.inflow_rate = static_cast<double>(pimpl_->transactions_added_window_) / std::max(elapsed, 1ULL);
        metrics.outflow_rate = static_cast<double>(pimpl_->transactions_removed_window_) / std::max(elapsed, 1ULL);
    } else {
        // Fallback to stored flow rates
        metrics.inflow_rate = pimpl_->current_flow_inflow_;
        metrics.outflow_rate = pimpl_->current_flow_outflow_;
    }

    // Calculate acceptance/rejection rates
    uint64_t total_tx = pimpl_->transactions_added_ + pimpl_->transactions_rejected_;
    if (total_tx > 0) {
        metrics.acceptance_rate = static_cast<double>(pimpl_->transactions_added_) / total_tx;
        metrics.rejection_rate = static_cast<double>(pimpl_->transactions_rejected_) / total_tx;
    }

    if (pimpl_->transactions_removed_ > 0) {
        metrics.eviction_rate = static_cast<double>(pimpl_->transactions_evicted_) /
                               pimpl_->transactions_removed_;
    }

    return metrics;
}

void MempoolAnalytics::TakeSnapshot() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    MempoolSnapshot snapshot;
    snapshot.timestamp = Impl::GetCurrentTimestamp();
    snapshot.stats = pimpl_->current_stats_;
    snapshot.stats.timestamp = snapshot.timestamp;

    pimpl_->history_.push_back(snapshot);

    // Prune old snapshots
    if (pimpl_->history_.size() > Impl::MAX_HISTORY_SIZE) {
        pimpl_->history_.pop_front();
    }
}

void MempoolAnalytics::OnTransactionAdded(
    uint64_t tx_size,
    double fee_rate,
    uint8_t priority
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    pimpl_->current_stats_.size++;
    pimpl_->current_stats_.bytes += tx_size;
    pimpl_->transactions_added_++;
    pimpl_->transactions_added_window_++;  // Track for flow window

    // Update priority distribution
    switch (priority) {
        case 0: pimpl_->current_stats_.priority_dist.low_count++; break;
        case 1: pimpl_->current_stats_.priority_dist.normal_count++; break;
        case 2: pimpl_->current_stats_.priority_dist.high_count++; break;
        case 3: pimpl_->current_stats_.priority_dist.htlc_count++; break;
        case 4: pimpl_->current_stats_.priority_dist.bridge_count++; break;
        case 5: pimpl_->current_stats_.priority_dist.critical_count++; break;
    }

    pimpl_->UpdateFlowMetrics();
}

void MempoolAnalytics::OnTransactionRemoved(
    uint64_t tx_size,
    double fee_rate,
    uint8_t priority
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->current_stats_.size > 0) {
        pimpl_->current_stats_.size--;
    }
    if (pimpl_->current_stats_.bytes >= tx_size) {
        pimpl_->current_stats_.bytes -= tx_size;
    }
    pimpl_->transactions_removed_++;
    pimpl_->transactions_removed_window_++;  // Track for flow window

    // Update priority distribution
    switch (priority) {
        case 0: if (pimpl_->current_stats_.priority_dist.low_count > 0)
                    pimpl_->current_stats_.priority_dist.low_count--; break;
        case 1: if (pimpl_->current_stats_.priority_dist.normal_count > 0)
                    pimpl_->current_stats_.priority_dist.normal_count--; break;
        case 2: if (pimpl_->current_stats_.priority_dist.high_count > 0)
                    pimpl_->current_stats_.priority_dist.high_count--; break;
        case 3: if (pimpl_->current_stats_.priority_dist.htlc_count > 0)
                    pimpl_->current_stats_.priority_dist.htlc_count--; break;
        case 4: if (pimpl_->current_stats_.priority_dist.bridge_count > 0)
                    pimpl_->current_stats_.priority_dist.bridge_count--; break;
        case 5: if (pimpl_->current_stats_.priority_dist.critical_count > 0)
                    pimpl_->current_stats_.priority_dist.critical_count--; break;
    }

    pimpl_->UpdateFlowMetrics();
}

void MempoolAnalytics::PruneHistory(uint64_t cutoff_time) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    auto it = pimpl_->history_.begin();
    while (it != pimpl_->history_.end() && it->timestamp < cutoff_time) {
        it = pimpl_->history_.erase(it);
    }
}

std::string MempoolAnalytics::ExportToJSON() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"current_stats\": {\n";
    oss << "    \"size\": " << pimpl_->current_stats_.size << ",\n";
    oss << "    \"bytes\": " << pimpl_->current_stats_.bytes << ",\n";
    oss << "    \"priority_distribution\": {\n";
    oss << "      \"low\": " << pimpl_->current_stats_.priority_dist.low_count << ",\n";
    oss << "      \"normal\": " << pimpl_->current_stats_.priority_dist.normal_count << ",\n";
    oss << "      \"high\": " << pimpl_->current_stats_.priority_dist.high_count << ",\n";
    oss << "      \"htlc\": " << pimpl_->current_stats_.priority_dist.htlc_count << ",\n";
    oss << "      \"bridge\": " << pimpl_->current_stats_.priority_dist.bridge_count << ",\n";
    oss << "      \"critical\": " << pimpl_->current_stats_.priority_dist.critical_count << "\n";
    oss << "    }\n";
    oss << "  },\n";
    oss << "  \"history_size\": " << pimpl_->history_.size() << "\n";
    oss << "}\n";

    return oss.str();
}

} // namespace mempool_analytics
} // namespace intcoin
