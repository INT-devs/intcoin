// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CHAIN_SELECTION_H
#define INTCOIN_CHAIN_SELECTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <algorithm>
#include <limits>
#include <cmath>

namespace intcoin {
namespace chain {

// Chain selection parameters
namespace params {
    // Maximum reorganization depth (prevents deep reorgs)
    constexpr uint32_t MAX_REORG_DEPTH = 100;

    // Checkpoint interval (blocks)
    constexpr uint32_t CHECKPOINT_INTERVAL = 10000;

    // Minimum chain work required for initial sync
    constexpr uint64_t MIN_CHAIN_WORK = 0;  // Set based on network

    // Selfish mining prevention: require this much more work to reorg
    constexpr double REORG_WORK_MULTIPLIER = 1.0;  // Can be > 1.0 for extra security
}

// Block header (minimal information for chain selection)
struct BlockHeader {
    uint32_t version;
    std::string prev_block_hash;
    std::string merkle_root;
    uint32_t timestamp;
    uint32_t bits;           // Compact difficulty target
    uint32_t nonce;
    uint32_t height;

    // Cached values
    mutable std::string block_hash;
    mutable uint64_t chain_work = 0;  // Cumulative work to this block

    // Calculate block hash
    std::string calculate_hash() const {
        if (block_hash.empty()) {
            // In production, this would hash the header
            block_hash = "block_" + std::to_string(height) + "_" + std::to_string(nonce);
        }
        return block_hash;
    }

    // Get difficulty target from compact bits
    uint64_t get_target() const {
        // Simplified: real implementation would decode compact format
        return static_cast<uint64_t>(bits);
    }

    // Calculate work (2^256 / target)
    uint64_t get_work() const {
        uint64_t target = get_target();
        if (target == 0) {
            return std::numeric_limits<uint64_t>::max();
        }
        // Simplified: real implementation would use 256-bit arithmetic
        return std::numeric_limits<uint64_t>::max() / target;
    }
};

// Checkpoint - hardcoded block hash at specific height
struct Checkpoint {
    uint32_t height;
    std::string block_hash;
    uint64_t timestamp;      // For validation
    std::string description; // Human-readable description

    bool operator==(const Checkpoint& other) const {
        return height == other.height && block_hash == other.block_hash;
    }
};

// Chain checkpoint manager
class CheckpointManager {
private:
    // Map of height -> checkpoint
    std::unordered_map<uint32_t, Checkpoint> checkpoints;

    // Statistics
    struct Statistics {
        uint64_t checkpoints_validated = 0;
        uint64_t checkpoint_failures = 0;
        uint64_t blocks_validated = 0;
    } stats;

public:
    // Add hardcoded checkpoints
    void initialize_checkpoints() {
        // Genesis block
        add_checkpoint(Checkpoint{
            0,
            "0000000000000000000000000000000000000000000000000000000000000000",
            1704672000,  // 2024-01-08
            "Genesis block"
        });

        // Add checkpoints at regular intervals for security
        // In production, these would be actual block hashes from the live network

        // Example checkpoints (would be real hashes in production)
        add_checkpoint(Checkpoint{
            10000,
            "checkpoint_10000",
            1704758400,
            "First checkpoint at height 10,000"
        });

        add_checkpoint(Checkpoint{
            50000,
            "checkpoint_50000",
            1706140800,
            "Checkpoint at height 50,000"
        });

        add_checkpoint(Checkpoint{
            100000,
            "checkpoint_100000",
            1708819200,
            "Checkpoint at height 100,000"
        });
    }

    // Add a checkpoint
    bool add_checkpoint(const Checkpoint& checkpoint) {
        // Check if checkpoint already exists at this height
        auto it = checkpoints.find(checkpoint.height);
        if (it != checkpoints.end()) {
            // Verify it matches existing checkpoint
            if (it->second.block_hash != checkpoint.block_hash) {
                return false;  // Conflicting checkpoint!
            }
            return true;  // Already exists, matches
        }

        checkpoints[checkpoint.height] = checkpoint;
        return true;
    }

    // Check if block at height matches checkpoint
    bool validate_checkpoint(uint32_t height, const std::string& block_hash) {
        stats.blocks_validated++;

        auto it = checkpoints.find(height);
        if (it == checkpoints.end()) {
            return true;  // No checkpoint at this height, okay
        }

        stats.checkpoints_validated++;

        bool matches = (it->second.block_hash == block_hash);
        if (!matches) {
            stats.checkpoint_failures++;
        }

        return matches;
    }

    // Check if height has a checkpoint
    bool has_checkpoint(uint32_t height) const {
        return checkpoints.count(height) > 0;
    }

    // Get checkpoint at height
    std::optional<Checkpoint> get_checkpoint(uint32_t height) const {
        auto it = checkpoints.find(height);
        if (it == checkpoints.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // Get last checkpoint before or at height
    std::optional<Checkpoint> get_last_checkpoint(uint32_t height) const {
        std::optional<Checkpoint> last_checkpoint;
        uint32_t max_height = 0;

        for (const auto& [cp_height, checkpoint] : checkpoints) {
            if (cp_height <= height && cp_height > max_height) {
                max_height = cp_height;
                last_checkpoint = checkpoint;
            }
        }

        return last_checkpoint;
    }

    // Get all checkpoints
    std::vector<Checkpoint> get_all_checkpoints() const {
        std::vector<Checkpoint> result;
        for (const auto& [height, checkpoint] : checkpoints) {
            result.push_back(checkpoint);
        }
        // Sort by height
        std::sort(result.begin(), result.end(),
            [](const Checkpoint& a, const Checkpoint& b) {
                return a.height < b.height;
            });
        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Chain state - represents a blockchain
struct ChainState {
    std::string tip_hash;           // Hash of chain tip
    uint32_t height;                // Chain height
    uint64_t total_work;            // Cumulative work
    uint64_t total_transactions;    // Total transactions in chain
    std::string genesis_hash;       // Genesis block hash (for fork detection)

    // Chain validity
    bool is_valid = true;
    std::vector<std::string> validation_errors;

    // Comparison operator (for chain selection)
    bool has_more_work_than(const ChainState& other) const {
        return total_work > other.total_work;
    }
};

// Selfish mining detector
class SelfishMiningDetector {
private:
    struct MiningPattern {
        std::string miner_id;
        std::vector<uint32_t> block_timestamps;
        std::vector<uint32_t> block_heights;
        uint32_t blocks_withheld = 0;
        double suspicious_score = 0.0;
    };

    // Track mining patterns per miner
    std::unordered_map<std::string, MiningPattern> miner_patterns;

    struct Statistics {
        uint64_t blocks_analyzed = 0;
        uint64_t suspicious_patterns = 0;
        uint64_t selfish_mining_detected = 0;
    } stats;

public:
    struct SelfishMiningReport {
        bool is_suspicious;
        std::string miner_id;
        double suspicious_score;  // 0.0 - 1.0
        std::vector<std::string> evidence;
    };

    // Analyze block for selfish mining patterns
    SelfishMiningReport analyze_block(
        const BlockHeader& block,
        const std::string& miner_id,
        const std::vector<BlockHeader>& recent_blocks
    ) {
        stats.blocks_analyzed++;
        SelfishMiningReport report;
        report.is_suspicious = false;
        report.miner_id = miner_id;
        report.suspicious_score = 0.0;

        // Get or create pattern tracker for this miner
        auto& pattern = miner_patterns[miner_id];
        pattern.miner_id = miner_id;
        pattern.block_timestamps.push_back(block.timestamp);
        pattern.block_heights.push_back(block.height);

        // Keep only last 100 blocks per miner
        if (pattern.block_timestamps.size() > 100) {
            pattern.block_timestamps.erase(pattern.block_timestamps.begin());
            pattern.block_heights.erase(pattern.block_heights.begin());
        }

        // Check for suspicious patterns

        // 1. Multiple blocks in quick succession (possible withheld blocks)
        if (pattern.block_timestamps.size() >= 2) {
            uint32_t time_diff = block.timestamp - pattern.block_timestamps[pattern.block_timestamps.size() - 2];
            if (time_diff < 60) {  // Less than 1 minute between blocks
                report.suspicious_score += 0.3;
                report.evidence.push_back("Blocks mined in rapid succession");
            }
        }

        // 2. Multiple blocks at same height (fork attack)
        uint32_t same_height_count = 0;
        for (uint32_t height : pattern.block_heights) {
            if (height == block.height) {
                same_height_count++;
            }
        }
        if (same_height_count > 1) {
            report.suspicious_score += 0.4;
            report.evidence.push_back("Multiple blocks at same height");
        }

        // 3. Unusual mining rate compared to network
        if (pattern.block_timestamps.size() >= 10) {
            double avg_time = 0;
            for (size_t i = 1; i < pattern.block_timestamps.size(); ++i) {
                avg_time += pattern.block_timestamps[i] - pattern.block_timestamps[i-1];
            }
            avg_time /= (pattern.block_timestamps.size() - 1);

            // Expected: ~10 minutes per block
            if (avg_time < 300) {  // Less than 5 minutes average
                report.suspicious_score += 0.2;
                report.evidence.push_back("Unusually fast mining rate");
            }
        }

        // 4. Check recent blockchain for reorg patterns
        uint32_t recent_reorgs = 0;
        for (size_t i = 1; i < recent_blocks.size(); ++i) {
            if (recent_blocks[i].height <= recent_blocks[i-1].height) {
                recent_reorgs++;
            }
        }
        if (recent_reorgs > 3) {
            report.suspicious_score += 0.3;
            report.evidence.push_back("Frequent blockchain reorganizations");
        }

        // Determine if suspicious
        if (report.suspicious_score >= 0.5) {
            report.is_suspicious = true;
            stats.suspicious_patterns++;

            if (report.suspicious_score >= 0.8) {
                stats.selfish_mining_detected++;
            }
        }

        return report;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Reset tracking for a miner
    void reset_miner(const std::string& miner_id) {
        miner_patterns.erase(miner_id);
    }
};

// Chain selection rule
class ChainSelector {
private:
    CheckpointManager checkpoint_manager;
    SelfishMiningDetector selfish_detector;

    struct Statistics {
        uint64_t chain_switches = 0;
        uint64_t reorgs_rejected = 0;
        uint64_t checkpoint_rejections = 0;
        uint64_t selfish_mining_rejections = 0;
    } stats;

public:
    struct ChainComparisonResult {
        bool should_switch;
        std::string reason;
        double confidence;  // 0.0 - 1.0
    };

    // Initialize with checkpoints
    void initialize() {
        checkpoint_manager.initialize_checkpoints();
    }

    // Compare two chains and determine which to follow
    ChainComparisonResult compare_chains(
        const ChainState& current_chain,
        const ChainState& candidate_chain,
        const std::vector<BlockHeader>& candidate_blocks
    ) {
        ChainComparisonResult result;
        result.should_switch = false;
        result.confidence = 1.0;

        // Rule 1: Check genesis blocks match (prevent consensus split)
        if (current_chain.genesis_hash != candidate_chain.genesis_hash) {
            result.reason = "Genesis blocks don't match - different network";
            return result;
        }

        // Rule 2: Validate candidate chain is valid
        if (!candidate_chain.is_valid) {
            result.reason = "Candidate chain is invalid";
            return result;
        }

        // Rule 3: Check reorg depth limit
        if (current_chain.height > candidate_chain.height) {
            // Candidate is shorter, not a valid switch
            result.reason = "Candidate chain is shorter";
            return result;
        }

        uint32_t reorg_depth = candidate_chain.height - current_chain.height;
        if (reorg_depth > params::MAX_REORG_DEPTH) {
            stats.reorgs_rejected++;
            result.reason = "Reorg depth (" + std::to_string(reorg_depth) +
                          ") exceeds maximum (" + std::to_string(params::MAX_REORG_DEPTH) + ")";
            return result;
        }

        // Rule 4: Check checkpoints
        for (const auto& block : candidate_blocks) {
            if (!checkpoint_manager.validate_checkpoint(block.height, block.calculate_hash())) {
                stats.checkpoint_rejections++;
                result.reason = "Candidate chain fails checkpoint validation at height " +
                              std::to_string(block.height);
                return result;
            }
        }

        // Rule 5: Require more work to switch (selfish mining prevention)
        double required_work = current_chain.total_work * params::REORG_WORK_MULTIPLIER;
        if (candidate_chain.total_work < required_work) {
            result.reason = "Candidate chain doesn't have enough work (" +
                          std::to_string(candidate_chain.total_work) + " < " +
                          std::to_string(required_work) + ")";
            return result;
        }

        // Rule 6: Check for selfish mining patterns
        if (!candidate_blocks.empty()) {
            auto selfish_report = selfish_detector.analyze_block(
                candidate_blocks.back(),
                "unknown",  // Would be actual miner ID
                candidate_blocks
            );

            if (selfish_report.is_suspicious && selfish_report.suspicious_score > 0.8) {
                stats.selfish_mining_rejections++;
                result.reason = "Selfish mining pattern detected (score: " +
                              std::to_string(selfish_report.suspicious_score) + ")";
                result.confidence = 1.0 - selfish_report.suspicious_score;
                // Still allow switch but flag it
            }
        }

        // Rule 7: Most accumulated work wins
        if (candidate_chain.total_work > current_chain.total_work) {
            stats.chain_switches++;
            result.should_switch = true;
            result.reason = "Candidate chain has more work (" +
                          std::to_string(candidate_chain.total_work) + " > " +
                          std::to_string(current_chain.total_work) + ")";
            return result;
        }

        // Chains are equal or candidate is worse
        result.reason = "Current chain is equal or better";
        return result;
    }

    // Validate a single block header
    struct BlockValidationResult {
        bool valid;
        std::string error;
    };

    BlockValidationResult validate_block_header(const BlockHeader& block) {
        BlockValidationResult result;
        result.valid = true;

        // Check checkpoint
        if (!checkpoint_manager.validate_checkpoint(block.height, block.calculate_hash())) {
            result.valid = false;
            result.error = "Block fails checkpoint validation";
            return result;
        }

        // Check timestamp (not too far in future)
        uint64_t current_time = std::time(nullptr);
        if (block.timestamp > current_time + 7200) {  // 2 hours in future
            result.valid = false;
            result.error = "Block timestamp too far in future";
            return result;
        }

        // Check proof of work (simplified)
        if (block.get_work() == 0) {
            result.valid = false;
            result.error = "Invalid proof of work";
            return result;
        }

        return result;
    }

    // Get checkpoint manager
    CheckpointManager& get_checkpoint_manager() {
        return checkpoint_manager;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Consensus split detector
class ConsensusSplitDetector {
private:
    struct ChainBranch {
        std::string branch_id;
        uint32_t fork_height;
        uint32_t current_height;
        uint64_t chain_work;
        std::vector<std::string> block_hashes;
        uint32_t peer_count = 0;  // How many peers follow this branch
    };

    std::vector<ChainBranch> known_branches;

    struct Statistics {
        uint64_t forks_detected = 0;
        uint64_t consensus_splits_detected = 0;
        uint64_t forks_resolved = 0;
    } stats;

public:
    struct ConsensusSplitReport {
        bool consensus_split_detected;
        uint32_t fork_height;
        std::vector<std::string> competing_branches;
        std::string recommended_branch;
        std::string reason;
    };

    // Detect consensus splits from observed branches
    ConsensusSplitReport detect_split(
        const std::vector<ChainBranch>& observed_branches,
        const ChainState& our_chain
    ) {
        ConsensusSplitReport report;
        report.consensus_split_detected = false;

        if (observed_branches.size() <= 1) {
            return report;  // No split, only one branch
        }

        stats.forks_detected++;

        // Check if multiple branches have significant peer support
        uint32_t total_peers = 0;
        for (const auto& branch : observed_branches) {
            total_peers += branch.peer_count;
        }

        std::vector<ChainBranch> significant_branches;
        for (const auto& branch : observed_branches) {
            // Branch is significant if it has >10% of peers
            if (branch.peer_count > total_peers / 10) {
                significant_branches.push_back(branch);
            }
        }

        // Consensus split if multiple significant branches exist
        if (significant_branches.size() > 1) {
            stats.consensus_splits_detected++;
            report.consensus_split_detected = true;

            // Find common fork point
            uint32_t min_fork_height = std::numeric_limits<uint32_t>::max();
            for (const auto& branch : significant_branches) {
                if (branch.fork_height < min_fork_height) {
                    min_fork_height = branch.fork_height;
                }
            }
            report.fork_height = min_fork_height;

            // List competing branches
            for (const auto& branch : significant_branches) {
                report.competing_branches.push_back(branch.branch_id);
            }

            // Recommend branch with most work
            uint64_t max_work = 0;
            std::string best_branch;
            for (const auto& branch : significant_branches) {
                if (branch.chain_work > max_work) {
                    max_work = branch.chain_work;
                    best_branch = branch.branch_id;
                }
            }

            report.recommended_branch = best_branch;
            report.reason = "Multiple competing chains detected - recommending chain with most work";
        }

        return report;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Chain selection manager
class ChainSelectionManager {
private:
    ChainSelector selector;
    ConsensusSplitDetector split_detector;
    ChainState current_chain;

    ChainSelectionManager() = default;

public:
    static ChainSelectionManager& instance() {
        static ChainSelectionManager instance;
        return instance;
    }

    // Initialize
    void initialize() {
        selector.initialize();
    }

    // Set current chain
    void set_current_chain(const ChainState& chain) {
        current_chain = chain;
    }

    // Get current chain
    const ChainState& get_current_chain() const {
        return current_chain;
    }

    // Compare and potentially switch chains
    ChainSelector::ChainComparisonResult compare_chains(
        const ChainState& candidate_chain,
        const std::vector<BlockHeader>& candidate_blocks
    ) {
        return selector.compare_chains(current_chain, candidate_chain, candidate_blocks);
    }

    // Validate block header
    ChainSelector::BlockValidationResult validate_block_header(const BlockHeader& block) {
        return selector.validate_block_header(block);
    }

    // Add checkpoint
    bool add_checkpoint(const Checkpoint& checkpoint) {
        return selector.get_checkpoint_manager().add_checkpoint(checkpoint);
    }

    // Get checkpoint at height
    std::optional<Checkpoint> get_checkpoint(uint32_t height) const {
        return selector.get_checkpoint_manager().get_checkpoint(height);
    }

    // Detect consensus splits
    ConsensusSplitDetector::ConsensusSplitReport detect_consensus_split(
        const std::vector<ConsensusSplitDetector::ChainBranch>& branches
    ) {
        return split_detector.detect_split(branches, current_chain);
    }

    // Get combined statistics
    struct CombinedStatistics {
        ChainSelector::Statistics selector_stats;
        ConsensusSplitDetector::Statistics split_stats;
    };

    CombinedStatistics get_statistics() const {
        CombinedStatistics stats;
        stats.selector_stats = selector.get_statistics();
        stats.split_stats = split_detector.get_statistics();
        return stats;
    }
};

} // namespace chain
} // namespace intcoin

#endif // INTCOIN_CHAIN_SELECTION_H
