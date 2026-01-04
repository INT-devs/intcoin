// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/defi/staking.h>
#include <map>
#include <mutex>
#include <chrono>
#include <algorithm>

namespace intcoin {
namespace defi {

class StakingProtocol::Impl {
public:
    std::map<std::string, StakingPool> pools_;
    std::map<std::string, std::map<std::string, StakingPosition>> positions_; // user -> pool -> position
    std::map<std::string, GovernanceProposal> proposals_;
    std::map<std::string, std::map<std::string, bool>> votes_; // proposal -> user -> vote
    std::mutex mutex_;

    uint64_t GetCurrentBlockHeight() const {
        // TODO: Get from blockchain
        return std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    }
};

StakingProtocol::StakingProtocol()
    : pimpl_(std::make_unique<Impl>()) {}

StakingProtocol::~StakingProtocol() = default;

bool StakingProtocol::CreatePool(
    const std::string& pool_id,
    const std::string& reward_token,
    uint64_t reward_rate,
    uint32_t lock_period,
    uint32_t early_withdrawal_penalty
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) > 0) {
        return false; // Pool already exists
    }

    StakingPool pool;
    pool.pool_id = pool_id;
    pool.reward_token = reward_token;
    pool.reward_rate = reward_rate;
    pool.lock_period = lock_period;
    pool.early_withdrawal_penalty = early_withdrawal_penalty;
    pool.active = true;

    pimpl_->pools_[pool_id] = pool;
    return true;
}

bool StakingProtocol::UpdatePoolRewardRate(const std::string& pool_id, uint64_t new_reward_rate) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) == 0) {
        return false;
    }

    pimpl_->pools_[pool_id].reward_rate = new_reward_rate;
    return true;
}

bool StakingProtocol::DeactivatePool(const std::string& pool_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) == 0) {
        return false;
    }

    pimpl_->pools_[pool_id].active = false;
    return true;
}

StakingPool StakingProtocol::GetPool(const std::string& pool_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) == 0) {
        return StakingPool{};
    }

    return pimpl_->pools_.at(pool_id);
}

std::vector<StakingPool> StakingProtocol::GetActivePools() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<StakingPool> pools;
    for (const auto& [id, pool] : pimpl_->pools_) {
        if (pool.active) {
            pools.push_back(pool);
        }
    }
    return pools;
}

bool StakingProtocol::Stake(
    const std::string& user_address,
    const std::string& pool_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) == 0) {
        return false;
    }

    auto& pool = pimpl_->pools_[pool_id];
    if (!pool.active) {
        return false;
    }

    uint64_t current_block = pimpl_->GetCurrentBlockHeight();

    StakingPosition& position = pimpl_->positions_[user_address][pool_id];
    position.user_address = user_address;
    position.pool_id = pool_id;
    position.staked_amount += amount;
    position.stake_timestamp = current_block;
    position.unlock_timestamp = current_block + pool.lock_period;
    position.apy = CalculatePoolAPY(pool_id);

    pool.total_staked += amount;

    return true;
}

bool StakingProtocol::Unstake(
    const std::string& user_address,
    const std::string& pool_id,
    uint64_t amount
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->positions_[user_address].count(pool_id) == 0) {
        return false;
    }

    auto& position = pimpl_->positions_[user_address][pool_id];
    if (position.staked_amount < amount) {
        return false;
    }

    uint64_t current_block = pimpl_->GetCurrentBlockHeight();
    auto& pool = pimpl_->pools_[pool_id];

    // Check if locked
    if (current_block < position.unlock_timestamp && pool.lock_period > 0) {
        // Apply early withdrawal penalty
        uint64_t penalty = (amount * pool.early_withdrawal_penalty) / 10000;
        amount -= penalty;
    }

    position.staked_amount -= amount;
    pool.total_staked -= amount;

    return true;
}

bool StakingProtocol::ClaimRewards(const std::string& user_address, const std::string& pool_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->positions_[user_address].count(pool_id) == 0) {
        return false;
    }

    auto& position = pimpl_->positions_[user_address][pool_id];
    RewardCalculation calc = CalculatePendingRewards(user_address, pool_id);

    position.claimed_rewards += calc.pending_rewards;
    position.pending_rewards = 0;

    return true;
}

bool StakingProtocol::CompoundRewards(const std::string& user_address, const std::string& pool_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->positions_[user_address].count(pool_id) == 0) {
        return false;
    }

    auto& position = pimpl_->positions_[user_address][pool_id];
    RewardCalculation calc = CalculatePendingRewards(user_address, pool_id);

    // Add rewards to staked amount
    position.staked_amount += calc.pending_rewards;
    position.pending_rewards = 0;

    pimpl_->pools_[pool_id].total_staked += calc.pending_rewards;

    return true;
}

StakingPosition StakingProtocol::GetUserPosition(
    const std::string& user_address,
    const std::string& pool_id
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->positions_.count(user_address) == 0 ||
        pimpl_->positions_.at(user_address).count(pool_id) == 0) {
        return StakingPosition{};
    }

    return pimpl_->positions_.at(user_address).at(pool_id);
}

std::vector<StakingPosition> StakingProtocol::GetAllUserPositions(
    const std::string& user_address
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<StakingPosition> positions;
    if (pimpl_->positions_.count(user_address) > 0) {
        for (const auto& [pool_id, position] : pimpl_->positions_.at(user_address)) {
            positions.push_back(position);
        }
    }
    return positions;
}

RewardCalculation StakingProtocol::CalculatePendingRewards(
    const std::string& user_address,
    const std::string& pool_id
) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    RewardCalculation calc;

    if (pimpl_->positions_.count(user_address) == 0 ||
        pimpl_->positions_.at(user_address).count(pool_id) == 0) {
        return calc;
    }

    const auto& position = pimpl_->positions_.at(user_address).at(pool_id);
    const auto& pool = pimpl_->pools_.at(pool_id);

    uint64_t current_block = pimpl_->GetCurrentBlockHeight();
    calc.blocks_staked = current_block - position.stake_timestamp;

    // Rewards = staked_amount * reward_rate * blocks_staked
    calc.pending_rewards = (position.staked_amount * pool.reward_rate * calc.blocks_staked) / 1000000000000;

    calc.effective_apy = CalculateUserAPY(user_address, pool_id);
    calc.next_unlock_block = position.unlock_timestamp;

    return calc;
}

uint64_t StakingProtocol::GetClaimableRewards(
    const std::string& user_address,
    const std::string& pool_id
) const {
    RewardCalculation calc = CalculatePendingRewards(user_address, pool_id);
    return calc.pending_rewards;
}

double StakingProtocol::CalculatePoolAPY(const std::string& pool_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->pools_.count(pool_id) == 0) {
        return 0.0;
    }

    const auto& pool = pimpl_->pools_.at(pool_id);

    if (pool.total_staked == 0) {
        return 0.0;
    }

    // APY = (reward_rate * blocks_per_year) / total_staked * 100
    // Assuming 6 blocks per minute = 3,153,600 blocks per year
    uint64_t blocks_per_year = 6 * 60 * 24 * 365;

    return (static_cast<double>(pool.reward_rate * blocks_per_year) / pool.total_staked) * 100.0;
}

double StakingProtocol::CalculateUserAPY(
    const std::string& user_address,
    const std::string& pool_id
) const {
    // For now, user APY = pool APY
    // In production, could vary based on lock period, etc.
    return CalculatePoolAPY(pool_id);
}

StakingProtocol::ProtocolStats StakingProtocol::GetProtocolStats() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    ProtocolStats stats;

    for (const auto& [id, pool] : pimpl_->pools_) {
        if (pool.active) {
            stats.total_value_staked += pool.total_staked;
            stats.num_active_pools++;
        }
    }

    stats.num_active_stakers = pimpl_->positions_.size();

    return stats;
}

bool StakingProtocol::CreateGovernanceProposal(
    const std::string& proposal_id,
    const std::string& description
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->proposals_.count(proposal_id) > 0) {
        return false;
    }

    GovernanceProposal proposal;
    proposal.proposal_id = proposal_id;
    proposal.description = description;

    pimpl_->proposals_[proposal_id] = proposal;
    return true;
}

bool StakingProtocol::VoteOnProposal(
    const std::string& user_address,
    const std::string& proposal_id,
    bool vote_for
) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->proposals_.count(proposal_id) == 0) {
        return false;
    }

    // Calculate voting power based on staked tokens
    uint64_t voting_power = 0;
    if (pimpl_->positions_.count(user_address) > 0) {
        for (const auto& [pool_id, position] : pimpl_->positions_.at(user_address)) {
            voting_power += position.staked_amount;
        }
    }

    auto& proposal = pimpl_->proposals_[proposal_id];

    if (vote_for) {
        proposal.votes_for += voting_power;
    } else {
        proposal.votes_against += voting_power;
    }

    pimpl_->votes_[proposal_id][user_address] = vote_for;

    return true;
}

bool StakingProtocol::ExecuteProposal(const std::string& proposal_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    if (pimpl_->proposals_.count(proposal_id) == 0) {
        return false;
    }

    auto& proposal = pimpl_->proposals_[proposal_id];

    // Check if proposal has enough votes
    if (proposal.votes_for <= proposal.votes_against) {
        return false;
    }

    // TODO: Execute proposal action

    proposal.executed = true;
    return true;
}

std::vector<StakingProtocol::GovernanceProposal> StakingProtocol::GetActiveProposals() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    std::vector<GovernanceProposal> proposals;
    for (const auto& [id, proposal] : pimpl_->proposals_) {
        if (!proposal.executed) {
            proposals.push_back(proposal);
        }
    }
    return proposals;
}

} // namespace defi
} // namespace intcoin
