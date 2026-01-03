// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_PRIVACY_STEALTH_ADDRESSES_H
#define INTCOIN_PRIVACY_STEALTH_ADDRESSES_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <array>

namespace intcoin {
namespace privacy {

/**
 * @brief Stealth Addresses for Recipient Privacy
 *
 * Implements stealth addresses (Dual-Key Stealth Address Protocol) to hide
 * transaction recipients. Each payment creates a unique one-time address.
 *
 * Based on Monero's stealth address implementation.
 */

// 32-byte keys
using PrivateKey = std::array<uint8_t, 32>;
using PublicKey = std::array<uint8_t, 32>;
using SharedSecret = std::array<uint8_t, 32>;

// Stealth address consists of two public keys
struct StealthAddress {
    PublicKey view_public_key;    // For scanning transactions
    PublicKey spend_public_key;   // For spending outputs
    std::string encoded;          // Bech32-encoded address
};

// Private keys for stealth address
struct StealthPrivateKeys {
    PrivateKey view_private_key;
    PrivateKey spend_private_key;
};

// One-time address created for a payment
struct OneTimeAddress {
    PublicKey public_key;         // One-time public key
    PublicKey tx_public_key;      // Transaction public key (ephemeral)
    std::vector<uint8_t> encrypted_payment_id; // Optional encrypted payment ID
};

// Received output information
struct ReceivedOutput {
    OneTimeAddress one_time_address;
    uint64_t amount;              // Decrypted amount
    uint64_t output_index;
    std::string tx_hash;
    PrivateKey output_private_key; // Derived private key for spending
};

/**
 * @class StealthAddressManager
 * @brief Manages stealth address generation and scanning
 */
class StealthAddressManager {
public:
    StealthAddressManager();
    ~StealthAddressManager();

    /**
     * Generate new stealth address (view + spend key pairs)
     */
    struct StealthKeyPair {
        StealthAddress address;
        StealthPrivateKeys private_keys;
    };

    StealthKeyPair GenerateStealthAddress();

    /**
     * Encode stealth address to string (Bech32)
     */
    std::string EncodeAddress(const StealthAddress& address, const std::string& hrp = "ints") const;

    /**
     * Decode stealth address from string
     */
    StealthAddress DecodeAddress(const std::string& encoded_address) const;

    /**
     * Create one-time address for sending to stealth address
     *
     * @param stealth_address Recipient's stealth address
     * @param tx_private_key Ephemeral private key for this transaction
     * @param output_index Index of this output in the transaction
     * @return One-time address to use as output
     */
    OneTimeAddress CreateOneTimeAddress(
        const StealthAddress& stealth_address,
        const PrivateKey& tx_private_key,
        uint32_t output_index
    );

    /**
     * Scan transaction for outputs belonging to stealth address
     *
     * @param stealth_keys Private keys for scanning
     * @param tx_public_key Transaction's public key
     * @param outputs All one-time addresses in the transaction
     * @return Vector of received outputs (empty if none belong to us)
     */
    std::vector<ReceivedOutput> ScanTransaction(
        const StealthPrivateKeys& stealth_keys,
        const PublicKey& tx_public_key,
        const std::vector<OneTimeAddress>& outputs
    );

    /**
     * Check if specific output belongs to stealth address
     */
    bool IsOutputMine(
        const StealthPrivateKeys& stealth_keys,
        const PublicKey& tx_public_key,
        const OneTimeAddress& output,
        uint32_t output_index
    );

    /**
     * Derive private key for spending a received output
     */
    PrivateKey DeriveOutputPrivateKey(
        const StealthPrivateKeys& stealth_keys,
        const PublicKey& tx_public_key,
        uint32_t output_index
    );

    /**
     * Generate shared secret (ECDH)
     */
    SharedSecret GenerateSharedSecret(
        const PrivateKey& private_key,
        const PublicKey& public_key
    ) const;

    /**
     * Payment IDs (encrypted)
     */
    struct PaymentId {
        std::array<uint8_t, 8> id;  // 8-byte payment ID
    };

    std::vector<uint8_t> EncryptPaymentId(
        const PaymentId& payment_id,
        const PublicKey& tx_public_key,
        const PrivateKey& view_private_key
    ) const;

    PaymentId DecryptPaymentId(
        const std::vector<uint8_t>& encrypted_payment_id,
        const PublicKey& tx_public_key,
        const PrivateKey& view_private_key
    ) const;

    /**
     * Subaddresses (for multiple receiving addresses from one seed)
     */
    struct Subaddress {
        uint32_t account{0};
        uint32_t index{0};
        StealthAddress address;
    };

    Subaddress DeriveSubaddress(
        const StealthPrivateKeys& master_keys,
        uint32_t account,
        uint32_t index
    );

    std::vector<Subaddress> DeriveSubaddresses(
        const StealthPrivateKeys& master_keys,
        uint32_t account,
        uint32_t start_index,
        uint32_t count
    );

    // Statistics
    struct StealthStats {
        uint64_t total_addresses_generated{0};
        uint64_t total_outputs_scanned{0};
        uint64_t total_outputs_received{0};
        uint64_t total_subaddresses{0};
    };

    StealthStats GetStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace privacy
} // namespace intcoin

#endif // INTCOIN_PRIVACY_STEALTH_ADDRESSES_H
