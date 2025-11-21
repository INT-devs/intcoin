// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Primitive data types and constants.

#ifndef INTCOIN_PRIMITIVES_H
#define INTCOIN_PRIMITIVES_H

#include <cstdint>
#include <array>
#include <vector>

namespace intcoin {

// Fundamental constants
static constexpr uint64_t COIN = 100000000ULL;  // 1 INT = 100,000,000 base units
static constexpr uint64_t MAX_SUPPLY = 221000000000000ULL;  // 221 trillion INT
static constexpr uint32_t BLOCK_TIME = 120;  // 2 minutes in seconds
static constexpr uint32_t BLOCKS_PER_YEAR = 262800;  // (365 * 24 * 60) / 2

// Cryptographic sizes (quantum-resistant) - from liboqs
static constexpr size_t HASH_SIZE = 32;  // SHA3-256
static constexpr size_t DILITHIUM_PUBKEY_SIZE = 2592;  // ML-DSA-87 public key (NIST FIPS 204)
static constexpr size_t DILITHIUM_SIGNATURE_SIZE = 4627;  // ML-DSA-87 signature (NIST FIPS 204)
static constexpr size_t KYBER_PUBKEY_SIZE = 1568;  // ML-KEM-1024 public key (NIST FIPS 203)
static constexpr size_t KYBER_CIPHERTEXT_SIZE = 1568;  // ML-KEM-1024 ciphertext (NIST FIPS 203)
static constexpr size_t KYBER_SHARED_SECRET_SIZE = 32;  // ML-KEM-1024 shared secret (NIST FIPS 203)

// Type aliases for clarity
using Hash256 = std::array<uint8_t, HASH_SIZE>;
using DilithiumPubKey = std::array<uint8_t, DILITHIUM_PUBKEY_SIZE>;
using DilithiumSignature = std::array<uint8_t, DILITHIUM_SIGNATURE_SIZE>;
using KyberPubKey = std::array<uint8_t, KYBER_PUBKEY_SIZE>;
using KyberCiphertext = std::array<uint8_t, KYBER_CIPHERTEXT_SIZE>;
using SharedSecret = std::array<uint8_t, KYBER_SHARED_SECRET_SIZE>;

// Network constants
namespace network {
    // Magic bytes for network identification
    static constexpr uint32_t MAINNET_MAGIC = 0x494E5443;  // "INTC"
    static constexpr uint32_t TESTNET_MAGIC = 0x54494E54;  // "TINT"

    // Ports (INTcoin unique range: 9330-9349)
    static constexpr uint16_t MAINNET_PORT = 9333;
    static constexpr uint16_t MAINNET_RPC_PORT = 9334;
    static constexpr uint16_t TESTNET_PORT = 19333;
    static constexpr uint16_t TESTNET_RPC_PORT = 19334;

    // Protocol version
    static constexpr uint32_t PROTOCOL_VERSION = 1;
    static constexpr uint32_t MIN_PROTOCOL_VERSION = 1;
}

// Consensus parameters
namespace consensus {
    static constexpr uint32_t MAX_BLOCK_SIZE = 2000000;  // 2 MB
    static constexpr uint32_t MAX_BLOCK_WEIGHT = 8000000;  // 8 million weight units
    static constexpr uint32_t COINBASE_MATURITY = 100;  // Blocks before coinbase can be spent
    static constexpr uint32_t MAX_FUTURE_BLOCK_TIME = 7200;  // 2 hours in seconds

    // Difficulty adjustment
    static constexpr uint32_t DIFFICULTY_ADJUSTMENT_INTERVAL = 2016;  // ~2.8 days
    static constexpr uint32_t DIFFICULTY_TARGET_TIMESPAN = DIFFICULTY_ADJUSTMENT_INTERVAL * BLOCK_TIME;

    // Block reward phases (Multi-Phase Hybrid model from REWARDS-PROGRAM.md)
    static constexpr uint64_t PHASE1_REWARD = 3000000 * COIN;
    static constexpr uint64_t PHASE2_REWARD = 2000000 * COIN;
    static constexpr uint64_t PHASE3_REWARD = 1000000 * COIN;
    static constexpr uint64_t PHASE4_REWARD = 500000 * COIN;

    static constexpr uint32_t PHASE1_END = BLOCKS_PER_YEAR * 10;   // Year 10
    static constexpr uint32_t PHASE2_END = BLOCKS_PER_YEAR * 25;   // Year 25
    static constexpr uint32_t PHASE3_END = BLOCKS_PER_YEAR * 45;   // Year 45
    static constexpr uint32_t PHASE4_END = BLOCKS_PER_YEAR * 60;   // Year 60
}

// Script constants
namespace script {
    static constexpr size_t MAX_SCRIPT_SIZE = 10000;
    static constexpr size_t MAX_SCRIPT_ELEMENT_SIZE = 520;
    static constexpr size_t MAX_OPS_PER_SCRIPT = 201;
    static constexpr size_t MAX_PUBKEYS_PER_MULTISIG = 20;
    static constexpr size_t MAX_STACK_SIZE = 1000;
}

// Lightning Network constants
namespace lightning {
    static constexpr uint64_t MIN_CHANNEL_SIZE = 100000 * COIN;  // 100k INT minimum
    static constexpr uint64_t MAX_CHANNEL_SIZE = 1000000000ULL * COIN;  // 1B INT maximum
    static constexpr uint32_t CHANNEL_TIMEOUT = 144;  // Blocks (~4.8 hours)
    static constexpr uint16_t DEFAULT_LIGHTNING_PORT = 9735;
}

// Smart contract constants
namespace contracts {
    static constexpr uint64_t MIN_GAS_PRICE = 1;
    static constexpr uint64_t MAX_GAS_LIMIT = 10000000;
    static constexpr size_t MAX_CONTRACT_SIZE = 24576;  // 24 KB
}

} // namespace intcoin

// Hash specializations for unordered containers
namespace std {
    template<>
    struct hash<intcoin::Hash256> {
        size_t operator()(const intcoin::Hash256& h) const noexcept {
            // Simple hash combining first 8 bytes
            size_t result = 0;
            for (size_t i = 0; i < 8; ++i) {
                result ^= static_cast<size_t>(h[i]) << (i * 8);
            }
            return result;
        }
    };
}

#endif // INTCOIN_PRIMITIVES_H
