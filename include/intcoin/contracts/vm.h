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
    SHA3_256 = 0x71,
    DILITHIUM_VERIFY = 0x72,
    DILITHIUM_SIGN = 0x73,
    KYBER_ENCAPS = 0x74,           // Kyber key encapsulation
    KYBER_DECAPS = 0x75,           // Kyber decapsulation
    SPHINCS_VERIFY = 0x76,         // SPHINCS+ signature verification
    SPHINCS_SIGN = 0x77,           // SPHINCS+ signature creation
    HASH160 = 0x78,                // RIPEMD160(SHA256(x))
    HASH256 = 0x79,                // SHA256(SHA256(x))

    // Call
    CALL = 0x80,
    DELEGATECALL = 0x81,
    STATICCALL = 0x82,

    // Create
    CREATE = 0x90,
    CREATE2 = 0x91,                // Deterministic contract creation

    // Quantum-resistant extensions (0x100-0x1FF range)
    // Time-locks (CLTV/CSV style)
    CHECKLOCKTIMEVERIFY = 0x100,   // Absolute time lock
    CHECKSEQUENCEVERIFY = 0x101,   // Relative time lock

    // Multi-signature operations
    CHECKMULTISIG = 0x110,         // M-of-N multi-sig verification
    CHECKMULTISIGVERIFY = 0x111,   // Multi-sig with verification

    // State channel operations
    CHANNEL_OPEN = 0x120,          // Open payment channel
    CHANNEL_UPDATE = 0x121,        // Update channel state
    CHANNEL_CLOSE = 0x122,         // Close payment channel
    CHANNEL_SETTLE = 0x123,        // Settle channel funds

    // Cross-chain operations
    ATOMIC_SWAP_LOCK = 0x130,      // Lock funds for atomic swap
    ATOMIC_SWAP_CLAIM = 0x131,     // Claim swapped funds
    ATOMIC_SWAP_REFUND = 0x132,    // Refund locked funds
    VERIFY_SPV_PROOF = 0x133,      // Verify SPV proof

    // Advanced crypto operations
    MERKLE_PROOF_VERIFY = 0x140,   // Verify Merkle proof
    SCHNORR_VERIFY = 0x141,        // Schnorr signature verification
    BLS_VERIFY = 0x142,            // BLS signature verification
    BLS_AGGREGATE = 0x143,         // BLS signature aggregation

    // Zero-knowledge proofs
    ZK_VERIFY = 0x150,             // Generic ZK proof verification
    ZK_RANGE_PROOF = 0x151,        // Range proof (value in range)
    ZK_MEMBERSHIP_PROOF = 0x152,   // Membership proof

    // Advanced state operations
    TRANSIENT_STORE = 0x160,       // Transient storage (cleared after tx)
    TRANSIENT_LOAD = 0x161,        // Load transient storage
    SELFDESTRUCT = 0x162,          // Destroy contract

    // Gas optimizations
    MCOPY = 0x170,                 // Memory copy
    PUSH0 = 0x171,                 // Push 0 (gas optimized)

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

    // Quantum-resistant operation costs
    static uint64_t get_dilithium_verify_cost();
    static uint64_t get_kyber_encaps_cost();
    static uint64_t get_sphincs_verify_cost();
    static uint64_t get_zk_proof_cost(size_t proof_size);

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

    // Quantum-resistant gas costs (higher due to complexity)
    static constexpr uint64_t GAS_DILITHIUM_VERIFY = 5000;
    static constexpr uint64_t GAS_KYBER_ENCAPS = 3000;
    static constexpr uint64_t GAS_SPHINCS_VERIFY = 8000;
    static constexpr uint64_t GAS_ZK_VERIFY_BASE = 10000;
    static constexpr uint64_t GAS_MULTISIG = 1000;
    static constexpr uint64_t GAS_MERKLE_VERIFY = 500;
};

/**
 * Quantum cryptography operations
 */
class QuantumCrypto {
public:
    /**
     * Dilithium signature operations
     */
    struct DilithiumOp {
        static bool verify(
            const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& public_key
        );

        static std::optional<std::vector<uint8_t>> sign(
            const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& private_key
        );
    };

    /**
     * Kyber key encapsulation operations
     */
    struct KyberOp {
        static std::optional<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> encapsulate(
            const std::vector<uint8_t>& public_key
        );  // Returns (ciphertext, shared_secret)

        static std::optional<std::vector<uint8_t>> decapsulate(
            const std::vector<uint8_t>& ciphertext,
            const std::vector<uint8_t>& private_key
        );  // Returns shared_secret
    };

    /**
     * SPHINCS+ signature operations
     */
    struct SphincsOp {
        static bool verify(
            const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& public_key
        );
    };
};

/**
 * Time-lock operations
 */
class TimeLockOps {
public:
    /**
     * Check absolute time lock (CLTV)
     */
    static bool check_lock_time_verify(
        uint64_t lock_time,
        uint64_t current_time,
        uint64_t tx_lock_time
    );

    /**
     * Check relative time lock (CSV)
     */
    static bool check_sequence_verify(
        uint32_t sequence,
        uint32_t tx_sequence,
        uint32_t current_height,
        uint32_t tx_height
    );
};

/**
 * Multi-signature operations
 */
class MultiSigOps {
public:
    /**
     * Verify M-of-N multi-signature
     */
    static bool check_multisig(
        const std::vector<uint8_t>& message,
        const std::vector<std::vector<uint8_t>>& signatures,
        const std::vector<std::vector<uint8_t>>& public_keys,
        uint32_t required_sigs
    );
};

/**
 * State channel operations
 */
class StateChannelOps {
public:
    struct ChannelState {
        Hash256 channel_id;
        Hash256 party_a;
        Hash256 party_b;
        uint64_t balance_a;
        uint64_t balance_b;
        uint32_t update_number;
        uint64_t challenge_period;
    };

    static std::optional<Hash256> open_channel(
        const Hash256& party_a,
        const Hash256& party_b,
        uint64_t balance_a,
        uint64_t balance_b
    );

    static bool update_channel(
        const Hash256& channel_id,
        uint32_t update_number,
        uint64_t new_balance_a,
        uint64_t new_balance_b
    );

    static bool close_channel(const Hash256& channel_id);
    static bool settle_channel(const Hash256& channel_id);
};

/**
 * Atomic swap operations
 */
class AtomicSwapOps {
public:
    struct SwapLock {
        Hash256 hash_lock;
        Hash256 secret;
        uint64_t amount;
        uint64_t time_lock;
        Hash256 recipient;
    };

    static std::optional<Hash256> lock_swap(
        const Hash256& hash_lock,
        uint64_t amount,
        uint64_t time_lock,
        const Hash256& recipient
    );

    static bool claim_swap(
        const Hash256& swap_id,
        const Hash256& secret
    );

    static bool refund_swap(const Hash256& swap_id);
};

/**
 * Merkle proof operations
 */
class MerkleProofOps {
public:
    static bool verify_merkle_proof(
        const Hash256& leaf,
        const Hash256& root,
        const std::vector<Hash256>& proof,
        const std::vector<bool>& path
    );
};

/**
 * Zero-knowledge proof operations
 */
class ZKProofOps {
public:
    enum class ProofType {
        GENERIC,
        RANGE,          // Prove value is in range [min, max]
        MEMBERSHIP,     // Prove value is in set
        EQUALITY,       // Prove two values are equal (without revealing)
        KNOWLEDGE       // Prove knowledge of value (without revealing)
    };

    static bool verify_zk_proof(
        ProofType type,
        const std::vector<uint8_t>& proof,
        const std::vector<uint8_t>& public_inputs
    );

    static bool verify_range_proof(
        const std::vector<uint8_t>& proof,
        uint64_t min_value,
        uint64_t max_value
    );

    static bool verify_membership_proof(
        const std::vector<uint8_t>& proof,
        const Hash256& commitment,
        const std::vector<Hash256>& set
    );
};

/**
 * Transient storage (EIP-1153 style)
 * Storage that is cleared after transaction execution
 */
class TransientStorage {
public:
    TransientStorage();
    ~TransientStorage();

    void store(const Hash256& key, const Hash256& value);
    std::optional<Hash256> load(const Hash256& key) const;
    void clear();

private:
    std::unordered_map<Hash256, Hash256> storage_;
    mutable std::mutex storage_mutex_;
};

/**
 * Contract opcode info and gas costs
 */
struct OpCodeInfo {
    OpCode opcode;
    std::string name;
    uint64_t base_gas_cost;
    bool is_quantum_resistant;
    bool is_experimental;

    OpCodeInfo(OpCode op, const std::string& n, uint64_t gas, bool qr = false, bool exp = false)
        : opcode(op), name(n), base_gas_cost(gas), is_quantum_resistant(qr), is_experimental(exp) {}
};

/**
 * Get opcode information
 */
inline std::optional<OpCodeInfo> get_opcode_info(OpCode op) {
    static const std::unordered_map<OpCode, OpCodeInfo> opcode_map = {
        // Arithmetic
        {OpCode::ADD, OpCodeInfo(OpCode::ADD, "ADD", 3)},
        {OpCode::SUB, OpCodeInfo(OpCode::SUB, "SUB", 3)},
        {OpCode::MUL, OpCodeInfo(OpCode::MUL, "MUL", 5)},
        {OpCode::DIV, OpCodeInfo(OpCode::DIV, "DIV", 5)},

        // Quantum-resistant crypto
        {OpCode::DILITHIUM_VERIFY, OpCodeInfo(OpCode::DILITHIUM_VERIFY, "DILITHIUM_VERIFY", 5000, true)},
        {OpCode::KYBER_ENCAPS, OpCodeInfo(OpCode::KYBER_ENCAPS, "KYBER_ENCAPS", 3000, true)},
        {OpCode::SPHINCS_VERIFY, OpCodeInfo(OpCode::SPHINCS_VERIFY, "SPHINCS_VERIFY", 8000, true)},

        // Time-locks
        {OpCode::CHECKLOCKTIMEVERIFY, OpCodeInfo(OpCode::CHECKLOCKTIMEVERIFY, "CHECKLOCKTIMEVERIFY", 100)},
        {OpCode::CHECKSEQUENCEVERIFY, OpCodeInfo(OpCode::CHECKSEQUENCEVERIFY, "CHECKSEQUENCEVERIFY", 100)},

        // Multi-sig
        {OpCode::CHECKMULTISIG, OpCodeInfo(OpCode::CHECKMULTISIG, "CHECKMULTISIG", 1000)},

        // State channels
        {OpCode::CHANNEL_OPEN, OpCodeInfo(OpCode::CHANNEL_OPEN, "CHANNEL_OPEN", 10000)},
        {OpCode::CHANNEL_UPDATE, OpCodeInfo(OpCode::CHANNEL_UPDATE, "CHANNEL_UPDATE", 5000)},

        // Atomic swaps
        {OpCode::ATOMIC_SWAP_LOCK, OpCodeInfo(OpCode::ATOMIC_SWAP_LOCK, "ATOMIC_SWAP_LOCK", 8000)},
        {OpCode::ATOMIC_SWAP_CLAIM, OpCodeInfo(OpCode::ATOMIC_SWAP_CLAIM, "ATOMIC_SWAP_CLAIM", 5000)},

        // Zero-knowledge proofs
        {OpCode::ZK_VERIFY, OpCodeInfo(OpCode::ZK_VERIFY, "ZK_VERIFY", 10000, false, true)},
        {OpCode::ZK_RANGE_PROOF, OpCodeInfo(OpCode::ZK_RANGE_PROOF, "ZK_RANGE_PROOF", 15000, false, true)},
    };

    auto it = opcode_map.find(op);
    if (it != opcode_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_VM_H
