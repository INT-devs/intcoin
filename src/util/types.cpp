/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Core Type Implementations
 */

#include "intcoin/types.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace intcoin {

// ============================================================================
// uint256 Utility Functions
// ============================================================================

std::string ToHex(const uint256& hash) {
    std::stringstream ss;
    // Output in natural byte order (big-endian, matching standard hex representations)
    for (size_t i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::optional<uint256> FromHex(const std::string& hex) {
    if (hex.length() != 64) {
        return std::nullopt;
    }

    uint256 result{};

    // Parse hex string (big-endian format, matching ToHex output)
    for (size_t i = 0; i < 32; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        char* end;
        long value = std::strtol(byte_str.c_str(), &end, 16);

        if (end != byte_str.c_str() + 2) {
            return std::nullopt; // Invalid hex character
        }

        // Store in natural byte order
        result[i] = static_cast<uint8_t>(value);
    }

    return result;
}

// ============================================================================
// uint256 Operators
// ============================================================================

// Note: std::array already provides these operators, so we don't need to redefine them
// The declarations in types.h are sufficient

std::size_t uint256_hash::operator()(const uint256& hash) const noexcept {
    // Simple hash of first 8 bytes
    uint64_t value = 0;
    std::memcpy(&value, hash.data(), sizeof(uint64_t));
    return std::hash<uint64_t>{}(value);
}

} // namespace intcoin
