// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Channel Factory Implementation
// Batch channel creation for improved efficiency
//
// Channel factories allow multiple payment channels to be created from a
// single on-chain funding transaction, significantly reducing blockchain
// footprint and improving scalability.

#ifndef INTCOIN_CHANNEL_FACTORY_H
#define INTCOIN_CHANNEL_FACTORY_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <map>
#include <optional>

namespace intcoin {
namespace factory {

/**
 * Channel factory version
 */
static constexpr uint32_t CHANNEL_FACTORY_VERSION = 1;

/**
 * Maximum participants in a single factory
 */
static constexpr size_t MAX_FACTORY_PARTICIPANTS = 20;

/**
 * Maximum channels per factory
 */
static constexpr size_t MAX_FACTORY_CHANNELS = 100;

/**
 * Factory state
 */
enum class FactoryState {
    INITIALIZING,   // Factory being set up
    OPEN,           // Factory open, channels can be created
    CLOSING,        // Factory closing, no new channels
    CLOSED          // Factory closed
};

/**
 * Channel state within factory
 */
enum class FactoryChannelState {
    PROPOSED,       // Channel proposed but not approved
    APPROVED,       // Channel approved by all parties
    ACTIVE,         // Channel active
    CLOSING,        // Channel closing
    CLOSED          // Channel closed
};

/**
 * Factory participant
 */
struct FactoryParticipant {
    DilithiumPubKey pubkey;             // Participant's public key
    uint64_t contribution_sat;          // Amount contributed to factory
    Address refund_address;             // Address for refunds

    std::vector<uint8_t> serialize() const;
    static FactoryParticipant deserialize(const std::vector<uint8_t>& data);
};

/**
 * Factory channel
 * A payment channel created within the factory
 */
struct FactoryChannel {
    Hash256 channel_id;                 // Unique channel identifier
    FactoryChannelState state;          // Current channel state

    // Channel parties (indices into factory participants)
    size_t party_a_index;
    size_t party_b_index;

    // Channel capacity allocation
    uint64_t party_a_balance_sat;
    uint64_t party_b_balance_sat;

    // Current commitment state
    uint32_t commitment_number;
    Hash256 latest_commitment_hash;

    // Creation and closure
    uint32_t created_at;                // Block height
    uint32_t closed_at;                 // Block height (0 if open)

    FactoryChannel()
        : state(FactoryChannelState::PROPOSED),
          party_a_index(0), party_b_index(0),
          party_a_balance_sat(0), party_b_balance_sat(0),
          commitment_number(0),
          created_at(0), closed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static FactoryChannel deserialize(const std::vector<uint8_t>& data);
};

/**
 * Channel factory
 * Multi-party funding transaction containing multiple channels
 */
struct ChannelFactory {
    Hash256 factory_id;                 // Unique factory identifier
    FactoryState state;                 // Current factory state

    std::vector<FactoryParticipant> participants;  // All participants
    std::vector<FactoryChannel> channels;          // All channels in factory

    // Factory funding
    Transaction funding_tx;              // On-chain funding transaction
    uint64_t total_capacity_sat;        // Total factory capacity
    uint32_t timeout_height;            // Timeout for cooperative close

    // Creation and closure
    uint32_t created_at;                // Block height when created
    uint32_t closed_at;                 // Block height when closed (0 if open)

    ChannelFactory()
        : state(FactoryState::INITIALIZING),
          total_capacity_sat(0),
          timeout_height(0),
          created_at(0),
          closed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static ChannelFactory deserialize(const std::vector<uint8_t>& data);

    // Get participant by pubkey
    std::optional<size_t> get_participant_index(const DilithiumPubKey& pubkey) const;

    // Get total allocated capacity
    uint64_t get_allocated_capacity() const;

    // Get unallocated capacity
    uint64_t get_unallocated_capacity() const;

    // Count channels by state
    size_t count_channels_by_state(FactoryChannelState state) const;
};

/**
 * Channel proposal
 * Proposal to create a new channel within the factory
 */
struct ChannelProposal {
    Hash256 proposal_id;                // Unique proposal identifier
    Hash256 factory_id;                 // Factory this belongs to

    size_t party_a_index;               // First party (participant index)
    size_t party_b_index;               // Second party (participant index)

    uint64_t party_a_balance_sat;       // Initial balance for party A
    uint64_t party_b_balance_sat;       // Initial balance for party B

    uint32_t timeout;                   // Proposal timeout (block height)

    // Approvals
    std::map<size_t, DilithiumSignature> approvals;  // participant_index -> signature

    std::vector<uint8_t> serialize() const;
    static ChannelProposal deserialize(const std::vector<uint8_t>& data);

    // Check if proposal is fully approved
    bool is_fully_approved(size_t num_participants) const;

    // Get total channel capacity
    uint64_t get_total_capacity() const {
        return party_a_balance_sat + party_b_balance_sat;
    }
};

/**
 * Factory manager
 * Manages channel factories and coordinates multi-party operations
 */
class ChannelFactoryManager {
public:
    ChannelFactoryManager();
    ~ChannelFactoryManager() = default;

    // ========================================================================
    // Factory Creation
    // ========================================================================

    /**
     * Initiate new channel factory
     *
     * @param participants List of participants with contributions
     * @param timeout_blocks Timeout for cooperative close (blocks)
     * @return Factory ID if successful
     */
    std::optional<Hash256> create_factory(
        const std::vector<FactoryParticipant>& participants,
        uint32_t timeout_blocks = 4320  // ~1 month
    );

    /**
     * Fund channel factory
     * Broadcasts the funding transaction
     *
     * @param factory_id Factory ID
     * @param funding_tx Multi-party funding transaction
     * @return True if successful
     */
    bool fund_factory(
        const Hash256& factory_id,
        const Transaction& funding_tx
    );

    /**
     * Activate factory
     * Called after funding transaction confirms
     *
     * @param factory_id Factory ID
     * @param confirmation_height Block height of confirmation
     * @return True if successful
     */
    bool activate_factory(
        const Hash256& factory_id,
        uint32_t confirmation_height
    );

    // ========================================================================
    // Channel Management
    // ========================================================================

    /**
     * Propose new channel within factory
     *
     * @param factory_id Factory ID
     * @param party_a_index First party (participant index)
     * @param party_b_index Second party (participant index)
     * @param party_a_balance Initial balance for party A
     * @param party_b_balance Initial balance for party B
     * @return Proposal ID if successful
     */
    std::optional<Hash256> propose_channel(
        const Hash256& factory_id,
        size_t party_a_index,
        size_t party_b_index,
        uint64_t party_a_balance,
        uint64_t party_b_balance
    );

    /**
     * Approve channel proposal
     * All participants must approve before channel is created
     *
     * @param proposal_id Proposal ID
     * @param participant_index Participant approving
     * @param signature Signature over proposal
     * @return True if approved successfully
     */
    bool approve_channel_proposal(
        const Hash256& proposal_id,
        size_t participant_index,
        const DilithiumSignature& signature
    );

    /**
     * Create channel from approved proposal
     * Called automatically when all approvals received
     *
     * @param proposal_id Proposal ID
     * @return Channel ID if successful
     */
    std::optional<Hash256> create_channel_from_proposal(
        const Hash256& proposal_id
    );

    /**
     * Update channel state
     * Updates commitment for a channel within factory
     *
     * @param factory_id Factory ID
     * @param channel_id Channel ID
     * @param new_commitment New commitment transaction
     * @return True if successful
     */
    bool update_channel_state(
        const Hash256& factory_id,
        const Hash256& channel_id,
        const lightning::CommitmentTransaction& new_commitment
    );

    /**
     * Close channel within factory
     * Closes a channel without closing the factory
     *
     * @param factory_id Factory ID
     * @param channel_id Channel ID
     * @param cooperative True for cooperative close
     * @return True if successful
     */
    bool close_factory_channel(
        const Hash256& factory_id,
        const Hash256& channel_id,
        bool cooperative = true
    );

    // ========================================================================
    // Factory Closure
    // ========================================================================

    /**
     * Initiate factory closure
     * Begins process of closing all channels and settling on-chain
     *
     * @param factory_id Factory ID
     * @param cooperative True for cooperative close
     * @return True if closure initiated
     */
    bool initiate_factory_closure(
        const Hash256& factory_id,
        bool cooperative = true
    );

    /**
     * Finalize factory closure
     * Broadcasts final settlement transaction
     *
     * @param factory_id Factory ID
     * @return Settlement transaction if successful
     */
    std::optional<Transaction> finalize_factory_closure(
        const Hash256& factory_id
    );

    // ========================================================================
    // Factory Queries
    // ========================================================================

    /**
     * Get factory details
     */
    std::optional<ChannelFactory> get_factory(const Hash256& factory_id) const;

    /**
     * List all factories
     */
    std::vector<ChannelFactory> list_factories() const;

    /**
     * List factories by state
     */
    std::vector<ChannelFactory> list_factories_by_state(FactoryState state) const;

    /**
     * Get channel details
     */
    std::optional<FactoryChannel> get_factory_channel(
        const Hash256& factory_id,
        const Hash256& channel_id
    ) const;

    /**
     * List all channels in factory
     */
    std::vector<FactoryChannel> list_factory_channels(
        const Hash256& factory_id
    ) const;

    /**
     * Get pending proposals
     */
    std::vector<ChannelProposal> get_pending_proposals(
        const Hash256& factory_id
    ) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct FactoryStats {
        size_t total_factories;
        size_t open_factories;
        size_t total_channels;
        size_t active_channels;
        uint64_t total_capacity_sat;
        uint64_t locked_capacity_sat;
        double avg_channels_per_factory;
        double on_chain_savings_percent;  // vs individual channels
    };

    FactoryStats get_stats() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Set maximum participants per factory
     */
    void set_max_participants(size_t max_participants);

    /**
     * Set maximum channels per factory
     */
    void set_max_channels(size_t max_channels);

private:
    // Factory storage
    std::map<Hash256, ChannelFactory> factories_;

    // Proposal storage
    std::map<Hash256, ChannelProposal> proposals_;

    // Configuration
    size_t max_participants_;
    size_t max_channels_;

    // Current block height
    uint32_t current_height_;

    // Thread safety
    mutable std::mutex mutex_;

    // Helper methods
    Hash256 generate_factory_id() const;
    Hash256 generate_channel_id() const;
    Hash256 generate_proposal_id() const;

    Transaction create_funding_transaction(const ChannelFactory& factory) const;
    Transaction create_settlement_transaction(const ChannelFactory& factory) const;

    bool validate_factory_participants(const std::vector<FactoryParticipant>& participants) const;
    bool validate_channel_proposal(const ChannelFactory& factory, const ChannelProposal& proposal) const;

    void update_factory_state(const Hash256& factory_id, FactoryState new_state);
    void update_channel_state(const Hash256& factory_id, const Hash256& channel_id, FactoryChannelState new_state);
};

/**
 * Factory coordinator
 * Coordinates multi-party operations for factory management
 */
class FactoryCoordinator {
public:
    FactoryCoordinator(const DilithiumPrivKey& participant_privkey);
    ~FactoryCoordinator() = default;

    /**
     * Join factory creation
     *
     * @param factory_id Factory being created
     * @param contribution Amount to contribute
     * @param refund_address Address for refunds
     * @return True if joined successfully
     */
    bool join_factory(
        const Hash256& factory_id,
        uint64_t contribution,
        const Address& refund_address
    );

    /**
     * Sign funding transaction
     * Multi-sig signing of factory funding transaction
     *
     * @param factory_id Factory ID
     * @param funding_tx Funding transaction to sign
     * @return Signature if successful
     */
    std::optional<DilithiumSignature> sign_funding_transaction(
        const Hash256& factory_id,
        const Transaction& funding_tx
    );

    /**
     * Request channel in factory
     * Proposes a new channel with another participant
     *
     * @param factory_id Factory ID
     * @param peer_pubkey Peer's public key
     * @param local_balance Desired local balance
     * @param remote_balance Desired remote balance
     * @return Proposal ID if successful
     */
    std::optional<Hash256> request_channel(
        const Hash256& factory_id,
        const DilithiumPubKey& peer_pubkey,
        uint64_t local_balance,
        uint64_t remote_balance
    );

    /**
     * Approve channel request
     * Approves a channel proposal in the factory
     *
     * @param proposal_id Proposal ID
     * @return True if approved
     */
    bool approve_channel_request(const Hash256& proposal_id);

    /**
     * Leave factory
     * Initiates cooperative exit from factory
     *
     * @param factory_id Factory ID
     * @return True if exit initiated
     */
    bool leave_factory(const Hash256& factory_id);

private:
    DilithiumPrivKey participant_privkey_;
    DilithiumPubKey participant_pubkey_;

    ChannelFactoryManager* factory_manager_;

    mutable std::mutex mutex_;
};

} // namespace factory
} // namespace intcoin

#endif // INTCOIN_CHANNEL_FACTORY_H
