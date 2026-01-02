// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_ROUTING_H
#define INTCOIN_LIGHTNING_V2_ROUTING_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Routing algorithm
 */
enum class RoutingAlgorithm {
    DIJKSTRA,                        // Classic Dijkstra shortest path
    ASTAR,                           // A* with heuristics
    YEN,                             // Yen's K-shortest paths
    MISSION_CONTROL                  // LND-style mission control
};

/**
 * Route optimization goal
 */
enum class RouteOptimization {
    MINIMIZE_FEE,                    // Lowest fees
    MINIMIZE_HOPS,                   // Fewest hops
    MAXIMIZE_PROBABILITY,            // Highest success probability
    BALANCED                         // Balance all factors
};

/**
 * Route hop
 */
struct RouteHop {
    std::string node_pubkey;
    std::string node_alias;
    std::string channel_id;
    uint64_t amount_msat{0};         // Amount forward (millisatoshis)
    uint64_t fee_msat{0};            // Fee for this hop
    uint32_t cltv_delta{0};          // CLTV delta
    double success_probability{1.0}; // Estimated success rate
};

/**
 * Payment route
 */
struct Route {
    std::string route_id;
    std::vector<RouteHop> hops;
    uint64_t total_amount_msat{0};
    uint64_t total_fee_msat{0};
    uint32_t total_cltv_delta{0};
    double success_probability{0.0};
    double route_score{0.0};         // Combined score (0.0-1.0)
    uint32_t hop_count{0};
};

/**
 * Route constraints
 */
struct RouteConstraints {
    uint64_t max_fee_msat{0};        // Maximum acceptable fee (0 = no limit)
    double max_fee_ratio{0.05};      // Max fee as ratio (default 5%)
    uint32_t max_hops{20};           // Maximum hop count
    uint32_t max_cltv_delta{1008};   // Maximum CLTV delta (~1 week)
    double min_success_probability{0.5};
    std::vector<std::string> ignored_nodes;    // Nodes to avoid
    std::vector<std::string> ignored_channels; // Channels to avoid
};

/**
 * Route hint (for private channels)
 */
struct RouteHint {
    std::string node_id;
    std::string channel_id;
    uint64_t fee_base_msat{0};
    uint32_t fee_proportional{0};
    uint32_t cltv_expiry_delta{0};
    uint64_t htlc_minimum_msat{0};
    uint64_t htlc_maximum_msat{0};
};

/**
 * Payment attempt result
 */
struct PaymentAttempt {
    std::string attempt_id;
    Route route;
    bool success{false};
    uint32_t failed_hop_index{0};
    std::string failure_reason;
    uint64_t attempted_at{0};
};

/**
 * Mission control entry
 */
struct MissionControlEntry {
    std::string node_pair;           // "source:dest"
    uint64_t last_success{0};
    uint64_t last_failure{0};
    uint32_t success_count{0};
    uint32_t failure_count{0};
    double success_probability{1.0};
    uint64_t decay_time{86400};      // 24 hours
};

/**
 * Advanced Routing Manager
 *
 * Provides optimized pathfinding for Lightning Network payments
 * with multiple algorithms and mission control learning.
 */
class RoutingManager {
public:
    struct Config {
        RoutingAlgorithm algorithm{RoutingAlgorithm::MISSION_CONTROL};
        RouteOptimization optimization{RouteOptimization::BALANCED};
        uint32_t max_routes{10};         // Max routes to find
        bool enable_route_hints{true};
        bool enable_mpp{true};           // Enable multi-path
        uint64_t mission_control_decay{86400}; // 24h decay
        double base_success_probability{0.6};
    };

    RoutingManager();
    explicit RoutingManager(const Config& config);
    ~RoutingManager();

    /**
     * Find route
     *
     * @param source Source node pubkey
     * @param destination Destination node pubkey
     * @param amount_msat Amount in millisatoshis
     * @param constraints Route constraints
     * @return Best route
     */
    Route FindRoute(
        const std::string& source,
        const std::string& destination,
        uint64_t amount_msat,
        const RouteConstraints& constraints = RouteConstraints()
    ) const;

    /**
     * Find multiple routes
     *
     * For multi-path payments
     *
     * @param source Source node pubkey
     * @param destination Destination node pubkey
     * @param amount_msat Amount per route
     * @param num_routes Number of routes to find
     * @param constraints Route constraints
     * @return Vector of routes
     */
    std::vector<Route> FindRoutes(
        const std::string& source,
        const std::string& destination,
        uint64_t amount_msat,
        uint32_t num_routes,
        const RouteConstraints& constraints = RouteConstraints()
    ) const;

    /**
     * Find route with hints
     *
     * Use route hints for private channels
     *
     * @param source Source node
     * @param destination Destination node
     * @param amount_msat Amount
     * @param route_hints Route hints
     * @param constraints Constraints
     * @return Route
     */
    Route FindRouteWithHints(
        const std::string& source,
        const std::string& destination,
        uint64_t amount_msat,
        const std::vector<RouteHint>& route_hints,
        const RouteConstraints& constraints = RouteConstraints()
    ) const;

    /**
     * Calculate route score
     *
     * Combined score based on fees, hops, and probability
     *
     * @param route Route to score
     * @return Score (0.0-1.0, higher is better)
     */
    double CalculateRouteScore(const Route& route) const;

    /**
     * Estimate success probability
     *
     * @param route Route to evaluate
     * @return Success probability (0.0-1.0)
     */
    double EstimateSuccessProbability(const Route& route) const;

    /**
     * Record payment attempt
     *
     * Update mission control with attempt result
     *
     * @param attempt Payment attempt
     */
    void RecordPaymentAttempt(const PaymentAttempt& attempt);

    /**
     * Get mission control entries
     */
    std::vector<MissionControlEntry> GetMissionControlEntries() const;

    /**
     * Get mission control entry
     *
     * @param source Source node
     * @param dest Destination node
     * @return Mission control entry
     */
    MissionControlEntry GetMissionControlEntry(
        const std::string& source,
        const std::string& dest
    ) const;

    /**
     * Clear mission control data
     */
    void ClearMissionControl();

    /**
     * Import mission control data
     *
     * @param json_data JSON mission control data
     * @return True if imported successfully
     */
    bool ImportMissionControl(const std::string& json_data);

    /**
     * Export mission control data
     *
     * @return JSON string
     */
    std::string ExportMissionControl() const;

    /**
     * Query route
     *
     * Get detailed route information without storing
     *
     * @param source Source node
     * @param destination Destination node
     * @param amount_msat Amount
     * @return Route details
     */
    Route QueryRoute(
        const std::string& source,
        const std::string& destination,
        uint64_t amount_msat
    ) const;

    /**
     * Build route from hops
     *
     * @param hops Vector of node pubkeys
     * @param amount_msat Amount to send
     * @return Constructed route
     */
    Route BuildRoute(
        const std::vector<std::string>& hops,
        uint64_t amount_msat
    ) const;

    /**
     * Set configuration
     *
     * @param config Routing configuration
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
        uint64_t routes_found{0};
        uint64_t routes_attempted{0};
        uint64_t successful_payments{0};
        uint64_t failed_payments{0};
        double average_success_rate{0.0};
        double average_fee{0.0};
        double average_hops{0.0};
        uint64_t mission_control_entries{0};
    };

    Statistics GetStatistics() const;

    /**
     * Reset statistics
     */
    void ResetStatistics();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get routing algorithm name
 */
std::string GetRoutingAlgorithmName(RoutingAlgorithm algorithm);

/**
 * Parse routing algorithm from string
 */
RoutingAlgorithm ParseRoutingAlgorithm(const std::string& name);

/**
 * Get route optimization name
 */
std::string GetRouteOptimizationName(RouteOptimization optimization);

/**
 * Parse route optimization from string
 */
RouteOptimization ParseRouteOptimization(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_ROUTING_H
