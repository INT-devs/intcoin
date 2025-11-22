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
 * Eltoo Watchtower
 * Monitors channels and responds to old state broadcasts
 * Much simpler than traditional Lightning watchtowers!
 */
class EltooWatchtower {
public:
    EltooWatchtower();
    ~EltooWatchtower() = default;

    /**
     * Store latest update for monitoring (simple!)
     * Only need latest state, not all previous states
     */
    bool store_update(const Hash256& channel_id, const EltooUpdate& update);

    /**
     * Retrieve latest stored update
     */
    std::optional<EltooUpdate> get_latest_update(const Hash256& channel_id) const;

    /**
     * Start monitoring a channel
     */
    void monitor_channel(const Hash256& channel_id);

    /**
     * Stop monitoring a channel
     */
    void stop_monitoring(const Hash256& channel_id);

    /**
     * Check for old state broadcasts
     */
    struct StateViolation {
        Hash256 channel_id;
        uint32_t broadcast_update_number;
        uint32_t latest_update_number;
        uint64_t detected_at_height;
        Transaction violating_tx;
        Hash256 violating_txid;
    };

    std::vector<StateViolation> check_for_violations(uint32_t current_height);

    /**
     * Respond to violation by broadcasting latest update
     */
    bool respond_to_violation(const StateViolation& violation);

    /**
     * Export watchtower data for backup
     */
    std::vector<uint8_t> export_data() const;
    bool import_data(const std::vector<uint8_t>& data);

    /**
     * Statistics
     */
    struct WatchtowerStats {
        size_t monitored_channels;
        size_t total_updates_stored;
        size_t violations_detected;
        size_t violations_responded;
        uint64_t total_storage_bytes;
    };

    WatchtowerStats get_stats() const;

    /**
     * Cleanup closed channels
     */
    void cleanup_closed_channels(uint32_t blocks_ago);

private:
    struct WatchtowerData {
        Hash256 channel_id;
        EltooUpdate latest_update;
        uint64_t last_check_height;
        bool actively_monitored;
        uint64_t added_timestamp;

        WatchtowerData()
            : last_check_height(0)
            , actively_monitored(false)
            , added_timestamp(0)
        {}
    };

    std::map<Hash256, WatchtowerData> watched_channels_;
    mutable std::mutex watchtower_mutex_;

    // Violation tracking
    std::vector<StateViolation> detected_violations_;
    size_t violations_responded_;

    // Callbacks
    std::function<void(const StateViolation&)> violation_callback_;
};

/**
 * Eltoo Channel Factory
 * Multi-party channels (much easier with Eltoo!)
 */
class EltooChannelFactory {
public:
    EltooChannelFactory();
    ~EltooChannelFactory() = default;

    /**
     * Multi-party channel structure
     */
    struct MultiPartyChannel {
        Hash256 factory_id;
        std::vector<DilithiumPubKey> participants;
        std::vector<uint64_t> balances;
        uint32_t update_number;
        Transaction funding_tx;
        uint32_t created_at_height;
        bool is_active;

        MultiPartyChannel()
            : update_number(0)
            , created_at_height(0)
            , is_active(false)
        {}

        uint64_t get_total_capacity() const {
            uint64_t total = 0;
            for (auto balance : balances) {
                total += balance;
            }
            return total;
        }
    };

    /**
     * Create multi-party channel factory
     */
    std::optional<Hash256> create_factory(
        const std::vector<DilithiumPubKey>& participants,
        const std::vector<uint64_t>& initial_balances
    );

    /**
     * Update factory state
     */
    bool update_factory(
        const Hash256& factory_id,
        const std::vector<uint64_t>& new_balances
    );

    /**
     * Create sub-channel within factory
     */
    std::optional<Hash256> create_subchannel(
        const Hash256& factory_id,
        size_t participant_a_index,
        size_t participant_b_index,
        uint64_t amount
    );

    /**
     * Close channel factory
     */
    bool close_factory(const Hash256& factory_id);

    /**
     * Get factory details
     */
    std::optional<MultiPartyChannel> get_factory(const Hash256& factory_id) const;

    /**
     * List all factories
     */
    std::vector<MultiPartyChannel> list_factories() const;

    /**
     * Statistics
     */
    struct FactoryStats {
        size_t total_factories;
        size_t active_factories;
        size_t total_participants;
        uint64_t total_capacity_sat;
        size_t total_subchannels_created;
    };

    FactoryStats get_stats() const;

private:
    std::map<Hash256, MultiPartyChannel> factories_;
    std::map<Hash256, Hash256> subchannel_to_factory_;  // Subchannel ID -> Factory ID
    mutable std::mutex factory_mutex_;

    size_t total_subchannels_created_;

    Hash256 generate_factory_id() const;
    bool validate_balances(const std::vector<uint64_t>& balances) const;
};

/**
 * Enhanced Eltoo configuration
 */
struct EltooConfig {
    // Settlement parameters
    uint32_t default_settlement_delay;     // Default CSV delay (blocks)
    uint32_t min_settlement_delay;         // Minimum allowed delay
    uint32_t max_settlement_delay;         // Maximum allowed delay

    // Funding parameters
    uint32_t funding_confirmations;        // Required funding tx confirmations
    uint64_t min_channel_capacity;         // Minimum channel size (sats)
    uint64_t max_channel_capacity;         // Maximum channel size (sats)

    // Update parameters
    size_t max_stored_updates;             // Max updates to store per channel
    uint32_t update_timeout_blocks;        // Timeout for update completion

    // Watchtower settings
    bool enable_watchtower;                // Enable watchtower monitoring
    uint32_t watchtower_check_interval;    // Check interval (blocks)
    size_t max_watchtower_channels;        // Max channels to monitor

    // Channel factory settings
    bool enable_channel_factories;         // Enable multi-party channels
    size_t max_factory_participants;       // Max participants per factory
    uint64_t min_factory_capacity;         // Minimum factory size

    // Performance settings
    size_t max_concurrent_channels;        // Max concurrent open channels
    bool enable_batch_updates;             // Enable batched update processing

    EltooConfig()
        : default_settlement_delay(144)     // ~1 day (5 min blocks)
        , min_settlement_delay(6)           // ~30 minutes
        , max_settlement_delay(2016)        // ~1 week
        , funding_confirmations(6)
        , min_channel_capacity(100000)      // 100K sats
        , max_channel_capacity(10000000000) // 100 INT
        , max_stored_updates(10)
        , update_timeout_blocks(144)
        , enable_watchtower(true)
        , watchtower_check_interval(1)      // Check every block
        , max_watchtower_channels(1000)
        , enable_channel_factories(false)   // Opt-in
        , max_factory_participants(10)
        , min_factory_capacity(1000000)     // 1M sats
        , max_concurrent_channels(100)
        , enable_batch_updates(false)
    {}
};

/**
 * SIGHASH_NOINPUT utilities
 */
class SigHashNOINPUT {
public:
    /**
     * Calculate signature hash with NOINPUT flag
     *
     * SIGHASH_NOINPUT doesn't commit to:
     * - Input transaction ID (txid)
     * - Input output index (vout)
     * - Input sequence number
     *
     * This allows the signature to be valid for spending any input!
     */
    static Hash256 calculate_sighash_noinput(
        const Transaction& tx,
        uint32_t input_index,
        const std::vector<uint8_t>& script_code,
        uint64_t amount
    );

    /**
     * Sign transaction with SIGHASH_NOINPUT
     */
    static DilithiumSignature sign_noinput(
        const Transaction& tx,
        uint32_t input_index,
        const std::vector<uint8_t>& script_code,
        uint64_t amount,
        const DilithiumPrivateKey& privkey
    );

    /**
     * Verify SIGHASH_NOINPUT signature
     */
    static bool verify_noinput(
        const Transaction& tx,
        uint32_t input_index,
        const std::vector<uint8_t>& script_code,
        uint64_t amount,
        const DilithiumSignature& signature,
        const DilithiumPubKey& pubkey
    );

    /**
     * Batch verify multiple NOINPUT signatures
     */
    static bool batch_verify_noinput(
        const std::vector<Transaction>& txs,
        const std::vector<uint32_t>& input_indices,
        const std::vector<std::vector<uint8_t>>& script_codes,
        const std::vector<uint64_t>& amounts,
        const std::vector<DilithiumSignature>& signatures,
        const std::vector<DilithiumPubKey>& pubkeys
    );
};

/**
 * Eltoo backup and restore utilities
 */
class EltooBackup {
public:
    /**
     * Export channel backup (minimal data needed with Eltoo!)
     */
    static std::vector<uint8_t> export_channel(const EltooChannel& channel);

    /**
     * Import channel from backup
     */
    static std::optional<EltooChannel> import_channel(const std::vector<uint8_t>& data);

    /**
     * Export all channels
     */
    static std::vector<uint8_t> export_all_channels(
        const std::vector<EltooChannel>& channels
    );

    /**
     * Import multiple channels
     */
    static std::vector<EltooChannel> import_all_channels(
        const std::vector<uint8_t>& data
    );

    /**
     * Verify backup integrity
     */
    static bool verify_backup(const std::vector<uint8_t>& data);
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
 * 8. Faster channel updates - no revocation ceremony
 * 9. Better privacy - no breach remedies to leak
 * 10. Easier implementation - less complex state machine
 *
 * Technical Requirements:
 * - SIGHASH_NOINPUT signature flag (soft fork required)
 * - Monotonically increasing update numbers
 * - CSV (CheckSequenceVerify) for settlement delay
 *
 * Performance Benefits:
 * - 80% reduction in storage per update
 * - O(1) backup size vs O(n) for traditional channels
 * - O(1) watchtower storage vs O(n)
 * - Faster update processing
 */

} // namespace eltoo
} // namespace intcoin

#endif // INTCOIN_ELTOO_H
