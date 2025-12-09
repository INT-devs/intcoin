/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Core Type Definitions
 */

#ifndef INTCOIN_TYPES_H
#define INTCOIN_TYPES_H

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace intcoin {

// ============================================================================
// Constants
// ============================================================================

/// INTcoin network magic bytes (0x494E5443 = "INTC")
constexpr uint32_t NETWORK_MAGIC = 0x494E5443;

/// Minimum INTS per INT (1 INT = 1,000,000 INTS)
constexpr uint64_t INTS_PER_INT = 1'000'000;

/// Total supply in INTS (221 Trillion INT)
constexpr uint64_t TOTAL_SUPPLY_INTS = 221'000'000'000'000ULL * INTS_PER_INT;

/// Initial block reward in INTS
constexpr uint64_t INITIAL_BLOCK_REWARD = 105'113'636ULL * INTS_PER_INT;

/// Block time target in seconds
constexpr uint32_t BLOCK_TIME_TARGET = 120; // 2 minutes

/// Halving interval in blocks (~4 years)
constexpr uint64_t HALVING_INTERVAL = 1'051'200;

/// Maximum halvings (64)
constexpr uint32_t MAX_HALVINGS = 64;

/// Default P2P port
constexpr uint16_t DEFAULT_P2P_PORT = 9333;

/// Default RPC port
constexpr uint16_t DEFAULT_RPC_PORT = 9334;

// ============================================================================
// Fixed-Size Types
// ============================================================================

/// 256-bit hash (SHA3-256 output)
using uint256 = std::array<uint8_t, 32>;

/// 512-bit hash
using uint512 = std::array<uint8_t, 64>;

/// Public key (Dilithium3)
using PublicKey = std::array<uint8_t, 1952>;

/// Secret key (Dilithium3 / ML-DSA-65)
using SecretKey = std::array<uint8_t, 4032>;

/// Signature (Dilithium3 / ML-DSA-65)
using Signature = std::array<uint8_t, 3309>;

/// Kyber public key
using KyberPublicKey = std::array<uint8_t, 1184>;

/// Kyber secret key
using KyberSecretKey = std::array<uint8_t, 2400>;

/// Kyber ciphertext
using KyberCiphertext = std::array<uint8_t, 1088>;

/// Kyber shared secret
using KyberSharedSecret = std::array<uint8_t, 32>;

// ============================================================================
// Utility Functions
// ============================================================================

/// Convert uint256 to hex string
std::string ToHex(const uint256& hash);

/// Parse hex string to uint256
std::optional<uint256> FromHex(const std::string& hex);

/// Convert INTS to INT (divide by 1,000,000)
inline double IntsToInt(uint64_t ints) {
    return static_cast<double>(ints) / INTS_PER_INT;
}

/// Convert INT to INTS (multiply by 1,000,000)
inline uint64_t IntToInts(double int_value) {
    return static_cast<uint64_t>(int_value * INTS_PER_INT);
}

/// Calculate block reward for given height
uint64_t GetBlockReward(uint64_t height);

/// Hash function for uint256 (for use in unordered containers)
struct uint256_hash {
    std::size_t operator()(const uint256& hash) const noexcept;
};

// ============================================================================
// Result Types
// ============================================================================

/// Result type for operations that can fail
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;

    bool IsOk() const { return value.has_value(); }
    bool IsError() const { return !value.has_value(); }

    /// Get the value (throws if error)
    T& GetValue() { return value.value(); }
    const T& GetValue() const { return value.value(); }

    static Result Ok(T v) {
        return Result{std::move(v), ""};
    }

    static Result Error(std::string err) {
        return Result{std::nullopt, std::move(err)};
    }
};

/// Specialization for void result
template<>
struct Result<void> {
    std::string error;

    bool IsOk() const { return error.empty(); }
    bool IsError() const { return !error.empty(); }

    static Result Ok() {
        return Result{""};
    }

    static Result Error(std::string err) {
        return Result{std::move(err)};
    }
};

} // namespace intcoin

#endif // INTCOIN_TYPES_H
