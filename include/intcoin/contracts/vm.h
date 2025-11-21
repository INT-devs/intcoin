// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONTRACTS_VM_H
#define INTCOIN_CONTRACTS_VM_H

#include "../primitives.h"
#include "../transaction.h"
#include <vector>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <limits>
#include <mutex>

namespace intcoin {
namespace contracts {

/**
 * Opcode definitions for smart contract VM
 */
enum class OpCode : uint8_t {
    // Arithmetic
    ADD = 0x01,
    SUB = 0x02,
    MUL = 0x03,
    DIV = 0x04,
    MOD = 0x05,

    // Comparison
    LT = 0x10,
    GT = 0x11,
    EQ = 0x12,

    // Logic
    AND = 0x20,
    OR = 0x21,
    NOT = 0x22,

    // Stack
    PUSH = 0x30,
    POP = 0x31,
    DUP = 0x32,
    SWAP = 0x33,

    // Storage
    SLOAD = 0x40,
    SSTORE = 0x41,

    // Control flow
    JUMP = 0x50,
    JUMPI = 0x51,
    JUMPDEST = 0x52,
    RETURN = 0x53,
    REVERT = 0x54,
    STOP = 0x55,

    // Blockchain access
    ADDRESS = 0x60,
    BALANCE = 0x61,
    CALLER = 0x62,
    CALLVALUE = 0x63,
    BLOCKNUMBER = 0x64,
    TIMESTAMP = 0x65,

    // Crypto
    SHA256 = 0x70,
    DILITHIUM_VERIFY = 0x71,

    // Call
    CALL = 0x80,
    DELEGATECALL = 0x81,
    STATICCALL = 0x82,

    // Create
    CREATE = 0x90,

    // Invalid
    INVALID = 0xFF
};

/**
 * Execution result
 */
enum class ExecutionResult {
    SUCCESS,
    OUT_OF_GAS,
    STACK_OVERFLOW,
    STACK_UNDERFLOW,
    INVALID_OPCODE,
    INVALID_JUMP,
    REVERT,
    MEMORY_OVERFLOW,
    DIVIDE_BY_ZERO,
    INTEGER_OVERFLOW
};

/**
 * Execution context for contract calls
 */
struct ExecutionContext {
    Hash256 contract_address;
    Hash256 caller;
    uint64_t call_value;
    std::vector<uint8_t> call_data;
    uint64_t gas_limit;
    uint64_t gas_used;
    uint32_t block_number;
    uint64_t block_timestamp;

    ExecutionContext() : call_value(0), gas_limit(0), gas_used(0),
                        block_number(0), block_timestamp(0) {}
};

/**
 * Contract state storage
 */
class ContractStorage {
public:
    ContractStorage();
    ~ContractStorage();

    // Storage operations with safety checks
    bool store(const Hash256& key, const Hash256& value);
    std::optional<Hash256> load(const Hash256& key) const;
    bool remove(const Hash256& key);

    // Bulk operations
    void clear();
    size_t size() const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<ContractStorage> deserialize(const std::vector<uint8_t>& data);

private:
    std::unordered_map<Hash256, Hash256> storage_;
    mutable std::mutex storage_mutex_;

    // Memory limit (prevent DOS)
    static constexpr size_t MAX_STORAGE_SIZE = 1024 * 1024;  // 1MB
};

/**
 * Safe integer arithmetic with overflow detection
 */
class SafeMath {
public:
    // Addition with overflow check
    static std::optional<uint64_t> safe_add(uint64_t a, uint64_t b) {
        if (a > std::numeric_limits<uint64_t>::max() - b) {
            return std::nullopt;  // Overflow
        }
        return a + b;
    }

    // Subtraction with underflow check
    static std::optional<uint64_t> safe_sub(uint64_t a, uint64_t b) {
        if (b > a) {
            return std::nullopt;  // Underflow
        }
        return a - b;
    }

    // Multiplication with overflow check
    static std::optional<uint64_t> safe_mul(uint64_t a, uint64_t b) {
        if (a == 0 || b == 0) return 0;
        if (a > std::numeric_limits<uint64_t>::max() / b) {
            return std::nullopt;  // Overflow
        }
        return a * b;
    }

    // Division with divide-by-zero check
    static std::optional<uint64_t> safe_div(uint64_t a, uint64_t b) {
        if (b == 0) {
            return std::nullopt;  // Divide by zero
        }
        return a / b;
    }

    // Modulo with divide-by-zero check
    static std::optional<uint64_t> safe_mod(uint64_t a, uint64_t b) {
        if (b == 0) {
            return std::nullopt;  // Divide by zero
        }
        return a % b;
    }
};

/**
 * Virtual Machine for executing smart contracts
 *
 * Security features:
 * - Gas metering to prevent infinite loops
 * - Stack depth limits to prevent overflow
 * - Memory limits to prevent DOS
 * - Integer overflow protection
 * - Input validation on all operations
 */
class VirtualMachine {
public:
    VirtualMachine();
    ~VirtualMachine();

    // Execute contract bytecode
    ExecutionResult execute(const std::vector<uint8_t>& bytecode,
                           ExecutionContext& context,
                           ContractStorage& storage,
                           std::vector<uint8_t>& return_data);

    // Configuration
    void set_gas_limit(uint64_t limit) { gas_limit_ = limit; }
    void set_max_stack_size(size_t size) { max_stack_size_ = size; }
    void set_max_memory_size(size_t size) { max_memory_size_ = size; }

    // Get execution stats
    uint64_t get_gas_used() const { return gas_used_; }
    size_t get_max_stack_depth() const { return max_stack_depth_reached_; }

private:
    // Stack operations with bounds checking
    bool push(uint64_t value);
    std::optional<uint64_t> pop();
    std::optional<uint64_t> peek(size_t depth = 0) const;

    // Memory operations with bounds checking
    bool write_memory(size_t offset, const std::vector<uint8_t>& data);
    std::optional<std::vector<uint8_t>> read_memory(size_t offset, size_t length) const;

    // Opcode execution with input validation
    ExecutionResult execute_opcode(OpCode op, ExecutionContext& context,
                                   ContractStorage& storage);

    // Arithmetic operations with overflow protection
    ExecutionResult op_add();
    ExecutionResult op_sub();
    ExecutionResult op_mul();
    ExecutionResult op_div();
    ExecutionResult op_mod();

    // Comparison operations
    ExecutionResult op_lt();
    ExecutionResult op_gt();
    ExecutionResult op_eq();

    // Logic operations
    ExecutionResult op_and();
    ExecutionResult op_or();
    ExecutionResult op_not();

    // Stack operations
    ExecutionResult op_push(const std::vector<uint8_t>& bytecode, size_t& pc);
    ExecutionResult op_pop();
    ExecutionResult op_dup();
    ExecutionResult op_swap();

    // Storage operations with validation
    ExecutionResult op_sload(ContractStorage& storage);
    ExecutionResult op_sstore(ContractStorage& storage);

    // Control flow
    ExecutionResult op_jump();
    ExecutionResult op_jumpi();
    bool is_valid_jump_dest(size_t dest) const;

    // Gas metering
    bool consume_gas(uint64_t amount);
    uint64_t calculate_gas_cost(OpCode op) const;

    // Validation
    bool validate_bytecode(const std::vector<uint8_t>& bytecode) const;
    bool validate_stack_depth() const;
    bool validate_memory_access(size_t offset, size_t length) const;

    // State
    std::stack<uint64_t> stack_;
    std::vector<uint8_t> memory_;
    std::vector<uint8_t> return_data_;
    std::unordered_set<size_t> valid_jump_destinations_;

    // Limits and counters
    uint64_t gas_limit_;
    uint64_t gas_used_;
    size_t max_stack_size_;
    size_t max_memory_size_;
    size_t max_stack_depth_reached_;

    // Constants
    static constexpr size_t DEFAULT_MAX_STACK_SIZE = 1024;
    static constexpr size_t DEFAULT_MAX_MEMORY_SIZE = 1024 * 1024;  // 1MB
    static constexpr uint64_t BASE_GAS_COST = 3;
};

/**
 * Gas price calculator
 */
class GasCalculator {
public:
    // Opcode gas costs
    static uint64_t get_base_cost(OpCode op);
    static uint64_t get_storage_cost(bool is_store, bool is_new_slot);
    static uint64_t get_memory_cost(size_t memory_size);
    static uint64_t get_call_cost(uint64_t value_transfer);

    // Transaction gas costs
    static uint64_t calculate_tx_gas(const Transaction& tx);
    static uint64_t calculate_contract_creation_gas(size_t code_size);

    // Gas price estimation
    static uint64_t estimate_gas_price();

private:
    // Gas schedule
    static constexpr uint64_t GAS_ZERO = 0;
    static constexpr uint64_t GAS_BASE = 2;
    static constexpr uint64_t GAS_VERYLOW = 3;
    static constexpr uint64_t GAS_LOW = 5;
    static constexpr uint64_t GAS_MID = 8;
    static constexpr uint64_t GAS_HIGH = 10;
    static constexpr uint64_t GAS_STORAGE_SET = 20000;
    static constexpr uint64_t GAS_STORAGE_CLEAR = 5000;
    static constexpr uint64_t GAS_CALL = 700;
    static constexpr uint64_t GAS_CREATE = 32000;
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_VM_H
