// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #4: Onion Routing Protocol - Full Sphinx Implementation

#ifndef INTCOIN_BOLT_ONION_H
#define INTCOIN_BOLT_ONION_H

#include "intcoin/types.h"
#include "intcoin/crypto.h"
#include <vector>
#include <array>

namespace intcoin {
namespace bolt {

// ============================================================================
// BOLT #4: Sphinx Onion Routing
// ============================================================================

// Sphinx packet version
constexpr uint8_t SPHINX_VERSION = 0;

// Sphinx packet sizes
constexpr size_t SPHINX_PACKET_SIZE = 1366;  // Fixed size onion packet
constexpr size_t HOP_DATA_SIZE = 65;          // Per-hop data size
constexpr size_t HOP_PAYLOAD_SIZE = 33;       // Per-hop payload size
constexpr size_t NUM_MAX_HOPS = 20;           // Maximum number of hops
constexpr size_t ROUTING_INFO_SIZE = HOP_DATA_SIZE * NUM_MAX_HOPS;  // 1300 bytes

// Hop payload TLV types
enum class HopPayloadTLV : uint64_t {
    AMT_TO_FORWARD = 2,
    OUTGOING_CLTV_VALUE = 4,
    SHORT_CHANNEL_ID = 6,
    PAYMENT_DATA = 8,
    PAYMENT_METADATA = 16,
};

// Per-hop payload
struct HopPayload {
    uint64_t amt_to_forward;         // Amount to forward (msat)
    uint32_t outgoing_cltv_value;    // Outgoing CLTV value
    uint64_t short_channel_id;        // Short channel ID
    std::optional<uint256> payment_secret;  // Payment secret
    std::optional<uint64_t> total_msat;     // Total amount (for MPP)
    std::vector<uint8_t> custom_records;     // Custom TLV records

    // Serialization (TLV format)
    std::vector<uint8_t> Serialize() const;
    static Result<HopPayload> Deserialize(const std::vector<uint8_t>& data);
};

// Sphinx onion packet
struct SphinxPacket {
    uint8_t version;                           // Version (0)
    std::array<uint8_t, 33> ephemeral_key;    // Ephemeral public key
    std::array<uint8_t, ROUTING_INFO_SIZE> routing_info;  // Encrypted routing info
    std::array<uint8_t, 32> hmac;              // HMAC authenticator

    SphinxPacket();

    // Serialization
    std::vector<uint8_t> Serialize() const;
    static Result<SphinxPacket> Deserialize(const std::vector<uint8_t>& data);
};

// Shared secret derived per hop
struct SharedSecret {
    std::array<uint8_t, 32> secret;
    std::array<uint8_t, 32> rho;      // Stream cipher key
    std::array<uint8_t, 32> mu;       // HMAC key
    std::array<uint8_t, 32> pad;      // Padding key
};

// Sphinx packet builder (sender)
class SphinxPacketBuilder {
public:
    SphinxPacketBuilder();

    // Create onion packet from route and payload
    Result<SphinxPacket> CreatePacket(
        const std::vector<PublicKey>& route_pubkeys,
        const std::vector<HopPayload>& hop_payloads,
        const uint256& session_key,
        const uint256& associated_data
    );

private:
    // Derive shared secrets for each hop
    std::vector<SharedSecret> DeriveSharedSecrets(
        const std::vector<PublicKey>& pubkeys,
        const uint256& session_key
    );

    // Generate filler for padding
    std::vector<uint8_t> GenerateFiller(
        const std::vector<SharedSecret>& shared_secrets,
        size_t num_hops,
        size_t hop_size
    );

    // Compute HMAC
    std::array<uint8_t, 32> ComputeHMAC(
        const std::array<uint8_t, 32>& key,
        const std::vector<uint8_t>& data
    );

    // Stream cipher (ChaCha20)
    std::vector<uint8_t> StreamCipher(
        const std::array<uint8_t, 32>& key,
        const std::vector<uint8_t>& data
    );
};

// Sphinx packet processor (intermediate/final hop)
class SphinxPacketProcessor {
public:
    SphinxPacketProcessor();

    // Process packet at intermediate hop
    struct ProcessResult {
        HopPayload payload;               // Extracted hop payload
        SphinxPacket next_packet;         // Packet for next hop
        bool is_final_hop;                 // True if this is the final hop
        std::array<uint8_t, 32> shared_secret;  // Shared secret (for replay detection)
    };

    Result<ProcessResult> ProcessPacket(
        const SphinxPacket& packet,
        const SecretKey& node_privkey,
        const uint256& associated_data
    );

private:
    // Derive shared secret
    SharedSecret DeriveSharedSecret(
        const std::array<uint8_t, 33>& ephemeral_key,
        const SecretKey& privkey
    );

    // Blind ephemeral key for next hop
    std::array<uint8_t, 33> BlindEphemeralKey(
        const std::array<uint8_t, 33>& ephemeral_key,
        const SharedSecret& shared_secret
    );

    // Extract hop payload
    Result<HopPayload> ExtractPayload(
        const std::vector<uint8_t>& routing_info
    );

    // Verify HMAC
    bool VerifyHMAC(
        const std::array<uint8_t, 32>& expected_hmac,
        const std::array<uint8_t, 32>& key,
        const std::vector<uint8_t>& data
    );
};

// Onion error messages (BOLT #4)
enum class OnionErrorCode : uint16_t {
    INVALID_REALM = 0x0001,
    TEMPORARY_NODE_FAILURE = 0x2002,
    PERMANENT_NODE_FAILURE = 0x4002,
    REQUIRED_NODE_FEATURE_MISSING = 0x4003,
    INVALID_ONION_VERSION = 0x8001,
    INVALID_ONION_HMAC = 0x8002,
    INVALID_ONION_KEY = 0x8003,
    TEMPORARY_CHANNEL_FAILURE = 0x1007,
    PERMANENT_CHANNEL_FAILURE = 0x4007,
    REQUIRED_CHANNEL_FEATURE_MISSING = 0x4009,
    UNKNOWN_NEXT_PEER = 0x400a,
    AMOUNT_BELOW_MINIMUM = 0x400b,
    FEE_INSUFFICIENT = 0x400c,
    INCORRECT_CLTV_EXPIRY = 0x400d,
    EXPIRY_TOO_SOON = 0x400e,
    CHANNEL_DISABLED = 0x4010,
    EXPIRY_TOO_FAR = 0x4011,
};

struct OnionError {
    OnionErrorCode code;
    std::vector<uint8_t> data;

    // Serialize for encrypted transmission back to sender
    std::vector<uint8_t> Serialize() const;
    static Result<OnionError> Deserialize(const std::vector<uint8_t>& data);
};

// Onion error encryptor (for returning errors)
class OnionErrorEncryptor {
public:
    OnionErrorEncryptor(const std::vector<SharedSecret>& shared_secrets);

    // Encrypt error for return to sender
    std::vector<uint8_t> EncryptError(
        size_t failing_hop,
        const OnionError& error
    );

    // Decrypt error at sender
    Result<OnionError> DecryptError(
        const std::vector<uint8_t>& encrypted_error
    );

private:
    std::vector<SharedSecret> shared_secrets_;
};

// MPP (Multi-Path Payments) support
struct MPPPayment {
    uint256 payment_secret;
    uint64_t total_msat;
    std::map<uint64_t, uint64_t> partial_payments;  // htlc_id -> amount

    bool IsComplete() const;
    uint64_t GetTotalReceived() const;
};

} // namespace bolt
} // namespace intcoin

#endif // INTCOIN_BOLT_ONION_H
