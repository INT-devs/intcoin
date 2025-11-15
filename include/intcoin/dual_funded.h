// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Dual-Funded Channel Implementation
// Both parties contribute funds during channel opening
//
// Dual-funded channels allow both parties to contribute funds when opening
// a channel, providing immediate bidirectional capacity and better liquidity
// distribution compared to single-funded channels.

#ifndef INTCOIN_DUAL_FUNDED_H
#define INTCOIN_DUAL_FUNDED_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>

namespace intcoin {
namespace dual_funded {

/**
 * Dual-funded protocol version
 */
static constexpr uint32_t DUAL_FUNDED_VERSION = 1;

/**
 * Minimum contribution per party (in satoshis)
 */
static constexpr uint64_t MIN_CONTRIBUTION = 10000;

/**
 * Dual-funded channel state
 */
enum class DualFundedState {
    INIT,               // Initialization started
    NEGOTIATING,        // Negotiating contributions and fees
    FUNDING,            // Creating funding transaction
    SIGNED,             // Funding transaction signed by both parties
    BROADCAST,          // Funding transaction broadcast
    CONFIRMED,          // Funding transaction confirmed
    ACTIVE,             // Channel active
    FAILED              // Channel opening failed
};

/**
 * Funding contribution
 */
struct FundingContribution {
    DilithiumPubKey contributor;        // Contributing party
    uint64_t amount_sat;                // Contribution amount
    std::vector<TxInput> inputs;        // UTXO inputs for funding
    Address change_address;             // Change address

    std::vector<uint8_t> serialize() const;
    static FundingContribution deserialize(const std::vector<uint8_t>& data);
};

/**
 * Fee negotiation proposal
 */
struct FeeProposal {
    uint64_t fee_sat;                   // Proposed total fee
    double fee_rate_sat_per_vbyte;      // Fee rate
    uint64_t local_fee_sat;             // Proposer's fee contribution
    uint64_t remote_fee_sat;            // Peer's fee contribution

    std::vector<uint8_t> serialize() const;
    static FeeProposal deserialize(const std::vector<uint8_t>& data);
};

/**
 * Dual-funded channel
 */
struct DualFundedChannel {
    Hash256 channel_id;                 // Unique channel identifier
    DualFundedState state;              // Current state

    // Participants
    DilithiumPubKey local_pubkey;
    DilithiumPubKey remote_pubkey;

    // Contributions
    FundingContribution local_contribution;
    FundingContribution remote_contribution;

    // Fee negotiation
    FeeProposal agreed_fee;
    std::vector<FeeProposal> fee_proposals;  // Negotiation history

    // Channel parameters
    uint64_t local_balance_sat;
    uint64_t remote_balance_sat;
    uint64_t total_capacity_sat;

    // Funding transaction
    Transaction funding_tx;
    uint32_t confirmation_height;

    // Commitment transactions
    lightning::CommitmentTransaction local_commitment;
    lightning::CommitmentTransaction remote_commitment;

    uint32_t created_at;                // Block height
    uint32_t activated_at;              // Block height

    DualFundedChannel()
        : state(DualFundedState::INIT),
          local_balance_sat(0), remote_balance_sat(0), total_capacity_sat(0),
          confirmation_height(0),
          created_at(0), activated_at(0) {}

    std::vector<uint8_t> serialize() const;
    static DualFundedChannel deserialize(const std::vector<uint8_t>& data);

    // Calculate total capacity
    uint64_t calculate_capacity() const {
        return local_contribution.amount_sat + remote_contribution.amount_sat;
    }
};

/**
 * Dual-funded channel manager
 */
class DualFundedChannelManager {
public:
    DualFundedChannelManager();
    ~DualFundedChannelManager() = default;

    // ========================================================================
    // Channel Opening (Initiator)
    // ========================================================================

    /**
     * Initiate dual-funded channel opening
     *
     * @param peer_pubkey Peer's public key
     * @param local_amount Local contribution
     * @param inputs UTXOs for funding
     * @param change_address Change address
     * @return Channel ID if successful
     */
    std::optional<Hash256> initiate_dual_funded_channel(
        const DilithiumPubKey& peer_pubkey,
        uint64_t local_amount,
        const std::vector<TxInput>& inputs,
        const Address& change_address
    );

    /**
     * Propose fee structure
     *
     * @param channel_id Channel ID
     * @param fee_proposal Fee proposal
     * @return True if sent successfully
     */
    bool propose_fee(
        const Hash256& channel_id,
        const FeeProposal& fee_proposal
    );

    // ========================================================================
    // Channel Opening (Acceptor)
    // ========================================================================

    /**
     * Accept dual-funded channel request
     *
     * @param channel_id Channel ID
     * @param local_amount Local contribution
     * @param inputs UTXOs for funding
     * @param change_address Change address
     * @return True if accepted
     */
    bool accept_dual_funded_channel(
        const Hash256& channel_id,
        uint64_t local_amount,
        const std::vector<TxInput>& inputs,
        const Address& change_address
    );

    /**
     * Accept fee proposal
     *
     * @param channel_id Channel ID
     * @param proposal_index Index of proposal to accept
     * @return True if accepted
     */
    bool accept_fee_proposal(
        const Hash256& channel_id,
        size_t proposal_index
    );

    // ========================================================================
    // Funding Transaction
    // ========================================================================

    /**
     * Create funding transaction
     * Called after fee agreement reached
     *
     * @param channel_id Channel ID
     * @return True if created successfully
     */
    bool create_funding_transaction(const Hash256& channel_id);

    /**
     * Sign funding transaction
     *
     * @param channel_id Channel ID
     * @param signature Signature over funding transaction
     * @return True if signed successfully
     */
    bool sign_funding_transaction(
        const Hash256& channel_id,
        const DilithiumSignature& signature
    );

    /**
     * Broadcast funding transaction
     * Called after both parties sign
     *
     * @param channel_id Channel ID
     * @return True if broadcast successfully
     */
    bool broadcast_funding_transaction(const Hash256& channel_id);

    /**
     * Confirm funding transaction
     * Called when funding tx confirms on-chain
     *
     * @param channel_id Channel ID
     * @param confirmation_height Block height
     * @return True if confirmed successfully
     */
    bool confirm_funding_transaction(
        const Hash256& channel_id,
        uint32_t confirmation_height
    );

    // ========================================================================
    // Channel Management
    // ========================================================================

    /**
     * Get channel details
     */
    std::optional<DualFundedChannel> get_channel(const Hash256& channel_id) const;

    /**
     * List all dual-funded channels
     */
    std::vector<DualFundedChannel> list_channels() const;

    /**
     * List channels by state
     */
    std::vector<DualFundedChannel> list_channels_by_state(DualFundedState state) const;

    /**
     * Cancel channel opening
     */
    bool cancel_channel(const Hash256& channel_id);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct DualFundedStats {
        size_t total_channels;
        size_t active_channels;
        size_t failed_channels;
        uint64_t total_local_contribution_sat;
        uint64_t total_remote_contribution_sat;
        uint64_t total_capacity_sat;
        double avg_contribution_ratio;  // local/total
    };

    DualFundedStats get_stats() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Set minimum contribution per party
     */
    void set_min_contribution(uint64_t min_contribution_sat);

    /**
     * Set default fee rate
     */
    void set_default_fee_rate(double sat_per_vbyte);

private:
    std::map<Hash256, DualFundedChannel> channels_;

    uint64_t min_contribution_;
    double default_fee_rate_;
    uint32_t current_height_;

    mutable std::mutex mutex_;

    Hash256 generate_channel_id() const;
    Transaction create_dual_funding_tx(const DualFundedChannel& channel) const;
    bool validate_contribution(const FundingContribution& contribution) const;
    bool validate_fee_proposal(const DualFundedChannel& channel, const FeeProposal& proposal) const;
};

} // namespace dual_funded
} // namespace intcoin

#endif // INTCOIN_DUAL_FUNDED_H
