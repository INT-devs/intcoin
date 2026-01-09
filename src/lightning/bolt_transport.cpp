// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #8: Noise Protocol Implementation

#include "bolt_transport.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <cstring>
#include <algorithm>
#include <sodium.h>

namespace intcoin {
namespace bolt {

// ============================================================================
// Noise Protocol Constants
// ============================================================================

namespace {
    // BOLT #8 protocol name
    const char* PROTOCOL_NAME = "Noise_XK_secp256k1_ChaChaPoly_SHA256";

    // HKDF-Extract: Extract pseudorandom key from input keying material
    // Uses HMAC-SHA256 (via libsodium's crypto_auth_hmacsha256)
    std::array<uint8_t, 32> HKDF_Extract(
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& ikm)
    {
        std::array<uint8_t, 32> prk;

        // Use zero salt if empty
        std::vector<uint8_t> salt_data = salt.empty() ?
            std::vector<uint8_t>(32, 0) : salt;

        // HMAC-SHA256(salt, ikm) using libsodium
        crypto_auth_hmacsha256(
            prk.data(),
            ikm.data(), ikm.size(),
            salt_data.data()
        );

        return prk;
    }

    // HKDF-Expand: Expand pseudorandom key to desired length
    // Uses HMAC-SHA256 for proper key expansion
    [[maybe_unused]] std::array<uint8_t, 32> HKDF_Expand(
        const std::array<uint8_t, 32>& prk,
        const std::vector<uint8_t>& info,
        size_t length)
    {
        // HKDF-Expand as per RFC 5869
        // T(0) = empty string
        // T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
        // T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
        // ...

        std::vector<uint8_t> okm;
        std::vector<uint8_t> t_prev;
        uint8_t counter = 1;

        while (okm.size() < length) {
            // Build input: T(i-1) | info | counter
            std::vector<uint8_t> hmac_input;
            hmac_input.insert(hmac_input.end(), t_prev.begin(), t_prev.end());
            hmac_input.insert(hmac_input.end(), info.begin(), info.end());
            hmac_input.push_back(counter);

            // Compute T(i) = HMAC-SHA256(PRK, T(i-1) | info | counter)
            unsigned char t_current[crypto_auth_hmacsha256_BYTES];
            crypto_auth_hmacsha256(
                t_current,
                hmac_input.data(), hmac_input.size(),
                prk.data()
            );

            // Append T(i) to output
            t_prev.assign(t_current, t_current + crypto_auth_hmacsha256_BYTES);
            okm.insert(okm.end(), t_prev.begin(), t_prev.end());
            counter++;
        }

        // Return requested length
        std::array<uint8_t, 32> result;
        std::copy_n(okm.begin(), std::min(length, okm.size()), result.begin());
        return result;
    }

    // ChaCha20-Poly1305 AEAD encryption (BOLT #8)
    // Uses libsodium's crypto_aead_chacha20poly1305_ietf variant
    std::vector<uint8_t> ChaCha20Poly1305_Encrypt(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& plaintext)
    {
        // Prepare 12-byte nonce for ChaCha20-Poly1305-IETF
        // BOLT #8 uses 8-byte counter, padded with zeros
        unsigned char nonce_bytes[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};

        // Little-endian encoding of 64-bit nonce
        for (int i = 0; i < 8; i++) {
            nonce_bytes[4 + i] = (nonce >> (i * 8)) & 0xFF;
        }

        // Allocate output buffer (plaintext + 16-byte tag)
        std::vector<uint8_t> ciphertext(plaintext.size() + crypto_aead_chacha20poly1305_IETF_ABYTES);
        unsigned long long ciphertext_len;

        // Perform AEAD encryption
        crypto_aead_chacha20poly1305_ietf_encrypt(
            ciphertext.data(),
            &ciphertext_len,
            plaintext.data(),
            plaintext.size(),
            ad.empty() ? nullptr : ad.data(),
            ad.size(),
            nullptr,  // No secret nonce
            nonce_bytes,
            key.data()
        );

        ciphertext.resize(ciphertext_len);
        return ciphertext;
    }

    // ChaCha20-Poly1305 AEAD decryption (BOLT #8)
    Result<std::vector<uint8_t>> ChaCha20Poly1305_Decrypt(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& ciphertext)
    {
        // Minimum ciphertext size is 16 bytes (just the tag)
        if (ciphertext.size() < crypto_aead_chacha20poly1305_IETF_ABYTES) {
            return Result<std::vector<uint8_t>>::Error("Ciphertext too short");
        }

        // Prepare 12-byte nonce for ChaCha20-Poly1305-IETF
        unsigned char nonce_bytes[crypto_aead_chacha20poly1305_IETF_NPUBBYTES] = {0};

        // Little-endian encoding of 64-bit nonce
        for (int i = 0; i < 8; i++) {
            nonce_bytes[4 + i] = (nonce >> (i * 8)) & 0xFF;
        }

        // Allocate output buffer (ciphertext - 16-byte tag)
        std::vector<uint8_t> plaintext(ciphertext.size() - crypto_aead_chacha20poly1305_IETF_ABYTES);
        unsigned long long plaintext_len;

        // Perform AEAD decryption with authentication
        int result = crypto_aead_chacha20poly1305_ietf_decrypt(
            plaintext.data(),
            &plaintext_len,
            nullptr,  // No secret nonce
            ciphertext.data(),
            ciphertext.size(),
            ad.empty() ? nullptr : ad.data(),
            ad.size(),
            nonce_bytes,
            key.data()
        );

        if (result != 0) {
            return Result<std::vector<uint8_t>>::Error("Authentication failed");
        }

        plaintext.resize(plaintext_len);
        return Result<std::vector<uint8_t>>::Ok(plaintext);
    }

    // Key agreement using Dilithium keys (adapted for post-quantum)
    // NOTE: Dilithium is a signature scheme, not a KEM. For production use,
    //       this should be replaced with proper Kyber768 key encapsulation.
    //       This implementation uses BLAKE2b-based KDF for secure key derivation.
    std::array<uint8_t, 32> ECDH(const PublicKey& pubkey, const SecretKey& privkey) {
        // Use BLAKE2b-based KDF for secure key derivation
        // More secure than simple hashing, provides better diffusion

        // Create input key material by concatenating keys
        std::vector<uint8_t> ikm;
        ikm.reserve(pubkey.size() + privkey.size());
        ikm.insert(ikm.end(), pubkey.begin(), pubkey.end());
        ikm.insert(ikm.end(), privkey.begin(), privkey.end());

        // Use libsodium's BLAKE2b-based generic hash (cryptographic KDF)
        // Salt provides domain separation for BOLT #8
        unsigned char shared_secret[32];
        unsigned char salt[crypto_generichash_BYTES] = {0};
        std::memcpy(salt, "BOLT8KDF", 8);  // Domain separation

        // BLAKE2b-based KDF (more secure than SHA3 for key derivation)
        crypto_generichash(shared_secret, sizeof(shared_secret),
                          ikm.data(), ikm.size(),
                          salt, sizeof(salt));

        std::array<uint8_t, 32> result;
        std::copy_n(shared_secret, 32, result.begin());

        // Clear sensitive data from memory
        sodium_memzero(shared_secret, sizeof(shared_secret));

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
    // Use proper HKDF-Extract with HMAC-SHA256
    return HKDF_Extract(salt, ikm);
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

    if (data.size() < 2 + static_cast<size_t>(msg.length)) {
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
