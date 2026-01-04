// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_CHANNEL_REBALANCING_H
#define INTCOIN_LIGHTNING_V2_CHANNEL_REBALANCING_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Rebalancing strategy
 */
enum class RebalanceStrategy {
    MANUAL,                          // Manual rebalancing only
    AUTO_BALANCED,                   // Auto-rebalance to 50/50
    AUTO_OPTIMIZED,                  // Auto-rebalance based on payment flow
    LIQUIDITY_PROVIDER,              // Maintain high inbound liquidity
    ROUTING_NODE,                    // Optimize for routing fees
    CUSTOM                           // Custom target ratios
};

/**
 * Rebalancing method
 */
enum class RebalanceMethod {
    CIRCULAR,                        // Circular rebalancing (self-payment)
    SWAP,                            // Use submarine swap
    DUAL_FUNDING,                    // Use dual-funded channels
    SPLICE                           // Use channel splicing
};

/**
 * Rebalance status
 */
enum class RebalanceStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    CANCELLED
};

/**
 * Channel balance info
 */
struct ChannelBalance {
    std::string channel_id;
    std::string peer_pubkey;
    std::string peer_alias;
    uint64_t local_balance{0};       // Our balance (can send)
    uint64_t remote_balance{0};      // Their balance (can receive)
    uint64_t capacity{0};            // Total capacity
    double local_ratio{0.0};         // Local / Capacity (0.0-1.0)
    double remote_ratio{0.0};        // Remote / Capacity (0.0-1.0)
    bool active{false};
};

/**
 * Rebalancing target
 */
struct RebalanceTarget {
    std::string channel_id;
    double target_local_ratio{0.5};  // Target local balance ratio
    uint64_t target_local_balance{0};
    uint64_t min_local_balance{0};
    uint64_t max_local_balance{0};
    uint32_t priority{5};            // 1-10 (10 = highest)
};

/**
 * Rebalancing operation
 */
struct RebalanceOperation {
    std::string rebalance_id;
    std::string source_channel;      // Channel to decrease local balance
    std::string dest_channel;        // Channel to increase local balance
    uint64_t amount{0};
    uint64_t fee{0};
    uint64_t max_fee{0};
    RebalanceMethod method{RebalanceMethod::CIRCULAR};
    RebalanceStatus status{RebalanceStatus::PENDING};
    std::string payment_hash;
    std::vector<std::string> route;  // Node pubkeys in route
    uint64_t started_at{0};
    uint64_t completed_at{0};
    std::string error_message;
};

/**
 * Rebalancing recommendation
 */
struct RebalanceRecommendation {
    std::string source_channel;
    std::string dest_channel;
    uint64_t recommended_amount{0};
    uint64_t estimated_fee{0};
    double priority_score{0.0};      // 0.0-1.0
    std::string reason;
};

/**
 * Channel Rebalancing Manager
 *
 * Manages Lightning channel liquidity through automatic and manual
 * rebalancing to optimize payment flow and routing capabilities.
 */
class ChannelRebalancingManager {
public:
    struct Config {
        RebalanceStrategy strategy{RebalanceStrategy::AUTO_BALANCED};
        RebalanceMethod preferred_method{RebalanceMethod::CIRCULAR};
        double target_local_ratio{0.5};      // Target 50/50 balance
        uint64_t max_fee_per_rebalance{1000};
        double max_fee_ratio{0.01};          // Max 1% fee
        uint32_t rebalance_interval_hours{24};
        bool auto_rebalance{false};
        uint64_t min_rebalance_amount{10000};
        uint64_t max_rebalance_amount{1000000};
    };

    ChannelRebalancingManager();
    explicit ChannelRebalancingManager(const Config& config);
    ~ChannelRebalancingManager();

    /**
     * Get channel balances
     */
    std::vector<ChannelBalance> GetChannelBalances() const;

    /**
     * Get channel balance
     *
     * @param channel_id Channel ID
     * @return Channel balance info
     */
    ChannelBalance GetChannelBalance(const std::string& channel_id) const;

    /**
     * Rebalance channel
     *
     * @param source_channel Channel to send from
     * @param dest_channel Channel to receive on
     * @param amount Amount to rebalance
     * @param max_fee Maximum acceptable fee
     * @return Rebalance operation ID
     */
    std::string RebalanceChannel(
        const std::string& source_channel,
        const std::string& dest_channel,
        uint64_t amount,
        uint64_t max_fee = 0
    );

    /**
     * Auto-rebalance all channels
     *
     * Rebalance channels according to strategy
     *
     * @return Number of rebalance operations initiated
     */
    uint32_t AutoRebalance();

    /**
     * Get rebalance recommendations
     *
     * @param limit Maximum recommendations
     * @return Vector of recommendations
     */
    std::vector<RebalanceRecommendation> GetRecommendations(
        uint32_t limit = 10
    ) const;

    /**
     * Set channel target
     *
     * @param channel_id Channel ID
     * @param target Rebalancing target
     */
    void SetChannelTarget(
        const std::string& channel_id,
        const RebalanceTarget& target
    );

    /**
     * Get channel target
     *
     * @param channel_id Channel ID
     * @return Rebalancing target
     */
    RebalanceTarget GetChannelTarget(const std::string& channel_id) const;

    /**
     * Remove channel target
     *
     * @param channel_id Channel ID
     */
    void RemoveChannelTarget(const std::string& channel_id);

    /**
     * Get active rebalancing operations
     */
    std::vector<RebalanceOperation> GetActiveOperations() const;

    /**
     * Get rebalancing operation
     *
     * @param rebalance_id Rebalance ID
     * @return Rebalance operation
     */
    RebalanceOperation GetOperation(const std::string& rebalance_id) const;

    /**
     * Get rebalancing history
     *
     * @param limit Maximum number of operations
     * @return Vector of completed operations
     */
    std::vector<RebalanceOperation> GetHistory(uint32_t limit = 100) const;

    /**
     * Cancel rebalancing operation
     *
     * @param rebalance_id Rebalance ID
     * @return True if cancelled successfully
     */
    bool CancelOperation(const std::string& rebalance_id);

    /**
     * Calculate optimal rebalance amount
     *
     * @param source_channel Source channel ID
     * @param dest_channel Destination channel ID
     * @return Optimal amount to rebalance
     */
    uint64_t CalculateOptimalAmount(
        const std::string& source_channel,
        const std::string& dest_channel
    ) const;

    /**
     * Estimate rebalancing fee
     *
     * @param source_channel Source channel
     * @param dest_channel Destination channel
     * @param amount Amount to rebalance
     * @param method Rebalancing method
     * @return Estimated fee
     */
    uint64_t EstimateFee(
        const std::string& source_channel,
        const std::string& dest_channel,
        uint64_t amount,
        RebalanceMethod method = RebalanceMethod::CIRCULAR
    ) const;

    /**
     * Find circular rebalance route
     *
     * @param source_channel Source channel
     * @param dest_channel Destination channel
     * @param amount Amount
     * @return Route (node pubkeys)
     */
    std::vector<std::string> FindCircularRoute(
        const std::string& source_channel,
        const std::string& dest_channel,
        uint64_t amount
    ) const;

    /**
     * Set configuration
     *
     * @param config Rebalancing configuration
     */
    void SetConfig(const Config& config);

    /**
     * Get configuration
     */
    Config GetConfig() const;

    /**
     * Get statistics
     */
    struct Statistics {
        uint32_t total_rebalances{0};
        uint32_t successful_rebalances{0};
        uint32_t failed_rebalances{0};
        uint64_t total_amount_rebalanced{0};
        uint64_t total_fees_paid{0};
        double average_fee_ratio{0.0};
        uint64_t last_rebalance_time{0};
    };

    Statistics GetStatistics() const;

    /**
     * Enable/disable auto-rebalancing
     *
     * @param enabled Enable flag
     */
    void SetAutoRebalance(bool enabled);

    /**
     * Check if auto-rebalancing is enabled
     */
    bool IsAutoRebalanceEnabled() const;

    /**
     * Clear rebalancing history
     */
    void ClearHistory();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get rebalance strategy name
 */
std::string GetRebalanceStrategyName(RebalanceStrategy strategy);

/**
 * Parse rebalance strategy from string
 */
RebalanceStrategy ParseRebalanceStrategy(const std::string& name);

/**
 * Get rebalance method name
 */
std::string GetRebalanceMethodName(RebalanceMethod method);

/**
 * Parse rebalance method from string
 */
RebalanceMethod ParseRebalanceMethod(const std::string& name);

/**
 * Get rebalance status name
 */
std::string GetRebalanceStatusName(RebalanceStatus status);

/**
 * Parse rebalance status from string
 */
RebalanceStatus ParseRebalanceStatus(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_CHANNEL_REBALANCING_H
