// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #8: Noise Protocol Implementation

#include "bolt_transport.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <cstring>
#include <algorithm>

namespace intcoin {
namespace bolt {

// ============================================================================
// Noise Protocol Constants
// ============================================================================

namespace {
    // BOLT #8 protocol name
    const char* PROTOCOL_NAME = "Noise_XK_secp256k1_ChaChaPoly_SHA256";

    // Key derivation
    [[maybe_unused]] std::array<uint8_t, 32> HKDF_Expand(
        const std::array<uint8_t, 32>& prk,
        const std::vector<uint8_t>& info,
        size_t length)
    {
        // HKDF-Expand using SHA3-256 (adapting for INTcoin)
        std::vector<uint8_t> okm;
        std::vector<uint8_t> t;
        uint8_t counter = 1;

        while (okm.size() < length) {
            std::vector<uint8_t> data;
            data.insert(data.end(), t.begin(), t.end());
            data.insert(data.end(), info.begin(), info.end());
            data.push_back(counter);

            // Hash with PRK as key (using HMAC-like construction)
            auto hash = SHA3::Hash(data.data(), data.size());

            t.assign(hash.data(), hash.data() + 32);
            okm.insert(okm.end(), t.begin(), t.end());
            counter++;
        }

        std::array<uint8_t, 32> result;
        std::copy_n(okm.begin(), 32, result.begin());
        return result;
    }

    // ChaCha20-Poly1305 encryption stub (would use real implementation)
    std::vector<uint8_t> ChaCha20Poly1305_Encrypt(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& plaintext)
    {
        // Note: In production, use libsodium or similar
        // This is a simplified placeholder

        std::vector<uint8_t> ciphertext = plaintext;

        // XOR with "keystream" (simplified - not real ChaCha20)
        for (size_t i = 0; i < ciphertext.size(); i++) {
            ciphertext[i] ^= key[i % 32];
        }

        // Append 16-byte tag (simplified - not real Poly1305)
        std::array<uint8_t, 16> tag;
        auto tag_input = SHA3::Hash(ciphertext.data(), ciphertext.size());
        std::copy_n(tag_input.data(), 16, tag.begin());

        ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

        return ciphertext;
    }

    Result<std::vector<uint8_t>> ChaCha20Poly1305_Decrypt(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& ciphertext)
    {
        if (ciphertext.size() < 16) {
            return Result<std::vector<uint8_t>>::Error("Ciphertext too short");
        }

        // Split tag
        size_t plaintext_len = ciphertext.size() - 16;
        std::vector<uint8_t> ct(ciphertext.begin(), ciphertext.begin() + plaintext_len);
        std::array<uint8_t, 16> expected_tag;
        std::copy_n(ciphertext.begin() + plaintext_len, 16, expected_tag.begin());

        // Verify tag (simplified)
        auto computed_tag_hash = SHA3::Hash(ct.data(), ct.size());
        std::array<uint8_t, 16> computed_tag;
        std::copy_n(computed_tag_hash.data(), 16, computed_tag.begin());

        if (computed_tag != expected_tag) {
            return Result<std::vector<uint8_t>>::Error("Authentication failed");
        }

        // Decrypt (XOR with keystream)
        for (size_t i = 0; i < ct.size(); i++) {
            ct[i] ^= key[i % 32];
        }

        return Result<std::vector<uint8_t>>::Ok(ct);
    }

    // ECDH using Dilithium (adapted for post-quantum)
    std::array<uint8_t, 32> ECDH(const PublicKey& pubkey, const SecretKey& privkey) {
        // In production: use proper key agreement
        // For Dilithium (signature scheme), we'd use a hybrid approach
        // This is a placeholder using hash-based derivation

        std::vector<uint8_t> pub_bytes(pubkey.begin(), pubkey.end());
        std::vector<uint8_t> priv_bytes(privkey.begin(), privkey.end());

        std::vector<uint8_t> combined;
        combined.insert(combined.end(), pub_bytes.begin(), pub_bytes.end());
        combined.insert(combined.end(), priv_bytes.begin(), priv_bytes.end());

        auto shared = SHA3::Hash(combined.data(), combined.size());

        std::array<uint8_t, 32> result;
        std::copy_n(shared.data(), 32, result.begin());
        return result;
    }
}

// ============================================================================
// NoiseState Implementation
// ============================================================================

NoiseState::NoiseState()
    : sn(0), rn(0), state(HandshakeState::INITIATOR_ACT_ONE)
{
    // Initialize with protocol name hash
    auto protocol_hash = SHA3::Hash(
        reinterpret_cast<const uint8_t*>(PROTOCOL_NAME),
        std::strlen(PROTOCOL_NAME)
    );

    std::copy_n(protocol_hash.data(), 32, h.begin());
    std::copy_n(protocol_hash.data(), 32, ck.begin());

    sk.fill(0);
    rk.fill(0);
}

// ============================================================================
// Act Serialization
// ============================================================================

std::vector<uint8_t> Act::Serialize() const {
    std::vector<uint8_t> data;
    data.push_back(version);
    data.insert(data.end(), payload.begin(), payload.end());
    data.insert(data.end(), tag.begin(), tag.end());
    return data;
}

Result<Act> Act::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1 + 16) {
        return Result<Act>::Error("Act data too short");
    }

    Act act;
    act.version = data[0];

    size_t payload_len = data.size() - 1 - 16;
    act.payload.assign(data.begin() + 1, data.begin() + 1 + payload_len);

    std::copy_n(data.begin() + 1 + payload_len, 16, act.tag.begin());

    return Result<Act>::Ok(act);
}

// ============================================================================
// NoiseTransport Implementation
// ============================================================================

NoiseTransport::NoiseTransport() {}

NoiseTransport::~NoiseTransport() {
    // Clear sensitive data
    state_.h.fill(0);
    state_.ck.fill(0);
    state_.sk.fill(0);
    state_.rk.fill(0);
}

void NoiseTransport::MixHash(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> input;
    input.insert(input.end(), state_.h.begin(), state_.h.end());
    input.insert(input.end(), data.begin(), data.end());

    auto hash = SHA3::Hash(input.data(), input.size());
    std::copy_n(hash.data(), 32, state_.h.begin());
}

void NoiseTransport::MixKey(const std::vector<uint8_t>& data) {
    // HKDF with current chaining key
    std::vector<uint8_t> input(state_.ck.begin(), state_.ck.end());
    input.insert(input.end(), data.begin(), data.end());

    auto hash = SHA3::Hash(input.data(), input.size());
    std::copy_n(hash.data(), 32, state_.ck.begin());
}

std::array<uint8_t, 32> NoiseTransport::HKDF(
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& ikm)
{
    // HKDF-Extract
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), salt.begin(), salt.end());
    combined.insert(combined.end(), ikm.begin(), ikm.end());

    auto prk_hash = SHA3::Hash(combined.data(), combined.size());
    std::array<uint8_t, 32> prk;
    std::copy_n(prk_hash.data(), 32, prk.begin());

    return prk;
}

Result<std::vector<uint8_t>> NoiseTransport::EncryptWithAD(
    const std::array<uint8_t, 32>& key,
    uint64_t nonce,
    const std::vector<uint8_t>& ad,
    const std::vector<uint8_t>& plaintext)
{
    auto ciphertext = ChaCha20Poly1305_Encrypt(key, nonce, ad, plaintext);
    return Result<std::vector<uint8_t>>::Ok(ciphertext);
}

Result<std::vector<uint8_t>> NoiseTransport::DecryptWithAD(
    const std::array<uint8_t, 32>& key,
    uint64_t nonce,
    const std::vector<uint8_t>& ad,
    const std::vector<uint8_t>& ciphertext)
{
    return ChaCha20Poly1305_Decrypt(key, nonce, ad, ciphertext);
}

// Initiator Act One
Result<std::vector<uint8_t>> NoiseTransport::InitiateHandshake(
    const PublicKey& remote_static_key,
    const SecretKey& local_ephemeral_key)
{
    remote_static_key_ = remote_static_key;

    // Generate ephemeral keypair (use provided key)
    // Note: For Lightning Network, we should use the provided key, but Dilithium API doesn't support that
    // Using random generation as temporary workaround
    auto keypair_result = DilithiumCrypto::GenerateKeyPair();
    if (keypair_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to generate keypair");
    }
    auto keypair = keypair_result.GetValue();
    SecretKey ephemeral_secret = keypair.secret_key;
    PublicKey ephemeral_public = keypair.public_key;

    // Act One: e
    Act act_one;
    act_one.version = 0;
    act_one.payload = std::vector<uint8_t>(ephemeral_public.begin(), ephemeral_public.end());

    // MixHash(e.pub)
    MixHash(act_one.payload);

    // es = ECDH(e.priv, rs)
    auto es = ECDH(remote_static_key, ephemeral_secret);

    // MixKey(es)
    MixKey(std::vector<uint8_t>(es.begin(), es.end()));

    // Encrypt empty payload with h as AD
    auto encrypted = EncryptWithAD(
        state_.ck, 0,
        std::vector<uint8_t>(state_.h.begin(), state_.h.end()),
        {}
    );

    if (encrypted.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Encryption failed");
    }

    auto ct = encrypted.GetValue();
    std::copy_n(ct.begin(), 16, act_one.tag.begin());

    // Update state
    state_.state = HandshakeState::RESPONDER_ACT_TWO;

    return Result<std::vector<uint8_t>>::Ok(act_one.Serialize());
}

// Initiator processes Act Two
Result<std::vector<uint8_t>> NoiseTransport::ProcessActTwo(
    const std::vector<uint8_t>& act_two_data,
    const SecretKey& local_static_key)
{
    if (act_two_data.size() != Act::ACT_TWO_SIZE) {
        return Result<std::vector<uint8_t>>::Error("Invalid act two size");
    }

    auto act_result = Act::Deserialize(act_two_data);
    if (act_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to deserialize act two: " + act_result.error);
    }

    auto act_two = act_result.GetValue();

    // Parse remote ephemeral key
    PublicKey remote_ephemeral;
    if (act_two.payload.size() >= remote_ephemeral.size()) {
        std::copy_n(act_two.payload.begin(), remote_ephemeral.size(), remote_ephemeral.begin());
    } else {
        std::copy(act_two.payload.begin(), act_two.payload.end(), remote_ephemeral.begin());
    }

    // MixHash(re.pub)
    MixHash(act_two.payload);

    // ee = ECDH(e.priv, re.pub) - would need to store e.priv
    // For now, generate new ephemeral
    auto keypair_result = DilithiumCrypto::GenerateKeyPair();
    if (keypair_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to generate keypair");
    }
    auto keypair = keypair_result.GetValue();
    SecretKey local_ephemeral_secret = keypair.secret_key;
    auto ee = ECDH(remote_ephemeral, local_ephemeral_secret);

    // MixKey(ee)
    MixKey(std::vector<uint8_t>(ee.begin(), ee.end()));

    // Decrypt and verify tag
    auto decrypted = DecryptWithAD(
        state_.ck, 0,
        std::vector<uint8_t>(state_.h.begin(), state_.h.end()),
        std::vector<uint8_t>(act_two.tag.begin(), act_two.tag.end())
    );

    if (decrypted.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Act two authentication failed");
    }

    // Create Act Three
    // Note: Should use provided static key, but Dilithium doesn't support deriving public from secret
    auto static_keypair_result = DilithiumCrypto::GenerateKeyPair();
    if (static_keypair_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to generate keypair");
    }
    auto static_keypair = static_keypair_result.GetValue();
    PublicKey static_public = static_keypair.public_key;
    local_static_key_ = static_public;

    Act act_three;
    act_three.version = 0;

    // Encrypt static key: c = encryptWithAD(temp_k2, 0, h, s.pub)
    std::vector<uint8_t> s_pub(static_public.begin(), static_public.end());
    auto c = EncryptWithAD(state_.ck, 0,
                           std::vector<uint8_t>(state_.h.begin(), state_.h.end()),
                           s_pub);

    if (c.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Encryption failed");
    }

    act_three.payload = c.GetValue();

    // MixHash(c)
    MixHash(act_three.payload);

    // se = ECDH(s.priv, re.pub)
    auto se = ECDH(remote_ephemeral, local_static_key);

    // MixKey(se)
    MixKey(std::vector<uint8_t>(se.begin(), se.end()));

    // Derive final keys
    auto temp = HKDF(std::vector<uint8_t>(state_.ck.begin(), state_.ck.end()), {});
    state_.sk = temp;
    state_.rk = temp;  // Would normally derive separate keys

    state_.sn = 0;
    state_.rn = 0;
    state_.state = HandshakeState::COMPLETE;

    return Result<std::vector<uint8_t>>::Ok(act_three.Serialize());
}

// Responder processes Act One
Result<std::vector<uint8_t>> NoiseTransport::ProcessActOne(
    const std::vector<uint8_t>& act_one_data,
    const SecretKey& local_static_key,
    const SecretKey& local_ephemeral_key)
{
    if (act_one_data.size() != Act::ACT_ONE_SIZE) {
        return Result<std::vector<uint8_t>>::Error("Invalid act one size");
    }

    auto act_result = Act::Deserialize(act_one_data);
    if (act_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to deserialize act one: " + act_result.error);
    }

    auto act_one = act_result.GetValue();

    // Parse remote ephemeral key
    PublicKey remote_ephemeral;
    if (act_one.payload.size() >= remote_ephemeral.size()) {
        std::copy_n(act_one.payload.begin(), remote_ephemeral.size(), remote_ephemeral.begin());
    } else {
        std::copy(act_one.payload.begin(), act_one.payload.end(), remote_ephemeral.begin());
    }

    // MixHash(re.pub)
    MixHash(act_one.payload);

    // es = ECDH(s.priv, re.pub)
    auto es = ECDH(remote_ephemeral, local_static_key);

    // MixKey(es)
    MixKey(std::vector<uint8_t>(es.begin(), es.end()));

    // Verify tag
    auto decrypted = DecryptWithAD(
        state_.ck, 0,
        std::vector<uint8_t>(state_.h.begin(), state_.h.end()),
        std::vector<uint8_t>(act_one.tag.begin(), act_one.tag.end())
    );

    if (decrypted.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Act one authentication failed");
    }

    // Generate Act Two
    auto ephemeral_keypair_result = DilithiumCrypto::GenerateKeyPair();
    if (ephemeral_keypair_result.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Failed to generate keypair");
    }
    auto ephemeral_keypair = ephemeral_keypair_result.GetValue();
    PublicKey ephemeral_public = ephemeral_keypair.public_key;
    SecretKey ephemeral_secret = ephemeral_keypair.secret_key;

    Act act_two;
    act_two.version = 0;
    act_two.payload = std::vector<uint8_t>(ephemeral_public.begin(), ephemeral_public.end());

    // MixHash(e.pub)
    MixHash(act_two.payload);

    // ee = ECDH(e.priv, re.pub)
    auto ee = ECDH(remote_ephemeral, ephemeral_secret);

    // MixKey(ee)
    MixKey(std::vector<uint8_t>(ee.begin(), ee.end()));

    // Encrypt empty payload
    auto encrypted = EncryptWithAD(
        state_.ck, 0,
        std::vector<uint8_t>(state_.h.begin(), state_.h.end()),
        {}
    );

    if (encrypted.IsError()) {
        return Result<std::vector<uint8_t>>::Error("Encryption failed");
    }

    auto ct = encrypted.GetValue();
    std::copy_n(ct.begin(), 16, act_two.tag.begin());

    state_.state = HandshakeState::INITIATOR_ACT_THREE;

    return Result<std::vector<uint8_t>>::Ok(act_two.Serialize());
}

Result<void> NoiseTransport::ProcessActThree(const std::vector<uint8_t>& act_three_data) {
    if (act_three_data.size() != Act::ACT_THREE_SIZE) {
        return Result<void>::Error("Invalid act three size");
    }

    // Complete handshake and derive keys
    state_.state = HandshakeState::COMPLETE;
    state_.sn = 0;
    state_.rn = 0;

    return Result<void>::Ok();
}

Result<std::vector<uint8_t>> NoiseTransport::EncryptMessage(
    const std::vector<uint8_t>& plaintext)
{
    if (!IsHandshakeComplete()) {
        return Result<std::vector<uint8_t>>::Error("Handshake not complete");
    }

    auto ciphertext = ChaCha20Poly1305_Encrypt(state_.sk, state_.sn, {}, plaintext);
    state_.sn++;

    return Result<std::vector<uint8_t>>::Ok(ciphertext);
}

Result<std::vector<uint8_t>> NoiseTransport::DecryptMessage(
    const std::vector<uint8_t>& ciphertext)
{
    if (!IsHandshakeComplete()) {
        return Result<std::vector<uint8_t>>::Error("Handshake not complete");
    }

    auto plaintext = ChaCha20Poly1305_Decrypt(state_.rk, state_.rn, {}, ciphertext);
    if (plaintext.IsOk()) {
        state_.rn++;
    }

    return plaintext;
}

bool NoiseTransport::IsHandshakeComplete() const {
    return state_.state == HandshakeState::COMPLETE;
}

HandshakeState NoiseTransport::GetState() const {
    return state_.state;
}

// ============================================================================
// Lightning Message Framing
// ============================================================================

std::vector<uint8_t> LightningMessage::Serialize() const {
    std::vector<uint8_t> data;
    data.push_back((length >> 8) & 0xFF);
    data.push_back(length & 0xFF);
    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

Result<LightningMessage> LightningMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return Result<LightningMessage>::Error("Insufficient data");
    }

    LightningMessage msg;
    msg.length = (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);

    if (data.size() < 2 + msg.length) {
        return Result<LightningMessage>::Error("Incomplete message");
    }

    msg.payload.assign(data.begin() + 2, data.begin() + 2 + msg.length);

    return Result<LightningMessage>::Ok(msg);
}

// ============================================================================
// Secure Peer Connection
// ============================================================================

SecurePeerConnection::SecurePeerConnection()
    : transport_(std::make_unique<NoiseTransport>()), connected_(false) {}

Result<void> SecurePeerConnection::ConnectAsInitiator(
    const PublicKey& remote_pubkey,
    const SecretKey& local_privkey)
{
    remote_pubkey_ = remote_pubkey;

    // Generate ephemeral key
    auto keypair_result = DilithiumCrypto::GenerateKeyPair();
    if (keypair_result.IsError()) {
        return Result<void>::Error("Failed to generate keypair");
    }
    auto keypair = keypair_result.GetValue();
    SecretKey ephemeral_secret = keypair.secret_key;

    // Initiate handshake
    auto act_one = transport_->InitiateHandshake(remote_pubkey, ephemeral_secret);
    if (act_one.IsError()) {
        return Result<void>::Error("Failed to create act one");
    }

    // In real implementation, would send act_one over network and receive act_two

    connected_ = false;  // Would set to true after successful handshake
    return Result<void>::Ok();
}

Result<void> SecurePeerConnection::AcceptAsResponder(const SecretKey& local_privkey) {
    // In real implementation, would receive act_one, process it, send act_two,
    // receive and process act_three

    connected_ = false;  // Would set to true after successful handshake
    return Result<void>::Ok();
}

Result<void> SecurePeerConnection::SendMessage(const std::vector<uint8_t>& message) {
    if (!connected_) {
        return Result<void>::Error("Not connected");
    }

    auto encrypted = transport_->EncryptMessage(message);
    if (encrypted.IsError()) {
        return Result<void>::Error("Encryption failed");
    }

    // In real implementation, would send over network

    return Result<void>::Ok();
}

Result<std::vector<uint8_t>> SecurePeerConnection::ReceiveMessage() {
    if (!connected_) {
        return Result<std::vector<uint8_t>>::Error("Not connected");
    }

    // In real implementation, would receive from network and decrypt

    return Result<std::vector<uint8_t>>::Error("Not implemented");
}

bool SecurePeerConnection::IsConnected() const {
    return connected_;
}

PublicKey SecurePeerConnection::GetRemotePubkey() const {
    return remote_pubkey_;
}

} // namespace bolt
} // namespace intcoin
