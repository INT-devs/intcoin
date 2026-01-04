// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_DEFI_AMM_H
#define INTCOIN_DEFI_AMM_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace intcoin {
namespace defi {

/**
 * @brief Automated Market Maker (AMM) - Constant Product Formula (x * y = k)
 *
 * Implements Uniswap V2-style AMM for decentralized token swaps
 * without the need for order books.
 */

// Liquidity pool for two tokens
struct LiquidityPool {
    std::string token_a;          // Token A identifier
    std::string token_b;          // Token B identifier
    uint64_t reserve_a{0};        // Token A reserve
    uint64_t reserve_b{0};        // Token B reserve
    uint64_t total_liquidity{0};  // Total LP tokens
    uint32_t fee_bps{30};         // Fee in basis points (0.3% = 30 bps)
};

// Swap quote/result
struct SwapQuote {
    uint64_t input_amount{0};
    uint64_t output_amount{0};
    uint64_t fee_amount{0};
    double price_impact{0.0};     // Price impact percentage
    double exchange_rate{0.0};
};

// Liquidity provision result
struct LiquidityPosition {
    uint64_t liquidity_tokens{0};
    uint64_t token_a_deposited{0};
    uint64_t token_b_deposited{0};
    double pool_share{0.0};       // Percentage of pool owned
};

/**
 * @class AutomatedMarketMaker
 * @brief Implements constant product AMM formula (x * y = k)
 */
class AutomatedMarketMaker {
public:
    AutomatedMarketMaker();
    ~AutomatedMarketMaker();

    // Pool management
    bool CreatePool(const std::string& token_a, const std::string& token_b, uint32_t fee_bps = 30);
    LiquidityPool GetPool(const std::string& token_a, const std::string& token_b) const;
    std::vector<LiquidityPool> GetAllPools() const;

    // Liquidity provision
    LiquidityPosition AddLiquidity(
        const std::string& token_a,
        const std::string& token_b,
        uint64_t amount_a,
        uint64_t amount_b
    );

    LiquidityPosition RemoveLiquidity(
        const std::string& token_a,
        const std::string& token_b,
        uint64_t liquidity_tokens
    );

    // Trading
    SwapQuote GetSwapQuote(
        const std::string& token_in,
        const std::string& token_out,
        uint64_t amount_in
    ) const;

    bool ExecuteSwap(
        const std::string& token_in,
        const std::string& token_out,
        uint64_t amount_in,
        uint64_t min_amount_out
    );

    // Price queries
    double GetExchangeRate(const std::string& token_a, const std::string& token_b) const;
    uint64_t GetAmountOut(
        uint64_t amount_in,
        uint64_t reserve_in,
        uint64_t reserve_out,
        uint32_t fee_bps
    ) const;

    // Statistics
    struct PoolStats {
        uint64_t total_volume_24h{0};
        uint64_t total_fees_24h{0};
        uint64_t total_swaps_24h{0};
        double apy{0.0};              // Annual percentage yield for LPs
    };

    PoolStats GetPoolStats(const std::string& token_a, const std::string& token_b) const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace defi
} // namespace intcoin

#endif // INTCOIN_DEFI_AMM_H
