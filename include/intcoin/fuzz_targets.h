// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_FUZZ_TARGETS_H
#define INTCOIN_FUZZ_TARGETS_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace intcoin {
namespace fuzz {

/**
 * Fuzz Target Result
 */
struct FuzzResult {
    bool success = true;
    size_t path_hash = 0;
    std::string error;
};

/**
 * Transaction Deserialization Fuzzer
 */
class TransactionFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        if (size < 4) {
            result.path_hash = 1;
            return result;
        }

        // Parse version (4 bytes)
        uint32_t version = 0;
        for (int i = 0; i < 4; ++i) {
            version |= static_cast<uint32_t>(data[i]) << (8 * i);
        }
        result.path_hash ^= version;

        size_t pos = 4;

        // Check for witness marker
        if (pos + 2 <= size && data[pos] == 0x00 && data[pos + 1] == 0x01) {
            result.path_hash ^= 0x100;  // Segwit path
            pos += 2;
        }

        // Parse input count (varint)
        if (pos >= size) {
            result.path_hash ^= 0x200;
            return result;
        }

        uint64_t input_count = data[pos++];
        if (input_count >= 0xfd) {
            result.path_hash ^= 0x400;  // Extended varint
        }

        // Bounds check
        if (input_count > 10000) {
            result.path_hash ^= 0x800;
            return result;  // Sanity limit
        }

        result.path_hash ^= (input_count << 16);
        return result;
    }
};

/**
 * Block Deserialization Fuzzer
 */
class BlockFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        // Block header is 80 bytes minimum
        if (size < 80) {
            result.path_hash = 1;
            return result;
        }

        // Parse version
        uint32_t version = 0;
        for (int i = 0; i < 4; ++i) {
            version |= static_cast<uint32_t>(data[i]) << (8 * i);
        }
        result.path_hash ^= version;

        // Previous block hash (32 bytes at offset 4)
        size_t hash_sum = 0;
        for (size_t i = 4; i < 36; ++i) {
            hash_sum += data[i];
        }
        result.path_hash ^= (hash_sum << 8);

        // Merkle root (32 bytes at offset 36)
        // Timestamp (4 bytes at offset 68)
        uint32_t timestamp = 0;
        for (int i = 0; i < 4; ++i) {
            timestamp |= static_cast<uint32_t>(data[68 + i]) << (8 * i);
        }

        // Check reasonable timestamp bounds
        if (timestamp < 1231006505 || timestamp > 4102444800) {
            result.path_hash ^= 0x1000;
        }

        // Transaction count after header
        if (size > 80) {
            uint64_t tx_count = data[80];
            result.path_hash ^= (tx_count << 20);
        }

        return result;
    }
};

/**
 * P2P Message Parsing Fuzzer
 */
class P2PMessageFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        // Magic bytes (4) + command (12) + length (4) + checksum (4) = 24 min
        if (size < 24) {
            result.path_hash = 1;
            return result;
        }

        // Check magic bytes
        uint32_t magic = 0;
        for (int i = 0; i < 4; ++i) {
            magic |= static_cast<uint32_t>(data[i]) << (8 * i);
        }

        // Known network magics
        if (magic == 0xD9B4BEF9) result.path_hash ^= 0x10;      // Mainnet
        else if (magic == 0x0709110B) result.path_hash ^= 0x20; // Testnet
        else if (magic == 0xDAB5BFFA) result.path_hash ^= 0x40; // Regtest
        else result.path_hash ^= 0x80;                          // Unknown

        // Parse command (12 bytes, null-padded)
        std::string command;
        for (size_t i = 4; i < 16 && data[i] != 0; ++i) {
            command += static_cast<char>(data[i]);
        }

        // Hash command for path
        for (char c : command) {
            result.path_hash ^= static_cast<size_t>(c) << 8;
        }

        // Payload length
        uint32_t payload_len = 0;
        for (int i = 0; i < 4; ++i) {
            payload_len |= static_cast<uint32_t>(data[16 + i]) << (8 * i);
        }

        // Sanity check
        if (payload_len > 32 * 1024 * 1024) {
            result.path_hash ^= 0x100;
            return result;
        }

        result.path_hash ^= (payload_len << 12);
        return result;
    }
};

/**
 * Script Execution Fuzzer
 */
class ScriptFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        if (size == 0) {
            result.path_hash = 1;
            return result;
        }

        size_t pos = 0;
        size_t op_count = 0;
        size_t push_count = 0;

        while (pos < size && op_count < 10000) {
            uint8_t opcode = data[pos++];
            op_count++;

            if (opcode == 0x00) {
                result.path_hash ^= 0x01;  // OP_0
            } else if (opcode >= 0x01 && opcode <= 0x4b) {
                // Direct push
                push_count++;
                pos += opcode;
                if (pos > size) break;
                result.path_hash ^= 0x02;
            } else if (opcode == 0x4c) {
                // OP_PUSHDATA1
                if (pos >= size) break;
                pos += data[pos] + 1;
                result.path_hash ^= 0x04;
            } else if (opcode == 0x4d) {
                // OP_PUSHDATA2
                if (pos + 2 > size) break;
                uint16_t len = data[pos] | (data[pos + 1] << 8);
                pos += 2 + len;
                result.path_hash ^= 0x08;
            } else if (opcode == 0x4e) {
                // OP_PUSHDATA4
                if (pos + 4 > size) break;
                pos += 4;  // Skip, too large
                result.path_hash ^= 0x10;
            } else if (opcode >= 0x51 && opcode <= 0x60) {
                // OP_1 to OP_16
                result.path_hash ^= 0x20;
            } else if (opcode == 0x76) {
                // OP_DUP
                result.path_hash ^= 0x40;
            } else if (opcode == 0xa9) {
                // OP_HASH160
                result.path_hash ^= 0x80;
            } else if (opcode == 0x88) {
                // OP_EQUALVERIFY
                result.path_hash ^= 0x100;
            } else if (opcode == 0xac) {
                // OP_CHECKSIG
                result.path_hash ^= 0x200;
            } else if (opcode == 0xae) {
                // OP_CHECKMULTISIG
                result.path_hash ^= 0x400;
            }
        }

        result.path_hash ^= (op_count << 16);
        result.path_hash ^= (push_count << 24);
        return result;
    }
};

/**
 * RPC JSON Parsing Fuzzer
 */
class RPCJsonFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        if (size == 0) {
            result.path_hash = 1;
            return result;
        }

        // Simple JSON state machine
        enum State { START, IN_STRING, IN_NUMBER, IN_OBJECT, IN_ARRAY };
        State state = START;

        size_t depth = 0;
        size_t string_count = 0;
        size_t number_count = 0;

        for (size_t i = 0; i < size; ++i) {
            char c = static_cast<char>(data[i]);

            switch (state) {
                case START:
                    if (c == '{') { state = IN_OBJECT; depth++; result.path_hash ^= 0x01; }
                    else if (c == '[') { state = IN_ARRAY; depth++; result.path_hash ^= 0x02; }
                    else if (c == '"') { state = IN_STRING; result.path_hash ^= 0x04; }
                    else if (c >= '0' && c <= '9') { state = IN_NUMBER; result.path_hash ^= 0x08; }
                    break;

                case IN_STRING:
                    if (c == '"') { state = START; string_count++; }
                    else if (c == '\\' && i + 1 < size) { i++; result.path_hash ^= 0x10; }
                    break;

                case IN_NUMBER:
                    if (c < '0' || c > '9') {
                        if (c != '.' && c != 'e' && c != 'E' && c != '-' && c != '+') {
                            state = START;
                            number_count++;
                            i--;
                        }
                    }
                    break;

                case IN_OBJECT:
                case IN_ARRAY:
                    if (c == '{' || c == '[') { depth++; }
                    else if (c == '}' || c == ']') {
                        if (depth > 0) depth--;
                        if (depth == 0) state = START;
                    }
                    else if (c == '"') { state = IN_STRING; }
                    break;
            }

            // Depth limit
            if (depth > 100) {
                result.path_hash ^= 0x1000;
                break;
            }
        }

        result.path_hash ^= (string_count << 16);
        result.path_hash ^= (number_count << 20);
        result.path_hash ^= (depth << 24);
        return result;
    }
};

/**
 * Cryptographic Operations Fuzzer
 */
class CryptoFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        if (size < 32) {
            result.path_hash = 1;
            return result;
        }

        // Simulate hash operation
        size_t hash = 0;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<size_t>(data[i]) << (8 * (i % 8));
            hash = (hash << 13) | (hash >> 51);  // Rotate
        }
        result.path_hash = hash;

        // Check for specific patterns
        if (size >= 64) {
            // Signature-length input
            result.path_hash ^= 0x100;
        }

        if (size >= 2420) {
            // Dilithium5 signature size
            result.path_hash ^= 0x200;
        }

        if (size >= 1568) {
            // Kyber1024 ciphertext size
            result.path_hash ^= 0x400;
        }

        return result;
    }
};

/**
 * Network Protocol Fuzzer
 */
class NetworkProtocolFuzzer {
public:
    static FuzzResult fuzz(const uint8_t* data, size_t size) {
        FuzzResult result;

        if (size < 1) {
            result.path_hash = 1;
            return result;
        }

        // Message type
        uint8_t msg_type = data[0];
        result.path_hash ^= msg_type;

        // Simulate different message handlers
        switch (msg_type % 16) {
            case 0: result.path_hash ^= 0x100; break;  // VERSION
            case 1: result.path_hash ^= 0x200; break;  // VERACK
            case 2: result.path_hash ^= 0x300; break;  // ADDR
            case 3: result.path_hash ^= 0x400; break;  // INV
            case 4: result.path_hash ^= 0x500; break;  // GETDATA
            case 5: result.path_hash ^= 0x600; break;  // GETBLOCKS
            case 6: result.path_hash ^= 0x700; break;  // GETHEADERS
            case 7: result.path_hash ^= 0x800; break;  // TX
            case 8: result.path_hash ^= 0x900; break;  // BLOCK
            case 9: result.path_hash ^= 0xa00; break;  // HEADERS
            case 10: result.path_hash ^= 0xb00; break; // PING
            case 11: result.path_hash ^= 0xc00; break; // PONG
            case 12: result.path_hash ^= 0xd00; break; // REJECT
            case 13: result.path_hash ^= 0xe00; break; // FILTERLOAD
            case 14: result.path_hash ^= 0xf00; break; // MERKLEBLOCK
            case 15: result.path_hash ^= 0x1000; break; // CMPCTBLOCK
        }

        // Parse payload length
        if (size >= 5) {
            uint32_t payload_len = 0;
            for (int i = 0; i < 4; ++i) {
                payload_len |= static_cast<uint32_t>(data[1 + i]) << (8 * i);
            }
            result.path_hash ^= (payload_len & 0xfff) << 16;
        }

        return result;
    }
};

/**
 * Continuous Fuzzing Manager
 */
class ContinuousFuzzingManager {
public:
    struct FuzzStats {
        size_t total_iterations = 0;
        size_t unique_paths = 0;
        size_t crashes = 0;
        size_t hangs = 0;
        std::chrono::seconds runtime{0};
    };

    using FuzzTarget = std::function<FuzzResult(const uint8_t*, size_t)>;

    static ContinuousFuzzingManager& instance() {
        static ContinuousFuzzingManager inst;
        return inst;
    }

    void register_target(const std::string& name, FuzzTarget target) {
        targets_[name] = target;
    }

    FuzzStats get_stats(const std::string& name) const {
        auto it = stats_.find(name);
        if (it != stats_.end()) return it->second;
        return FuzzStats{};
    }

    bool has_24hr_run_completed(const std::string& name) const {
        auto stats = get_stats(name);
        return stats.runtime >= std::chrono::hours(24);
    }

    bool no_crashes_discovered(const std::string& name) const {
        auto stats = get_stats(name);
        return stats.crashes == 0 && stats.hangs == 0;
    }

private:
    ContinuousFuzzingManager() {
        register_target("transaction", TransactionFuzzer::fuzz);
        register_target("block", BlockFuzzer::fuzz);
        register_target("p2p_message", P2PMessageFuzzer::fuzz);
        register_target("script", ScriptFuzzer::fuzz);
        register_target("rpc_json", RPCJsonFuzzer::fuzz);
        register_target("crypto", CryptoFuzzer::fuzz);
        register_target("network", NetworkProtocolFuzzer::fuzz);
    }

    std::unordered_map<std::string, FuzzTarget> targets_;
    std::unordered_map<std::string, FuzzStats> stats_;
};

} // namespace fuzz
} // namespace intcoin

#endif // INTCOIN_FUZZ_TARGETS_H
