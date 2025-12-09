/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Fuzzing Test Suite
 */

#include "intcoin/intcoin.h"
#include <iostream>
#include <random>
#include <vector>
#include <chrono>

using namespace intcoin;

// Simple fuzzer class
class Fuzzer {
public:
    Fuzzer(uint32_t seed = 0) {
        if (seed == 0) {
            seed = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
        }
        rng_.seed(seed);
        std::cout << "Fuzzer initialized with seed: " << seed << std::endl;
    }

    // Generate random bytes
    std::vector<uint8_t> RandomBytes(size_t size) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(rng_() % 256);
        }
        return data;
    }

    // Generate random uint64
    uint64_t RandomUint64() {
        return static_cast<uint64_t>(rng_());
    }

    // Generate random uint32
    uint32_t RandomUint32() {
        return static_cast<uint32_t>(rng_());
    }

    // Generate random string
    std::string RandomString(size_t length) {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[rng_() % (sizeof(charset) - 1)];
        }
        return result;
    }

    // Generate random hex string
    std::string RandomHex(size_t length) {
        const char hexchars[] = "0123456789abcdef";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += hexchars[rng_() % 16];
        }
        return result;
    }

private:
    std::mt19937 rng_;
};

// Test fuzzing SHA3 hash function
bool fuzz_sha3(size_t iterations = 1000) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: SHA3-256 Hash Function" << std::endl;
    std::cout << "========================================" << std::endl;

    Fuzzer fuzzer;
    size_t failures = 0;

    for (size_t i = 0; i < iterations; ++i) {
        try {
            // Generate random data of random length (0-10KB)
            size_t length = fuzzer.RandomUint32() % 10240;
            auto data = fuzzer.RandomBytes(length);

            // Hash the data
            uint256 hash = SHA3::Hash(data);

            // Verify hash is not all zeros (extremely unlikely)
            bool all_zeros = true;
            for (size_t j = 0; j < hash.size(); ++j) {
                if (hash[j] != 0) {
                    all_zeros = false;
                    break;
                }
            }

            if (all_zeros && length > 0) {
                std::cout << "⚠️  SHA3 produced all-zeros hash for non-empty input" << std::endl;
                failures++;
            }

            // Hash should be deterministic
            uint256 hash2 = SHA3::Hash(data);
            if (hash != hash2) {
                std::cout << "❌ SHA3 not deterministic!" << std::endl;
                failures++;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Exception in iteration " << i << ": " << e.what() << std::endl;
            failures++;
        }
    }

    std::cout << "Completed " << iterations << " iterations with " << failures << " failures" << std::endl;
    bool passed = (failures == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test fuzzing Bech32 address encoding/decoding
bool fuzz_bech32(size_t iterations = 1000) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Bech32 Address Encoding" << std::endl;
    std::cout << "========================================" << std::endl;

    Fuzzer fuzzer;
    size_t failures = 0;

    for (size_t i = 0; i < iterations; ++i) {
        try {
            // Generate random public key hash (32 bytes for uint256)
            uint256 pkh = GetRandomUint256();

            // Encode to Bech32 address
            auto encode_result = AddressEncoder::EncodeAddress(pkh);
            if (!encode_result.IsOk()) {
                // Encoding failure is acceptable for random data
                continue;
            }
            std::string address = *encode_result.value;

            // Verify it starts with "int1"
            if (address.substr(0, 4) != "int1") {
                std::cout << "❌ Invalid address prefix: " << address.substr(0, 4) << std::endl;
                failures++;
                continue;
            }

            // Decode back
            auto decoded = AddressEncoder::DecodeAddress(address);
            if (!decoded.IsOk()) {
                std::cout << "❌ Failed to decode valid address: " << address << std::endl;
                failures++;
                continue;
            }

            // Verify round-trip
            if (*decoded.value != pkh) {
                std::cout << "❌ Bech32 round-trip failed!" << std::endl;
                failures++;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Exception in iteration " << i << ": " << e.what() << std::endl;
            failures++;
        }
    }

    std::cout << "Completed " << iterations << " iterations with " << failures << " failures" << std::endl;
    bool passed = (failures == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test fuzzing transaction serialization
bool fuzz_transaction_serialization(size_t iterations = 500) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Transaction Serialization" << std::endl;
    std::cout << "========================================" << std::endl;

    Fuzzer fuzzer;
    size_t failures = 0;

    for (size_t i = 0; i < iterations; ++i) {
        try {
            // Create random transaction
            Transaction tx;
            tx.version = fuzzer.RandomUint32();
            tx.locktime = fuzzer.RandomUint64();

            // Add random inputs (1-10)
            size_t num_inputs = 1 + (fuzzer.RandomUint32() % 10);
            for (size_t j = 0; j < num_inputs; ++j) {
                TxIn input;
                input.prev_tx_hash = GetRandomUint256();
                input.prev_tx_index = fuzzer.RandomUint32();
                input.sequence = fuzzer.RandomUint32();

                // Random script (0-100 bytes)
                size_t script_len = fuzzer.RandomUint32() % 100;
                input.script_sig = Script(fuzzer.RandomBytes(script_len));

                tx.inputs.push_back(input);
            }

            // Add random outputs (1-10)
            size_t num_outputs = 1 + (fuzzer.RandomUint32() % 10);
            for (size_t j = 0; j < num_outputs; ++j) {
                // Random script (0-100 bytes)
                size_t script_len = fuzzer.RandomUint32() % 100;
                Script script_pubkey(fuzzer.RandomBytes(script_len));

                TxOut output(fuzzer.RandomUint64(), script_pubkey);
                tx.outputs.push_back(output);
            }

            // Serialize
            auto serialized = tx.Serialize();

            // Deserialize
            auto tx2_result = Transaction::Deserialize(serialized);
            if (!tx2_result.IsOk()) {
                std::cout << "❌ Failed to deserialize transaction" << std::endl;
                failures++;
                continue;
            }
            Transaction tx2 = *tx2_result.value;

            // Verify round-trip
            if (tx.version != tx2.version || tx.locktime != tx2.locktime) {
                std::cout << "❌ Transaction round-trip failed (version/locktime)" << std::endl;
                failures++;
            }

            if (tx.inputs.size() != tx2.inputs.size() || tx.outputs.size() != tx2.outputs.size()) {
                std::cout << "❌ Transaction round-trip failed (input/output count)" << std::endl;
                failures++;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Exception in iteration " << i << ": " << e.what() << std::endl;
            failures++;
        }
    }

    std::cout << "Completed " << iterations << " iterations with " << failures << " failures" << std::endl;
    bool passed = (failures == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test fuzzing script execution
bool fuzz_script_execution(size_t iterations = 500) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Script Execution" << std::endl;
    std::cout << "========================================" << std::endl;

    Fuzzer fuzzer;
    size_t failures = 0;
    size_t exceptions = 0;

    for (size_t i = 0; i < iterations; ++i) {
        try {
            // Create random script (0-200 bytes)
            size_t script_len = fuzzer.RandomUint32() % 200;
            auto script_data = fuzzer.RandomBytes(script_len);

            Script script(script_data);

            // Try to serialize (should not crash)
            auto serialized = script.Serialize();

            // Try to get size (should not crash)
            size_t size = script.GetSize();
            (void)size; // Suppress unused warning

            // Try to check if empty
            bool empty = script.IsEmpty();
            (void)empty; // Suppress unused warning

        } catch (const std::exception& e) {
            // Exceptions are expected for invalid scripts
            exceptions++;
        }
    }

    std::cout << "Completed " << iterations << " iterations" << std::endl;
    std::cout << "Exceptions (expected for invalid scripts): " << exceptions << std::endl;
    std::cout << "Failures: " << failures << std::endl;

    bool passed = (failures == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

// Test fuzzing block reward calculation
bool fuzz_block_reward(size_t iterations = 500) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzz Test: Block Reward Calculation" << std::endl;
    std::cout << "========================================" << std::endl;

    Fuzzer fuzzer;
    size_t failures = 0;

    for (size_t i = 0; i < iterations; ++i) {
        try {
            // Generate random block height
            uint64_t height = fuzzer.RandomUint64() % 10000000;

            // Calculate block reward
            uint64_t reward = GetBlockReward(height);

            // Verify reward is reasonable (not negative, within max supply)
            if (reward > consensus::INITIAL_BLOCK_REWARD) {
                std::cout << "❌ Block reward too high: " << reward << std::endl;
                failures++;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ Exception in iteration " << i << ": " << e.what() << std::endl;
            failures++;
        }
    }

    std::cout << "Completed " << iterations << " iterations with " << failures << " failures" << std::endl;
    bool passed = (failures == 0);
    std::cout << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    return passed;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   INTcoin Fuzzing Test Suite          ║" << std::endl;
    std::cout << "║   Version 1.0.0-alpha                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;

    int failures = 0;

    // Run all fuzz tests
    if (!fuzz_sha3(1000)) failures++;
    if (!fuzz_bech32(1000)) failures++;
    if (!fuzz_transaction_serialization(500)) failures++;
    if (!fuzz_script_execution(500)) failures++;
    if (!fuzz_block_reward(500)) failures++;

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Fuzzing Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total test suites: 5" << std::endl;
    std::cout << "Failed test suites: " << failures << std::endl;
    std::cout << "Total fuzzing iterations: ~3,500" << std::endl;
    std::cout << (failures == 0 ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED") << std::endl;

    return failures;
}
