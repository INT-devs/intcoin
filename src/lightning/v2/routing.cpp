// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/routing.h>
#include <sstream>
#include <algorithm>
#include <map>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetRoutingAlgorithmName(RoutingAlgorithm algorithm) {
    switch (algorithm) {
        case RoutingAlgorithm::DIJKSTRA: return "DIJKSTRA";
        case RoutingAlgorithm::ASTAR: return "ASTAR";
        case RoutingAlgorithm::YEN: return "YEN";
        case RoutingAlgorithm::MISSION_CONTROL: return "MISSION_CONTROL";
        default: return "UNKNOWN";
    }
}

RoutingAlgorithm ParseRoutingAlgorithm(const std::string& name) {
    if (name == "DIJKSTRA") return RoutingAlgorithm::DIJKSTRA;
    if (name == "ASTAR") return RoutingAlgorithm::ASTAR;
    if (name == "YEN") return RoutingAlgorithm::YEN;
    if (name == "MISSION_CONTROL") return RoutingAlgorithm::MISSION_CONTROL;
    return RoutingAlgorithm::MISSION_CONTROL;
}

std::string GetRouteOptimizationName(RouteOptimization optimization) {
    switch (optimization) {
        case RouteOptimization::MINIMIZE_FEE: return "MINIMIZE_FEE";
        case RouteOptimization::MINIMIZE_HOPS: return "MINIMIZE_HOPS";
        case RouteOptimization::MAXIMIZE_PROBABILITY: return "MAXIMIZE_PROBABILITY";
        case RouteOptimization::BALANCED: return "BALANCED";
        default: return "UNKNOWN";
    }
}

RouteOptimization ParseRouteOptimization(const std::string& name) {
    if (name == "MINIMIZE_FEE") return RouteOptimization::MINIMIZE_FEE;
    if (name == "MINIMIZE_HOPS") return RouteOptimization::MINIMIZE_HOPS;
    if (name == "MAXIMIZE_PROBABILITY") return RouteOptimization::MAXIMIZE_PROBABILITY;
    if (name == "BALANCED") return RouteOptimization::BALANCED;
    return RouteOptimization::BALANCED;
}

// Pimpl implementation
class RoutingManager::Impl {
public:
    Config config_;
    std::map<std::string, MissionControlEntry> mission_control_;
    Statistics stats_;

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}
};

RoutingManager::RoutingManager()
    : pimpl_(std::make_unique<Impl>()) {}

RoutingManager::RoutingManager(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

RoutingManager::~RoutingManager() = default;

Route RoutingManager::FindRoute(
    const std::string& source,
    const std::string& destination,
    uint64_t amount_msat,
    const RouteConstraints& constraints
) const {
    (void)constraints;

    Route route;
    route.route_id = "route_0";

    // Simple stub: direct route
    RouteHop hop;
    hop.node_pubkey = destination;
    hop.node_alias = "Destination";
    hop.amount_msat = amount_msat;
    hop.fee_msat = amount_msat / 1000;  // 0.1% fee
    hop.cltv_delta = 40;
    hop.success_probability = 0.95;

    route.hops.push_back(hop);
    route.total_amount_msat = amount_msat;
    route.total_fee_msat = hop.fee_msat;
    route.total_cltv_delta = hop.cltv_delta;
    route.success_probability = 0.95;
    route.route_score = 0.9;
    route.hop_count = 1;

    (void)source;  // Suppress unused warning

    return route;
}

std::vector<Route> RoutingManager::FindRoutes(
    const std::string& source,
    const std::string& destination,
    uint64_t amount_msat,
    uint32_t num_routes,
    const RouteConstraints& constraints
) const {
    std::vector<Route> routes;

    for (uint32_t i = 0; i < num_routes; i++) {
        Route route = FindRoute(source, destination, amount_msat, constraints);
        route.route_id = "route_" + std::to_string(i);
        routes.push_back(route);
    }

    return routes;
}

Route RoutingManager::FindRouteWithHints(
    const std::string& source,
    const std::string& destination,
    uint64_t amount_msat,
    const std::vector<RouteHint>& route_hints,
    const RouteConstraints& constraints
) const {
    (void)route_hints;
    return FindRoute(source, destination, amount_msat, constraints);
}

double RoutingManager::CalculateRouteScore(const Route& route) const {
    (void)route;
    return 0.9;  // Stub
}

double RoutingManager::EstimateSuccessProbability(const Route& route) const {
    return route.success_probability;
}

void RoutingManager::RecordPaymentAttempt(const PaymentAttempt& attempt) {
    if (attempt.success) {
        pimpl_->stats_.successful_payments++;
    } else {
        pimpl_->stats_.failed_payments++;
    }
    pimpl_->stats_.routes_attempted++;
}

std::vector<MissionControlEntry> RoutingManager::GetMissionControlEntries() const {
    std::vector<MissionControlEntry> entries;
    for (const auto& [key, entry] : pimpl_->mission_control_) {
        entries.push_back(entry);
    }
    return entries;
}

MissionControlEntry RoutingManager::GetMissionControlEntry(
    const std::string& source,
    const std::string& dest
) const {
    std::string key = source + ":" + dest;
    auto it = pimpl_->mission_control_.find(key);
    if (it != pimpl_->mission_control_.end()) {
        return it->second;
    }
    return MissionControlEntry{};
}

void RoutingManager::ClearMissionControl() {
    pimpl_->mission_control_.clear();
}

bool RoutingManager::ImportMissionControl(const std::string& json_data) {
    (void)json_data;
    return true;  // Stub
}

std::string RoutingManager::ExportMissionControl() const {
    return "{}";  // Stub: empty JSON
}

Route RoutingManager::QueryRoute(
    const std::string& source,
    const std::string& destination,
    uint64_t amount_msat
) const {
    return FindRoute(source, destination, amount_msat);
}

Route RoutingManager::BuildRoute(
    const std::vector<std::string>& hops,
    uint64_t amount_msat
) const {
    Route route;
    route.route_id = "custom_route";

    uint64_t remaining_amount = amount_msat;

    for (const auto& node_pubkey : hops) {
        RouteHop hop;
        hop.node_pubkey = node_pubkey;
        hop.amount_msat = remaining_amount;
        hop.fee_msat = remaining_amount / 1000;
        hop.cltv_delta = 40;
        hop.success_probability = 0.95;

        route.hops.push_back(hop);
        route.total_fee_msat += hop.fee_msat;
        route.total_cltv_delta += hop.cltv_delta;

        remaining_amount -= hop.fee_msat;
    }

    route.total_amount_msat = amount_msat;
    route.hop_count = static_cast<uint32_t>(hops.size());
    route.success_probability = 0.95;
    route.route_score = 0.9;

    return route;
}

void RoutingManager::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

RoutingManager::Config RoutingManager::GetConfig() const {
    return pimpl_->config_;
}

RoutingManager::Statistics RoutingManager::GetStatistics() const {
    return pimpl_->stats_;
}

void RoutingManager::ResetStatistics() {
    pimpl_->stats_ = Statistics{};
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
