// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONSENSUS_H
#define INTCOIN_CONSENSUS_H

#include "primitives.h"
#include "block.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>

namespace intcoin {
namespace consensus {

/**
 * Consensus parameters for INTcoin
 */
struct ConsensusParams {
    // Difficulty adjustment
    uint32_t difficulty_adjustment_interval = 2016;  // ~4.67 days at 2 min/block
    uint32_t target_timespan = 2016 * 120;           // ~4.67 days in seconds
    uint32_t target_spacing = 120;                   // 2 minutes in seconds

    // Difficulty limits
    uint32_t pow_limit = 0x1d00ffff;  // Initial difficulty (Bitcoin-compatible)
    uint32_t pow_no_retargeting = false;  // Allow difficulty adjustment

    // Fork detection
    uint32_t max_reorg_depth = 100;  // Maximum blocks to reorganize

    // Checkpoints (block_height -> block_hash)
    std::map<uint32_t, Hash256> checkpoints;
};

/**
 * Advanced difficulty adjustment algorithm
 *
 * Implements Bitcoin-style difficulty adjustment with improvements:
 * - Prevents difficulty from changing too rapidly
 * - Clamps adjustment to reasonable bounds
 * - Handles edge cases (genesis, early blocks)
 */
class DifficultyCalculator {
public:
    DifficultyCalculator(const ConsensusParams& params);

    /**
     * Calculate next difficulty target
     * @param prev_block Previous block
     * @param block_index Map of height -> block hash
     * @param blocks Map of block hash -> block
     * @return New difficulty bits
     */
    uint32_t calculate_next_difficulty(
        const Block& prev_block,
        const std::map<uint32_t, Hash256>& block_index,
        const std::unordered_map<Hash256, Block>& blocks
    ) const;

    /**
     * Get difficulty from bits
     */
    static double get_difficulty(uint32_t bits);

    /**
     * Convert difficulty target to bits
     */
    static uint32_t difficulty_to_bits(double difficulty);

    /**
     * Verify block meets difficulty target
     */
    static bool check_proof_of_work(const Hash256& hash, uint32_t bits);

private:
    ConsensusParams params_;

    uint32_t calculate_next_work_required(
        uint32_t prev_bits,
        int64_t actual_timespan
    ) const;
};

/**
 * Fork detector and chain selector
 *
 * Detects competing chains and selects the best chain based on:
 * - Total accumulated work
 * - Chain length
 * - Checkpoint validation
 */
class ForkDetector {
public:
    ForkDetector(const ConsensusParams& params);

    struct ChainInfo {
        Hash256 tip_hash;
        uint32_t height;
        double total_work;
        std::vector<Hash256> chain_hashes;
    };

    /**
     * Detect if there are competing chains
     * @param chains All known chain tips
     * @return Detected forks
     */
    std::vector<ChainInfo> detect_forks(
        const std::map<uint32_t, Hash256>& block_index,
        const std::unordered_map<Hash256, Block>& blocks
    ) const;

    /**
     * Select the best chain from competing forks
     * @param chains Competing chains
     * @return Best chain info
     */
    ChainInfo select_best_chain(const std::vector<ChainInfo>& chains) const;

    /**
     * Calculate total work for a chain
     */
    double calculate_chain_work(
        const Hash256& tip_hash,
        const std::unordered_map<Hash256, Block>& blocks
    ) const;

private:
    ConsensusParams params_;
};

/**
 * Chain reorganization handler
 *
 * Handles switching from one chain to another:
 * - Finds common ancestor
 * - Disconnects old chain
 * - Connects new chain
 * - Updates UTXO set
 */
class ReorgHandler {
public:
    struct ReorgInfo {
        Hash256 common_ancestor;
        std::vector<Hash256> disconnect_blocks;  // Old chain blocks to disconnect
        std::vector<Hash256> connect_blocks;     // New chain blocks to connect
        uint32_t reorg_depth;
    };

    /**
     * Find common ancestor between two chains
     */
    static Hash256 find_common_ancestor(
        const Hash256& old_tip,
        const Hash256& new_tip,
        const std::unordered_map<Hash256, Block>& blocks
    );

    /**
     * Calculate reorganization plan
     */
    static ReorgInfo calculate_reorg(
        const Hash256& old_tip,
        const Hash256& new_tip,
        const std::unordered_map<Hash256, Block>& blocks
    );

    /**
     * Validate reorganization is safe
     * @param reorg Reorganization info
     * @param max_depth Maximum allowed reorg depth
     * @return true if reorg is safe
     */
    static bool validate_reorg(const ReorgInfo& reorg, uint32_t max_depth);
};

/**
 * Checkpoint system
 *
 * Hardcoded checkpoints prevent deep reorganizations:
 * - Validates chain against known checkpoints
 * - Rejects chains that don't match checkpoints
 * - Provides security against long-range attacks
 */
class CheckpointSystem {
public:
    CheckpointSystem(const ConsensusParams& params);

    /**
     * Add a checkpoint
     */
    void add_checkpoint(uint32_t height, const Hash256& hash);

    /**
     * Verify block hash matches checkpoint
     */
    bool verify_checkpoint(uint32_t height, const Hash256& hash) const;

    /**
     * Get last checkpoint before height
     */
    std::optional<std::pair<uint32_t, Hash256>> get_last_checkpoint(uint32_t height) const;

    /**
     * Check if reorganization conflicts with checkpoints
     */
    bool reorg_violates_checkpoint(
        uint32_t reorg_height,
        const Hash256& new_hash
    ) const;

    /**
     * Get all checkpoints
     */
    const std::map<uint32_t, Hash256>& get_checkpoints() const { return checkpoints_; }

private:
    std::map<uint32_t, Hash256> checkpoints_;
};

/**
 * Default consensus parameters for mainnet
 */
ConsensusParams get_mainnet_params();

/**
 * Default consensus parameters for testnet
 */
ConsensusParams get_testnet_params();

} // namespace consensus
} // namespace intcoin

#endif // INTCOIN_CONSENSUS_H
