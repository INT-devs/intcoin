// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// INTcoin Smart Contract Virtual Machine
// EVM-compatible with quantum-resistant extensions

#ifndef INTCOIN_CONTRACTS_VM_H
#define INTCOIN_CONTRACTS_VM_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <array>
#include <unordered_map>
#include <functional>

namespace intcoin {
namespace contracts {

// ============================================================================
// Bytecode Specification
// ============================================================================

/**
 * INTcoin VM Opcodes
 * EVM-compatible (0x00-0xFF) with quantum extensions (0x100-0x1FF)
 */
enum class Opcode : uint16_t {
    // Stop and Arithmetic (0x00-0x0F)
    STOP = 0x00,
    ADD = 0x01,
    MUL = 0x02,
    SUB = 0x03,
    DIV = 0x04,
    SDIV = 0x05,
    MOD = 0x06,
    SMOD = 0x07,
    ADDMOD = 0x08,
    MULMOD = 0x09,
    EXP = 0x0A,
    SIGNEXTEND = 0x0B,

    // Comparison & Bitwise (0x10-0x1F)
    LT = 0x10,
    GT = 0x11,
    SLT = 0x12,
    SGT = 0x13,
    EQ = 0x14,
    ISZERO = 0x15,
    AND = 0x16,
    OR = 0x17,
    XOR = 0x18,
    NOT = 0x19,
    BYTE = 0x1A,
    SHL = 0x1B,
    SHR = 0x1C,
    SAR = 0x1D,

    // SHA3 (0x20)
    SHA3 = 0x20,

    // Environmental Information (0x30-0x3F)
    ADDRESS = 0x30,
    BALANCE = 0x31,
    ORIGIN = 0x32,
    CALLER = 0x33,
    CALLVALUE = 0x34,
    CALLDATALOAD = 0x35,
    CALLDATASIZE = 0x36,
    CALLDATACOPY = 0x37,
    CODESIZE = 0x38,
    CODECOPY = 0x39,
    GASPRICE = 0x3A,
    EXTCODESIZE = 0x3B,
    EXTCODECOPY = 0x3C,
    RETURNDATASIZE = 0x3D,
    RETURNDATACOPY = 0x3E,
    EXTCODEHASH = 0x3F,

    // Block Information (0x40-0x4F)
    BLOCKHASH = 0x40,
    COINBASE = 0x41,
    TIMESTAMP = 0x42,
    NUMBER = 0x43,
    DIFFICULTY = 0x44,
    GASLIMIT = 0x45,
    CHAINID = 0x46,
    SELFBALANCE = 0x47,
    BASEFEE = 0x48,

    // Stack, Memory, Storage (0x50-0x5F)
    POP = 0x50,
    MLOAD = 0x51,
    MSTORE = 0x52,
    MSTORE8 = 0x53,
    SLOAD = 0x54,
    SSTORE = 0x55,
    JUMP = 0x56,
    JUMPI = 0x57,
    PC = 0x58,
    MSIZE = 0x59,
    GAS = 0x5A,
    JUMPDEST = 0x5B,

    // Push Operations (0x60-0x7F)
    PUSH1 = 0x60,
    PUSH2 = 0x61,
    PUSH3 = 0x62,
    PUSH4 = 0x63,
    PUSH5 = 0x64,
    PUSH6 = 0x65,
    PUSH7 = 0x66,
    PUSH8 = 0x67,
    PUSH9 = 0x68,
    PUSH10 = 0x69,
    PUSH11 = 0x6A,
    PUSH12 = 0x6B,
    PUSH13 = 0x6C,
    PUSH14 = 0x6D,
    PUSH15 = 0x6E,
    PUSH16 = 0x6F,
    PUSH17 = 0x70,
    PUSH18 = 0x71,
    PUSH19 = 0x72,
    PUSH20 = 0x73,
    PUSH21 = 0x74,
    PUSH22 = 0x75,
    PUSH23 = 0x76,
    PUSH24 = 0x77,
    PUSH25 = 0x78,
    PUSH26 = 0x79,
    PUSH27 = 0x7A,
    PUSH28 = 0x7B,
    PUSH29 = 0x7C,
    PUSH30 = 0x7D,
    PUSH31 = 0x7E,
    PUSH32 = 0x7F,

    // Duplication Operations (0x80-0x8F)
    DUP1 = 0x80,
    DUP2 = 0x81,
    DUP3 = 0x82,
    DUP4 = 0x83,
    DUP5 = 0x84,
    DUP6 = 0x85,
    DUP7 = 0x86,
    DUP8 = 0x87,
    DUP9 = 0x88,
    DUP10 = 0x89,
    DUP11 = 0x8A,
    DUP12 = 0x8B,
    DUP13 = 0x8C,
    DUP14 = 0x8D,
    DUP15 = 0x8E,
    DUP16 = 0x8F,

    // Exchange Operations (0x90-0x9F)
    SWAP1 = 0x90,
    SWAP2 = 0x91,
    SWAP3 = 0x92,
    SWAP4 = 0x93,
    SWAP5 = 0x94,
    SWAP6 = 0x95,
    SWAP7 = 0x96,
    SWAP8 = 0x97,
    SWAP9 = 0x98,
    SWAP10 = 0x99,
    SWAP11 = 0x9A,
    SWAP12 = 0x9B,
    SWAP13 = 0x9C,
    SWAP14 = 0x9D,
    SWAP15 = 0x9E,
    SWAP16 = 0x9F,

    // Logging Operations (0xA0-0xA4)
    LOG0 = 0xA0,
    LOG1 = 0xA1,
    LOG2 = 0xA2,
    LOG3 = 0xA3,
    LOG4 = 0xA4,

    // System Operations (0xF0-0xFF)
    CREATE = 0xF0,
    CALL = 0xF1,
    CALLCODE = 0xF2,
    RETURN = 0xF3,
    DELEGATECALL = 0xF4,
    CREATE2 = 0xF5,
    STATICCALL = 0xFA,
    REVERT = 0xFD,
    INVALID = 0xFE,
    SELFDESTRUCT = 0xFF,

    // ========== Quantum-Resistant Extensions (0x100-0x1FF) ==========

    // Dilithium signature operations
    DILITHIUM_VERIFY = 0x100,      // Verify Dilithium signature
    DILITHIUM_RECOVER = 0x101,     // Recover signer from signature

    // Kyber key exchange
    KYBER_ENCAP = 0x110,           // Encapsulate shared secret
    KYBER_DECAP = 0x111,           // Decapsulate shared secret

    // SHA3 variants
    SHA3_256 = 0x120,              // SHA3-256 hash
    SHA3_512 = 0x121,              // SHA3-512 hash
    SHAKE128 = 0x122,              // SHAKE128 XOF
    SHAKE256 = 0x123,              // SHAKE256 XOF

    // Quantum-safe random
    QRANDOM = 0x130,               // Quantum-safe random number
};

// ============================================================================
// Gas Costs
// ============================================================================

/**
 * Gas cost schedule
 */
struct GasSchedule {
    // Base costs
    static constexpr uint64_t ZERO = 0;
    static constexpr uint64_t BASE = 2;
    static constexpr uint64_t VERYLOW = 3;
    static constexpr uint64_t LOW = 5;
    static constexpr uint64_t MID = 8;
    static constexpr uint64_t HIGH = 10;

    // Memory
    static constexpr uint64_t MEMORY = 3;
    static constexpr uint64_t MEMORY_WORD = 3;

    // Storage
    static constexpr uint64_t SLOAD = 800;
    static constexpr uint64_t SSTORE_SET = 20000;
    static constexpr uint64_t SSTORE_RESET = 5000;
    static constexpr uint64_t SSTORE_CLEAR_REFUND = 15000;

    // Calls
    static constexpr uint64_t CALL = 700;
    static constexpr uint64_t CALL_VALUE = 9000;
    static constexpr uint64_t CALL_STIPEND = 2300;
    static constexpr uint64_t NEW_ACCOUNT = 25000;

    // Contract creation
    static constexpr uint64_t CREATE = 32000;
    static constexpr uint64_t CREATE_DATA = 200;

    // Transaction
    static constexpr uint64_t TX_BASE = 21000;
    static constexpr uint64_t TX_DATA_ZERO = 4;
    static constexpr uint64_t TX_DATA_NONZERO = 16;
    static constexpr uint64_t TX_CREATE = 53000;

    // Cryptographic
    static constexpr uint64_t SHA3_BASE = 30;
    static constexpr uint64_t SHA3_WORD = 6;
    static constexpr uint64_t ECRECOVER = 3000;

    // Quantum-resistant (higher due to larger signatures)
    static constexpr uint64_t DILITHIUM_VERIFY_BASE = 15000;
    static constexpr uint64_t KYBER_ENCAP = 10000;
    static constexpr uint64_t KYBER_DECAP = 8000;

    // Precompiles
    static constexpr uint64_t ECADD = 150;
    static constexpr uint64_t ECMUL = 6000;
    static constexpr uint64_t ECPAIRING_BASE = 45000;
    static constexpr uint64_t ECPAIRING_POINT = 34000;

    // Logging
    static constexpr uint64_t LOG_BASE = 375;
    static constexpr uint64_t LOG_TOPIC = 375;
    static constexpr uint64_t LOG_DATA = 8;

    // Other
    static constexpr uint64_t COPY = 3;
    static constexpr uint64_t EXP_BASE = 10;
    static constexpr uint64_t EXP_BYTE = 50;
    static constexpr uint64_t BALANCE = 700;
    static constexpr uint64_t BLOCKHASH = 20;
    static constexpr uint64_t SELFDESTRUCT = 5000;
    static constexpr uint64_t SELFDESTRUCT_NEWACCOUNT = 25000;
};

// ============================================================================
// Data Types
// ============================================================================

using Word = std::array<uint8_t, 32>;  // 256-bit word
using Address = std::array<uint8_t, 20>;  // 160-bit address (compatible)
using Hash256 = std::array<uint8_t, 32>;

/**
 * Execution result status
 */
enum class ExecStatus {
    SUCCESS,
    REVERT,
    OUT_OF_GAS,
    INVALID_OPCODE,
    STACK_UNDERFLOW,
    STACK_OVERFLOW,
    INVALID_JUMP,
    WRITE_PROTECTION,
    CALL_DEPTH_EXCEEDED,
    INSUFFICIENT_BALANCE,
    CONTRACT_SIZE_EXCEEDED,
    INTERNAL_ERROR
};

/**
 * Log entry (event)
 */
struct LogEntry {
    Address address;
    std::vector<Hash256> topics;
    std::vector<uint8_t> data;
};

/**
 * Execution result
 */
struct ExecutionResult {
    ExecStatus status;
    uint64_t gas_used;
    uint64_t gas_refund;
    std::vector<uint8_t> output;
    std::vector<LogEntry> logs;
    std::optional<Address> created_address;
    std::string error_message;

    bool success() const { return status == ExecStatus::SUCCESS; }
};

/**
 * Contract account state
 */
struct ContractState {
    uint64_t nonce;
    Word balance;
    Hash256 code_hash;
    Hash256 storage_root;
    std::vector<uint8_t> code;
    std::unordered_map<Hash256, Word, std::hash<std::string>> storage;
};

/**
 * Message call parameters
 */
struct Message {
    Address sender;
    Address recipient;
    Word value;
    std::vector<uint8_t> data;
    uint64_t gas;
    uint8_t depth;
    bool is_create;
    bool is_static;
};

// ============================================================================
// Virtual Machine
// ============================================================================

/**
 * State interface for VM
 */
class StateInterface {
public:
    virtual ~StateInterface() = default;

    // Account access
    virtual bool account_exists(const Address& addr) const = 0;
    virtual Word get_balance(const Address& addr) const = 0;
    virtual uint64_t get_nonce(const Address& addr) const = 0;
    virtual std::vector<uint8_t> get_code(const Address& addr) const = 0;
    virtual Hash256 get_code_hash(const Address& addr) const = 0;

    // Storage access
    virtual Word get_storage(const Address& addr, const Hash256& key) const = 0;
    virtual void set_storage(const Address& addr, const Hash256& key, const Word& value) = 0;

    // State modification
    virtual void set_balance(const Address& addr, const Word& balance) = 0;
    virtual void set_nonce(const Address& addr, uint64_t nonce) = 0;
    virtual void set_code(const Address& addr, const std::vector<uint8_t>& code) = 0;
    virtual void create_account(const Address& addr) = 0;
    virtual void destruct_account(const Address& addr, const Address& beneficiary) = 0;

    // Block context
    virtual Hash256 get_block_hash(uint64_t number) const = 0;
    virtual Address get_coinbase() const = 0;
    virtual uint64_t get_timestamp() const = 0;
    virtual uint64_t get_block_number() const = 0;
    virtual uint64_t get_difficulty() const = 0;
    virtual uint64_t get_gas_limit() const = 0;
    virtual uint64_t get_chain_id() const = 0;
    virtual uint64_t get_base_fee() const = 0;

    // Snapshots for revert
    virtual size_t snapshot() = 0;
    virtual void revert(size_t snapshot) = 0;
    virtual void commit() = 0;
};

/**
 * INTcoin Virtual Machine
 */
class VM {
public:
    VM();
    ~VM();

    // Execute contract call
    ExecutionResult execute(
        StateInterface& state,
        const Message& msg
    );

    // Execute contract creation
    ExecutionResult create(
        StateInterface& state,
        const Message& msg
    );

    // Deploy contract (convenience)
    ExecutionResult deploy(
        StateInterface& state,
        const Address& sender,
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& constructor_args,
        const Word& value,
        uint64_t gas
    );

    // Call contract (convenience)
    ExecutionResult call(
        StateInterface& state,
        const Address& sender,
        const Address& contract,
        const std::vector<uint8_t>& calldata,
        const Word& value,
        uint64_t gas
    );

    // Static call (read-only)
    ExecutionResult static_call(
        StateInterface& state,
        const Address& sender,
        const Address& contract,
        const std::vector<uint8_t>& calldata,
        uint64_t gas
    );

    // Estimate gas
    uint64_t estimate_gas(
        StateInterface& state,
        const Message& msg
    );

    // Configuration
    void set_max_call_depth(uint32_t depth);
    void set_max_code_size(size_t size);
    void set_gas_schedule(const GasSchedule& schedule);

    // Precompiled contracts
    using PrecompileFunc = std::function<ExecutionResult(
        const std::vector<uint8_t>& input, uint64_t gas)>;
    void register_precompile(const Address& addr, PrecompileFunc func);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Contract ABI
// ============================================================================

/**
 * ABI type definitions
 */
enum class ABIType {
    UINT,      // uint8 to uint256
    INT,       // int8 to int256
    ADDRESS,   // address (20 bytes)
    BOOL,      // bool
    BYTES,     // bytes (dynamic)
    STRING,    // string (dynamic)
    BYTES_N,   // bytes1 to bytes32
    ARRAY,     // T[] (dynamic array)
    TUPLE      // (T1, T2, ...) (struct)
};

/**
 * ABI encoder/decoder
 */
class ABI {
public:
    // Encode function call
    static std::vector<uint8_t> encode_call(
        const std::string& function_signature,
        const std::vector<std::vector<uint8_t>>& args
    );

    // Encode function selector
    static std::array<uint8_t, 4> encode_selector(
        const std::string& function_signature
    );

    // Encode single value
    static std::vector<uint8_t> encode_uint256(const Word& value);
    static std::vector<uint8_t> encode_address(const Address& addr);
    static std::vector<uint8_t> encode_bool(bool value);
    static std::vector<uint8_t> encode_bytes(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encode_string(const std::string& str);

    // Decode return data
    static Word decode_uint256(const std::vector<uint8_t>& data, size_t offset = 0);
    static Address decode_address(const std::vector<uint8_t>& data, size_t offset = 0);
    static bool decode_bool(const std::vector<uint8_t>& data, size_t offset = 0);
    static std::vector<uint8_t> decode_bytes(const std::vector<uint8_t>& data, size_t offset = 0);
    static std::string decode_string(const std::vector<uint8_t>& data, size_t offset = 0);

    // Event topics
    static Hash256 event_signature(const std::string& event_signature);
};

// ============================================================================
// Contract Deployment
// ============================================================================

/**
 * Contract metadata
 */
struct ContractMetadata {
    std::string name;
    std::string version;
    std::string compiler;
    std::string source_hash;
    std::vector<uint8_t> abi_json;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> deployed_bytecode;
};

/**
 * Contract deployer
 */
class ContractDeployer {
public:
    ContractDeployer(VM& vm, StateInterface& state);

    // Deploy from bytecode
    std::pair<Address, ExecutionResult> deploy(
        const Address& sender,
        const std::vector<uint8_t>& bytecode,
        const Word& value = {},
        uint64_t gas = 3000000
    );

    // Deploy with constructor args
    std::pair<Address, ExecutionResult> deploy_with_args(
        const Address& sender,
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& constructor_args,
        const Word& value = {},
        uint64_t gas = 3000000
    );

    // Deploy from metadata
    std::pair<Address, ExecutionResult> deploy_metadata(
        const Address& sender,
        const ContractMetadata& metadata,
        const std::vector<uint8_t>& constructor_args = {},
        const Word& value = {},
        uint64_t gas = 3000000
    );

    // Verify deployed contract
    bool verify(const Address& addr, const ContractMetadata& metadata) const;

    // Calculate CREATE address
    static Address calculate_create_address(const Address& sender, uint64_t nonce);

    // Calculate CREATE2 address
    static Address calculate_create2_address(
        const Address& sender,
        const Hash256& salt,
        const Hash256& init_code_hash
    );

private:
    VM& vm_;
    StateInterface& state_;
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_VM_H
