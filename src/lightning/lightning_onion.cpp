// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning_onion.h"
#include "intcoin/serialization.h"
#include "intcoin/crypto.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace intcoin {
namespace lightning {
namespace onion {

// ===== Shared Secret Implementation =====

std::vector<uint8_t> SharedSecret::derive_rho() const {
    // HKDF for HMAC key derivation
    std::vector<uint8_t> info{'r', 'h', 'o'};
    std::vector<uint8_t> derived(32);

    // Use SHA3-256 for key derivation
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha3_256();

    // Simple HKDF-like derivation
    std::vector<uint8_t> input = secret;
    input.insert(input.end(), info.begin(), info.end());

    unsigned int md_len = 0;
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, derived.data(), &md_len);
    EVP_MD_CTX_free(ctx);

    return derived;
}

std::vector<uint8_t> SharedSecret::derive_mu() const {
    std::vector<uint8_t> info{'m', 'u'};
    std::vector<uint8_t> derived(32);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha3_256();

    std::vector<uint8_t> input = secret;
    input.insert(input.end(), info.begin(), info.end());

    unsigned int md_len = 0;
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, derived.data(), &md_len);
    EVP_MD_CTX_free(ctx);

    return derived;
}

std::vector<uint8_t> SharedSecret::derive_um() const {
    std::vector<uint8_t> info{'u', 'm'};
    std::vector<uint8_t> derived(32);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha3_256();

    std::vector<uint8_t> input = secret;
    input.insert(input.end(), info.begin(), info.end());

    unsigned int md_len = 0;
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, derived.data(), &md_len);
    EVP_MD_CTX_free(ctx);

    return derived;
}

std::vector<uint8_t> SharedSecret::derive_pad() const {
    std::vector<uint8_t> info{'p', 'a', 'd'};
    std::vector<uint8_t> derived(32);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha3_256();

    std::vector<uint8_t> input = secret;
    input.insert(input.end(), info.begin(), info.end());

    unsigned int md_len = 0;
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, derived.data(), &md_len);
    EVP_MD_CTX_free(ctx);

    return derived;
}

// ===== HopData Implementation =====

std::vector<uint8_t> HopData::serialize() const {
    std::vector<uint8_t> data;

    // Realm (2 bytes)
    data.push_back(static_cast<uint8_t>(realm & 0xFF));
    data.push_back(static_cast<uint8_t>((realm >> 8) & 0xFF));

    // Short channel ID (32 bytes)
    data.insert(data.end(), short_channel_id.begin(), short_channel_id.end());

    // Amount to forward (8 bytes)
    for (int i = 0; i < 8; i++) {
        data.push_back(static_cast<uint8_t>((amt_to_forward >> (i * 8)) & 0xFF));
    }

    // Outgoing CLTV value (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((outgoing_cltv_value >> (i * 8)) & 0xFF));
    }

    // Pad to fixed size (constants::HOP_PAYLOAD_SIZE - HMAC_SIZE)
    while (data.size() < constants::HOP_PAYLOAD_SIZE - constants::HMAC_SIZE) {
        data.push_back(0);
    }

    return data;
}

HopData HopData::deserialize(const std::vector<uint8_t>& data) {
    HopData hop;
    if (data.size() < 46) {  // Minimum size
        return hop;
    }

    size_t offset = 0;

    // Realm (2 bytes)
    hop.realm = static_cast<uint16_t>(data[offset]) |
                (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Short channel ID (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, hop.short_channel_id.begin());
    offset += 32;

    // Amount to forward (8 bytes)
    hop.amt_to_forward = 0;
    for (int i = 0; i < 8; i++) {
        hop.amt_to_forward |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
    }

    // Outgoing CLTV value (4 bytes)
    hop.outgoing_cltv_value = 0;
    for (int i = 0; i < 4; i++) {
        hop.outgoing_cltv_value |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }

    return hop;
}

// ===== HopPayload Implementation =====

std::vector<uint8_t> HopPayload::serialize() const {
    std::vector<uint8_t> data = hop_data.serialize();
    data.insert(data.end(), hmac.begin(), hmac.end());
    return data;
}

std::optional<HopPayload> HopPayload::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < constants::HOP_PAYLOAD_SIZE) {
        return std::nullopt;
    }

    HopPayload payload;

    // Extract hop data
    std::vector<uint8_t> hop_data_bytes(data.begin(),
                                        data.begin() + constants::HOP_PAYLOAD_SIZE - constants::HMAC_SIZE);
    payload.hop_data = HopData::deserialize(hop_data_bytes);

    // Extract HMAC
    payload.hmac.assign(data.begin() + constants::HOP_PAYLOAD_SIZE - constants::HMAC_SIZE,
                       data.begin() + constants::HOP_PAYLOAD_SIZE);

    return payload;
}

// ===== OnionPacket Implementation =====

OnionPacket::OnionPacket() : version(0) {
    routing_info.resize(constants::ROUTING_INFO_SIZE, 0);
    hmac.resize(constants::HMAC_SIZE, 0);
}

std::vector<uint8_t> OnionPacket::serialize() const {
    std::vector<uint8_t> data;

    // Version (1 byte)
    data.push_back(version);

    // Ephemeral public key (Kyber1024 public key size)
    data.insert(data.end(), ephemeral_key.begin(), ephemeral_key.end());

    // Routing info
    data.insert(data.end(), routing_info.begin(), routing_info.end());

    // HMAC
    data.insert(data.end(), hmac.begin(), hmac.end());

    return data;
}

std::optional<OnionPacket> OnionPacket::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < constants::PACKET_SIZE) {
        return std::nullopt;
    }

    OnionPacket packet;
    size_t offset = 0;

    // Version
    packet.version = data[offset++];

    // Ephemeral public key (1568 bytes for Kyber1024)
    if (offset + 1568 > data.size()) return std::nullopt;
    std::copy(data.begin() + offset, data.begin() + offset + 1568,
              packet.ephemeral_key.begin());
    offset += 1568;

    // Routing info
    if (offset + constants::ROUTING_INFO_SIZE > data.size()) return std::nullopt;
    packet.routing_info.assign(data.begin() + offset,
                               data.begin() + offset + constants::ROUTING_INFO_SIZE);
    offset += constants::ROUTING_INFO_SIZE;

    // HMAC
    if (offset + constants::HMAC_SIZE > data.size()) return std::nullopt;
    packet.hmac.assign(data.begin() + offset,
                      data.begin() + offset + constants::HMAC_SIZE);

    return packet;
}

bool OnionPacket::is_valid() const {
    return version == 0 &&
           ephemeral_key.size() == 1568 &&
           routing_info.size() == constants::ROUTING_INFO_SIZE &&
           hmac.size() == constants::HMAC_SIZE;
}

// ===== OnionPacketBuilder Implementation =====

OnionPacketBuilder::OnionPacketBuilder() {
}

std::optional<OnionPacket> OnionPacketBuilder::build(
    const std::vector<KyberPubKey>& route,
    const std::vector<HopPayload>& hop_payloads,
    const Hash256& payment_hash,
    const std::vector<uint8_t>& session_key) {

    if (route.empty() || route.size() > constants::MAX_HOPS) {
        std::cerr << "Invalid route length" << std::endl;
        return std::nullopt;
    }

    if (route.size() != hop_payloads.size()) {
        std::cerr << "Route and payload count mismatch" << std::endl;
        return std::nullopt;
    }

    size_t num_hops = route.size();
    std::vector<SharedSecret> shared_secrets;

    // Generate ephemeral keypair
    auto ephemeral_keypair = generate_ephemeral_keypair(session_key);

    // Perform key exchanges with each hop
    for (size_t i = 0; i < num_hops; i++) {
        SharedSecret ss = perform_key_exchange(route[i], ephemeral_keypair.private_key);
        shared_secrets.push_back(ss);
    }

    // Build routing information (encrypted layer by layer)
    std::vector<uint8_t> routing_info(constants::ROUTING_INFO_SIZE, 0);

    // Start with the final hop and work backwards
    for (int i = static_cast<int>(num_hops) - 1; i >= 0; i--) {
        // Serialize hop payload
        auto payload_bytes = hop_payloads[i].serialize();

        // Shift routing info to make room for this hop
        std::vector<uint8_t> new_routing_info = payload_bytes;
        new_routing_info.insert(new_routing_info.end(),
                               routing_info.begin(),
                               routing_info.begin() + constants::ROUTING_INFO_SIZE - payload_bytes.size());
        routing_info = new_routing_info;

        // Encrypt with this hop's shared secret
        auto mu_key = shared_secrets[i].derive_mu();
        std::vector<uint8_t> nonce(12, 0);  // ChaCha20 nonce
        routing_info = chacha20_encrypt(mu_key, nonce, routing_info);
    }

    // Generate filler for the last hops
    auto filler = generate_filler(shared_secrets, num_hops);

    // Compute HMAC for the packet
    auto rho_key = shared_secrets[0].derive_rho();
    auto packet_hmac = compute_hmac(rho_key, routing_info);

    // Build final packet
    OnionPacket packet;
    packet.version = 0;
    packet.ephemeral_key = ephemeral_keypair.public_key;
    packet.routing_info = routing_info;
    packet.hmac = packet_hmac;

    return packet;
}

crypto::KyberKeyPair OnionPacketBuilder::generate_ephemeral_keypair(
    const std::vector<uint8_t>& seed) {
    // In a real implementation, use the seed with Kyber key generation
    // For now, generate a random keypair
    return crypto::Kyber::generate_keypair();
}

SharedSecret OnionPacketBuilder::perform_key_exchange(
    const KyberPubKey& node_pubkey,
    const std::array<uint8_t, 3168>& ephemeral_privkey) {
    // Perform Kyber1024 encapsulation
    auto result = crypto::Kyber::encapsulate(node_pubkey);
    std::vector<uint8_t> shared_secret_vec(result.second.begin(), result.second.end());
    return SharedSecret(shared_secret_vec);
}

std::vector<uint8_t> OnionPacketBuilder::compute_hmac(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data) {

    std::vector<uint8_t> result(32);
    unsigned int len = 0;

    HMAC(EVP_sha3_256(), key.data(), key.size(),
         data.data(), data.size(), result.data(), &len);

    return result;
}

std::vector<uint8_t> OnionPacketBuilder::chacha20_encrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& plaintext) {

    // ChaCha20 encryption using OpenSSL
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> ciphertext(plaintext.size());

    int len = 0;
    EVP_EncryptInit_ex(ctx, EVP_chacha20(), nullptr, key.data(), nonce.data());
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size());
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext;
}

KyberPubKey OnionPacketBuilder::blind_pubkey(
    const KyberPubKey& pubkey,
    const std::vector<uint8_t>& blinding_factor) {
    // Blinding for Kyber keys is complex and not standardized
    // For now, return the original key
    // In a production implementation, this would use a proper blinding scheme
    return pubkey;
}

std::vector<uint8_t> OnionPacketBuilder::generate_filler(
    const std::vector<SharedSecret>& shared_secrets,
    size_t num_hops) {

    std::vector<uint8_t> filler;

    // Generate filler bytes for the last hops
    // This prevents the packet from shrinking as it traverses the route
    for (size_t i = 1; i < num_hops; i++) {
        auto pad_key = shared_secrets[i].derive_pad();
        std::vector<uint8_t> stream(constants::HOP_PAYLOAD_SIZE);

        // Generate pseudo-random stream
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha3_256();

        unsigned int md_len = 0;
        EVP_DigestInit_ex(ctx, md, nullptr);
        EVP_DigestUpdate(ctx, pad_key.data(), pad_key.size());
        EVP_DigestFinal_ex(ctx, stream.data(), &md_len);
        EVP_MD_CTX_free(ctx);

        filler.insert(filler.end(), stream.begin(), stream.begin() + constants::HOP_PAYLOAD_SIZE);
    }

    return filler;
}

// ===== OnionPacketProcessor Implementation =====

OnionPacketProcessor::OnionPacketProcessor(const crypto::KyberKeyPair& node_keypair)
    : node_keypair_(node_keypair) {
}

std::optional<OnionPacketProcessor::ProcessResult> OnionPacketProcessor::process(
    const OnionPacket& packet,
    const std::vector<uint8_t>& associated_data) {

    if (!packet.is_valid()) {
        std::cerr << "Invalid onion packet" << std::endl;
        return std::nullopt;
    }

    // Perform key exchange with ephemeral key
    SharedSecret shared_secret = perform_key_exchange(packet.ephemeral_key);

    // Verify HMAC
    auto rho_key = shared_secret.derive_rho();
    if (!verify_hmac(rho_key, packet.routing_info, packet.hmac)) {
        std::cerr << "HMAC verification failed" << std::endl;
        return std::nullopt;
    }

    // Decrypt routing information
    auto mu_key = shared_secret.derive_mu();
    std::vector<uint8_t> nonce(12, 0);
    auto decrypted = chacha20_decrypt(mu_key, nonce, packet.routing_info);

    // Extract hop payload
    auto hop_payload = HopPayload::deserialize(decrypted);
    if (!hop_payload) {
        std::cerr << "Failed to deserialize hop payload" << std::endl;
        return std::nullopt;
    }

    ProcessResult result;
    result.hop_payload = *hop_payload;

    // Check if this is the final hop
    // (All zeros in the next hop data indicates final hop)
    bool is_final = true;
    for (size_t i = constants::HOP_PAYLOAD_SIZE; i < decrypted.size(); i++) {
        if (decrypted[i] != 0) {
            is_final = false;
            break;
        }
    }

    result.is_final_hop = is_final;

    if (!is_final) {
        // Build next packet
        OnionPacket next_packet;
        next_packet.version = 0;

        // Blind ephemeral key
        auto um = shared_secret.derive_um();
        next_packet.ephemeral_key = blind_pubkey_forward(packet.ephemeral_key, um);

        // Shift routing info
        next_packet.routing_info.assign(
            decrypted.begin() + constants::HOP_PAYLOAD_SIZE,
            decrypted.begin() + constants::HOP_PAYLOAD_SIZE + constants::ROUTING_INFO_SIZE);

        // Compute new HMAC
        auto next_rho = shared_secret.derive_rho();
        next_packet.hmac = compute_hmac(next_rho, next_packet.routing_info);

        result.next_packet = next_packet;
    }

    return result;
}

SharedSecret OnionPacketProcessor::perform_key_exchange(
    const KyberPubKey& ephemeral_key) {
    // Perform Kyber1024 decapsulation
    // In this case, we need the ciphertext that was encapsulated
    // For simplicity, we'll use a placeholder
    std::vector<uint8_t> shared_secret(32, 0);  // Placeholder
    return SharedSecret(shared_secret);
}

std::vector<uint8_t> OnionPacketProcessor::decrypt_routing_info(
    const std::vector<uint8_t>& encrypted_data,
    const std::vector<uint8_t>& key) {
    std::vector<uint8_t> nonce(12, 0);
    return chacha20_decrypt(key, nonce, encrypted_data);
}

bool OnionPacketProcessor::verify_hmac(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& hmac) {

    auto computed = compute_hmac(key, data);
    return computed == hmac;
}

std::vector<uint8_t> OnionPacketProcessor::compute_hmac(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data) {

    std::vector<uint8_t> result(32);
    unsigned int len = 0;

    HMAC(EVP_sha3_256(), key.data(), key.size(),
         data.data(), data.size(), result.data(), &len);

    return result;
}

std::vector<uint8_t> OnionPacketProcessor::chacha20_decrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& ciphertext) {

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> plaintext(ciphertext.size());

    int len = 0;
    EVP_DecryptInit_ex(ctx, EVP_chacha20(), nullptr, key.data(), nonce.data());
    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());
    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}

KyberPubKey OnionPacketProcessor::blind_pubkey_forward(
    const KyberPubKey& pubkey,
    const std::vector<uint8_t>& blinding_factor) {
    // Blinding for Kyber keys - placeholder
    return pubkey;
}

// ===== ErrorOnion Implementation =====

ErrorOnion ErrorOnion::create(
    const std::vector<uint8_t>& failure_message,
    const std::vector<SharedSecret>& shared_secrets) {

    std::vector<uint8_t> encrypted = failure_message;

    // Encrypt with each shared secret (in reverse order)
    for (auto it = shared_secrets.rbegin(); it != shared_secrets.rend(); ++it) {
        auto key = it->derive_mu();
        std::vector<uint8_t> nonce(12, 0);

        // ChaCha20 encryption
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<uint8_t> ciphertext(encrypted.size());

        int len = 0;
        EVP_EncryptInit_ex(ctx, EVP_chacha20(), nullptr, key.data(), nonce.data());
        EVP_EncryptUpdate(ctx, ciphertext.data(), &len, encrypted.data(), encrypted.size());
        EVP_CIPHER_CTX_free(ctx);

        encrypted = ciphertext;
    }

    return ErrorOnion(encrypted);
}

ErrorOnion ErrorOnion::decrypt(const SharedSecret& shared_secret) const {
    auto key = shared_secret.derive_mu();
    std::vector<uint8_t> nonce(12, 0);

    // ChaCha20 decryption
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> plaintext(encrypted_failure.size());

    int len = 0;
    EVP_DecryptInit_ex(ctx, EVP_chacha20(), nullptr, key.data(), nonce.data());
    EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                     encrypted_failure.data(), encrypted_failure.size());
    EVP_CIPHER_CTX_free(ctx);

    return ErrorOnion(plaintext);
}

std::vector<uint8_t> ErrorOnion::serialize() const {
    return encrypted_failure;
}

std::optional<ErrorOnion> ErrorOnion::deserialize(const std::vector<uint8_t>& data) {
    return ErrorOnion(data);
}

// ===== FailureMessage Implementation =====

std::vector<uint8_t> FailureMessage::serialize() const {
    std::vector<uint8_t> result;

    // Failure code (2 bytes)
    uint16_t code_val = static_cast<uint16_t>(code);
    result.push_back(static_cast<uint8_t>(code_val & 0xFF));
    result.push_back(static_cast<uint8_t>((code_val >> 8) & 0xFF));

    // Data
    result.insert(result.end(), data.begin(), data.end());

    return result;
}

std::optional<FailureMessage> FailureMessage::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return std::nullopt;
    }

    FailureMessage msg;

    // Failure code
    uint16_t code_val = static_cast<uint16_t>(data[0]) |
                        (static_cast<uint16_t>(data[1]) << 8);
    msg.code = static_cast<FailureCode>(code_val);

    // Data
    if (data.size() > 2) {
        msg.data.assign(data.begin() + 2, data.end());
    }

    return msg;
}

} // namespace onion
} // namespace lightning
} // namespace intcoin
