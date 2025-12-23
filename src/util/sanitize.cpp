/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Input Sanitization and Validation Implementation
 */

#include "intcoin/sanitize.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <chrono>

namespace intcoin {
namespace sanitize {

// ============================================================================
// String Sanitization
// ============================================================================

std::string SanitizeString(const std::string& input, size_t max_length) {
    if (input.empty()) {
        return "";
    }

    // Truncate to maximum length
    std::string result = input.substr(0, std::min(input.length(), max_length));

    // Remove null bytes
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());

    return result;
}

std::optional<std::string> SanitizeAlphanumeric(const std::string& input, size_t max_length) {
    if (input.empty() || input.length() > max_length) {
        return std::nullopt;
    }

    // Check all characters are alphanumeric
    for (char c : input) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            return std::nullopt;
        }
    }

    return input;
}

std::optional<std::string> SanitizeFilename(const std::string& filename) {
    if (filename.empty() || filename.length() > MAX_FILENAME_LENGTH) {
        return std::nullopt;
    }

    // Check for path traversal attempts
    if (filename.find("..") != std::string::npos ||
        filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos) {
        return std::nullopt;
    }

    // Check for null bytes
    if (filename.find('\0') != std::string::npos) {
        return std::nullopt;
    }

    // Disallow special filenames
    if (filename == "." || filename == ".." ||
        filename.find('\0') != std::string::npos) {
        return std::nullopt;
    }

    return filename;
}

std::optional<std::string> SanitizePath(const std::string& path) {
    if (path.empty() || path.length() > MAX_PATH_LENGTH) {
        return std::nullopt;
    }

    // Check for null bytes
    if (path.find('\0') != std::string::npos) {
        return std::nullopt;
    }

    // Check for suspicious patterns
    if (path.find("..") != std::string::npos) {
        // Allow .. but verify it doesn't escape root
        // This is a simplified check - in production, use canonicalization
    }

    return path;
}

std::string RemoveControlCharacters(const std::string& input) {
    std::string result;
    result.reserve(input.length());

    for (char c : input) {
        unsigned char uc = static_cast<unsigned char>(c);
        // Keep printable characters, space, tab, and newline
        if (uc >= 32 || uc == '\t' || uc == '\n' || uc == '\r') {
            result += c;
        }
    }

    return result;
}

std::string EscapeString(const std::string& input) {
    std::ostringstream escaped;

    for (char c : input) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    // Escape other control characters as \uXXXX
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    escaped << c;
                }
                break;
        }
    }

    return escaped.str();
}

bool IsValidUTF8(const std::string& input) {
    size_t i = 0;
    while (i < input.length()) {
        unsigned char c = input[i];

        int bytes = 0;
        if ((c & 0x80) == 0) {
            // Single-byte character (ASCII)
            bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            bytes = 4;
        } else {
            return false; // Invalid UTF-8 start byte
        }

        // Check continuation bytes
        for (int j = 1; j < bytes; ++j) {
            if (i + j >= input.length()) {
                return false; // Incomplete sequence
            }
            if ((static_cast<unsigned char>(input[i + j]) & 0xC0) != 0x80) {
                return false; // Invalid continuation byte
            }
        }

        i += bytes;
    }

    return true;
}

std::string SanitizeUTF8(const std::string& input) {
    std::string result;
    result.reserve(input.length());

    size_t i = 0;
    while (i < input.length()) {
        unsigned char c = input[i];

        int bytes = 0;
        if ((c & 0x80) == 0) {
            bytes = 1;
        } else if ((c & 0xE0) == 0xC0) {
            bytes = 2;
        } else if ((c & 0xF0) == 0xE0) {
            bytes = 3;
        } else if ((c & 0xF8) == 0xF0) {
            bytes = 4;
        } else {
            // Replace invalid byte with replacement character
            result += "\xEF\xBF\xBD"; // U+FFFD
            i++;
            continue;
        }

        // Validate continuation bytes
        bool valid = true;
        for (int j = 1; j < bytes; ++j) {
            if (i + j >= input.length() ||
                (static_cast<unsigned char>(input[i + j]) & 0xC0) != 0x80) {
                valid = false;
                break;
            }
        }

        if (valid) {
            // Copy valid UTF-8 sequence
            for (int j = 0; j < bytes; ++j) {
                result += input[i + j];
            }
            i += bytes;
        } else {
            // Replace invalid sequence
            result += "\xEF\xBF\xBD"; // U+FFFD
            i++;
        }
    }

    return result;
}

// ============================================================================
// Format Validation
// ============================================================================

bool IsValidHex(const std::string& input) {
    if (input.empty() || input.length() > MAX_HEX_LENGTH) {
        return false;
    }

    for (char c : input) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    return true;
}

bool IsValidBase64(const std::string& input) {
    if (input.empty()) {
        return false;
    }

    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (base64_chars.find(c) == std::string::npos && c != '=') {
            return false;
        }
        // Padding can only appear at the end
        if (c == '=' && i < input.length() - 2) {
            return false;
        }
    }

    // Check length is multiple of 4
    return input.length() % 4 == 0;
}

bool IsValidBech32Format(const std::string& address) {
    if (address.length() < 8 || address.length() > MAX_ADDRESS_LENGTH) {
        return false;
    }

    // Check for separator
    size_t sep_pos = address.rfind('1');
    if (sep_pos == std::string::npos || sep_pos == 0) {
        return false;
    }

    // HRP (human-readable part) must be lowercase or uppercase, not mixed
    bool has_lower = false, has_upper = false;
    for (size_t i = 0; i < sep_pos; ++i) {
        char c = address[i];
        if (c >= 'a' && c <= 'z') has_lower = true;
        if (c >= 'A' && c <= 'Z') has_upper = true;
        if (c < 33 || c > 126) return false; // Must be printable ASCII
    }

    if (has_lower && has_upper) {
        return false; // Mixed case
    }

    // Data part
    static const std::string bech32_chars = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    for (size_t i = sep_pos + 1; i < address.length(); ++i) {
        char c = std::tolower(address[i]);
        if (bech32_chars.find(c) == std::string::npos) {
            return false;
        }
    }

    return true;
}

bool IsValidIPv4(const std::string& ip) {
    std::istringstream iss(ip);
    std::string octet;
    int count = 0;

    while (std::getline(iss, octet, '.')) {
        if (++count > 4) return false;

        try {
            int value = std::stoi(octet);
            if (value < 0 || value > 255) return false;
        } catch (...) {
            return false;
        }
    }

    return count == 4;
}

bool IsValidIPv6(const std::string& ip) {
    // Simplified IPv6 validation
    if (ip.empty() || ip.length() > 45) {
        return false;
    }

    int colons = 0;
    int double_colons = 0;

    for (size_t i = 0; i < ip.length(); ++i) {
        char c = ip[i];
        if (c == ':') {
            colons++;
            if (i + 1 < ip.length() && ip[i + 1] == ':') {
                double_colons++;
                i++; // Skip next colon
            }
        } else if (!std::isxdigit(c)) {
            return false;
        }
    }

    // IPv6 has 7 colons (8 groups) or less with ::
    return (colons >= 2 && colons <= 7) || (double_colons == 1 && colons <= 7);
}

bool IsValidPort(uint16_t port) {
    // Port 0 is reserved, ports 1-65535 are valid
    return port > 0;
}

bool IsValidURL(const std::string& url) {
    // Basic URL validation
    if (url.empty() || url.length() > 2048) {
        return false;
    }

    // Must start with protocol
    if (url.find("://") == std::string::npos) {
        return false;
    }

    // Check for null bytes
    if (url.find('\0') != std::string::npos) {
        return false;
    }

    return true;
}

bool IsValidEmail(const std::string& email) {
    // Basic email validation
    if (email.empty() || email.length() > 254) {
        return false;
    }

    size_t at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0 || at_pos == email.length() - 1) {
        return false;
    }

    // Check for multiple @
    if (email.find('@', at_pos + 1) != std::string::npos) {
        return false;
    }

    return true;
}

// ============================================================================
// Buffer Sanitization
// ============================================================================

bool ValidateBufferSize(size_t size, size_t max_size) {
    return size <= max_size;
}

std::vector<uint8_t> SanitizeBuffer(const std::vector<uint8_t>& buffer, size_t max_size) {
    if (buffer.size() <= max_size) {
        return buffer;
    }
    return std::vector<uint8_t>(buffer.begin(), buffer.begin() + max_size);
}

bool WillBufferOverflow(size_t current_size, size_t add_size, size_t max_size) {
    return current_size > max_size - add_size;
}

// ============================================================================
// SQL/Injection Prevention
// ============================================================================

std::string EscapeSQL(const std::string& input) {
    std::string result;
    result.reserve(input.length() * 2);

    for (char c : input) {
        if (c == '\'' || c == '"' || c == '\\') {
            result += '\\';
        }
        result += c;
    }

    return result;
}

bool ContainsSuspiciousPatterns(const std::string& input) {
    // Check for common injection patterns
    static const std::vector<std::string> patterns = {
        "' OR '1'='1",
        "' OR 1=1",
        "--",
        "/*",
        "*/",
        "xp_",
        "sp_",
        "0x",
        "../",
        "..\\",
        "<script",
        "javascript:",
        "onerror=",
        "onclick="
    };

    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    for (const auto& pattern : patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

std::string SanitizeShellInput(const std::string& input) {
    std::string result;
    result.reserve(input.length());

    // Remove or escape shell metacharacters
    for (char c : input) {
        if (c == '$' || c == '`' || c == '!' || c == '&' ||
            c == '|' || c == ';' || c == '<' || c == '>' ||
            c == '(' || c == ')' || c == '{' || c == '}' ||
            c == '[' || c == ']' || c == '*' || c == '?' ||
            c == '~' || c == '#' || c == '\n' || c == '\r') {
            // Skip dangerous characters
            continue;
        }
        result += c;
    }

    return result;
}

// ============================================================================
// JSON Sanitization
// ============================================================================

bool ValidateJSONDepth(const std::string& json, size_t max_depth) {
    size_t depth = 0;
    size_t max_seen = 0;

    for (char c : json) {
        if (c == '{' || c == '[') {
            depth++;
            max_seen = std::max(max_seen, depth);
            if (max_seen > max_depth) {
                return false;
            }
        } else if (c == '}' || c == ']') {
            if (depth > 0) depth--;
        }
    }

    return true;
}

std::string SanitizeJSONString(const std::string& input) {
    return EscapeString(input);
}

bool IsValidJSONKey(const std::string& key) {
    if (key.empty() || key.length() > 256) {
        return false;
    }

    // Keys should be printable characters
    for (char c : key) {
        if (static_cast<unsigned char>(c) < 32) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Network Message Sanitization
// ============================================================================

bool ValidateMessageSize(uint32_t size, uint32_t max_size) {
    return size > 0 && size <= max_size;
}

bool IsValidNetworkCommand(const std::string& command) {
    if (command.empty() || command.length() > 12) {
        return false;
    }

    // Commands should be lowercase alphanumeric
    for (char c : command) {
        if (!std::islower(c) && !std::isdigit(c)) {
            return false;
        }
    }

    return true;
}

std::optional<std::string> SanitizePeerAddress(const std::string& address) {
    if (address.empty() || address.length() > 256) {
        return std::nullopt;
    }

    // Check for IPv4 or IPv6 format with optional port
    size_t colon_pos = address.rfind(':');

    if (colon_pos != std::string::npos) {
        std::string ip = address.substr(0, colon_pos);
        std::string port_str = address.substr(colon_pos + 1);

        // Validate port
        try {
            int port = std::stoi(port_str);
            if (port <= 0 || port > 65535) {
                return std::nullopt;
            }
        } catch (...) {
            return std::nullopt;
        }

        // Validate IP
        if (!IsValidIPv4(ip) && !IsValidIPv6(ip)) {
            return std::nullopt;
        }
    } else {
        // Just IP without port
        if (!IsValidIPv4(address) && !IsValidIPv6(address)) {
            return std::nullopt;
        }
    }

    return address;
}

// ============================================================================
// Cryptographic Input Validation
// ============================================================================

bool IsValidPublicKeySize(size_t size) {
    // Dilithium3 public key is 1952 bytes
    return size == 1952;
}

bool IsValidSignatureSize(size_t size) {
    // Dilithium3 signature is 3293 bytes
    return size == 3293;
}

bool IsValidHashSize(size_t size) {
    // SHA3-256 hash is 32 bytes
    return size == 32;
}

// ============================================================================
// Rate Limiting Helpers
// ============================================================================

bool IsRateLimitExceeded(RateLimitState& state, uint64_t current_time_ms) {
    // Check if we're in a new window
    if (current_time_ms - state.window_start >= state.window_duration_ms) {
        // Reset window
        state.window_start = current_time_ms;
        state.count = 0;
    }

    // Increment counter
    state.count++;

    // Check if limit exceeded
    return state.count > state.max_count;
}

// ============================================================================
// Whitelist/Blacklist Validation
// ============================================================================

bool ContainsOnly(const std::string& input, const std::string& whitelist) {
    for (char c : input) {
        if (whitelist.find(c) == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool ContainsAny(const std::string& input, const std::string& blacklist) {
    for (char c : input) {
        if (blacklist.find(c) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool MatchesPattern(const std::string& input, const std::regex& pattern) {
    return std::regex_match(input, pattern);
}

} // namespace sanitize
} // namespace intcoin
