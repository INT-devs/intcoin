// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_SCRIPT_VALIDATION_H
#define INTCOIN_SCRIPT_VALIDATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <stack>
#include <unordered_set>
#include <optional>
#include <memory>
#include <limits>
#include <cstring>

namespace intcoin {
namespace script {

// Script execution limits
namespace limits {
    constexpr size_t MAX_SCRIPT_SIZE = 10000;           // Maximum script size in bytes
    constexpr size_t MAX_SCRIPT_ELEMENT_SIZE = 520;     // Maximum size of stack element
    constexpr size_t MAX_OPS_PER_SCRIPT = 201;          // Maximum operations per script
    constexpr size_t MAX_STACK_SIZE = 1000;             // Maximum stack depth
    constexpr size_t MAX_PUBKEYS_PER_MULTISIG = 20;     // Maximum public keys in multisig
    constexpr uint32_t MAX_SCRIPT_RECURSION_DEPTH = 10; // Prevent infinite recursion
}

// Script opcodes (subset of Bitcoin-like opcodes)
enum class Opcode : uint8_t {
    // Constants
    OP_0 = 0x00,
    OP_FALSE = 0x00,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_1 = 0x51,
    OP_TRUE = 0x51,
    OP_2 = 0x52,
    OP_16 = 0x60,

    // Flow control
    OP_NOP = 0x61,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
    OP_RETURN = 0x6a,

    // Stack operations
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,

    // Splice operations
    OP_SIZE = 0x82,

    // Bitwise logic
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,

    // Arithmetic
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,
    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,
    OP_WITHIN = 0xa5,

    // Crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,

    // Post-quantum crypto (INTcoin-specific)
    OP_CHECKDILITHIUMSIG = 0xf0,
    OP_CHECKDILITHIUMSIGVERIFY = 0xf1,

    // Disabled opcodes (for security)
    OP_CAT = 0x7e,           // Disabled (can cause memory issues)
    OP_SUBSTR = 0x7f,        // Disabled
    OP_LEFT = 0x80,          // Disabled
    OP_RIGHT = 0x81,         // Disabled
    OP_INVERT = 0x83,        // Disabled
    OP_AND = 0x84,           // Disabled
    OP_OR = 0x85,            // Disabled
    OP_XOR = 0x86,           // Disabled
    OP_2MUL = 0x8d,          // Disabled
    OP_2DIV = 0x8e,          // Disabled
    OP_MUL = 0x95,           // Disabled
    OP_DIV = 0x96,           // Disabled
    OP_MOD = 0x97,           // Disabled
    OP_LSHIFT = 0x98,        // Disabled
    OP_RSHIFT = 0x99,        // Disabled

    // Invalid opcode marker
    OP_INVALIDOPCODE = 0xff
};

// Check if opcode is disabled
inline bool is_disabled_opcode(Opcode op) {
    return op == Opcode::OP_CAT || op == Opcode::OP_SUBSTR ||
           op == Opcode::OP_LEFT || op == Opcode::OP_RIGHT ||
           op == Opcode::OP_INVERT || op == Opcode::OP_AND ||
           op == Opcode::OP_OR || op == Opcode::OP_XOR ||
           op == Opcode::OP_2MUL || op == Opcode::OP_2DIV ||
           op == Opcode::OP_MUL || op == Opcode::OP_DIV ||
           op == Opcode::OP_MOD || op == Opcode::OP_LSHIFT ||
           op == Opcode::OP_RSHIFT;
}

// Script serialization (canonical, deterministic)
class ScriptSerializer {
public:
    // Serialize script to bytes (canonical form)
    static std::vector<uint8_t> serialize(const std::vector<uint8_t>& script) {
        // Scripts are already in byte form, but we ensure canonical encoding
        std::vector<uint8_t> result;

        size_t i = 0;
        while (i < script.size()) {
            uint8_t opcode = script[i];

            // Handle push data opcodes
            if (opcode <= 0x4b) {
                // Direct push of N bytes
                size_t push_size = opcode;
                result.push_back(opcode);
                i++;

                if (i + push_size > script.size()) {
                    throw std::runtime_error("Invalid push: not enough data");
                }

                for (size_t j = 0; j < push_size; ++j) {
                    result.push_back(script[i++]);
                }
            } else if (opcode == static_cast<uint8_t>(Opcode::OP_PUSHDATA1)) {
                result.push_back(opcode);
                i++;
                if (i >= script.size()) {
                    throw std::runtime_error("Invalid OP_PUSHDATA1");
                }
                uint8_t size = script[i++];
                result.push_back(size);

                for (uint8_t j = 0; j < size && i < script.size(); ++j) {
                    result.push_back(script[i++]);
                }
            } else {
                // Regular opcode
                result.push_back(opcode);
                i++;
            }
        }

        return result;
    }

    // Validate canonical encoding
    static bool is_canonical(const std::vector<uint8_t>& script) {
        try {
            auto serialized = serialize(script);
            return serialized == script;
        } catch (...) {
            return false;
        }
    }

    // Check for ambiguous encodings
    static bool has_ambiguous_encoding(const std::vector<uint8_t>& script) {
        size_t i = 0;
        while (i < script.size()) {
            uint8_t opcode = script[i++];

            if (opcode <= 0x4b) {
                // Push of N bytes - check if it should use PUSHDATA1 instead
                if (opcode > 75 && i + opcode <= script.size()) {
                    return true;  // Should use PUSHDATA1 for sizes >75
                }
                i += opcode;
            } else if (opcode == static_cast<uint8_t>(Opcode::OP_PUSHDATA1)) {
                if (i >= script.size()) return true;
                uint8_t size = script[i++];
                if (size <= 75) {
                    return true;  // Should use direct push for sizes ≤75
                }
                i += size;
            }
        }
        return false;
    }
};

// Endianness handler (deterministic across platforms)
class EndiannessHandler {
public:
    // Convert to little-endian (canonical format for scripts)
    static std::vector<uint8_t> to_little_endian(uint64_t value) {
        std::vector<uint8_t> result(8);
        for (size_t i = 0; i < 8; ++i) {
            result[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
        return result;
    }

    // Convert from little-endian
    static uint64_t from_little_endian(const std::vector<uint8_t>& data) {
        if (data.size() > 8) {
            throw std::runtime_error("Data too large for uint64_t");
        }

        uint64_t result = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            result |= (static_cast<uint64_t>(data[i]) << (i * 8));
        }
        return result;
    }

    // Check if system is little-endian
    static bool is_little_endian() {
        uint16_t test = 0x0001;
        return *reinterpret_cast<uint8_t*>(&test) == 0x01;
    }
};

// Script execution engine
class ScriptExecutor {
private:
    std::stack<std::vector<uint8_t>> stack;
    std::stack<std::vector<uint8_t>> alt_stack;
    size_t op_count = 0;
    uint32_t recursion_depth = 0;
    bool execution_valid = true;

    // Re-entrancy protection
    bool is_executing = false;
    std::unordered_set<std::string> executed_scripts;

    struct Statistics {
        uint64_t scripts_executed = 0;
        uint64_t scripts_passed = 0;
        uint64_t scripts_failed = 0;
        uint64_t reentrant_calls_blocked = 0;
        uint64_t disabled_opcodes_blocked = 0;
    } stats;

public:
    struct ExecutionResult {
        bool success;
        std::string error;
        size_t operations_executed;
        bool stack_empty;
    };

    // Execute script with re-entrancy protection
    ExecutionResult execute(
        const std::vector<uint8_t>& script,
        const std::vector<uint8_t>& transaction_hash = {}
    ) {
        stats.scripts_executed++;
        ExecutionResult result;
        result.operations_executed = 0;

        // Re-entrancy protection
        if (is_executing) {
            stats.reentrant_calls_blocked++;
            result.success = false;
            result.error = "Re-entrant script execution blocked";
            stats.scripts_failed++;
            return result;
        }

        // Create script identifier for duplicate detection
        std::string script_id = compute_script_hash(script);
        if (executed_scripts.count(script_id) > 0) {
            stats.reentrant_calls_blocked++;
            result.success = false;
            result.error = "Script already executed (duplicate execution blocked)";
            stats.scripts_failed++;
            return result;
        }

        // Set re-entrancy guard
        is_executing = true;
        executed_scripts.insert(script_id);

        // Validate script size
        if (script.size() > limits::MAX_SCRIPT_SIZE) {
            result.success = false;
            result.error = "Script too large";
            is_executing = false;
            stats.scripts_failed++;
            return result;
        }

        // Validate canonical encoding
        if (!ScriptSerializer::is_canonical(script)) {
            result.success = false;
            result.error = "Script encoding is not canonical";
            is_executing = false;
            stats.scripts_failed++;
            return result;
        }

        // Check for ambiguous encodings
        if (ScriptSerializer::has_ambiguous_encoding(script)) {
            result.success = false;
            result.error = "Script has ambiguous encoding";
            is_executing = false;
            stats.scripts_failed++;
            return result;
        }

        // Reset state
        while (!stack.empty()) stack.pop();
        while (!alt_stack.empty()) alt_stack.pop();
        op_count = 0;
        execution_valid = true;

        // Execute script
        size_t pc = 0;  // Program counter
        while (pc < script.size() && execution_valid) {
            // Check operation count limit
            if (op_count >= limits::MAX_OPS_PER_SCRIPT) {
                result.success = false;
                result.error = "Operation count limit exceeded";
                is_executing = false;
                stats.scripts_failed++;
                return result;
            }

            uint8_t opcode_byte = script[pc++];
            Opcode opcode = static_cast<Opcode>(opcode_byte);

            // Check for disabled opcodes
            if (is_disabled_opcode(opcode)) {
                stats.disabled_opcodes_blocked++;
                result.success = false;
                result.error = "Disabled opcode encountered";
                is_executing = false;
                stats.scripts_failed++;
                return result;
            }

            // Execute opcode
            if (!execute_opcode(opcode, script, pc)) {
                result.success = false;
                result.error = "Opcode execution failed";
                is_executing = false;
                stats.scripts_failed++;
                return result;
            }

            op_count++;
            result.operations_executed++;
        }

        // Check final state
        result.stack_empty = stack.empty();
        if (!execution_valid) {
            result.success = false;
            result.error = "Script execution validation failed";
            stats.scripts_failed++;
        } else if (stack.empty()) {
            result.success = false;
            result.error = "Stack empty after execution";
            stats.scripts_failed++;
        } else {
            // Check top of stack
            auto top = stack.top();
            result.success = !top.empty() && cast_to_bool(top);
            if (result.success) {
                stats.scripts_passed++;
            } else {
                stats.scripts_failed++;
            }
        }

        // Clear re-entrancy guard
        is_executing = false;

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Clear executed scripts set (for new block)
    void clear_executed_scripts() {
        executed_scripts.clear();
    }

private:
    // Execute single opcode (deterministic)
    bool execute_opcode(Opcode op, const std::vector<uint8_t>& script, size_t& pc) {
        // Check stack size limit
        if (stack.size() > limits::MAX_STACK_SIZE) {
            return false;
        }

        switch (op) {
            // Constants
            case Opcode::OP_0:
            case Opcode::OP_FALSE:
                stack.push({});
                break;

            case Opcode::OP_1:
            case Opcode::OP_TRUE:
                stack.push({0x01});
                break;

            // Stack operations
            case Opcode::OP_DUP:
                if (stack.empty()) return false;
                stack.push(stack.top());
                break;

            case Opcode::OP_DROP:
                if (stack.empty()) return false;
                stack.pop();
                break;

            case Opcode::OP_SWAP:
                if (stack.size() < 2) return false;
                {
                    auto a = stack.top(); stack.pop();
                    auto b = stack.top(); stack.pop();
                    stack.push(a);
                    stack.push(b);
                }
                break;

            // Verification
            case Opcode::OP_VERIFY:
                if (stack.empty()) return false;
                {
                    auto value = stack.top();
                    stack.pop();
                    if (!cast_to_bool(value)) {
                        execution_valid = false;
                        return false;
                    }
                }
                break;

            case Opcode::OP_RETURN:
                execution_valid = false;
                return false;

            case Opcode::OP_EQUAL:
                if (stack.size() < 2) return false;
                {
                    auto a = stack.top(); stack.pop();
                    auto b = stack.top(); stack.pop();
                    stack.push(a == b ? std::vector<uint8_t>{0x01} : std::vector<uint8_t>{});
                }
                break;

            case Opcode::OP_EQUALVERIFY:
                if (stack.size() < 2) return false;
                {
                    auto a = stack.top(); stack.pop();
                    auto b = stack.top(); stack.pop();
                    if (a != b) {
                        execution_valid = false;
                        return false;
                    }
                }
                break;

            // Checksig (simplified - real implementation would verify signature)
            case Opcode::OP_CHECKSIG:
                if (stack.size() < 2) return false;
                {
                    auto pubkey = stack.top(); stack.pop();
                    auto signature = stack.top(); stack.pop();
                    // Simplified: just check they're non-empty
                    bool valid = !pubkey.empty() && !signature.empty();
                    stack.push(valid ? std::vector<uint8_t>{0x01} : std::vector<uint8_t>{});
                }
                break;

            case Opcode::OP_CHECKSIGVERIFY:
                if (stack.size() < 2) return false;
                {
                    auto pubkey = stack.top(); stack.pop();
                    auto signature = stack.top(); stack.pop();
                    if (pubkey.empty() || signature.empty()) {
                        execution_valid = false;
                        return false;
                    }
                }
                break;

            // Post-quantum signature verification (INTcoin-specific)
            case Opcode::OP_CHECKDILITHIUMSIG:
                if (stack.size() < 2) return false;
                {
                    auto pubkey = stack.top(); stack.pop();
                    auto signature = stack.top(); stack.pop();
                    // Simplified: check sizes match Dilithium5
                    bool valid = (pubkey.size() == 2592) && (signature.size() == 4627);
                    stack.push(valid ? std::vector<uint8_t>{0x01} : std::vector<uint8_t>{});
                }
                break;

            default:
                // Unknown or unimplemented opcode
                return false;
        }

        return execution_valid;
    }

    // Cast stack element to boolean (deterministic)
    bool cast_to_bool(const std::vector<uint8_t>& data) const {
        if (data.empty()) {
            return false;
        }

        // Check for negative zero
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] != 0) {
                // Last byte can be 0x80 (negative zero)
                if (i == data.size() - 1 && data[i] == 0x80) {
                    return false;
                }
                return true;
            }
        }

        return false;
    }

    // Compute script hash (for duplicate detection)
    std::string compute_script_hash(const std::vector<uint8_t>& script) const {
        // Simplified: in production would use SHA256
        std::string result;
        for (uint8_t byte : script) {
            result += std::to_string(byte) + "_";
        }
        return result;
    }
};

// Script validator
class ScriptValidator {
private:
    ScriptExecutor executor;

    struct Statistics {
        uint64_t scripts_validated = 0;
        uint64_t validation_passed = 0;
        uint64_t validation_failed = 0;
    } stats;

public:
    struct ValidationResult {
        bool valid;
        std::string error;
        size_t script_size;
        size_t operations_count;
    };

    // Validate script without executing
    ValidationResult validate_script(const std::vector<uint8_t>& script) {
        stats.scripts_validated++;
        ValidationResult result;
        result.script_size = script.size();
        result.operations_count = 0;

        // Check size limit
        if (script.size() > limits::MAX_SCRIPT_SIZE) {
            result.valid = false;
            result.error = "Script exceeds maximum size";
            stats.validation_failed++;
            return result;
        }

        // Check canonical encoding
        if (!ScriptSerializer::is_canonical(script)) {
            result.valid = false;
            result.error = "Script encoding is not canonical";
            stats.validation_failed++;
            return result;
        }

        // Check for ambiguous encodings
        if (ScriptSerializer::has_ambiguous_encoding(script)) {
            result.valid = false;
            result.error = "Script has ambiguous encoding";
            stats.validation_failed++;
            return result;
        }

        // Count operations
        size_t pc = 0;
        size_t op_count = 0;

        while (pc < script.size()) {
            uint8_t opcode_byte = script[pc++];

            // Handle push data
            if (opcode_byte <= 0x4b) {
                pc += opcode_byte;
            } else if (opcode_byte == static_cast<uint8_t>(Opcode::OP_PUSHDATA1)) {
                if (pc >= script.size()) {
                    result.valid = false;
                    result.error = "Invalid PUSHDATA1";
                    stats.validation_failed++;
                    return result;
                }
                uint8_t size = script[pc++];
                pc += size;
            }

            // Check for disabled opcodes
            Opcode opcode = static_cast<Opcode>(opcode_byte);
            if (is_disabled_opcode(opcode)) {
                result.valid = false;
                result.error = "Disabled opcode in script";
                stats.validation_failed++;
                return result;
            }

            op_count++;
        }

        result.operations_count = op_count;

        // Check operation count limit
        if (op_count > limits::MAX_OPS_PER_SCRIPT) {
            result.valid = false;
            result.error = "Script exceeds maximum operation count";
            stats.validation_failed++;
            return result;
        }

        result.valid = true;
        stats.validation_passed++;
        return result;
    }

    // Validate and execute script
    ScriptExecutor::ExecutionResult validate_and_execute(
        const std::vector<uint8_t>& script_sig,
        const std::vector<uint8_t>& script_pubkey,
        const std::vector<uint8_t>& transaction_hash = {}
    ) {
        // First validate both scripts
        auto sig_validation = validate_script(script_sig);
        if (!sig_validation.valid) {
            ScriptExecutor::ExecutionResult result;
            result.success = false;
            result.error = "ScriptSig validation failed: " + sig_validation.error;
            return result;
        }

        auto pubkey_validation = validate_script(script_pubkey);
        if (!pubkey_validation.valid) {
            ScriptExecutor::ExecutionResult result;
            result.success = false;
            result.error = "ScriptPubKey validation failed: " + pubkey_validation.error;
            return result;
        }

        // Combine scripts (script_sig then script_pubkey)
        std::vector<uint8_t> combined_script;
        combined_script.insert(combined_script.end(), script_sig.begin(), script_sig.end());
        combined_script.insert(combined_script.end(), script_pubkey.begin(), script_pubkey.end());

        // Execute combined script
        return executor.execute(combined_script, transaction_hash);
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Get executor statistics
    const ScriptExecutor::Statistics& get_executor_statistics() const {
        return executor.get_statistics();
    }
};

} // namespace script
} // namespace intcoin

#endif // INTCOIN_SCRIPT_VALIDATION_H
