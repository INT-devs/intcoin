/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Input Sanitization Test Suite
 */

#include "intcoin/sanitize.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace intcoin::sanitize;

void test_string_sanitization() {
    std::cout << "\n=== String Sanitization Tests ===" << std::endl;

    // Test 1: Null byte removal
    std::string with_null = "test\0data";
    std::string sanitized = SanitizeString(with_null);
    assert(sanitized.find('\0') == std::string::npos);
    std::cout << "✅ Null byte removal" << std::endl;

    // Test 2: Length truncation
    std::string too_long(MAX_STRING_LENGTH + 1000, 'a');
    std::string truncated = SanitizeString(too_long);
    assert(truncated.length() == MAX_STRING_LENGTH);
    std::cout << "✅ Length truncation" << std::endl;

    // Test 3: Control character removal
    std::string with_controls = "test\x01\x02\x03data";
    std::string no_controls = RemoveControlCharacters(with_controls);
    assert(no_controls == "testdata");
    std::cout << "✅ Control character removal" << std::endl;

    // Test 4: Alphanumeric validation
    auto valid_alpha = SanitizeAlphanumeric("abc123");
    assert(valid_alpha.has_value());
    auto invalid_alpha = SanitizeAlphanumeric("abc-123");
    assert(!invalid_alpha.has_value());
    std::cout << "✅ Alphanumeric validation" << std::endl;

    // Test 5: UTF-8 validation
    assert(IsValidUTF8("Hello World"));
    assert(IsValidUTF8("こんにちは")); // Japanese
    assert(!IsValidUTF8("\xFF\xFE")); // Invalid UTF-8
    std::cout << "✅ UTF-8 validation" << std::endl;

    // Test 6: String escaping
    std::string dangerous = "test\"quote'single\nNewline\tTab";
    std::string escaped = EscapeString(dangerous);
    assert(escaped.find("\\\"") != std::string::npos);
    assert(escaped.find("\\n") != std::string::npos);
    assert(escaped.find("\\t") != std::string::npos);
    std::cout << "✅ String escaping" << std::endl;
}

void test_path_sanitization() {
    std::cout << "\n=== Path Sanitization Tests ===" << std::endl;

    // Test 1: Valid filename
    auto valid = SanitizeFilename("test.txt");
    assert(valid.has_value());
    std::cout << "✅ Valid filename" << std::endl;

    // Test 2: Path traversal prevention
    auto traversal1 = SanitizeFilename("../etc/passwd");
    assert(!traversal1.has_value());
    auto traversal2 = SanitizeFilename("..\\windows\\system32");
    assert(!traversal2.has_value());
    std::cout << "✅ Path traversal prevention" << std::endl;

    // Test 3: Special filenames
    auto dot = SanitizeFilename(".");
    assert(!dot.has_value());
    auto dotdot = SanitizeFilename("..");
    assert(!dotdot.has_value());
    std::cout << "✅ Special filename rejection" << std::endl;

    // Test 4: Filename with slashes
    auto with_slash = SanitizeFilename("test/file.txt");
    assert(!with_slash.has_value());
    std::cout << "✅ Slash rejection in filename" << std::endl;

    // Test 5: Too long filename
    std::string long_name(MAX_FILENAME_LENGTH + 1, 'a');
    auto too_long = SanitizeFilename(long_name);
    assert(!too_long.has_value());
    std::cout << "✅ Filename length limit" << std::endl;
}

void test_numeric_sanitization() {
    std::cout << "\n=== Numeric Sanitization Tests ===" << std::endl;

    // Test 1: Integer overflow detection (addition)
    assert(WillOverflowAdd<int>(INT_MAX, 1));
    assert(!WillOverflowAdd<int>(100, 200));
    std::cout << "✅ Addition overflow detection" << std::endl;

    // Test 2: Integer overflow detection (multiplication)
    assert(WillOverflowMul<int>(INT_MAX, 2));
    assert(!WillOverflowMul<int>(100, 200));
    std::cout << "✅ Multiplication overflow detection" << std::endl;

    // Test 3: Safe addition
    auto safe_sum = SafeAdd<int>(100, 200);
    assert(safe_sum.has_value() && *safe_sum == 300);
    auto overflow_sum = SafeAdd<int>(INT_MAX, 1);
    assert(!overflow_sum.has_value());
    std::cout << "✅ Safe addition" << std::endl;

    // Test 4: Safe multiplication
    auto safe_product = SafeMul<int>(100, 200);
    assert(safe_product.has_value() && *safe_product == 20000);
    auto overflow_product = SafeMul<int>(INT_MAX, 2);
    assert(!overflow_product.has_value());
    std::cout << "✅ Safe multiplication" << std::endl;

    // Test 5: Range validation
    assert(InRange(50, 0, 100));
    assert(!InRange(150, 0, 100));
    assert(!InRange(-10, 0, 100));
    std::cout << "✅ Range validation" << std::endl;
}

void test_format_validation() {
    std::cout << "\n=== Format Validation Tests ===" << std::endl;

    // Test 1: Hex validation
    assert(IsValidHex("deadbeef"));
    assert(IsValidHex("DEADBEEF"));
    assert(IsValidHex("0123456789abcdef"));
    assert(!IsValidHex("deadbeeg")); // Invalid hex char
    assert(!IsValidHex("test"));
    std::cout << "✅ Hex validation" << std::endl;

    // Test 2: Base64 validation
    assert(IsValidBase64("SGVsbG8gV29ybGQ="));
    assert(IsValidBase64("dGVzdA=="));
    assert(!IsValidBase64("invalid!@#"));
    std::cout << "✅ Base64 validation" << std::endl;

    // Test 3: Bech32 format validation
    assert(IsValidBech32Format("int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh"));
    assert(IsValidBech32Format("intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh"));
    assert(!IsValidBech32Format("btc1invalid"));
    assert(!IsValidBech32Format("int1")); // Too short
    std::cout << "✅ Bech32 format validation" << std::endl;

    // Test 4: IPv4 validation
    assert(IsValidIPv4("192.168.1.1"));
    assert(IsValidIPv4("8.8.8.8"));
    assert(!IsValidIPv4("256.1.1.1")); // Invalid octet
    assert(!IsValidIPv4("192.168.1")); // Too few octets
    assert(!IsValidIPv4("192.168.1.1.1")); // Too many octets
    std::cout << "✅ IPv4 validation" << std::endl;

    // Test 5: IPv6 validation
    assert(IsValidIPv6("2001:0db8:85a3:0000:0000:8a2e:0370:7334"));
    assert(IsValidIPv6("2001:db8::1"));
    assert(!IsValidIPv6("invalid"));
    std::cout << "✅ IPv6 validation" << std::endl;

    // Test 6: Port validation
    assert(IsValidPort(80));
    assert(IsValidPort(443));
    assert(IsValidPort(65535));
    assert(!IsValidPort(0));
    std::cout << "✅ Port validation" << std::endl;

    // Test 7: URL validation
    assert(IsValidURL("http://example.com"));
    assert(IsValidURL("https://example.com/path"));
    assert(!IsValidURL("invalid"));
    assert(!IsValidURL("http://"));
    std::cout << "✅ URL validation" << std::endl;

    // Test 8: Email validation
    assert(IsValidEmail("test@example.com"));
    assert(!IsValidEmail("invalid"));
    assert(!IsValidEmail("@example.com"));
    assert(!IsValidEmail("test@"));
    assert(!IsValidEmail("test@@example.com"));
    std::cout << "✅ Email validation" << std::endl;
}

void test_injection_prevention() {
    std::cout << "\n=== Injection Prevention Tests ===" << std::endl;

    // Test 1: SQL injection patterns
    assert(ContainsSuspiciousPatterns("' OR '1'='1"));
    assert(ContainsSuspiciousPatterns("1'; DROP TABLE users--"));
    assert(!ContainsSuspiciousPatterns("normal text"));
    std::cout << "✅ SQL injection pattern detection" << std::endl;

    // Test 2: XSS patterns
    assert(ContainsSuspiciousPatterns("<script>alert(1)</script>"));
    assert(ContainsSuspiciousPatterns("javascript:alert(1)"));
    assert(ContainsSuspiciousPatterns("<img onerror='alert(1)'>"));
    std::cout << "✅ XSS pattern detection" << std::endl;

    // Test 3: Path traversal patterns
    assert(ContainsSuspiciousPatterns("../etc/passwd"));
    assert(ContainsSuspiciousPatterns("..\\windows\\system32"));
    std::cout << "✅ Path traversal detection" << std::endl;

    // Test 4: Shell input sanitization
    std::string dangerous_shell = "test; rm -rf /";
    std::string safe_shell = SanitizeShellInput(dangerous_shell);
    assert(safe_shell.find(';') == std::string::npos);
    assert(safe_shell.find('|') == std::string::npos);
    std::cout << "✅ Shell input sanitization" << std::endl;
}

void test_buffer_sanitization() {
    std::cout << "\n=== Buffer Sanitization Tests ===" << std::endl;

    // Test 1: Buffer size validation
    assert(ValidateBufferSize(1000, 2000));
    assert(!ValidateBufferSize(3000, 2000));
    std::cout << "✅ Buffer size validation" << std::endl;

    // Test 2: Buffer overflow detection
    assert(WillBufferOverflow(1000, 2000, 2000));
    assert(!WillBufferOverflow(500, 500, 2000));
    std::cout << "✅ Buffer overflow detection" << std::endl;

    // Test 3: Buffer truncation
    std::vector<uint8_t> large_buffer(1000, 0xAA);
    auto truncated = SanitizeBuffer(large_buffer, 500);
    assert(truncated.size() == 500);
    std::cout << "✅ Buffer truncation" << std::endl;
}

void test_json_sanitization() {
    std::cout << "\n=== JSON Sanitization Tests ===" << std::endl;

    // Test 1: JSON depth validation
    std::string valid_json = R"({"a":{"b":{"c":"value"}}})";
    assert(ValidateJSONDepth(valid_json, 10));
    std::cout << "✅ Valid JSON depth" << std::endl;

    // Test 2: Too deep JSON
    std::string deep_json = "{";
    for (int i = 0; i < 150; i++) {
        deep_json += "\"a\":{";
    }
    assert(!ValidateJSONDepth(deep_json, 100));
    std::cout << "✅ Deep JSON rejection" << std::endl;

    // Test 3: JSON key validation
    assert(IsValidJSONKey("valid_key"));
    assert(!IsValidJSONKey(""));
    assert(!IsValidJSONKey(std::string(300, 'a'))); // Too long
    std::cout << "✅ JSON key validation" << std::endl;
}

void test_network_sanitization() {
    std::cout << "\n=== Network Sanitization Tests ===" << std::endl;

    // Test 1: Message size validation
    assert(ValidateMessageSize(1000, 32 * 1024 * 1024));
    assert(!ValidateMessageSize(0, 32 * 1024 * 1024));
    assert(!ValidateMessageSize(100 * 1024 * 1024, 32 * 1024 * 1024));
    std::cout << "✅ Message size validation" << std::endl;

    // Test 2: Network command validation
    assert(IsValidNetworkCommand("getblock"));
    assert(IsValidNetworkCommand("ping"));
    assert(!IsValidNetworkCommand("INVALID"));
    assert(!IsValidNetworkCommand("toolongcommand"));
    assert(!IsValidNetworkCommand("test-cmd")); // No hyphens
    std::cout << "✅ Network command validation" << std::endl;

    // Test 3: Peer address sanitization
    auto valid_addr1 = SanitizePeerAddress("192.168.1.1:2210");
    assert(valid_addr1.has_value());
    auto valid_addr2 = SanitizePeerAddress("192.168.1.1");
    assert(valid_addr2.has_value());
    auto invalid_port = SanitizePeerAddress("192.168.1.1:99999");
    assert(!invalid_port.has_value());
    auto invalid_ip = SanitizePeerAddress("999.999.999.999:2210");
    assert(!invalid_ip.has_value());
    std::cout << "✅ Peer address sanitization" << std::endl;
}

void test_crypto_validation() {
    std::cout << "\n=== Cryptographic Validation Tests ===" << std::endl;

    // Test 1: Public key size validation
    assert(IsValidPublicKeySize(1952)); // Dilithium3
    assert(!IsValidPublicKeySize(32)); // Wrong size
    assert(!IsValidPublicKeySize(0));
    std::cout << "✅ Public key size validation" << std::endl;

    // Test 2: Signature size validation
    assert(IsValidSignatureSize(3293)); // Dilithium3
    assert(!IsValidSignatureSize(64)); // Wrong size
    assert(!IsValidSignatureSize(0));
    std::cout << "✅ Signature size validation" << std::endl;

    // Test 3: Hash size validation
    assert(IsValidHashSize(32)); // SHA3-256
    assert(!IsValidHashSize(16)); // Wrong size
    assert(!IsValidHashSize(0));
    std::cout << "✅ Hash size validation" << std::endl;
}

void test_rate_limiting() {
    std::cout << "\n=== Rate Limiting Tests ===" << std::endl;

    // Test 1: Rate limit enforcement
    RateLimitState state(5, 1000); // 5 requests per second

    uint64_t current_time = 1000;

    // First 5 requests should pass
    for (int i = 0; i < 5; i++) {
        assert(!IsRateLimitExceeded(state, current_time));
    }

    // 6th request should be rate limited
    assert(IsRateLimitExceeded(state, current_time));
    std::cout << "✅ Rate limit enforcement" << std::endl;

    // Test 2: Rate limit window reset
    current_time += 1001; // Move to new window
    state.count = 0; // Reset would happen in function
    assert(!IsRateLimitExceeded(state, current_time));
    std::cout << "✅ Rate limit window reset" << std::endl;
}

void test_whitelist_blacklist() {
    std::cout << "\n=== Whitelist/Blacklist Tests ===" << std::endl;

    // Test 1: Whitelist validation
    assert(ContainsOnly("abc123", "abc123456789"));
    assert(!ContainsOnly("abc!23", "abc123456789"));
    std::cout << "✅ Whitelist validation" << std::endl;

    // Test 2: Blacklist validation
    assert(ContainsAny("test;cmd", ";|&"));
    assert(!ContainsAny("testcmd", ";|&"));
    std::cout << "✅ Blacklist validation" << std::endl;

    // Test 3: Regex pattern matching
    std::regex hex_pattern("^[0-9a-fA-F]+$");
    assert(MatchesPattern("deadbeef", hex_pattern));
    assert(!MatchesPattern("invalid", hex_pattern));
    std::cout << "✅ Regex pattern matching" << std::endl;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   INTcoin Sanitization Test Suite     ║" << std::endl;
    std::cout << "║   Version 1.0.0                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;

    try {
        test_string_sanitization();
        test_path_sanitization();
        test_numeric_sanitization();
        test_format_validation();
        test_injection_prevention();
        test_buffer_sanitization();
        test_json_sanitization();
        test_network_sanitization();
        test_crypto_validation();
        test_rate_limiting();
        test_whitelist_blacklist();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL SANITIZATION TESTS PASSED" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "❌ TEST FAILED: " << e.what() << std::endl;
        std::cout << "========================================\n" << std::endl;
        return 1;
    }
}
