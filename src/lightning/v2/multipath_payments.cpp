// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/multipath_payments.h>
#include <intcoin/lightning/v2/routing.h>
#include <intcoin/lightning/v2/network_explorer.h>
#include <sstream>
#include <algorithm>
#include <map>
#include <chrono>
#include <random>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetPaymentStatusName(PaymentStatus status) {
    switch (status) {
        case PaymentStatus::PENDING: return "PENDING";
        case PaymentStatus::IN_FLIGHT: return "IN_FLIGHT";
        case PaymentStatus::SUCCEEDED: return "SUCCEEDED";
        case PaymentStatus::FAILED: return "FAILED";
        case PaymentStatus::TIMEOUT: return "TIMEOUT";
        case PaymentStatus::PARTIALLY_FAILED: return "PARTIALLY_FAILED";
        default: return "UNKNOWN";
    }
}

PaymentStatus ParsePaymentStatus(const std::string& name) {
    if (name == "PENDING") return PaymentStatus::PENDING;
    if (name == "IN_FLIGHT") return PaymentStatus::IN_FLIGHT;
    if (name == "SUCCEEDED") return PaymentStatus::SUCCEEDED;
    if (name == "FAILED") return PaymentStatus::FAILED;
    if (name == "TIMEOUT") return PaymentStatus::TIMEOUT;
    if (name == "PARTIALLY_FAILED") return PaymentStatus::PARTIALLY_FAILED;
    return PaymentStatus::PENDING;
}

std::string GetSplitStrategyName(SplitStrategy strategy) {
    switch (strategy) {
        case SplitStrategy::EQUAL_SPLIT: return "EQUAL_SPLIT";
        case SplitStrategy::BALANCED_LIQUIDITY: return "BALANCED_LIQUIDITY";
        case SplitStrategy::MINIMIZE_FEES: return "MINIMIZE_FEES";
        case SplitStrategy::OPTIMIZE_SUCCESS_RATE: return "OPTIMIZE_SUCCESS_RATE";
        case SplitStrategy::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

SplitStrategy ParseSplitStrategy(const std::string& name) {
    if (name == "EQUAL_SPLIT") return SplitStrategy::EQUAL_SPLIT;
    if (name == "BALANCED_LIQUIDITY") return SplitStrategy::BALANCED_LIQUIDITY;
    if (name == "MINIMIZE_FEES") return SplitStrategy::MINIMIZE_FEES;
    if (name == "OPTIMIZE_SUCCESS_RATE") return SplitStrategy::OPTIMIZE_SUCCESS_RATE;
    if (name == "CUSTOM") return SplitStrategy::CUSTOM;
    return SplitStrategy::OPTIMIZE_SUCCESS_RATE;
}

// Pimpl implementation
class MultiPathPaymentManager::Impl {
public:
    MPPConfig config_;
    std::map<std::string, MPPayment> payments_;
    Statistics stats_;
    bool enabled_{true};
    std::unique_ptr<RoutingManager> routing_manager_;
    std::unique_ptr<NetworkExplorer> network_explorer_;

    Impl() : config_(),
             routing_manager_(std::make_unique<RoutingManager>()),
             network_explorer_(std::make_unique<NetworkExplorer>()) {}

    explicit Impl(const MPPConfig& config) : config_(config),
             routing_manager_(std::make_unique<RoutingManager>()),
             network_explorer_(std::make_unique<NetworkExplorer>()) {}

    // Generate unique payment ID
    std::string GeneratePaymentId() {
        return "mpp_" + std::to_string(payments_.size()) + "_" +
               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }

    // Generate AMP child payment secret
    std::array<uint8_t, 32> GenerateChildSecret(const std::array<uint8_t, 32>& root_secret, uint32_t child_index) {
        std::array<uint8_t, 32> child_secret;
        // Simple derivation: hash(root_secret || child_index)
        std::vector<uint8_t> data(root_secret.begin(), root_secret.end());
        for (int i = 3; i >= 0; i--) {
            data.push_back((child_index >> (i * 8)) & 0xFF);
        }

        // Use SHA256 for derivation (simplified - production would use HMAC)
        // For now, just XOR the child index across the secret
        child_secret = root_secret;
        for (size_t i = 0; i < 4 && i < child_secret.size(); i++) {
            child_secret[i] ^= (child_index >> (i * 8)) & 0xFF;
        }

        return child_secret;
    }
};

MultiPathPaymentManager::MultiPathPaymentManager()
    : pimpl_(std::make_unique<Impl>()) {}

MultiPathPaymentManager::MultiPathPaymentManager(const MPPConfig& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

MultiPathPaymentManager::~MultiPathPaymentManager() = default;

std::string MultiPathPaymentManager::SendPayment(
    const std::string& destination,
    uint64_t amount_msat,
    const std::string& payment_hash,
    uint64_t max_fee_msat
) {
    if (!pimpl_->enabled_) {
        return "";
    }

    // Create payment record
    MPPayment payment;
    payment.payment_id = pimpl_->GeneratePaymentId();
    payment.payment_hash = payment_hash;
    payment.total_amount_msat = amount_msat;
    payment.destination = destination;
    payment.status = PaymentStatus::PENDING;
    payment.is_amp = false;
    payment.created_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    // Perform payment splitting
    auto split_result = SplitPayment(destination, amount_msat, max_fee_msat);

    if (split_result.routes.empty()) {
        payment.status = PaymentStatus::FAILED;
        pimpl_->payments_[payment.payment_id] = payment;
        pimpl_->stats_.failed_payments++;
        return payment.payment_id;
    }

    // Copy payment routes (already converted in SplitPayment)
    payment.routes = split_result.routes;

    payment.total_fee_msat = split_result.total_fee_msat;
    payment.status = PaymentStatus::IN_FLIGHT;

    // Store payment
    pimpl_->payments_[payment.payment_id] = payment;
    pimpl_->stats_.total_payments++;
    pimpl_->stats_.mpp_payments++;

    return payment.payment_id;
}

std::string MultiPathPaymentManager::SendAMPPayment(
    const std::string& destination,
    uint64_t amount_msat,
    uint64_t max_fee_msat
) {
    if (!pimpl_->enabled_ || !pimpl_->config_.enable_amp) {
        return "";
    }

    // Create payment record
    MPPayment payment;
    payment.payment_id = pimpl_->GeneratePaymentId();
    payment.total_amount_msat = amount_msat;
    payment.destination = destination;
    payment.status = PaymentStatus::PENDING;
    payment.is_amp = true;
    payment.created_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    // Generate root AMP secret (32 random bytes)
    std::array<uint8_t, 32> root_secret;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    for (size_t i = 0; i < 4; i++) {
        uint64_t random_val = dis(gen);
        for (size_t j = 0; j < 8; j++) {
            root_secret[i * 8 + j] = (random_val >> (j * 8)) & 0xFF;
        }
    }

    payment.amp_root_secret = root_secret;

    // Perform payment splitting
    auto split_result = SplitPayment(destination, amount_msat, max_fee_msat);

    if (split_result.routes.empty()) {
        payment.status = PaymentStatus::FAILED;
        pimpl_->payments_[payment.payment_id] = payment;
        pimpl_->stats_.failed_payments++;
        return payment.payment_id;
    }

    // Copy payment routes and add AMP child secrets
    payment.routes = split_result.routes;
    uint32_t child_index = 0;
    for (auto& payment_route : payment.routes) {
        // Generate child secret for this payment part
        auto child_secret = pimpl_->GenerateChildSecret(root_secret, child_index++);

        // In AMP, each part has its own payment hash derived from child secret
        // Hash the child secret to get payment_hash (simplified - production would use preimage)
        std::string child_hash_str;
        for (auto byte : child_secret) {
            child_hash_str += static_cast<char>(byte);
        }
        payment_route.payment_hash = child_hash_str;
    }

    payment.total_fee_msat = split_result.total_fee_msat;
    payment.status = PaymentStatus::IN_FLIGHT;

    // Store payment
    pimpl_->payments_[payment.payment_id] = payment;
    pimpl_->stats_.total_payments++;
    pimpl_->stats_.amp_payments++;

    return payment.payment_id;
}

MPPayment MultiPathPaymentManager::GetPayment(const std::string& payment_id) const {
    auto it = pimpl_->payments_.find(payment_id);
    if (it != pimpl_->payments_.end()) {
        return it->second;
    }
    return MPPayment{};
}

bool MultiPathPaymentManager::CancelPayment(const std::string& payment_id) {
    auto it = pimpl_->payments_.find(payment_id);
    if (it != pimpl_->payments_.end() && it->second.status == PaymentStatus::PENDING) {
        it->second.status = PaymentStatus::FAILED;
        return true;
    }
    return false;
}

SplitResult MultiPathPaymentManager::SplitPayment(
    const std::string& destination,
    uint64_t amount_msat,
    uint64_t max_fee_msat
) const {
    SplitResult result;

    // Determine number of paths to use
    uint32_t num_paths = pimpl_->config_.max_paths;
    if (num_paths < 1) {
        num_paths = 1;
    }

    // Set up route constraints
    RouteConstraints constraints;
    constraints.max_fee_msat = max_fee_msat;
    if (max_fee_msat == 0) {
        // Default to 1% fee
        constraints.max_fee_ratio = 0.01;
    } else {
        constraints.max_fee_ratio = static_cast<double>(max_fee_msat) / static_cast<double>(amount_msat);
    }

    // Find multiple routes using routing manager
    // Note: We need a source node - using empty string as placeholder
    // In production, this would be the local node's pubkey
    std::string source = "";
    auto routes = pimpl_->routing_manager_->FindRoutes(
        source,
        destination,
        amount_msat / num_paths,  // Amount per route
        num_paths,
        constraints
    );

    if (routes.empty()) {
        // Fallback: try single route with full amount
        auto single_route = pimpl_->routing_manager_->FindRoute(
            source,
            destination,
            amount_msat,
            constraints
        );

        if (!single_route.route_id.empty()) {
            routes.push_back(single_route);
        } else {
            return result;  // No routes found
        }
    }

    // Convert routing::Route to PaymentRoute first
    std::vector<PaymentRoute> payment_routes;
    for (const auto& route : routes) {
        PaymentRoute payment_route;
        payment_route.route_id = route.route_id;
        payment_route.amount_msat = route.total_amount_msat;
        payment_route.fee_msat = route.total_fee_msat;
        payment_route.cltv_delta = route.total_cltv_delta;
        payment_route.success_probability = route.success_probability;
        payment_route.status = PaymentStatus::PENDING;

        // Convert hops from RouteHop to node pubkeys
        for (const auto& hop : route.hops) {
            payment_route.hops.push_back(hop.node_pubkey);
        }

        payment_routes.push_back(payment_route);
    }

    // Calculate optimal split based on strategy
    auto split_amounts = CalculateOptimalSplit(amount_msat, payment_routes);

    // Apply split amounts to routes
    uint64_t total_amount = 0;
    uint64_t total_fee = 0;
    double combined_probability = 1.0;

    for (size_t i = 0; i < payment_routes.size() && i < split_amounts.size(); i++) {
        // Update route with split amount
        uint64_t split_amount = split_amounts[i];

        // Recalculate fees proportionally
        if (payment_routes[i].amount_msat > 0) {
            uint64_t base_fee = payment_routes[i].fee_msat;
            payment_routes[i].fee_msat = (base_fee * split_amount) / payment_routes[i].amount_msat;
        } else {
            payment_routes[i].fee_msat = split_amount / 1000;  // 0.1% default fee
        }

        payment_routes[i].amount_msat = split_amount;

        total_amount += split_amount;
        total_fee += payment_routes[i].fee_msat;
        combined_probability *= payment_routes[i].success_probability;

        result.routes.push_back(payment_routes[i]);
    }

    result.total_amount_msat = total_amount;
    result.total_fee_msat = total_fee;
    result.estimated_success_rate = combined_probability;

    return result;
}

std::vector<PaymentRoute> MultiPathPaymentManager::FindRoutes(
    const std::string& destination,
    uint64_t amount_msat,
    uint32_t num_routes
) const {
    std::vector<PaymentRoute> payment_routes;

    // Use routing manager to find routes
    std::string source = "";  // Local node (placeholder)
    RouteConstraints constraints;
    constraints.max_fee_ratio = 0.01;  // 1% max fee
    constraints.max_hops = 20;

    auto routes = pimpl_->routing_manager_->FindRoutes(
        source,
        destination,
        amount_msat / num_routes,
        num_routes,
        constraints
    );

    // Convert Route to PaymentRoute
    for (const auto& route : routes) {
        PaymentRoute payment_route;
        payment_route.route_id = route.route_id;
        payment_route.amount_msat = route.total_amount_msat;
        payment_route.fee_msat = route.total_fee_msat;
        payment_route.cltv_delta = route.total_cltv_delta;
        payment_route.success_probability = route.success_probability;
        payment_route.status = PaymentStatus::PENDING;

        // Convert hops
        for (const auto& hop : route.hops) {
            payment_route.hops.push_back(hop.node_pubkey);
        }

        payment_routes.push_back(payment_route);
    }

    return payment_routes;
}

std::vector<uint64_t> MultiPathPaymentManager::CalculateOptimalSplit(
    uint64_t total_amount,
    const std::vector<PaymentRoute>& available_routes
) const {
    std::vector<uint64_t> splits;

    if (available_routes.empty()) {
        return splits;
    }

    size_t num_routes = available_routes.size();

    switch (pimpl_->config_.strategy) {
        case SplitStrategy::EQUAL_SPLIT: {
            // Divide amount equally across all routes
            uint64_t per_route = total_amount / num_routes;
            uint64_t remainder = total_amount % num_routes;

            for (size_t i = 0; i < num_routes; i++) {
                uint64_t amount = per_route;
                if (i < remainder) {
                    amount++;  // Distribute remainder
                }
                splits.push_back(amount);
            }
            break;
        }

        case SplitStrategy::BALANCED_LIQUIDITY: {
            // Weight by success probability (proxy for liquidity)
            double total_probability = 0.0;
            for (const auto& route : available_routes) {
                total_probability += route.success_probability;
            }

            if (total_probability > 0.0) {
                for (const auto& route : available_routes) {
                    double weight = route.success_probability / total_probability;
                    uint64_t amount = static_cast<uint64_t>(total_amount * weight);
                    splits.push_back(amount);
                }
            } else {
                // Fallback to equal split
                uint64_t per_route = total_amount / num_routes;
                for (size_t i = 0; i < num_routes; i++) {
                    splits.push_back(per_route);
                }
            }
            break;
        }

        case SplitStrategy::MINIMIZE_FEES: {
            // Prefer routes with lower fees
            double total_inverse_fee = 0.0;
            std::vector<double> inverse_fees;

            for (const auto& route : available_routes) {
                double inverse_fee = 1.0 / (1.0 + route.fee_msat);
                inverse_fees.push_back(inverse_fee);
                total_inverse_fee += inverse_fee;
            }

            if (total_inverse_fee > 0.0) {
                for (size_t i = 0; i < num_routes; i++) {
                    double weight = inverse_fees[i] / total_inverse_fee;
                    uint64_t amount = static_cast<uint64_t>(total_amount * weight);
                    splits.push_back(amount);
                }
            } else {
                uint64_t per_route = total_amount / num_routes;
                for (size_t i = 0; i < num_routes; i++) {
                    splits.push_back(per_route);
                }
            }
            break;
        }

        case SplitStrategy::OPTIMIZE_SUCCESS_RATE: {
            // Combine probability and fee for optimal score
            double total_score = 0.0;
            std::vector<double> scores;

            for (const auto& route : available_routes) {
                // Score = probability * (1 / (1 + fee_ratio))
                double fee_ratio = static_cast<double>(route.fee_msat) / std::max(route.amount_msat, uint64_t(1));
                double score = route.success_probability * (1.0 / (1.0 + fee_ratio));
                scores.push_back(score);
                total_score += score;
            }

            if (total_score > 0.0) {
                for (size_t i = 0; i < num_routes; i++) {
                    double weight = scores[i] / total_score;
                    uint64_t amount = static_cast<uint64_t>(total_amount * weight);
                    splits.push_back(amount);
                }
            } else {
                uint64_t per_route = total_amount / num_routes;
                for (size_t i = 0; i < num_routes; i++) {
                    splits.push_back(per_route);
                }
            }
            break;
        }

        case SplitStrategy::CUSTOM:
        default: {
            // Default to equal split
            uint64_t per_route = total_amount / num_routes;
            for (size_t i = 0; i < num_routes; i++) {
                splits.push_back(per_route);
            }
            break;
        }
    }

    // Ensure total equals requested amount (account for rounding)
    uint64_t split_total = 0;
    for (auto amount : splits) {
        split_total += amount;
    }

    if (split_total != total_amount && !splits.empty()) {
        // Adjust last split to match exact total
        int64_t diff = static_cast<int64_t>(total_amount) - static_cast<int64_t>(split_total);
        splits.back() += diff;
    }

    return splits;
}

bool MultiPathPaymentManager::RetryFailedParts(const std::string& payment_id) {
    auto it = pimpl_->payments_.find(payment_id);
    if (it == pimpl_->payments_.end()) {
        return false;  // Payment not found
    }

    auto& payment = it->second;

    // Find failed routes
    std::vector<size_t> failed_indices;
    uint64_t failed_amount = 0;

    for (size_t i = 0; i < payment.routes.size(); i++) {
        if (payment.routes[i].status == PaymentStatus::FAILED) {
            failed_indices.push_back(i);
            failed_amount += payment.routes[i].amount_msat;
        }
    }

    if (failed_indices.empty()) {
        return true;  // No failed routes to retry
    }

    // Find alternative routes for failed amount
    std::string source = "";  // Local node
    RouteConstraints constraints;
    constraints.max_fee_ratio = 0.01;

    // Add previously failed nodes to ignore list
    for (size_t idx : failed_indices) {
        for (const auto& hop : payment.routes[idx].hops) {
            constraints.ignored_nodes.push_back(hop);
        }
    }

    auto retry_routes = pimpl_->routing_manager_->FindRoutes(
        source,
        payment.destination,
        failed_amount / failed_indices.size(),
        static_cast<uint32_t>(failed_indices.size()),
        constraints
    );

    if (retry_routes.empty()) {
        payment.status = PaymentStatus::FAILED;
        pimpl_->stats_.failed_payments++;
        return false;
    }

    // Replace failed routes with new routes
    size_t retry_idx = 0;
    for (size_t idx : failed_indices) {
        if (retry_idx < retry_routes.size()) {
            PaymentRoute payment_route;
            payment_route.route_id = retry_routes[retry_idx].route_id + "_retry";
            payment_route.amount_msat = retry_routes[retry_idx].total_amount_msat;
            payment_route.fee_msat = retry_routes[retry_idx].total_fee_msat;
            payment_route.cltv_delta = retry_routes[retry_idx].total_cltv_delta;
            payment_route.success_probability = retry_routes[retry_idx].success_probability;
            payment_route.status = PaymentStatus::PENDING;

            for (const auto& hop : retry_routes[retry_idx].hops) {
                payment_route.hops.push_back(hop.node_pubkey);
            }

            payment.routes[idx] = payment_route;
            retry_idx++;
        }
    }

    payment.status = PaymentStatus::IN_FLIGHT;
    pimpl_->payments_[payment_id] = payment;

    return true;
}

std::vector<MPPayment> MultiPathPaymentManager::GetActivePayments() const {
    std::vector<MPPayment> active;
    for (const auto& [id, payment] : pimpl_->payments_) {
        if (payment.status == PaymentStatus::PENDING || payment.status == PaymentStatus::IN_FLIGHT) {
            active.push_back(payment);
        }
    }
    return active;
}

std::vector<MPPayment> MultiPathPaymentManager::GetPaymentHistory(uint32_t limit) const {
    std::vector<MPPayment> history;
    uint32_t count = 0;

    for (auto it = pimpl_->payments_.rbegin();
         it != pimpl_->payments_.rend() && count < limit;
         ++it, ++count) {
        history.push_back(it->second);
    }

    return history;
}

void MultiPathPaymentManager::SetConfig(const MPPConfig& config) {
    pimpl_->config_ = config;
}

MPPConfig MultiPathPaymentManager::GetConfig() const {
    return pimpl_->config_;
}

MultiPathPaymentManager::Statistics MultiPathPaymentManager::GetStatistics() const {
    return pimpl_->stats_;
}

void MultiPathPaymentManager::ClearHistory() {
    pimpl_->payments_.clear();
}

void MultiPathPaymentManager::SetEnabled(bool enabled) {
    pimpl_->enabled_ = enabled;
}

bool MultiPathPaymentManager::IsEnabled() const {
    return pimpl_->enabled_;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
