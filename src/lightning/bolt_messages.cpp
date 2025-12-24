// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT Message Implementation - Full BOLT Specification

#include "bolt_messages.h"
#include "intcoin/util.h"
#include <cstring>

namespace intcoin {
namespace bolt {

// Helper functions for serialization
namespace {
    void WriteU16(std::vector<uint8_t>& data, uint16_t value) {
        data.push_back((value >> 8) & 0xFF);
        data.push_back(value & 0xFF);
    }

    void WriteU32(std::vector<uint8_t>& data, uint32_t value) {
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back(value & 0xFF);
    }

    void WriteU64(std::vector<uint8_t>& data, uint64_t value) {
        for (int i = 7; i >= 0; i--) {
            data.push_back((value >> (i * 8)) & 0xFF);
        }
    }

    void WriteBytes(std::vector<uint8_t>& data, const std::vector<uint8_t>& bytes) {
        data.insert(data.end(), bytes.begin(), bytes.end());
    }

    void WriteUint256(std::vector<uint8_t>& data, const uint256& value) {
        const uint8_t* bytes = value.data();
        data.insert(data.end(), bytes, bytes + 32);
    }

    uint16_t ReadU16(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + 2 > data.size()) return 0;
        uint16_t value = (static_cast<uint16_t>(data[offset]) << 8) |
                        static_cast<uint16_t>(data[offset + 1]);
        offset += 2;
        return value;
    }

    uint32_t ReadU32(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + 4 > data.size()) return 0;
        uint32_t value = (static_cast<uint32_t>(data[offset]) << 24) |
                        (static_cast<uint32_t>(data[offset + 1]) << 16) |
                        (static_cast<uint32_t>(data[offset + 2]) << 8) |
                        static_cast<uint32_t>(data[offset + 3]);
        offset += 4;
        return value;
    }

    uint64_t ReadU64(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + 8 > data.size()) return 0;
        uint64_t value = 0;
        for (int i = 0; i < 8; i++) {
            value = (value << 8) | static_cast<uint64_t>(data[offset++]);
        }
        return value;
    }

    std::vector<uint8_t> ReadBytes(const std::vector<uint8_t>& data, size_t& offset, size_t length) {
        if (offset + length > data.size()) return {};
        std::vector<uint8_t> result(data.begin() + offset, data.begin() + offset + length);
        offset += length;
        return result;
    }

    uint256 ReadUint256(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + 32 > data.size()) return uint256();
        std::vector<uint8_t> bytes(data.begin() + offset, data.begin() + offset + 32);
        offset += 32;
        uint256 result;
        std::memcpy(result.data(), bytes.data(), 32);
        return result;
    }

    // Dilithium3 Signature helpers (3309 bytes)
    void WriteSignature(std::vector<uint8_t>& data, const Signature& sig) {
        data.insert(data.end(), sig.begin(), sig.end());
    }

    Signature ReadSignature(const std::vector<uint8_t>& data, size_t& offset) {
        Signature sig;
        auto sig_bytes = ReadBytes(data, offset, 3309);
        if (sig_bytes.size() == 3309) {
            std::copy(sig_bytes.begin(), sig_bytes.end(), sig.begin());
        }
        return sig;
    }

    // Dilithium3 PublicKey helpers (1952 bytes)
    void WritePublicKey(std::vector<uint8_t>& data, const PublicKey& key) {
        data.insert(data.end(), key.begin(), key.end());
    }

    PublicKey ReadPublicKey(const std::vector<uint8_t>& data, size_t& offset) {
        PublicKey key;
        auto key_bytes = ReadBytes(data, offset, 1952);
        if (key_bytes.size() == 1952) {
            std::copy(key_bytes.begin(), key_bytes.end(), key.begin());
        }
        return key;
    }
}

// ============================================================================
// TLV and Message Header
// ============================================================================

std::vector<uint8_t> TLVRecord::Serialize() const {
    std::vector<uint8_t> data;
    // Encode type (BigSize)
    if (type < 253) {
        data.push_back(static_cast<uint8_t>(type));
    } else if (type < 65536) {
        data.push_back(253);
        WriteU16(data, static_cast<uint16_t>(type));
    } else {
        data.push_back(254);
        WriteU32(data, static_cast<uint32_t>(type));
    }
    // Encode length (BigSize)
    uint64_t length = value.size();
    if (length < 253) {
        data.push_back(static_cast<uint8_t>(length));
    } else if (length < 65536) {
        data.push_back(253);
        WriteU16(data, static_cast<uint16_t>(length));
    } else {
        data.push_back(254);
        WriteU32(data, static_cast<uint32_t>(length));
    }
    // Encode value
    WriteBytes(data, value);
    return data;
}

Result<TLVRecord> TLVRecord::Deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size()) {
        return Result<TLVRecord>::Error("Insufficient data for TLV record");
    }

    TLVRecord record;

    // Decode type (BigSize)
    uint8_t first = data[offset++];
    if (first < 253) {
        record.type = first;
    } else if (first == 253) {
        record.type = ReadU16(data, offset);
    } else if (first == 254) {
        record.type = ReadU32(data, offset);
    } else {
        record.type = ReadU64(data, offset);
    }

    // Decode length (BigSize)
    if (offset >= data.size()) {
        return Result<TLVRecord>::Error("Insufficient data for TLV length");
    }
    first = data[offset++];
    uint64_t length;
    if (first < 253) {
        length = first;
    } else if (first == 253) {
        length = ReadU16(data, offset);
    } else if (first == 254) {
        length = ReadU32(data, offset);
    } else {
        length = ReadU64(data, offset);
    }

    // Decode value
    record.value = ReadBytes(data, offset, length);
    if (record.value.size() != length) {
        return Result<TLVRecord>::Error("Insufficient data for TLV value");
    }

    return Result<TLVRecord>::Ok(record);
}

std::vector<uint8_t> MessageHeader::Serialize() const {
    std::vector<uint8_t> data;
    WriteU16(data, type);
    WriteU16(data, length);
    return data;
}

Result<MessageHeader> MessageHeader::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < SIZE) {
        return Result<MessageHeader>::Error("Insufficient data for message header");
    }

    MessageHeader header;
    size_t offset = 0;
    header.type = ReadU16(data, offset);
    header.length = ReadU16(data, offset);

    return Result<MessageHeader>::Ok(header);
}

// ============================================================================
// BOLT #1: Base Protocol Messages
// ============================================================================

std::vector<uint8_t> InitMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteU16(data, global_features);
    WriteU16(data, local_features);

    // Encode TLV records
    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<InitMessage> InitMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return Result<InitMessage>::Error("Insufficient data for init message");
    }

    InitMessage msg;
    size_t offset = 0;
    msg.global_features = ReadU16(data, offset);
    msg.local_features = ReadU16(data, offset);

    // Decode TLV records
    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<InitMessage>::Ok(msg);
}

std::vector<uint8_t> ErrorMessage::Serialize() const {
    std::vector<uint8_t> result;
    WriteUint256(result, channel_id);
    WriteU16(result, static_cast<uint16_t>(data.length()));
    for (char c : data) {
        result.push_back(static_cast<uint8_t>(c));
    }
    return result;
}

Result<ErrorMessage> ErrorMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 34) {
        return Result<ErrorMessage>::Error("Insufficient data for error message");
    }

    ErrorMessage msg;
    size_t offset = 0;
    msg.channel_id = ReadUint256(data, offset);
    uint16_t len = ReadU16(data, offset);

    auto bytes = ReadBytes(data, offset, len);
    msg.data = std::string(bytes.begin(), bytes.end());

    return Result<ErrorMessage>::Ok(msg);
}

std::vector<uint8_t> PingMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteU16(data, num_pong_bytes);
    WriteU16(data, static_cast<uint16_t>(ignored.size()));
    WriteBytes(data, ignored);
    return data;
}

Result<PingMessage> PingMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return Result<PingMessage>::Error("Insufficient data for ping message");
    }

    PingMessage msg;
    size_t offset = 0;
    msg.num_pong_bytes = ReadU16(data, offset);
    uint16_t len = ReadU16(data, offset);
    msg.ignored = ReadBytes(data, offset, len);

    return Result<PingMessage>::Ok(msg);
}

std::vector<uint8_t> PongMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteU16(data, static_cast<uint16_t>(ignored.size()));
    WriteBytes(data, ignored);
    return data;
}

Result<PongMessage> PongMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return Result<PongMessage>::Error("Insufficient data for pong message");
    }

    PongMessage msg;
    size_t offset = 0;
    uint16_t len = ReadU16(data, offset);
    msg.ignored = ReadBytes(data, offset, len);

    return Result<PongMessage>::Ok(msg);
}

// ============================================================================
// BOLT #2: Channel Management Messages (Partial - Key Messages)
// ============================================================================

std::vector<uint8_t> OpenChannelMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    WriteUint256(data, temporary_channel_id);
    WriteU64(data, funding_satoshis);
    WriteU64(data, push_msat);
    WriteU64(data, dust_limit_satoshis);
    WriteU64(data, max_htlc_value_in_flight_msat);
    WriteU64(data, channel_reserve_satoshis);
    WriteU64(data, htlc_minimum_msat);
    WriteU32(data, feerate_per_kw);
    WriteU16(data, to_self_delay);
    WriteU16(data, max_accepted_htlcs);

    // Public keys (1952 bytes each for Dilithium3)
    auto keys = {funding_pubkey, revocation_basepoint, payment_basepoint,
                 delayed_payment_basepoint, htlc_basepoint, first_per_commitment_point};
    for (const auto& key : keys) {
        WriteBytes(data, std::vector<uint8_t>(key.begin(), key.end()));
    }

    data.push_back(channel_flags);

    // TLV records
    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<OpenChannelMessage> OpenChannelMessage::Deserialize(const std::vector<uint8_t>& data) {
    // Minimum size check (32+32+8*6+4+2+2+33*6+1 = 315 bytes minimum)
    if (data.size() < 315) {
        return Result<OpenChannelMessage>::Error("Insufficient data for open_channel message");
    }

    OpenChannelMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    msg.temporary_channel_id = ReadUint256(data, offset);
    msg.funding_satoshis = ReadU64(data, offset);
    msg.push_msat = ReadU64(data, offset);
    msg.dust_limit_satoshis = ReadU64(data, offset);
    msg.max_htlc_value_in_flight_msat = ReadU64(data, offset);
    msg.channel_reserve_satoshis = ReadU64(data, offset);
    msg.htlc_minimum_msat = ReadU64(data, offset);
    msg.feerate_per_kw = ReadU32(data, offset);
    msg.to_self_delay = ReadU16(data, offset);
    msg.max_accepted_htlcs = ReadU16(data, offset);

    // Parse public keys (1952 bytes each for Dilithium3)
    auto parse_key = [&](PublicKey& key) {
        auto key_bytes = ReadBytes(data, offset, 1952);
        if (key_bytes.size() == 1952) {
            std::copy(key_bytes.begin(), key_bytes.end(), key.begin());
        }
    };

    parse_key(msg.funding_pubkey);
    parse_key(msg.revocation_basepoint);
    parse_key(msg.payment_basepoint);
    parse_key(msg.delayed_payment_basepoint);
    parse_key(msg.htlc_basepoint);
    parse_key(msg.first_per_commitment_point);

    msg.channel_flags = data[offset++];

    // Parse TLV records
    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<OpenChannelMessage>::Ok(msg);
}

std::vector<uint8_t> UpdateAddHTLCMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, id);
    WriteU64(data, amount_msat);
    WriteUint256(data, payment_hash);
    WriteU32(data, cltv_expiry);
    WriteU16(data, static_cast<uint16_t>(onion_routing_packet.size()));
    WriteBytes(data, onion_routing_packet);
    return data;
}

Result<UpdateAddHTLCMessage> UpdateAddHTLCMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 8 + 32 + 4 + 2) {
        return Result<UpdateAddHTLCMessage>::Error("Insufficient data for update_add_htlc message");
    }

    UpdateAddHTLCMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.id = ReadU64(data, offset);
    msg.amount_msat = ReadU64(data, offset);
    msg.payment_hash = ReadUint256(data, offset);
    msg.cltv_expiry = ReadU32(data, offset);
    uint16_t onion_len = ReadU16(data, offset);
    msg.onion_routing_packet = ReadBytes(data, offset, onion_len);

    return Result<UpdateAddHTLCMessage>::Ok(msg);
}

std::vector<uint8_t> UpdateFulfillHTLCMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, id);
    WriteUint256(data, payment_preimage);
    return data;
}

Result<UpdateFulfillHTLCMessage> UpdateFulfillHTLCMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 32) {
        return Result<UpdateFulfillHTLCMessage>::Error("Insufficient data");
    }

    UpdateFulfillHTLCMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.id = ReadU64(data, offset);
    msg.payment_preimage = ReadUint256(data, offset);

    return Result<UpdateFulfillHTLCMessage>::Ok(msg);
}

std::vector<uint8_t> CommitmentSignedMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteSignature(data, signature);
    WriteU16(data, static_cast<uint16_t>(htlc_signatures.size()));
    for (const auto& sig : htlc_signatures) {
        WriteSignature(data, sig);
    }
    return data;
}

Result<CommitmentSignedMessage> CommitmentSignedMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 64 + 2) {
        return Result<CommitmentSignedMessage>::Error("Insufficient data");
    }

    CommitmentSignedMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.signature = ReadSignature(data, offset);

    uint16_t num_sigs = ReadU16(data, offset);
    for (uint16_t i = 0; i < num_sigs; i++) {
        msg.htlc_signatures.push_back(ReadSignature(data, offset));
    }

    return Result<CommitmentSignedMessage>::Ok(msg);
}

std::vector<uint8_t> RevokeAndAckMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteUint256(data, per_commitment_secret);
    WritePublicKey(data, next_per_commitment_point);
    return data;
}

Result<RevokeAndAckMessage> RevokeAndAckMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 32 + 33) {
        return Result<RevokeAndAckMessage>::Error("Insufficient data");
    }

    RevokeAndAckMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.per_commitment_secret = ReadUint256(data, offset);
    msg.next_per_commitment_point = ReadPublicKey(data, offset);

    return Result<RevokeAndAckMessage>::Ok(msg);
}

// ============================================================================
// BOLT #7: Gossip Messages (Partial - Key Messages)
// ============================================================================

std::vector<uint8_t> ChannelAnnouncementMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteSignature(data, node_signature_1);
    WriteSignature(data, node_signature_2);
    WriteSignature(data, bitcoin_signature_1);
    WriteSignature(data, bitcoin_signature_2);
    WriteU16(data, static_cast<uint16_t>(features.size()));
    WriteBytes(data, features);
    WriteUint256(data, chain_hash);
    WriteU64(data, short_channel_id);
    WritePublicKey(data, node_id_1);
    WritePublicKey(data, node_id_2);
    WritePublicKey(data, bitcoin_key_1);
    WritePublicKey(data, bitcoin_key_2);
    return data;
}

Result<ChannelAnnouncementMessage> ChannelAnnouncementMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 64*4 + 2 + 32 + 8 + 33*4) {
        return Result<ChannelAnnouncementMessage>::Error("Insufficient data");
    }

    ChannelAnnouncementMessage msg;
    size_t offset = 0;

    msg.node_signature_1 = ReadSignature(data, offset);
    msg.node_signature_2 = ReadSignature(data, offset);
    msg.bitcoin_signature_1 = ReadSignature(data, offset);
    msg.bitcoin_signature_2 = ReadSignature(data, offset);

    uint16_t feature_len = ReadU16(data, offset);
    msg.features = ReadBytes(data, offset, feature_len);

    msg.chain_hash = ReadUint256(data, offset);
    msg.short_channel_id = ReadU64(data, offset);

    msg.node_id_1 = ReadPublicKey(data, offset);
    msg.node_id_2 = ReadPublicKey(data, offset);
    msg.bitcoin_key_1 = ReadPublicKey(data, offset);
    msg.bitcoin_key_2 = ReadPublicKey(data, offset);

    return Result<ChannelAnnouncementMessage>::Ok(msg);
}

std::vector<uint8_t> NodeAnnouncementMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteSignature(data, signature);
    WriteU16(data, static_cast<uint16_t>(features.size()));
    WriteBytes(data, features);
    WriteU32(data, timestamp);
    WritePublicKey(data, node_id);
    data.insert(data.end(), rgb_color, rgb_color + 3);

    // Alias (32 bytes, padded with zeros)
    std::vector<uint8_t> alias_bytes(32, 0);
    size_t copy_len = std::min(alias.length(), size_t(32));
    std::memcpy(alias_bytes.data(), alias.data(), copy_len);
    WriteBytes(data, alias_bytes);

    WriteU16(data, static_cast<uint16_t>(addresses.size()));
    for (const auto& addr : addresses) {
        WriteU16(data, static_cast<uint16_t>(addr.size()));
        WriteBytes(data, addr);
    }

    return data;
}

Result<NodeAnnouncementMessage> NodeAnnouncementMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 64 + 2 + 4 + 33 + 3 + 32 + 2) {
        return Result<NodeAnnouncementMessage>::Error("Insufficient data");
    }

    NodeAnnouncementMessage msg;
    size_t offset = 0;

    msg.signature = ReadSignature(data, offset);

    uint16_t feature_len = ReadU16(data, offset);
    msg.features = ReadBytes(data, offset, feature_len);

    msg.timestamp = ReadU32(data, offset);

    msg.node_id = ReadPublicKey(data, offset);

    std::memcpy(msg.rgb_color, &data[offset], 3);
    offset += 3;

    auto alias_bytes = ReadBytes(data, offset, 32);
    msg.alias = std::string(alias_bytes.begin(),
                           std::find(alias_bytes.begin(), alias_bytes.end(), 0));

    uint16_t num_addresses = ReadU16(data, offset);
    for (uint16_t i = 0; i < num_addresses && offset < data.size(); i++) {
        uint16_t addr_len = ReadU16(data, offset);
        auto addr = ReadBytes(data, offset, addr_len);
        msg.addresses.push_back(addr);
    }

    return Result<NodeAnnouncementMessage>::Ok(msg);
}

// ============================================================================
// BOLT #9: Feature Flags
// ============================================================================

FeatureFlags::FeatureFlags() : features_(32, 0) {}  // 256 bits

void FeatureFlags::SetFeature(FeatureBit bit, bool required) {
    uint16_t bit_num = static_cast<uint16_t>(bit);
    if (!required) bit_num |= 1;  // Odd bit = optional, even = required

    size_t byte_pos = bit_num / 8;
    uint8_t bit_pos = bit_num % 8;

    if (byte_pos >= features_.size()) {
        features_.resize(byte_pos + 1, 0);
    }

    features_[byte_pos] |= (1 << bit_pos);
}

bool FeatureFlags::HasFeature(FeatureBit bit) const {
    uint16_t bit_num = static_cast<uint16_t>(bit);
    size_t byte_pos = bit_num / 8;
    uint8_t bit_pos = bit_num % 8;

    if (byte_pos >= features_.size()) return false;
    return (features_[byte_pos] & (1 << bit_pos)) != 0;
}

bool FeatureFlags::IsRequired(FeatureBit bit) const {
    // Even bit numbers are required
    return (static_cast<uint16_t>(bit) % 2) == 0;
}

bool FeatureFlags::IsCompatible(const FeatureFlags& other) const {
    // Check if all required features are supported
    size_t max_size = std::max(features_.size(), other.features_.size());

    for (size_t i = 0; i < max_size; i++) {
        uint8_t our_byte = (i < features_.size()) ? features_[i] : 0;
        uint8_t their_byte = (i < other.features_.size()) ? other.features_[i] : 0;

        // Check even (required) bits
        for (int bit = 0; bit < 8; bit += 2) {
            bool we_require = (our_byte & (1 << bit)) != 0;
            bool they_support = (their_byte & ((1 << bit) | (1 << (bit+1)))) != 0;
            bool they_require = (their_byte & (1 << bit)) != 0;
            bool we_support = (our_byte & ((1 << bit) | (1 << (bit+1)))) != 0;

            if (we_require && !they_support) return false;
            if (they_require && !we_support) return false;
        }
    }

    return true;
}

std::vector<uint8_t> FeatureFlags::Serialize() const {
    // Trim trailing zeros
    size_t last_nonzero = 0;
    for (size_t i = 0; i < features_.size(); i++) {
        if (features_[i] != 0) {
            last_nonzero = i;
        }
    }

    return std::vector<uint8_t>(features_.begin(),
                               features_.begin() + last_nonzero + 1);
}

Result<FeatureFlags> FeatureFlags::Deserialize(const std::vector<uint8_t>& data) {
    FeatureFlags flags;
    flags.features_ = data;
    return Result<FeatureFlags>::Ok(flags);
}

// ============================================================================
// Remaining Channel Management Message Implementations
// ============================================================================

std::vector<uint8_t> AcceptChannelMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, temporary_channel_id);
    WriteU64(data, dust_limit_satoshis);
    WriteU64(data, max_htlc_value_in_flight_msat);
    WriteU64(data, channel_reserve_satoshis);
    WriteU64(data, htlc_minimum_msat);
    WriteU32(data, minimum_depth);
    WriteU16(data, to_self_delay);
    WriteU16(data, max_accepted_htlcs);

    auto keys = {funding_pubkey, revocation_basepoint, payment_basepoint,
                 delayed_payment_basepoint, htlc_basepoint, first_per_commitment_point};
    for (const auto& key : keys) {
        WritePublicKey(data, key);
    }

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<AcceptChannelMessage> AcceptChannelMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8*4 + 4 + 2 + 2 + 33*6) {
        return Result<AcceptChannelMessage>::Error("Insufficient data");
    }

    AcceptChannelMessage msg;
    size_t offset = 0;

    msg.temporary_channel_id = ReadUint256(data, offset);
    msg.dust_limit_satoshis = ReadU64(data, offset);
    msg.max_htlc_value_in_flight_msat = ReadU64(data, offset);
    msg.channel_reserve_satoshis = ReadU64(data, offset);
    msg.htlc_minimum_msat = ReadU64(data, offset);
    msg.minimum_depth = ReadU32(data, offset);
    msg.to_self_delay = ReadU16(data, offset);
    msg.max_accepted_htlcs = ReadU16(data, offset);

    msg.funding_pubkey = ReadPublicKey(data, offset);
    msg.revocation_basepoint = ReadPublicKey(data, offset);
    msg.payment_basepoint = ReadPublicKey(data, offset);
    msg.delayed_payment_basepoint = ReadPublicKey(data, offset);
    msg.htlc_basepoint = ReadPublicKey(data, offset);
    msg.first_per_commitment_point = ReadPublicKey(data, offset);

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<AcceptChannelMessage>::Ok(msg);
}

std::vector<uint8_t> FundingCreatedMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, temporary_channel_id);
    WriteUint256(data, funding_txid);
    WriteU16(data, funding_output_index);
    WriteSignature(data, signature);
    return data;
}

Result<FundingCreatedMessage> FundingCreatedMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 32 + 2 + 64) {
        return Result<FundingCreatedMessage>::Error("Insufficient data");
    }

    FundingCreatedMessage msg;
    size_t offset = 0;

    msg.temporary_channel_id = ReadUint256(data, offset);
    msg.funding_txid = ReadUint256(data, offset);
    msg.funding_output_index = ReadU16(data, offset);
    msg.signature = ReadSignature(data, offset);

    return Result<FundingCreatedMessage>::Ok(msg);
}

std::vector<uint8_t> FundingSignedMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteSignature(data, signature);
    return data;
}

Result<FundingSignedMessage> FundingSignedMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 64) {
        return Result<FundingSignedMessage>::Error("Insufficient data");
    }

    FundingSignedMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.signature = ReadSignature(data, offset);

    return Result<FundingSignedMessage>::Ok(msg);
}

std::vector<uint8_t> FundingLockedMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WritePublicKey(data, next_per_commitment_point);

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<FundingLockedMessage> FundingLockedMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 33) {
        return Result<FundingLockedMessage>::Error("Insufficient data");
    }

    FundingLockedMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.next_per_commitment_point = ReadPublicKey(data, offset);

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<FundingLockedMessage>::Ok(msg);
}

std::vector<uint8_t> ShutdownMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU16(data, static_cast<uint16_t>(scriptpubkey.size()));
    WriteBytes(data, scriptpubkey);
    return data;
}

Result<ShutdownMessage> ShutdownMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 2) {
        return Result<ShutdownMessage>::Error("Insufficient data");
    }

    ShutdownMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    uint16_t script_len = ReadU16(data, offset);
    msg.scriptpubkey = ReadBytes(data, offset, script_len);

    return Result<ShutdownMessage>::Ok(msg);
}

std::vector<uint8_t> ClosingSignedMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, fee_satoshis);
    WriteSignature(data, signature);

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<ClosingSignedMessage> ClosingSignedMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 64) {
        return Result<ClosingSignedMessage>::Error("Insufficient data");
    }

    ClosingSignedMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.fee_satoshis = ReadU64(data, offset);
    msg.signature = ReadSignature(data, offset);

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<ClosingSignedMessage>::Ok(msg);
}

std::vector<uint8_t> UpdateFailHTLCMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, id);
    WriteU16(data, static_cast<uint16_t>(reason.size()));
    WriteBytes(data, reason);
    return data;
}

Result<UpdateFailHTLCMessage> UpdateFailHTLCMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 2) {
        return Result<UpdateFailHTLCMessage>::Error("Insufficient data");
    }

    UpdateFailHTLCMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.id = ReadU64(data, offset);
    uint16_t reason_len = ReadU16(data, offset);
    msg.reason = ReadBytes(data, offset, reason_len);

    return Result<UpdateFailHTLCMessage>::Ok(msg);
}

std::vector<uint8_t> UpdateFeeMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU32(data, feerate_per_kw);
    return data;
}

Result<UpdateFeeMessage> UpdateFeeMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 4) {
        return Result<UpdateFeeMessage>::Error("Insufficient data");
    }

    UpdateFeeMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.feerate_per_kw = ReadU32(data, offset);

    return Result<UpdateFeeMessage>::Ok(msg);
}

std::vector<uint8_t> UpdateFailMalformedHTLCMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, id);
    WriteUint256(data, sha256_of_onion);
    WriteU16(data, failure_code);
    return data;
}

Result<UpdateFailMalformedHTLCMessage> UpdateFailMalformedHTLCMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 32 + 2) {
        return Result<UpdateFailMalformedHTLCMessage>::Error("Insufficient data");
    }

    UpdateFailMalformedHTLCMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.id = ReadU64(data, offset);
    msg.sha256_of_onion = ReadUint256(data, offset);
    msg.failure_code = ReadU16(data, offset);

    return Result<UpdateFailMalformedHTLCMessage>::Ok(msg);
}

std::vector<uint8_t> ChannelReestablishMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, next_commitment_number);
    WriteU64(data, next_revocation_number);
    WriteUint256(data, your_last_per_commitment_secret);
    WritePublicKey(data, my_current_per_commitment_point);
    return data;
}

Result<ChannelReestablishMessage> ChannelReestablishMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 8 + 32 + 33) {
        return Result<ChannelReestablishMessage>::Error("Insufficient data");
    }

    ChannelReestablishMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.next_commitment_number = ReadU64(data, offset);
    msg.next_revocation_number = ReadU64(data, offset);
    msg.your_last_per_commitment_secret = ReadUint256(data, offset);

    msg.my_current_per_commitment_point = ReadPublicKey(data, offset);

    return Result<ChannelReestablishMessage>::Ok(msg);
}

// ============================================================================
// Remaining Gossip Message Implementations
// ============================================================================

std::vector<uint8_t> AnnouncementSignaturesMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, channel_id);
    WriteU64(data, short_channel_id);
    WriteSignature(data, node_signature);
    WriteSignature(data, bitcoin_signature);
    return data;
}

Result<AnnouncementSignaturesMessage> AnnouncementSignaturesMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 8 + 64 + 64) {
        return Result<AnnouncementSignaturesMessage>::Error("Insufficient data");
    }

    AnnouncementSignaturesMessage msg;
    size_t offset = 0;

    msg.channel_id = ReadUint256(data, offset);
    msg.short_channel_id = ReadU64(data, offset);
    msg.node_signature = ReadSignature(data, offset);
    msg.bitcoin_signature = ReadSignature(data, offset);

    return Result<AnnouncementSignaturesMessage>::Ok(msg);
}

std::vector<uint8_t> ChannelUpdateMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteSignature(data, signature);
    WriteUint256(data, chain_hash);
    WriteU64(data, short_channel_id);
    WriteU32(data, timestamp);
    data.push_back(message_flags);
    data.push_back(channel_flags);
    WriteU16(data, cltv_expiry_delta);
    WriteU64(data, htlc_minimum_msat);
    WriteU32(data, fee_base_msat);
    WriteU32(data, fee_proportional_millionths);

    if (htlc_maximum_msat.has_value()) {
        WriteU64(data, htlc_maximum_msat.value());
    }

    return data;
}

Result<ChannelUpdateMessage> ChannelUpdateMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 64 + 32 + 8 + 4 + 1 + 1 + 2 + 8 + 4 + 4) {
        return Result<ChannelUpdateMessage>::Error("Insufficient data");
    }

    ChannelUpdateMessage msg;
    size_t offset = 0;

    msg.signature = ReadSignature(data, offset);
    msg.chain_hash = ReadUint256(data, offset);
    msg.short_channel_id = ReadU64(data, offset);
    msg.timestamp = ReadU32(data, offset);
    msg.message_flags = data[offset++];
    msg.channel_flags = data[offset++];
    msg.cltv_expiry_delta = ReadU16(data, offset);
    msg.htlc_minimum_msat = ReadU64(data, offset);
    msg.fee_base_msat = ReadU32(data, offset);
    msg.fee_proportional_millionths = ReadU32(data, offset);

    if (offset + 8 <= data.size()) {
        msg.htlc_maximum_msat = ReadU64(data, offset);
    }

    return Result<ChannelUpdateMessage>::Ok(msg);
}

std::vector<uint8_t> QueryShortChannelIdsMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    WriteU16(data, static_cast<uint16_t>(short_channel_ids.size()));
    for (uint64_t scid : short_channel_ids) {
        WriteU64(data, scid);
    }

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<QueryShortChannelIdsMessage> QueryShortChannelIdsMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 2) {
        return Result<QueryShortChannelIdsMessage>::Error("Insufficient data");
    }

    QueryShortChannelIdsMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    uint16_t num_ids = ReadU16(data, offset);

    for (uint16_t i = 0; i < num_ids && offset + 8 <= data.size(); i++) {
        msg.short_channel_ids.push_back(ReadU64(data, offset));
    }

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<QueryShortChannelIdsMessage>::Ok(msg);
}

std::vector<uint8_t> ReplyShortChannelIdsEndMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    data.push_back(complete);
    return data;
}

Result<ReplyShortChannelIdsEndMessage> ReplyShortChannelIdsEndMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 1) {
        return Result<ReplyShortChannelIdsEndMessage>::Error("Insufficient data");
    }

    ReplyShortChannelIdsEndMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    msg.complete = data[offset++];

    return Result<ReplyShortChannelIdsEndMessage>::Ok(msg);
}

std::vector<uint8_t> QueryChannelRangeMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    WriteU32(data, first_blocknum);
    WriteU32(data, number_of_blocks);

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<QueryChannelRangeMessage> QueryChannelRangeMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 4 + 4) {
        return Result<QueryChannelRangeMessage>::Error("Insufficient data");
    }

    QueryChannelRangeMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    msg.first_blocknum = ReadU32(data, offset);
    msg.number_of_blocks = ReadU32(data, offset);

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<QueryChannelRangeMessage>::Ok(msg);
}

std::vector<uint8_t> ReplyChannelRangeMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    WriteU32(data, first_blocknum);
    WriteU32(data, number_of_blocks);
    data.push_back(complete);
    WriteU16(data, static_cast<uint16_t>(short_channel_ids.size()));
    for (uint64_t scid : short_channel_ids) {
        WriteU64(data, scid);
    }

    for (const auto& [type, value] : tlv_records) {
        TLVRecord record{type, value};
        WriteBytes(data, record.Serialize());
    }

    return data;
}

Result<ReplyChannelRangeMessage> ReplyChannelRangeMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 4 + 4 + 1 + 2) {
        return Result<ReplyChannelRangeMessage>::Error("Insufficient data");
    }

    ReplyChannelRangeMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    msg.first_blocknum = ReadU32(data, offset);
    msg.number_of_blocks = ReadU32(data, offset);
    msg.complete = data[offset++];

    uint16_t num_ids = ReadU16(data, offset);
    for (uint16_t i = 0; i < num_ids && offset + 8 <= data.size(); i++) {
        msg.short_channel_ids.push_back(ReadU64(data, offset));
    }

    while (offset < data.size()) {
        auto tlv_result = TLVRecord::Deserialize(data, offset);
        if (tlv_result.IsError()) break;
        auto tlv = tlv_result.GetValue();
        msg.tlv_records[tlv.type] = tlv.value;
    }

    return Result<ReplyChannelRangeMessage>::Ok(msg);
}

std::vector<uint8_t> GossipTimestampFilterMessage::Serialize() const {
    std::vector<uint8_t> data;
    WriteUint256(data, chain_hash);
    WriteU32(data, first_timestamp);
    WriteU32(data, timestamp_range);
    return data;
}

Result<GossipTimestampFilterMessage> GossipTimestampFilterMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 4 + 4) {
        return Result<GossipTimestampFilterMessage>::Error("Insufficient data");
    }

    GossipTimestampFilterMessage msg;
    size_t offset = 0;

    msg.chain_hash = ReadUint256(data, offset);
    msg.first_timestamp = ReadU32(data, offset);
    msg.timestamp_range = ReadU32(data, offset);

    return Result<GossipTimestampFilterMessage>::Ok(msg);
}

} // namespace bolt
} // namespace intcoin
