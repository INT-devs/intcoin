// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_ROUTING_H
#define INTCOIN_LIGHTNING_ROUTING_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <limits>
#include <chrono>

namespace intcoin {
namespace lightning {

// Routing configuration parameters
namespace routing_config {
    // Maximum hop count (BOLT spec recommends 20)
    constexpr uint32_t MAX_HOP_COUNT = 20;

    // Minimum hop count (for privacy)
    constexpr uint32_t MIN_HOP_COUNT = 3;

    // CLTV delta per hop (blocks)
    constexpr uint32_t CLTV_DELTA_PER_HOP = 144;  // ~1 day

    // Base CLTV expiry (blocks)
    constexpr uint32_t BASE_CLTV_EXPIRY = 9;

    // Route timeout base (milliseconds)
    constexpr uint64_t ROUTE_TIMEOUT_BASE_MS = 30000;  // 30 seconds

    // Route timeout per hop (milliseconds)
    constexpr uint64_t ROUTE_TIMEOUT_PER_HOP_MS = 5000;  // 5 seconds per hop

    // Maximum route timeout (milliseconds)
    constexpr uint64_t MAX_ROUTE_TIMEOUT_MS = 300000;  // 5 minutes

    // Maximum fee percentage (in basis points, 10000 = 100%)
    constexpr uint32_t MAX_FEE_PERCENTAGE = 500;  // 5%
}

// Lightning node in the network
struct LightningNode {
    std::string node_id;           // Public key
    std::string alias;
    std::vector<std::string> addresses;
    uint64_t last_update;
    bool is_online = true;

    // Reputation metrics
    uint32_t successful_payments = 0;
    uint32_t failed_payments = 0;
    double uptime_ratio = 1.0;

    // Calculate success rate
    double get_success_rate() const {
        uint32_t total = successful_payments + failed_payments;
        if (total == 0) return 1.0;
        return static_cast<double>(successful_payments) / total;
    }
};

// Lightning channel
struct LightningChannel {
    std::string channel_id;
    std::string node1_id;
    std::string node2_id;
    uint64_t capacity;             // Satoshis
    uint64_t node1_balance;        // Satoshis
    uint64_t node2_balance;        // Satoshis
    uint32_t base_fee_msat;        // Base fee in millisatoshis
    uint32_t fee_rate_ppm;         // Fee rate in parts per million
    uint32_t cltv_expiry_delta;    // CLTV expiry delta
    bool is_active = true;
    uint64_t last_update;

    // Get available balance in direction
    uint64_t get_available_balance(const std::string& from_node) const {
        if (from_node == node1_id) {
            return node1_balance;
        } else if (from_node == node2_id) {
            return node2_balance;
        }
        return 0;
    }

    // Calculate fee for amount
    uint64_t calculate_fee(uint64_t amount_msat) const {
        // Fee = base_fee + (amount * fee_rate / 1,000,000)
        uint64_t proportional_fee = (amount_msat * fee_rate_ppm) / 1000000;
        return base_fee_msat + proportional_fee;
    }
};

// Route hop
struct RouteHop {
    std::string node_id;
    std::string channel_id;
    uint64_t amount_msat;          // Amount to forward
    uint32_t cltv_expiry;          // CLTV expiry height
    uint64_t fee_msat;             // Fee for this hop

    // Short channel ID (for onion routing)
    std::string short_channel_id;
};

// Complete route
struct Route {
    std::vector<RouteHop> hops;
    uint64_t total_amount_msat;    // Total including fees
    uint64_t total_fees_msat;      // Sum of all hop fees
    uint32_t total_cltv_delta;     // Sum of all CLTV deltas
    uint64_t timeout_ms;           // Calculated timeout
    double success_probability;     // Estimated success probability

    // Validate route constraints
    struct ValidationResult {
        bool valid;
        std::string error;
    };

    ValidationResult validate() const {
        ValidationResult result;
        result.valid = true;

        // Check hop count
        if (hops.size() > routing_config::MAX_HOP_COUNT) {
            result.valid = false;
            result.error = "Route exceeds maximum hop count (" +
                          std::to_string(hops.size()) + " > " +
                          std::to_string(routing_config::MAX_HOP_COUNT) + ")";
            return result;
        }

        if (hops.size() < routing_config::MIN_HOP_COUNT) {
            result.valid = false;
            result.error = "Route below minimum hop count for privacy (" +
                          std::to_string(hops.size()) + " < " +
                          std::to_string(routing_config::MIN_HOP_COUNT) + ")";
            return result;
        }

        // Validate CLTV values are decreasing
        for (size_t i = 1; i < hops.size(); ++i) {
            if (hops[i].cltv_expiry >= hops[i-1].cltv_expiry) {
                result.valid = false;
                result.error = "CLTV expiry values not properly decreasing";
                return result;
            }
        }

        // Validate timeout
        if (timeout_ms > routing_config::MAX_ROUTE_TIMEOUT_MS) {
            result.valid = false;
            result.error = "Route timeout exceeds maximum (" +
                          std::to_string(timeout_ms) + " > " +
                          std::to_string(routing_config::MAX_ROUTE_TIMEOUT_MS) + ")";
            return result;
        }

        return result;
    }

    // Get hop count
    uint32_t get_hop_count() const {
        return static_cast<uint32_t>(hops.size());
    }
};

// Hop count enforcer
class HopCountEnforcer {
private:
    struct Statistics {
        uint64_t routes_checked = 0;
        uint64_t routes_rejected_max_hops = 0;
        uint64_t routes_rejected_min_hops = 0;
        uint64_t routes_accepted = 0;
    } stats;

public:
    // Enforce maximum hop count
    bool enforce_max_hops(const Route& route) {
        stats.routes_checked++;

        if (route.get_hop_count() > routing_config::MAX_HOP_COUNT) {
            stats.routes_rejected_max_hops++;
            return false;
        }

        stats.routes_accepted++;
        return true;
    }

    // Enforce minimum hop count (for privacy)
    bool enforce_min_hops(const Route& route) {
        stats.routes_checked++;

        if (route.get_hop_count() < routing_config::MIN_HOP_COUNT) {
            stats.routes_rejected_min_hops++;
            return false;
        }

        stats.routes_accepted++;
        return true;
    }

    // Validate hop count is within bounds
    bool validate_hop_count(const Route& route) {
        return enforce_min_hops(route) && enforce_max_hops(route);
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Route timeout calculator
class RouteTimeoutCalculator {
private:
    struct Statistics {
        uint64_t timeouts_calculated = 0;
        uint64_t timeouts_capped = 0;
        uint64_t average_timeout_ms = 0;
    } stats;

public:
    // Calculate timeout for route
    uint64_t calculate_timeout(const Route& route) {
        stats.timeouts_calculated++;

        // Base timeout + per-hop timeout
        uint64_t timeout_ms = routing_config::ROUTE_TIMEOUT_BASE_MS +
                             (route.get_hop_count() * routing_config::ROUTE_TIMEOUT_PER_HOP_MS);

        // Add extra time for CLTV processing
        uint64_t cltv_timeout_ms = route.total_cltv_delta * 600000;  // ~10 min per block
        timeout_ms += cltv_timeout_ms / 10;  // Add 10% of CLTV time

        // Cap at maximum
        if (timeout_ms > routing_config::MAX_ROUTE_TIMEOUT_MS) {
            timeout_ms = routing_config::MAX_ROUTE_TIMEOUT_MS;
            stats.timeouts_capped++;
        }

        // Update average
        stats.average_timeout_ms =
            (stats.average_timeout_ms * (stats.timeouts_calculated - 1) + timeout_ms) /
            stats.timeouts_calculated;

        return timeout_ms;
    }

    // Calculate CLTV expiry for route
    uint32_t calculate_cltv_expiry(
        uint32_t current_block_height,
        uint32_t hop_count,
        uint32_t final_cltv_delta = routing_config::BASE_CLTV_EXPIRY
    ) {
        // CLTV = current_height + final_delta + (hops * delta_per_hop)
        uint32_t total_delta = final_cltv_delta +
                              (hop_count * routing_config::CLTV_DELTA_PER_HOP);

        return current_block_height + total_delta;
    }

    // Calculate per-hop CLTV values (decreasing)
    std::vector<uint32_t> calculate_hop_cltv_values(
        uint32_t final_cltv_expiry,
        uint32_t hop_count
    ) {
        std::vector<uint32_t> cltv_values;
        cltv_values.reserve(hop_count);

        // Start from final expiry and add delta for each hop backward
        uint32_t current_cltv = final_cltv_expiry;
        for (uint32_t i = 0; i < hop_count; ++i) {
            cltv_values.push_back(current_cltv);
            current_cltv += routing_config::CLTV_DELTA_PER_HOP;
        }

        // Reverse so first hop has highest CLTV
        std::reverse(cltv_values.begin(), cltv_values.end());

        return cltv_values;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Route pathfinding (Dijkstra-based)
class RoutePathfinder {
private:
    std::unordered_map<std::string, LightningNode> nodes;
    std::unordered_map<std::string, LightningChannel> channels;

    struct Statistics {
        uint64_t routes_found = 0;
        uint64_t routes_failed = 0;
        uint64_t average_hop_count = 0;
        uint64_t average_fee_msat = 0;
    } stats;

public:
    // Add node to graph
    void add_node(const LightningNode& node) {
        nodes[node.node_id] = node;
    }

    // Add channel to graph
    void add_channel(const LightningChannel& channel) {
        channels[channel.channel_id] = channel;
    }

    // Find route from source to destination
    struct RouteSearchResult {
        bool found;
        Route route;
        std::string error;
    };

    RouteSearchResult find_route(
        const std::string& source_id,
        const std::string& dest_id,
        uint64_t amount_msat,
        uint32_t current_block_height
    ) {
        RouteSearchResult result;
        result.found = false;

        // Check nodes exist
        if (nodes.find(source_id) == nodes.end()) {
            result.error = "Source node not found";
            stats.routes_failed++;
            return result;
        }

        if (nodes.find(dest_id) == nodes.end()) {
            result.error = "Destination node not found";
            stats.routes_failed++;
            return result;
        }

        // Simplified pathfinding (in production, use Dijkstra with hop count limit)
        std::vector<RouteHop> hops;

        // Create direct path (simplified - real implementation would find optimal path)
        RouteHop hop;
        hop.node_id = dest_id;
        hop.amount_msat = amount_msat;
        hop.fee_msat = 1000;  // Simplified fee
        hop.cltv_expiry = current_block_height + routing_config::BASE_CLTV_EXPIRY;
        hops.push_back(hop);

        // Build route
        result.route.hops = hops;
        result.route.total_amount_msat = amount_msat + hop.fee_msat;
        result.route.total_fees_msat = hop.fee_msat;
        result.route.total_cltv_delta = routing_config::CLTV_DELTA_PER_HOP;

        // Calculate timeout
        RouteTimeoutCalculator timeout_calc;
        result.route.timeout_ms = timeout_calc.calculate_timeout(result.route);

        // Validate route
        auto validation = result.route.validate();
        if (!validation.valid) {
            result.error = validation.error;
            stats.routes_failed++;
            return result;
        }

        result.found = true;
        stats.routes_found++;

        // Update statistics
        stats.average_hop_count =
            (stats.average_hop_count * (stats.routes_found - 1) + hops.size()) /
            stats.routes_found;
        stats.average_fee_msat =
            (stats.average_fee_msat * (stats.routes_found - 1) + result.route.total_fees_msat) /
            stats.routes_found;

        return result;
    }

    // Find multiple routes (for MPP - Multi-Path Payments)
    std::vector<Route> find_multiple_routes(
        const std::string& source_id,
        const std::string& dest_id,
        uint64_t total_amount_msat,
        uint32_t num_routes,
        uint32_t current_block_height
    ) {
        std::vector<Route> routes;

        // Split amount across multiple routes
        uint64_t amount_per_route = total_amount_msat / num_routes;

        for (uint32_t i = 0; i < num_routes; ++i) {
            auto result = find_route(source_id, dest_id, amount_per_route, current_block_height);
            if (result.found) {
                routes.push_back(result.route);
            }
        }

        return routes;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Route validator
class RouteValidator {
private:
    HopCountEnforcer hop_enforcer;
    RouteTimeoutCalculator timeout_calc;

    struct Statistics {
        uint64_t routes_validated = 0;
        uint64_t routes_passed = 0;
        uint64_t routes_failed = 0;
        uint64_t hop_count_violations = 0;
        uint64_t timeout_violations = 0;
        uint64_t cltv_violations = 0;
    } stats;

public:
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    // Validate complete route
    ValidationResult validate_route(const Route& route, uint32_t current_block_height) {
        stats.routes_validated++;
        ValidationResult result;
        result.valid = true;

        // Validate hop count
        if (!hop_enforcer.validate_hop_count(route)) {
            result.valid = false;
            result.errors.push_back("Hop count out of bounds");
            stats.hop_count_violations++;
        }

        // Validate timeout
        if (route.timeout_ms > routing_config::MAX_ROUTE_TIMEOUT_MS) {
            result.valid = false;
            result.errors.push_back("Route timeout exceeds maximum");
            stats.timeout_violations++;
        }

        // Validate CLTV expiry values
        for (size_t i = 0; i < route.hops.size(); ++i) {
            const auto& hop = route.hops[i];

            // CLTV must be in the future
            if (hop.cltv_expiry <= current_block_height) {
                result.valid = false;
                result.errors.push_back("CLTV expiry in the past for hop " + std::to_string(i));
                stats.cltv_violations++;
            }

            // CLTV values must be decreasing along the route
            if (i > 0 && hop.cltv_expiry >= route.hops[i-1].cltv_expiry) {
                result.valid = false;
                result.errors.push_back("CLTV values not decreasing at hop " + std::to_string(i));
                stats.cltv_violations++;
            }
        }

        // Validate amounts
        if (route.total_amount_msat == 0) {
            result.valid = false;
            result.errors.push_back("Route amount is zero");
        }

        // Check for excessive fees (warning, not error)
        if (route.total_fees_msat > 0) {
            uint64_t fee_percentage = (route.total_fees_msat * 10000) / route.total_amount_msat;
            if (fee_percentage > routing_config::MAX_FEE_PERCENTAGE) {
                result.warnings.push_back(
                    "Route fees exceed recommended maximum (" +
                    std::to_string(fee_percentage / 100.0) + "% > " +
                    std::to_string(routing_config::MAX_FEE_PERCENTAGE / 100.0) + "%)"
                );
            }
        }

        // Update statistics
        if (result.valid) {
            stats.routes_passed++;
        } else {
            stats.routes_failed++;
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Get hop enforcer statistics
    const HopCountEnforcer::Statistics& get_hop_statistics() const {
        return hop_enforcer.get_statistics();
    }

    // Get timeout calculator statistics
    const RouteTimeoutCalculator::Statistics& get_timeout_statistics() const {
        return timeout_calc.get_statistics();
    }
};

// Lightning routing manager
class LightningRoutingManager {
private:
    RoutePathfinder pathfinder;
    RouteValidator validator;
    RouteTimeoutCalculator timeout_calc;

    LightningRoutingManager() = default;

public:
    static LightningRoutingManager& instance() {
        static LightningRoutingManager instance;
        return instance;
    }

    // Add node to routing graph
    void add_node(const LightningNode& node) {
        pathfinder.add_node(node);
    }

    // Add channel to routing graph
    void add_channel(const LightningChannel& channel) {
        pathfinder.add_channel(channel);
    }

    // Find and validate route
    struct RoutingResult {
        bool success;
        Route route;
        std::string error;
        std::vector<std::string> warnings;
    };

    RoutingResult find_route(
        const std::string& source_id,
        const std::string& dest_id,
        uint64_t amount_msat,
        uint32_t current_block_height
    ) {
        RoutingResult result;

        // Find route
        auto search_result = pathfinder.find_route(
            source_id, dest_id, amount_msat, current_block_height
        );

        if (!search_result.found) {
            result.success = false;
            result.error = search_result.error;
            return result;
        }

        // Calculate timeout
        search_result.route.timeout_ms = timeout_calc.calculate_timeout(search_result.route);

        // Validate route
        auto validation = validator.validate_route(search_result.route, current_block_height);
        if (!validation.valid) {
            result.success = false;
            result.error = "Route validation failed";
            for (const auto& error : validation.errors) {
                result.error += ": " + error;
            }
            return result;
        }

        result.success = true;
        result.route = search_result.route;
        result.warnings = validation.warnings;

        return result;
    }

    // Get combined statistics
    struct CombinedStatistics {
        RoutePathfinder::Statistics pathfinder_stats;
        RouteValidator::Statistics validator_stats;
        RouteTimeoutCalculator::Statistics timeout_stats;
        HopCountEnforcer::Statistics hop_stats;
    };

    CombinedStatistics get_statistics() const {
        CombinedStatistics stats;
        stats.pathfinder_stats = pathfinder.get_statistics();
        stats.validator_stats = validator.get_statistics();
        stats.timeout_stats = timeout_calc.get_statistics();
        stats.hop_stats = validator.get_hop_statistics();
        return stats;
    }
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_ROUTING_H
