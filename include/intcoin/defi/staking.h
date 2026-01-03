// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_DEFI_STAKING_H
#define INTCOIN_DEFI_STAKING_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace intcoin {
namespace defi {

/**
 * @brief Staking Protocol - Earn rewards by locking tokens
 *
 * Implements flexible and fixed-term staking with reward distribution.
 * Supports multiple staking pools with different reward rates and lock periods.
 */

// Staking pool configuration
struct StakingPool {
    std::string pool_id;
    std::string reward_token;         // Token distributed as rewards
    uint64_t total_staked{0};         // Total tokens staked in pool
    uint64_t reward_rate{0};          // Rewards per block per token
    uint32_t lock_period{0};          // Lock period in blocks (0 = flexible)
    uint32_t early_withdrawal_penalty{0}; // Penalty in basis points
    bool active{true};
};

// User's staking position
struct StakingPosition {
    std::string user_address;
    std::string pool_id;
    uint64_t staked_amount{0};
    uint64_t pending_rewards{0};
    uint64_t claimed_rewards{0};
    uint64_t stake_timestamp{0};      // Block height when staked
    uint64_t unlock_timestamp{0};     // Block height when unlocked
    double apy{0.0};                  // Current APY
};

// Reward calculation result
struct RewardCalculation {
    uint64_t pending_rewards{0};
    uint64_t blocks_staked{0};
    double effective_apy{0.0};
    uint64_t next_unlock_block{0};
};

/**
 * @class StakingProtocol
 * @brief Implements token staking with reward distribution
 */
class StakingProtocol {
public:
    StakingProtocol();
    ~StakingProtocol();

    // Pool management
    bool CreatePool(
        const std::string& pool_id,
        const std::string& reward_token,
        uint64_t reward_rate,
        uint32_t lock_period,
        uint32_t early_withdrawal_penalty
    );

    bool UpdatePoolRewardRate(const std::string& pool_id, uint64_t new_reward_rate);
    bool DeactivatePool(const std::string& pool_id);

    StakingPool GetPool(const std::string& pool_id) const;
    std::vector<StakingPool> GetActivePools() const;

    // Staking operations
    bool Stake(const std::string& user_address, const std::string& pool_id, uint64_t amount);
    bool Unstake(const std::string& user_address, const std::string& pool_id, uint64_t amount);
    bool ClaimRewards(const std::string& user_address, const std::string& pool_id);
    bool CompoundRewards(const std::string& user_address, const std::string& pool_id);

    // Position queries
    StakingPosition GetUserPosition(const std::string& user_address, const std::string& pool_id) const;
    std::vector<StakingPosition> GetAllUserPositions(const std::string& user_address) const;

    // Reward calculations
    RewardCalculation CalculatePendingRewards(
        const std::string& user_address,
        const std::string& pool_id
    ) const;

    uint64_t GetClaimableRewards(const std::string& user_address, const std::string& pool_id) const;

    // APY calculations
    double CalculatePoolAPY(const std::string& pool_id) const;
    double CalculateUserAPY(const std::string& user_address, const std::string& pool_id) const;

    // Protocol statistics
    struct ProtocolStats {
        uint64_t total_value_staked{0};   // Total value locked in all pools
        uint64_t total_rewards_distributed{0};
        uint32_t num_active_stakers{0};
        uint32_t num_active_pools{0};
    };

    ProtocolStats GetProtocolStats() const;

    // Governance (for DAO-controlled pools)
    struct GovernanceProposal {
        std::string proposal_id;
        std::string description;
        uint64_t voting_power_required{0};
        uint64_t votes_for{0};
        uint64_t votes_against{0};
        bool executed{false};
    };

    bool CreateGovernanceProposal(const std::string& proposal_id, const std::string& description);
    bool VoteOnProposal(const std::string& user_address, const std::string& proposal_id, bool vote_for);
    bool ExecuteProposal(const std::string& proposal_id);
    std::vector<GovernanceProposal> GetActiveProposals() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace defi
} // namespace intcoin

#endif // INTCOIN_DEFI_STAKING_H
