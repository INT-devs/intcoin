// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/bridge/atomic_swap.h"
#include "../../include/intcoin/crypto/hash.h"
#include <openssl/rand.h>
#include <ctime>
#include <sstream>

namespace intcoin {
namespace bridge {

//==============================================================================
// HTLC Implementation
//==============================================================================

bool HTLC::verify_secret(const Hash256& preimage) const {
    // Hash the preimage and compare with hash_lock
    crypto::SHA256 hasher;
    hasher.update(preimage.data(), preimage.size());

    Hash256 computed_hash;
    hasher.finalize(computed_hash.data());

    return computed_hash == hash_lock;
}

bool HTLC::is_timelocked(uint32_t current_height) const {
    // Check if current time/height is past the timelock
    return current_height < time_lock;
}

//==============================================================================
// HTLC Utility Functions
//==============================================================================

namespace htlc {

/**
 * Generate a random secret for HTLC
 */
Hash256 generate_secret() {
    Hash256 secret;
    RAND_bytes(secret.data(), secret.size());
    return secret;
}

/**
 * Create hash lock from secret
 */
Hash256 create_hash_lock(const Hash256& secret) {
    crypto::SHA256 hasher;
    hasher.update(secret.data(), secret.size());

    Hash256 hash_lock;
    hasher.finalize(hash_lock.data());

    return hash_lock;
}

/**
 * Create a new HTLC
 */
HTLC create_htlc(
    const PublicKey& sender,
    const PublicKey& receiver,
    uint64_t amount,
    uint32_t time_lock,
    ChainType chain,
    const Hash256& hash_lock
) {
    HTLC htlc;
    htlc.sender = sender;
    htlc.receiver = receiver;
    htlc.amount = amount;
    htlc.time_lock = time_lock;
    htlc.chain = chain;
    htlc.hash_lock = hash_lock;
    htlc.secret_revealed = false;

    return htlc;
}

/**
 * Create HTLC with generated secret
 */
std::pair<HTLC, Hash256> create_htlc_with_secret(
    const PublicKey& sender,
    const PublicKey& receiver,
    uint64_t amount,
    uint32_t time_lock,
    ChainType chain
) {
    // Generate secret
    Hash256 secret = generate_secret();

    // Create hash lock
    Hash256 hash_lock = create_hash_lock(secret);

    // Create HTLC
    HTLC htlc = create_htlc(sender, receiver, amount, time_lock, chain, hash_lock);

    return {htlc, secret};
}

/**
 * Verify HTLC secret
 */
bool verify_htlc_secret(const HTLC& htlc, const Hash256& secret) {
    return htlc.verify_secret(secret);
}

/**
 * Check if HTLC is expired
 */
bool is_htlc_expired(const HTLC& htlc, uint32_t current_time) {
    return current_time >= htlc.time_lock;
}

/**
 * Check if HTLC is still locked
 */
bool is_htlc_locked(const HTLC& htlc, uint32_t current_time) {
    return !is_htlc_expired(htlc, current_time) && !htlc.secret_revealed;
}

/**
 * Claim HTLC with secret
 */
bool claim_htlc(HTLC& htlc, const Hash256& secret) {
    // Verify secret matches hash lock
    if (!verify_htlc_secret(htlc, secret)) {
        return false;
    }

    // Check HTLC is not expired
    uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));
    if (is_htlc_expired(htlc, current_time)) {
        return false;
    }

    // Reveal secret
    htlc.secret = secret;
    htlc.secret_revealed = true;

    return true;
}

/**
 * Refund HTLC after timeout
 */
bool refund_htlc(const HTLC& htlc, uint32_t current_time) {
    // Can only refund if expired and secret not revealed
    return is_htlc_expired(htlc, current_time) && !htlc.secret_revealed;
}

/**
 * Calculate safe timelock duration for chain
 */
uint32_t calculate_safe_timelock(ChainType chain, uint32_t confirmations) {
    uint32_t block_time;

    switch (chain) {
        case ChainType::BITCOIN:
            block_time = 600;  // 10 minutes
            break;
        case ChainType::ETHEREUM:
            block_time = 15;   // 15 seconds
            break;
        case ChainType::LITECOIN:
            block_time = 150;  // 2.5 minutes
            break;
        case ChainType::MONERO:
            block_time = 120;  // 2 minutes
            break;
        case ChainType::CARDANO:
            block_time = 20;   // 20 seconds
            break;
        case ChainType::INTCOIN:
            block_time = 60;   // 1 minute
            break;
        default:
            block_time = 60;
    }

    // Calculate timelock with buffer
    uint32_t base_time = confirmations * block_time;
    uint32_t buffer = base_time / 2;  // 50% buffer

    return base_time + buffer;
}

/**
 * Get recommended confirmations for chain
 */
uint32_t get_recommended_confirmations(ChainType chain) {
    switch (chain) {
        case ChainType::BITCOIN:
            return 6;   // ~1 hour
        case ChainType::ETHEREUM:
            return 12;  // ~3 minutes
        case ChainType::LITECOIN:
            return 12;  // ~30 minutes
        case ChainType::MONERO:
            return 10;  // ~20 minutes
        case ChainType::CARDANO:
            return 15;  // ~5 minutes
        case ChainType::INTCOIN:
            return 6;   // ~6 minutes
        default:
            return 6;
    }
}

/**
 * Create timelock for HTLC
 */
uint32_t create_timelock(ChainType chain) {
    uint32_t confirmations = get_recommended_confirmations(chain);
    uint32_t duration = calculate_safe_timelock(chain, confirmations);
    uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));

    return current_time + duration;
}

/**
 * Serialize HTLC to bytes
 */
std::vector<uint8_t> serialize_htlc(const HTLC& htlc) {
    std::vector<uint8_t> data;

    // Hash lock (32 bytes)
    data.insert(data.end(), htlc.hash_lock.begin(), htlc.hash_lock.end());

    // Secret (32 bytes) - if revealed
    if (htlc.secret_revealed) {
        data.insert(data.end(), htlc.secret.begin(), htlc.secret.end());
    } else {
        data.insert(data.end(), 32, 0);  // Padding
    }

    // Sender (33 bytes compressed public key)
    data.insert(data.end(), htlc.sender.begin(), htlc.sender.end());

    // Receiver (33 bytes compressed public key)
    data.insert(data.end(), htlc.receiver.begin(), htlc.receiver.end());

    // Amount (8 bytes)
    for (int i = 0; i < 8; ++i) {
        data.push_back((htlc.amount >> (i * 8)) & 0xFF);
    }

    // Time lock (4 bytes)
    for (int i = 0; i < 4; ++i) {
        data.push_back((htlc.time_lock >> (i * 8)) & 0xFF);
    }

    // Chain type (1 byte)
    data.push_back(static_cast<uint8_t>(htlc.chain));

    // Secret revealed flag (1 byte)
    data.push_back(htlc.secret_revealed ? 1 : 0);

    return data;
}

/**
 * Deserialize HTLC from bytes
 */
std::optional<HTLC> deserialize_htlc(const std::vector<uint8_t>& data) {
    // Minimum size: 32 + 32 + 33 + 33 + 8 + 4 + 1 + 1 = 144 bytes
    if (data.size() < 144) {
        return std::nullopt;
    }

    HTLC htlc;
    size_t offset = 0;

    // Hash lock (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, htlc.hash_lock.begin());
    offset += 32;

    // Secret (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, htlc.secret.begin());
    offset += 32;

    // Sender (33 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 33, htlc.sender.begin());
    offset += 33;

    // Receiver (33 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 33, htlc.receiver.begin());
    offset += 33;

    // Amount (8 bytes)
    htlc.amount = 0;
    for (int i = 0; i < 8; ++i) {
        htlc.amount |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;

    // Time lock (4 bytes)
    htlc.time_lock = 0;
    for (int i = 0; i < 4; ++i) {
        htlc.time_lock |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
    }
    offset += 4;

    // Chain type (1 byte)
    htlc.chain = static_cast<ChainType>(data[offset]);
    offset += 1;

    // Secret revealed flag (1 byte)
    htlc.secret_revealed = (data[offset] != 0);

    return htlc;
}

/**
 * Create HTLC script (Bitcoin-style)
 */
std::string create_htlc_script(const HTLC& htlc) {
    // Simplified script representation
    // In production: create actual Bitcoin/Ethereum script

    std::ostringstream ss;
    ss << "OP_IF\n";
    ss << "  OP_SHA256\n";
    ss << "  <hash_lock>\n";
    ss << "  OP_EQUALVERIFY\n";
    ss << "  <receiver_pubkey>\n";
    ss << "  OP_CHECKSIG\n";
    ss << "OP_ELSE\n";
    ss << "  <timelock>\n";
    ss << "  OP_CHECKLOCKTIMEVERIFY\n";
    ss << "  OP_DROP\n";
    ss << "  <sender_pubkey>\n";
    ss << "  OP_CHECKSIG\n";
    ss << "OP_ENDIF\n";

    return ss.str();
}

/**
 * Verify HTLC matches expected parameters
 */
bool verify_htlc_parameters(
    const HTLC& htlc,
    const PublicKey& expected_sender,
    const PublicKey& expected_receiver,
    uint64_t expected_amount,
    const Hash256& expected_hash_lock
) {
    return htlc.sender == expected_sender &&
           htlc.receiver == expected_receiver &&
           htlc.amount == expected_amount &&
           htlc.hash_lock == expected_hash_lock;
}

/**
 * Get HTLC status string
 */
std::string get_htlc_status(const HTLC& htlc, uint32_t current_time) {
    if (htlc.secret_revealed) {
        return "CLAIMED";
    } else if (is_htlc_expired(htlc, current_time)) {
        return "EXPIRED";
    } else {
        return "LOCKED";
    }
}

/**
 * Calculate remaining time for HTLC
 */
uint32_t get_htlc_remaining_time(const HTLC& htlc, uint32_t current_time) {
    if (current_time >= htlc.time_lock) {
        return 0;
    }
    return htlc.time_lock - current_time;
}

} // namespace htlc

} // namespace bridge
} // namespace intcoin
