// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_MEMPOOL_ANALYTICS_ANALYTICS_H
#define INTCOIN_MEMPOOL_ANALYTICS_ANALYTICS_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace intcoin {
namespace mempool_analytics {

/**
 * Priority level distribution in mempool
 */
struct PriorityDistribution {
    uint64_t low_count{0};
    uint64_t normal_count{0};
    uint64_t high_count{0};
    uint64_t htlc_count{0};
    uint64_t bridge_count{0};
    uint64_t critical_count{0};

    uint64_t GetTotalCount() const {
        return low_count + normal_count + high_count +
               htlc_count + bridge_count + critical_count;
    }
};

/**
 * Real-time mempool statistics
 */
struct MempoolStats {
    uint64_t size{0};                    // Total transactions
    uint64_t bytes{0};                   // Total size in bytes
    uint64_t usage{0};                   // Memory usage
    double avg_fee_rate{0.0};            // Average fee rate (sat/byte)
    double median_fee_rate{0.0};         // Median fee rate
    PriorityDistribution priority_dist;  // Priority distribution
    uint64_t timestamp{0};               // Snapshot timestamp
};

/**
 * Historical mempool snapshot
 */
struct MempoolSnapshot {
    uint64_t timestamp{0};
    MempoolStats stats;
};

/**
 * Transaction flow metrics
 */
struct FlowMetrics {
    double inflow_rate{0.0};             // Transactions per second entering
    double outflow_rate{0.0};            // Transactions per second leaving
    double acceptance_rate{0.0};         // % of transactions accepted
    double rejection_rate{0.0};          // % of transactions rejected
    double eviction_rate{0.0};           // % of transactions evicted
    uint64_t avg_time_in_mempool{0};     // Average time in mempool (seconds)
};

/**
 * Mempool Analytics Engine
 *
 * Provides real-time analytics, historical tracking, and prediction
 * for the enhanced mempool system.
 */
class MempoolAnalytics {
public:
    MempoolAnalytics();
    ~MempoolAnalytics();

    /**
     * Get current mempool statistics
     */
    MempoolStats GetCurrentStats() const;

    /**
     * Get historical mempool snapshots
     *
     * @param start_time Unix timestamp to start from
     * @param end_time Unix timestamp to end at
     * @return Vector of snapshots in the time range
     */
    std::vector<MempoolSnapshot> GetHistory(
        uint64_t start_time,
        uint64_t end_time
    ) const;

    /**
     * Analyze transaction flow
     *
     * @return Current flow metrics
     */
    FlowMetrics AnalyzeTransactionFlow() const;

    /**
     * Take snapshot of current mempool state
     *
     * Stores a snapshot for historical analysis
     */
    void TakeSnapshot();

    /**
     * Update statistics with new transaction
     *
     * @param tx_size Transaction size in bytes
     * @param fee_rate Fee rate in ints per byte
     * @param priority Priority level (0-5)
     */
    void OnTransactionAdded(uint64_t tx_size, double fee_rate, uint8_t priority);

    /**
     * Update statistics when transaction is removed
     *
     * @param tx_size Transaction size in bytes
     * @param fee_rate Fee rate in ints per byte
     * @param priority Priority level (0-5)
     */
    void OnTransactionRemoved(uint64_t tx_size, double fee_rate, uint8_t priority);

    /**
     * Clear historical data older than specified time
     *
     * @param cutoff_time Unix timestamp cutoff
     */
    void PruneHistory(uint64_t cutoff_time);

    /**
     * Export analytics data to JSON
     *
     * @return JSON string with analytics data
     */
    std::string ExportToJSON() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace mempool_analytics
} // namespace intcoin

#endif // INTCOIN_MEMPOOL_ANALYTICS_ANALYTICS_H
