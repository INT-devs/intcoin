// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/consensus.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace intcoin {
namespace consensus {

// DifficultyCalculator implementation

DifficultyCalculator::DifficultyCalculator(const ConsensusParams& params)
    : params_(params) {}

uint32_t DifficultyCalculator::calculate_next_difficulty(
    const Block& prev_block,
    const std::map<uint32_t, Hash256>& block_index,
    const std::unordered_map<Hash256, Block>& blocks
) const {
    // Find previous block height
    uint32_t prev_height = 0;
    for (const auto& [height, hash] : block_index) {
        if (hash == prev_block.get_hash()) {
            prev_height = height;
            break;
        }
    }

    uint32_t current_height = prev_height + 1;

    // Don't adjust difficulty before first interval
    if (current_height < params_.difficulty_adjustment_interval) {
        return prev_block.header.bits;
    }

    // Only adjust at interval boundaries
    if (current_height % params_.difficulty_adjustment_interval != 0) {
        return prev_block.header.bits;
    }

    // If no retargeting, return same difficulty
    if (params_.pow_no_retargeting) {
        return prev_block.header.bits;
    }

    // Find the block at the start of this interval
    uint32_t interval_start_height = current_height - params_.difficulty_adjustment_interval;
    auto interval_start_hash_it = block_index.find(interval_start_height);
    if (interval_start_hash_it == block_index.end()) {
        return prev_block.header.bits;
    }

    auto interval_start_block_it = blocks.find(interval_start_hash_it->second);
    if (interval_start_block_it == blocks.end()) {
        return prev_block.header.bits;
    }

    const Block& interval_start_block = interval_start_block_it->second;

    // Calculate actual timespan
    int64_t actual_timespan = prev_block.header.timestamp - interval_start_block.header.timestamp;

    // Calculate new difficulty
    return calculate_next_work_required(prev_block.header.bits, actual_timespan);
}

uint32_t DifficultyCalculator::calculate_next_work_required(
    uint32_t prev_bits,
    int64_t actual_timespan
) const {
    // Clamp timespan to prevent extreme changes
    // Allow 4x increase or 1/4 decrease
    int64_t target_timespan = params_.target_timespan;

    if (actual_timespan < target_timespan / 4) {
        actual_timespan = target_timespan / 4;
    }
    if (actual_timespan > target_timespan * 4) {
        actual_timespan = target_timespan * 4;
    }

    // Convert bits to target
    uint32_t exponent = prev_bits >> 24;
    uint32_t mantissa = prev_bits & 0x00ffffff;

    // Calculate new target: target * actual_timespan / target_timespan
    uint64_t new_target = mantissa;
    new_target *= actual_timespan;
    new_target /= target_timespan;

    // Adjust exponent if mantissa overflowed
    while (new_target > 0x00ffffff) {
        new_target >>= 8;
        exponent++;
    }

    // Ensure we don't go below the PoW limit
    uint32_t new_bits = (exponent << 24) | (new_target & 0x00ffffff);

    // Apply PoW limit
    if (new_bits > params_.pow_limit) {
        new_bits = params_.pow_limit;
    }

    return new_bits;
}

double DifficultyCalculator::get_difficulty(uint32_t bits) {
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00ffffff;

    if (mantissa == 0 || exponent == 0) {
        return 0.0;
    }

    // Difficulty = max_target / current_target
    // max_target corresponds to bits 0x1d00ffff
    double max_target = 0x00ffff * std::pow(256, 0x1d - 3);
    double current_target = mantissa * std::pow(256, exponent - 3);

    if (current_target <= 0) {
        return 0.0;
    }

    return max_target / current_target;
}

uint32_t DifficultyCalculator::difficulty_to_bits(double difficulty) {
    if (difficulty <= 0) {
        return 0x1d00ffff;  // Minimum difficulty
    }

    // Convert difficulty back to bits
    double max_target = 0x00ffff * std::pow(256, 0x1d - 3);
    double target = max_target / difficulty;

    // Find exponent
    uint32_t exponent = 3;
    while (target >= 256.0 && exponent < 32) {
        target /= 256.0;
        exponent++;
    }

    uint32_t mantissa = static_cast<uint32_t>(target);
    if (mantissa > 0x00ffffff) {
        mantissa = 0x00ffffff;
    }

    return (exponent << 24) | mantissa;
}

bool DifficultyCalculator::check_proof_of_work(const Hash256& hash, uint32_t bits) {
    // Convert bits to target
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00ffffff;

    // Build target as 256-bit number
    Hash256 target{};

    if (exponent <= 3) {
        // Target fits in first few bytes
        for (uint32_t i = 0; i < 3 && i < exponent; ++i) {
            target[i] = (mantissa >> (8 * (2 - i))) & 0xff;
        }
    } else {
        // Larger exponent
        size_t offset = exponent - 3;
        if (offset < 32) {
            target[offset] = (mantissa >> 16) & 0xff;
            if (offset + 1 < 32) target[offset + 1] = (mantissa >> 8) & 0xff;
            if (offset + 2 < 32) target[offset + 2] = mantissa & 0xff;
        }
    }

    // Compare hash <= target (in little-endian)
    for (int i = 31; i >= 0; --i) {
        if (hash[i] < target[i]) return true;
        if (hash[i] > target[i]) return false;
    }
    return true;  // Equal is valid
}

// ForkDetector implementation

ForkDetector::ForkDetector(const ConsensusParams& params)
    : params_(params) {}

std::vector<ForkDetector::ChainInfo> ForkDetector::detect_forks(
    const std::map<uint32_t, Hash256>& block_index,
    const std::unordered_map<Hash256, Block>& blocks
) const {
    std::vector<ChainInfo> chains;

    // Multi-chain fork detection
    // Tracks orphan blocks and identifies competing chain tips
    // In a full implementation, would maintain a tree of all valid blocks
    // and identify all chain tips with their cumulative work
    //
    // For now, implementation tracks the main chain only
    // Future enhancement: maintain orphan block pool and detect forks with
    // work comparison to determine the best chain

    if (block_index.empty()) {
        return chains;
    }

    // Get current chain tip
    uint32_t max_height = block_index.rbegin()->first;
    const Hash256& tip_hash = block_index.rbegin()->second;

    ChainInfo main_chain;
    main_chain.tip_hash = tip_hash;
    main_chain.height = max_height;
    main_chain.total_work = calculate_chain_work(tip_hash, blocks);

    // Build chain hashes
    Hash256 current = tip_hash;
    while (true) {
        main_chain.chain_hashes.push_back(current);

        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        current = it->second.header.previous_block_hash;
        if (current == Hash256{}) break;  // Genesis
    }

    std::reverse(main_chain.chain_hashes.begin(), main_chain.chain_hashes.end());
    chains.push_back(main_chain);

    return chains;
}

ForkDetector::ChainInfo ForkDetector::select_best_chain(
    const std::vector<ChainInfo>& chains
) const {
    if (chains.empty()) {
        return ChainInfo{};
    }

    // Select chain with most work
    const ChainInfo* best = &chains[0];
    for (const auto& chain : chains) {
        if (chain.total_work > best->total_work) {
            best = &chain;
        } else if (chain.total_work == best->total_work && chain.height > best->height) {
            // If equal work, prefer longer chain
            best = &chain;
        }
    }

    return *best;
}

double ForkDetector::calculate_chain_work(
    const Hash256& tip_hash,
    const std::unordered_map<Hash256, Block>& blocks
) const {
    double total_work = 0.0;
    Hash256 current = tip_hash;

    while (true) {
        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        const Block& block = it->second;

        // Work = 2^256 / (target + 1)
        double difficulty = DifficultyCalculator::get_difficulty(block.header.bits);
        total_work += difficulty;

        current = block.header.previous_block_hash;
        if (current == Hash256{}) break;  // Genesis
    }

    return total_work;
}

// ReorgHandler implementation

Hash256 ReorgHandler::find_common_ancestor(
    const Hash256& old_tip,
    const Hash256& new_tip,
    const std::unordered_map<Hash256, Block>& blocks
) {
    // Build path from old_tip to genesis
    std::vector<Hash256> old_chain;
    Hash256 current = old_tip;
    while (true) {
        old_chain.push_back(current);

        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        current = it->second.header.previous_block_hash;
        if (current == Hash256{}) break;
    }

    // Walk back from new_tip until we find a block in old_chain
    current = new_tip;
    while (true) {
        if (std::find(old_chain.begin(), old_chain.end(), current) != old_chain.end()) {
            return current;  // Found common ancestor
        }

        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        current = it->second.header.previous_block_hash;
        if (current == Hash256{}) break;
    }

    return Hash256{};  // No common ancestor (shouldn't happen)
}

ReorgHandler::ReorgInfo ReorgHandler::calculate_reorg(
    const Hash256& old_tip,
    const Hash256& new_tip,
    const std::unordered_map<Hash256, Block>& blocks
) {
    ReorgInfo info;

    // Find common ancestor
    info.common_ancestor = find_common_ancestor(old_tip, new_tip, blocks);

    // Build list of blocks to disconnect (old chain)
    Hash256 current = old_tip;
    while (current != info.common_ancestor) {
        info.disconnect_blocks.push_back(current);

        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        current = it->second.header.previous_block_hash;
    }

    // Build list of blocks to connect (new chain)
    current = new_tip;
    std::vector<Hash256> temp;
    while (current != info.common_ancestor) {
        temp.push_back(current);

        auto it = blocks.find(current);
        if (it == blocks.end()) break;

        current = it->second.header.previous_block_hash;
    }

    // Reverse to get correct order (from ancestor to tip)
    info.connect_blocks.assign(temp.rbegin(), temp.rend());
    info.reorg_depth = info.disconnect_blocks.size();

    return info;
}

bool ReorgHandler::validate_reorg(const ReorgInfo& reorg, uint32_t max_depth) {
    // Check if reorg is too deep
    if (reorg.reorg_depth > max_depth) {
        return false;
    }

    // Ensure we have blocks to connect
    if (reorg.connect_blocks.empty()) {
        return false;
    }

    return true;
}

// CheckpointSystem implementation

CheckpointSystem::CheckpointSystem(const ConsensusParams& params) {
    checkpoints_ = params.checkpoints;
}

void CheckpointSystem::add_checkpoint(uint32_t height, const Hash256& hash) {
    checkpoints_[height] = hash;
}

bool CheckpointSystem::verify_checkpoint(uint32_t height, const Hash256& hash) const {
    auto it = checkpoints_.find(height);
    if (it == checkpoints_.end()) {
        return true;  // No checkpoint at this height
    }
    return it->second == hash;
}

std::optional<std::pair<uint32_t, Hash256>> CheckpointSystem::get_last_checkpoint(
    uint32_t height
) const {
    // Find highest checkpoint <= height
    for (auto it = checkpoints_.rbegin(); it != checkpoints_.rend(); ++it) {
        if (it->first <= height) {
            return std::make_pair(it->first, it->second);
        }
    }
    return std::nullopt;
}

bool CheckpointSystem::reorg_violates_checkpoint(
    uint32_t reorg_height,
    const Hash256& new_hash
) const {
    // Check if any checkpoint would be affected by this reorg
    for (const auto& [cp_height, cp_hash] : checkpoints_) {
        if (cp_height >= reorg_height) {
            // This checkpoint would be affected
            // Verify the new hash matches
            if (cp_height == reorg_height && new_hash != cp_hash) {
                return true;  // Violates checkpoint
            }
        }
    }
    return false;
}

// Default parameters

ConsensusParams get_mainnet_params() {
    ConsensusParams params;

    // INTcoin mainnet parameters
    params.difficulty_adjustment_interval = 2016;
    params.target_timespan = 2016 * 120;     // ~4.67 days (2016 blocks * 2 minutes)
    params.target_spacing = 120;              // 2 minutes
    params.pow_limit = 0x1d00ffff;
    params.pow_no_retargeting = false;
    params.max_reorg_depth = 100;

    // Add mainnet checkpoints
    // Checkpoints are added as the network grows to prevent deep reorganizations
    // Genesis block checkpoint (placeholder - will be set when network launches)
    Hash256 genesis_hash{};  // Will be set to actual genesis block hash
    params.checkpoints[0] = genesis_hash;

    // Additional checkpoints will be added periodically (every ~10,000 blocks)
    // Format: params.checkpoints[height] = block_hash;
    // These are hardcoded after blocks are mined and verified by community

    return params;
}

ConsensusParams get_testnet_params() {
    ConsensusParams params;

    // Faster difficulty adjustment for testing
    params.difficulty_adjustment_interval = 100;
    params.target_timespan = 100 * 120;      // ~3.33 hours (100 blocks * 2 minutes)
    params.target_spacing = 120;              // 2 minutes
    params.pow_limit = 0x1d00ffff;
    params.pow_no_retargeting = false;
    params.max_reorg_depth = 50;

    // Testnet checkpoints
    // params.checkpoints[0] = testnet_genesis_hash;

    return params;
}

} // namespace consensus
} // namespace intcoin
