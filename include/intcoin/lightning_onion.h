// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network onion routing implementation (Sphinx protocol).
// Quantum-resistant adaptation using Kyber1024 for key exchange.

#ifndef INTCOIN_LIGHTNING_ONION_H
#define INTCOIN_LIGHTNING_ONION_H

#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <optional>
#include <cstdint>

namespace intcoin {
namespace lightning {
namespace onion {

/**
 * Onion routing constants
 */
namespace constants {
    static constexpr size_t MAX_HOPS = 20;              // Maximum route length
    static constexpr size_t HOP_PAYLOAD_SIZE = 65;      // Per-hop payload size
    static constexpr size_t HMAC_SIZE = 32;             // HMAC-SHA3-256
    static constexpr size_t SHARED_SECRET_SIZE = 32;    // Shared secret size
    static constexpr size_t PACKET_SIZE = 1366;         // Fixed onion packet size
    static constexpr size_t VERSION_SIZE = 1;           // Version byte
    static constexpr size_t PUBKEY_SIZE = 1568;         // Kyber1024 public key (compressed)
    static constexpr size_t ROUTING_INFO_SIZE = 1300;   // Encrypted routing information
}

/**
 * Hop data for a single node in the route
 */
struct HopData {
    uint16_t realm;                    // Realm identifier (0 = Bitcoin-like)
    Hash256 short_channel_id;          // Channel to forward through
    uint64_t amt_to_forward;           // Amount to forward (msat)
    uint32_t outgoing_cltv_value;      // CLTV value for outgoing HTLC
    std::vector<uint8_t> padding;      // Padding for fixed size

    HopData() : realm(0), short_channel_id{}, amt_to_forward(0), outgoing_cltv_value(0) {}

    // Serialize to fixed-size payload
    std::vector<uint8_t> serialize() const;
    static HopData deserialize(const std::vector<uint8_t>& data);
};

/**
 * Per-hop payload (encrypted for each hop)
 */
struct HopPayload {
    HopData hop_data;
    std::vector<uint8_t> hmac;  // HMAC for integrity

    HopPayload() {}

    std::vector<uint8_t> serialize() const;
    static std::optional<HopPayload> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Shared secret derived from Kyber1024 key exchange
 */
struct SharedSecret {
    std::vector<uint8_t> secret;  // 32-byte shared secret

    SharedSecret() : secret(32, 0) {}
    explicit SharedSecret(const std::vector<uint8_t>& s) : secret(s) {}

    // Derive sub-secrets for different purposes
    std::vector<uint8_t> derive_rho() const;        // For HMAC key
    std::vector<uint8_t> derive_mu() const;         // For encryption key
    std::vector<uint8_t> derive_um() const;         // For blinding factor
    std::vector<uint8_t> derive_pad() const;        // For padding generation
};

/**
 * Onion packet (Sphinx protocol)
 */
class OnionPacket {
public:
    uint8_t version;                         // Protocol version
    KyberPubKey ephemeral_key;    // Ephemeral public key (Kyber1024)
    std::vector<uint8_t> routing_info;       // Encrypted routing information
    std::vector<uint8_t> hmac;               // Packet integrity HMAC

    OnionPacket();

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<OnionPacket> deserialize(const std::vector<uint8_t>& data);

    // Validate packet size and structure
    bool is_valid() const;
};

/**
 * Onion packet builder
 * Constructs layered encryption for multi-hop routing
 */
class OnionPacketBuilder {
public:
    OnionPacketBuilder();

    /**
     * Create onion packet for a payment route
     *
     * @param route List of node public keys in the route
     * @param hop_payloads Per-hop payment data
     * @param payment_hash Payment hash for the HTLC
     * @param session_key Random session key for ephemeral key generation
     * @return Onion packet ready for transmission
     */
    std::optional<OnionPacket> build(
        const std::vector<KyberPubKey>& route,
        const std::vector<HopPayload>& hop_payloads,
        const Hash256& payment_hash,
        const std::vector<uint8_t>& session_key);

private:
    // Generate ephemeral keys using Kyber1024
    crypto::KyberKeyPair generate_ephemeral_keypair(const std::vector<uint8_t>& seed);

    // Perform Kyber1024 key exchange
    SharedSecret perform_key_exchange(
        const KyberPubKey& node_pubkey,
        const std::array<uint8_t, 3168>& ephemeral_privkey);

    // Compute HMAC-SHA3-256
    std::vector<uint8_t> compute_hmac(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data);

    // ChaCha20 stream cipher for encryption
    std::vector<uint8_t> chacha20_encrypt(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& plaintext);

    // Blind ephemeral public key
    KyberPubKey blind_pubkey(
        const KyberPubKey& pubkey,
        const std::vector<uint8_t>& blinding_factor);

    // Generate filler for last hops
    std::vector<uint8_t> generate_filler(
        const std::vector<SharedSecret>& shared_secrets,
        size_t num_hops);
};

/**
 * Onion packet processor
 * Decrypts and processes onion packets at each hop
 */
class OnionPacketProcessor {
public:
    OnionPacketProcessor(const crypto::KyberKeyPair& node_keypair);

    /**
     * Process onion packet at this node
     *
     * @param packet Incoming onion packet
     * @param associated_data Additional data for HMAC
     * @return Decrypted hop payload and next packet (if not final hop)
     */
    struct ProcessResult {
        HopPayload hop_payload;              // Decrypted payload for this hop
        std::optional<OnionPacket> next_packet;  // Packet for next hop (nullopt if final)
        bool is_final_hop;                   // True if this is the destination
    };

    std::optional<ProcessResult> process(
        const OnionPacket& packet,
        const std::vector<uint8_t>& associated_data);

private:
    crypto::KyberKeyPair node_keypair_;

    // Perform Kyber1024 key exchange
    SharedSecret perform_key_exchange(
        const KyberPubKey& ephemeral_key);

    // Decrypt routing information
    std::vector<uint8_t> decrypt_routing_info(
        const std::vector<uint8_t>& encrypted_data,
        const std::vector<uint8_t>& key);

    // Verify HMAC
    bool verify_hmac(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& hmac);

    // Compute HMAC
    std::vector<uint8_t> compute_hmac(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data);

    // ChaCha20 stream cipher for decryption
    std::vector<uint8_t> chacha20_decrypt(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& ciphertext);

    // Blind ephemeral public key for next hop
    KyberPubKey blind_pubkey_forward(
        const KyberPubKey& pubkey,
        const std::vector<uint8_t>& blinding_factor);
};

/**
 * Error onion packet
 * Encrypted failure message sent back along the route
 */
class ErrorOnion {
public:
    std::vector<uint8_t> encrypted_failure;

    ErrorOnion() {}
    explicit ErrorOnion(const std::vector<uint8_t>& failure)
        : encrypted_failure(failure) {}

    /**
     * Create error onion for a failed payment
     *
     * @param failure_message Error message to encrypt
     * @param shared_secrets Shared secrets from the forward path
     * @return Encrypted error onion
     */
    static ErrorOnion create(
        const std::vector<uint8_t>& failure_message,
        const std::vector<SharedSecret>& shared_secrets);

    /**
     * Decrypt error onion at a hop
     *
     * @param shared_secret Shared secret at this hop
     * @return Decrypted error onion
     */
    ErrorOnion decrypt(const SharedSecret& shared_secret) const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<ErrorOnion> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Failure codes (BOLT #4)
 */
enum class FailureCode : uint16_t {
    INVALID_REALM = 0x0001,
    TEMPORARY_NODE_FAILURE = 0x2002,
    PERMANENT_NODE_FAILURE = 0x4002,
    REQUIRED_NODE_FEATURE_MISSING = 0x4003,
    INVALID_ONION_VERSION = 0x8001,
    INVALID_ONION_HMAC = 0x8002,
    INVALID_ONION_KEY = 0x8003,
    TEMPORARY_CHANNEL_FAILURE = 0x1007,
    PERMANENT_CHANNEL_FAILURE = 0x4007,
    REQUIRED_CHANNEL_FEATURE_MISSING = 0x4008,
    UNKNOWN_NEXT_PEER = 0x400A,
    AMOUNT_BELOW_MINIMUM = 0x400B,
    FEE_INSUFFICIENT = 0x400C,
    INCORRECT_CLTV_EXPIRY = 0x400D,
    EXPIRY_TOO_SOON = 0x400E,
    CHANNEL_DISABLED = 0x4010,
    EXPIRY_TOO_FAR = 0x4011,
    INCORRECT_OR_UNKNOWN_PAYMENT_DETAILS = 0x400F,
    FINAL_INCORRECT_CLTV_EXPIRY = 0x4012,
    FINAL_INCORRECT_HTLC_AMOUNT = 0x4013
};

/**
 * Failure message structure
 */
struct FailureMessage {
    FailureCode code;
    std::vector<uint8_t> data;  // Optional failure-specific data

    FailureMessage() : code(FailureCode::TEMPORARY_NODE_FAILURE) {}
    FailureMessage(FailureCode c, const std::vector<uint8_t>& d)
        : code(c), data(d) {}

    std::vector<uint8_t> serialize() const;
    static std::optional<FailureMessage> deserialize(const std::vector<uint8_t>& data);
};

} // namespace onion
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_ONION_H
