#ifndef INTCOIN_VALIDATION_H
#define INTCOIN_VALIDATION_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <regex>
#include <limits>

namespace intcoin {
namespace validation {

/**
 * Input Validation Framework
 * Provides comprehensive validation for all external data inputs
 * to prevent injection attacks, malformed data, and security vulnerabilities.
 */

// Validation result with error details
struct ValidationResult {
    bool valid;
    std::string error_message;

    ValidationResult() : valid(true) {}
    ValidationResult(bool v, const std::string& msg = "")
        : valid(v), error_message(msg) {}

    static ValidationResult success() {
        return ValidationResult(true);
    }

    static ValidationResult failure(const std::string& msg) {
        return ValidationResult(false, msg);
    }

    operator bool() const { return valid; }
};

/**
 * String Validation
 * Validates string inputs with various security checks
 */
class StringValidator {
public:
    // Maximum safe string lengths
    static constexpr size_t MAX_ADDRESS_LENGTH = 128;
    static constexpr size_t MAX_MESSAGE_LENGTH = 4096;
    static constexpr size_t MAX_SCRIPT_LENGTH = 10000;
    static constexpr size_t MAX_PUBKEY_LENGTH = 65;  // Uncompressed pubkey
    static constexpr size_t MAX_SIGNATURE_LENGTH = 73; // DER signature
    static constexpr size_t MAX_HASH_LENGTH = 32;
    static constexpr size_t MAX_HOSTNAME_LENGTH = 255;
    static constexpr size_t MAX_USERNAME_LENGTH = 64;

    // Validate string length
    static ValidationResult validate_length(
        const std::string& str,
        size_t min_length,
        size_t max_length,
        const std::string& field_name = "field"
    ) {
        if (str.length() < min_length) {
            return ValidationResult::failure(
                field_name + " is too short (min: " + std::to_string(min_length) + ")"
            );
        }
        if (str.length() > max_length) {
            return ValidationResult::failure(
                field_name + " is too long (max: " + std::to_string(max_length) + ")"
            );
        }
        return ValidationResult::success();
    }

    // Validate string contains only allowed characters
    static ValidationResult validate_charset(
        const std::string& str,
        const std::string& allowed_chars,
        const std::string& field_name = "field"
    ) {
        for (char c : str) {
            if (allowed_chars.find(c) == std::string::npos) {
                return ValidationResult::failure(
                    field_name + " contains invalid character: " + std::string(1, c)
                );
            }
        }
        return ValidationResult::success();
    }

    // Validate string matches regex pattern
    static ValidationResult validate_pattern(
        const std::string& str,
        const std::string& pattern,
        const std::string& field_name = "field"
    ) {
        try {
            std::regex regex_pattern(pattern);
            if (!std::regex_match(str, regex_pattern)) {
                return ValidationResult::failure(
                    field_name + " does not match required pattern"
                );
            }
        } catch (const std::regex_error& e) {
            return ValidationResult::failure(
                "Invalid regex pattern: " + std::string(e.what())
            );
        }
        return ValidationResult::success();
    }

    // Validate hexadecimal string
    static ValidationResult validate_hex(
        const std::string& str,
        size_t expected_length = 0,
        const std::string& field_name = "field"
    ) {
        if (str.empty()) {
            return ValidationResult::failure(field_name + " is empty");
        }

        if (expected_length > 0 && str.length() != expected_length) {
            return ValidationResult::failure(
                field_name + " has invalid length (expected: " +
                std::to_string(expected_length) + ")"
            );
        }

        for (char c : str) {
            if (!std::isxdigit(c)) {
                return ValidationResult::failure(
                    field_name + " contains non-hexadecimal character: " +
                    std::string(1, c)
                );
            }
        }
        return ValidationResult::success();
    }

    // Validate base58 string (for addresses)
    static ValidationResult validate_base58(
        const std::string& str,
        const std::string& field_name = "field"
    ) {
        if (str.empty()) {
            return ValidationResult::failure(field_name + " is empty");
        }

        // Base58 alphabet (no 0, O, I, l to avoid confusion)
        const std::string base58_chars =
            "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

        for (char c : str) {
            if (base58_chars.find(c) == std::string::npos) {
                return ValidationResult::failure(
                    field_name + " contains invalid base58 character: " +
                    std::string(1, c)
                );
            }
        }
        return ValidationResult::success();
    }

    // Validate hostname/domain name
    static ValidationResult validate_hostname(const std::string& hostname) {
        auto length_check = validate_length(
            hostname, 1, MAX_HOSTNAME_LENGTH, "hostname"
        );
        if (!length_check) return length_check;

        // Hostname pattern: alphanumeric, dots, hyphens
        std::regex pattern(R"(^([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?$)");

        if (!std::regex_match(hostname, pattern)) {
            return ValidationResult::failure("Invalid hostname format");
        }

        return ValidationResult::success();
    }

    // Sanitize string by removing control characters
    static std::string sanitize(const std::string& str) {
        std::string result;
        result.reserve(str.length());

        for (char c : str) {
            // Only allow printable ASCII and common whitespace
            if ((c >= 32 && c <= 126) || c == '\n' || c == '\t') {
                result += c;
            }
        }

        return result;
    }
};

/**
 * Numeric Validation
 * Validates numeric inputs with range checks
 */
class NumericValidator {
public:
    // Validate integer is within range
    template<typename T>
    static ValidationResult validate_range(
        T value,
        T min_value,
        T max_value,
        const std::string& field_name = "field"
    ) {
        if (value < min_value) {
            return ValidationResult::failure(
                field_name + " is below minimum value"
            );
        }
        if (value > max_value) {
            return ValidationResult::failure(
                field_name + " exceeds maximum value"
            );
        }
        return ValidationResult::success();
    }

    // Validate unsigned integer
    static ValidationResult validate_uint32(
        uint64_t value,
        const std::string& field_name = "field"
    ) {
        return validate_range<uint64_t>(
            value, 0, std::numeric_limits<uint32_t>::max(), field_name
        );
    }

    // Validate port number
    static ValidationResult validate_port(uint32_t port) {
        return validate_range<uint32_t>(port, 1, 65535, "port");
    }

    // Validate timestamp (not too far in past or future)
    static ValidationResult validate_timestamp(uint64_t timestamp) {
        // Allow timestamps from 2009 (Bitcoin genesis) to 100 years in future
        constexpr uint64_t MIN_TIMESTAMP = 1231006505; // 2009-01-03
        const uint64_t max_timestamp =
            static_cast<uint64_t>(std::time(nullptr)) + (100 * 365 * 24 * 60 * 60);

        return validate_range<uint64_t>(
            timestamp, MIN_TIMESTAMP, max_timestamp, "timestamp"
        );
    }

    // Validate block height
    static ValidationResult validate_block_height(uint32_t height) {
        // Maximum reasonable block height (100 years at 10 min blocks)
        constexpr uint32_t MAX_BLOCK_HEIGHT = 100 * 365 * 24 * 6;
        return validate_range<uint32_t>(height, 0, MAX_BLOCK_HEIGHT, "block height");
    }

    // Validate amount (no negative amounts)
    static ValidationResult validate_amount(int64_t amount) {
        if (amount < 0) {
            return ValidationResult::failure("Amount cannot be negative");
        }

        // Maximum supply check (21 million INTcoin * 100 million satoshis)
        constexpr int64_t MAX_AMOUNT = 21000000LL * 100000000LL;
        if (amount > MAX_AMOUNT) {
            return ValidationResult::failure("Amount exceeds maximum supply");
        }

        return ValidationResult::success();
    }
};

/**
 * Binary Data Validation
 * Validates binary data and byte arrays
 */
class BinaryValidator {
public:
    // Validate byte array length
    static ValidationResult validate_length(
        const std::vector<uint8_t>& data,
        size_t expected_length,
        const std::string& field_name = "field"
    ) {
        if (data.size() != expected_length) {
            return ValidationResult::failure(
                field_name + " has invalid length (expected: " +
                std::to_string(expected_length) + ", got: " +
                std::to_string(data.size()) + ")"
            );
        }
        return ValidationResult::success();
    }

    // Validate byte array length is within range
    static ValidationResult validate_length_range(
        const std::vector<uint8_t>& data,
        size_t min_length,
        size_t max_length,
        const std::string& field_name = "field"
    ) {
        if (data.size() < min_length) {
            return ValidationResult::failure(
                field_name + " is too short (min: " + std::to_string(min_length) + ")"
            );
        }
        if (data.size() > max_length) {
            return ValidationResult::failure(
                field_name + " is too long (max: " + std::to_string(max_length) + ")"
            );
        }
        return ValidationResult::success();
    }

    // Validate hash (32 bytes)
    static ValidationResult validate_hash(const std::vector<uint8_t>& hash) {
        return validate_length(hash, 32, "hash");
    }

    // Validate public key
    static ValidationResult validate_pubkey(const std::vector<uint8_t>& pubkey) {
        if (pubkey.empty()) {
            return ValidationResult::failure("Public key is empty");
        }

        // Compressed pubkey: 33 bytes (0x02 or 0x03 prefix)
        // Uncompressed pubkey: 65 bytes (0x04 prefix)
        if (pubkey.size() == 33) {
            if (pubkey[0] != 0x02 && pubkey[0] != 0x03) {
                return ValidationResult::failure(
                    "Invalid compressed public key prefix"
                );
            }
        } else if (pubkey.size() == 65) {
            if (pubkey[0] != 0x04) {
                return ValidationResult::failure(
                    "Invalid uncompressed public key prefix"
                );
            }
        } else {
            return ValidationResult::failure(
                "Invalid public key length (expected 33 or 65 bytes)"
            );
        }

        return ValidationResult::success();
    }

    // Validate signature (DER format)
    static ValidationResult validate_signature(const std::vector<uint8_t>& sig) {
        if (sig.size() < 8 || sig.size() > 73) {
            return ValidationResult::failure(
                "Invalid signature length (expected 8-73 bytes)"
            );
        }

        // DER signature must start with 0x30
        if (sig[0] != 0x30) {
            return ValidationResult::failure("Invalid DER signature prefix");
        }

        // Basic DER structure validation
        size_t length = sig[1];
        if (length + 2 != sig.size()) {
            return ValidationResult::failure("Invalid DER signature length field");
        }

        return ValidationResult::success();
    }
};

/**
 * Network Data Validation
 * Validates network-related inputs
 */
class NetworkValidator {
public:
    // Validate IPv4 address
    static ValidationResult validate_ipv4(const std::string& ip) {
        std::regex ipv4_pattern(
            R"(^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.){3}(25[0-5]|(2[0-4]|1\d|[1-9]|)\d)$)"
        );

        if (!std::regex_match(ip, ipv4_pattern)) {
            return ValidationResult::failure("Invalid IPv4 address format");
        }

        return ValidationResult::success();
    }

    // Validate IPv6 address (simplified)
    static ValidationResult validate_ipv6(const std::string& ip) {
        std::regex ipv6_pattern(
            R"(^(([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|::([0-9a-fA-F]{1,4}:){0,6}[0-9a-fA-F]{1,4})$)"
        );

        if (!std::regex_match(ip, ipv6_pattern)) {
            return ValidationResult::failure("Invalid IPv6 address format");
        }

        return ValidationResult::success();
    }

    // Validate peer address
    static ValidationResult validate_peer_address(
        const std::string& address,
        uint16_t port
    ) {
        // Try IPv4 first
        auto ipv4_result = validate_ipv4(address);
        if (!ipv4_result) {
            // Try IPv6
            auto ipv6_result = validate_ipv6(address);
            if (!ipv6_result) {
                // Try hostname
                auto hostname_result = StringValidator::validate_hostname(address);
                if (!hostname_result) {
                    return ValidationResult::failure(
                        "Invalid peer address (not IPv4, IPv6, or hostname)"
                    );
                }
            }
        }

        // Validate port
        return NumericValidator::validate_port(port);
    }
};

/**
 * Composite Validators
 * Higher-level validation for complex structures
 */
class CompositeValidator {
public:
    // Validate transaction input
    static ValidationResult validate_transaction_input(
        const std::vector<uint8_t>& prev_tx_hash,
        uint32_t prev_output_index,
        const std::vector<uint8_t>& signature_script
    ) {
        auto hash_result = BinaryValidator::validate_hash(prev_tx_hash);
        if (!hash_result) return hash_result;

        // Output index should be reasonable (max 100k outputs per tx)
        auto index_result = NumericValidator::validate_range<uint32_t>(
            prev_output_index, 0, 100000, "output index"
        );
        if (!index_result) return index_result;

        // Signature script size check
        auto script_result = BinaryValidator::validate_length_range(
            signature_script, 0, StringValidator::MAX_SCRIPT_LENGTH,
            "signature script"
        );
        if (!script_result) return script_result;

        return ValidationResult::success();
    }

    // Validate transaction output
    static ValidationResult validate_transaction_output(
        int64_t amount,
        const std::vector<uint8_t>& pubkey_script
    ) {
        auto amount_result = NumericValidator::validate_amount(amount);
        if (!amount_result) return amount_result;

        auto script_result = BinaryValidator::validate_length_range(
            pubkey_script, 0, StringValidator::MAX_SCRIPT_LENGTH,
            "pubkey script"
        );
        if (!script_result) return script_result;

        return ValidationResult::success();
    }

    // Validate block header
    static ValidationResult validate_block_header(
        uint32_t version,
        const std::vector<uint8_t>& prev_block_hash,
        const std::vector<uint8_t>& merkle_root,
        uint64_t timestamp,
        uint32_t bits,
        uint32_t nonce
    ) {
        // Version should be reasonable (1-4 currently used in Bitcoin)
        auto version_result = NumericValidator::validate_range<uint32_t>(
            version, 1, 10, "version"
        );
        if (!version_result) return version_result;

        auto hash_result = BinaryValidator::validate_hash(prev_block_hash);
        if (!hash_result) return hash_result;

        auto merkle_result = BinaryValidator::validate_hash(merkle_root);
        if (!merkle_result) return merkle_result;

        auto timestamp_result = NumericValidator::validate_timestamp(timestamp);
        if (!timestamp_result) return timestamp_result;

        return ValidationResult::success();
    }
};

} // namespace validation
} // namespace intcoin

#endif // INTCOIN_VALIDATION_H
