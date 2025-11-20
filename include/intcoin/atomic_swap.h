// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_ATOMIC_SWAP_H
#define INTCOIN_ATOMIC_SWAP_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <optional>
#include <memory>
#include <mutex>
#include <chrono>

namespace intcoin {
namespace swap {

// Hash Time-Locked Contract (HTLC) implementation
class HTLC {
public:
    // HTLC parameters
    struct Parameters {
        std::array<uint8_t, 32> payment_hash;  // SHA-256 hash of preimage
        uint64_t amount;                       // Amount locked
        uint32_t timeout_height;               // Block height timeout
        uint64_t timeout_timestamp;            // Unix timestamp timeout
        std::string recipient_pubkey;          // Recipient's public key
        std::string sender_pubkey;             // Sender's public key (for refund)
        std::string chain_id;                  // Blockchain identifier
    };

private:
    Parameters params;
    bool is_locked = false;
    bool is_claimed = false;
    bool is_refunded = false;
    std::array<uint8_t, 32> preimage;  // Only known after claim
    uint32_t creation_height = 0;
    uint64_t creation_time = 0;

    mutable std::mutex mtx;  // Thread safety

    struct Statistics {
        uint64_t htlcs_created = 0;
        uint64_t htlcs_claimed = 0;
        uint64_t htlcs_refunded = 0;
        uint64_t htlcs_expired = 0;
    };

    static Statistics& get_stats() {
        static Statistics stats;
        return stats;
    }

public:
    // Create HTLC
    HTLC(const Parameters& p) : params(p) {
        creation_time = std::chrono::system_clock::now().time_since_epoch().count();
        get_stats().htlcs_created++;
    }

    // Lock funds in HTLC
    struct LockResult {
        bool success;
        std::string error;
        std::string htlc_id;
    };

    LockResult lock(uint32_t current_height) {
        std::lock_guard<std::mutex> lock(mtx);

        LockResult result;
        result.success = false;

        // Validation 1: Not already locked
        if (is_locked) {
            result.error = "HTLC already locked";
            return result;
        }

        // Validation 2: Amount is positive
        if (params.amount == 0) {
            result.error = "Amount must be positive";
            return result;
        }

        // Validation 3: Timeout is in future
        if (params.timeout_height <= current_height) {
            result.error = "Timeout height must be in future";
            return result;
        }

        // Validation 4: Reasonable timeout window (at least 24 blocks, ~4 hours)
        if (params.timeout_height < current_height + 24) {
            result.error = "Timeout too soon (minimum 24 blocks)";
            return result;
        }

        // Validation 5: Maximum timeout (not more than 1008 blocks, ~1 week)
        if (params.timeout_height > current_height + 1008) {
            result.error = "Timeout too far (maximum 1008 blocks)";
            return result;
        }

        // Validation 6: Valid payment hash
        bool hash_valid = false;
        for (uint8_t byte : params.payment_hash) {
            if (byte != 0) {
                hash_valid = true;
                break;
            }
        }
        if (!hash_valid) {
            result.error = "Invalid payment hash (all zeros)";
            return result;
        }

        // Lock the HTLC
        is_locked = true;
        creation_height = current_height;
        result.success = true;

        // Generate HTLC ID (hash of parameters)
        result.htlc_id = generate_htlc_id();

        return result;
    }

    // Claim HTLC with preimage
    struct ClaimResult {
        bool success;
        std::string error;
        uint64_t claimed_amount;
    };

    ClaimResult claim(const std::array<uint8_t, 32>& provided_preimage, uint32_t current_height) {
        std::lock_guard<std::mutex> lock(mtx);

        ClaimResult result;
        result.success = false;
        result.claimed_amount = 0;

        // Check 1: HTLC must be locked
        if (!is_locked) {
            result.error = "HTLC not locked";
            return result;
        }

        // Check 2: Not already claimed
        if (is_claimed) {
            result.error = "HTLC already claimed";
            return result;
        }

        // Check 3: Not already refunded
        if (is_refunded) {
            result.error = "HTLC already refunded";
            return result;
        }

        // Check 4: Not expired
        if (current_height >= params.timeout_height) {
            result.error = "HTLC expired (past timeout height)";
            is_refunded = true;  // Auto-refund on expiry
            get_stats().htlcs_expired++;
            return result;
        }

        // Check 5: Preimage hash matches payment hash
        std::array<uint8_t, 32> preimage_hash = sha256(provided_preimage);
        if (preimage_hash != params.payment_hash) {
            result.error = "Preimage does not match payment hash";
            return result;
        }

        // Claim successful
        is_claimed = true;
        preimage = provided_preimage;
        result.success = true;
        result.claimed_amount = params.amount;
        get_stats().htlcs_claimed++;

        return result;
    }

    // Refund HTLC after timeout
    struct RefundResult {
        bool success;
        std::string error;
        uint64_t refunded_amount;
    };

    RefundResult refund(uint32_t current_height) {
        std::lock_guard<std::mutex> lock(mtx);

        RefundResult result;
        result.success = false;
        result.refunded_amount = 0;

        // Check 1: HTLC must be locked
        if (!is_locked) {
            result.error = "HTLC not locked";
            return result;
        }

        // Check 2: Not already claimed
        if (is_claimed) {
            result.error = "HTLC already claimed (cannot refund)";
            return result;
        }

        // Check 3: Not already refunded
        if (is_refunded) {
            result.error = "HTLC already refunded";
            return result;
        }

        // Check 4: Timeout must have passed
        if (current_height < params.timeout_height) {
            result.error = "Timeout not reached (blocks remaining: " +
                         std::to_string(params.timeout_height - current_height) + ")";
            return result;
        }

        // Refund successful
        is_refunded = true;
        result.success = true;
        result.refunded_amount = params.amount;
        get_stats().htlcs_refunded++;

        return result;
    }

    // Get HTLC status
    struct Status {
        bool locked;
        bool claimed;
        bool refunded;
        bool expired;
        uint32_t blocks_until_timeout;
    };

    Status get_status(uint32_t current_height) const {
        std::lock_guard<std::mutex> lock(mtx);

        Status status;
        status.locked = is_locked;
        status.claimed = is_claimed;
        status.refunded = is_refunded;
        status.expired = (current_height >= params.timeout_height && !is_claimed);
        status.blocks_until_timeout = (current_height < params.timeout_height) ?
            (params.timeout_height - current_height) : 0;

        return status;
    }

    // Get preimage (only after successful claim)
    std::optional<std::array<uint8_t, 32>> get_preimage() const {
        std::lock_guard<std::mutex> lock(mtx);

        if (is_claimed) {
            return preimage;
        }
        return std::nullopt;
    }

    // Get parameters
    const Parameters& get_parameters() const {
        return params;
    }

    // Get statistics
    static const Statistics& get_statistics() {
        return get_stats();
    }

private:
    // Generate HTLC ID from parameters
    std::string generate_htlc_id() const {
        // In production, would hash all parameters
        // For now, simplified
        std::string id = "htlc_";
        for (size_t i = 0; i < 8; ++i) {
            id += std::to_string(params.payment_hash[i]);
        }
        return id;
    }

    // SHA-256 hash (simplified placeholder)
    static std::array<uint8_t, 32> sha256(const std::array<uint8_t, 32>& data) {
        // In production, use actual SHA-256
        // For now, return the data itself as placeholder
        return data;
    }
};

// SPV (Simplified Payment Verification) proof validator
class SPVProofValidator {
private:
    struct BlockHeader {
        uint32_t version;
        std::array<uint8_t, 32> prev_block_hash;
        std::array<uint8_t, 32> merkle_root;
        uint32_t timestamp;
        uint32_t bits;  // Difficulty target
        uint32_t nonce;
        uint32_t height;
    };

    struct MerkleProof {
        std::vector<std::array<uint8_t, 32>> hashes;  // Sibling hashes
        std::vector<bool> directions;                  // Left (false) or right (true)
        uint32_t position;                             // Transaction position in block
    };

    // Known block headers (simplified chain state)
    std::unordered_map<uint32_t, BlockHeader> headers;

    struct Statistics {
        uint64_t proofs_validated = 0;
        uint64_t proofs_valid = 0;
        uint64_t proofs_invalid = 0;
        uint64_t merkle_computations = 0;
    } stats;

public:
    // Validation result
    struct ValidationResult {
        bool is_valid;
        std::string error;
        uint32_t confirmations;
    };

    // Validate SPV proof
    ValidationResult validate_proof(
        const std::array<uint8_t, 32>& tx_hash,
        const MerkleProof& proof,
        uint32_t block_height,
        uint32_t current_height
    ) {
        stats.proofs_validated++;
        ValidationResult result;
        result.is_valid = false;
        result.confirmations = 0;

        // Check 1: Block header exists
        auto header_it = headers.find(block_height);
        if (header_it == headers.end()) {
            result.error = "Block header not found";
            stats.proofs_invalid++;
            return result;
        }

        const BlockHeader& header = header_it->second;

        // Check 2: Compute merkle root from proof
        std::array<uint8_t, 32> computed_root = compute_merkle_root(tx_hash, proof);
        stats.merkle_computations++;

        // Check 3: Verify computed root matches block's merkle root
        if (computed_root != header.merkle_root) {
            result.error = "Merkle root mismatch";
            stats.proofs_invalid++;
            return result;
        }

        // Check 4: Sufficient confirmations (require at least 6)
        if (current_height < block_height) {
            result.error = "Block is in future";
            stats.proofs_invalid++;
            return result;
        }

        result.confirmations = current_height - block_height + 1;

        if (result.confirmations < 6) {
            result.error = "Insufficient confirmations (minimum 6 required)";
            stats.proofs_invalid++;
            return result;
        }

        // Validation successful
        result.is_valid = true;
        stats.proofs_valid++;

        return result;
    }

    // Add block header (for SPV chain tracking)
    void add_header(const BlockHeader& header) {
        headers[header.height] = header;
    }

    // Verify header chain continuity
    struct ChainValidation {
        bool is_valid;
        std::string error;
        uint32_t verified_depth;
    };

    ChainValidation verify_chain(uint32_t start_height, uint32_t end_height) {
        ChainValidation result;
        result.is_valid = true;
        result.verified_depth = 0;

        for (uint32_t height = start_height; height < end_height; ++height) {
            auto current_it = headers.find(height);
            auto next_it = headers.find(height + 1);

            if (current_it == headers.end()) {
                result.is_valid = false;
                result.error = "Missing header at height " + std::to_string(height);
                return result;
            }

            if (next_it == headers.end()) {
                break;  // End of available chain
            }

            // Verify next block points to current block
            std::array<uint8_t, 32> current_hash = compute_block_hash(current_it->second);
            if (next_it->second.prev_block_hash != current_hash) {
                result.is_valid = false;
                result.error = "Chain discontinuity at height " + std::to_string(height + 1);
                return result;
            }

            result.verified_depth++;
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

private:
    // Compute merkle root from transaction hash and proof
    std::array<uint8_t, 32> compute_merkle_root(
        const std::array<uint8_t, 32>& tx_hash,
        const MerkleProof& proof
    ) {
        std::array<uint8_t, 32> current = tx_hash;

        // Traverse merkle tree using proof hashes
        for (size_t i = 0; i < proof.hashes.size(); ++i) {
            if (i >= proof.directions.size()) break;

            std::array<uint8_t, 32> sibling = proof.hashes[i];
            bool is_right = proof.directions[i];

            if (is_right) {
                // Current is left, sibling is right
                current = hash_pair(current, sibling);
            } else {
                // Sibling is left, current is right
                current = hash_pair(sibling, current);
            }
        }

        return current;
    }

    // Hash pair of nodes (simplified)
    static std::array<uint8_t, 32> hash_pair(
        const std::array<uint8_t, 32>& left,
        const std::array<uint8_t, 32>& right
    ) {
        // In production, would do SHA-256(SHA-256(left || right))
        // For now, XOR as placeholder
        std::array<uint8_t, 32> result;
        for (size_t i = 0; i < 32; ++i) {
            result[i] = left[i] ^ right[i];
        }
        return result;
    }

    // Compute block hash (simplified)
    static std::array<uint8_t, 32> compute_block_hash(const BlockHeader& header) {
        // In production, would hash header fields
        // For now, simplified
        std::array<uint8_t, 32> hash = {};
        hash[0] = static_cast<uint8_t>(header.height & 0xFF);
        hash[1] = static_cast<uint8_t>((header.height >> 8) & 0xFF);
        return hash;
    }
};

// Timeout enforcer (prevents fund loss)
class TimeoutEnforcer {
private:
    struct TimeoutPolicy {
        uint32_t min_timeout_blocks;   // Minimum timeout (24 blocks = ~4 hours)
        uint32_t max_timeout_blocks;   // Maximum timeout (1008 blocks = ~1 week)
        uint32_t safety_margin_blocks; // Safety margin before timeout (6 blocks)
    };

    TimeoutPolicy policy;

    struct Statistics {
        uint64_t timeouts_enforced = 0;
        uint64_t timeouts_prevented = 0;
        uint64_t refunds_triggered = 0;
        uint64_t warnings_issued = 0;
    } stats;

public:
    // Default policy
    TimeoutEnforcer() {
        policy.min_timeout_blocks = 24;     // ~4 hours
        policy.max_timeout_blocks = 1008;   // ~1 week
        policy.safety_margin_blocks = 6;    // ~1 hour
    }

    // Validate timeout parameters
    struct TimeoutValidation {
        bool is_valid;
        std::string error;
        std::vector<std::string> warnings;
    };

    TimeoutValidation validate_timeout(
        uint32_t timeout_height,
        uint32_t current_height
    ) {
        TimeoutValidation result;
        result.is_valid = true;

        // Check 1: Timeout is in future
        if (timeout_height <= current_height) {
            result.is_valid = false;
            result.error = "Timeout must be in future";
            stats.timeouts_prevented++;
            return result;
        }

        uint32_t timeout_blocks = timeout_height - current_height;

        // Check 2: Minimum timeout
        if (timeout_blocks < policy.min_timeout_blocks) {
            result.is_valid = false;
            result.error = "Timeout too short (minimum " +
                         std::to_string(policy.min_timeout_blocks) + " blocks)";
            stats.timeouts_prevented++;
            return result;
        }

        // Check 3: Maximum timeout
        if (timeout_blocks > policy.max_timeout_blocks) {
            result.is_valid = false;
            result.error = "Timeout too long (maximum " +
                         std::to_string(policy.max_timeout_blocks) + " blocks)";
            stats.timeouts_prevented++;
            return result;
        }

        // Warning: Approaching minimum
        if (timeout_blocks < policy.min_timeout_blocks + 6) {
            result.warnings.push_back("Timeout is close to minimum (consider longer timeout)");
            stats.warnings_issued++;
        }

        stats.timeouts_enforced++;
        return result;
    }

    // Check if close to timeout
    struct TimeoutProximity {
        bool is_close;
        bool is_expired;
        uint32_t blocks_remaining;
        bool should_act_now;
    };

    TimeoutProximity check_proximity(
        uint32_t timeout_height,
        uint32_t current_height
    ) {
        TimeoutProximity proximity;
        proximity.is_expired = (current_height >= timeout_height);
        proximity.blocks_remaining = proximity.is_expired ? 0 :
                                    (timeout_height - current_height);

        // Close to timeout if within safety margin
        proximity.is_close = (proximity.blocks_remaining <= policy.safety_margin_blocks) &&
                           !proximity.is_expired;

        // Should act now if within half of safety margin
        proximity.should_act_now = (proximity.blocks_remaining <= policy.safety_margin_blocks / 2) &&
                                  !proximity.is_expired;

        return proximity;
    }

    // Trigger automatic refund if timeout passed
    bool should_auto_refund(uint32_t timeout_height, uint32_t current_height) {
        if (current_height >= timeout_height) {
            stats.refunds_triggered++;
            return true;
        }
        return false;
    }

    // Set custom policy
    void set_policy(const TimeoutPolicy& p) {
        policy = p;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Race condition preventer
class RaceConditionPreventer {
private:
    // Track swap states to prevent double-spend and race conditions
    struct SwapState {
        std::string swap_id;
        bool initiator_locked;
        bool participant_locked;
        bool initiator_claimed;
        bool participant_claimed;
        bool initiator_refunded;
        bool participant_refunded;
        uint64_t last_update_time;
        std::mutex state_mutex;
    };

    std::unordered_map<std::string, std::shared_ptr<SwapState>> swap_states;
    mutable std::mutex global_mutex;

    struct Statistics {
        uint64_t swaps_tracked = 0;
        uint64_t race_conditions_prevented = 0;
        uint64_t double_spend_prevented = 0;
        uint64_t state_conflicts_detected = 0;
    } stats;

public:
    // Register new swap
    bool register_swap(const std::string& swap_id) {
        std::lock_guard<std::mutex> lock(global_mutex);

        if (swap_states.find(swap_id) != swap_states.end()) {
            return false;  // Already registered
        }

        auto state = std::make_shared<SwapState>();
        state->swap_id = swap_id;
        state->initiator_locked = false;
        state->participant_locked = false;
        state->initiator_claimed = false;
        state->participant_claimed = false;
        state->initiator_refunded = false;
        state->participant_refunded = false;
        state->last_update_time = std::chrono::system_clock::now().time_since_epoch().count();

        swap_states[swap_id] = state;
        stats.swaps_tracked++;

        return true;
    }

    // Attempt to lock initiator side
    struct LockAttempt {
        bool success;
        std::string error;
    };

    LockAttempt try_lock_initiator(const std::string& swap_id) {
        LockAttempt result;
        result.success = false;

        auto state = get_swap_state(swap_id);
        if (!state) {
            result.error = "Swap not registered";
            return result;
        }

        std::lock_guard<std::mutex> lock(state->state_mutex);

        // Check race condition: already locked
        if (state->initiator_locked) {
            result.error = "Initiator already locked (race condition prevented)";
            stats.race_conditions_prevented++;
            return result;
        }

        // Check race condition: already claimed
        if (state->initiator_claimed) {
            result.error = "Initiator already claimed (double-spend prevented)";
            stats.double_spend_prevented++;
            return result;
        }

        // Check race condition: already refunded
        if (state->initiator_refunded) {
            result.error = "Initiator already refunded";
            stats.state_conflicts_detected++;
            return result;
        }

        // Lock successful
        state->initiator_locked = true;
        state->last_update_time = std::chrono::system_clock::now().time_since_epoch().count();
        result.success = true;

        return result;
    }

    // Attempt to lock participant side
    LockAttempt try_lock_participant(const std::string& swap_id) {
        LockAttempt result;
        result.success = false;

        auto state = get_swap_state(swap_id);
        if (!state) {
            result.error = "Swap not registered";
            return result;
        }

        std::lock_guard<std::mutex> lock(state->state_mutex);

        // Require initiator locked first (ordering)
        if (!state->initiator_locked) {
            result.error = "Initiator must lock first (ordering violation)";
            stats.race_conditions_prevented++;
            return result;
        }

        // Check race condition: already locked
        if (state->participant_locked) {
            result.error = "Participant already locked (race condition prevented)";
            stats.race_conditions_prevented++;
            return result;
        }

        // Check race condition: already claimed
        if (state->participant_claimed) {
            result.error = "Participant already claimed (double-spend prevented)";
            stats.double_spend_prevented++;
            return result;
        }

        // Lock successful
        state->participant_locked = true;
        state->last_update_time = std::chrono::system_clock::now().time_since_epoch().count();
        result.success = true;

        return result;
    }

    // Attempt to claim
    struct ClaimAttempt {
        bool success;
        std::string error;
    };

    ClaimAttempt try_claim(const std::string& swap_id, bool is_initiator) {
        ClaimAttempt result;
        result.success = false;

        auto state = get_swap_state(swap_id);
        if (!state) {
            result.error = "Swap not registered";
            return result;
        }

        std::lock_guard<std::mutex> lock(state->state_mutex);

        if (is_initiator) {
            // Initiator can claim after participant locks and provides preimage
            if (!state->participant_locked) {
                result.error = "Participant not locked yet";
                return result;
            }

            if (state->initiator_claimed) {
                result.error = "Already claimed (double-spend prevented)";
                stats.double_spend_prevented++;
                return result;
            }

            if (state->initiator_refunded) {
                result.error = "Already refunded (conflict)";
                stats.state_conflicts_detected++;
                return result;
            }

            state->initiator_claimed = true;
        } else {
            // Participant claims first (after getting preimage from blockchain)
            if (!state->participant_locked) {
                result.error = "Not locked yet";
                return result;
            }

            if (state->participant_claimed) {
                result.error = "Already claimed (double-spend prevented)";
                stats.double_spend_prevented++;
                return result;
            }

            if (state->participant_refunded) {
                result.error = "Already refunded (conflict)";
                stats.state_conflicts_detected++;
                return result;
            }

            state->participant_claimed = true;
        }

        state->last_update_time = std::chrono::system_clock::now().time_since_epoch().count();
        result.success = true;

        return result;
    }

    // Get swap state (thread-safe copy)
    struct SwapStatus {
        bool exists;
        bool initiator_locked;
        bool participant_locked;
        bool initiator_claimed;
        bool participant_claimed;
        bool is_complete;
    };

    SwapStatus get_status(const std::string& swap_id) const {
        SwapStatus status;
        status.exists = false;

        auto state = get_swap_state(swap_id);
        if (!state) {
            return status;
        }

        std::lock_guard<std::mutex> lock(state->state_mutex);

        status.exists = true;
        status.initiator_locked = state->initiator_locked;
        status.participant_locked = state->participant_locked;
        status.initiator_claimed = state->initiator_claimed;
        status.participant_claimed = state->participant_claimed;
        status.is_complete = state->initiator_claimed && state->participant_claimed;

        return status;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

private:
    std::shared_ptr<SwapState> get_swap_state(const std::string& swap_id) const {
        std::lock_guard<std::mutex> lock(global_mutex);
        auto it = swap_states.find(swap_id);
        return (it != swap_states.end()) ? it->second : nullptr;
    }
};

// Atomic swap manager
class AtomicSwapManager {
private:
    TimeoutEnforcer timeout_enforcer;
    SPVProofValidator spv_validator;
    RaceConditionPreventer race_preventer;

    AtomicSwapManager() = default;

public:
    static AtomicSwapManager& instance() {
        static AtomicSwapManager instance;
        return instance;
    }

    // Create new atomic swap
    struct SwapCreation {
        bool success;
        std::string swap_id;
        std::string error;
    };

    SwapCreation create_swap(
        const std::string& swap_id,
        const HTLC::Parameters& initiator_params,
        const HTLC::Parameters& participant_params,
        uint32_t current_height
    ) {
        SwapCreation result;
        result.success = false;

        // Validate timeout parameters
        auto initiator_timeout = timeout_enforcer.validate_timeout(
            initiator_params.timeout_height,
            current_height
        );

        if (!initiator_timeout.is_valid) {
            result.error = "Initiator timeout invalid: " + initiator_timeout.error;
            return result;
        }

        auto participant_timeout = timeout_enforcer.validate_timeout(
            participant_params.timeout_height,
            current_height
        );

        if (!participant_timeout.is_valid) {
            result.error = "Participant timeout invalid: " + participant_timeout.error;
            return result;
        }

        // Ensure participant timeout < initiator timeout (safety)
        if (participant_params.timeout_height >= initiator_params.timeout_height) {
            result.error = "Participant timeout must be before initiator timeout";
            return result;
        }

        // Register swap to prevent race conditions
        if (!race_preventer.register_swap(swap_id)) {
            result.error = "Swap ID already exists";
            return result;
        }

        result.success = true;
        result.swap_id = swap_id;

        return result;
    }

    // Get timeout enforcer
    TimeoutEnforcer& get_timeout_enforcer() {
        return timeout_enforcer;
    }

    // Get SPV validator
    SPVProofValidator& get_spv_validator() {
        return spv_validator;
    }

    // Get race condition preventer
    RaceConditionPreventer& get_race_preventer() {
        return race_preventer;
    }
};

} // namespace swap
} // namespace intcoin

#endif // INTCOIN_ATOMIC_SWAP_H
