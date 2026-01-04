// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/lightning/v2/submarine_swaps.h>
#include <sstream>
#include <algorithm>
#include <map>

namespace intcoin {
namespace lightning {
namespace v2 {

// Utility functions for enum conversion
std::string GetSwapTypeName(SwapType type) {
    switch (type) {
        case SwapType::SWAP_IN: return "SWAP_IN";
        case SwapType::SWAP_OUT: return "SWAP_OUT";
        case SwapType::LOOP_IN: return "LOOP_IN";
        case SwapType::LOOP_OUT: return "LOOP_OUT";
        default: return "UNKNOWN";
    }
}

SwapType ParseSwapType(const std::string& name) {
    if (name == "SWAP_IN" || name == "LOOP_IN") return SwapType::SWAP_IN;
    if (name == "SWAP_OUT" || name == "LOOP_OUT") return SwapType::SWAP_OUT;
    return SwapType::SWAP_IN;
}

std::string GetSwapStatusName(SwapStatus status) {
    switch (status) {
        case SwapStatus::PENDING: return "PENDING";
        case SwapStatus::INVOICE_GENERATED: return "INVOICE_GENERATED";
        case SwapStatus::LOCKUP_TX_BROADCAST: return "LOCKUP_TX_BROADCAST";
        case SwapStatus::LOCKUP_TX_CONFIRMED: return "LOCKUP_TX_CONFIRMED";
        case SwapStatus::CLAIM_TX_BROADCAST: return "CLAIM_TX_BROADCAST";
        case SwapStatus::CLAIM_TX_CONFIRMED: return "CLAIM_TX_CONFIRMED";
        case SwapStatus::COMPLETED: return "COMPLETED";
        case SwapStatus::REFUNDED: return "REFUNDED";
        case SwapStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

SwapStatus ParseSwapStatus(const std::string& name) {
    if (name == "PENDING") return SwapStatus::PENDING;
    if (name == "INVOICE_GENERATED") return SwapStatus::INVOICE_GENERATED;
    if (name == "LOCKUP_TX_BROADCAST") return SwapStatus::LOCKUP_TX_BROADCAST;
    if (name == "LOCKUP_TX_CONFIRMED") return SwapStatus::LOCKUP_TX_CONFIRMED;
    if (name == "CLAIM_TX_BROADCAST") return SwapStatus::CLAIM_TX_BROADCAST;
    if (name == "CLAIM_TX_CONFIRMED") return SwapStatus::CLAIM_TX_CONFIRMED;
    if (name == "COMPLETED") return SwapStatus::COMPLETED;
    if (name == "REFUNDED") return SwapStatus::REFUNDED;
    if (name == "FAILED") return SwapStatus::FAILED;
    return SwapStatus::PENDING;
}

// Pimpl implementation
class SubmarineSwapManager::Impl {
public:
    Config config_;
    std::map<std::string, SubmarineSwap> swaps_;
    Statistics stats_;
    bool enabled_{true};

    Impl() : config_() {}
    explicit Impl(const Config& config) : config_(config) {}
};

SubmarineSwapManager::SubmarineSwapManager()
    : pimpl_(std::make_unique<Impl>()) {}

SubmarineSwapManager::SubmarineSwapManager(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

SubmarineSwapManager::~SubmarineSwapManager() = default;

SwapQuote SubmarineSwapManager::GetQuote(SwapType type, uint64_t amount) const {
    SwapQuote quote;
    quote.type = type;
    quote.amount = amount;
    quote.service_fee = amount / 100;  // 1% service fee
    quote.onchain_fee = 5000;          // ~5000 sats miner fee
    quote.total_fee = quote.service_fee + quote.onchain_fee;
    quote.fee_percentage = 1.0 + (static_cast<double>(quote.onchain_fee) / amount * 100.0);
    quote.timeout_blocks = pimpl_->config_.default_timeout;
    quote.min_amount = pimpl_->config_.min_swap_amount;
    quote.max_amount = pimpl_->config_.max_swap_amount;

    return quote;
}

SubmarineSwap SubmarineSwapManager::CreateSwapIn(
    uint64_t amount,
    const std::string& refund_address
) {
    SubmarineSwap swap;
    swap.swap_id = "swap_in_" + std::to_string(pimpl_->swaps_.size());
    swap.type = SwapType::SWAP_IN;
    swap.status = SwapStatus::PENDING;
    swap.amount = amount;
    swap.refund_address = refund_address;

    auto quote = GetQuote(SwapType::SWAP_IN, amount);
    swap.fee = quote.total_fee;

    pimpl_->swaps_[swap.swap_id] = swap;
    pimpl_->stats_.total_swaps++;

    return swap;
}

SubmarineSwap SubmarineSwapManager::CreateSwapOut(
    uint64_t amount,
    const std::string& claim_address
) {
    SubmarineSwap swap;
    swap.swap_id = "swap_out_" + std::to_string(pimpl_->swaps_.size());
    swap.type = SwapType::SWAP_OUT;
    swap.status = SwapStatus::PENDING;
    swap.amount = amount;
    swap.claim_address = claim_address;

    auto quote = GetQuote(SwapType::SWAP_OUT, amount);
    swap.fee = quote.total_fee;

    pimpl_->swaps_[swap.swap_id] = swap;
    pimpl_->stats_.total_swaps++;

    return swap;
}

bool SubmarineSwapManager::CompleteSwapIn(
    const std::string& swap_id,
    const std::string& lockup_txid
) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.lockup_txid = lockup_txid;
        it->second.status = SwapStatus::COMPLETED;
        pimpl_->stats_.completed_swaps++;
        pimpl_->stats_.total_swapped_in += it->second.amount;
        return true;
    }
    return false;
}

std::string SubmarineSwapManager::CompleteSwapOut(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.status = SwapStatus::COMPLETED;
        pimpl_->stats_.completed_swaps++;
        pimpl_->stats_.total_swapped_out += it->second.amount;
        return "preimage_" + swap_id;
    }
    return "";
}

std::string SubmarineSwapManager::RefundSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        it->second.status = SwapStatus::REFUNDED;
        it->second.refund_txid = "refund_tx_" + swap_id;
        pimpl_->stats_.refunded_swaps++;
        return it->second.refund_txid;
    }
    return "";
}

SubmarineSwap SubmarineSwapManager::GetSwap(const std::string& swap_id) const {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        return it->second;
    }
    return SubmarineSwap{};
}

std::vector<SubmarineSwap> SubmarineSwapManager::GetActiveSwaps() const {
    std::vector<SubmarineSwap> active;

    for (const auto& [id, swap] : pimpl_->swaps_) {
        if (swap.status != SwapStatus::COMPLETED &&
            swap.status != SwapStatus::FAILED &&
            swap.status != SwapStatus::REFUNDED) {
            active.push_back(swap);
        }
    }

    return active;
}

std::vector<SubmarineSwap> SubmarineSwapManager::GetSwapHistory(uint32_t limit) const {
    std::vector<SubmarineSwap> history;
    uint32_t count = 0;

    for (auto it = pimpl_->swaps_.rbegin();
         it != pimpl_->swaps_.rend() && count < limit;
         ++it, ++count) {
        history.push_back(it->second);
    }

    return history;
}

SwapStatus SubmarineSwapManager::MonitorSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end()) {
        return it->second.status;
    }
    return SwapStatus::FAILED;
}

bool SubmarineSwapManager::CancelSwap(const std::string& swap_id) {
    auto it = pimpl_->swaps_.find(swap_id);
    if (it != pimpl_->swaps_.end() && it->second.status == SwapStatus::PENDING) {
        it->second.status = SwapStatus::FAILED;
        pimpl_->stats_.failed_swaps++;
        return true;
    }
    return false;
}

SubmarineSwapManager::SwapLimits SubmarineSwapManager::GetSwapLimits(SwapType type) const {
    (void)type;

    SwapLimits limits;
    limits.min_amount = pimpl_->config_.min_swap_amount;
    limits.max_amount = pimpl_->config_.max_swap_amount;
    return limits;
}

uint64_t SubmarineSwapManager::EstimateFees(SwapType type, uint64_t amount) const {
    auto quote = GetQuote(type, amount);
    return quote.total_fee;
}

void SubmarineSwapManager::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

SubmarineSwapManager::Config SubmarineSwapManager::GetConfig() const {
    return pimpl_->config_;
}

SubmarineSwapManager::Statistics SubmarineSwapManager::GetStatistics() const {
    return pimpl_->stats_;
}

void SubmarineSwapManager::SetEnabled(bool enabled) {
    pimpl_->enabled_ = enabled;
}

bool SubmarineSwapManager::IsEnabled() const {
    return pimpl_->enabled_;
}

} // namespace v2
} // namespace lightning
} // namespace intcoin
