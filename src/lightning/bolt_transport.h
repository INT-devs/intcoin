// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #8: Encrypted and Authenticated Transport (Noise_XK)

#ifndef INTCOIN_BOLT_TRANSPORT_H
#define INTCOIN_BOLT_TRANSPORT_H

#include "intcoin/types.h"
#include "intcoin/crypto.h"
#include <vector>
#include <array>
#include <memory>

namespace intcoin {
namespace bolt {

// ============================================================================
// BOLT #8: Noise Protocol Framework - Noise_XK Pattern
// ============================================================================

// Noise protocol handshake states
enum class HandshakeState {
    INITIATOR_ACT_ONE,      // Initiator sending act one
    RESPONDER_ACT_TWO,      // Responder sending act two
    INITIATOR_ACT_THREE,    // Initiator sending act three
    COMPLETE                 // Handshake complete
};

// Noise transport state
struct NoiseState {
    // Handshake hash and chaining key
    std::array<uint8_t, 32> h;  // Handshake hash
    std::array<uint8_t, 32> ck; // Chaining key

    // Encryption keys (after handshake)
    std::array<uint8_t, 32> sk; // Sending key
    std::array<uint8_t, 32> rk; // Receiving key

    // Nonces for ChaChaPoly
    uint64_t sn;  // Sending nonce
    uint64_t rn;  // Receiving nonce

    HandshakeState state;

    NoiseState();
};

// BOLT #8 Handshake Acts
struct Act {
    uint8_t version;
    std::vector<uint8_t> payload;
    std::array<uint8_t, 16> tag;  // ChaCha20-Poly1305 tag

    static constexpr size_t ACT_ONE_SIZE = 50;     // 1 + 33 + 16
    static constexpr size_t ACT_TWO_SIZE = 50;     // 1 + 33 + 16
    static constexpr size_t ACT_THREE_SIZE = 66;   // 1 + 33 + 16 + 16

    std::vector<uint8_t> Serialize() const;
    static Result<Act> Deserialize(const std::vector<uint8_t>& data);
};

// Noise Transport Layer (BOLT #8)
class NoiseTransport {
public:
    NoiseTransport();
    ~NoiseTransport();

    // Initiator handshake
    Result<std::vector<uint8_t>> InitiateHandshake(
        const PublicKey& remote_static_key,
        const SecretKey& local_ephemeral_key
    );

    Result<std::vector<uint8_t>> ProcessActTwo(
        const std::vector<uint8_t>& act_two,
        const SecretKey& local_static_key
    );

    // Responder handshake
    Result<std::vector<uint8_t>> ProcessActOne(
        const std::vector<uint8_t>& act_one,
        const SecretKey& local_static_key,
        const SecretKey& local_ephemeral_key
    );

    Result<void> ProcessActThree(
        const std::vector<uint8_t>& act_three
    );

    // Message encryption/decryption (after handshake)
    Result<std::vector<uint8_t>> EncryptMessage(const std::vector<uint8_t>& plaintext);
    Result<std::vector<uint8_t>> DecryptMessage(const std::vector<uint8_t>& ciphertext);

    // State queries
    bool IsHandshakeComplete() const;
    HandshakeState GetState() const;

private:
    NoiseState state_;
    PublicKey remote_static_key_;
    PublicKey local_static_key_;

    // Noise protocol primitives
    void MixHash(const std::vector<uint8_t>& data);
    void MixKey(const std::vector<uint8_t>& data);
    std::array<uint8_t, 32> HKDF(const std::vector<uint8_t>& salt,
                                   const std::vector<uint8_t>& ikm);

    Result<std::vector<uint8_t>> EncryptWithAD(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& plaintext
    );

    Result<std::vector<uint8_t>> DecryptWithAD(
        const std::array<uint8_t, 32>& key,
        uint64_t nonce,
        const std::vector<uint8_t>& ad,
        const std::vector<uint8_t>& ciphertext
    );
};

// Lightning Message Framing (BOLT #8)
struct LightningMessage {
    uint16_t length;
    std::vector<uint8_t> payload;

    static constexpr size_t HEADER_SIZE = 18;  // 2 bytes length + 16 bytes MAC
    static constexpr size_t MAX_MESSAGE_SIZE = 65535;

    std::vector<uint8_t> Serialize() const;
    static Result<LightningMessage> Deserialize(const std::vector<uint8_t>& data);
};

// Secure peer connection using BOLT #8
class SecurePeerConnection {
public:
    SecurePeerConnection();

    // Connect as initiator
    Result<void> ConnectAsInitiator(
        const PublicKey& remote_pubkey,
        const SecretKey& local_privkey
    );

    // Accept as responder
    Result<void> AcceptAsResponder(
        const SecretKey& local_privkey
    );

    // Send/receive messages
    Result<void> SendMessage(const std::vector<uint8_t>& message);
    Result<std::vector<uint8_t>> ReceiveMessage();

    // State
    bool IsConnected() const;
    PublicKey GetRemotePubkey() const;

private:
    std::unique_ptr<NoiseTransport> transport_;
    PublicKey remote_pubkey_;
    bool connected_;

    std::vector<uint8_t> pending_data_;
};

} // namespace bolt
} // namespace intcoin

#endif // INTCOIN_BOLT_TRANSPORT_H
