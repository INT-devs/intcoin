// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network implementation for quantum-resistant payment channels.
// Adapted for Dilithium5 signatures and SHA3-256 hashing.

#ifndef INTCOIN_LIGHTNING_H
#define INTCOIN_LIGHTNING_H

#include "primitives.h"
#include "transaction.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <map>

namespace intcoin {
namespace lightning {

/**
 * Lightning Network protocol version
 */
static constexpr uint32_t LIGHTNING_VERSION = 1;

/**
 * Channel states
 */
enum class ChannelState {
    OPENING,        // Channel opening in progress
    OPEN,           // Channel is open and operational
    CLOSING,        // Cooperative close initiated
    FORCE_CLOSING,  // Unilateral close initiated
    CLOSED,         // Channel fully closed
    ERROR           // Channel in error state
};

/**
 * HTLC (Hash Time-Locked Contract) direction
 */
enum class HTLCDirection {
    OFFERED,   // We offered this HTLC
    RECEIVED   // We received this HTLC
};

/**
 * Hash Time-Locked Contract
 * Used for routing payments across multiple channels
 */
struct HTLC {
    uint64_t id;                        // HTLC identifier
    uint64_t amount_sat;                // Amount in satoshis
    Hash256 payment_hash;               // SHA3-256 hash of preimage
    uint32_t cltv_expiry;               // Absolute block height for timeout
    HTLCDirection direction;            // Offered or received
    std::vector<uint8_t> onion_routing; // Encrypted routing information

    HTLC() : id(0), amount_sat(0), payment_hash{}, cltv_expiry(0),
             direction(HTLCDirection::OFFERED) {}

    std::vector<uint8_t> serialize() const;
    static HTLC deserialize(const std::vector<uint8_t>& data);
};

/**
 * Channel commitment transaction
 * Represents the current state of the channel
 */
struct CommitmentTransaction {
    uint64_t commitment_number;         // Monotonically increasing
    uint64_t to_local_sat;              // Balance for local node
    uint64_t to_remote_sat;             // Balance for remote node
    std::vector<HTLC> htlcs;            // Active HTLCs

    Transaction funding_tx;             // Original funding transaction
    Transaction commitment_tx;          // Current commitment transaction
    DilithiumSignature local_sig;   // Local signature
    DilithiumSignature remote_sig;  // Remote signature

    // Revocation keys for penalty mechanism
    DilithiumPubKey revocation_pubkey;
    Hash256 revocation_hash;

    CommitmentTransaction() : commitment_number(0), to_local_sat(0),
                             to_remote_sat(0), revocation_hash{} {}

    Hash256 get_hash() const;
    std::vector<uint8_t> serialize() const;
    static CommitmentTransaction deserialize(const std::vector<uint8_t>& data);
};

/**
 * Lightning payment channel
 * Represents a bidirectional payment channel between two nodes
 */
class Channel {
public:
    // Channel identification
    Hash256 channel_id;                 // Unique channel identifier
    Hash256 funding_txid;               // Funding transaction ID
    uint32_t funding_output_index;      // Output index in funding tx

    // Channel participants
    DilithiumPubKey local_pubkey;
    DilithiumPubKey remote_pubkey;

    // Channel parameters
    uint64_t capacity_sat;              // Total channel capacity
    uint64_t local_balance_sat;         // Our balance
    uint64_t remote_balance_sat;        // Remote balance
    uint32_t dust_limit_sat;            // Minimum HTLC amount
    uint32_t max_htlc_value_in_flight_sat; // Max total HTLC value
    uint16_t max_accepted_htlcs;        // Max concurrent HTLCs

    // Channel state
    ChannelState state;
    uint64_t commitment_number;         // Current commitment number

    // Commitment transactions
    std::shared_ptr<CommitmentTransaction> latest_commitment;
    std::vector<std::shared_ptr<CommitmentTransaction>> revoked_commitments;

    // Active HTLCs
    std::map<uint64_t, HTLC> pending_htlcs;
    uint64_t next_htlc_id;

    // Timeouts and security
    uint32_t to_self_delay;             // CSV delay for our outputs
    uint32_t channel_reserve_sat;       // Minimum balance to maintain

    Channel();

    // Channel lifecycle
    bool open(const DilithiumPubKey& remote_key, uint64_t capacity);
    bool close_cooperative();
    bool close_unilateral();
    bool is_open() const { return state == ChannelState::OPEN; }

    // Balance management
    bool can_send(uint64_t amount_sat) const;
    bool can_receive(uint64_t amount_sat) const;
    uint64_t available_to_send() const;
    uint64_t available_to_receive() const;

    // HTLC operations
    bool add_htlc(uint64_t amount_sat, const Hash256& payment_hash,
                  uint32_t cltv_expiry, const std::vector<uint8_t>& onion);
    bool settle_htlc(uint64_t htlc_id, const std::vector<uint8_t>& preimage);
    bool fail_htlc(uint64_t htlc_id);

    // Commitment operations
    bool create_new_commitment();
    bool sign_commitment(const crypto::DilithiumKeyPair& keypair);
    bool verify_remote_signature(const DilithiumSignature& sig);
    bool revoke_previous_commitment();

    // Serialization
    std::vector<uint8_t> serialize() const;
    static Channel deserialize(const std::vector<uint8_t>& data);
};

/**
 * Lightning Network node
 * Manages multiple channels and routes payments
 */
class LightningNode {
public:
    LightningNode(const crypto::DilithiumKeyPair& keypair);

    // Node identity
    DilithiumPubKey get_node_id() const { return keypair_.public_key; }

    // Channel management
    std::optional<Hash256> open_channel(const DilithiumPubKey& remote_pubkey,
                                       uint64_t capacity_sat,
                                       uint64_t push_amount_sat = 0);
    bool close_channel(const Hash256& channel_id, bool force = false);
    std::shared_ptr<Channel> get_channel(const Hash256& channel_id);
    std::vector<std::shared_ptr<Channel>> get_all_channels() const;
    size_t active_channel_count() const;

    // Payment operations
    bool send_payment(uint64_t amount_sat, const Hash256& payment_hash,
                     const std::vector<DilithiumPubKey>& route);
    bool receive_payment(uint64_t amount_sat, std::string& invoice_out);
    bool forward_htlc(const Hash256& incoming_channel, const Hash256& outgoing_channel,
                     uint64_t htlc_id);

    // Invoice management
    struct Invoice {
        Hash256 payment_hash;
        std::vector<uint8_t> preimage;
        uint64_t amount_sat;
        std::string description;
        uint32_t expiry_time;
        std::string encoded_invoice;

        Invoice() : payment_hash{}, amount_sat(0), expiry_time(0) {}
    };

    Invoice create_invoice(uint64_t amount_sat, const std::string& description);
    bool pay_invoice(const std::string& encoded_invoice);

    // Network graph and routing
    struct ChannelInfo {
        Hash256 channel_id;
        DilithiumPubKey node1;
        DilithiumPubKey node2;
        uint64_t capacity_sat;
        uint32_t fee_base_msat;
        uint32_t fee_rate_ppm;
        uint32_t cltv_expiry_delta;
        bool enabled;
    };

    void add_channel_to_graph(const ChannelInfo& info);
    void remove_channel_from_graph(const Hash256& channel_id);
    std::vector<DilithiumPubKey> find_route(
        const DilithiumPubKey& destination,
        uint64_t amount_sat);

    // Statistics
    struct NodeStats {
        size_t total_channels;
        size_t active_channels;
        uint64_t total_capacity_sat;
        uint64_t total_local_balance_sat;
        uint64_t total_remote_balance_sat;
        size_t successful_payments;
        size_t failed_payments;
        uint64_t total_fees_earned_sat;
    };

    NodeStats get_stats() const;

private:
    crypto::DilithiumKeyPair keypair_;
    std::map<Hash256, std::shared_ptr<Channel>> channels_;
    std::map<Hash256, ChannelInfo> network_graph_;
    std::map<Hash256, Invoice> invoices_;

    // Statistics
    size_t successful_payments_;
    size_t failed_payments_;
    uint64_t total_fees_earned_sat_;

    // Helper functions
    Hash256 generate_channel_id(const Hash256& funding_txid, uint32_t output_index);
    bool validate_route(const std::vector<DilithiumPubKey>& route,
                       uint64_t amount_sat);
    uint64_t calculate_route_fees(const std::vector<DilithiumPubKey>& route,
                                  uint64_t amount_sat);
};

/**
 * Lightning Network protocol messages
 */
namespace messages {

enum class MessageType : uint16_t {
    // Channel establishment
    OPEN_CHANNEL = 32,
    ACCEPT_CHANNEL = 33,
    FUNDING_CREATED = 34,
    FUNDING_SIGNED = 35,
    FUNDING_LOCKED = 36,

    // Channel operation
    UPDATE_ADD_HTLC = 128,
    UPDATE_FULFILL_HTLC = 130,
    UPDATE_FAIL_HTLC = 131,
    COMMITMENT_SIGNED = 132,
    REVOKE_AND_ACK = 133,
    UPDATE_FEE = 134,

    // Channel closing
    SHUTDOWN = 38,
    CLOSING_SIGNED = 39,

    // Gossip
    CHANNEL_ANNOUNCEMENT = 256,
    NODE_ANNOUNCEMENT = 257,
    CHANNEL_UPDATE = 258,

    // Error
    ERROR = 17
};

struct Message {
    MessageType type;
    std::vector<uint8_t> payload;

    Message() : type(MessageType::ERROR) {}
    Message(MessageType t, const std::vector<uint8_t>& p) : type(t), payload(p) {}

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);
};

// Channel establishment messages
struct OpenChannel {
    Hash256 chain_hash;
    Hash256 temporary_channel_id;
    uint64_t funding_satoshis;
    uint64_t push_msat;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint32_t htlc_minimum_msat;
    uint32_t feerate_per_kw;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    DilithiumPubKey funding_pubkey;
    DilithiumPubKey revocation_basepoint;
    DilithiumPubKey payment_basepoint;
    DilithiumPubKey delayed_payment_basepoint;
    DilithiumPubKey htlc_basepoint;
    DilithiumPubKey first_per_commitment_point;

    Message to_message() const;
    static OpenChannel from_message(const Message& msg);
};

struct AcceptChannel {
    Hash256 temporary_channel_id;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint32_t htlc_minimum_msat;
    uint32_t minimum_depth;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    DilithiumPubKey funding_pubkey;
    DilithiumPubKey revocation_basepoint;
    DilithiumPubKey payment_basepoint;
    DilithiumPubKey delayed_payment_basepoint;
    DilithiumPubKey htlc_basepoint;
    DilithiumPubKey first_per_commitment_point;

    Message to_message() const;
    static AcceptChannel from_message(const Message& msg);
};

} // namespace messages

} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_H
