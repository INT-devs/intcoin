/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Input Sanitization and Validation Utilities
 */

#ifndef INTCOIN_SANITIZE_H
#define INTCOIN_SANITIZE_H

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <regex>

namespace intcoin {
namespace sanitize {

// ============================================================================
// String Sanitization
// ============================================================================

/// Maximum safe string length for various inputs
constexpr size_t MAX_STRING_LENGTH = 1024 * 1024;      // 1 MB
constexpr size_t MAX_FILENAME_LENGTH = 255;
constexpr size_t MAX_PATH_LENGTH = 4096;
constexpr size_t MAX_JSON_LENGTH = 10 * 1024 * 1024;   // 10 MB
constexpr size_t MAX_COMMAND_LENGTH = 1024;
constexpr size_t MAX_ADDRESS_LENGTH = 128;
constexpr size_t MAX_HEX_LENGTH = 1024 * 1024;         // 1 MB

/// Sanitize string by removing or escaping dangerous characters
std::string SanitizeString(const std::string& input, size_t max_length = MAX_STRING_LENGTH);

/// Validate and sanitize alphanumeric string
std::optional<std::string> SanitizeAlphanumeric(const std::string& input, size_t max_length = MAX_STRING_LENGTH);

/// Validate and sanitize filename (no path traversal)
std::optional<std::string> SanitizeFilename(const std::string& filename);

/// Validate and sanitize file path (prevent directory traversal)
std::optional<std::string> SanitizePath(const std::string& path);

/// Remove null bytes and control characters
std::string RemoveControlCharacters(const std::string& input);

/// Escape special characters for safe display
std::string EscapeString(const std::string& input);

/// Validate UTF-8 encoding
bool IsValidUTF8(const std::string& input);

/// Sanitize UTF-8 string (replace invalid sequences)
std::string SanitizeUTF8(const std::string& input);

// ============================================================================
// Numeric Sanitization
// ============================================================================

/// Check for integer overflow in addition
template<typename T>
bool WillOverflowAdd(T a, T b) {
    if (a > 0 && b > 0) {
        return a > (std::numeric_limits<T>::max() - b);
    } else if (a < 0 && b < 0) {
        return a < (std::numeric_limits<T>::min() - b);
    }
    return false;
}

/// Check for integer overflow in multiplication
template<typename T>
bool WillOverflowMul(T a, T b) {
    if (a == 0 || b == 0) return false;

    if (a > 0 && b > 0) {
        return a > (std::numeric_limits<T>::max() / b);
    } else if (a < 0 && b < 0) {
        return a < (std::numeric_limits<T>::max() / b);
    } else {
        T positive = (a > 0) ? a : b;
        T negative = (a < 0) ? a : b;
        return negative < (std::numeric_limits<T>::min() / positive);
    }
}

/// Safe addition with overflow check
template<typename T>
std::optional<T> SafeAdd(T a, T b) {
    if (WillOverflowAdd(a, b)) {
        return std::nullopt;
    }
    return a + b;
}

/// Safe multiplication with overflow check
template<typename T>
std::optional<T> SafeMul(T a, T b) {
    if (WillOverflowMul(a, b)) {
        return std::nullopt;
    }
    return a * b;
}

/// Validate numeric range
template<typename T>
bool InRange(T value, T min, T max) {
    return value >= min && value <= max;
}

// ============================================================================
// Format Validation
// ============================================================================

/// Validate hexadecimal string
bool IsValidHex(const std::string& input);

/// Validate Base64 string
bool IsValidBase64(const std::string& input);

/// Validate Bech32 address format (basic check)
bool IsValidBech32Format(const std::string& address);

/// Validate IPv4 address
bool IsValidIPv4(const std::string& ip);

/// Validate IPv6 address
bool IsValidIPv6(const std::string& ip);

/// Validate port number
bool IsValidPort(uint16_t port);

/// Validate URL (basic check)
bool IsValidURL(const std::string& url);

/// Validate email (basic check)
bool IsValidEmail(const std::string& email);

// ============================================================================
// Buffer Sanitization
// ============================================================================

/// Validate buffer size is within limits
bool ValidateBufferSize(size_t size, size_t max_size);

/// Sanitize buffer by truncating to maximum size
std::vector<uint8_t> SanitizeBuffer(const std::vector<uint8_t>& buffer, size_t max_size);

/// Check for buffer overflow
bool WillBufferOverflow(size_t current_size, size_t add_size, size_t max_size);

// ============================================================================
// SQL/Injection Prevention
// ============================================================================

/// Escape SQL special characters (though we use RocksDB, not SQL)
std::string EscapeSQL(const std::string& input);

/// Detect potential injection patterns
bool ContainsSuspiciousPatterns(const std::string& input);

/// Remove or escape shell metacharacters
std::string SanitizeShellInput(const std::string& input);

// ============================================================================
// JSON Sanitization
// ============================================================================

/// Validate JSON structure depth (prevent stack overflow)
bool ValidateJSONDepth(const std::string& json, size_t max_depth = 100);

/// Sanitize JSON string value
std::string SanitizeJSONString(const std::string& input);

/// Validate JSON object key
bool IsValidJSONKey(const std::string& key);

// ============================================================================
// Network Message Sanitization
// ============================================================================

/// Validate network message size
bool ValidateMessageSize(uint32_t size, uint32_t max_size = 32 * 1024 * 1024); // 32 MB default

/// Validate network command string
bool IsValidNetworkCommand(const std::string& command);

/// Sanitize peer address input
std::optional<std::string> SanitizePeerAddress(const std::string& address);

// ============================================================================
// Cryptographic Input Validation
// ============================================================================

/// Validate public key size
bool IsValidPublicKeySize(size_t size);

/// Validate signature size
bool IsValidSignatureSize(size_t size);

/// Validate hash size
bool IsValidHashSize(size_t size);

// ============================================================================
// Rate Limiting Helpers
// ============================================================================

/// Simple rate limiter state
struct RateLimitState {
    size_t count;
    uint64_t window_start;
    uint64_t window_duration_ms;
    size_t max_count;

    RateLimitState(size_t max, uint64_t duration_ms)
        : count(0), window_start(0), window_duration_ms(duration_ms), max_count(max) {}
};

/// Check if rate limit is exceeded
bool IsRateLimitExceeded(RateLimitState& state, uint64_t current_time_ms);

// ============================================================================
// Whitelist/Blacklist Validation
// ============================================================================

/// Check if string contains only whitelisted characters
bool ContainsOnly(const std::string& input, const std::string& whitelist);

/// Check if string contains any blacklisted characters
bool ContainsAny(const std::string& input, const std::string& blacklist);

/// Validate against regex pattern
bool MatchesPattern(const std::string& input, const std::regex& pattern);

} // namespace sanitize
} // namespace intcoin

#endif // INTCOIN_SANITIZE_H
