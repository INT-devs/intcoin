// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/defi/amm.h>
#include <map>
#include <mutex>
#include <cmath>
#include <algorithm>

namespace intcoin {
namespace defi {

class AutomatedMarketMaker::Impl {
public:
    std::map<std::string, LiquidityPool> pools_;
    std::mutex mutex_;

    std::string GetPoolKey(const std::string& token_a, const std::string& token_b) const {
        // Ensure consistent ordering
        if (token_a < token_b) {
            return token_a + "_" + token_b;
        }
        return token_b + "_" + token_a;
    }
};

AutomatedMarketMaker::AutomatedMarketMaker()
    : pimpl_(std::make_unique<Impl>()) {}

AutomatedMarketMaker::~AutomatedMarketMaker() = default;

bool AutomatedMarketMaker::CreatePool(
    const std::string& token_a,
    const std::string& token_b,
    uint32_t fee_bps
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_a, token_b);
    if (pimpl_->pools_.count(key) > 0) {
        return false; // Pool already exists
    }

    LiquidityPool pool;
    pool.token_a = (token_a < token_b) ? token_a : token_b;
    pool.token_b = (token_a < token_b) ? token_b : token_a;
    pool.fee_bps = fee_bps;

    pimpl_->pools_[key] = pool;
    return true;
}

LiquidityPool AutomatedMarketMaker::GetPool(
    const std::string& token_a,
    const std::string& token_b
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_a, token_b);
    if (pimpl_->pools_.count(key) == 0) {
        return LiquidityPool{};
    }

    return pimpl_->pools_.at(key);
}

std::vector<LiquidityPool> AutomatedMarketMaker::GetAllPools() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<LiquidityPool> pools;
    for (const auto& [key, pool] : pimpl_->pools_) {
        pools.push_back(pool);
    }
    return pools;
}

LiquidityPosition AutomatedMarketMaker::AddLiquidity(
    const std::string& token_a,
    const std::string& token_b,
    uint64_t amount_a,
    uint64_t amount_b
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_a, token_b);
    if (pimpl_->pools_.count(key) == 0) {
        return LiquidityPosition{};
    }

    auto& pool = pimpl_->pools_[key];

    LiquidityPosition position;

    if (pool.total_liquidity == 0) {
        // Initial liquidity
        position.liquidity_tokens = std::sqrt(amount_a * amount_b);
        pool.reserve_a = amount_a;
        pool.reserve_b = amount_b;
        pool.total_liquidity = position.liquidity_tokens;
    } else {
        // Proportional liquidity
        uint64_t liquidity_a = (amount_a * pool.total_liquidity) / pool.reserve_a;
        uint64_t liquidity_b = (amount_b * pool.total_liquidity) / pool.reserve_b;
        position.liquidity_tokens = std::min(liquidity_a, liquidity_b);

        pool.reserve_a += amount_a;
        pool.reserve_b += amount_b;
        pool.total_liquidity += position.liquidity_tokens;
    }

    position.token_a_deposited = amount_a;
    position.token_b_deposited = amount_b;
    position.pool_share = (static_cast<double>(position.liquidity_tokens) / pool.total_liquidity) * 100.0;

    return position;
}

LiquidityPosition AutomatedMarketMaker::RemoveLiquidity(
    const std::string& token_a,
    const std::string& token_b,
    uint64_t liquidity_tokens
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_a, token_b);
    if (pimpl_->pools_.count(key) == 0) {
        return LiquidityPosition{};
    }

    auto& pool = pimpl_->pools_[key];

    LiquidityPosition position;
    position.liquidity_tokens = liquidity_tokens;
    position.token_a_deposited = (liquidity_tokens * pool.reserve_a) / pool.total_liquidity;
    position.token_b_deposited = (liquidity_tokens * pool.reserve_b) / pool.total_liquidity;

    pool.reserve_a -= position.token_a_deposited;
    pool.reserve_b -= position.token_b_deposited;
    pool.total_liquidity -= liquidity_tokens;

    return position;
}

SwapQuote AutomatedMarketMaker::GetSwapQuote(
    const std::string& token_in,
    const std::string& token_out,
    uint64_t amount_in
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_in, token_out);
    if (pimpl_->pools_.count(key) == 0) {
        return SwapQuote{};
    }

    const auto& pool = pimpl_->pools_.at(key);

    uint64_t reserve_in = (token_in == pool.token_a) ? pool.reserve_a : pool.reserve_b;
    uint64_t reserve_out = (token_in == pool.token_a) ? pool.reserve_b : pool.reserve_a;

    SwapQuote quote;
    quote.input_amount = amount_in;
    quote.output_amount = GetAmountOut(amount_in, reserve_in, reserve_out, pool.fee_bps);
    quote.fee_amount = (amount_in * pool.fee_bps) / 10000;

    // Calculate price impact
    double old_price = static_cast<double>(reserve_out) / reserve_in;
    double new_reserve_in = reserve_in + amount_in;
    double new_reserve_out = reserve_out - quote.output_amount;
    double new_price = new_reserve_out / new_reserve_in;
    quote.price_impact = std::abs((new_price - old_price) / old_price) * 100.0;

    quote.exchange_rate = static_cast<double>(quote.output_amount) / amount_in;

    return quote;
}

bool AutomatedMarketMaker::ExecuteSwap(
    const std::string& token_in,
    const std::string& token_out,
    uint64_t amount_in,
    uint64_t min_amount_out
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_in, token_out);
    if (pimpl_->pools_.count(key) == 0) {
        return false;
    }

    auto& pool = pimpl_->pools_[key];

    uint64_t reserve_in = (token_in == pool.token_a) ? pool.reserve_a : pool.reserve_b;
    uint64_t reserve_out = (token_in == pool.token_a) ? pool.reserve_b : pool.reserve_a;

    uint64_t amount_out = GetAmountOut(amount_in, reserve_in, reserve_out, pool.fee_bps);

    if (amount_out < min_amount_out) {
        return false; // Slippage too high
    }

    // Update reserves
    if (token_in == pool.token_a) {
        pool.reserve_a += amount_in;
        pool.reserve_b -= amount_out;
    } else {
        pool.reserve_b += amount_in;
        pool.reserve_a -= amount_out;
    }

    return true;
}

double AutomatedMarketMaker::GetExchangeRate(
    const std::string& token_a,
    const std::string& token_b
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::string key = pimpl_->GetPoolKey(token_a, token_b);
    if (pimpl_->pools_.count(key) == 0) {
        return 0.0;
    }

    const auto& pool = pimpl_->pools_.at(key);

    if (token_a == pool.token_a) {
        return static_cast<double>(pool.reserve_b) / pool.reserve_a;
    }
    return static_cast<double>(pool.reserve_a) / pool.reserve_b;
}

uint64_t AutomatedMarketMaker::GetAmountOut(
    uint64_t amount_in,
    uint64_t reserve_in,
    uint64_t reserve_out,
    uint32_t fee_bps
) const {
    if (amount_in == 0 || reserve_in == 0 || reserve_out == 0) {
        return 0;
    }

    // Apply fee
    uint64_t amount_in_with_fee = amount_in * (10000 - fee_bps);

    // Constant product formula: (x + dx) * (y - dy) = x * y
    uint64_t numerator = amount_in_with_fee * reserve_out;
    uint64_t denominator = (reserve_in * 10000) + amount_in_with_fee;

    return numerator / denominator;
}

AutomatedMarketMaker::PoolStats AutomatedMarketMaker::GetPoolStats(
    const std::string& token_a,
    const std::string& token_b
) const {
    // TODO: Implement statistics tracking
    return PoolStats{};
}

} // namespace defi
} // namespace intcoin
