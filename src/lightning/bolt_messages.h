// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT Message Structures - Full BOLT Specification Implementation

#ifndef INTCOIN_BOLT_MESSAGES_H
#define INTCOIN_BOLT_MESSAGES_H

#include "intcoin/types.h"
#include "intcoin/crypto.h"
#include <vector>
#include <string>
#include <map>
#include <optional>

namespace intcoin {
namespace bolt {

// ============================================================================
// BOLT #1: Base Protocol - Message Framing
// ============================================================================

// TLV (Type-Length-Value) encoding
struct TLVRecord {
    uint64_t type;
    std::vector<uint8_t> value;

    std::vector<uint8_t> Serialize() const;
    static Result<TLVRecord> Deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

// BOLT message header
struct MessageHeader {
    uint16_t type;
    uint16_t length;

    static constexpr size_t SIZE = 4;  // 2 bytes type + 2 bytes length

    std::vector<uint8_t> Serialize() const;
    static Result<MessageHeader> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #1: Init message
struct InitMessage {
    uint16_t global_features;
    uint16_t local_features;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 16;

    std::vector<uint8_t> Serialize() const;
    static Result<InitMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #1: Error message
struct ErrorMessage {
    uint256 channel_id;  // All zeros = general error
    std::string data;    // Error description

    static constexpr uint16_t TYPE = 17;

    std::vector<uint8_t> Serialize() const;
    static Result<ErrorMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #1: Ping message
struct PingMessage {
    uint16_t num_pong_bytes;
    std::vector<uint8_t> ignored;

    static constexpr uint16_t TYPE = 18;

    std::vector<uint8_t> Serialize() const;
    static Result<PingMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #1: Pong message
struct PongMessage {
    std::vector<uint8_t> ignored;

    static constexpr uint16_t TYPE = 19;

    std::vector<uint8_t> Serialize() const;
    static Result<PongMessage> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #2: Peer Protocol for Channel Management
// ============================================================================

// BOLT #2: open_channel message
struct OpenChannelMessage {
    uint256 chain_hash;
    uint256 temporary_channel_id;
    uint64_t funding_satoshis;
    uint64_t push_msat;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint64_t htlc_minimum_msat;
    uint32_t feerate_per_kw;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;
    PublicKey first_per_commitment_point;
    uint8_t channel_flags;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 32;

    std::vector<uint8_t> Serialize() const;
    static Result<OpenChannelMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: accept_channel message
struct AcceptChannelMessage {
    uint256 temporary_channel_id;
    uint64_t dust_limit_satoshis;
    uint64_t max_htlc_value_in_flight_msat;
    uint64_t channel_reserve_satoshis;
    uint64_t htlc_minimum_msat;
    uint32_t minimum_depth;
    uint16_t to_self_delay;
    uint16_t max_accepted_htlcs;
    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;
    PublicKey first_per_commitment_point;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 33;

    std::vector<uint8_t> Serialize() const;
    static Result<AcceptChannelMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: funding_created message
struct FundingCreatedMessage {
    uint256 temporary_channel_id;
    uint256 funding_txid;
    uint16_t funding_output_index;
    Signature signature;

    static constexpr uint16_t TYPE = 34;

    std::vector<uint8_t> Serialize() const;
    static Result<FundingCreatedMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: funding_signed message
struct FundingSignedMessage {
    uint256 channel_id;
    Signature signature;

    static constexpr uint16_t TYPE = 35;

    std::vector<uint8_t> Serialize() const;
    static Result<FundingSignedMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: funding_locked message (channel_ready in newer specs)
struct FundingLockedMessage {
    uint256 channel_id;
    PublicKey next_per_commitment_point;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 36;

    std::vector<uint8_t> Serialize() const;
    static Result<FundingLockedMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: shutdown message
struct ShutdownMessage {
    uint256 channel_id;
    std::vector<uint8_t> scriptpubkey;

    static constexpr uint16_t TYPE = 38;

    std::vector<uint8_t> Serialize() const;
    static Result<ShutdownMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: closing_signed message
struct ClosingSignedMessage {
    uint256 channel_id;
    uint64_t fee_satoshis;
    Signature signature;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 39;

    std::vector<uint8_t> Serialize() const;
    static Result<ClosingSignedMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: update_add_htlc message
struct UpdateAddHTLCMessage {
    uint256 channel_id;
    uint64_t id;
    uint64_t amount_msat;
    uint256 payment_hash;
    uint32_t cltv_expiry;
    std::vector<uint8_t> onion_routing_packet;

    static constexpr uint16_t TYPE = 128;

    std::vector<uint8_t> Serialize() const;
    static Result<UpdateAddHTLCMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: update_fulfill_htlc message
struct UpdateFulfillHTLCMessage {
    uint256 channel_id;
    uint64_t id;
    uint256 payment_preimage;

    static constexpr uint16_t TYPE = 130;

    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFulfillHTLCMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: update_fail_htlc message
struct UpdateFailHTLCMessage {
    uint256 channel_id;
    uint64_t id;
    std::vector<uint8_t> reason;

    static constexpr uint16_t TYPE = 131;

    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFailHTLCMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: commitment_signed message
struct CommitmentSignedMessage {
    uint256 channel_id;
    Signature signature;
    std::vector<Signature> htlc_signatures;

    static constexpr uint16_t TYPE = 132;

    std::vector<uint8_t> Serialize() const;
    static Result<CommitmentSignedMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: revoke_and_ack message
struct RevokeAndAckMessage {
    uint256 channel_id;
    uint256 per_commitment_secret;
    PublicKey next_per_commitment_point;

    static constexpr uint16_t TYPE = 133;

    std::vector<uint8_t> Serialize() const;
    static Result<RevokeAndAckMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: update_fee message
struct UpdateFeeMessage {
    uint256 channel_id;
    uint32_t feerate_per_kw;

    static constexpr uint16_t TYPE = 134;

    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFeeMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: update_fail_malformed_htlc message
struct UpdateFailMalformedHTLCMessage {
    uint256 channel_id;
    uint64_t id;
    uint256 sha256_of_onion;
    uint16_t failure_code;

    static constexpr uint16_t TYPE = 135;

    std::vector<uint8_t> Serialize() const;
    static Result<UpdateFailMalformedHTLCMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #2: channel_reestablish message
struct ChannelReestablishMessage {
    uint256 channel_id;
    uint64_t next_commitment_number;
    uint64_t next_revocation_number;
    uint256 your_last_per_commitment_secret;
    PublicKey my_current_per_commitment_point;

    static constexpr uint16_t TYPE = 136;

    std::vector<uint8_t> Serialize() const;
    static Result<ChannelReestablishMessage> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #7: P2P Node and Channel Discovery
// ============================================================================

// BOLT #7: announcement_signatures message
struct AnnouncementSignaturesMessage {
    uint256 channel_id;
    uint64_t short_channel_id;
    Signature node_signature;
    Signature bitcoin_signature;

    static constexpr uint16_t TYPE = 259;

    std::vector<uint8_t> Serialize() const;
    static Result<AnnouncementSignaturesMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: channel_announcement message
struct ChannelAnnouncementMessage {
    Signature node_signature_1;
    Signature node_signature_2;
    Signature bitcoin_signature_1;
    Signature bitcoin_signature_2;
    std::vector<uint8_t> features;
    uint256 chain_hash;
    uint64_t short_channel_id;
    PublicKey node_id_1;
    PublicKey node_id_2;
    PublicKey bitcoin_key_1;
    PublicKey bitcoin_key_2;

    static constexpr uint16_t TYPE = 256;

    std::vector<uint8_t> Serialize() const;
    static Result<ChannelAnnouncementMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: node_announcement message
struct NodeAnnouncementMessage {
    Signature signature;
    std::vector<uint8_t> features;
    uint32_t timestamp;
    PublicKey node_id;
    uint8_t rgb_color[3];
    std::string alias;  // 32 bytes
    std::vector<std::vector<uint8_t>> addresses;

    static constexpr uint16_t TYPE = 257;

    std::vector<uint8_t> Serialize() const;
    static Result<NodeAnnouncementMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: channel_update message
struct ChannelUpdateMessage {
    Signature signature;
    uint256 chain_hash;
    uint64_t short_channel_id;
    uint32_t timestamp;
    uint8_t message_flags;
    uint8_t channel_flags;
    uint16_t cltv_expiry_delta;
    uint64_t htlc_minimum_msat;
    uint32_t fee_base_msat;
    uint32_t fee_proportional_millionths;
    std::optional<uint64_t> htlc_maximum_msat;

    static constexpr uint16_t TYPE = 258;

    std::vector<uint8_t> Serialize() const;
    static Result<ChannelUpdateMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: query_short_channel_ids message
struct QueryShortChannelIdsMessage {
    uint256 chain_hash;
    std::vector<uint64_t> short_channel_ids;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 261;

    std::vector<uint8_t> Serialize() const;
    static Result<QueryShortChannelIdsMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: reply_short_channel_ids_end message
struct ReplyShortChannelIdsEndMessage {
    uint256 chain_hash;
    uint8_t complete;

    static constexpr uint16_t TYPE = 262;

    std::vector<uint8_t> Serialize() const;
    static Result<ReplyShortChannelIdsEndMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: query_channel_range message
struct QueryChannelRangeMessage {
    uint256 chain_hash;
    uint32_t first_blocknum;
    uint32_t number_of_blocks;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 263;

    std::vector<uint8_t> Serialize() const;
    static Result<QueryChannelRangeMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: reply_channel_range message
struct ReplyChannelRangeMessage {
    uint256 chain_hash;
    uint32_t first_blocknum;
    uint32_t number_of_blocks;
    uint8_t complete;
    std::vector<uint64_t> short_channel_ids;
    std::map<uint64_t, std::vector<uint8_t>> tlv_records;

    static constexpr uint16_t TYPE = 264;

    std::vector<uint8_t> Serialize() const;
    static Result<ReplyChannelRangeMessage> Deserialize(const std::vector<uint8_t>& data);
};

// BOLT #7: gossip_timestamp_filter message
struct GossipTimestampFilterMessage {
    uint256 chain_hash;
    uint32_t first_timestamp;
    uint32_t timestamp_range;

    static constexpr uint16_t TYPE = 265;

    std::vector<uint8_t> Serialize() const;
    static Result<GossipTimestampFilterMessage> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// BOLT #9: Feature Flags
// ============================================================================

enum class FeatureBit : uint16_t {
    OPTION_DATA_LOSS_PROTECT = 0,           // Required
    INITIAL_ROUTING_SYNC = 3,               // Optional
    OPTION_UPFRONT_SHUTDOWN_SCRIPT = 4,     // Required/Optional
    GOSSIP_QUERIES = 6,                      // Required/Optional
    VAR_ONION_OPTIN = 8,                    // Required
    GOSSIP_QUERIES_EX = 10,                  // Optional
    OPTION_STATIC_REMOTEKEY = 12,           // Required/Optional
    PAYMENT_SECRET = 14,                     // Required
    BASIC_MPP = 16,                          // Optional
    OPTION_SUPPORT_LARGE_CHANNEL = 18,      // Optional
    OPTION_ANCHOR_OUTPUTS = 20,              // Required/Optional
    OPTION_ANCHORS_ZERO_FEE_HTLC_TX = 22,   // Required/Optional
    OPTION_ROUTE_BLINDING = 24,              // Optional
    OPTION_SHUTDOWN_ANYSEGWIT = 26,         // Optional
    OPTION_CHANNEL_TYPE = 44,                // Required/Optional
    OPTION_SCID_ALIAS = 46,                  // Optional
    OPTION_PAYMENT_METADATA = 48,            // Optional
    OPTION_ZEROCONF = 50,                    // Optional
};

class FeatureFlags {
public:
    FeatureFlags();

    void SetFeature(FeatureBit bit, bool required = false);
    bool HasFeature(FeatureBit bit) const;
    bool IsRequired(FeatureBit bit) const;
    bool IsCompatible(const FeatureFlags& other) const;

    std::vector<uint8_t> Serialize() const;
    static Result<FeatureFlags> Deserialize(const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> features_;  // Bit field
};

} // namespace bolt
} // namespace intcoin

#endif // INTCOIN_BOLT_MESSAGES_H
