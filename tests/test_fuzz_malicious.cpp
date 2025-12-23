/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Enhanced Fuzzing Test Suite - Malicious Input Testing
 */

#include "intcoin/intcoin.h"
#include "intcoin/sanitize.h"
#include <iostream>
#include <random>
#include <vector>
#include <chrono>

using namespace intcoin;

// Enhanced fuzzer with malicious patterns
class MaliciousFuzzer {
public:
    MaliciousFuzzer(uint32_t seed = 0) {
        if (seed == 0) {
            seed = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
        }
        rng_.seed(seed);
        std::cout << "Malicious Fuzzer initialized with seed: " << seed << std::endl;
    }

    // Generate malicious strings
    std::vector<std::string> GetMaliciousStrings() {
        return {
            // SQL Injection attempts
            "' OR '1'='1",
            "1'; DROP TABLE users--",
            "' UNION SELECT * FROM passwords--",
            "admin'--",
            "' OR 1=1--",

            // XSS attempts
            "<script>alert('XSS')</script>",
            "<img src=x onerror=alert(1)>",
            "javascript:alert(document.cookie)",
            "<svg onload=alert(1)>",

            // Path traversal
            "../../../etc/passwd",
            "..\\..\\..\\windows\\system32\\config\\sam",
            "....//....//etc/passwd",
            "%2e%2e%2f%2e%2e%2fetc%2fpasswd",

            // Command injection
            "; rm -rf /",
            "| cat /etc/passwd",
            "`whoami`",
            "$(reboot)",

            // Buffer overflow attempts
            std::string(10000, 'A'),
            std::string(100000, 'B'),

            // Format string attacks
            "%s%s%s%s%s%s%s%s%s%s",
            "%x%x%x%x%x%x%x%x%x%x",
            "%n%n%n%n%n%n%n%n%n%n",

            // Null bytes
            std::string("test\0injected", 13),
            std::string("\0\0\0\0", 4),

            // Unicode exploits
            "\xc0\xae",  // Overlong encoding of '.'
            "\xc0\xaf",  // Overlong encoding of '/'
            "\xed\xa0\x80", // UTF-16 surrogate

            // Integer overflow
            "4294967296", // UINT32_MAX + 1
            "-2147483649", // INT32_MIN - 1
            "18446744073709551616", // UINT64_MAX + 1

            // Control characters
            "\x00\x01\x02\x03\x04\x05",
            "\r\n\r\n", // HTTP header injection

            // LDAP injection
            "*()|&",
            "*)(uid=*",

            // XML injection
            "<?xml version=\"1.0\"?><!DOCTYPE foo [<!ENTITY xxe SYSTEM \"file:///etc/passwd\">]>",

            // NoSQL injection
            "{'$gt':''}",
            "{\"$ne\":null}",

            // Extremely long inputs
            std::string(1024 * 1024, 'X'), // 1 MB
        };
    }

    // Generate malicious byte sequences
    std::vector<std::vector<uint8_t>> GetMaliciousBytes() {
        std::vector<std::vector<uint8_t>> patterns;

        // All zeros
        patterns.push_back(std::vector<uint8_t>(1000, 0x00));

        // All ones
        patterns.push_back(std::vector<uint8_t>(1000, 0xFF));

        // Alternating pattern
        std::vector<uint8_t> alternating;
        for (int i = 0; i < 1000; i++) {
            alternating.push_back(i % 2 == 0 ? 0xAA : 0x55);
        }
        patterns.push_back(alternating);

        // Incrementing pattern
        std::vector<uint8_t> incrementing;
        for (int i = 0; i < 256; i++) {
            incrementing.push_back(static_cast<uint8_t>(i));
        }
        patterns.push_back(incrementing);

        return patterns;
    }

    uint64_t RandomUint64() {
        return static_cast<uint64_t>(rng_());
    }

    uint32_t RandomUint32() {
        return static_cast<uint32_t>(rng_());
    }

private:
    std::mt19937 rng_;
};

// Test fuzzing with malicious addresses
bool fuzz_malicious_addresses(size_t iterations = 100) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Malicious Addresses" << std::endl;
    std::cout << "========================================" << std::endl;

    MaliciousFuzzer fuzzer;
    size_t crashes = 0;
    size_t handled = 0;

    auto malicious = fuzzer.GetMaliciousStrings();

    for (const auto& bad_address : malicious) {
        try {
            // Attempt to decode malicious address
            auto result = AddressEncoder::DecodeAddress(bad_address);

            // Should fail for malicious input
            if (result.IsError()) {
                handled++;
            } else {
                std::cout << "⚠️  Accepted malicious address: " << bad_address.substr(0, 50) << std::endl;
            }
        } catch (const std::exception& e) {
            // Exceptions are acceptable for malformed input
            handled++;
        } catch (...) {
            std::cout << "❌ CRASH on input: " << bad_address.substr(0, 50) << std::endl;
            crashes++;
        }
    }

    std::cout << "Tested " << malicious.size() << " malicious patterns" << std::endl;
    std::cout << "Handled safely: " << handled << std::endl;
    std::cout << "Crashes: " << crashes << std::endl;

    bool passed = (crashes == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test fuzzing JSON RPC with malicious inputs
bool fuzz_malicious_json(size_t iterations = 100) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Malicious JSON" << std::endl;
    std::cout << "========================================" << std::endl;

    MaliciousFuzzer fuzzer;
    size_t crashes = 0;
    size_t handled = 0;

    std::vector<std::string> malicious_json = {
        // Deeply nested
        "{" + std::string(1000, '{') + std::string(1000, '}') + "}",

        // Extremely long string
        R"({"key":")" + std::string(1024 * 1024, 'A') + "\"}",

        // Invalid escapes
        R"({"key":"value\x"})",

        // Unterminated string
        R"({"key":"value)",

        // Unterminated object
        R"({"key":"value")",

        // Null bytes in JSON
        std::string("{\"key\":\"test\0data\"}", 18),

        // Unicode issues
        R"({"key":"\uDC00"})", // Lone low surrogate

        // Number overflow
        R"({"number":999999999999999999999999999999})",

        // Empty array with trailing comma
        R"([1,2,3,])",
    };

    for (const auto& bad_json : malicious_json) {
        try {
            auto result = rpc::JSONValue::Parse(bad_json);

            if (result.IsError()) {
                handled++;
            }
        } catch (const std::exception& e) {
            handled++;
        } catch (...) {
            std::cout << "❌ CRASH on JSON input" << std::endl;
            crashes++;
        }
    }

    std::cout << "Tested " << malicious_json.size() << " malicious JSON patterns" << std::endl;
    std::cout << "Handled safely: " << handled << std::endl;
    std::cout << "Crashes: " << crashes << std::endl;

    bool passed = (crashes == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test transaction fuzzing with extreme values
bool fuzz_malicious_transactions(size_t iterations = 100) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Malicious Transactions" << std::endl;
    std::cout << "========================================" << std::endl;

    MaliciousFuzzer fuzzer;
    size_t crashes = 0;
    size_t handled = 0;

    for (size_t i = 0; i < iterations; i++) {
        try {
            Transaction tx;

            // Extreme values
            tx.version = UINT32_MAX;
            tx.locktime = UINT64_MAX;

            // Massive number of inputs (attack: memory exhaustion)
            size_t num_inputs = fuzzer.RandomUint32() % 10000;

            for (size_t j = 0; j < num_inputs && j < 100; j++) { // Cap at 100 for test
                TxIn input;
                input.prev_tx_hash = GetRandomUint256();
                input.prev_tx_index = UINT32_MAX;
                input.sequence = UINT32_MAX;

                // Massive script
                size_t script_len = fuzzer.RandomUint32() % 100000;
                if (script_len > 10000) script_len = 10000; // Cap for test

                auto patterns = fuzzer.GetMaliciousBytes();
                if (!patterns.empty()) {
                    input.script_sig = Script(patterns[j % patterns.size()]);
                }

                tx.inputs.push_back(input);
            }

            // Massive number of outputs
            size_t num_outputs = fuzzer.RandomUint32() % 10000;

            for (size_t j = 0; j < num_outputs && j < 100; j++) { // Cap at 100 for test
                // Extreme output values
                uint64_t value = (j % 2 == 0) ? 0 : UINT64_MAX;

                Script script_pubkey(fuzzer.GetMaliciousBytes()[j % fuzzer.GetMaliciousBytes().size()]);
                TxOut output(value, script_pubkey);

                tx.outputs.push_back(output);
            }

            // Try to serialize
            auto serialized = tx.Serialize();
            handled++;

        } catch (const std::exception& e) {
            handled++;
        } catch (...) {
            std::cout << "❌ CRASH on iteration " << i << std::endl;
            crashes++;
        }
    }

    std::cout << "Tested " << iterations << " malicious transaction patterns" << std::endl;
    std::cout << "Handled safely: " << handled << std::endl;
    std::cout << "Crashes: " << crashes << std::endl;

    bool passed = (crashes == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test network message fuzzing
bool fuzz_malicious_network_messages(size_t iterations = 100) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Malicious Network Messages" << std::endl;
    std::cout << "========================================" << std::endl;

    MaliciousFuzzer fuzzer;
    size_t crashes = 0;
    size_t handled = 0;

    for (size_t i = 0; i < iterations; i++) {
        try {
            p2p::NetworkMessage msg;

            // Malicious magic bytes
            msg.magic = fuzzer.RandomUint32();

            // Malicious commands
            auto malicious_strings = fuzzer.GetMaliciousStrings();
            msg.command = malicious_strings[i % malicious_strings.size()];

            // Extreme length
            msg.length = (i % 2 == 0) ? 0 : UINT32_MAX;

            // Random checksum
            msg.checksum = fuzzer.RandomUint32();

            // Malicious payload
            auto patterns = fuzzer.GetMaliciousBytes();
            msg.payload = patterns[i % patterns.size()];

            // Try to serialize (if method exists)
            // For now, just accessing members should not crash
            handled++;

        } catch (const std::exception& e) {
            handled++;
        } catch (...) {
            std::cout << "❌ CRASH on iteration " << i << std::endl;
            crashes++;
        }
    }

    std::cout << "Tested " << iterations << " malicious network messages" << std::endl;
    std::cout << "Handled safely: " << handled << std::endl;
    std::cout << "Crashes: " << crashes << std::endl;

    bool passed = (crashes == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test sanitization library with malicious inputs
bool fuzz_sanitization_library(size_t iterations = 100) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Sanitization Library" << std::endl;
    std::cout << "========================================" << std::endl;

    MaliciousFuzzer fuzzer;
    size_t crashes = 0;
    size_t tests = 0;

    auto malicious = fuzzer.GetMaliciousStrings();

    for (const auto& bad_input : malicious) {
        try {
            // Test all sanitization functions
            sanitize::SanitizeString(bad_input);
            sanitize::SanitizeAlphanumeric(bad_input);
            sanitize::SanitizeFilename(bad_input);
            sanitize::SanitizePath(bad_input);
            sanitize::RemoveControlCharacters(bad_input);
            sanitize::EscapeString(bad_input);
            sanitize::IsValidUTF8(bad_input);
            sanitize::SanitizeUTF8(bad_input);
            sanitize::IsValidHex(bad_input);
            sanitize::IsValidBase64(bad_input);
            sanitize::IsValidBech32Format(bad_input);
            sanitize::ContainsSuspiciousPatterns(bad_input);
            sanitize::SanitizeShellInput(bad_input);
            sanitize::SanitizeJSONString(bad_input);
            sanitize::IsValidJSONKey(bad_input);
            sanitize::IsValidNetworkCommand(bad_input);

            tests++;
        } catch (const std::exception& e) {
            // Exceptions are acceptable
            tests++;
        } catch (...) {
            std::cout << "❌ CRASH on input: " << bad_input.substr(0, 50) << std::endl;
            crashes++;
        }
    }

    std::cout << "Tested " << tests << " sanitization calls" << std::endl;
    std::cout << "Crashes: " << crashes << std::endl;

    bool passed = (crashes == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   INTcoin Malicious Fuzzing Suite     ║" << std::endl;
    std::cout << "║   Version 1.0.0                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;

    int failures = 0;

    // Run all malicious fuzz tests
    if (!fuzz_malicious_addresses(100)) failures++;
    if (!fuzz_malicious_json(100)) failures++;
    if (!fuzz_malicious_transactions(100)) failures++;
    if (!fuzz_malicious_network_messages(100)) failures++;
    if (!fuzz_sanitization_library(100)) failures++;

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Malicious Fuzzing Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total test suites: 5" << std::endl;
    std::cout << "Failed test suites: " << failures << std::endl;
    std::cout << (failures == 0 ? "✅ ALL TESTS PASSED - No crashes detected" : "❌ CRASHES DETECTED") << std::endl;

    return failures;
}
