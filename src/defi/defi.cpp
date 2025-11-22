// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/defi/defi.h"
#include "../../include/intcoin/crypto/hash.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>

namespace intcoin {
namespace defi {

//==============================================================================
// LiquidityPool Implementation
//==============================================================================

LiquidityPool::LiquidityPool(const AssetPair& pair, uint64_t initial_a, uint64_t initial_b)
    : pair_(pair),
      reserve_a_(initial_a),
      reserve_b_(initial_b),
      total_lp_supply_(0),
      fee_rate_(0.003),  // 0.3% default fee
      volume_24h_(0),
      fees_24h_(0),
      last_stats_reset_(std::time(nullptr)) {
    
    if (initial_a == 0 || initial_b == 0) {
        throw std::runtime_error("Initial reserves must be non-zero");
    }
    
    // Initial LP tokens = sqrt(a * b)
    total_lp_supply_ = static_cast<uint64_t>(std::sqrt(
        static_cast<double>(initial_a) * static_cast<double>(initial_b)
    ));
}

Hash256 LiquidityPool::add_liquidity(const PublicKey& provider, uint64_t amount_a, uint64_t amount_b) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Calculate optimal amounts based on current ratio
    uint64_t optimal_b = (amount_a * reserve_b_) / reserve_a_;
    
    if (amount_b < optimal_b) {
        // Adjust amount_a to match provided amount_b
        amount_a = (amount_b * reserve_a_) / reserve_b_;
        optimal_b = amount_b;
    } else {
        amount_b = optimal_b;
    }
    
    // Calculate LP tokens to mint
    uint64_t lp_tokens = calculate_lp_tokens(amount_a, amount_b);
    
    // Create position
    Hash256 position_id;
    crypto::SHA256 hasher;
    hasher.update(provider.data(), provider.size());
    hasher.update(reinterpret_cast<const uint8_t*>(&amount_a), sizeof(amount_a));
    hasher.update(reinterpret_cast<const uint8_t*>(&amount_b), sizeof(amount_b));
    uint64_t timestamp = std::time(nullptr);
    hasher.update(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    hasher.finalize(position_id.data());
    
    LiquidityPosition position;
    position.position_id = position_id;
    position.provider = provider;
    position.pair = pair_;
    position.amount_a = amount_a;
    position.amount_b = amount_b;
    position.lp_tokens = lp_tokens;
    position.timestamp = timestamp;
    position.rewards_earned = 0;
    position.active = true;
    
    positions_[position_id] = position;
    
    // Update reserves and LP supply
    reserve_a_ += amount_a;
    reserve_b_ += amount_b;
    total_lp_supply_ += lp_tokens;
    
    std::cout << "Added liquidity: " << amount_a << " / " << amount_b 
              << " -> " << lp_tokens << " LP tokens" << std::endl;
    
    return position_id;
}

bool LiquidityPool::remove_liquidity(const Hash256& position_id, uint64_t lp_tokens) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto it = positions_.find(position_id);
    if (it == positions_.end() || !it->second.active) {
        return false;
    }
    
    LiquidityPosition& position = it->second;
    
    if (lp_tokens > position.lp_tokens) {
        return false;
    }
    
    // Calculate amounts to return
    uint64_t amount_a = (lp_tokens * reserve_a_) / total_lp_supply_;
    uint64_t amount_b = (lp_tokens * reserve_b_) / total_lp_supply_;
    
    // Update position
    position.lp_tokens -= lp_tokens;
    if (position.lp_tokens == 0) {
        position.active = false;
    }
    
    // Update reserves and LP supply
    reserve_a_ -= amount_a;
    reserve_b_ -= amount_b;
    total_lp_supply_ -= lp_tokens;
    
    std::cout << "Removed liquidity: " << lp_tokens << " LP tokens -> " 
              << amount_a << " / " << amount_b << std::endl;
    
    return true;
}

uint64_t LiquidityPool::calculate_swap_output(uint64_t input_amount, bool a_to_b) const {
    if (input_amount == 0) {
        return 0;
    }
    
    uint64_t reserve_in = a_to_b ? reserve_a_ : reserve_b_;
    uint64_t reserve_out = a_to_b ? reserve_b_ : reserve_a_;
    
    // Apply fee
    uint64_t input_with_fee = input_amount * (1000 - static_cast<uint64_t>(fee_rate_ * 1000));
    input_with_fee /= 1000;
    
    // Constant product formula: (x + Δx) * (y - Δy) = x * y
    // Δy = (y * Δx) / (x + Δx)
    uint64_t output_amount = (reserve_out * input_with_fee) / (reserve_in + input_with_fee);
    
    return output_amount;
}

bool LiquidityPool::execute_swap(const PublicKey& trader, uint64_t input_amount, 
                                 uint64_t min_output, bool a_to_b) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    uint64_t output_amount = calculate_swap_output(input_amount, a_to_b);
    
    if (output_amount < min_output) {
        std::cerr << "Slippage too high: output " << output_amount 
                  << " < minimum " << min_output << std::endl;
        return false;
    }
    
    // Calculate fee
    uint64_t fee = input_amount * fee_rate_;
    
    // Update reserves
    if (a_to_b) {
        reserve_a_ += input_amount;
        reserve_b_ -= output_amount;
    } else {
        reserve_b_ += input_amount;
        reserve_a_ -= output_amount;
    }
    
    // Update statistics
    update_stats(input_amount, fee);
    
    std::cout << "Swap executed: " << input_amount << " -> " << output_amount 
              << " (fee: " << fee << ")" << std::endl;
    
    return true;
}

PoolStats LiquidityPool::get_stats() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    PoolStats stats;
    stats.total_liquidity_a = reserve_a_;
    stats.total_liquidity_b = reserve_b_;
    stats.total_volume_24h = volume_24h_;
    stats.total_fees_24h = fees_24h_;
    stats.num_providers = 0;
    
    for (const auto& [id, pos] : positions_) {
        if (pos.active) {
            stats.num_providers++;
        }
    }
    
    stats.current_price = get_price();
    stats.price_change_24h = 0.0;  // Would need historical data
    
    return stats;
}

std::optional<LiquidityPosition> LiquidityPool::get_position(const Hash256& position_id) const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto it = positions_.find(position_id);
    if (it == positions_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<LiquidityPosition> LiquidityPool::get_positions_by_provider(const PublicKey& provider) const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    std::vector<LiquidityPosition> result;
    for (const auto& [id, pos] : positions_) {
        if (pos.provider == provider && pos.active) {
            result.push_back(pos);
        }
    }
    
    return result;
}

double LiquidityPool::get_price() const {
    if (reserve_a_ == 0) return 0.0;
    return static_cast<double>(reserve_b_) / static_cast<double>(reserve_a_);
}

void LiquidityPool::set_fee_rate(double fee_rate) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    fee_rate_ = fee_rate;
}

uint64_t LiquidityPool::calculate_lp_tokens(uint64_t amount_a, uint64_t amount_b) const {
    if (total_lp_supply_ == 0) {
        return static_cast<uint64_t>(std::sqrt(
            static_cast<double>(amount_a) * static_cast<double>(amount_b)
        ));
    }
    
    // Proportional to existing supply
    uint64_t lp_from_a = (amount_a * total_lp_supply_) / reserve_a_;
    uint64_t lp_from_b = (amount_b * total_lp_supply_) / reserve_b_;
    
    return std::min(lp_from_a, lp_from_b);
}

void LiquidityPool::update_stats(uint64_t volume, uint64_t fees) {
    uint64_t current_time = std::time(nullptr);
    
    // Reset 24h stats if needed
    if (current_time - last_stats_reset_ > 86400) {
        volume_24h_ = 0;
        fees_24h_ = 0;
        last_stats_reset_ = current_time;
    }
    
    volume_24h_ += volume;
    fees_24h_ += fees;
}

//==============================================================================
// YieldFarm Implementation
//==============================================================================

YieldFarm::YieldFarm(bridge::ChainType reward_chain, double base_apy)
    : reward_chain_(reward_chain),
      base_apy_(base_apy),
      reward_pool_(0),
      total_staked_(0) {
}

Hash256 YieldFarm::stake(const PublicKey& staker, uint64_t amount, uint32_t lock_period) {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    // Generate stake ID
    Hash256 stake_id;
    crypto::SHA256 hasher;
    hasher.update(staker.data(), staker.size());
    hasher.update(reinterpret_cast<const uint8_t*>(&amount), sizeof(amount));
    uint64_t timestamp = std::time(nullptr);
    hasher.update(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    hasher.finalize(stake_id.data());
    
    // Calculate APY with lock period bonus
    double apy = calculate_apy(lock_period);
    
    // Create stake
    YieldStake stake;
    stake.stake_id = stake_id;
    stake.staker = staker;
    stake.chain = reward_chain_;
    stake.amount = amount;
    stake.timestamp = timestamp;
    stake.lock_period = lock_period;
    stake.apy = apy;
    stake.rewards_claimed = 0;
    stake.active = true;
    
    stakes_[stake_id] = stake;
    total_staked_ += amount;
    
    std::cout << "Staked " << amount << " with APY " << (apy * 100) << "%" << std::endl;
    
    return stake_id;
}

bool YieldFarm::unstake(const Hash256& stake_id) {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    auto it = stakes_.find(stake_id);
    if (it == stakes_.end() || !it->second.active) {
        return false;
    }
    
    YieldStake& stake = it->second;
    
    // Check if lock period has expired
    uint64_t current_time = std::time(nullptr);
    uint64_t unlock_time = stake.timestamp + stake.lock_period;
    
    if (current_time < unlock_time) {
        std::cerr << "Stake still locked for " << (unlock_time - current_time) << " seconds" << std::endl;
        return false;
    }
    
    // Claim any pending rewards first
    uint64_t pending = calculate_pending_rewards(stake_id);
    if (pending > 0 && pending <= reward_pool_) {
        stake.rewards_claimed += pending;
        reward_pool_ -= pending;
    }
    
    // Deactivate stake
    stake.active = false;
    total_staked_ -= stake.amount;
    
    std::cout << "Unstaked " << stake.amount << " with total rewards " 
              << stake.rewards_claimed << std::endl;
    
    return true;
}

bool YieldFarm::claim_rewards(const Hash256& stake_id) {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    auto it = stakes_.find(stake_id);
    if (it == stakes_.end() || !it->second.active) {
        return false;
    }
    
    YieldStake& stake = it->second;
    
    uint64_t pending = calculate_pending_rewards(stake_id);
    
    if (pending == 0) {
        return true;  // Nothing to claim
    }
    
    if (pending > reward_pool_) {
        std::cerr << "Insufficient rewards in pool" << std::endl;
        return false;
    }
    
    stake.rewards_claimed += pending;
    stake.timestamp = std::time(nullptr);  // Reset for next period
    reward_pool_ -= pending;
    
    std::cout << "Claimed rewards: " << pending << std::endl;
    
    return true;
}

std::optional<YieldStake> YieldFarm::get_stake(const Hash256& stake_id) const {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    auto it = stakes_.find(stake_id);
    if (it == stakes_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<YieldStake> YieldFarm::get_stakes_by_staker(const PublicKey& staker) const {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    std::vector<YieldStake> result;
    for (const auto& [id, stake] : stakes_) {
        if (stake.staker == staker && stake.active) {
            result.push_back(stake);
        }
    }
    
    return result;
}

FarmStats YieldFarm::get_stats() const {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    
    FarmStats stats;
    stats.total_staked = total_staked_;
    stats.pool_balance = reward_pool_;
    stats.current_apy = base_apy_;
    stats.num_stakers = 0;
    stats.total_rewards_distributed = 0;
    
    for (const auto& [id, stake] : stakes_) {
        if (stake.active) {
            stats.num_stakers++;
        }
        stats.total_rewards_distributed += stake.rewards_claimed;
    }
    
    return stats;
}

uint64_t YieldFarm::calculate_pending_rewards(const Hash256& stake_id) const {
    auto it = stakes_.find(stake_id);
    if (it == stakes_.end() || !it->second.active) {
        return 0;
    }
    
    const YieldStake& stake = it->second;
    
    uint64_t current_time = std::time(nullptr);
    uint64_t time_staked = current_time - stake.timestamp;
    
    // Calculate rewards: amount * APY * (time / year)
    double years = static_cast<double>(time_staked) / (365.25 * 24 * 3600);
    double rewards = static_cast<double>(stake.amount) * stake.apy * years;
    
    return static_cast<uint64_t>(rewards);
}

double YieldFarm::calculate_apy(uint32_t lock_period) const {
    double multiplier = get_lock_period_multiplier(lock_period);
    return base_apy_ * multiplier;
}

void YieldFarm::add_rewards(uint64_t amount) {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    reward_pool_ += amount;
    std::cout << "Added " << amount << " to reward pool (total: " << reward_pool_ << ")" << std::endl;
}

void YieldFarm::set_base_apy(double apy) {
    std::lock_guard<std::mutex> lock(farm_mutex_);
    base_apy_ = apy;
}

double YieldFarm::get_lock_period_multiplier(uint32_t lock_period) const {
    // Lock period bonuses
    if (lock_period >= 365 * 24 * 3600) {  // 1 year
        return 2.0;  // 2x APY
    } else if (lock_period >= 180 * 24 * 3600) {  // 6 months
        return 1.5;
    } else if (lock_period >= 90 * 24 * 3600) {  // 3 months
        return 1.25;
    } else if (lock_period >= 30 * 24 * 3600) {  // 1 month
        return 1.1;
    }
    
    return 1.0;  // No lock period bonus
}

//==============================================================================
// CrossChainRouter Implementation
//==============================================================================

CrossChainRouter::CrossChainRouter(bridge::BridgeManager* bridge_manager)
    : bridge_manager_(bridge_manager) {
    if (!bridge_manager_) {
        throw std::runtime_error("CrossChainRouter: bridge_manager cannot be null");
    }
}

Hash256 CrossChainRouter::create_swap_order(const PublicKey& trader,
                                            bridge::ChainType from_chain,
                                            bridge::ChainType to_chain,
                                            uint64_t from_amount,
                                            uint64_t min_to_amount,
                                            uint32_t deadline) {
    std::lock_guard<std::mutex> lock(router_mutex_);
    
    // Generate order ID
    Hash256 order_id;
    crypto::SHA256 hasher;
    hasher.update(trader.data(), trader.size());
    hasher.update(reinterpret_cast<const uint8_t*>(&from_amount), sizeof(from_amount));
    uint64_t timestamp = std::time(nullptr);
    hasher.update(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    hasher.finalize(order_id.data());
    
    // Estimate output amount
    uint64_t estimated_output = estimate_output(from_chain, to_chain, from_amount);
    
    if (estimated_output < min_to_amount) {
        throw std::runtime_error("Estimated output below minimum");
    }
    
    // Create HTLC hash
    Hash256 htlc_hash;
    crypto::SHA256 htlc_hasher;
    htlc_hasher.update(order_id.data(), order_id.size());
    htlc_hasher.finalize(htlc_hash.data());
    
    // Create order
    SwapOrder order;
    order.order_id = order_id;
    order.trader = trader;
    order.from_chain = from_chain;
    order.to_chain = to_chain;
    order.from_amount = from_amount;
    order.to_amount = estimated_output;
    order.timestamp = timestamp;
    order.deadline = deadline;
    order.htlc_hash = htlc_hash;
    order.status = SwapOrder::Status::PENDING;
    
    orders_[order_id] = order;
    
    std::cout << "Created swap order: " << from_amount << " "
              << bridge::BridgeUtils::chain_type_to_string(from_chain) << " -> "
              << estimated_output << " "
              << bridge::BridgeUtils::chain_type_to_string(to_chain) << std::endl;
    
    return order_id;
}

bool CrossChainRouter::cancel_order(const Hash256& order_id) {
    std::lock_guard<std::mutex> lock(router_mutex_);
    
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    SwapOrder& order = it->second;
    
    if (order.status != SwapOrder::Status::PENDING) {
        return false;
    }
    
    order.status = SwapOrder::Status::CANCELLED;
    
    std::cout << "Cancelled order " << order_id << std::endl;
    
    return true;
}

bool CrossChainRouter::execute_order(const Hash256& order_id) {
    std::lock_guard<std::mutex> lock(router_mutex_);
    return match_and_execute(order_id);
}

std::optional<SwapOrder> CrossChainRouter::get_order(const Hash256& order_id) const {
    std::lock_guard<std::mutex> lock(router_mutex_);
    
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<SwapOrder> CrossChainRouter::get_pending_orders() const {
    std::lock_guard<std::mutex> lock(router_mutex_);
    
    std::vector<SwapOrder> result;
    for (const auto& [id, order] : orders_) {
        if (order.status == SwapOrder::Status::PENDING) {
            result.push_back(order);
        }
    }
    
    return result;
}

std::vector<SwapOrder> CrossChainRouter::get_orders_by_trader(const PublicKey& trader) const {
    std::lock_guard<std::mutex> lock(router_mutex_);
    
    std::vector<SwapOrder> result;
    for (const auto& [id, order] : orders_) {
        if (order.trader == trader) {
            result.push_back(order);
        }
    }
    
    return result;
}

uint64_t CrossChainRouter::estimate_output(bridge::ChainType from_chain,
                                           bridge::ChainType to_chain,
                                           uint64_t from_amount) const {
    // Simplified: use conversion rates and apply fee
    // In production: query liquidity pools and oracle prices
    
    uint64_t converted = utils::convert_chain_amount(from_amount, from_chain, to_chain);
    
    // Apply 0.5% cross-chain swap fee
    uint64_t fee = converted * 5 / 1000;
    
    return converted - fee;
}

bool CrossChainRouter::match_and_execute(const Hash256& order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    SwapOrder& order = it->second;
    
    // Check deadline
    uint64_t current_time = std::time(nullptr);
    if (current_time > order.deadline) {
        order.status = SwapOrder::Status::EXPIRED;
        return false;
    }
    
    // Get bridges for both chains
    auto from_bridge = bridge_manager_->get_bridge(order.from_chain);
    auto to_bridge = bridge_manager_->get_bridge(order.to_chain);
    
    if (!from_bridge || !to_bridge) {
        return false;
    }
    
    order.status = SwapOrder::Status::EXECUTING;
    
    // Execute via bridge atomic swap
    // In production: create HTLCs on both chains
    
    order.status = SwapOrder::Status::COMPLETED;
    
    std::cout << "Executed cross-chain swap order " << order_id << std::endl;
    
    return true;
}

//==============================================================================
// DeFiManager Implementation
//==============================================================================

DeFiManager::DeFiManager(bridge::BridgeManager* bridge_manager)
    : bridge_manager_(bridge_manager) {
    if (!bridge_manager_) {
        throw std::runtime_error("DeFiManager: bridge_manager cannot be null");
    }
    
    router_ = std::make_unique<CrossChainRouter>(bridge_manager_);
}

DeFiManager::~DeFiManager() = default;

bool DeFiManager::create_pool(const AssetPair& pair, uint64_t initial_a, uint64_t initial_b) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (pools_.find(pair) != pools_.end()) {
        return false;  // Pool already exists
    }
    
    auto pool = std::make_shared<LiquidityPool>(pair, initial_a, initial_b);
    pools_[pair] = pool;
    
    std::cout << "Created liquidity pool for " 
              << pair.symbol_a << "/" << pair.symbol_b << std::endl;
    
    return true;
}

std::shared_ptr<LiquidityPool> DeFiManager::get_pool(const AssetPair& pair) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = pools_.find(pair);
    if (it == pools_.end()) {
        return nullptr;
    }
    
    return it->second;
}

std::vector<AssetPair> DeFiManager::get_available_pools() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    std::vector<AssetPair> result;
    for (const auto& [pair, pool] : pools_) {
        result.push_back(pair);
    }
    
    return result;
}

bool DeFiManager::create_farm(bridge::ChainType reward_chain, double base_apy) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (farms_.find(reward_chain) != farms_.end()) {
        return false;  // Farm already exists
    }
    
    auto farm = std::make_shared<YieldFarm>(reward_chain, base_apy);
    farms_[reward_chain] = farm;
    
    std::cout << "Created yield farm for " 
              << bridge::BridgeUtils::chain_type_to_string(reward_chain)
              << " with APY " << (base_apy * 100) << "%" << std::endl;
    
    return true;
}

std::shared_ptr<YieldFarm> DeFiManager::get_farm(bridge::ChainType chain) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = farms_.find(chain);
    if (it == farms_.end()) {
        return nullptr;
    }
    
    return it->second;
}

DeFiManager::TotalStats DeFiManager::get_total_stats() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    TotalStats stats;
    stats.total_liquidity_usd = 0;
    stats.total_volume_24h_usd = 0;
    stats.total_staked_usd = 0;
    stats.num_pools = pools_.size();
    stats.num_farms = farms_.size();
    stats.num_active_orders = 0;
    
    // Aggregate pool stats
    for (const auto& [pair, pool] : pools_) {
        PoolStats pool_stats = pool->get_stats();
        stats.total_liquidity_usd += utils::estimate_usd_value(pair.chain_a, pool_stats.total_liquidity_a);
        stats.total_liquidity_usd += utils::estimate_usd_value(pair.chain_b, pool_stats.total_liquidity_b);
        stats.total_volume_24h_usd += utils::estimate_usd_value(pair.chain_a, pool_stats.total_volume_24h);
    }
    
    // Aggregate farm stats
    for (const auto& [chain, farm] : farms_) {
        FarmStats farm_stats = farm->get_stats();
        stats.total_staked_usd += utils::estimate_usd_value(chain, farm_stats.total_staked);
    }
    
    // Count active orders
    auto pending_orders = router_->get_pending_orders();
    stats.num_active_orders = pending_orders.size();
    
    return stats;
}

//==============================================================================
// Utility Functions
//==============================================================================

namespace utils {

uint64_t convert_chain_amount(uint64_t amount, 
                               bridge::ChainType from_chain,
                               bridge::ChainType to_chain) {
    // Handle decimal conversions between chains
    // This is a simplified version - production would use oracle prices
    
    if (from_chain == to_chain) {
        return amount;
    }
    
    // Convert to common base (18 decimals like ETH)
    uint64_t base_amount = amount;
    
    // From chain to base
    if (from_chain == bridge::ChainType::BITCOIN || 
        from_chain == bridge::ChainType::LITECOIN ||
        from_chain == bridge::ChainType::INTCOIN) {
        // 8 decimals -> 18 decimals
        base_amount *= 10000000000ULL;
    }
    
    // Base to target chain
    if (to_chain == bridge::ChainType::BITCOIN || 
        to_chain == bridge::ChainType::LITECOIN ||
        to_chain == bridge::ChainType::INTCOIN) {
        // 18 decimals -> 8 decimals
        base_amount /= 10000000000ULL;
    }
    
    return base_amount;
}

double calculate_price_impact(uint64_t input_amount,
                              uint64_t reserve_in,
                              uint64_t reserve_out) {
    if (reserve_in == 0 || reserve_out == 0) {
        return 0.0;
    }
    
    double price_before = static_cast<double>(reserve_out) / static_cast<double>(reserve_in);
    double new_reserve_in = reserve_in + input_amount;
    double new_reserve_out = (static_cast<double>(reserve_in) * static_cast<double>(reserve_out)) / new_reserve_in;
    double price_after = new_reserve_out / new_reserve_in;
    
    return ((price_after - price_before) / price_before) * 100.0;
}

uint64_t estimate_usd_value(bridge::ChainType chain, uint64_t amount) {
    // Placeholder USD values - in production, query oracle
    std::map<bridge::ChainType, double> usd_prices = {
        {bridge::ChainType::BITCOIN, 45000.0},
        {bridge::ChainType::ETHEREUM, 2500.0},
        {bridge::ChainType::LITECOIN, 90.0},
        {bridge::ChainType::CARDANO, 0.5},
        {bridge::ChainType::INTCOIN, 1.0}
    };
    
    auto it = usd_prices.find(chain);
    if (it == usd_prices.end()) {
        return 0;
    }
    
    // Convert to USD (assuming 8 decimal places)
    double value = (static_cast<double>(amount) / 100000000.0) * it->second;
    return static_cast<uint64_t>(value * 100);  // USD cents
}

double calculate_impermanent_loss(double price_ratio_initial,
                                   double price_ratio_current) {
    if (price_ratio_initial == 0) {
        return 0.0;
    }
    
    double ratio = price_ratio_current / price_ratio_initial;
    double hold_value = 2.0;
    double pool_value = 2.0 * std::sqrt(ratio);
    
    double loss = ((pool_value - hold_value) / hold_value) * 100.0;
    
    return loss;
}

} // namespace utils

} // namespace defi
} // namespace intcoin
