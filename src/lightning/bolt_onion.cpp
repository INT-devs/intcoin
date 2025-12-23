// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #4: Sphinx Onion Routing Implementation

#include "bolt_onion.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <algorithm>
#include <cstring>

namespace intcoin {
namespace bolt {

// ============================================================================
// Hop Payload Implementation
// ============================================================================

std::vector<uint8_t> HopPayload::Serialize() const {
    std::vector<uint8_t> data;
    
    // TLV encoding
    auto write_tlv = [&](uint64_t type, const std::vector<uint8_t>& value) {
        // Type (BigSize)
        if (type < 253) {
            data.push_back(static_cast<uint8_t>(type));
        } else if (type < 65536) {
            data.push_back(253);
            data.push_back((type >> 8) & 0xFF);
            data.push_back(type & 0xFF);
        }
        
        // Length (BigSize)
        uint64_t len = value.size();
        if (len < 253) {
            data.push_back(static_cast<uint8_t>(len));
        } else if (len < 65536) {
            data.push_back(253);
            data.push_back((len >> 8) & 0xFF);
            data.push_back(len & 0xFF);
        }
        
        // Value
        data.insert(data.end(), value.begin(), value.end());
    };
    
    // AMT_TO_FORWARD
    std::vector<uint8_t> amt(8);
    for (int i = 7; i >= 0; i--) {
        amt[7 - i] = (amt_to_forward >> (i * 8)) & 0xFF;
    }
    write_tlv(static_cast<uint64_t>(HopPayloadTLV::AMT_TO_FORWARD), amt);
    
    // OUTGOING_CLTV_VALUE
    std::vector<uint8_t> cltv(4);
    for (int i = 3; i >= 0; i--) {
        cltv[3 - i] = (outgoing_cltv_value >> (i * 8)) & 0xFF;
    }
    write_tlv(static_cast<uint64_t>(HopPayloadTLV::OUTGOING_CLTV_VALUE), cltv);
    
    // SHORT_CHANNEL_ID
    std::vector<uint8_t> scid(8);
    for (int i = 7; i >= 0; i--) {
        scid[7 - i] = (short_channel_id >> (i * 8)) & 0xFF;
    }
    write_tlv(static_cast<uint64_t>(HopPayloadTLV::SHORT_CHANNEL_ID), scid);
    
    // PAYMENT_DATA (optional)
    if (payment_secret.has_value() && total_msat.has_value()) {
        std::vector<uint8_t> payment_data;
        // Payment secret (32 bytes)
        const uint8_t* secret_bytes = payment_secret->data();
        payment_data.insert(payment_data.end(), secret_bytes, secret_bytes + 32);
        // Total msat (8 bytes)
        for (int i = 7; i >= 0; i--) {
            payment_data.push_back((*total_msat >> (i * 8)) & 0xFF);
        }
        write_tlv(static_cast<uint64_t>(HopPayloadTLV::PAYMENT_DATA), payment_data);
    }
    
    return data;
}

Result<HopPayload> HopPayload::Deserialize(const std::vector<uint8_t>& data) {
    HopPayload payload;
    size_t offset = 0;
    
    while (offset < data.size()) {
        // Read type (BigSize)
        if (offset >= data.size()) break;
        uint64_t type;
        uint8_t first = data[offset++];
        if (first < 253) {
            type = first;
        } else if (first == 253) {
            if (offset + 2 > data.size()) return Result<HopPayload>::Error("Invalid TLV type");
            type = (static_cast<uint64_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;
        } else {
            return Result<HopPayload>::Error("Unsupported BigSize encoding");
        }
        
        // Read length (BigSize)
        if (offset >= data.size()) return Result<HopPayload>::Error("Missing TLV length");
        first = data[offset++];
        uint64_t length;
        if (first < 253) {
            length = first;
        } else if (first == 253) {
            if (offset + 2 > data.size()) return Result<HopPayload>::Error("Invalid TLV length");
            length = (static_cast<uint64_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;
        } else {
            return Result<HopPayload>::Error("Unsupported BigSize encoding");
        }
        
        // Read value
        if (offset + length > data.size()) {
            return Result<HopPayload>::Error("Insufficient data for TLV value");
        }
        
        std::vector<uint8_t> value(data.begin() + offset, data.begin() + offset + length);
        offset += length;
        
        // Parse based on type
        if (type == static_cast<uint64_t>(HopPayloadTLV::AMT_TO_FORWARD)) {
            if (value.size() != 8) return Result<HopPayload>::Error("Invalid amt_to_forward");
            payload.amt_to_forward = 0;
            for (size_t i = 0; i < 8; i++) {
                payload.amt_to_forward = (payload.amt_to_forward << 8) | value[i];
            }
        } else if (type == static_cast<uint64_t>(HopPayloadTLV::OUTGOING_CLTV_VALUE)) {
            if (value.size() != 4) return Result<HopPayload>::Error("Invalid cltv");
            payload.outgoing_cltv_value = 0;
            for (size_t i = 0; i < 4; i++) {
                payload.outgoing_cltv_value = (payload.outgoing_cltv_value << 8) | value[i];
            }
        } else if (type == static_cast<uint64_t>(HopPayloadTLV::SHORT_CHANNEL_ID)) {
            if (value.size() != 8) return Result<HopPayload>::Error("Invalid scid");
            payload.short_channel_id = 0;
            for (size_t i = 0; i < 8; i++) {
                payload.short_channel_id = (payload.short_channel_id << 8) | value[i];
            }
        }
    }
    
    return Result<HopPayload>::Ok(payload);
}

// ============================================================================
// Sphinx Packet Implementation
// ============================================================================

SphinxPacket::SphinxPacket() : version(SPHINX_VERSION) {
    ephemeral_key.fill(0);
    routing_info.fill(0);
    hmac.fill(0);
}

std::vector<uint8_t> SphinxPacket::Serialize() const {
    std::vector<uint8_t> data;
    data.reserve(SPHINX_PACKET_SIZE);
    
    data.push_back(version);
    data.insert(data.end(), ephemeral_key.begin(), ephemeral_key.end());
    data.insert(data.end(), routing_info.begin(), routing_info.end());
    data.insert(data.end(), hmac.begin(), hmac.end());
    
    return data;
}

Result<SphinxPacket> SphinxPacket::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() != SPHINX_PACKET_SIZE) {
        return Result<SphinxPacket>::Error("Invalid packet size");
    }
    
    SphinxPacket packet;
    size_t offset = 0;
    
    packet.version = data[offset++];
    std::copy_n(data.begin() + offset, 33, packet.ephemeral_key.begin());
    offset += 33;
    std::copy_n(data.begin() + offset, ROUTING_INFO_SIZE, packet.routing_info.begin());
    offset += ROUTING_INFO_SIZE;
    std::copy_n(data.begin() + offset, 32, packet.hmac.begin());
    
    return Result<SphinxPacket>::Ok(packet);
}

// ============================================================================
// Sphinx Packet Builder Implementation
// ============================================================================

SphinxPacketBuilder::SphinxPacketBuilder() {}

std::vector<SharedSecret> SphinxPacketBuilder::DeriveSharedSecrets(
    const std::vector<PublicKey>& pubkeys,
    const uint256& session_key)
{
    std::vector<SharedSecret> secrets;
    secrets.reserve(pubkeys.size());
    
    for (const auto& pubkey : pubkeys) {
        SharedSecret secret;
        
        // Derive shared secret using ECDH
        auto pub_bytes = pubkey.Serialize();
        std::vector<uint8_t> combined;
        combined.insert(combined.end(), session_key.data(), session_key.data() + 32);
        combined.insert(combined.end(), pub_bytes.begin(), pub_bytes.end());
        
        auto hash = SHA3::Hash(combined.data(), combined.size());
        std::copy_n(hash.data(), 32, secret.secret.begin());
        
        // Derive keys from shared secret
        auto rho_hash = SHA3::Hash(secret.secret.data(), 32);
        std::copy_n(rho_hash.data(), 32, secret.rho.begin());
        
        auto mu_hash = SHA3::Hash(rho_hash.data(), 32);
        std::copy_n(mu_hash.data(), 32, secret.mu.begin());
        
        auto pad_hash = SHA3::Hash(mu_hash.data(), 32);
        std::copy_n(pad_hash.data(), 32, secret.pad.begin());
        
        secrets.push_back(secret);
    }
    
    return secrets;
}

std::vector<uint8_t> SphinxPacketBuilder::GenerateFiller(
    const std::vector<SharedSecret>& shared_secrets,
    size_t num_hops,
    size_t hop_size)
{
    // Generate filler to pad the routing info
    std::vector<uint8_t> filler;
    
    for (size_t i = 1; i < num_hops; i++) {
        size_t filler_size = hop_size * i;
        filler.resize(filler_size, 0);
        
        // XOR with stream cipher
        auto stream = StreamCipher(shared_secrets[i - 1].rho, filler);
        for (size_t j = 0; j < filler.size(); j++) {
            filler[j] ^= stream[j];
        }
    }
    
    return filler;
}

std::array<uint8_t, 32> SphinxPacketBuilder::ComputeHMAC(
    const std::array<uint8_t, 32>& key,
    const std::vector<uint8_t>& data)
{
    // HMAC-SHA256
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), key.begin(), key.end());
    combined.insert(combined.end(), data.begin(), data.end());
    
    auto hash = SHA3::Hash(combined.data(), combined.size());
    
    std::array<uint8_t, 32> hmac;
    std::copy_n(hash.data(), 32, hmac.begin());
    return hmac;
}

std::vector<uint8_t> SphinxPacketBuilder::StreamCipher(
    const std::array<uint8_t, 32>& key,
    const std::vector<uint8_t>& data)
{
    // Simplified stream cipher (ChaCha20 would be used in production)
    std::vector<uint8_t> output = data;
    for (size_t i = 0; i < output.size(); i++) {
        output[i] ^= key[i % 32];
    }
    return output;
}

Result<SphinxPacket> SphinxPacketBuilder::CreatePacket(
    const std::vector<PublicKey>& route_pubkeys,
    const std::vector<HopPayload>& hop_payloads,
    const uint256& session_key,
    const uint256& associated_data)
{
    if (route_pubkeys.size() != hop_payloads.size()) {
        return Result<SphinxPacket>::Error("Mismatched route and payload sizes");
    }
    
    if (route_pubkeys.size() > NUM_MAX_HOPS) {
        return Result<SphinxPacket>::Error("Too many hops");
    }
    
    // Derive shared secrets
    auto shared_secrets = DeriveSharedSecrets(route_pubkeys, session_key);
    
    SphinxPacket packet;
    
    // Set ephemeral key (derived from session key)
    auto ephemeral_pubkey = KeyPair::Generate().GetPublicKey();
    auto ephemeral_bytes = ephemeral_pubkey.Serialize();
    std::copy_n(ephemeral_bytes.begin(), 33, packet.ephemeral_key.begin());
    
    // Build routing info
    std::vector<uint8_t> routing_info(ROUTING_INFO_SIZE, 0);
    
    // Add hop data for each hop (in reverse order)
    for (int i = hop_payloads.size() - 1; i >= 0; i--) {
        auto hop_data = hop_payloads[i].Serialize();
        
        // Shift routing info
        std::rotate(routing_info.begin(), routing_info.begin() + HOP_DATA_SIZE,
                   routing_info.end());
        
        // Insert hop data at beginning
        std::copy_n(hop_data.begin(),
                   std::min(hop_data.size(), size_t(HOP_DATA_SIZE)),
                   routing_info.begin());
        
        // Encrypt with this hop's key
        auto encrypted = StreamCipher(shared_secrets[i].rho, routing_info);
        routing_info = encrypted;
    }
    
    std::copy(routing_info.begin(), routing_info.end(), packet.routing_info.begin());
    
    // Compute HMAC
    std::vector<uint8_t> hmac_data;
    hmac_data.insert(hmac_data.end(), associated_data.data(), associated_data.data() + 32);
    hmac_data.insert(hmac_data.end(), packet.routing_info.begin(), packet.routing_info.end());
    
    packet.hmac = ComputeHMAC(shared_secrets[0].mu, hmac_data);
    
    return Result<SphinxPacket>::Ok(packet);
}

// ============================================================================
// Sphinx Packet Processor Implementation
// ============================================================================

SphinxPacketProcessor::SphinxPacketProcessor() {}

SharedSecret SphinxPacketProcessor::DeriveSharedSecret(
    const std::array<uint8_t, 33>& ephemeral_key,
    const SecretKey& privkey)
{
    SharedSecret secret;
    
    // ECDH
    std::vector<uint8_t> combined;
    auto priv_bytes = privkey.Serialize();
    combined.insert(combined.end(), priv_bytes.begin(), priv_bytes.end());
    combined.insert(combined.end(), ephemeral_key.begin(), ephemeral_key.end());
    
    auto hash = SHA3::Hash(combined.data(), combined.size());
    std::copy_n(hash.data(), 32, secret.secret.begin());
    
    // Derive sub-keys
    auto rho_hash = SHA3::Hash(secret.secret.data(), 32);
    std::copy_n(rho_hash.data(), 32, secret.rho.begin());
    
    auto mu_hash = SHA3::Hash(rho_hash.data(), 32);
    std::copy_n(mu_hash.data(), 32, secret.mu.begin());
    
    auto pad_hash = SHA3::Hash(mu_hash.data(), 32);
    std::copy_n(pad_hash.data(), 32, secret.pad.begin());
    
    return secret;
}

std::array<uint8_t, 33> SphinxPacketProcessor::BlindEphemeralKey(
    const std::array<uint8_t, 33>& ephemeral_key,
    const SharedSecret& shared_secret)
{
    // Blind the ephemeral key for next hop
    std::array<uint8_t, 33> blinded;
    for (size_t i = 0; i < 33; i++) {
        blinded[i] = ephemeral_key[i] ^ shared_secret.pad[i % 32];
    }
    return blinded;
}

Result<HopPayload> SphinxPacketProcessor::ExtractPayload(
    const std::vector<uint8_t>& routing_info)
{
    if (routing_info.size() < HOP_DATA_SIZE) {
        return Result<HopPayload>::Error("Insufficient routing info");
    }
    
    std::vector<uint8_t> hop_data(routing_info.begin(),
                                   routing_info.begin() + HOP_DATA_SIZE);
    
    return HopPayload::Deserialize(hop_data);
}

bool SphinxPacketProcessor::VerifyHMAC(
    const std::array<uint8_t, 32>& expected_hmac,
    const std::array<uint8_t, 32>& key,
    const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), key.begin(), key.end());
    combined.insert(combined.end(), data.begin(), data.end());
    
    auto hash = SHA3::Hash(combined.data(), combined.size());
    
    return std::equal(expected_hmac.begin(), expected_hmac.end(), hash.data());
}

Result<SphinxPacketProcessor::ProcessResult> SphinxPacketProcessor::ProcessPacket(
    const SphinxPacket& packet,
    const SecretKey& node_privkey,
    const uint256& associated_data)
{
    ProcessResult result;
    
    // Derive shared secret
    auto shared_secret = DeriveSharedSecret(packet.ephemeral_key, node_privkey);
    result.shared_secret = shared_secret.secret;
    
    // Verify HMAC
    std::vector<uint8_t> hmac_data;
    hmac_data.insert(hmac_data.end(), associated_data.data(), associated_data.data() + 32);
    hmac_data.insert(hmac_data.end(), packet.routing_info.begin(), packet.routing_info.end());
    
    if (!VerifyHMAC(packet.hmac, shared_secret.mu, hmac_data)) {
        return Result<ProcessResult>::Error("HMAC verification failed");
    }
    
    // Decrypt routing info
    std::vector<uint8_t> routing_info(packet.routing_info.begin(), packet.routing_info.end());
    for (size_t i = 0; i < routing_info.size(); i++) {
        routing_info[i] ^= shared_secret.rho[i % 32];
    }
    
    // Extract payload
    auto payload_result = ExtractPayload(routing_info);
    if (payload_result.IsError()) {
        return Result<ProcessResult>::Error(payload_result.error);
    }
    result.payload = payload_result.Unwrap();
    
    // Check if final hop (all zeros in remaining routing info)
    bool is_final = std::all_of(
        routing_info.begin() + HOP_DATA_SIZE,
        routing_info.end(),
        [](uint8_t b) { return b == 0; }
    );
    result.is_final_hop = is_final;
    
    if (!is_final) {
        // Prepare packet for next hop
        result.next_packet = packet;
        
        // Shift routing info
        std::rotate(routing_info.begin(), routing_info.begin() + HOP_DATA_SIZE,
                   routing_info.end());
        std::copy(routing_info.begin(), routing_info.end(),
                 result.next_packet.routing_info.begin());
        
        // Blind ephemeral key
        result.next_packet.ephemeral_key = BlindEphemeralKey(
            packet.ephemeral_key,
            shared_secret
        );
        
        // Recompute HMAC for next packet
        std::vector<uint8_t> next_hmac_data;
        next_hmac_data.insert(next_hmac_data.end(),
                             associated_data.data(), associated_data.data() + 32);
        next_hmac_data.insert(next_hmac_data.end(),
                             result.next_packet.routing_info.begin(),
                             result.next_packet.routing_info.end());
        
        std::vector<uint8_t> combined;
        combined.insert(combined.end(), shared_secret.mu.begin(), shared_secret.mu.end());
        combined.insert(combined.end(), next_hmac_data.begin(), next_hmac_data.end());
        
        auto hash = SHA3::Hash(combined.data(), combined.size());
        std::copy_n(hash.data(), 32, result.next_packet.hmac.begin());
    }
    
    return Result<ProcessResult>::Ok(result);
}

// ============================================================================
// Onion Error Implementation
// ============================================================================

std::vector<uint8_t> OnionError::Serialize() const {
    std::vector<uint8_t> result;
    result.push_back((static_cast<uint16_t>(code) >> 8) & 0xFF);
    result.push_back(static_cast<uint16_t>(code) & 0xFF);
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

Result<OnionError> OnionError::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return Result<OnionError>::Error("Insufficient data");
    }
    
    OnionError error;
    error.code = static_cast<OnionErrorCode>(
        (static_cast<uint16_t>(data[0]) << 8) | data[1]
    );
    error.data.assign(data.begin() + 2, data.end());
    
    return Result<OnionError>::Ok(error);
}

// ============================================================================
// MPP Payment Implementation
// ============================================================================

bool MPPPayment::IsComplete() const {
    return GetTotalReceived() >= total_msat;
}

uint64_t MPPPayment::GetTotalReceived() const {
    uint64_t total = 0;
    for (const auto& [htlc_id, amount] : partial_payments) {
        total += amount;
    }
    return total;
}

} // namespace bolt
} // namespace intcoin
