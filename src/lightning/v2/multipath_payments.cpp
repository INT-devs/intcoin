// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/multipath_payments.h>
#include <sstream>
#include <algorithm>
#include <map>

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

    Impl() : config_() {}
    explicit Impl(const MPPConfig& config) : config_(config) {}
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
    (void)max_fee_msat;

    MPPayment payment;
    payment.payment_id = "mpp_" + std::to_string(pimpl_->payments_.size());
    payment.payment_hash = payment_hash;
    payment.total_amount_msat = amount_msat;
    payment.status = PaymentStatus::PENDING;

    // Create stub route
    PaymentRoute route;
    route.route_id = "route_0";
    route.hops = {destination};
    route.amount_msat = amount_msat;
    payment.routes.push_back(route);

    pimpl_->payments_[payment.payment_id] = payment;
    pimpl_->stats_.total_payments++;

    return payment.payment_id;
}

std::string MultiPathPaymentManager::SendAMPPayment(
    const std::string& destination,
    uint64_t amount_msat,
    uint64_t max_fee_msat
) {
    return SendPayment(destination, amount_msat, "amp_hash", max_fee_msat);
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
    (void)max_fee_msat;

    SplitResult result;

    // Simple stub: single route
    PaymentRoute route;
    route.route_id = "route_0";
    route.hops = {destination};
    route.amount_msat = amount_msat;
    route.fee_msat = amount_msat / 1000;  // 0.1% fee
    route.success_probability = 0.9;

    result.routes.push_back(route);
    result.total_amount_msat = amount_msat;
    result.total_fee_msat = route.fee_msat;
    result.estimated_success_rate = 0.9;

    return result;
}

std::vector<PaymentRoute> MultiPathPaymentManager::FindRoutes(
    const std::string& destination,
    uint64_t amount_msat,
    uint32_t num_routes
) const {
    std::vector<PaymentRoute> routes;

    for (uint32_t i = 0; i < num_routes; i++) {
        PaymentRoute route;
        route.route_id = "route_" + std::to_string(i);
        route.hops = {destination};
        route.amount_msat = amount_msat;
        route.fee_msat = amount_msat / 1000;
        route.success_probability = 0.9;
        routes.push_back(route);
    }

    return routes;
}

std::vector<uint64_t> MultiPathPaymentManager::CalculateOptimalSplit(
    uint64_t total_amount,
    const std::vector<PaymentRoute>& available_routes
) const {
    std::vector<uint64_t> splits;

    if (available_routes.empty()) {
        return splits;
    }

    // Simple equal split
    uint64_t per_route = total_amount / available_routes.size();
    for (size_t i = 0; i < available_routes.size(); i++) {
        splits.push_back(per_route);
    }

    return splits;
}

bool MultiPathPaymentManager::RetryFailedParts(const std::string& payment_id) {
    auto it = pimpl_->payments_.find(payment_id);
    if (it != pimpl_->payments_.end()) {
        return true;  // Stub: always success
    }
    return false;
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
