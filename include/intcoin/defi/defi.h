// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_DEFI_DEFI_H
#define INTCOIN_DEFI_DEFI_H

#include "../types.h"
#include "../bridge/bridge.h"
#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <mutex>
#include <string>

namespace intcoin {
namespace defi {

//==============================================================================
// Forward Declarations
//==============================================================================

class LiquidityPool;
class YieldFarm;
class CrossChainSwap;

//==============================================================================
// Data Structures
//==============================================================================

/**
 * Asset pair for trading/liquidity
 */
struct AssetPair {
    bridge::ChainType chain_a;
    bridge::ChainType chain_b;
    std::string symbol_a;  // e.g., "INT", "BTC", "ETH"
    std::string symbol_b;
    
    bool operator==(const AssetPair& other) const {
        return chain_a == other.chain_a && chain_b == other.chain_b &&
               symbol_a == other.symbol_a && symbol_b == other.symbol_b;
    }
};

/**
 * Liquidity provider position
 */
struct LiquidityPosition {
    Hash256 position_id;
    PublicKey provider;
    AssetPair pair;
    uint64_t amount_a;
    uint64_t amount_b;
    uint64_t lp_tokens;       // Liquidity provider tokens
    uint64_t timestamp;
    uint64_t rewards_earned;
    bool active;
};

/**
 * Yield farming stake
 */
struct YieldStake {
    Hash256 stake_id;
    PublicKey staker;
    bridge::ChainType chain;
    uint64_t amount;
    uint64_t timestamp;
    uint32_t lock_period;     // Seconds
    double apy;               // Annual Percentage Yield
    uint64_t rewards_claimed;
    bool active;
};

/**
 * Cross-chain swap order
 */
struct SwapOrder {
    Hash256 order_id;
    PublicKey trader;
    bridge::ChainType from_chain;
    bridge::ChainType to_chain;
    uint64_t from_amount;
    uint64_t to_amount;
    uint64_t timestamp;
    uint32_t deadline;
    Hash256 htlc_hash;
    enum class Status {
        PENDING,
        MATCHED,
        EXECUTING,
        COMPLETED,
        CANCELLED,
        EXPIRED
    } status;
};

/**
 * Pool statistics
 */
struct PoolStats {
    uint64_t total_liquidity_a;
    uint64_t total_liquidity_b;
    uint64_t total_volume_24h;
    uint64_t total_fees_24h;
    uint32_t num_providers;
    double current_price;      // Token B per Token A
    double price_change_24h;   // Percentage
};

/**
 * Farm statistics
 */
struct FarmStats {
    uint64_t total_staked;
    uint64_t total_rewards_distributed;
    uint32_t num_stakers;
    double current_apy;
    uint64_t pool_balance;
};

//==============================================================================
// Liquidity Pool (AMM)
//==============================================================================

/**
 * Automated Market Maker liquidity pool
 * Implements constant product formula (x * y = k)
 */
class LiquidityPool {
public:
    LiquidityPool(const AssetPair& pair, uint64_t initial_a, uint64_t initial_b);
    ~LiquidityPool() = default;

    // Pool operations
    Hash256 add_liquidity(const PublicKey& provider, uint64_t amount_a, uint64_t amount_b);
    bool remove_liquidity(const Hash256& position_id, uint64_t lp_tokens);
    
    // Swap operations
    uint64_t calculate_swap_output(uint64_t input_amount, bool a_to_b) const;
    bool execute_swap(const PublicKey& trader, uint64_t input_amount, 
                     uint64_t min_output, bool a_to_b);
    
    // Queries
    AssetPair get_pair() const { return pair_; }
    PoolStats get_stats() const;
    std::optional<LiquidityPosition> get_position(const Hash256& position_id) const;
    std::vector<LiquidityPosition> get_positions_by_provider(const PublicKey& provider) const;
    
    // Price calculations
    double get_price() const;  // Token B per Token A
    uint64_t get_reserve_a() const { return reserve_a_; }
    uint64_t get_reserve_b() const { return reserve_b_; }
    
    // Fee management
    void set_fee_rate(double fee_rate);  // e.g., 0.003 for 0.3%
    double get_fee_rate() const { return fee_rate_; }

private:
    AssetPair pair_;
    uint64_t reserve_a_;
    uint64_t reserve_b_;
    uint64_t total_lp_supply_;
    double fee_rate_;  // Trading fee (e.g., 0.003 = 0.3%)
    
    std::map<Hash256, LiquidityPosition> positions_;
    mutable std::mutex pool_mutex_;
    
    // Statistics tracking
    uint64_t volume_24h_;
    uint64_t fees_24h_;
    uint64_t last_stats_reset_;
    
    uint64_t calculate_lp_tokens(uint64_t amount_a, uint64_t amount_b) const;
    void update_stats(uint64_t volume, uint64_t fees);
};

//==============================================================================
// Yield Farm
//==============================================================================

/**
 * Cross-chain yield farming contract
 */
class YieldFarm {
public:
    YieldFarm(bridge::ChainType reward_chain, double base_apy);
    ~YieldFarm() = default;

    // Staking operations
    Hash256 stake(const PublicKey& staker, uint64_t amount, uint32_t lock_period);
    bool unstake(const Hash256& stake_id);
    bool claim_rewards(const Hash256& stake_id);
    
    // Queries
    std::optional<YieldStake> get_stake(const Hash256& stake_id) const;
    std::vector<YieldStake> get_stakes_by_staker(const PublicKey& staker) const;
    FarmStats get_stats() const;
    
    // Reward calculations
    uint64_t calculate_pending_rewards(const Hash256& stake_id) const;
    double calculate_apy(uint32_t lock_period) const;
    
    // Pool management
    void add_rewards(uint64_t amount);
    void set_base_apy(double apy);

private:
    bridge::ChainType reward_chain_;
    double base_apy_;
    uint64_t reward_pool_;
    uint64_t total_staked_;
    
    std::map<Hash256, YieldStake> stakes_;
    mutable std::mutex farm_mutex_;
    
    double get_lock_period_multiplier(uint32_t lock_period) const;
};

//==============================================================================
// Cross-Chain Swap Router
//==============================================================================

/**
 * Cross-chain swap order matching and execution
 */
class CrossChainRouter {
public:
    CrossChainRouter(bridge::BridgeManager* bridge_manager);
    ~CrossChainRouter() = default;

    // Order operations
    Hash256 create_swap_order(const PublicKey& trader,
                              bridge::ChainType from_chain,
                              bridge::ChainType to_chain,
                              uint64_t from_amount,
                              uint64_t min_to_amount,
                              uint32_t deadline);
    
    bool cancel_order(const Hash256& order_id);
    bool execute_order(const Hash256& order_id);
    
    // Queries
    std::optional<SwapOrder> get_order(const Hash256& order_id) const;
    std::vector<SwapOrder> get_pending_orders() const;
    std::vector<SwapOrder> get_orders_by_trader(const PublicKey& trader) const;
    
    // Price discovery
    uint64_t estimate_output(bridge::ChainType from_chain,
                            bridge::ChainType to_chain,
                            uint64_t from_amount) const;

private:
    bridge::BridgeManager* bridge_manager_;
    std::map<Hash256, SwapOrder> orders_;
    mutable std::mutex router_mutex_;
    
    bool match_and_execute(const Hash256& order_id);
};

//==============================================================================
// DeFi Manager
//==============================================================================

/**
 * Main DeFi protocol coordinator
 */
class DeFiManager {
public:
    DeFiManager(bridge::BridgeManager* bridge_manager);
    ~DeFiManager();

    // Pool management
    bool create_pool(const AssetPair& pair, uint64_t initial_a, uint64_t initial_b);
    std::shared_ptr<LiquidityPool> get_pool(const AssetPair& pair);
    std::vector<AssetPair> get_available_pools() const;
    
    // Farm management
    bool create_farm(bridge::ChainType reward_chain, double base_apy);
    std::shared_ptr<YieldFarm> get_farm(bridge::ChainType chain);
    
    // Router access
    CrossChainRouter* get_router() { return router_.get(); }
    
    // Statistics
    struct TotalStats {
        uint64_t total_liquidity_usd;
        uint64_t total_volume_24h_usd;
        uint64_t total_staked_usd;
        uint32_t num_pools;
        uint32_t num_farms;
        uint32_t num_active_orders;
    };
    
    TotalStats get_total_stats() const;

private:
    bridge::BridgeManager* bridge_manager_;
    std::map<AssetPair, std::shared_ptr<LiquidityPool>> pools_;
    std::map<bridge::ChainType, std::shared_ptr<YieldFarm>> farms_;
    std::unique_ptr<CrossChainRouter> router_;
    
    mutable std::mutex manager_mutex_;
};

//==============================================================================
// Utility Functions
//==============================================================================

namespace utils {

/**
 * Convert amount between different chain decimals
 */
uint64_t convert_chain_amount(uint64_t amount, 
                               bridge::ChainType from_chain,
                               bridge::ChainType to_chain);

/**
 * Calculate price impact of a swap
 */
double calculate_price_impact(uint64_t input_amount,
                              uint64_t reserve_in,
                              uint64_t reserve_out);

/**
 * Get USD value estimate for asset
 */
uint64_t estimate_usd_value(bridge::ChainType chain, uint64_t amount);

/**
 * Calculate impermanent loss
 */
double calculate_impermanent_loss(double price_ratio_initial,
                                   double price_ratio_current);

} // namespace utils

} // namespace defi
} // namespace intcoin

#endif // INTCOIN_DEFI_DEFI_H
