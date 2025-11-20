// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONSENSUS_ACTIVATION_H
#define INTCOIN_CONSENSUS_ACTIVATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <chrono>

namespace intcoin {
namespace consensus {

// Deployment states for soft forks
enum class DeploymentState {
    DEFINED,      // First state, waiting for start time
    STARTED,      // Started, waiting for threshold
    LOCKED_IN,    // Threshold met, waiting for activation
    ACTIVE,       // Active and enforced
    FAILED        // Failed to activate in time window
};

// Activation mechanism types
enum class ActivationMechanism {
    BIP9,           // Version bits with miner signaling (95% threshold)
    BIP8,           // Version bits with mandatory activation
    SPEEDY_TRIAL,   // Fast signaling with early activation (90% threshold)
    FLAG_DAY,       // Activation at specific block height
    USER_ACTIVATED  // UASF - User Activated Soft Fork
};

// Soft fork deployment
struct Deployment {
    std::string name;
    ActivationMechanism mechanism;
    uint32_t bit;                    // Version bit position (0-28)
    uint64_t start_time;             // Earliest activation time (Unix timestamp)
    uint64_t timeout;                // Timeout for activation (Unix timestamp)
    uint32_t min_activation_height;  // Minimum height for activation
    uint32_t threshold_numerator;    // e.g., 1916 for 95% (out of 2016)
    uint32_t threshold_denominator;  // e.g., 2016 blocks (2 weeks)
    uint32_t signal_period;          // Signaling period in blocks
    DeploymentState state;
    uint32_t state_since_height;     // Height when current state began
    std::string description;
};

// Version bits parser
class VersionBitsParser {
private:
    struct Statistics {
        uint64_t blocks_parsed = 0;
        uint64_t bits_signaled = 0;
    } stats;

public:
    // Extract version bits from block version
    static uint32_t extract_version_bits(uint32_t version) {
        // BIP9: Top 3 bits must be 001 for version bits
        // Bits 0-28 available for signaling
        if ((version >> 29) != 0x01) {
            return 0;  // Not using version bits
        }
        return version & 0x1FFFFFFF;  // Extract lower 29 bits
    }

    // Check if specific bit is set
    static bool is_bit_set(uint32_t version, uint32_t bit) {
        if (bit > 28) return false;  // Only bits 0-28 valid
        uint32_t bits = extract_version_bits(version);
        return (bits & (1u << bit)) != 0;
    }

    // Set specific bit in version
    static uint32_t set_bit(uint32_t version, uint32_t bit) {
        if (bit > 28) return version;

        // Ensure top 3 bits are 001 (version bits format)
        version = (version & 0x1FFFFFFF) | 0x20000000;

        // Set the bit
        return version | (1u << bit);
    }

    // Clear specific bit in version
    static uint32_t clear_bit(uint32_t version, uint32_t bit) {
        if (bit > 28) return version;
        return version & ~(1u << bit);
    }

    // Count signaling blocks in window
    struct SignalingCount {
        uint32_t signaling_blocks;
        uint32_t total_blocks;
        double percentage;
        bool threshold_met;
    };

    SignalingCount count_signaling(
        const std::vector<uint32_t>& block_versions,
        uint32_t bit,
        uint32_t threshold_numerator,
        uint32_t threshold_denominator
    ) {
        stats.blocks_parsed += block_versions.size();

        SignalingCount count;
        count.signaling_blocks = 0;
        count.total_blocks = block_versions.size();

        for (uint32_t version : block_versions) {
            if (is_bit_set(version, bit)) {
                count.signaling_blocks++;
                stats.bits_signaled++;
            }
        }

        count.percentage = (count.total_blocks > 0) ?
            (static_cast<double>(count.signaling_blocks) / count.total_blocks * 100.0) : 0.0;

        // Check if threshold met
        count.threshold_met = (count.signaling_blocks * threshold_denominator) >=
                             (count.total_blocks * threshold_numerator);

        return count;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Threshold calculator for different activation mechanisms
class ThresholdCalculator {
public:
    // BIP9: 95% threshold (1916 out of 2016 blocks)
    static constexpr uint32_t BIP9_NUMERATOR = 1916;
    static constexpr uint32_t BIP9_DENOMINATOR = 2016;

    // Speedy Trial: 90% threshold (1815 out of 2016 blocks)
    static constexpr uint32_t SPEEDY_TRIAL_NUMERATOR = 1815;
    static constexpr uint32_t SPEEDY_TRIAL_DENOMINATOR = 2016;

    // Calculate if threshold is met
    struct ThresholdResult {
        bool threshold_met;
        uint32_t signaling_blocks;
        uint32_t required_blocks;
        uint32_t total_blocks;
        double percentage;
        uint32_t blocks_remaining;  // Blocks until end of period
    };

    static ThresholdResult calculate_threshold(
        uint32_t signaling_blocks,
        uint32_t total_blocks,
        uint32_t threshold_numerator,
        uint32_t threshold_denominator,
        uint32_t blocks_in_period
    ) {
        ThresholdResult result;
        result.signaling_blocks = signaling_blocks;
        result.total_blocks = total_blocks;
        result.blocks_remaining = (blocks_in_period > total_blocks) ?
                                 (blocks_in_period - total_blocks) : 0;

        // Calculate required blocks for threshold
        result.required_blocks =
            (blocks_in_period * threshold_numerator + threshold_denominator - 1) /
            threshold_denominator;

        // Calculate percentage
        result.percentage = (total_blocks > 0) ?
            (static_cast<double>(signaling_blocks) / total_blocks * 100.0) : 0.0;

        // Check if threshold met
        result.threshold_met = signaling_blocks >= result.required_blocks;

        return result;
    }

    // Check if can still reach threshold
    static bool can_reach_threshold(
        uint32_t current_signaling,
        uint32_t blocks_processed,
        uint32_t blocks_in_period,
        uint32_t threshold_numerator,
        uint32_t threshold_denominator
    ) {
        uint32_t required = (blocks_in_period * threshold_numerator + threshold_denominator - 1) /
                           threshold_denominator;
        uint32_t blocks_remaining = blocks_in_period - blocks_processed;

        // Even if all remaining blocks signal, can we reach threshold?
        return (current_signaling + blocks_remaining) >= required;
    }
};

// State machine for deployment state transitions
class DeploymentStateMachine {
private:
    struct StateTransition {
        DeploymentState from_state;
        DeploymentState to_state;
        uint32_t transition_height;
        uint64_t transition_time;
        std::string reason;
    };

    std::unordered_map<std::string, std::vector<StateTransition>> transition_history;

    struct Statistics {
        uint64_t state_transitions = 0;
        uint64_t activations_completed = 0;
        uint64_t activations_failed = 0;
    } stats;

public:
    // Transition result
    struct TransitionResult {
        bool transitioned;
        DeploymentState new_state;
        std::string reason;
    };

    // Attempt state transition
    TransitionResult transition(
        Deployment& deployment,
        uint32_t current_height,
        uint64_t current_time,
        bool threshold_met
    ) {
        TransitionResult result;
        result.transitioned = false;
        result.new_state = deployment.state;

        DeploymentState old_state = deployment.state;

        switch (deployment.state) {
            case DeploymentState::DEFINED:
                // Check if we've reached start time
                if (current_time >= deployment.start_time) {
                    deployment.state = DeploymentState::STARTED;
                    deployment.state_since_height = current_height;
                    result.transitioned = true;
                    result.new_state = DeploymentState::STARTED;
                    result.reason = "Start time reached";
                }
                break;

            case DeploymentState::STARTED:
                // Check timeout first
                if (current_time >= deployment.timeout) {
                    deployment.state = DeploymentState::FAILED;
                    deployment.state_since_height = current_height;
                    result.transitioned = true;
                    result.new_state = DeploymentState::FAILED;
                    result.reason = "Timeout reached without activation";
                    stats.activations_failed++;
                }
                // Check if threshold met
                else if (threshold_met) {
                    deployment.state = DeploymentState::LOCKED_IN;
                    deployment.state_since_height = current_height;
                    result.transitioned = true;
                    result.new_state = DeploymentState::LOCKED_IN;
                    result.reason = "Threshold met, locked in for activation";
                }
                // BIP8 mandatory activation
                else if (deployment.mechanism == ActivationMechanism::BIP8 &&
                         current_time >= deployment.timeout) {
                    deployment.state = DeploymentState::LOCKED_IN;
                    deployment.state_since_height = current_height;
                    result.transitioned = true;
                    result.new_state = DeploymentState::LOCKED_IN;
                    result.reason = "BIP8 mandatory activation at timeout";
                }
                break;

            case DeploymentState::LOCKED_IN:
                // Move to active after one full period
                if (current_height >= deployment.state_since_height + deployment.signal_period &&
                    current_height >= deployment.min_activation_height) {
                    deployment.state = DeploymentState::ACTIVE;
                    deployment.state_since_height = current_height;
                    result.transitioned = true;
                    result.new_state = DeploymentState::ACTIVE;
                    result.reason = "Activation height reached";
                    stats.activations_completed++;
                }
                break;

            case DeploymentState::ACTIVE:
            case DeploymentState::FAILED:
                // Terminal states, no transitions
                break;
        }

        if (result.transitioned) {
            stats.state_transitions++;
            record_transition(deployment.name, old_state, result.new_state,
                            current_height, current_time, result.reason);
        }

        return result;
    }

    // Record state transition
    void record_transition(
        const std::string& deployment_name,
        DeploymentState from,
        DeploymentState to,
        uint32_t height,
        uint64_t time,
        const std::string& reason
    ) {
        StateTransition transition;
        transition.from_state = from;
        transition.to_state = to;
        transition.transition_height = height;
        transition.transition_time = time;
        transition.reason = reason;

        transition_history[deployment_name].push_back(transition);
    }

    // Get transition history
    const std::vector<StateTransition>& get_history(const std::string& deployment_name) const {
        static const std::vector<StateTransition> empty;
        auto it = transition_history.find(deployment_name);
        return (it != transition_history.end()) ? it->second : empty;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Soft fork compatibility checker
class SoftForkCompatibility {
private:
    struct CompatibilityIssue {
        std::string issue_type;
        std::string description;
        std::string affected_feature;
        bool is_blocking;  // Blocks activation if true
    };

    struct Statistics {
        uint64_t compatibility_checks = 0;
        uint64_t issues_found = 0;
        uint64_t blocking_issues = 0;
    } stats;

public:
    // Compatibility check result
    struct CompatibilityResult {
        bool is_compatible;
        std::vector<CompatibilityIssue> issues;
        std::vector<std::string> warnings;
    };

    // Check if new deployment is compatible with existing active deployments
    CompatibilityResult check_compatibility(
        const Deployment& new_deployment,
        const std::vector<Deployment>& active_deployments
    ) {
        stats.compatibility_checks++;
        CompatibilityResult result;
        result.is_compatible = true;

        // Check 1: Version bit conflicts
        for (const auto& active : active_deployments) {
            if (active.state == DeploymentState::ACTIVE ||
                active.state == DeploymentState::LOCKED_IN ||
                active.state == DeploymentState::STARTED) {

                if (active.bit == new_deployment.bit) {
                    CompatibilityIssue issue;
                    issue.issue_type = "VERSION_BIT_CONFLICT";
                    issue.description = "Bit " + std::to_string(new_deployment.bit) +
                                      " already used by deployment: " + active.name;
                    issue.affected_feature = active.name;
                    issue.is_blocking = true;
                    result.issues.push_back(issue);
                    result.is_compatible = false;
                    stats.blocking_issues++;
                }
            }
        }

        // Check 2: Time window overlaps
        for (const auto& active : active_deployments) {
            if (active.state == DeploymentState::STARTED) {
                // Check if time windows overlap
                bool overlap = !(new_deployment.timeout < active.start_time ||
                               new_deployment.start_time > active.timeout);

                if (overlap && active.bit == new_deployment.bit) {
                    CompatibilityIssue issue;
                    issue.issue_type = "TIME_WINDOW_OVERLAP";
                    issue.description = "Signaling period overlaps with: " + active.name;
                    issue.affected_feature = active.name;
                    issue.is_blocking = true;
                    result.issues.push_back(issue);
                    result.is_compatible = false;
                    stats.blocking_issues++;
                }
            }
        }

        // Check 3: Activation height conflicts
        for (const auto& active : active_deployments) {
            if (active.state == DeploymentState::LOCKED_IN) {
                uint32_t active_activation = active.state_since_height + active.signal_period;

                if (new_deployment.min_activation_height == active_activation) {
                    result.warnings.push_back(
                        "Activation at same height as " + active.name +
                        " - may cause confusion"
                    );
                }
            }
        }

        // Check 4: Soft fork rule conflicts (simplified check)
        // In production, would check if new rules conflict with existing rules
        // For now, just warn if multiple forks activating simultaneously
        uint32_t concurrent_forks = 0;
        for (const auto& active : active_deployments) {
            if (active.state == DeploymentState::STARTED ||
                active.state == DeploymentState::LOCKED_IN) {
                concurrent_forks++;
            }
        }

        if (concurrent_forks >= 3) {
            result.warnings.push_back(
                "Many concurrent deployments (" + std::to_string(concurrent_forks) +
                ") may complicate consensus"
            );
        }

        // Update statistics
        if (!result.issues.empty()) {
            stats.issues_found += result.issues.size();
        }

        return result;
    }

    // Check if old nodes will accept new blocks
    struct BackwardCompatibility {
        bool old_nodes_accept_new_blocks;
        std::string reason;
    };

    BackwardCompatibility check_backward_compatibility(
        const Deployment& deployment
    ) {
        BackwardCompatibility result;
        result.old_nodes_accept_new_blocks = true;
        result.reason = "Soft fork - new rules are stricter subset of old rules";

        // Soft forks are by definition backward compatible
        // Old nodes will accept blocks created by new rules
        // (because new rules are stricter)

        // Only issue: if deployment changes something old nodes rely on
        // This would be checked in production with specific rule analysis

        return result;
    }

    // Check if new nodes will accept old blocks
    struct ForwardCompatibility {
        bool new_nodes_accept_old_blocks;
        std::string reason;
        uint32_t grace_period_blocks;  // Blocks to accept old format
    };

    ForwardCompatibility check_forward_compatibility(
        const Deployment& deployment,
        uint32_t current_height
    ) {
        ForwardCompatibility result;
        result.new_nodes_accept_old_blocks = true;
        result.grace_period_blocks = 0;

        if (deployment.state == DeploymentState::ACTIVE) {
            // After activation, new nodes enforce new rules
            result.reason = "Soft fork active - new rules enforced";
            result.grace_period_blocks = 0;
        } else if (deployment.state == DeploymentState::LOCKED_IN) {
            // Grace period until activation
            uint32_t activation_height = deployment.state_since_height + deployment.signal_period;
            if (current_height < activation_height) {
                result.grace_period_blocks = activation_height - current_height;
                result.reason = "Locked in - grace period until activation";
            }
        } else {
            // Before lock-in, old blocks accepted
            result.reason = "Not active - old rules still valid";
            result.grace_period_blocks = UINT32_MAX;  // Indefinite
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Consensus activation manager
class ConsensusActivationManager {
private:
    std::unordered_map<std::string, Deployment> deployments;
    VersionBitsParser version_parser;
    DeploymentStateMachine state_machine;
    SoftForkCompatibility compatibility_checker;

    ConsensusActivationManager() {
        initialize_deployments();
    }

    void initialize_deployments() {
        // Example: SegWit-style deployment
        Deployment segwit;
        segwit.name = "segwit";
        segwit.mechanism = ActivationMechanism::BIP9;
        segwit.bit = 1;
        segwit.start_time = 1704672000;  // 2024-01-08
        segwit.timeout = 1736208000;     // 2025-01-07
        segwit.min_activation_height = 0;
        segwit.threshold_numerator = ThresholdCalculator::BIP9_NUMERATOR;
        segwit.threshold_denominator = ThresholdCalculator::BIP9_DENOMINATOR;
        segwit.signal_period = 2016;     // 2 weeks
        segwit.state = DeploymentState::ACTIVE;  // Already active
        segwit.state_since_height = 0;
        segwit.description = "Segregated Witness - transaction malleability fix";
        deployments["segwit"] = segwit;

        // Example: Taproot-style deployment (future)
        Deployment taproot;
        taproot.name = "taproot";
        taproot.mechanism = ActivationMechanism::SPEEDY_TRIAL;
        taproot.bit = 2;
        taproot.start_time = 1735776000;  // 2025-01-02
        taproot.timeout = 1767312000;     // 2026-01-01
        taproot.min_activation_height = 100000;
        taproot.threshold_numerator = ThresholdCalculator::SPEEDY_TRIAL_NUMERATOR;
        taproot.threshold_denominator = ThresholdCalculator::SPEEDY_TRIAL_DENOMINATOR;
        taproot.signal_period = 2016;
        taproot.state = DeploymentState::DEFINED;
        taproot.state_since_height = 0;
        taproot.description = "Taproot - Schnorr signatures and MAST";
        deployments["taproot"] = taproot;
    }

public:
    static ConsensusActivationManager& instance() {
        static ConsensusActivationManager instance;
        return instance;
    }

    // Add new deployment
    bool add_deployment(const Deployment& deployment) {
        // Check compatibility with existing deployments
        std::vector<Deployment> active_deps;
        for (const auto& [name, dep] : deployments) {
            if (dep.state != DeploymentState::FAILED) {
                active_deps.push_back(dep);
            }
        }

        auto compat = compatibility_checker.check_compatibility(deployment, active_deps);
        if (!compat.is_compatible) {
            return false;  // Incompatible
        }

        deployments[deployment.name] = deployment;
        return true;
    }

    // Update deployment state based on new block
    void update_deployment(
        const std::string& deployment_name,
        uint32_t current_height,
        uint64_t current_time,
        const std::vector<uint32_t>& recent_block_versions
    ) {
        auto it = deployments.find(deployment_name);
        if (it == deployments.end()) return;

        Deployment& deployment = it->second;

        // Count signaling in current period
        auto count = version_parser.count_signaling(
            recent_block_versions,
            deployment.bit,
            deployment.threshold_numerator,
            deployment.threshold_denominator
        );

        // Attempt state transition
        state_machine.transition(
            deployment,
            current_height,
            current_time,
            count.threshold_met
        );
    }

    // Get deployment state
    std::optional<Deployment> get_deployment(const std::string& name) const {
        auto it = deployments.find(name);
        if (it != deployments.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Get all deployments
    const std::unordered_map<std::string, Deployment>& get_all_deployments() const {
        return deployments;
    }

    // Check if specific rules should be enforced at height
    bool should_enforce_rules(const std::string& deployment_name, uint32_t height) const {
        auto it = deployments.find(deployment_name);
        if (it == deployments.end()) return false;

        const Deployment& deployment = it->second;

        return (deployment.state == DeploymentState::ACTIVE &&
                height >= deployment.state_since_height);
    }

    // Get version bits parser
    VersionBitsParser& get_version_parser() {
        return version_parser;
    }

    // Get state machine
    DeploymentStateMachine& get_state_machine() {
        return state_machine;
    }

    // Get compatibility checker
    SoftForkCompatibility& get_compatibility_checker() {
        return compatibility_checker;
    }
};

} // namespace consensus
} // namespace intcoin

#endif // INTCOIN_CONSENSUS_ACTIVATION_H
