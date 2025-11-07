// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_CHANNEL_H
#define INTCOIN_LIGHTNING_CHANNEL_H

#include "../primitives.h"
#include "../transaction.h"
#include "../crypto/crypto.h"
#include <vector>
#include <memory>
#include <chrono>

namespace intcoin {
namespace lightning {

/**
 * Channel state
 */
enum class ChannelState {
    OPENING,              // Channel is being opened
    OPEN,                 // Channel is open and active
    CLOSING_MUTUAL,       // Cooperative close in progress
    CLOSING_UNILATERAL,   // Force close in progress
    CLOSED                // Channel is closed
};

/**
 * Channel direction
 */
enum class ChannelDirection {
    OUTBOUND,  // We initiated the channel
    INBOUND    // Peer initiated the channel
};

/**
 * Hash Time Locked Contract (HTLC)
 *
 * Enables conditional payments that can be claimed with a preimage
 * or refunded after a timeout.
 */
struct HTLC {
    Hash256 payment_hash;      // Hash of payment preimage
    uint64_t amount;           // Amount in satoshis
    uint32_t cltv_expiry;      // Absolute block height for timeout
    bool incoming;             // True if receiving, false if sending
    Hash256 preimage;          // Payment preimage (if known)
    bool settled;              // True if claimed/refunded

    HTLC() : amount(0), cltv_expiry(0), incoming(false), settled(false) {}

    // Verify preimage matches hash
    bool verify_preimage(const Hash256& img) const;
};

/**
 * Commitment transaction
 *
 * Represents the current state of the channel balance.
 * Each update creates a new commitment transaction.
 */
struct CommitmentTransaction {
    uint64_t commitment_number;     // Monotonically increasing
    uint64_t local_balance;         // Our balance
    uint64_t remote_balance;        // Peer's balance
    std::vector<HTLC> htlcs;        // Pending HTLCs
    Transaction tx;                 // Actual Bitcoin transaction
    Hash256 revocation_hash;        // For revocation
    uint64_t fee;                   // Transaction fee

    CommitmentTransaction() : commitment_number(0), local_balance(0),
                             remote_balance(0), fee(0) {}

    // Calculate total balance
    uint64_t total_balance() const {
        return local_balance + remote_balance;
    }

    // Verify transaction is valid
    bool verify() const;
};

/**
 * Payment channel between two Lightning nodes
 *
 * Enables instant, low-cost off-chain transactions between peers.
 * Channels are bidirectional and can route payments through the network.
 */
class PaymentChannel {
public:
    PaymentChannel(const PublicKey& local_key, const PublicKey& remote_key,
                   uint64_t capacity, ChannelDirection direction);
    ~PaymentChannel();

    // Channel lifecycle
    bool open(const Transaction& funding_tx, uint32_t output_index);
    bool close_mutual();
    bool close_unilateral();
    bool is_open() const { return state_ == ChannelState::OPEN; }

    // Channel state
    ChannelState get_state() const { return state_; }
    Hash256 get_channel_id() const { return channel_id_; }
    uint64_t get_capacity() const { return capacity_; }
    uint64_t get_local_balance() const;
    uint64_t get_remote_balance() const;
    uint64_t get_available_balance() const;

    // Commitment management
    CommitmentTransaction get_current_commitment() const;
    bool update_commitment(const CommitmentTransaction& new_commitment);
    void revoke_old_commitment(uint64_t commitment_number);

    // Payment operations
    bool add_htlc(const HTLC& htlc);
    bool settle_htlc(const Hash256& payment_hash, const Hash256& preimage);
    bool fail_htlc(const Hash256& payment_hash);
    std::vector<HTLC> get_pending_htlcs() const;

    // Channel info
    PublicKey get_local_key() const { return local_key_; }
    PublicKey get_remote_key() const { return remote_key_; }
    ChannelDirection get_direction() const { return direction_; }
    uint64_t get_update_count() const { return update_count_; }

    // Fees
    void set_fee_rate(uint64_t fee_rate) { fee_rate_ = fee_rate; }
    uint64_t calculate_fee() const;

    // Channel monitoring
    bool is_expired(uint32_t current_height) const;
    std::chrono::system_clock::time_point get_last_update() const {
        return last_update_;
    }

private:
    Hash256 channel_id_;
    PublicKey local_key_;
    PublicKey remote_key_;
    uint64_t capacity_;
    ChannelDirection direction_;
    ChannelState state_;

    // Commitment state
    CommitmentTransaction current_commitment_;
    std::vector<CommitmentTransaction> old_commitments_;
    uint64_t update_count_;

    // Channel parameters
    uint64_t fee_rate_;              // Satoshis per byte
    uint32_t cltv_expiry_delta_;     // Blocks before HTLC expires
    uint64_t htlc_minimum_;          // Minimum HTLC amount
    uint64_t reserve_amount_;        // Channel reserve

    // Funding
    Transaction funding_tx_;
    uint32_t funding_output_index_;

    // Timing
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point last_update_;

    // Helper methods
    bool validate_htlc(const HTLC& htlc) const;
    CommitmentTransaction create_commitment(uint64_t local_bal,
                                           uint64_t remote_bal,
                                           const std::vector<HTLC>& htlcs);
    Hash256 calculate_channel_id() const;
};

/**
 * Channel manager
 *
 * Manages multiple payment channels for a Lightning node.
 */
class ChannelManager {
public:
    ChannelManager();
    ~ChannelManager();

    // Channel operations
    std::shared_ptr<PaymentChannel> open_channel(const PublicKey& peer,
                                                  uint64_t amount);
    bool close_channel(const Hash256& channel_id, bool force = false);
    std::shared_ptr<PaymentChannel> get_channel(const Hash256& channel_id);

    // Channel queries
    std::vector<std::shared_ptr<PaymentChannel>> get_all_channels() const;
    std::vector<std::shared_ptr<PaymentChannel>> get_open_channels() const;
    std::vector<std::shared_ptr<PaymentChannel>> get_channels_with_peer(
        const PublicKey& peer) const;

    // Statistics
    size_t get_channel_count() const;
    uint64_t get_total_capacity() const;
    uint64_t get_total_local_balance() const;
    uint64_t get_total_remote_balance() const;

    // Monitoring
    void monitor_channels(uint32_t current_height);
    std::vector<Hash256> get_expired_channels(uint32_t current_height) const;

private:
    std::unordered_map<Hash256, std::shared_ptr<PaymentChannel>> channels_;
    mutable std::mutex channels_mutex_;

    // Callbacks
    std::function<void(const Hash256&)> channel_opened_callback_;
    std::function<void(const Hash256&)> channel_closed_callback_;
};

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_CHANNEL_H
