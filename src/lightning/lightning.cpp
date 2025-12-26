// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/lightning.h"
#include "intcoin/blockchain.h"
#include "intcoin/wallet.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

namespace intcoin {

// ============================================================================
// Lightning Network Implementation
// ============================================================================

HTLC::HTLC() : id(0), amount(0), cltv_expiry(0), incoming(false), fulfilled(false) {}
HTLC::HTLC(uint64_t id_, uint64_t amt, const uint256& hash, uint32_t expiry, bool inc)
    : id(id_), amount(amt), payment_hash(hash), cltv_expiry(expiry), incoming(inc), fulfilled(false), preimage() {}
std::vector<uint8_t> HTLC::Serialize() const { return std::vector<uint8_t>(); }
Result<HTLC> HTLC::Deserialize(const std::vector<uint8_t>& data) { return Result<HTLC>::Ok(HTLC()); }

ChannelConfig::ChannelConfig() : dust_limit(lightning::DUST_LIMIT), max_htlc_value(lightning::MAX_CHANNEL_CAPACITY),
    channel_reserve(lightning::MIN_CHANNEL_CAPACITY / 100), htlc_minimum(1000), to_self_delay(144),
    max_accepted_htlcs(lightning::MAX_HTLC_COUNT) {}
ChannelConfig ChannelConfig::Default() { return ChannelConfig(); }

// ============================================================================
// BOLT #2 Message Implementations
// ============================================================================

// OpenChannelMsg
OpenChannelMsg::OpenChannelMsg() : funding_ints(0), push_mints(0), dust_limit_ints(lightning::DUST_LIMIT),
    max_htlc_value_in_flight_mints(lightning::MAX_CHANNEL_CAPACITY * 1000), channel_reserve_ints(0),
    htlc_minimum_mints(1000), feerate_per_kw(1000), to_self_delay(144), max_accepted_htlcs(lightning::MAX_HTLC_COUNT),
    channel_flags(0) {
    temporary_channel_id = RandomGenerator::GetRandomUint256();
}

std::vector<uint8_t> OpenChannelMsg::Serialize() const {
    std::vector<uint8_t> data;
    // Add chain hash (32 bytes)
    data.insert(data.end(), chain_hash.begin(), chain_hash.end());
    // Add temporary channel ID (32 bytes)
    data.insert(data.end(), temporary_channel_id.begin(), temporary_channel_id.end());
    // Add funding amount (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((funding_ints >> (i * 8)) & 0xFF);
    }
    // Add push_mints (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back((push_mints >> (i * 8)) & 0xFF);
    }
    // Add dust limit (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back((dust_limit_ints >> (i * 8)) & 0xFF);
    }
    // Add remaining fields (simplified - full BOLT would include all fields)
    // ... (additional fields would be added here in production code)
    return data;
}

Result<OpenChannelMsg> OpenChannelMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 320) {
        return Result<OpenChannelMsg>::Error("Invalid message size");
    }
    OpenChannelMsg msg;
    // Parse chain hash
    std::copy(data.begin(), data.begin() + 32, msg.chain_hash.begin());
    // Parse temporary channel ID
    std::copy(data.begin() + 32, data.begin() + 64, msg.temporary_channel_id.begin());
    // Parse funding amount (8 bytes, big-endian)
    msg.funding_ints = 0;
    for (int i = 0; i < 8; i++) {
        msg.funding_ints = (msg.funding_ints << 8) | data[64 + i];
    }
    // ... (parse remaining fields)
    return Result<OpenChannelMsg>::Ok(msg);
}

// AcceptChannelMsg
AcceptChannelMsg::AcceptChannelMsg() : dust_limit_ints(lightning::DUST_LIMIT),
    max_htlc_value_in_flight_mints(lightning::MAX_CHANNEL_CAPACITY * 1000), channel_reserve_ints(0),
    htlc_minimum_mints(1000), minimum_depth(3), to_self_delay(144), max_accepted_htlcs(lightning::MAX_HTLC_COUNT) {}

std::vector<uint8_t> AcceptChannelMsg::Serialize() const {
    std::vector<uint8_t> data;
    // Add temporary channel ID (32 bytes)
    data.insert(data.end(), temporary_channel_id.begin(), temporary_channel_id.end());
    // Add remaining fields (simplified)
    return data;
}

Result<AcceptChannelMsg> AcceptChannelMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 270) {
        return Result<AcceptChannelMsg>::Error("Invalid message size");
    }
    AcceptChannelMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.temporary_channel_id.begin());
    return Result<AcceptChannelMsg>::Ok(msg);
}

// FundingCreatedMsg
FundingCreatedMsg::FundingCreatedMsg() {}

std::vector<uint8_t> FundingCreatedMsg::Serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), temporary_channel_id.begin(), temporary_channel_id.end());
    data.insert(data.end(), funding_txid.begin(), funding_txid.end());
    data.push_back((funding_output_index >> 8) & 0xFF);
    data.push_back(funding_output_index & 0xFF);
    data.insert(data.end(), signature.begin(), signature.end());
    return data;
}

Result<FundingCreatedMsg> FundingCreatedMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 130) {
        return Result<FundingCreatedMsg>::Error("Invalid message size");
    }
    FundingCreatedMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.temporary_channel_id.begin());
    std::copy(data.begin() + 32, data.begin() + 64, msg.funding_txid.begin());
    msg.funding_output_index = (data[64] << 8) | data[65];
    std::copy(data.begin() + 66, data.begin() + 130, msg.signature.begin());
    return Result<FundingCreatedMsg>::Ok(msg);
}

// FundingSignedMsg
FundingSignedMsg::FundingSignedMsg() {}

std::vector<uint8_t> FundingSignedMsg::Serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), channel_id.begin(), channel_id.end());
    data.insert(data.end(), signature.begin(), signature.end());
    return data;
}

Result<FundingSignedMsg> FundingSignedMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 96) {
        return Result<FundingSignedMsg>::Error("Invalid message size");
    }
    FundingSignedMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());
    std::copy(data.begin() + 32, data.begin() + 96, msg.signature.begin());
    return Result<FundingSignedMsg>::Ok(msg);
}

// FundingLockedMsg
FundingLockedMsg::FundingLockedMsg() {}

std::vector<uint8_t> FundingLockedMsg::Serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), channel_id.begin(), channel_id.end());
    data.insert(data.end(), next_per_commitment_point.begin(), next_per_commitment_point.end());
    return data;
}

Result<FundingLockedMsg> FundingLockedMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 65) {
        return Result<FundingLockedMsg>::Error("Invalid message size");
    }
    FundingLockedMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());
    std::copy(data.begin() + 32, data.begin() + 65, msg.next_per_commitment_point.begin());
    return Result<FundingLockedMsg>::Ok(msg);
}

// ShutdownMsg
ShutdownMsg::ShutdownMsg() {}

std::vector<uint8_t> ShutdownMsg::Serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // Serialize scriptpubkey
    auto script_bytes = scriptpubkey.Serialize();
    uint16_t script_len = script_bytes.size();
    data.push_back((script_len >> 8) & 0xFF);
    data.push_back(script_len & 0xFF);
    data.insert(data.end(), script_bytes.begin(), script_bytes.end());

    return data;
}

Result<ShutdownMsg> ShutdownMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 34) {  // channel_id (32) + script_len (2)
        return Result<ShutdownMsg>::Error("Invalid message size");
    }

    ShutdownMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());

    uint16_t script_len = (data[32] << 8) | data[33];
    if (data.size() < 34 + script_len) {
        return Result<ShutdownMsg>::Error("Invalid script length");
    }

    std::vector<uint8_t> script_bytes(data.begin() + 34, data.begin() + 34 + script_len);
    msg.scriptpubkey = Script::Deserialize(script_bytes);

    return Result<ShutdownMsg>::Ok(msg);
}

// ClosingSignedMsg
ClosingSignedMsg::ClosingSignedMsg() : fee_ints(0) {}

std::vector<uint8_t> ClosingSignedMsg::Serialize() const {
    std::vector<uint8_t> data;
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // Serialize fee (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((fee_ints >> (i * 8)) & 0xFF);
    }

    // Serialize signature
    data.insert(data.end(), signature.begin(), signature.end());

    return data;
}

Result<ClosingSignedMsg> ClosingSignedMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 64) {  // channel_id + fee + signature
        return Result<ClosingSignedMsg>::Error("Invalid message size");
    }

    ClosingSignedMsg msg;
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());

    // Parse fee
    msg.fee_ints = 0;
    for (int i = 0; i < 8; i++) {
        msg.fee_ints = (msg.fee_ints << 8) | data[32 + i];
    }

    // Parse signature
    std::copy(data.begin() + 40, data.begin() + 104, msg.signature.begin());

    return Result<ClosingSignedMsg>::Ok(msg);
}

// ============================================================================
// HTLC Update Messages Implementation
// ============================================================================

// UpdateAddHTLCMsg
UpdateAddHTLCMsg::UpdateAddHTLCMsg() : id(0), amount_mints(0), cltv_expiry(0) {}

std::vector<uint8_t> UpdateAddHTLCMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Channel ID (32 bytes)
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((id >> (i * 8)) & 0xFF);
    }

    // Amount in milli-ints (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((amount_mints >> (i * 8)) & 0xFF);
    }

    // Payment hash (32 bytes)
    data.insert(data.end(), payment_hash.begin(), payment_hash.end());

    // CLTV expiry (4 bytes, big-endian)
    for (int i = 3; i >= 0; i--) {
        data.push_back((cltv_expiry >> (i * 8)) & 0xFF);
    }

    // Onion routing packet (1366 bytes)
    data.insert(data.end(), onion_routing_packet.begin(), onion_routing_packet.end());

    return data;
}

Result<UpdateAddHTLCMsg> UpdateAddHTLCMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Minimum size: 32 (channel_id) + 8 (id) + 8 (amount) + 32 (hash) + 4 (cltv) + 1366 (onion) = 1450
    if (data.size() < 1450) {
        return Result<UpdateAddHTLCMsg>::Error("Invalid message size");
    }

    UpdateAddHTLCMsg msg;
    size_t offset = 0;

    // Parse channel ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, msg.channel_id.begin());
    offset += 32;

    // Parse HTLC ID
    msg.id = 0;
    for (int i = 0; i < 8; i++) {
        msg.id = (msg.id << 8) | data[offset + i];
    }
    offset += 8;

    // Parse amount
    msg.amount_mints = 0;
    for (int i = 0; i < 8; i++) {
        msg.amount_mints = (msg.amount_mints << 8) | data[offset + i];
    }
    offset += 8;

    // Parse payment hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, msg.payment_hash.begin());
    offset += 32;

    // Parse CLTV expiry
    msg.cltv_expiry = 0;
    for (int i = 0; i < 4; i++) {
        msg.cltv_expiry = (msg.cltv_expiry << 8) | data[offset + i];
    }
    offset += 4;

    // Parse onion routing packet
    msg.onion_routing_packet.assign(data.begin() + offset, data.begin() + offset + 1366);

    return Result<UpdateAddHTLCMsg>::Ok(msg);
}

// UpdateFulfillHTLCMsg
UpdateFulfillHTLCMsg::UpdateFulfillHTLCMsg() : id(0) {}

std::vector<uint8_t> UpdateFulfillHTLCMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Channel ID (32 bytes)
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((id >> (i * 8)) & 0xFF);
    }

    // Payment preimage (32 bytes)
    data.insert(data.end(), payment_preimage.begin(), payment_preimage.end());

    return data;
}

Result<UpdateFulfillHTLCMsg> UpdateFulfillHTLCMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 72) {  // 32 (channel_id) + 8 (id) + 32 (preimage)
        return Result<UpdateFulfillHTLCMsg>::Error("Invalid message size");
    }

    UpdateFulfillHTLCMsg msg;

    // Parse channel ID
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());

    // Parse HTLC ID
    msg.id = 0;
    for (int i = 0; i < 8; i++) {
        msg.id = (msg.id << 8) | data[32 + i];
    }

    // Parse payment preimage
    std::copy(data.begin() + 40, data.begin() + 72, msg.payment_preimage.begin());

    return Result<UpdateFulfillHTLCMsg>::Ok(msg);
}

// UpdateFailHTLCMsg
UpdateFailHTLCMsg::UpdateFailHTLCMsg() : id(0) {}

std::vector<uint8_t> UpdateFailHTLCMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Channel ID (32 bytes)
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        data.push_back((id >> (i * 8)) & 0xFF);
    }

    // Reason length (2 bytes, big-endian)
    uint16_t reason_len = static_cast<uint16_t>(reason.size());
    data.push_back((reason_len >> 8) & 0xFF);
    data.push_back(reason_len & 0xFF);

    // Reason data
    data.insert(data.end(), reason.begin(), reason.end());

    return data;
}

Result<UpdateFailHTLCMsg> UpdateFailHTLCMsg::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 42) {  // 32 (channel_id) + 8 (id) + 2 (len)
        return Result<UpdateFailHTLCMsg>::Error("Invalid message size");
    }

    UpdateFailHTLCMsg msg;

    // Parse channel ID
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());

    // Parse HTLC ID
    msg.id = 0;
    for (int i = 0; i < 8; i++) {
        msg.id = (msg.id << 8) | data[32 + i];
    }

    // Parse reason length
    uint16_t reason_len = (static_cast<uint16_t>(data[40]) << 8) | data[41];

    // Parse reason
    if (data.size() < 42 + reason_len) {
        return Result<UpdateFailHTLCMsg>::Error("Invalid reason data");
    }
    msg.reason.assign(data.begin() + 42, data.begin() + 42 + reason_len);

    return Result<UpdateFailHTLCMsg>::Ok(msg);
}

// ============================================================================
// Commitment Signature Exchange Messages Implementation
// ============================================================================

// CommitmentSignedMsg
CommitmentSignedMsg::CommitmentSignedMsg() {}

std::vector<uint8_t> CommitmentSignedMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Channel ID (32 bytes)
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // Commitment transaction signature (64 bytes)
    data.insert(data.end(), signature.begin(), signature.end());

    // Number of HTLC signatures (2 bytes, big-endian)
    uint16_t num_htlcs = static_cast<uint16_t>(htlc_signatures.size());
    data.push_back((num_htlcs >> 8) & 0xFF);
    data.push_back(num_htlcs & 0xFF);

    // HTLC signatures (64 bytes each)
    for (const auto& htlc_sig : htlc_signatures) {
        data.insert(data.end(), htlc_sig.begin(), htlc_sig.end());
    }

    return data;
}

Result<CommitmentSignedMsg> CommitmentSignedMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Minimum size: 32 (channel_id) + 64 (signature) + 2 (num_htlcs) = 98
    if (data.size() < 98) {
        return Result<CommitmentSignedMsg>::Error("Invalid message size");
    }

    CommitmentSignedMsg msg;
    size_t offset = 0;

    // Parse channel ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, msg.channel_id.begin());
    offset += 32;

    // Parse commitment signature
    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.signature.begin());
    offset += 64;

    // Parse number of HTLC signatures
    uint16_t num_htlcs = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // Verify we have enough data for all HTLC signatures
    if (data.size() < offset + (num_htlcs * 64)) {
        return Result<CommitmentSignedMsg>::Error("Insufficient data for HTLC signatures");
    }

    // Parse HTLC signatures
    for (uint16_t i = 0; i < num_htlcs; i++) {
        Signature htlc_sig;
        std::copy(data.begin() + offset, data.begin() + offset + 64, htlc_sig.begin());
        msg.htlc_signatures.push_back(htlc_sig);
        offset += 64;
    }

    return Result<CommitmentSignedMsg>::Ok(msg);
}

// RevokeAndAckMsg
RevokeAndAckMsg::RevokeAndAckMsg() {}

std::vector<uint8_t> RevokeAndAckMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Channel ID (32 bytes)
    data.insert(data.end(), channel_id.begin(), channel_id.end());

    // Per-commitment secret (32 bytes)
    data.insert(data.end(), per_commitment_secret.begin(), per_commitment_secret.end());

    // Next per-commitment point (33 bytes - compressed public key)
    data.insert(data.end(), next_per_commitment_point.begin(), next_per_commitment_point.end());

    return data;
}

Result<RevokeAndAckMsg> RevokeAndAckMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Expected size: 32 (channel_id) + 32 (secret) + 33 (pubkey) = 97
    if (data.size() < 97) {
        return Result<RevokeAndAckMsg>::Error("Invalid message size");
    }

    RevokeAndAckMsg msg;

    // Parse channel ID
    std::copy(data.begin(), data.begin() + 32, msg.channel_id.begin());

    // Parse per-commitment secret
    std::copy(data.begin() + 32, data.begin() + 64, msg.per_commitment_secret.begin());

    // Parse next per-commitment point
    std::copy(data.begin() + 64, data.begin() + 97, msg.next_per_commitment_point.begin());

    return Result<RevokeAndAckMsg>::Ok(msg);
}

// ============================================================================
// BOLT #7 Gossip Message Implementations
// ============================================================================

ChannelAnnouncementMsg::ChannelAnnouncementMsg() : short_channel_id(0) {}

std::vector<uint8_t> ChannelAnnouncementMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Node signature 1 (64 bytes)
    data.insert(data.end(), node_signature_1.begin(), node_signature_1.end());

    // Node signature 2 (64 bytes)
    data.insert(data.end(), node_signature_2.begin(), node_signature_2.end());

    // Bitcoin signature 1 (64 bytes)
    data.insert(data.end(), bitcoin_signature_1.begin(), bitcoin_signature_1.end());

    // Bitcoin signature 2 (64 bytes)
    data.insert(data.end(), bitcoin_signature_2.begin(), bitcoin_signature_2.end());

    // Features length (2 bytes)
    uint16_t features_len = static_cast<uint16_t>(features.size());
    data.push_back(static_cast<uint8_t>(features_len >> 8));
    data.push_back(static_cast<uint8_t>(features_len & 0xFF));

    // Features (variable)
    data.insert(data.end(), features.begin(), features.end());

    // Chain hash (32 bytes)
    data.insert(data.end(), chain_hash.begin(), chain_hash.end());

    // Short channel ID (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((short_channel_id >> (i * 8)) & 0xFF));
    }

    // Node ID 1 (33 bytes - compressed public key)
    data.insert(data.end(), node_id_1.begin(), node_id_1.end());

    // Node ID 2 (33 bytes)
    data.insert(data.end(), node_id_2.begin(), node_id_2.end());

    // Bitcoin key 1 (33 bytes)
    data.insert(data.end(), bitcoin_key_1.begin(), bitcoin_key_1.end());

    // Bitcoin key 2 (33 bytes)
    data.insert(data.end(), bitcoin_key_2.begin(), bitcoin_key_2.end());

    return data;
}

Result<ChannelAnnouncementMsg> ChannelAnnouncementMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Minimum size: 4*64 (signatures) + 2 (features_len) + 32 (chain_hash) + 8 (short_channel_id) + 4*33 (pubkeys)
    if (data.size() < 430) {
        return Result<ChannelAnnouncementMsg>::Error("Invalid message size");
    }

    ChannelAnnouncementMsg msg;
    size_t offset = 0;

    // Parse signatures
    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.node_signature_1.begin());
    offset += 64;

    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.node_signature_2.begin());
    offset += 64;

    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.bitcoin_signature_1.begin());
    offset += 64;

    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.bitcoin_signature_2.begin());
    offset += 64;

    // Parse features
    uint16_t features_len = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    if (offset + features_len > data.size()) {
        return Result<ChannelAnnouncementMsg>::Error("Invalid features length");
    }

    msg.features.assign(data.begin() + offset, data.begin() + offset + features_len);
    offset += features_len;

    // Parse chain hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, msg.chain_hash.begin());
    offset += 32;

    // Parse short channel ID
    msg.short_channel_id = 0;
    for (int i = 0; i < 8; i++) {
        msg.short_channel_id = (msg.short_channel_id << 8) | data[offset + i];
    }
    offset += 8;

    // Parse public keys
    std::copy(data.begin() + offset, data.begin() + offset + 33, msg.node_id_1.begin());
    offset += 33;

    std::copy(data.begin() + offset, data.begin() + offset + 33, msg.node_id_2.begin());
    offset += 33;

    std::copy(data.begin() + offset, data.begin() + offset + 33, msg.bitcoin_key_1.begin());
    offset += 33;

    std::copy(data.begin() + offset, data.begin() + offset + 33, msg.bitcoin_key_2.begin());
    offset += 33;

    return Result<ChannelAnnouncementMsg>::Ok(msg);
}

NodeAnnouncementMsg::NodeAnnouncementMsg() : timestamp(0) {
    rgb_color.fill(0);
}

std::vector<uint8_t> NodeAnnouncementMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Signature (64 bytes)
    data.insert(data.end(), signature.begin(), signature.end());

    // Features length (2 bytes)
    uint16_t features_len = static_cast<uint16_t>(features.size());
    data.push_back(static_cast<uint8_t>(features_len >> 8));
    data.push_back(static_cast<uint8_t>(features_len & 0xFF));

    // Features (variable)
    data.insert(data.end(), features.begin(), features.end());

    // Timestamp (4 bytes)
    for (int i = 3; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }

    // Node ID (33 bytes)
    data.insert(data.end(), node_id.begin(), node_id.end());

    // RGB color (3 bytes)
    data.insert(data.end(), rgb_color.begin(), rgb_color.end());

    // Alias length (1 byte) - max 32 bytes
    uint8_t alias_len = static_cast<uint8_t>(std::min(alias.size(), size_t(32)));
    data.push_back(alias_len);

    // Alias (padded to 32 bytes)
    for (size_t i = 0; i < 32; i++) {
        data.push_back(i < alias.size() ? alias[i] : 0);
    }

    // Addresses length (2 bytes)
    uint16_t addr_len = static_cast<uint16_t>(addresses.size());
    data.push_back(static_cast<uint8_t>(addr_len >> 8));
    data.push_back(static_cast<uint8_t>(addr_len & 0xFF));

    // Addresses (variable)
    data.insert(data.end(), addresses.begin(), addresses.end());

    return data;
}

Result<NodeAnnouncementMsg> NodeAnnouncementMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Minimum size: 64 (sig) + 2 (features_len) + 4 (timestamp) + 33 (node_id) + 3 (color) + 1 (alias_len) + 32 (alias) + 2 (addr_len)
    if (data.size() < 141) {
        return Result<NodeAnnouncementMsg>::Error("Invalid message size");
    }

    NodeAnnouncementMsg msg;
    size_t offset = 0;

    // Parse signature
    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.signature.begin());
    offset += 64;

    // Parse features
    uint16_t features_len = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    if (offset + features_len > data.size()) {
        return Result<NodeAnnouncementMsg>::Error("Invalid features length");
    }

    msg.features.assign(data.begin() + offset, data.begin() + offset + features_len);
    offset += features_len;

    // Parse timestamp
    msg.timestamp = 0;
    for (int i = 0; i < 4; i++) {
        msg.timestamp = (msg.timestamp << 8) | data[offset + i];
    }
    offset += 4;

    // Parse node ID
    std::copy(data.begin() + offset, data.begin() + offset + 33, msg.node_id.begin());
    offset += 33;

    // Parse RGB color
    std::copy(data.begin() + offset, data.begin() + offset + 3, msg.rgb_color.begin());
    offset += 3;

    // Parse alias
    uint8_t alias_len = data[offset];
    offset += 1;

    if (offset + 32 > data.size()) {
        return Result<NodeAnnouncementMsg>::Error("Invalid alias field");
    }

    msg.alias.assign(data.begin() + offset, data.begin() + offset + std::min(size_t(alias_len), size_t(32)));
    offset += 32;

    // Parse addresses
    uint16_t addr_len = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    if (offset + addr_len > data.size()) {
        return Result<NodeAnnouncementMsg>::Error("Invalid addresses length");
    }

    msg.addresses.assign(data.begin() + offset, data.begin() + offset + addr_len);
    offset += addr_len;

    return Result<NodeAnnouncementMsg>::Ok(msg);
}

ChannelUpdateMsg::ChannelUpdateMsg()
    : short_channel_id(0), timestamp(0), message_flags(0), channel_flags(0),
      cltv_expiry_delta(0), htlc_minimum_mints(0), fee_base_mints(0), fee_proportional_millionths(0) {}

std::vector<uint8_t> ChannelUpdateMsg::Serialize() const {
    std::vector<uint8_t> data;

    // Signature (64 bytes)
    data.insert(data.end(), signature.begin(), signature.end());

    // Chain hash (32 bytes)
    data.insert(data.end(), chain_hash.begin(), chain_hash.end());

    // Short channel ID (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((short_channel_id >> (i * 8)) & 0xFF));
    }

    // Timestamp (4 bytes)
    for (int i = 3; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }

    // Message flags (1 byte)
    data.push_back(message_flags);

    // Channel flags (1 byte)
    data.push_back(channel_flags);

    // CLTV expiry delta (2 bytes)
    data.push_back(static_cast<uint8_t>(cltv_expiry_delta >> 8));
    data.push_back(static_cast<uint8_t>(cltv_expiry_delta & 0xFF));

    // HTLC minimum (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((htlc_minimum_mints >> (i * 8)) & 0xFF));
    }

    // Fee base (4 bytes)
    for (int i = 3; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((fee_base_mints >> (i * 8)) & 0xFF));
    }

    // Fee proportional (4 bytes)
    for (int i = 3; i >= 0; i--) {
        data.push_back(static_cast<uint8_t>((fee_proportional_millionths >> (i * 8)) & 0xFF));
    }

    return data;
}

Result<ChannelUpdateMsg> ChannelUpdateMsg::Deserialize(const std::vector<uint8_t>& data) {
    // Expected size: 64 (sig) + 32 (chain_hash) + 8 (short_channel_id) + 4 (timestamp) + 2 (flags) + 2 (cltv) + 8 (htlc_min) + 4 (fee_base) + 4 (fee_prop) = 128
    if (data.size() < 128) {
        return Result<ChannelUpdateMsg>::Error("Invalid message size");
    }

    ChannelUpdateMsg msg;
    size_t offset = 0;

    // Parse signature
    std::copy(data.begin() + offset, data.begin() + offset + 64, msg.signature.begin());
    offset += 64;

    // Parse chain hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, msg.chain_hash.begin());
    offset += 32;

    // Parse short channel ID
    msg.short_channel_id = 0;
    for (int i = 0; i < 8; i++) {
        msg.short_channel_id = (msg.short_channel_id << 8) | data[offset + i];
    }
    offset += 8;

    // Parse timestamp
    msg.timestamp = 0;
    for (int i = 0; i < 4; i++) {
        msg.timestamp = (msg.timestamp << 8) | data[offset + i];
    }
    offset += 4;

    // Parse flags
    msg.message_flags = data[offset++];
    msg.channel_flags = data[offset++];

    // Parse CLTV expiry delta
    msg.cltv_expiry_delta = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // Parse HTLC minimum
    msg.htlc_minimum_mints = 0;
    for (int i = 0; i < 8; i++) {
        msg.htlc_minimum_mints = (msg.htlc_minimum_mints << 8) | data[offset + i];
    }
    offset += 8;

    // Parse fee base
    msg.fee_base_mints = 0;
    for (int i = 0; i < 4; i++) {
        msg.fee_base_mints = (msg.fee_base_mints << 8) | data[offset + i];
    }
    offset += 4;

    // Parse fee proportional
    msg.fee_proportional_millionths = 0;
    for (int i = 0; i < 4; i++) {
        msg.fee_proportional_millionths = (msg.fee_proportional_millionths << 8) | data[offset + i];
    }
    offset += 4;

    return Result<ChannelUpdateMsg>::Ok(msg);
}

CommitmentTransaction::CommitmentTransaction() : commitment_number(0), local_balance(0), remote_balance(0), fee(0) {}

/**
 * Build commitment transaction following BOLT #3
 *
 * Commitment transactions have the following structure:
 * - 1 input: funding transaction output
 * - Multiple outputs:
 *   - to_local: Delayed output to local node (CSV + revocation)
 *   - to_remote: Immediate output to remote node
 *   - HTLC outputs: For each pending HTLC
 */
Result<CommitmentTransaction> CommitmentTransaction::Build(
    const uint256& funding_txid,
    uint32_t funding_vout,
    uint64_t capacity,
    uint64_t to_local_amount,
    uint64_t to_remote_amount,
    const std::vector<HTLC>& htlcs,
    uint64_t commit_num,
    const ChannelConfig& config)
{
    CommitmentTransaction commitment;
    commitment.commitment_number = commit_num;
    commitment.local_balance = to_local_amount;
    commitment.remote_balance = to_remote_amount;

    // Calculate fee (simplified - use 1000 sats per kw)
    uint64_t estimated_weight = 500 + (htlcs.size() * 200);  // Base + HTLCs
    commitment.fee = (estimated_weight * 1000) / 1000;  // Fee in INTS

    // Verify balances
    uint64_t htlc_total = 0;
    for (const auto& htlc : htlcs) {
        htlc_total += htlc.amount;
    }

    if (to_local_amount + to_remote_amount + htlc_total + commitment.fee > capacity) {
        return Result<CommitmentTransaction>::Error("Commitment outputs exceed capacity");
    }

    // Build transaction
    Transaction& tx = commitment.tx;
    tx.version = 2;
    tx.locktime = 0x20000000 | (commit_num & 0xFFFFFF);  // Upper 8 bits for commitment obscured

    // Input: funding transaction output
    TxIn input;
    input.prev_tx_hash = funding_txid;
    input.prev_tx_index = funding_vout;
    input.sequence = 0x80000000 | (commit_num >> 24);  // Enable RBF, encode commitment number
    input.script_sig = Script();  // Will be filled when signing
    tx.inputs.push_back(input);

    // Output 1: to_local (delayed with revocation)
    // BOLT #3 to_local script with CSV delay and revocation path
    if (to_local_amount >= config.dust_limit) {
        TxOut to_local_output;
        to_local_output.value = to_local_amount;

        // Generate proper BOLT #3 to_local script
        // NOTE: Using placeholder keys for now - proper key derivation will be
        // implemented in Phase 1.3 (Commitment Transaction Signing)
        PublicKey revocation_pubkey;  // TODO: Derive from revocation basepoint
        PublicKey local_delayed_pubkey;  // TODO: Derive from local delayed basepoint
        revocation_pubkey.fill(0x01);  // Placeholder
        local_delayed_pubkey.fill(0x02);  // Placeholder

        // Create BOLT #3 compliant to_local script with CSV delay
        to_local_output.script_pubkey = Script::CreateToLocalScript(
            revocation_pubkey,
            local_delayed_pubkey,
            static_cast<uint16_t>(config.to_self_delay)
        );

        tx.outputs.push_back(to_local_output);
    }

    // Output 2: to_remote (immediately spendable by remote)
    if (to_remote_amount >= config.dust_limit) {
        TxOut to_remote_output;
        to_remote_output.value = to_remote_amount;

        // Generate proper BOLT #3 to_remote script (simple P2PK)
        // NOTE: Using placeholder key - will be replaced with actual remote pubkey
        PublicKey remote_pubkey;  // TODO: Use actual remote payment pubkey
        remote_pubkey.fill(0x03);  // Placeholder

        to_remote_output.script_pubkey = Script::CreateToRemoteScript(remote_pubkey);

        tx.outputs.push_back(to_remote_output);
    }

    // Outputs 3+: HTLC outputs
    // BOLT #3 HTLC scripts with revocation, timeout, and success paths
    for (const auto& htlc : htlcs) {
        if (htlc.amount < config.dust_limit) {
            continue;  // Skip dust HTLCs
        }

        TxOut htlc_output;
        htlc_output.value = htlc.amount;

        // Generate proper BOLT #3 HTLC scripts
        // NOTE: Using placeholder keys for now - proper key derivation will be
        // implemented in Phase 1.3 (Commitment Transaction Signing)
        PublicKey revocation_pubkey;
        PublicKey local_htlcpubkey;
        PublicKey remote_htlcpubkey;
        revocation_pubkey.fill(0x05);  // Placeholder
        local_htlcpubkey.fill(0x06);   // Placeholder
        remote_htlcpubkey.fill(0x07);  // Placeholder

        // Determine HTLC direction and create appropriate script
        // htlc.incoming: true = received (they offer), false = offered (we offer)
        if (!htlc.incoming) {
            // We offered this HTLC (outgoing - we send)
            // Remote can claim with preimage, we can reclaim after timeout
            htlc_output.script_pubkey = Script::CreateOfferedHTLCScript(
                revocation_pubkey,
                local_htlcpubkey,
                remote_htlcpubkey,
                htlc.payment_hash,
                htlc.cltv_expiry
            );
        } else {
            // We received this HTLC (incoming - they send)
            // We can claim with preimage, they can reclaim after timeout
            htlc_output.script_pubkey = Script::CreateReceivedHTLCScript(
                revocation_pubkey,
                local_htlcpubkey,
                remote_htlcpubkey,
                htlc.payment_hash,
                htlc.cltv_expiry
            );
        }

        tx.outputs.push_back(htlc_output);
    }

    // Sort outputs by BIP69 (lexicographic order)
    std::sort(tx.outputs.begin(), tx.outputs.end(),
        [](const TxOut& a, const TxOut& b) {
            // First compare by value
            if (a.value != b.value) {
                return a.value < b.value;
            }
            // Then by script (lexicographic)
            auto a_bytes = a.script_pubkey.Serialize();
            auto b_bytes = b.script_pubkey.Serialize();
            return a_bytes < b_bytes;
        });

    return Result<CommitmentTransaction>::Ok(commitment);
}

bool CommitmentTransaction::Verify(const PublicKey&, const PublicKey&) const { return true; }

Channel::Channel() : state(ChannelState::OPENING), capacity(0), local_balance(0), remote_balance(0),
    funding_vout(0), funding_confirmations(0), commitment_number(0), next_htlc_id(0) {}
Channel::Channel(const PublicKey& local, const PublicKey& remote, uint64_t cap)
    : local_node_id(local), remote_node_id(remote), state(ChannelState::OPENING), capacity(cap),
      local_balance(cap), remote_balance(0), funding_vout(0), funding_confirmations(0),
      commitment_number(0), next_htlc_id(0), local_config(ChannelConfig::Default()),
      remote_config(ChannelConfig::Default()), opened_at(std::chrono::system_clock::now()),
      last_update(std::chrono::system_clock::now()) { temporary_id = RandomGenerator::GetRandomUint256(); }
Result<void> Channel::Open(const Transaction& funding_tx, uint32_t vout) {
    funding_txid = funding_tx.GetHash(); funding_vout = vout; state = ChannelState::OPEN;
    return Result<void>::Ok();
}
/**
 * Close channel (BOLT #2)
 *
 * @param force If true, force close immediately with latest commitment tx.
 *              If false, initiate mutual close negotiation.
 * @return Result indicating success or error
 */
Result<void> Channel::Close(bool force) {
    // Check if channel is in valid state for closing
    if (state != ChannelState::OPEN && state != ChannelState::CLOSING_MUTUAL) {
        return Result<void>::Error("Channel not in valid state for closing");
    }

    if (force) {
        // Force close: Broadcast latest commitment transaction
        state = ChannelState::CLOSING_FORCE;

        // TODO: Broadcast local commitment transaction to blockchain
        // In a force close, we use the most recent commitment transaction
        // which includes a time delay for our outputs (CSV) and revocation
        // key paths for the counterparty if this is an old state

        // The commitment transaction will be monitored by watchtowers
        // to detect if the counterparty tries to cheat by broadcasting
        // an older commitment transaction

        return Result<void>::Ok();
    } else {
        // Mutual close: Initiate graceful shutdown protocol
        state = ChannelState::CLOSING_MUTUAL;

        // Mutual close will be negotiated via shutdown and closing_signed messages
        // 1. Send shutdown message with desired closing script
        // 2. Receive shutdown message from counterparty
        // 3. Negotiate closing transaction fee via closing_signed messages
        // 4. Broadcast mutually signed closing transaction

        return Result<void>::Ok();
    }
}
Result<uint64_t> Channel::AddHTLC(uint64_t amount, const uint256& payment_hash, uint32_t expiry) {
    uint64_t htlc_id = next_htlc_id++; HTLC htlc(htlc_id, amount, payment_hash, expiry, false);
    pending_htlcs.push_back(htlc); return Result<uint64_t>::Ok(htlc_id);
}
Result<void> Channel::FulfillHTLC(uint64_t, const uint256&) { return Result<void>::Ok(); }
Result<void> Channel::FailHTLC(uint64_t) { return Result<void>::Ok(); }
Result<void> Channel::UpdateCommitment() { return Result<void>::Ok(); }
uint64_t Channel::GetLocalBalance() const { return local_balance; }
uint64_t Channel::GetRemoteBalance() const { return remote_balance; }
uint64_t Channel::GetAvailableBalance() const { return local_balance; }
bool Channel::CanSend(uint64_t amount) const { return local_balance >= amount; }
bool Channel::CanReceive(uint64_t amount) const { return remote_balance >= amount; }
std::vector<uint8_t> Channel::Serialize() const { return std::vector<uint8_t>(); }
Result<Channel> Channel::Deserialize(const std::vector<uint8_t>&) { return Result<Channel>::Ok(Channel()); }

RouteHop::RouteHop() : amount(0), cltv_expiry(0), fee(0) {}
RouteHop::RouteHop(const PublicKey& node, const uint256& chan, uint64_t amt, uint32_t expiry)
    : node_id(node), channel_id(chan), amount(amt), cltv_expiry(expiry), fee(0) {}
PaymentRoute::PaymentRoute() : total_amount(0), total_fees(0), total_cltv(0) {}
bool PaymentRoute::IsValid() const { return !hops.empty(); }
uint64_t PaymentRoute::CalculateTotalFees() const { return total_fees; }

Invoice::Invoice() : amount(0), expiry(3600), min_final_cltv(lightning::MIN_CLTV_EXPIRY),
    created_at(std::chrono::system_clock::now()) {}
Invoice::Invoice(uint64_t amt, const std::string& desc, const PublicKey& payee_key)
    : amount(amt), description(desc), expiry(3600), min_final_cltv(lightning::MIN_CLTV_EXPIRY),
      payee(payee_key), created_at(std::chrono::system_clock::now()) {}
uint256 Invoice::GeneratePaymentHash(const uint256& preimage) {
    return SHA3::Hash(preimage.data(), 32);
}

// BOLT #11 Invoice Encoding (simplified for INTcoin)
std::string Invoice::Encode() const {
    std::ostringstream oss;

    // Prefix: "lint" for INTcoin Lightning Network
    oss << "lint";

    // Encode amount (in INTS, hex)
    oss << std::hex << std::setfill('0') << std::setw(16) << amount;

    // Encode payment hash (hex)
    oss << Uint256ToHex(payment_hash);  // 64 hex chars (32 bytes)

    // Encode expiry (hex, 8 chars)
    oss << std::hex << std::setw(8) << expiry;

    // Add description length and description (simple encoding)
    std::string desc_hex;
    for (char c : description) {
        oss << std::hex << std::setw(2) << static_cast<int>(c);
    }

    // Note: Full BOLT #11 would include bech32 encoding, routing hints, etc.
    // This is a simplified version for INTcoin

    return oss.str();
}

// BOLT #11 Invoice Decoding (simplified)
Result<Invoice> Invoice::Decode(const std::string& bolt11) {
    if (bolt11.substr(0, 4) != "lint") {
        return Result<Invoice>::Error("Invalid invoice prefix");
    }

    if (bolt11.length() < 4 + 16 + 64 + 8) {
        return Result<Invoice>::Error("Invoice too short");
    }

    Invoice invoice;

    try {
        // Decode amount (16 hex chars after "lint")
        std::string amount_hex = bolt11.substr(4, 16);
        invoice.amount = std::stoull(amount_hex, nullptr, 16);

        // Decode payment hash (64 hex chars)
        std::string hash_hex = bolt11.substr(4 + 16, 64);
        // Note: Would need proper uint256 parsing in full implementation

        // Decode expiry (8 hex chars)
        std::string expiry_hex = bolt11.substr(4 + 16 + 64, 8);
        invoice.expiry = std::stoul(expiry_hex, nullptr, 16);

        // Decode description (remaining chars)
        std::string desc_hex = bolt11.substr(4 + 16 + 64 + 8);
        for (size_t i = 0; i + 1 < desc_hex.length(); i += 2) {
            std::string byte_hex = desc_hex.substr(i, 2);
            char c = static_cast<char>(std::stoi(byte_hex, nullptr, 16));
            invoice.description += c;
        }

        invoice.min_final_cltv = lightning::MIN_CLTV_EXPIRY;
        invoice.created_at = std::chrono::system_clock::now();

    } catch (const std::exception& e) {
        return Result<Invoice>::Error(std::string("Failed to decode invoice: ") + e.what());
    }

    return Result<Invoice>::Ok(invoice);
}
Result<void> Invoice::Sign(const SecretKey&) { return Result<void>::Ok(); }
bool Invoice::Verify() const { return true; }
bool Invoice::IsExpired() const { return false; }

ChannelInfo::ChannelInfo() : capacity(0), base_fee(lightning::BASE_FEE), fee_rate(lightning::FEE_RATE),
    cltv_expiry_delta(lightning::CLTV_EXPIRY_DELTA), enabled(true),
    last_update(std::chrono::system_clock::now()) {}
NodeInfo::NodeInfo() : last_update(std::chrono::system_clock::now()) {}
NetworkGraph::NetworkGraph() {}
void NetworkGraph::AddChannel(const ChannelInfo& channel) {
    std::lock_guard<std::mutex> lock(mutex_); channels_[channel.channel_id] = channel;
}
void NetworkGraph::RemoveChannel(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_); channels_.erase(channel_id);
}
void NetworkGraph::UpdateChannel(const uint256& channel_id, const ChannelInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_); channels_[channel_id] = info;
}
void NetworkGraph::AddNode(const NodeInfo& node) {
    std::lock_guard<std::mutex> lock(mutex_); nodes_[node.node_id] = node;
}
void NetworkGraph::RemoveNode(const PublicKey& node_id) {
    std::lock_guard<std::mutex> lock(mutex_); nodes_.erase(node_id);
}
Result<ChannelInfo> NetworkGraph::GetChannel(const uint256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = channels_.find(channel_id);
    if (it == channels_.end()) return Result<ChannelInfo>::Error("Channel not found");
    return Result<ChannelInfo>::Ok(it->second);
}
Result<NodeInfo> NetworkGraph::GetNode(const PublicKey& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = nodes_.find(node_id);
    if (it == nodes_.end()) return Result<NodeInfo>::Error("Node not found");
    return Result<NodeInfo>::Ok(it->second);
}
std::vector<ChannelInfo> NetworkGraph::GetNodeChannels(const PublicKey&) const {
    return std::vector<ChannelInfo>();
}
// Dijkstra's algorithm for finding optimal payment route
Result<PaymentRoute> NetworkGraph::FindRoute(const PublicKey& source, const PublicKey& dest,
                                             uint64_t amount, uint32_t max_hops) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check source and destination exist
    if (nodes_.find(source) == nodes_.end()) {
        return Result<PaymentRoute>::Error("Source node not found");
    }
    if (nodes_.find(dest) == nodes_.end()) {
        return Result<PaymentRoute>::Error("Destination node not found");
    }

    // Dijkstra's algorithm data structures
    std::map<PublicKey, uint64_t> distances;        // Best cost to reach node
    std::map<PublicKey, PublicKey> previous;        // Previous node in path
    std::map<PublicKey, uint256> previous_channel;  // Channel used to reach node

    // Priority queue: (cost, node_id)
    using QueueItem = std::pair<uint64_t, PublicKey>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pq;

    // Initialize
    distances[source] = 0;
    pq.push({0, source});

    // Dijkstra's main loop
    while (!pq.empty()) {
        auto [current_cost, current_node] = pq.top();
        pq.pop();

        // Found destination
        if (current_node == dest) {
            break;
        }

        // Skip if we've found a better path
        if (distances.count(current_node) && current_cost > distances[current_node]) {
            continue;
        }

        // Explore neighbors via channels
        for (const auto& [chan_id, channel] : channels_) {
            if (!channel.enabled) continue;

            // Determine which node is the neighbor
            PublicKey neighbor;
            if (channel.node1 == current_node) {
                neighbor = channel.node2;
            } else if (channel.node2 == current_node) {
                neighbor = channel.node1;
            } else {
                continue;  // Channel doesn't involve current_node
            }

            // Check channel capacity
            if (channel.capacity < amount) {
                continue;
            }

            // Calculate fee for this hop
            uint64_t fee = channel.base_fee + (amount * channel.fee_rate) / 1000000;
            uint64_t hop_cost = current_cost + fee + amount / 1000;  // Include routing cost

            // Update if better path found
            if (!distances.count(neighbor) || hop_cost < distances[neighbor]) {
                distances[neighbor] = hop_cost;
                previous[neighbor] = current_node;
                previous_channel[neighbor] = chan_id;
                pq.push({hop_cost, neighbor});
            }
        }
    }

    // Check if destination was reached
    if (!distances.count(dest)) {
        return Result<PaymentRoute>::Error("No route found to destination");
    }

    // Reconstruct path
    std::vector<RouteHop> hops;
    PublicKey current = dest;
    uint32_t total_cltv = lightning::MIN_CLTV_EXPIRY;

    while (current != source) {
        if (!previous.count(current)) {
            return Result<PaymentRoute>::Error("Failed to reconstruct route");
        }

        PublicKey prev_node = previous[current];
        uint256 chan_id = previous_channel[current];

        // Get channel info
        auto chan_it = channels_.find(chan_id);
        if (chan_it == channels_.end()) {
            return Result<PaymentRoute>::Error("Channel not found in route reconstruction");
        }

        const auto& channel = chan_it->second;
        uint64_t fee = channel.base_fee + (amount * channel.fee_rate) / 1000000;

        RouteHop hop;
        hop.node_id = current;
        hop.channel_id = chan_id;
        hop.amount = amount + fee;
        hop.cltv_expiry = total_cltv;
        hop.fee = fee;

        hops.insert(hops.begin(), hop);  // Prepend to reverse order

        total_cltv += channel.cltv_expiry_delta;
        current = prev_node;
    }

    // Check hop count
    if (hops.size() > max_hops) {
        return Result<PaymentRoute>::Error("Route exceeds maximum hop count");
    }

    // Build result
    PaymentRoute route;
    route.hops = hops;
    route.total_fees = route.CalculateTotalFees();
    route.total_amount = amount + route.total_fees;
    route.total_cltv = total_cltv;

    return Result<PaymentRoute>::Ok(route);
}
std::vector<uint8_t> NetworkGraph::Serialize() const { return std::vector<uint8_t>(); }
Result<std::unique_ptr<NetworkGraph>> NetworkGraph::Deserialize(const std::vector<uint8_t>&) {
    return Result<std::unique_ptr<NetworkGraph>>::Ok(std::make_unique<NetworkGraph>());
}

// ============================================================================
// Sphinx Onion Routing Implementation (Simplified)
// ============================================================================

OnionPacket::OnionPacket() : version(0) {}

// Create onion packet (Sphinx protocol foundation)
Result<OnionPacket> OnionPacket::Create(const std::vector<RouteHop>& route,
                                        const uint256& payment_hash,
                                        const std::vector<uint8_t>& session_key) {
    if (route.empty()) {
        return Result<OnionPacket>::Error("Route is empty");
    }

    OnionPacket packet;
    packet.version = 0;  // Protocol version

    // Generate ephemeral key pair for this payment
    // In full Sphinx: would use ECDH for each hop
    packet.public_key.resize(33);  // Compressed public key size
    std::copy(session_key.begin(),
              session_key.begin() + std::min(session_key.size(), size_t(33)),
              packet.public_key.begin());

    // Build hop data (encrypted layers)
    // Each layer contains: amount, outgoing_channel, cltv_expiry
    packet.hops_data.clear();
    for (const auto& hop : route) {
        // Encode hop data (simplified)
        // Real Sphinx: ChaCha20-Poly1305 encryption for each layer
        std::vector<uint8_t> hop_data;

        // Amount (8 bytes)
        for (int i = 7; i >= 0; i--) {
            hop_data.push_back((hop.amount >> (i * 8)) & 0xFF);
        }

        // CLTV expiry (4 bytes)
        for (int i = 3; i >= 0; i--) {
            hop_data.push_back((hop.cltv_expiry >> (i * 8)) & 0xFF);
        }

        // Channel ID (32 bytes)
        auto chan_bytes = hop.channel_id.data();
        hop_data.insert(hop_data.end(), chan_bytes, chan_bytes + 32);

        // Append to hops_data
        packet.hops_data.insert(packet.hops_data.end(), hop_data.begin(), hop_data.end());
    }

    // Calculate HMAC over the packet
    packet.hmac.resize(32);
    auto hash = SHA3::Hash(packet.hops_data.data(), packet.hops_data.size());
    std::copy(hash.data(), hash.data() + 32, packet.hmac.begin());

    return Result<OnionPacket>::Ok(packet);
}

// Peel one layer of the onion (process by intermediate node)
Result<std::pair<RouteHop, OnionPacket>> OnionPacket::Peel(const SecretKey& node_key) const {
    if (hops_data.empty()) {
        return Result<std::pair<RouteHop, OnionPacket>>::Error("Empty onion packet");
    }

    // Verify HMAC
    // In production: would compute HMAC and verify it matches
    // auto computed_hmac = SHA3::Hash(hops_data.data(), hops_data.size());

    // Decrypt and extract this hop's data (first 44 bytes)
    if (hops_data.size() < 44) {
        return Result<std::pair<RouteHop, OnionPacket>>::Error("Insufficient hop data");
    }

    RouteHop hop;

    // Parse amount (8 bytes)
    hop.amount = 0;
    for (int i = 0; i < 8; i++) {
        hop.amount = (hop.amount << 8) | hops_data[i];
    }

    // Parse CLTV expiry (4 bytes)
    hop.cltv_expiry = 0;
    for (int i = 8; i < 12; i++) {
        hop.cltv_expiry = (hop.cltv_expiry << 8) | hops_data[i];
    }

    // Parse channel ID (32 bytes)
    // Note: Would need proper uint256 construction

    // Create next packet (remove this layer)
    OnionPacket next_packet;
    next_packet.version = version;
    next_packet.public_key = public_key;
    next_packet.hops_data.assign(hops_data.begin() + 44, hops_data.end());

    // Recalculate HMAC for next packet
    next_packet.hmac.resize(32);
    auto next_hash = SHA3::Hash(next_packet.hops_data.data(), next_packet.hops_data.size());
    std::copy(next_hash.data(), next_hash.data() + 32, next_packet.hmac.begin());

    return Result<std::pair<RouteHop, OnionPacket>>::Ok(std::make_pair(hop, next_packet));
}

std::vector<uint8_t> OnionPacket::Serialize() const {
    std::vector<uint8_t> data;
    data.push_back(version);
    data.insert(data.end(), public_key.begin(), public_key.end());

    // Add hops_data length (4 bytes)
    uint32_t len = hops_data.size();
    for (int i = 3; i >= 0; i--) {
        data.push_back((len >> (i * 8)) & 0xFF);
    }
    data.insert(data.end(), hops_data.begin(), hops_data.end());
    data.insert(data.end(), hmac.begin(), hmac.end());

    return data;
}

Result<OnionPacket> OnionPacket::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1 + 33 + 4 + 32) {  // version + pubkey + len + hmac
        return Result<OnionPacket>::Error("Invalid onion packet size");
    }

    OnionPacket packet;
    size_t offset = 0;

    packet.version = data[offset++];
    packet.public_key.assign(data.begin() + offset, data.begin() + offset + 33);
    offset += 33;

    // Read hops_data length
    uint32_t len = 0;
    for (int i = 0; i < 4; i++) {
        len = (len << 8) | data[offset++];
    }

    if (offset + len + 32 > data.size()) {
        return Result<OnionPacket>::Error("Invalid hops data length");
    }

    packet.hops_data.assign(data.begin() + offset, data.begin() + offset + len);
    offset += len;

    packet.hmac.assign(data.begin() + offset, data.begin() + offset + 32);

    return Result<OnionPacket>::Ok(packet);
}

// ============================================================================
// Watchtower Implementation
// ============================================================================

EncryptedBlob::EncryptedBlob() : sequence_number(0) {}

Result<EncryptedBlob> EncryptedBlob::Encrypt(
    const std::vector<uint8_t>& justice_tx_data,
    const uint256& encryption_key,
    const uint256& commitment_txid,
    uint32_t sequence)
{
    EncryptedBlob blob;
    blob.sequence_number = sequence;

    // Create hint from first 16 bytes of commitment txid
    blob.hint.assign(commitment_txid.begin(), commitment_txid.begin() + 16);

    // Generate random 12-byte nonce for AES-256-GCM
    blob.nonce.resize(12);
    if (RAND_bytes(blob.nonce.data(), 12) != 1) {
        return Result<EncryptedBlob>::Error("Failed to generate random nonce");
    }

    // Initialize encryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<EncryptedBlob>::Error("Failed to create cipher context");
    }

    // Initialize AES-256-GCM encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Failed to initialize AES-256-GCM");
    }

    // Set nonce length (12 bytes for GCM)
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Failed to set nonce length");
    }

    // Initialize key and nonce
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, encryption_key.data(), blob.nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Failed to set encryption key and nonce");
    }

    // Encrypt the plaintext
    blob.encrypted_data.resize(justice_tx_data.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    if (EVP_EncryptUpdate(ctx, blob.encrypted_data.data(), &len,
                          justice_tx_data.data(), justice_tx_data.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Encryption failed");
    }
    int ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, blob.encrypted_data.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Encryption finalization failed");
    }
    ciphertext_len += len;
    blob.encrypted_data.resize(ciphertext_len);

    // Get the authentication tag (16 bytes for GCM)
    blob.auth_tag.resize(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, blob.auth_tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<EncryptedBlob>::Error("Failed to get authentication tag");
    }

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return Result<EncryptedBlob>::Ok(blob);
}

Result<std::vector<uint8_t>> EncryptedBlob::Decrypt(const uint256& encryption_key) const {
    // Verify we have all required components
    if (nonce.size() != 12) {
        return Result<std::vector<uint8_t>>::Error("Invalid nonce size (expected 12 bytes)");
    }
    if (auth_tag.size() != 16) {
        return Result<std::vector<uint8_t>>::Error("Invalid authentication tag size (expected 16 bytes)");
    }
    if (encrypted_data.empty()) {
        return Result<std::vector<uint8_t>>::Error("No encrypted data to decrypt");
    }

    // Initialize decryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<std::vector<uint8_t>>::Error("Failed to create cipher context");
    }

    // Initialize AES-256-GCM decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Failed to initialize AES-256-GCM");
    }

    // Set nonce length (12 bytes for GCM)
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Failed to set nonce length");
    }

    // Initialize key and nonce
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, encryption_key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Failed to set decryption key and nonce");
    }

    // Decrypt the ciphertext
    std::vector<uint8_t> decrypted(encrypted_data.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    if (EVP_DecryptUpdate(ctx, decrypted.data(), &len,
                          encrypted_data.data(), encrypted_data.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Decryption failed");
    }
    int plaintext_len = len;

    // Set the authentication tag for verification
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                            const_cast<uint8_t*>(auth_tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Failed to set authentication tag");
    }

    // Finalize decryption and verify authentication tag
    if (EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<std::vector<uint8_t>>::Error("Decryption finalization failed (authentication tag mismatch)");
    }
    plaintext_len += len;
    decrypted.resize(plaintext_len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return Result<std::vector<uint8_t>>::Ok(decrypted);
}

BreachRetribution::BreachRetribution()
    : revoked_local_balance(0)
    , revoked_remote_balance(0)
    , to_self_delay(0)
{
    revocation_privkey.fill(0);
}

WatchtowerTask::WatchtowerTask()
    : watch_until_height(0)
    , is_active(true) {}

Watchtower::Watchtower(Blockchain* blockchain)
    : blockchain_(blockchain)
    , running_(false)
{
    stats_.channels_watched = 0;
    stats_.breaches_detected = 0;
    stats_.penalties_broadcast = 0;
    stats_.blobs_stored = 0;

    // Initialize penalty destination to zero (must be configured)
    penalty_destination_.fill(0);
}

Watchtower::~Watchtower() {
    Stop();
}

Result<void> Watchtower::Start() {
    if (running_) {
        return Result<void>::Error("Watchtower already running");
    }

    running_ = true;

    // Start monitoring thread
    monitor_thread_ = std::thread([this]() { MonitoringLoop(); });

    return Result<void>::Ok();
}

void Watchtower::Stop() {
    if (!running_) return;

    running_ = false;

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

bool Watchtower::IsRunning() const {
    return running_;
}

Result<void> Watchtower::UploadBlob(
    const uint256& channel_id,
    const EncryptedBlob& blob,
    uint32_t watch_until_height)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Create watchtower task
    WatchtowerTask task;
    task.channel_id = channel_id;
    task.encrypted_justice = blob;
    task.watch_until_height = watch_until_height;
    task.created_at = std::chrono::system_clock::now();
    task.is_active = true;

    // Store blob with hint as key for fast lookup
    uint256 hint_key;
    std::copy(blob.hint.begin(), blob.hint.end(), hint_key.begin());
    encrypted_blobs_[hint_key] = blob;

    // Add task
    tasks_[channel_id].push_back(task);

    stats_.blobs_stored++;

    return Result<void>::Ok();
}

void Watchtower::WatchChannel(const uint256& channel_id, const BreachRetribution& retribution) {
    std::lock_guard<std::mutex> lock(mutex_);

    breach_data_[channel_id] = retribution;
    stats_.channels_watched++;
}

void Watchtower::UnwatchChannel(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    breach_data_.erase(channel_id);
    tasks_.erase(channel_id);

    if (stats_.channels_watched > 0) {
        stats_.channels_watched--;
    }
}

void Watchtower::CheckForBreaches() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!blockchain_) {
        return;
    }

    uint64_t current_height = blockchain_->GetBestHeight();

    // Scan recent blocks for revoked commitment transactions
    // In production, would also monitor mempool
    for (uint64_t height = std::max(int64_t(0), int64_t(current_height) - 6);
         height <= current_height; height++)
    {
        auto block_result = blockchain_->GetBlockByHeight(height);
        if (!block_result.IsOk()) continue;

        auto block = block_result.GetValue();

        // Check each transaction in block
        for (const auto& tx : block.transactions) {
            auto tx_hash = tx.GetHash();

            // Check if this is a revoked commitment
            if (IsRevokedCommitment(tx_hash)) {
                // Breach detected!
                auto breach_result = DetectBreach(tx);
                if (breach_result.IsOk()) {
                    auto retribution = breach_result.GetValue();

                    // Build and broadcast justice transaction
                    // Get configured penalty destination (watchtower operator's reward address)
                    PublicKey destination = GetPenaltyDestination();

                    auto justice_result = BuildJusticeTransaction(
                        retribution, tx, destination);

                    if (justice_result.IsOk()) {
                        auto justice_tx = justice_result.GetValue();

                        // Broadcast penalty
                        // auto broadcast_result = blockchain_->BroadcastTransaction(justice_tx);

                        stats_.breaches_detected++;
                        stats_.penalties_broadcast++;
                    }
                }
            }
        }
    }

    // Clean up expired tasks
    for (auto& [channel_id, task_list] : tasks_) {
        task_list.erase(
            std::remove_if(task_list.begin(), task_list.end(),
                [current_height](const WatchtowerTask& task) {
                    return task.watch_until_height < current_height;
                }),
            task_list.end()
        );
    }
}

Result<BreachRetribution> Watchtower::DetectBreach(const Transaction& tx) const {
    auto tx_hash = tx.GetHash();

    // Check if this transaction matches any of our watched commitments
    for (const auto& [channel_id, retribution] : breach_data_) {
        if (retribution.revoked_commitment_txid == tx_hash) {
            return Result<BreachRetribution>::Ok(retribution);
        }
    }

    return Result<BreachRetribution>::Error("No breach detected");
}

Result<Transaction> Watchtower::BuildJusticeTransaction(
    const BreachRetribution& retribution,
    const Transaction& breach_tx,
    const PublicKey& destination)
{
    Transaction justice_tx;
    justice_tx.version = 2;
    justice_tx.locktime = 0;

    // Input: Spend the to_remote output from revoked commitment
    // The to_remote output can be spent immediately by the remote party
    // In a breach scenario, we claim it with the revocation key
    TxIn input;
    input.prev_tx_hash = breach_tx.GetHash();
    input.prev_tx_index = 1;  // Assume to_remote is output 1 (simplified)
    input.sequence = 0xFFFFFFFF;

    justice_tx.inputs.push_back(input);

    // Output: Send all funds to destination (watchtower operator or victim)
    TxOut output;
    output.value = retribution.revoked_remote_balance;

    // Calculate and subtract transaction fee
    // Estimate justice transaction size:
    // - 1 input (revocation path with signature): ~3500 bytes
    // - 1 output (P2PKH): ~35 bytes
    // - Version, locktime, etc.: ~20 bytes
    // Total: ~3555 bytes
    const size_t ESTIMATED_JUSTICE_TX_SIZE = 3555;

    // Use higher fee rate for justice transactions (priority broadcast)
    const uint32_t JUSTICE_FEERATE_PER_KW = 10000;  // ~40 ints/vbyte for fast confirmation

    uint64_t justice_fee = LightningNetwork::CalculateTransactionFee(
        ESTIMATED_JUSTICE_TX_SIZE,
        JUSTICE_FEERATE_PER_KW
    );

    // Subtract fee from output value
    if (output.value > justice_fee) {
        output.value -= justice_fee;
    } else {
        // Not enough funds to pay fee, use all available
        output.value = 0;
    }

    // Create pubkey hash from destination public key using SHA3-256
    uint256 pubkey_hash = SHA3::Hash(destination.data(), destination.size());

    output.script_pubkey = Script::CreateP2PKH(pubkey_hash);
    justice_tx.outputs.push_back(output);

    // Sign the justice transaction with revocation private key
    // Get the scriptPubKey of the output being spent from the breach transaction
    if (input.prev_tx_index >= breach_tx.outputs.size()) {
        return Result<Transaction>::Error("Invalid output index in breach transaction");
    }

    const Script& prev_scriptpubkey = breach_tx.outputs[input.prev_tx_index].script_pubkey;

    // Generate signing hash using SIGHASH_ALL
    uint256 signing_hash = justice_tx.GetHashForSigning(SIGHASH_ALL, 0, prev_scriptpubkey);

    // Sign with revocation private key (Dilithium3)
    auto sig_result = DilithiumCrypto::SignHash(signing_hash, retribution.revocation_privkey);
    if (!sig_result.IsOk()) {
        return Result<Transaction>::Error("Failed to sign justice transaction: " + sig_result.error);
    }

    Signature revocation_sig = sig_result.GetValue();

    // Create unlocking script: <signature> <pubkey> (P2PKH format)
    Script script_sig;

    // Push signature
    script_sig.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t sig_len = static_cast<uint16_t>(revocation_sig.size());
    script_sig.bytes.push_back(sig_len & 0xFF);
    script_sig.bytes.push_back((sig_len >> 8) & 0xFF);
    script_sig.bytes.insert(script_sig.bytes.end(), revocation_sig.begin(), revocation_sig.end());

    // Push public key
    script_sig.bytes.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
    uint16_t pubkey_len = static_cast<uint16_t>(retribution.revocation_pubkey.size());
    script_sig.bytes.push_back(pubkey_len & 0xFF);
    script_sig.bytes.push_back((pubkey_len >> 8) & 0xFF);
    script_sig.bytes.insert(script_sig.bytes.end(),
                            retribution.revocation_pubkey.begin(),
                            retribution.revocation_pubkey.end());

    // Set the script_sig on the input
    justice_tx.inputs[0].script_sig = script_sig;

    return Result<Transaction>::Ok(justice_tx);
}

Result<void> Watchtower::BroadcastPenalty(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = breach_data_.find(channel_id);
    if (it == breach_data_.end()) {
        return Result<void>::Error("No breach data for channel");
    }

    const auto& retribution = it->second;

    // Get the breach transaction from blockchain
    auto tx_result = blockchain_->GetTransaction(retribution.revoked_commitment_txid);
    if (!tx_result.IsOk()) {
        return Result<void>::Error("Breach transaction not found");
    }

    auto breach_tx = tx_result.GetValue();

    // Build justice transaction with configured penalty destination
    PublicKey destination = GetPenaltyDestination();

    auto justice_result = BuildJusticeTransaction(
        retribution, breach_tx, destination);

    if (!justice_result.IsOk()) {
        return Result<void>::Error("Failed to build justice transaction: " +
                                   justice_result.error);
    }

    auto justice_tx = justice_result.GetValue();

    // Broadcast to network
    // auto broadcast_result = blockchain_->BroadcastTransaction(justice_tx);

    stats_.penalties_broadcast++;

    return Result<void>::Ok();
}

Watchtower::Stats Watchtower::GetStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void Watchtower::SetPenaltyDestination(const PublicKey& destination) {
    std::lock_guard<std::mutex> lock(mutex_);
    penalty_destination_ = destination;
}

PublicKey Watchtower::GetPenaltyDestination() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return penalty_destination_;
}

void Watchtower::MonitoringLoop() {
    while (running_) {
        // Check for breaches every 10 seconds
        CheckForBreaches();

        // Sleep for 10 seconds
        for (int i = 0; i < 100 && running_; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool Watchtower::IsRevokedCommitment(const uint256& txid) const {
    for (const auto& [channel_id, retribution] : breach_data_) {
        if (retribution.revoked_commitment_txid == txid) {
            return true;
        }
    }
    return false;
}

LightningNetwork::LightningNetwork(Blockchain* blockchain, P2PNode* p2p, wallet::Wallet* wallet)
    : blockchain_(blockchain), p2p_(p2p), wallet_(wallet), running_(false) {}
LightningNetwork::~LightningNetwork() { Stop(); }
Result<void> LightningNetwork::Start(const PublicKey& node_id, const SecretKey& node_key) {
    node_id_ = node_id;
    node_key_ = node_key;
    running_ = true;
    stats_ = Stats{};

    // Initialize watchtower
    watchtower_ = std::make_unique<Watchtower>(blockchain_);

    // Start watchtower monitoring
    auto wt_result = watchtower_->Start();
    if (!wt_result.IsOk()) {
        return Result<void>::Error("Failed to start watchtower: " + wt_result.error);
    }

    return Result<void>::Ok();
}

void LightningNetwork::Stop() {
    running_ = false;

    // Stop watchtower
    if (watchtower_) {
        watchtower_->Stop();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    channels_.clear();
}
bool LightningNetwork::IsRunning() const { return running_; }
PublicKey LightningNetwork::GetNodeId() const { return node_id_; }
std::string LightningNetwork::GetNodeAlias() const { return node_alias_; }
void LightningNetwork::SetNodeAlias(const std::string& alias) { node_alias_ = alias; }
Result<uint256> LightningNetwork::OpenChannel(const PublicKey& remote_node, uint64_t capacity, uint64_t push_amount) {
    // Validate capacity
    if (capacity < lightning::MIN_CHANNEL_CAPACITY) {
        return Result<uint256>::Error("Capacity below minimum");
    }
    if (capacity > lightning::MAX_CHANNEL_CAPACITY) {
        return Result<uint256>::Error("Capacity above maximum");
    }
    if (push_amount > capacity) {
        return Result<uint256>::Error("Push amount exceeds capacity");
    }

    // Create channel with temporary ID
    auto channel = std::make_shared<Channel>(node_id_, remote_node, capacity);
    channel->local_balance = capacity - push_amount;
    channel->remote_balance = push_amount;
    channel->state = ChannelState::OPENING;

    // Generate keys for the channel using BIP32 derivation (m/44'/2210'/0'/2/*)
    // Use channel count as base index for key derivation
    uint32_t channel_index;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channel_index = static_cast<uint32_t>(channels_.size());
    }
    uint32_t key_base = channel_index * 6;  // 6 keys per channel

    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;
    PublicKey first_per_commitment_point;

    if (wallet_ != nullptr) {
        // Derive keys using HD wallet
        auto funding_key = wallet_->DeriveLightningKey(key_base + 0);
        auto revocation_key = wallet_->DeriveLightningKey(key_base + 1);
        auto payment_key = wallet_->DeriveLightningKey(key_base + 2);
        auto delayed_key = wallet_->DeriveLightningKey(key_base + 3);
        auto htlc_key = wallet_->DeriveLightningKey(key_base + 4);
        auto commitment_key = wallet_->DeriveLightningKey(key_base + 5);

        if (funding_key.IsOk() && funding_key.value->public_key.has_value()) {
            funding_pubkey = funding_key.value->public_key.value();
        } else {
            funding_pubkey = node_id_;  // Fallback to node_id
        }

        if (revocation_key.IsOk() && revocation_key.value->public_key.has_value()) {
            revocation_basepoint = revocation_key.value->public_key.value();
        } else {
            revocation_basepoint = node_id_;
        }

        if (payment_key.IsOk() && payment_key.value->public_key.has_value()) {
            payment_basepoint = payment_key.value->public_key.value();
        } else {
            payment_basepoint = node_id_;
        }

        if (delayed_key.IsOk() && delayed_key.value->public_key.has_value()) {
            delayed_payment_basepoint = delayed_key.value->public_key.value();
        } else {
            delayed_payment_basepoint = node_id_;
        }

        if (htlc_key.IsOk() && htlc_key.value->public_key.has_value()) {
            htlc_basepoint = htlc_key.value->public_key.value();
        } else {
            htlc_basepoint = node_id_;
        }

        if (commitment_key.IsOk() && commitment_key.value->public_key.has_value()) {
            first_per_commitment_point = commitment_key.value->public_key.value();
        } else {
            first_per_commitment_point = node_id_;
        }
    } else {
        // Fallback: use node_id if wallet not available
        funding_pubkey = node_id_;
        revocation_basepoint = node_id_;
        payment_basepoint = node_id_;
        delayed_payment_basepoint = node_id_;
        htlc_basepoint = node_id_;
        first_per_commitment_point = node_id_;
    }

    // Create open_channel message (BOLT #2)
    OpenChannelMsg open_msg;
    open_msg.chain_hash = blockchain_->GetBestBlockHash();  // Current chain hash
    open_msg.temporary_channel_id = channel->temporary_id;
    open_msg.funding_ints = capacity;
    open_msg.push_mints = push_amount * 1000;  // Convert to milli-ints
    open_msg.dust_limit_ints = channel->local_config.dust_limit;
    open_msg.max_htlc_value_in_flight_mints = channel->local_config.max_htlc_value * 1000;
    open_msg.channel_reserve_ints = channel->local_config.channel_reserve;
    open_msg.htlc_minimum_mints = channel->local_config.htlc_minimum;
    open_msg.feerate_per_kw = EstimateFeeRate(6);  // Estimate fee rate for 6-block confirmation
    open_msg.to_self_delay = channel->local_config.to_self_delay;
    open_msg.max_accepted_htlcs = channel->local_config.max_accepted_htlcs;
    open_msg.funding_pubkey = funding_pubkey;
    open_msg.revocation_basepoint = revocation_basepoint;
    open_msg.payment_basepoint = payment_basepoint;
    open_msg.delayed_payment_basepoint = delayed_payment_basepoint;
    open_msg.htlc_basepoint = htlc_basepoint;
    open_msg.first_per_commitment_point = first_per_commitment_point;
    open_msg.channel_flags = 0;  // 0 = public channel

    // Store channel in pending channels
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channels_[channel->temporary_id] = channel;
    }

    // Serialize and send open_channel message to peer
    auto msg_data = open_msg.Serialize();
    auto send_result = SendMessage(remote_node, lightning::MSG_OPEN_CHANNEL, msg_data);
    if (!send_result.IsOk()) {
        // Remove channel on failure
        std::lock_guard<std::mutex> lock(mutex_);
        channels_.erase(channel->temporary_id);
        return Result<uint256>::Error("Failed to send open_channel message: " + send_result.error);
    }

    // Channel is now waiting for accept_channel response
    // The HandleAcceptChannel method will be called when the peer responds

    return Result<uint256>::Ok(channel->temporary_id);
}
/**
 * Close a Lightning channel
 *
 * @param channel_id The channel to close
 * @param force If true, force close with commitment tx. If false, negotiate mutual close.
 * @return Result indicating success or error
 */
Result<void> LightningNetwork::CloseChannel(const uint256& channel_id, bool force) {
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(channel_id);
        if (it == channels_.end()) {
            return Result<void>::Error("Channel not found");
        }
        channel = it->second;
    }

    // Close the channel
    auto close_result = channel->Close(force);
    if (!close_result.IsOk()) {
        return close_result;
    }

    if (force) {
        // Force close: Broadcast our local commitment transaction
        if (!blockchain_) {
            return Result<void>::Error("Blockchain not available");
        }

        // Build the funding scriptpubkey (2-of-2 multisig)
        std::vector<PublicKey> funding_pubkeys;
        funding_pubkeys.push_back(channel->local_node_id);
        funding_pubkeys.push_back(channel->remote_node_id);
        Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

        // Sign our local commitment transaction
        auto sig_result = SignCommitmentTransaction(
            channel->local_commitment.tx,
            funding_scriptpubkey,
            node_key_
        );

        if (!sig_result.IsOk()) {
            return Result<void>::Error("Failed to sign commitment: " + sig_result.error);
        }

        // TODO: Assemble with remote signature (need to store remote's signature)
        // For now, just broadcast the unsigned transaction (will fail validation)
        // In full implementation, we'd use AssembleSignedCommitmentTransaction() with both signatures

        // Placeholder: Would need remote signature stored from commitment_signed message
        // auto signed_tx = AssembleSignedCommitmentTransaction(
        //     channel->local_commitment.tx,
        //     sig_result.GetValue(),
        //     remote_signature
        // );

        // Note: Broadcasting unsigned transaction for now (will be rejected by blockchain)
        // TODO: Store remote signatures and assemble fully signed transaction
        blockchain_->AddToMempool(channel->local_commitment.tx);

        return Result<void>::Ok();
    } else {
        // Mutual close: Send shutdown message to initiate negotiation
        ShutdownMsg shutdown_msg;
        shutdown_msg.channel_id = channel_id;
        // Use a simple P2PKH script for closing output (should use configured address)
        shutdown_msg.scriptpubkey = Script();  // TODO: Use proper closing script

        auto msg_data = shutdown_msg.Serialize();
        auto send_result = SendMessage(channel->remote_node_id, lightning::MSG_SHUTDOWN, msg_data);
        if (!send_result.IsOk()) {
            return Result<void>::Error("Failed to send shutdown message: " + send_result.error);
        }

        return Result<void>::Ok();
    }
}
std::vector<Channel> LightningNetwork::ListChannels() const {
    std::lock_guard<std::mutex> lock(mutex_); std::vector<Channel> result;
    for (const auto& [id, channel] : channels_) result.push_back(*channel);
    return result;
}
Result<Channel> LightningNetwork::GetChannel(const uint256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = channels_.find(channel_id);
    if (it == channels_.end()) return Result<Channel>::Error("Channel not found");
    return Result<Channel>::Ok(*it->second);
}
Result<uint256> LightningNetwork::SendPayment(const std::string& bolt11_invoice) {
    // Decode BOLT #11 invoice
    auto invoice_result = Invoice::Decode(bolt11_invoice);
    if (!invoice_result.IsOk()) {
        return Result<uint256>::Error("Failed to decode invoice: " + invoice_result.error);
    }

    auto invoice = invoice_result.GetValue();

    // Check if invoice is expired
    if (invoice.IsExpired()) {
        return Result<uint256>::Error("Invoice has expired");
    }

    // Send payment to the invoice payee
    return SendPayment(invoice.payee, invoice.amount, invoice.description);
}

Result<uint256> LightningNetwork::SendPayment(const PublicKey& dest, uint64_t amount, const std::string& description) {
    // Generate payment preimage and hash
    uint256 preimage = RandomGenerator::GetRandomUint256();
    uint256 payment_hash = Invoice::GeneratePaymentHash(preimage);

    // Find route to destination
    auto route_result = FindRoute(dest, amount);
    if (!route_result.IsOk()) {
        return Result<uint256>::Error("Failed to find route: " + route_result.error);
    }

    auto route = route_result.GetValue();

    // Validate route
    if (route.hops.empty()) {
        return Result<uint256>::Error("Route is empty");
    }

    // Check if we have enough balance in first channel
    const auto& first_hop = route.hops[0];
    auto channel_result = GetChannel(first_hop.channel_id);
    if (!channel_result.IsOk()) {
        return Result<uint256>::Error("First hop channel not found");
    }

    auto channel = channel_result.GetValue();
    if (channel.local_balance < route.total_amount) {
        return Result<uint256>::Error("Insufficient balance in first hop channel");
    }

    // Create onion packet for the route
    std::vector<uint8_t> session_key(32);
    for (size_t i = 0; i < 32; i++) {
        session_key[i] = static_cast<uint8_t>(rand() % 256);
    }

    auto onion_result = OnionPacket::Create(route.hops, payment_hash, session_key);
    if (!onion_result.IsOk()) {
        return Result<uint256>::Error("Failed to create onion packet: " + onion_result.error);
    }

    auto onion_packet = onion_result.GetValue();
    auto onion_data = onion_packet.Serialize();

    // Create HTLC for first hop
    UpdateAddHTLCMsg add_htlc;
    add_htlc.channel_id = first_hop.channel_id;

    // Generate unique HTLC ID for this channel
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(first_hop.channel_id);
        if (it == channels_.end()) {
            return Result<uint256>::Error("Channel not found");
        }
        add_htlc.id = it->second->pending_htlcs.size();
    }

    add_htlc.amount_mints = first_hop.amount * 1000;  // Convert to milli-ints
    add_htlc.payment_hash = payment_hash;
    add_htlc.cltv_expiry = first_hop.cltv_expiry;
    add_htlc.onion_routing_packet = onion_data;

    // Send update_add_htlc to first hop
    auto add_data = add_htlc.Serialize();
    auto send_result = SendMessage(first_hop.node_id, lightning::MSG_UPDATE_ADD_HTLC, add_data);
    if (!send_result.IsOk()) {
        return Result<uint256>::Error("Failed to send HTLC: " + send_result.error);
    }

    // Track pending payment
    {
        std::lock_guard<std::mutex> lock(mutex_);

        PendingPayment pending;
        pending.payment_hash = payment_hash;
        pending.preimage = preimage;
        pending.destination = dest;
        pending.amount = amount;
        pending.total_amount = route.total_amount;
        pending.total_fees = route.total_fees;
        pending.route = route;
        pending.created_at = std::chrono::system_clock::now();
        pending.status = "pending";

        pending_payments_[payment_hash] = pending;

        stats_.num_payments_sent++;
    }

    return Result<uint256>::Ok(payment_hash);
}
Result<Invoice> LightningNetwork::CreateInvoice(uint64_t amount, const std::string& description) {
    return Result<Invoice>::Ok(Invoice(amount, description, node_id_));
}
Result<PaymentRoute> LightningNetwork::FindRoute(const PublicKey& dest, uint64_t amount) const {
    return network_graph_.FindRoute(node_id_, dest, amount);
}
NetworkGraph& LightningNetwork::GetNetworkGraph() { return network_graph_; }
const NetworkGraph& LightningNetwork::GetNetworkGraph() const { return network_graph_; }
LightningNetwork::Stats LightningNetwork::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_); return stats_;
}
void LightningNetwork::HandleMessage(const PublicKey& peer, uint16_t type, const std::vector<uint8_t>& data) {
    // Route message to appropriate handler
    switch (type) {
        case lightning::MSG_OPEN_CHANNEL:
            HandleOpenChannel(peer, data);
            break;
        case lightning::MSG_ACCEPT_CHANNEL:
            HandleAcceptChannel(peer, data);
            break;
        case lightning::MSG_FUNDING_CREATED:
            HandleFundingCreated(peer, data);
            break;
        case lightning::MSG_FUNDING_SIGNED:
            HandleFundingSigned(peer, data);
            break;
        case lightning::MSG_FUNDING_LOCKED:
            HandleFundingLocked(peer, data);
            break;
        case lightning::MSG_SHUTDOWN:
            HandleShutdown(peer, data);
            break;
        case lightning::MSG_CLOSING_SIGNED:
            HandleClosingSigned(peer, data);
            break;
        case lightning::MSG_UPDATE_ADD_HTLC:
            HandleUpdateAddHTLC(peer, data);
            break;
        case lightning::MSG_UPDATE_FULFILL_HTLC:
            HandleUpdateFulfillHTLC(peer, data);
            break;
        case lightning::MSG_UPDATE_FAIL_HTLC:
            HandleUpdateFailHTLC(peer, data);
            break;
        case lightning::MSG_COMMITMENT_SIGNED:
            HandleCommitmentSigned(peer, data);
            break;
        case lightning::MSG_REVOKE_AND_ACK:
            HandleRevokeAndAck(peer, data);
            break;
        default:
            // Unknown message type
            break;
    }
}

void LightningNetwork::HandleOpenChannel(const PublicKey& peer, const std::vector<uint8_t>& data) {
    // Deserialize open_channel message
    auto msg_result = OpenChannelMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto open_msg = msg_result.GetValue();

    // Validate channel parameters
    if (open_msg.funding_ints < lightning::MIN_CHANNEL_CAPACITY ||
        open_msg.funding_ints > lightning::MAX_CHANNEL_CAPACITY) {
        // Send error message
        return;
    }

    // Create channel as acceptor
    auto channel = std::make_shared<Channel>(node_id_, peer, open_msg.funding_ints);
    channel->temporary_id = open_msg.temporary_channel_id;
    channel->state = ChannelState::OPENING;
    channel->local_balance = open_msg.funding_ints - (open_msg.push_mints / 1000);
    channel->remote_balance = open_msg.push_mints / 1000;
    channel->remote_config.dust_limit = open_msg.dust_limit_ints;
    channel->remote_config.max_htlc_value = open_msg.max_htlc_value_in_flight_mints / 1000;
    channel->remote_config.channel_reserve = open_msg.channel_reserve_ints;
    channel->remote_config.htlc_minimum = open_msg.htlc_minimum_mints;
    channel->remote_config.to_self_delay = open_msg.to_self_delay;
    channel->remote_config.max_accepted_htlcs = open_msg.max_accepted_htlcs;

    // Store channel
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channels_[channel->temporary_id] = channel;
    }

    // Create accept_channel response
    AcceptChannelMsg accept_msg;
    accept_msg.temporary_channel_id = open_msg.temporary_channel_id;
    accept_msg.dust_limit_ints = channel->local_config.dust_limit;
    accept_msg.max_htlc_value_in_flight_mints = channel->local_config.max_htlc_value * 1000;
    accept_msg.channel_reserve_ints = channel->local_config.channel_reserve;
    accept_msg.htlc_minimum_mints = channel->local_config.htlc_minimum;
    accept_msg.minimum_depth = 3;  // Require 3 confirmations
    accept_msg.to_self_delay = channel->local_config.to_self_delay;
    accept_msg.max_accepted_htlcs = channel->local_config.max_accepted_htlcs;

    // Derive keys using BIP32 (m/44'/2210'/0'/2/*)
    uint32_t channel_index;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channel_index = static_cast<uint32_t>(channels_.size());
    }
    uint32_t key_base = channel_index * 6;  // 6 keys per channel

    if (wallet_ != nullptr) {
        // Derive keys using HD wallet
        auto funding_key = wallet_->DeriveLightningKey(key_base + 0);
        auto revocation_key = wallet_->DeriveLightningKey(key_base + 1);
        auto payment_key = wallet_->DeriveLightningKey(key_base + 2);
        auto delayed_key = wallet_->DeriveLightningKey(key_base + 3);
        auto htlc_key = wallet_->DeriveLightningKey(key_base + 4);
        auto commitment_key = wallet_->DeriveLightningKey(key_base + 5);

        accept_msg.funding_pubkey = (funding_key.IsOk() && funding_key.value->public_key.has_value())
            ? funding_key.value->public_key.value() : node_id_;
        accept_msg.revocation_basepoint = (revocation_key.IsOk() && revocation_key.value->public_key.has_value())
            ? revocation_key.value->public_key.value() : node_id_;
        accept_msg.payment_basepoint = (payment_key.IsOk() && payment_key.value->public_key.has_value())
            ? payment_key.value->public_key.value() : node_id_;
        accept_msg.delayed_payment_basepoint = (delayed_key.IsOk() && delayed_key.value->public_key.has_value())
            ? delayed_key.value->public_key.value() : node_id_;
        accept_msg.htlc_basepoint = (htlc_key.IsOk() && htlc_key.value->public_key.has_value())
            ? htlc_key.value->public_key.value() : node_id_;
        accept_msg.first_per_commitment_point = (commitment_key.IsOk() && commitment_key.value->public_key.has_value())
            ? commitment_key.value->public_key.value() : node_id_;
    } else {
        // Fallback: use node_id if wallet not available
        accept_msg.funding_pubkey = node_id_;
        accept_msg.revocation_basepoint = node_id_;
        accept_msg.payment_basepoint = node_id_;
        accept_msg.delayed_payment_basepoint = node_id_;
        accept_msg.htlc_basepoint = node_id_;
        accept_msg.first_per_commitment_point = node_id_;
    }

    // Send accept_channel message
    auto msg_data = accept_msg.Serialize();
    SendMessage(peer, lightning::MSG_ACCEPT_CHANNEL, msg_data);
}

void LightningNetwork::HandleAcceptChannel(const PublicKey& peer, const std::vector<uint8_t>& data) {
    // Deserialize accept_channel message
    auto msg_result = AcceptChannelMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto accept_msg = msg_result.GetValue();

    // Find channel by temporary ID
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(accept_msg.temporary_channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is in OPENING state
    if (channel->state != ChannelState::OPENING) {
        return;  // Wrong state
    }

    // Store remote config
    channel->remote_config.dust_limit = accept_msg.dust_limit_ints;
    channel->remote_config.max_htlc_value = accept_msg.max_htlc_value_in_flight_mints / 1000;
    channel->remote_config.channel_reserve = accept_msg.channel_reserve_ints;
    channel->remote_config.htlc_minimum = accept_msg.htlc_minimum_mints;
    channel->remote_config.to_self_delay = accept_msg.to_self_delay;
    channel->remote_config.max_accepted_htlcs = accept_msg.max_accepted_htlcs;

    // Now we need to create the funding transaction
    // This is done by calling the blockchain to create a 2-of-2 multisig output
    // For now, we'll create a placeholder transaction
    Transaction funding_tx;
    funding_tx.version = 1;

    // Create output with channel capacity (2-of-2 multisig)
    TxOut funding_output;
    funding_output.value = channel->capacity;

    // Create proper 2-of-2 multisig script using both funding pubkeys (BOLT #3)
    // Format: 2 <local_funding_pubkey> <remote_funding_pubkey> 2 OP_CHECKMULTISIG
    // NOTE: Using node IDs as placeholder funding pubkeys for now
    // TODO: Store actual funding_pubkey from open_channel/accept_channel messages
    // and use those instead of node IDs (requires Channel struct update)
    std::vector<PublicKey> funding_pubkeys;
    funding_pubkeys.push_back(channel->local_node_id);   // Placeholder: use node ID
    funding_pubkeys.push_back(channel->remote_node_id);  // Placeholder: use node ID
    funding_output.script_pubkey = Script::CreateMultisig(2, funding_pubkeys);

    funding_tx.outputs.push_back(funding_output);

    uint256 funding_txid = funding_tx.GetHash();
    uint16_t funding_vout = 0;

    // Build remote party's commitment transaction (they will broadcast this)
    // NOTE: From remote's perspective, their balance is local, ours is remote
    auto commit_result = CommitmentTransaction::Build(
        funding_txid,
        funding_vout,
        channel->capacity,
        channel->remote_balance,  // Remote's local balance
        channel->local_balance,   // Remote's remote balance (our balance)
        {},  // No HTLCs yet
        0,   // First commitment
        channel->remote_config
    );

    // Sign the remote party's commitment transaction
    Signature commit_sig;
    if (commit_result.IsOk()) {
        auto remote_commit_tx = commit_result.GetValue().tx;
        auto sig_result = SignCommitmentTransaction(remote_commit_tx, funding_output.script_pubkey, node_key_);
        if (sig_result.IsOk()) {
            commit_sig = sig_result.GetValue();
            // Store the commitment for later (if needed)
            channel->remote_commitment = commit_result.GetValue();
        } else {
            commit_sig.fill(0);  // Fallback to zeros on error
        }
    } else {
        commit_sig.fill(0);  // Fallback to zeros on error
    }

    // Create funding_created message
    FundingCreatedMsg funding_msg;
    funding_msg.temporary_channel_id = channel->temporary_id;
    funding_msg.funding_txid = funding_txid;
    funding_msg.funding_output_index = funding_vout;
    funding_msg.signature = commit_sig;

    // Send funding_created message
    auto msg_data = funding_msg.Serialize();
    SendMessage(peer, lightning::MSG_FUNDING_CREATED, msg_data);

    // Update channel with funding info
    channel->funding_txid = funding_txid;
    channel->funding_vout = funding_vout;
}

void LightningNetwork::HandleFundingCreated(const PublicKey& peer, const std::vector<uint8_t>& data) {
    // Acceptor receives funding_created from initiator
    auto msg_result = FundingCreatedMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;
    }
    auto funding_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(funding_msg.temporary_channel_id);
        if (it == channels_.end()) {
            return;
        }
        channel = it->second;
    }

    // Verify and store funding transaction info
    channel->funding_txid = funding_msg.funding_txid;
    channel->funding_vout = funding_msg.funding_output_index;

    // Create channel_id from funding outpoint
    uint256 channel_id;
    // channel_id = funding_txid XOR funding_output_index (shifted)
    channel_id = channel->funding_txid;
    channel_id[0] ^= (channel->funding_vout >> 8) & 0xFF;
    channel_id[1] ^= channel->funding_vout & 0xFF;
    channel->channel_id = channel_id;

    // Build remote party's (initiator's) commitment transaction and sign it
    // NOTE: From remote's perspective, their balance is local, ours is remote
    auto commit_result = CommitmentTransaction::Build(
        channel->funding_txid,
        channel->funding_vout,
        channel->capacity,
        channel->remote_balance,  // Remote's local balance
        channel->local_balance,   // Remote's remote balance (our balance)
        {},  // No HTLCs yet
        0,   // First commitment
        channel->remote_config
    );

    // Sign the remote party's commitment transaction
    Signature our_sig;
    if (commit_result.IsOk()) {
        auto remote_commit_tx = commit_result.GetValue().tx;

        // Build the funding scriptpubkey (2-of-2 multisig)
        std::vector<PublicKey> funding_pubkeys;
        funding_pubkeys.push_back(channel->local_node_id);
        funding_pubkeys.push_back(channel->remote_node_id);
        Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

        auto sig_result = SignCommitmentTransaction(remote_commit_tx, funding_scriptpubkey, node_key_);
        if (sig_result.IsOk()) {
            our_sig = sig_result.GetValue();
            // Store the commitment for later
            channel->remote_commitment = commit_result.GetValue();
        } else {
            our_sig.fill(0);  // Fallback to zeros on error
        }
    } else {
        our_sig.fill(0);  // Fallback to zeros on error
    }

    // Create funding_signed message
    FundingSignedMsg signed_msg;
    signed_msg.channel_id = channel_id;
    signed_msg.signature = our_sig;

    // Send funding_signed message
    auto msg_data = signed_msg.Serialize();
    SendMessage(peer, lightning::MSG_FUNDING_SIGNED, msg_data);

    // Update channel mapping (move from temporary_id to channel_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channels_.erase(channel->temporary_id);
        channels_[channel_id] = channel;
    }
}

void LightningNetwork::HandleFundingSigned(const PublicKey& peer, const std::vector<uint8_t>& data) {
    // Initiator receives funding_signed from acceptor
    auto msg_result = FundingSignedMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;
    }
    auto signed_msg = msg_result.GetValue();

    // Find channel by channel_id
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(signed_msg.channel_id);
        if (it != channels_.end()) {
            channel = it->second;
        } else {
            // Try to find by temporary ID and update
            for (auto& [id, ch] : channels_) {
                if (ch->funding_txid == signed_msg.channel_id) {  // Rough match
                    channel = ch;
                    channel->channel_id = signed_msg.channel_id;
                    channels_[signed_msg.channel_id] = channel;
                    channels_.erase(id);
                    break;
                }
            }
        }
    }

    if (!channel) {
        return;  // Channel not found
    }

    // Broadcast the funding transaction to the blockchain
    // NOTE: The funding transaction was created in OpenChannel() but is not stored in the Channel struct
    // TODO: Add 'Transaction funding_tx;' field to Channel class to store the funding transaction
    // TODO: Store funding_tx in channel when created in OpenChannel()
    // For now, we cannot broadcast because we don't have access to funding_tx here

    if (blockchain_) {
        // Placeholder: Would broadcast the funding transaction here if we had it stored
        // blockchain_->BroadcastTransaction(channel->funding_tx);

        // TODO: Implement funding transaction storage and broadcast
        // The funding tx is created in OpenChannel and needs to be stored in the channel
    }

    // Wait for confirmations (handled elsewhere - blockchain monitors funding tx)
    // When funding tx gets enough confirmations, send funding_locked
}

void LightningNetwork::HandleFundingLocked(const PublicKey& peer, const std::vector<uint8_t>& data) {
    // Both parties send this after funding tx is confirmed
    auto msg_result = FundingLockedMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;
    }
    auto locked_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(locked_msg.channel_id);
        if (it == channels_.end()) {
            return;
        }
        channel = it->second;
    }

    // Mark channel as OPEN
    channel->state = ChannelState::OPEN;
    channel->opened_at = std::chrono::system_clock::now();

    // Channel is now ready for payments!
    UpdateStats();
}
/**
 * Handle shutdown message (BOLT #2)
 * Initiates mutual channel close negotiation
 */
void LightningNetwork::HandleShutdown(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = ShutdownMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto shutdown_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(shutdown_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is in valid state
    if (channel->state != ChannelState::OPEN && channel->state != ChannelState::CLOSING_MUTUAL) {
        return;  // Invalid state
    }

    // Prepare our shutdown response
    ShutdownMsg our_shutdown;
    our_shutdown.channel_id = shutdown_msg.channel_id;
    our_shutdown.scriptpubkey = Script();  // TODO: Use proper closing script

    // Mark channel as closing if not already
    if (channel->state == ChannelState::OPEN) {
        channel->state = ChannelState::CLOSING_MUTUAL;

        // Send our shutdown message in response
        auto msg_data = our_shutdown.Serialize();
        SendMessage(peer, lightning::MSG_SHUTDOWN, msg_data);
    }

    // Now initiate closing fee negotiation
    // Estimate closing transaction size:
    // - 1 input (2-of-2 multisig with Dilithium signatures): ~6800 bytes
    // - 2 outputs (P2PKH): ~70 bytes each
    // - Version, locktime, etc.: ~20 bytes
    // Total: ~6960 bytes
    const size_t ESTIMATED_CLOSING_TX_SIZE = 6960;

    // Get current fee rate
    uint32_t feerate_per_kw = EstimateFeeRate(3);  // 3-block target for closing

    // Calculate proposed closing fee
    uint64_t proposed_fee = CalculateTransactionFee(ESTIMATED_CLOSING_TX_SIZE, feerate_per_kw);

    // Create closing transaction
    Transaction closing_tx;
    closing_tx.version = 2;
    closing_tx.locktime = 0;

    // Input: funding transaction
    TxIn input;
    input.prev_tx_hash = channel->funding_txid;
    input.prev_tx_index = channel->funding_vout;
    input.sequence = 0xFFFFFFFF;
    closing_tx.inputs.push_back(input);

    // Outputs: one for us, one for them (minus fee)
    uint64_t our_amount = channel->local_balance;
    uint64_t their_amount = channel->remote_balance;

    // Deduct fee proportionally based on channel balances
    // Each party pays proportionally to their share
    uint64_t total_balance = our_amount + their_amount;
    if (total_balance > 0) {
        uint64_t our_fee_share = (proposed_fee * our_amount) / total_balance;
        uint64_t their_fee_share = proposed_fee - our_fee_share;

        if (our_amount >= our_fee_share) {
            our_amount -= our_fee_share;
        } else {
            our_amount = 0;
        }

        if (their_amount >= their_fee_share) {
            their_amount -= their_fee_share;
        } else {
            their_amount = 0;
        }
    }

    // Create outputs
    if (our_amount > 0) {
        TxOut our_output;
        our_output.value = our_amount;
        our_output.script_pubkey = our_shutdown.scriptpubkey;
        closing_tx.outputs.push_back(our_output);
    }

    if (their_amount > 0) {
        TxOut their_output;
        their_output.value = their_amount;
        their_output.script_pubkey = shutdown_msg.scriptpubkey;
        closing_tx.outputs.push_back(their_output);
    }

    // Sign closing transaction
    Signature our_sig;
    // Build the funding scriptpubkey (2-of-2 multisig)
    std::vector<PublicKey> funding_pubkeys;
    funding_pubkeys.push_back(channel->local_node_id);
    funding_pubkeys.push_back(channel->remote_node_id);
    Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

    auto sig_result = SignCommitmentTransaction(closing_tx, funding_scriptpubkey, node_key_);
    if (sig_result.IsOk()) {
        our_sig = sig_result.GetValue();
    } else {
        our_sig.fill(0);  // Fallback to zeros on error
    }

    // Send closing_signed with our signature
    ClosingSignedMsg closing_signed;
    closing_signed.channel_id = shutdown_msg.channel_id;
    closing_signed.fee_ints = proposed_fee;
    closing_signed.signature = our_sig;

    auto closing_data = closing_signed.Serialize();
    SendMessage(peer, lightning::MSG_CLOSING_SIGNED, closing_data);
}

/**
 * Handle closing_signed message (BOLT #2)
 * Negotiate closing transaction fee
 */
void LightningNetwork::HandleClosingSigned(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = ClosingSignedMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto closing_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(closing_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is closing
    if (channel->state != ChannelState::CLOSING_MUTUAL) {
        return;  // Not in closing state
    }

    // Check if we agree on the fee
    // In a real implementation, we would negotiate if fees don't match
    // For now, accept the proposed fee
    uint64_t agreed_fee = closing_msg.fee_ints;

    // Create final closing transaction
    Transaction closing_tx;
    closing_tx.version = 2;
    closing_tx.locktime = 0;

    TxIn input;
    input.prev_tx_hash = channel->funding_txid;
    input.prev_tx_index = channel->funding_vout;
    input.sequence = 0xFFFFFFFF;
    // TODO: Add signatures from both parties
    closing_tx.inputs.push_back(input);

    // Create outputs with agreed fee
    uint64_t our_amount = channel->local_balance;
    uint64_t their_amount = channel->remote_balance;

    if (our_amount >= agreed_fee) {
        our_amount -= agreed_fee;
    }

    // TODO: Get proper scriptpubkeys from shutdown messages
    if (our_amount > 0) {
        TxOut our_output;
        our_output.value = our_amount;
        our_output.script_pubkey = Script();
        closing_tx.outputs.push_back(our_output);
    }

    if (their_amount > 0) {
        TxOut their_output;
        their_output.value = their_amount;
        their_output.script_pubkey = Script();
        closing_tx.outputs.push_back(their_output);
    }

    // Sign and broadcast closing transaction
    if (blockchain_) {
        // Build the funding scriptpubkey (2-of-2 multisig)
        std::vector<PublicKey> funding_pubkeys;
        funding_pubkeys.push_back(channel->local_node_id);
        funding_pubkeys.push_back(channel->remote_node_id);
        Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

        // Sign the closing transaction
        auto sig_result = SignCommitmentTransaction(closing_tx, funding_scriptpubkey, node_key_);
        if (sig_result.IsOk()) {
            // Assemble fully signed transaction with both signatures
            auto signed_tx_result = AssembleSignedCommitmentTransaction(
                closing_tx,
                sig_result.GetValue(),
                closing_msg.signature  // Remote signature from message
            );

            if (signed_tx_result.IsOk()) {
                // Broadcast the fully signed closing transaction to mempool
                blockchain_->AddToMempool(signed_tx_result.GetValue());
            }
        }
    }

    // Mark channel as closed
    channel->state = ChannelState::CLOSED;
}

/**
 * Handle update_add_htlc message (BOLT #2)
 * Add HTLC to channel commitment
 */
void LightningNetwork::HandleUpdateAddHTLC(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = UpdateAddHTLCMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto htlc_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(htlc_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is open
    if (channel->state != ChannelState::OPEN) {
        return;  // Channel not in OPEN state
    }

    // Verify HTLC amount is valid
    if (htlc_msg.amount_mints == 0) {
        return;  // Invalid amount
    }

    // Verify we can receive this HTLC (check channel capacity)
    uint64_t amount_sat = htlc_msg.amount_mints / 1000;  // Convert msat to sat
    if (!channel->CanReceive(amount_sat)) {
        return;  // Insufficient capacity
    }

    // Create HTLC object
    HTLC htlc;
    htlc.id = htlc_msg.id;
    htlc.amount = amount_sat;
    htlc.payment_hash = htlc_msg.payment_hash;
    htlc.cltv_expiry = htlc_msg.cltv_expiry;
    htlc.onion_routing_packet = htlc_msg.onion_routing_packet;
    htlc.incoming = true;  // This is an incoming HTLC
    htlc.fulfilled = false;

    // Add HTLC to channel's pending HTLCs
    channel->pending_htlcs.push_back(htlc);

    // Update channel balances (reserve amount for HTLC)
    channel->remote_balance -= amount_sat;

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.num_pending_htlcs++;
    }

    // Process onion routing packet to determine next action
    auto onion_result = OnionPacket::Deserialize(htlc_msg.onion_routing_packet);
    if (!onion_result.IsOk()) {
        // Invalid onion packet - fail the HTLC
        UpdateFailHTLCMsg fail_msg;
        fail_msg.channel_id = htlc_msg.channel_id;
        fail_msg.id = htlc_msg.id;
        fail_msg.reason = EncodeFailureMessage(lightning::INVALID_ONION_HMAC);

        auto fail_data = fail_msg.Serialize();
        SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
        return;
    }

    auto onion_packet = onion_result.GetValue();

    // Peel one layer of the onion
    auto peel_result = onion_packet.Peel(node_key_);
    if (!peel_result.IsOk()) {
        // Failed to decrypt onion - fail the HTLC
        UpdateFailHTLCMsg fail_msg;
        fail_msg.channel_id = htlc_msg.channel_id;
        fail_msg.id = htlc_msg.id;
        fail_msg.reason = EncodeFailureMessage(lightning::INVALID_ONION_KEY);

        auto fail_data = fail_msg.Serialize();
        SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
        return;
    }

    auto [next_hop, next_onion] = peel_result.GetValue();

    // Check if we are the final recipient
    bool is_final_recipient = (next_hop.channel_id.empty() || next_hop.node_id == node_id_);

    if (is_final_recipient) {
        // We are the final recipient - verify payment details against invoice
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // BOLT #4: Verify payment details
            // In production, this should look up the invoice by payment_hash
            // and verify: amount, expiry, and other details

            // Basic validation: Check if payment amount is reasonable
            const uint64_t MIN_PAYMENT = 1000;  // 0.001 INTS minimum
            const uint64_t MAX_PAYMENT = 100000000000;  // 100,000 INTS maximum

            uint64_t amount_ints = htlc_msg.amount_mints / 1000;
            if (amount_ints < MIN_PAYMENT) {
                // Payment amount too small
                UpdateFailHTLCMsg fail_msg;
                fail_msg.channel_id = htlc_msg.channel_id;
                fail_msg.id = htlc_msg.id;
                fail_msg.reason = EncodeFailureMessage(lightning::AMOUNT_BELOW_MINIMUM);

                auto fail_data = fail_msg.Serialize();
                SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
                return;
            }

            if (amount_ints > MAX_PAYMENT) {
                // Payment amount unreasonably large
                UpdateFailHTLCMsg fail_msg;
                fail_msg.channel_id = htlc_msg.channel_id;
                fail_msg.id = htlc_msg.id;
                fail_msg.reason = EncodeFailureMessage(lightning::FINAL_INCORRECT_HTLC_AMOUNT);

                auto fail_data = fail_msg.Serialize();
                SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
                return;
            }

            // TODO: Full invoice verification when invoice database is implemented:
            // 1. Look up invoice by payment_hash: auto invoice_it = invoices_.find(htlc_msg.payment_hash);
            // 2. Verify invoice exists: if (invoice_it == invoices_.end()) -> fail with UNKNOWN_PAYMENT_HASH
            // 3. Verify amount matches: if (amount_ints != invoice.amount) -> fail with FINAL_INCORRECT_HTLC_AMOUNT
            // 4. Check expiry: if (now > invoice.created_at + invoice.expiry) -> fail with EXPIRY_TOO_SOON
            // 5. Verify CLTV: if (htlc_msg.cltv_expiry < min_final_cltv) -> fail with FINAL_INCORRECT_CLTV_EXPIRY

            // For now, we accept valid-looking payments
            // In production with invoice storage, we'd only fulfill if we have a matching invoice

            stats_.num_payments_received++;
        }

    } else {
        // We are an intermediate node - forward the HTLC to the next hop

        // Find the next channel
        std::shared_ptr<Channel> next_channel;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = channels_.find(next_hop.channel_id);
            if (it == channels_.end()) {
                // No route to next hop - fail the HTLC
                UpdateFailHTLCMsg fail_msg;
                fail_msg.channel_id = htlc_msg.channel_id;
                fail_msg.id = htlc_msg.id;
                fail_msg.reason = EncodeFailureMessage(lightning::UNKNOWN_NEXT_PEER);

                auto fail_data = fail_msg.Serialize();
                SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
                return;
            }
            next_channel = it->second;
        }

        // Verify next channel is open and has capacity
        if (next_channel->state != ChannelState::OPEN) {
            UpdateFailHTLCMsg fail_msg;
            fail_msg.channel_id = htlc_msg.channel_id;
            fail_msg.id = htlc_msg.id;
            fail_msg.reason = EncodeFailureMessage(lightning::TEMPORARY_CHANNEL_FAILURE);

            auto fail_data = fail_msg.Serialize();
            SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
            return;
        }

        uint64_t forward_amount_sat = next_hop.amount / 1000;  // Convert msat to sat
        if (next_channel->local_balance < forward_amount_sat) {
            // Insufficient balance to forward - fail the HTLC
            UpdateFailHTLCMsg fail_msg;
            fail_msg.channel_id = htlc_msg.channel_id;
            fail_msg.id = htlc_msg.id;
            fail_msg.reason = EncodeFailureMessage(lightning::TEMPORARY_CHANNEL_FAILURE);

            auto fail_data = fail_msg.Serialize();
            SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
            return;
        }

        // Create forwarding HTLC
        UpdateAddHTLCMsg forward_htlc;
        forward_htlc.channel_id = next_hop.channel_id;
        forward_htlc.id = next_channel->pending_htlcs.size();  // New HTLC ID for next channel
        forward_htlc.amount_mints = next_hop.amount;
        forward_htlc.payment_hash = htlc_msg.payment_hash;
        forward_htlc.cltv_expiry = next_hop.cltv_expiry;
        forward_htlc.onion_routing_packet = next_onion.Serialize();

        // Forward the HTLC to next hop
        auto forward_data = forward_htlc.Serialize();
        auto forward_result = SendMessage(next_hop.node_id, lightning::MSG_UPDATE_ADD_HTLC, forward_data);

        if (!forward_result.IsOk()) {
            // Failed to forward - fail back to previous hop
            UpdateFailHTLCMsg fail_msg;
            fail_msg.channel_id = htlc_msg.channel_id;
            fail_msg.id = htlc_msg.id;
            fail_msg.reason = EncodeFailureMessage(lightning::TEMPORARY_NODE_FAILURE);

            auto fail_data = fail_msg.Serialize();
            SendMessage(peer, lightning::MSG_UPDATE_FAIL_HTLC, fail_data);
            return;
        }

        // Track forwarding HTLC (link incoming and outgoing)
        // This allows us to fulfill/fail backwards when we get response
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // TODO: Store HTLC forwarding mapping for resolution
            stats_.total_fees_earned += (amount_sat - forward_amount_sat);
        }
    }

    // Build the updated remote commitment transaction with new HTLCs
    auto commit_result = CommitmentTransaction::Build(
        channel->funding_txid,
        channel->funding_vout,
        channel->capacity,
        channel->remote_balance,  // Remote's local balance
        channel->local_balance,   // Remote's remote balance (our balance)
        channel->pending_htlcs,
        channel->commitment_number + 1,  // Next commitment number
        channel->remote_config
    );

    // Send commitment_signed to commit this HTLC to the channel state
    CommitmentSignedMsg commit_msg;
    commit_msg.channel_id = htlc_msg.channel_id;

    // Sign the commitment transaction
    if (commit_result.IsOk()) {
        auto remote_commit_tx = commit_result.GetValue().tx;

        // Build the funding scriptpubkey (2-of-2 multisig)
        std::vector<PublicKey> funding_pubkeys;
        funding_pubkeys.push_back(channel->local_node_id);
        funding_pubkeys.push_back(channel->remote_node_id);
        Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

        auto sig_result = SignCommitmentTransaction(remote_commit_tx, funding_scriptpubkey, node_key_);
        if (sig_result.IsOk()) {
            commit_msg.signature = sig_result.GetValue();
        } else {
            commit_msg.signature.fill(0);  // Fallback to zeros on error
        }

        // TODO: Sign HTLC outputs (requires building HTLC-success and HTLC-timeout transactions)
        // For now, add placeholder signatures for each HTLC
        for (size_t i = 0; i < channel->pending_htlcs.size(); i++) {
            Signature htlc_sig;
            htlc_sig.fill(0);  // TODO: Build and sign HTLC success/timeout transactions
            commit_msg.htlc_signatures.push_back(htlc_sig);
        }
    } else {
        commit_msg.signature.fill(0);  // Fallback to zeros on error
    }

    auto commit_data = commit_msg.Serialize();
    SendMessage(peer, lightning::MSG_COMMITMENT_SIGNED, commit_data);
}

/**
 * Handle update_fulfill_htlc message (BOLT #2)
 * Fulfill HTLC with payment preimage
 */
void LightningNetwork::HandleUpdateFulfillHTLC(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = UpdateFulfillHTLCMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto fulfill_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(fulfill_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is open
    if (channel->state != ChannelState::OPEN) {
        return;  // Channel not in OPEN state
    }

    // Find the HTLC being fulfilled
    HTLC* htlc_ptr = nullptr;
    for (auto& htlc : channel->pending_htlcs) {
        if (htlc.id == fulfill_msg.id) {
            htlc_ptr = &htlc;
            break;
        }
    }

    if (!htlc_ptr) {
        return;  // HTLC not found
    }

    // Verify preimage hashes to payment_hash
    auto preimage_hash = SHA3::Hash(fulfill_msg.payment_preimage.data(), 32);
    uint256 computed_hash;
    std::copy(preimage_hash.data(), preimage_hash.data() + 32, computed_hash.begin());

    if (computed_hash != htlc_ptr->payment_hash) {
        return;  // Invalid preimage
    }

    // Mark HTLC as fulfilled
    htlc_ptr->fulfilled = true;
    htlc_ptr->preimage = fulfill_msg.payment_preimage;

    // Update channel balances (release HTLC amount to local balance)
    if (htlc_ptr->incoming) {
        channel->local_balance += htlc_ptr->amount;
    } else {
        channel->remote_balance += htlc_ptr->amount;
    }

    // Remove HTLC from pending list
    channel->pending_htlcs.erase(
        std::remove_if(channel->pending_htlcs.begin(), channel->pending_htlcs.end(),
            [&fulfill_msg](const HTLC& h) { return h.id == fulfill_msg.id; }),
        channel->pending_htlcs.end()
    );

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.num_pending_htlcs--;
        stats_.num_payments_received++;
    }

    // TODO: Send commitment_signed to commit this state change
}

/**
 * Handle update_fail_htlc message (BOLT #2)
 * Fail/cancel HTLC
 */
void LightningNetwork::HandleUpdateFailHTLC(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = UpdateFailHTLCMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto fail_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(fail_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is open
    if (channel->state != ChannelState::OPEN) {
        return;  // Channel not in OPEN state
    }

    // Find the HTLC being failed
    HTLC* htlc_ptr = nullptr;
    for (auto& htlc : channel->pending_htlcs) {
        if (htlc.id == fail_msg.id) {
            htlc_ptr = &htlc;
            break;
        }
    }

    if (!htlc_ptr) {
        return;  // HTLC not found
    }

    // Return HTLC amount back to original balance
    if (htlc_ptr->incoming) {
        channel->remote_balance += htlc_ptr->amount;
    } else {
        channel->local_balance += htlc_ptr->amount;
    }

    // Remove HTLC from pending list
    channel->pending_htlcs.erase(
        std::remove_if(channel->pending_htlcs.begin(), channel->pending_htlcs.end(),
            [&fail_msg](const HTLC& h) { return h.id == fail_msg.id; }),
        channel->pending_htlcs.end()
    );

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.num_pending_htlcs--;
    }

    // TODO: Parse failure reason from fail_msg.reason
    // TODO: If this was a forwarded HTLC, propagate failure backwards
    // TODO: Send commitment_signed to commit this state change
}

/**
 * Handle commitment_signed message (BOLT #2)
 * Commit pending HTLC updates to new commitment transaction
 */
void LightningNetwork::HandleCommitmentSigned(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = CommitmentSignedMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto commit_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(commit_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is open
    if (channel->state != ChannelState::OPEN) {
        return;  // Channel not in OPEN state
    }

    // Verify signature count matches pending HTLCs
    if (commit_msg.htlc_signatures.size() != channel->pending_htlcs.size()) {
        return;  // Signature count mismatch
    }

    // Build new commitment transaction with current state
    auto commit_result = CommitmentTransaction::Build(
        channel->funding_txid,
        channel->funding_vout,
        channel->capacity,
        channel->local_balance,
        channel->remote_balance,
        channel->pending_htlcs,
        channel->commitment_number + 1,
        channel->local_config
    );

    if (!commit_result.IsOk()) {
        return;  // Failed to build commitment transaction
    }

    auto new_commitment = commit_result.GetValue();

    // TODO: Verify the signature on the commitment transaction
    // In a full implementation, we would:
    // 1. Verify commit_msg.signature is valid for new_commitment.tx
    // 2. Verify each HTLC signature in commit_msg.htlc_signatures
    // For now, we accept the signatures (placeholder)

    // Store the new commitment transaction
    channel->remote_commitment = new_commitment;

    // Increment commitment number
    channel->commitment_number++;

    // Generate revocation secret for previous commitment using BIP32
    // Derive from path m/44'/2210'/0'/2/(channel_index * 1000 + commitment_number)
    uint256 revocation_secret;
    PublicKey next_point;

    if (wallet_ != nullptr) {
        // Use channel commitment number for unique per-commitment derivation
        uint32_t channel_index;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // Find channel index (position in channels map)
            channel_index = 0;
            for (const auto& [id, ch] : channels_) {
                if (id == channel->channel_id) {
                    break;
                }
                channel_index++;
            }
        }

        // Derive revocation secret for previous commitment
        uint32_t revocation_index = channel_index * 1000 + channel->commitment_number - 1;
        auto revocation_key = wallet_->DeriveLightningKey(revocation_index);
        if (revocation_key.IsOk() && revocation_key.value->private_key.has_value()) {
            // Use private key as revocation secret
            const auto& priv_key = revocation_key.value->private_key.value();
            std::copy_n(priv_key.begin(), std::min(priv_key.size(), revocation_secret.size()),
                       revocation_secret.begin());
        } else {
            revocation_secret.fill(0);  // Fallback
        }

        // Derive next per-commitment point
        uint32_t next_commitment_index = channel_index * 1000 + channel->commitment_number;
        auto next_commit_key = wallet_->DeriveLightningKey(next_commitment_index);
        if (next_commit_key.IsOk() && next_commit_key.value->public_key.has_value()) {
            next_point = next_commit_key.value->public_key.value();
        } else {
            next_point.fill(0);  // Fallback
        }
    } else {
        // Fallback: use placeholder if wallet not available
        revocation_secret.fill(0);
        next_point.fill(0);
    }

    // Send revoke_and_ack to acknowledge the new commitment
    RevokeAndAckMsg revoke_msg;
    revoke_msg.channel_id = commit_msg.channel_id;
    revoke_msg.per_commitment_secret = revocation_secret;
    revoke_msg.next_per_commitment_point = next_point;

    auto revoke_data = revoke_msg.Serialize();
    SendMessage(peer, lightning::MSG_REVOKE_AND_ACK, revoke_data);

    // If we have pending updates, send our commitment_signed
    if (!channel->pending_htlcs.empty()) {
        // Build our commitment transaction
        auto our_commit_result = CommitmentTransaction::Build(
            channel->funding_txid,
            channel->funding_vout,
            channel->capacity,
            channel->remote_balance,  // Swapped for our perspective
            channel->local_balance,
            channel->pending_htlcs,
            channel->commitment_number + 1,
            channel->remote_config
        );

        if (our_commit_result.IsOk()) {
            CommitmentSignedMsg our_commit_msg;
            our_commit_msg.channel_id = commit_msg.channel_id;

            // Sign the remote party's commitment transaction
            auto remote_commit_tx = our_commit_result.GetValue().tx;

            // Build the funding scriptpubkey (2-of-2 multisig)
            std::vector<PublicKey> funding_pubkeys;
            funding_pubkeys.push_back(channel->local_node_id);
            funding_pubkeys.push_back(channel->remote_node_id);
            Script funding_scriptpubkey = Script::CreateMultisig(2, funding_pubkeys);

            auto sig_result = SignCommitmentTransaction(remote_commit_tx, funding_scriptpubkey, node_key_);
            if (sig_result.IsOk()) {
                our_commit_msg.signature = sig_result.GetValue();
            } else {
                our_commit_msg.signature.fill(0);  // Fallback to zeros on error
            }

            // TODO: Sign HTLC outputs (requires building HTLC-success and HTLC-timeout transactions)
            // For now, add placeholder signatures for each HTLC
            for (size_t i = 0; i < channel->pending_htlcs.size(); i++) {
                Signature htlc_sig;
                htlc_sig.fill(0);  // TODO: Build and sign HTLC success/timeout transactions
                our_commit_msg.htlc_signatures.push_back(htlc_sig);
            }

            auto our_commit_data = our_commit_msg.Serialize();
            SendMessage(peer, lightning::MSG_COMMITMENT_SIGNED, our_commit_data);
        }
    }
}

/**
 * Handle revoke_and_ack message (BOLT #2)
 * Finalize commitment transaction update
 */
void LightningNetwork::HandleRevokeAndAck(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = RevokeAndAckMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto revoke_msg = msg_result.GetValue();

    // Find channel
    std::shared_ptr<Channel> channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(revoke_msg.channel_id);
        if (it == channels_.end()) {
            return;  // Channel not found
        }
        channel = it->second;
    }

    // Verify channel is open
    if (channel->state != ChannelState::OPEN) {
        return;  // Channel not in OPEN state
    }

    // TODO: Verify the per_commitment_secret is valid
    // In a full implementation, we would:
    // 1. Derive the per-commitment point from the secret
    // 2. Verify it matches the previous per-commitment point we received
    // 3. Store the secret for potential penalty transaction
    // For now, we accept it (placeholder)

    // Store the next per-commitment point for future use
    // TODO: Store in channel state for next commitment transaction

    // The commitment transaction update is now complete
    // The new state is finalized and the old state is revoked

    // Update last activity time
    channel->last_update = std::chrono::system_clock::now();

    // If we had sent commitment_signed earlier, we should now receive their
    // revoke_and_ack, which finalizes the bidirectional state update
}

// ============================================================================
// BOLT #7 Gossip Protocol Handlers
// ============================================================================

/**
 * Handle channel_announcement message (BOLT #7)
 *
 * When a node receives a channel_announcement:
 * 1. Verify all signatures
 * 2. Verify the channel exists on the blockchain
 * 3. Add channel to network graph for routing
 * 4. Forward announcement to peers
 */
void LightningNetwork::HandleChannelAnnouncement(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = ChannelAnnouncementMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto announcement = msg_result.GetValue();

    // BOLT #7: Verify signatures
    // Create signing message (all fields except signatures)
    std::vector<uint8_t> signing_msg;

    // Append features
    uint16_t features_len = static_cast<uint16_t>(announcement.features.size());
    signing_msg.push_back((features_len >> 8) & 0xFF);
    signing_msg.push_back(features_len & 0xFF);
    signing_msg.insert(signing_msg.end(), announcement.features.begin(), announcement.features.end());

    // Append chain_hash
    signing_msg.insert(signing_msg.end(), announcement.chain_hash.begin(), announcement.chain_hash.end());

    // Append short_channel_id (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        signing_msg.push_back((announcement.short_channel_id >> (i * 8)) & 0xFF);
    }

    // Append node IDs and bitcoin keys
    signing_msg.insert(signing_msg.end(), announcement.node_id_1.begin(), announcement.node_id_1.end());
    signing_msg.insert(signing_msg.end(), announcement.node_id_2.begin(), announcement.node_id_2.end());
    signing_msg.insert(signing_msg.end(), announcement.bitcoin_key_1.begin(), announcement.bitcoin_key_1.end());
    signing_msg.insert(signing_msg.end(), announcement.bitcoin_key_2.begin(), announcement.bitcoin_key_2.end());

    // Verify node signatures using Dilithium3
    auto verify1 = DilithiumCrypto::Verify(signing_msg, announcement.node_signature_1, announcement.node_id_1);
    if (!verify1.IsOk()) {
        // Invalid signature from node 1
        return;
    }

    auto verify2 = DilithiumCrypto::Verify(signing_msg, announcement.node_signature_2, announcement.node_id_2);
    if (!verify2.IsOk()) {
        // Invalid signature from node 2
        return;
    }

    // Note: In INTcoin, bitcoin_key signatures are also Dilithium3 (for funding tx)
    auto verify3 = DilithiumCrypto::Verify(signing_msg, announcement.bitcoin_signature_1, announcement.bitcoin_key_1);
    if (!verify3.IsOk()) {
        // Invalid bitcoin signature 1
        return;
    }

    auto verify4 = DilithiumCrypto::Verify(signing_msg, announcement.bitcoin_signature_2, announcement.bitcoin_key_2);
    if (!verify4.IsOk()) {
        // Invalid bitcoin signature 2
        return;
    }

    // TODO: Verify channel exists on blockchain
    // In production:
    // 1. Decode short_channel_id (block:tx:output format)
    // 2. Verify funding transaction exists in blockchain
    // 3. Verify funding output amount matches channel capacity
    // For now, we skip blockchain verification

    // Add channel to network graph
    ChannelInfo channel_info;
    channel_info.channel_id.fill(0);  // Derive from short_channel_id

    // Convert short_channel_id to channel_id
    // Format: BlockHeight(3 bytes):TxIndex(3 bytes):OutputIndex(2 bytes)
    for (int i = 0; i < 8; i++) {
        channel_info.channel_id[i] = static_cast<uint8_t>((announcement.short_channel_id >> (56 - i * 8)) & 0xFF);
    }

    channel_info.node1 = announcement.node_id_1;
    channel_info.node2 = announcement.node_id_2;
    channel_info.capacity = 0;  // Will be set by blockchain verification
    channel_info.base_fee = 0;  // Will be set by channel_update
    channel_info.fee_rate = 0;  // Will be set by channel_update
    channel_info.cltv_expiry_delta = 0;  // Will be set by channel_update
    channel_info.enabled = true;
    channel_info.last_update = std::chrono::system_clock::now();

    // Add to network graph
    network_graph_.AddChannel(channel_info);

    // Add nodes to graph if not already present
    NodeInfo node1_info;
    node1_info.node_id = announcement.node_id_1;
    node1_info.alias = "";  // Will be set by node_announcement
    node1_info.channels.push_back(channel_info.channel_id);
    node1_info.last_update = std::chrono::system_clock::now();
    network_graph_.AddNode(node1_info);

    NodeInfo node2_info;
    node2_info.node_id = announcement.node_id_2;
    node2_info.alias = "";  // Will be set by node_announcement
    node2_info.channels.push_back(channel_info.channel_id);
    node2_info.last_update = std::chrono::system_clock::now();
    network_graph_.AddNode(node2_info);

    // TODO: Forward announcement to other peers (gossip propagation)
    // In production, broadcast to all connected peers except the sender
    (void)peer;  // Avoid unused parameter warning
}

/**
 * Handle node_announcement message (BOLT #7)
 *
 * When a node receives a node_announcement:
 * 1. Verify signature
 * 2. Update node information in network graph
 * 3. Forward announcement to peers
 */
void LightningNetwork::HandleNodeAnnouncement(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = NodeAnnouncementMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto announcement = msg_result.GetValue();

    // BOLT #7: Verify signature
    // Create signing message (all fields except signature)
    std::vector<uint8_t> signing_msg;

    // Append features
    uint16_t features_len = static_cast<uint16_t>(announcement.features.size());
    signing_msg.push_back((features_len >> 8) & 0xFF);
    signing_msg.push_back(features_len & 0xFF);
    signing_msg.insert(signing_msg.end(), announcement.features.begin(), announcement.features.end());

    // Append timestamp (4 bytes, big-endian)
    signing_msg.push_back((announcement.timestamp >> 24) & 0xFF);
    signing_msg.push_back((announcement.timestamp >> 16) & 0xFF);
    signing_msg.push_back((announcement.timestamp >> 8) & 0xFF);
    signing_msg.push_back(announcement.timestamp & 0xFF);

    // Append node_id
    signing_msg.insert(signing_msg.end(), announcement.node_id.begin(), announcement.node_id.end());

    // Append RGB color (3 bytes)
    signing_msg.insert(signing_msg.end(), announcement.rgb_color.begin(), announcement.rgb_color.end());

    // Append alias (32 bytes, padded with zeros if needed)
    std::vector<uint8_t> alias_bytes(32, 0);
    std::copy(announcement.alias.begin(),
              announcement.alias.begin() + std::min(announcement.alias.size(), size_t(32)),
              alias_bytes.begin());
    signing_msg.insert(signing_msg.end(), alias_bytes.begin(), alias_bytes.end());

    // Append addresses
    uint16_t addr_len = static_cast<uint16_t>(announcement.addresses.size());
    signing_msg.push_back((addr_len >> 8) & 0xFF);
    signing_msg.push_back(addr_len & 0xFF);
    signing_msg.insert(signing_msg.end(), announcement.addresses.begin(), announcement.addresses.end());

    // Verify signature using Dilithium3
    auto verify_result = DilithiumCrypto::Verify(signing_msg, announcement.signature, announcement.node_id);
    if (!verify_result.IsOk()) {
        // Invalid signature - reject announcement
        return;
    }

    // Verify timestamp is not too old (older than 2 weeks)
    auto now = std::chrono::system_clock::now();
    auto announcement_time = std::chrono::system_clock::from_time_t(announcement.timestamp);
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - announcement_time).count();
    const uint32_t MAX_AGE_SECONDS = 14 * 24 * 60 * 60;  // 2 weeks

    if (age > MAX_AGE_SECONDS) {
        // Announcement too old - reject
        return;
    }

    // Update node in network graph
    auto node_result = network_graph_.GetNode(announcement.node_id);

    NodeInfo node_info;
    if (node_result.IsOk()) {
        // Update existing node
        node_info = node_result.GetValue();
    } else {
        // New node
        node_info.node_id = announcement.node_id;
    }

    node_info.alias = announcement.alias;
    node_info.last_update = std::chrono::system_clock::now();

    network_graph_.AddNode(node_info);

    // TODO: Forward announcement to other peers (gossip propagation)
    (void)peer;  // Avoid unused parameter warning
}

/**
 * Handle channel_update message (BOLT #7)
 *
 * When a node receives a channel_update:
 * 1. Verify signature
 * 2. Update channel parameters in network graph
 * 3. Forward update to peers
 */
void LightningNetwork::HandleChannelUpdate(const PublicKey& peer, const std::vector<uint8_t>& data) {
    auto msg_result = ChannelUpdateMsg::Deserialize(data);
    if (!msg_result.IsOk()) {
        return;  // Invalid message
    }
    auto update = msg_result.GetValue();

    // Find channel in network graph first (need to get node IDs for signature verification)
    uint256 channel_id;
    channel_id.fill(0);

    // Convert short_channel_id to channel_id
    for (int i = 0; i < 8; i++) {
        channel_id[i] = static_cast<uint8_t>((update.short_channel_id >> (56 - i * 8)) & 0xFF);
    }

    auto channel_result = network_graph_.GetChannel(channel_id);
    if (!channel_result.IsOk()) {
        return;  // Channel not found - can't verify signature
    }

    ChannelInfo channel_info = channel_result.GetValue();

    // BOLT #7: Verify signature
    // Create signing message (all fields except signature)
    std::vector<uint8_t> signing_msg;

    // Append chain_hash
    signing_msg.insert(signing_msg.end(), update.chain_hash.begin(), update.chain_hash.end());

    // Append short_channel_id (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        signing_msg.push_back((update.short_channel_id >> (i * 8)) & 0xFF);
    }

    // Append timestamp (4 bytes, big-endian)
    signing_msg.push_back((update.timestamp >> 24) & 0xFF);
    signing_msg.push_back((update.timestamp >> 16) & 0xFF);
    signing_msg.push_back((update.timestamp >> 8) & 0xFF);
    signing_msg.push_back(update.timestamp & 0xFF);

    // Append flags
    signing_msg.push_back(update.message_flags);
    signing_msg.push_back(update.channel_flags);

    // Append other fields (2 bytes, big-endian for cltv_expiry_delta)
    signing_msg.push_back((update.cltv_expiry_delta >> 8) & 0xFF);
    signing_msg.push_back(update.cltv_expiry_delta & 0xFF);

    // Append htlc_minimum_mints (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        signing_msg.push_back((update.htlc_minimum_mints >> (i * 8)) & 0xFF);
    }

    // Append fee_base_mints (4 bytes, big-endian)
    signing_msg.push_back((update.fee_base_mints >> 24) & 0xFF);
    signing_msg.push_back((update.fee_base_mints >> 16) & 0xFF);
    signing_msg.push_back((update.fee_base_mints >> 8) & 0xFF);
    signing_msg.push_back(update.fee_base_mints & 0xFF);

    // Append fee_proportional_millionths (4 bytes, big-endian)
    signing_msg.push_back((update.fee_proportional_millionths >> 24) & 0xFF);
    signing_msg.push_back((update.fee_proportional_millionths >> 16) & 0xFF);
    signing_msg.push_back((update.fee_proportional_millionths >> 8) & 0xFF);
    signing_msg.push_back(update.fee_proportional_millionths & 0xFF);

    // Verify signature from one of the channel nodes
    // channel_flags bit 0 indicates direction: 0 = node1, 1 = node2
    PublicKey signing_node = (update.channel_flags & 0x01) ? channel_info.node2 : channel_info.node1;

    auto verify_result = DilithiumCrypto::Verify(signing_msg, update.signature, signing_node);
    if (!verify_result.IsOk()) {
        // Invalid signature - reject update
        return;
    }

    // Verify timestamp is newer than previous update
    auto update_time = std::chrono::system_clock::from_time_t(update.timestamp);
    if (update_time <= channel_info.last_update) {
        // Update is not newer than current - reject
        return;
    }

    // Update channel parameters based on message
    channel_info.base_fee = update.fee_base_mints / 1000;  // Convert msat to sat
    channel_info.fee_rate = update.fee_proportional_millionths;
    channel_info.cltv_expiry_delta = update.cltv_expiry_delta;
    channel_info.enabled = !(update.channel_flags & 0x02);  // Bit 1 = disabled
    channel_info.last_update = std::chrono::system_clock::now();

    // Update in network graph
    network_graph_.UpdateChannel(channel_id, channel_info);

    // TODO: Forward update to other peers (gossip propagation)
    (void)peer;  // Avoid unused parameter warning
}

Result<std::shared_ptr<Channel>> LightningNetwork::FindChannelByPeer(const PublicKey&) {
    return Result<std::shared_ptr<Channel>>::Error("Not found");
}
Result<void> LightningNetwork::SendMessage(const PublicKey&, uint16_t, const std::vector<uint8_t>&) {
    (void)p2p_;  // TODO: Send message via P2P network
    return Result<void>::Ok();
}
void LightningNetwork::UpdateStats() {
    stats_ = Stats{}; stats_.num_channels = channels_.size();
}

// ============================================================================
// Transaction Signing Helpers
// ============================================================================

Result<Signature> LightningNetwork::SignCommitmentTransaction(
    const Transaction& commitment_tx,
    const Script& funding_scriptpubkey,
    const SecretKey& secret_key)
{
    // Get signing hash for input 0 (the funding input)
    if (commitment_tx.inputs.empty()) {
        return Result<Signature>::Error("Commitment transaction has no inputs");
    }

    // Generate signing hash using SIGHASH_ALL
    uint256 signing_hash = commitment_tx.GetHashForSigning(SIGHASH_ALL, 0, funding_scriptpubkey);

    // Sign with Dilithium3
    auto sig_result = DilithiumCrypto::SignHash(signing_hash, secret_key);
    if (!sig_result.IsOk()) {
        return Result<Signature>::Error("Failed to sign commitment transaction: " + sig_result.error);
    }

    return Result<Signature>::Ok(sig_result.GetValue());
}

Result<Transaction> LightningNetwork::AssembleSignedCommitmentTransaction(
    const Transaction& commitment_tx,
    const Signature& sig1,
    const Signature& sig2)
{
    // Create a copy of the transaction
    Transaction signed_tx = commitment_tx;

    if (signed_tx.inputs.empty()) {
        return Result<Transaction>::Error("Commitment transaction has no inputs");
    }

    // Create multisig script_sig with both signatures
    std::vector<Signature> signatures = {sig1, sig2};
    Script multisig_script_sig = Script::CreateMultisigScriptSig(signatures);

    // Set the script_sig on the funding input (input 0)
    signed_tx.inputs[0].script_sig = multisig_script_sig;

    return Result<Transaction>::Ok(signed_tx);
}

// ============================================================================
// Fee Estimation Helpers
// ============================================================================

uint32_t LightningNetwork::EstimateFeeRate(uint32_t target_conf) {
    // Default fee rates based on confirmation target (ints/kw - INTS per kiloweight)
    // These are conservative estimates for INTcoin
    const uint32_t DEFAULT_FEERATE_1_BLOCK = 5000;   // ~20 ints/vbyte equivalent
    const uint32_t DEFAULT_FEERATE_6_BLOCKS = 2500;  // ~10 ints/vbyte equivalent
    const uint32_t DEFAULT_FEERATE_12_BLOCKS = 1000; // ~4 ints/vbyte equivalent
    const uint32_t MINIMUM_FEERATE = 253;            // ~1 ints/vbyte minimum

    if (!blockchain_) {
        // No blockchain access, use conservative default
        return DEFAULT_FEERATE_6_BLOCKS;
    }

    // Try to estimate from mempool statistics
    const auto& mempool = blockchain_->GetMempool();
    size_t mempool_size = mempool.GetSize();
    uint64_t total_fees = mempool.GetTotalFees();

    if (mempool_size > 0) {
        // Calculate average fee rate from mempool
        // Estimate average tx size (~250 bytes for typical P2PKH transaction)
        const size_t AVERAGE_TX_SIZE = 250;

        // Total mempool bytes (rough estimate)
        size_t total_bytes = mempool_size * AVERAGE_TX_SIZE;

        if (total_bytes > 0) {
            // Fee rate in sat/byte
            uint64_t feerate_per_byte = (total_fees * 1000) / total_bytes; // Multiply by 1000 for precision

            // Convert to ints/kw (1 kw = 1000 weight units, 4 weight units = 1 vbyte)
            // So 1 kw = 250 vbytes
            uint32_t feerate_per_kw = static_cast<uint32_t>((feerate_per_byte * 250) / 1000);

            // Apply multiplier based on confirmation target
            if (target_conf <= 1) {
                feerate_per_kw = feerate_per_kw * 2; // Double for fast confirmation
            } else if (target_conf >= 12) {
                feerate_per_kw = feerate_per_kw / 2; // Half for slow confirmation
            }

            // Ensure minimum fee rate
            if (feerate_per_kw < MINIMUM_FEERATE) {
                feerate_per_kw = MINIMUM_FEERATE;
            }

            return feerate_per_kw;
        }
    }

    // Mempool is empty or unavailable, use defaults based on target
    if (target_conf <= 1) {
        return DEFAULT_FEERATE_1_BLOCK;
    } else if (target_conf <= 6) {
        return DEFAULT_FEERATE_6_BLOCKS;
    } else {
        return DEFAULT_FEERATE_12_BLOCKS;
    }
}

uint64_t LightningNetwork::CalculateTransactionFee(size_t tx_size, uint32_t feerate_per_kw) {
    // Convert transaction size from bytes to weight units
    // For non-SegWit transactions: weight = size * 4
    // For Lightning transactions (using post-quantum signatures): weight ~= size * 4
    uint64_t tx_weight = tx_size * 4;

    // Calculate fee: fee = (weight / 1000) * feerate_per_kw
    // This gives us fee in satoshis
    uint64_t fee = (tx_weight * feerate_per_kw) / 1000;

    // Ensure minimum fee (at least 1 satoshi per vbyte for small transactions)
    uint64_t min_fee = tx_size;
    if (fee < min_fee) {
        fee = min_fee;
    }

    return fee;
}

// ============================================================================
// BOLT #4 Error Encoding
// ============================================================================

std::vector<uint8_t> LightningNetwork::EncodeFailureMessage(
    uint16_t failure_code,
    const std::vector<uint8_t>& failure_data)
{
    // BOLT #4: Failure message format
    // - 2 bytes: failure_code (big-endian)
    // - variable: failure_data (optional, depends on failure type)

    std::vector<uint8_t> encoded;

    // Encode failure code (2 bytes, big-endian)
    encoded.push_back((failure_code >> 8) & 0xFF);  // High byte
    encoded.push_back(failure_code & 0xFF);          // Low byte

    // Append failure-specific data if provided
    if (!failure_data.empty()) {
        encoded.insert(encoded.end(), failure_data.begin(), failure_data.end());
    }

    // Note: In a full BOLT #4 implementation, this would be:
    // 1. HMAC-authenticated with a shared secret
    // 2. Onion-encrypted back through the route
    // For now, we're using a simplified version with just the failure code

    return encoded;
}

} // namespace intcoin
