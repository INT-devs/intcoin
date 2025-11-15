// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Eltoo Implementation
// Simplified channel updates using SIGHASH_NOINPUT
//
// Eltoo is a channel update mechanism that simplifies Lightning channels
// by eliminating the need for penalty transactions and revocation keys.
// It uses SIGHASH_NOINPUT to allow update transactions to spend any
// previous update, making channel management much simpler.

#ifndef INTCOIN_ELTOO_H
#define INTCOIN_ELTOO_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>

namespace intcoin {
namespace eltoo {

/**
 * Eltoo protocol version
 */
static constexpr uint32_t ELTOO_VERSION = 1;

/**
 * SIGHASH flags for Eltoo
 */
enum class SigHashType : uint8_t {
    ALL = 0x01,                 // Sign all inputs and outputs
    NONE = 0x02,                // Sign all inputs, no outputs
    SINGLE = 0x03,              // Sign all inputs, one output
    NOINPUT = 0x40,             // Don't commit to any input (Eltoo)
    ANYPREVOUT = 0x41           // SIGHASH_ANYPREVOUT (alternative name)
};

/**
 * Eltoo update transaction
 * Can spend any previous update transaction
 */
struct EltooUpdate {
    uint32_t update_number;             // Monotonically increasing update counter

    // Settlement outputs
    uint64_t party_a_balance_sat;
    uint64_t party_b_balance_sat;

    // Keys
    DilithiumPubKey party_a_pubkey;
    DilithiumPubKey party_b_pubkey;

    // Update transaction (spends funding output)
    Transaction update_tx;

    // Settlement transaction (spends update output after timelock)
    Transaction settlement_tx;
    uint32_t settlement_delay;          // CSV timelock (blocks)

    // Signatures (using SIGHASH_NOINPUT)
    DilithiumSignature party_a_sig;
    DilithiumSignature party_b_sig;

    uint64_t timestamp;                 // Creation timestamp

    EltooUpdate()
        : update_number(0),
          party_a_balance_sat(0), party_b_balance_sat(0),
          settlement_delay(144),
          timestamp(0) {}

    std::vector<uint8_t> serialize() const;
    static EltooUpdate deserialize(const std::vector<uint8_t>& data);

    // Get total channel capacity
    uint64_t get_capacity() const {
        return party_a_balance_sat + party_b_balance_sat;
    }
};

/**
 * Eltoo channel state
 */
enum class EltooChannelState {
    INITIALIZING,       // Channel being set up
    OPEN,               // Channel open and active
    CLOSING,            // Cooperative close in progress
    FORCE_CLOSING,      // Unilateral close (update tx broadcast)
    CLOSED              // Channel closed
};

/**
 * Eltoo channel
 * Lightning channel using Eltoo update mechanism
 */
struct EltooChannel {
    Hash256 channel_id;                 // Unique channel identifier
    EltooChannelState state;            // Current state

    // Participants
    DilithiumPubKey local_pubkey;
    DilithiumPubKey remote_pubkey;

    // Funding
    Transaction funding_tx;
    uint64_t funding_amount_sat;
    uint32_t funding_confirmation_height;

    // Current state
    uint32_t current_update_number;
    uint64_t local_balance_sat;
    uint64_t remote_balance_sat;

    // Update history (keep recent updates)
    std::vector<EltooUpdate> recent_updates;
    size_t max_stored_updates;

    // Settlement parameters
    uint32_t settlement_delay_blocks;   // Default CSV delay

    // Channel parameters
    uint64_t dust_limit_sat;
    uint64_t max_htlc_value_in_flight_sat;
    uint64_t channel_reserve_sat;

    uint32_t created_at;                // Block height
    uint32_t closed_at;                 // Block height (0 if open)

    EltooChannel()
        : state(EltooChannelState::INITIALIZING),
          funding_amount_sat(0), funding_confirmation_height(0),
          current_update_number(0),
          local_balance_sat(0), remote_balance_sat(0),
          max_stored_updates(10),
          settlement_delay_blocks(144),
          dust_limit_sat(546),
          max_htlc_value_in_flight_sat(0),
          channel_reserve_sat(0),
          created_at(0), closed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static EltooChannel deserialize(const std::vector<uint8_t>& data);

    // Get latest update
    std::optional<EltooUpdate> get_latest_update() const {
        if (recent_updates.empty()) return std::nullopt;
        return recent_updates.back();
    }

    // Add new update
    void add_update(const EltooUpdate& update) {
        recent_updates.push_back(update);
        if (recent_updates.size() > max_stored_updates) {
            recent_updates.erase(recent_updates.begin());
        }
    }
};

/**
 * Eltoo channel manager
 * Manages channels using Eltoo update mechanism
 */
class EltooChannelManager {
public:
    EltooChannelManager();
    ~EltooChannelManager() = default;

    // ========================================================================
    // Channel Opening
    // ========================================================================

    /**
     * Open new Eltoo channel
     *
     * @param peer_pubkey Peer's public key
     * @param local_funding Local funding amount
     * @param remote_funding Remote funding amount
     * @param settlement_delay Settlement delay in blocks
     * @return Channel ID if successful
     */
    std::optional<Hash256> open_channel(
        const DilithiumPubKey& peer_pubkey,
        uint64_t local_funding,
        uint64_t remote_funding = 0,
        uint32_t settlement_delay = 144
    );

    /**
     * Accept channel opening from peer
     */
    bool accept_channel(
        const Hash256& channel_id,
        uint64_t remote_funding
    );

    /**
     * Confirm funding transaction
     */
    bool confirm_funding(
        const Hash256& channel_id,
        uint32_t confirmation_height
    );

    // ========================================================================
    // Channel Updates (Simplified with Eltoo)
    // ========================================================================

    /**
     * Create new channel update
     * With Eltoo, no revocation is needed - just create new update
     *
     * @param channel_id Channel ID
     * @param new_local_balance New local balance
     * @param new_remote_balance New remote balance
     * @return Update if successful
     */
    std::optional<EltooUpdate> create_update(
        const Hash256& channel_id,
        uint64_t new_local_balance,
        uint64_t new_remote_balance
    );

    /**
     * Sign update transaction
     * Uses SIGHASH_NOINPUT to allow spending any previous update
     */
    bool sign_update(
        const Hash256& channel_id,
        uint32_t update_number,
        const DilithiumSignature& signature
    );

    /**
     * Apply signed update
     * Updates channel state to new balances
     */
    bool apply_update(
        const Hash256& channel_id,
        const EltooUpdate& update
    );

    /**
     * Get signing hash for update transaction
     * Uses SIGHASH_NOINPUT flag
     */
    Hash256 get_update_sighash(
        const EltooUpdate& update,
        SigHashType sighash_type = SigHashType::NOINPUT
    ) const;

    // ========================================================================
    // Payment Operations
    // ========================================================================

    /**
     * Send payment through channel
     * Creates new update with updated balances
     */
    bool send_payment(
        const Hash256& channel_id,
        uint64_t amount_sat
    );

    /**
     * Receive payment through channel
     * Creates new update with updated balances
     */
    bool receive_payment(
        const Hash256& channel_id,
        uint64_t amount_sat
    );

    // ========================================================================
    // Channel Closing
    // ========================================================================

    /**
     * Close channel cooperatively
     * Creates final settlement transaction
     */
    bool close_channel_cooperative(
        const Hash256& channel_id
    );

    /**
     * Force close channel
     * Broadcasts latest update transaction
     * Settlement tx can be broadcast after timelock
     */
    bool close_channel_force(
        const Hash256& channel_id
    );

    /**
     * Broadcast settlement transaction
     * Called after settlement delay expires
     */
    bool broadcast_settlement(
        const Hash256& channel_id
    );

    // ========================================================================
    // Channel Queries
    // ========================================================================

    /**
     * Get channel details
     */
    std::optional<EltooChannel> get_channel(const Hash256& channel_id) const;

    /**
     * List all channels
     */
    std::vector<EltooChannel> list_channels() const;

    /**
     * List channels by state
     */
    std::vector<EltooChannel> list_channels_by_state(EltooChannelState state) const;

    /**
     * Get channel balance
     */
    std::pair<uint64_t, uint64_t> get_channel_balance(const Hash256& channel_id) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct EltooStats {
        size_t total_channels;
        size_t open_channels;
        uint64_t total_capacity_sat;
        uint64_t total_local_balance_sat;
        uint64_t total_remote_balance_sat;
        uint64_t total_updates_created;
        double avg_updates_per_channel;
    };

    EltooStats get_stats() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Set default settlement delay
     */
    void set_default_settlement_delay(uint32_t blocks);

    /**
     * Set maximum stored updates per channel
     */
    void set_max_stored_updates(size_t max_updates);

private:
    std::map<Hash256, EltooChannel> channels_;

    uint32_t default_settlement_delay_;
    size_t max_stored_updates_;
    uint32_t current_height_;

    mutable std::mutex mutex_;

    // Helper methods
    Hash256 generate_channel_id() const;

    Transaction create_funding_transaction(const EltooChannel& channel) const;

    Transaction create_update_transaction(
        const EltooChannel& channel,
        uint32_t update_number,
        uint64_t party_a_balance,
        uint64_t party_b_balance
    ) const;

    Transaction create_settlement_transaction(
        const EltooChannel& channel,
        const EltooUpdate& update
    ) const;

    bool validate_update(
        const EltooChannel& channel,
        const EltooUpdate& update
    ) const;

    // SIGHASH_NOINPUT signing
    Hash256 compute_sighash_noinput(
        const Transaction& tx,
        size_t input_index,
        const std::vector<uint8_t>& script_code,
        uint64_t amount,
        SigHashType sighash_type
    ) const;
};

/**
 * Eltoo advantages over traditional Lightning channels:
 *
 * 1. No penalty transactions - simpler and safer
 * 2. No revocation keys - easier key management
 * 3. Any update can be published - no need to track all states
 * 4. Simpler watchtower protocol - just store latest update
 * 5. Better for channel factories - easier to manage multiple channels
 * 6. Reduced storage requirements - don't need to keep all old states
 * 7. Simpler backup/restore - just need latest state
 *
 * Technical Requirements:
 * - SIGHASH_NOINPUT signature flag (soft fork required)
 * - Monotonically increasing update numbers
 * - CSV (CheckSequenceVerify) for settlement delay
 */

} // namespace eltoo
} // namespace intcoin

#endif // INTCOIN_ELTOO_H
