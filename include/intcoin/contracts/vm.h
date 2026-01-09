// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_VM_H
#define INTCOIN_CONTRACTS_VM_H

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <map>
#include <functional>

namespace intcoin {
namespace contracts {

// 256-bit word (32 bytes)
using Word256 = std::array<uint8_t, 32>;

/**
 * IntSC Opcodes
 *
 * EVM-compatible opcodes with post-quantum extensions
 */
enum class Opcode : uint8_t {
    // Arithmetic
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
    EXP = 0x0a,
    SIGNEXTEND = 0x0b,

    // Comparison
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
    BYTE = 0x1a,
    SHL = 0x1b,
    SHR = 0x1c,
    SAR = 0x1d,

    // Hash
    SHA3 = 0x20,

    // Environment
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
    GASPRICE = 0x3a,
    EXTCODESIZE = 0x3b,
    EXTCODECOPY = 0x3c,
    RETURNDATASIZE = 0x3d,
    RETURNDATACOPY = 0x3e,
    EXTCODEHASH = 0x3f,

    // Block
    BLOCKHASH = 0x40,
    COINBASE = 0x41,
    TIMESTAMP = 0x42,
    NUMBER = 0x43,
    DIFFICULTY = 0x44,
    GASLIMIT = 0x45,
    CHAINID = 0x46,
    SELFBALANCE = 0x47,
    BASEFEE = 0x48,

    // Stack
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
    GAS = 0x5a,
    JUMPDEST = 0x5b,

    // Push (0x60-0x7f)
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
    PUSH11 = 0x6a,
    PUSH12 = 0x6b,
    PUSH13 = 0x6c,
    PUSH14 = 0x6d,
    PUSH15 = 0x6e,
    PUSH16 = 0x6f,
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
    PUSH27 = 0x7a,
    PUSH28 = 0x7b,
    PUSH29 = 0x7c,
    PUSH30 = 0x7d,
    PUSH31 = 0x7e,
    PUSH32 = 0x7f,

    // Dup (0x80-0x8f)
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
    DUP11 = 0x8a,
    DUP12 = 0x8b,
    DUP13 = 0x8c,
    DUP14 = 0x8d,
    DUP15 = 0x8e,
    DUP16 = 0x8f,

    // Swap (0x90-0x9f)
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
    SWAP11 = 0x9a,
    SWAP12 = 0x9b,
    SWAP13 = 0x9c,
    SWAP14 = 0x9d,
    SWAP15 = 0x9e,
    SWAP16 = 0x9f,

    // Post-Quantum Cryptography (0xa0-0xa3)
    DILITHIUM_VERIFY = 0xa0,
    KYBER_ENCAP = 0xa1,
    KYBER_DECAP = 0xa2,
    PQC_PUBKEY = 0xa3,

    // Log (0xa0-0xa4 in EVM, shifted for us)
    LOG0 = 0xa4,
    LOG1 = 0xa5,
    LOG2 = 0xa6,
    LOG3 = 0xa7,
    LOG4 = 0xa8,

    // Contract operations
    CREATE = 0xf0,
    CALL = 0xf1,
    CALLCODE = 0xf2,
    RETURN = 0xf3,
    DELEGATECALL = 0xf4,
    CREATE2 = 0xf5,
    STATICCALL = 0xfa,
    REVERT = 0xfd,
    INVALID = 0xfe,
    SELFDESTRUCT = 0xff
};

/**
 * VM execution result
 */
enum class ExecutionResult {
    SUCCESS,
    OUT_OF_GAS,
    STACK_OVERFLOW,
    STACK_UNDERFLOW,
    INVALID_OPCODE,
    INVALID_JUMP,
    REVERT,
    CONTRACT_CREATION_FAILED,
    CALL_FAILED,
    INTERNAL_ERROR
};

/**
 * VM execution mode
 */
enum class ExecutionMode {
    NORMAL,
    STATIC,      // No state modifications (STATICCALL)
    DELEGATE     // DELEGATECALL context
};

/**
 * Account state
 */
struct Account {
    uint64_t balance{0};           // Balance in satINT
    uint64_t nonce{0};             // Transaction counter
    Word256 code_hash{};           // Hash of contract code
    Word256 storage_root{};        // Merkle root of storage
};

/**
 * Execution context
 */
struct ExecutionContext {
    std::string caller;            // msg.sender
    std::string origin;            // tx.origin
    std::string address;           // Contract address being executed
    uint64_t value{0};             // msg.value
    std::vector<uint8_t> calldata; // msg.data
    uint64_t gas_limit{0};         // Gas limit
    uint64_t gas_price{0};         // Gas price
    uint64_t block_number{0};      // Current block number
    uint64_t block_timestamp{0};   // Current block timestamp
    std::string block_coinbase;    // Miner address
    uint64_t block_difficulty{0};  // Block difficulty
    uint64_t block_gas_limit{0};   // Block gas limit
    uint64_t chain_id{1337};       // Chain ID (1337 for INTcoin)
    ExecutionMode mode{ExecutionMode::NORMAL};
};

/**
 * Storage interface
 *
 * Provides persistent key-value storage for contracts
 */
class IStorage {
public:
    virtual ~IStorage() = default;

    /**
     * Load value from storage
     *
     * @param address Contract address
     * @param key Storage key
     * @return Storage value
     */
    virtual Word256 Load(const std::string& address, const Word256& key) = 0;

    /**
     * Store value to storage
     *
     * @param address Contract address
     * @param key Storage key
     * @param value Storage value
     */
    virtual void Store(const std::string& address, const Word256& key, const Word256& value) = 0;

    /**
     * Check if key exists
     *
     * @param address Contract address
     * @param key Storage key
     * @return True if key exists
     */
    virtual bool Exists(const std::string& address, const Word256& key) = 0;
};

/**
 * VM Stack
 *
 * Stack-based execution with 256-bit words, max depth 1024
 */
class Stack {
public:
    static constexpr size_t MAX_DEPTH = 1024;

    Stack() = default;

    /**
     * Push word onto stack
     *
     * @param word 256-bit word
     * @return True if successful
     */
    bool Push(const Word256& word);

    /**
     * Pop word from stack
     *
     * @return 256-bit word
     */
    Word256 Pop();

    /**
     * Peek at top of stack
     *
     * @param depth Depth from top (0 = top)
     * @return 256-bit word
     */
    Word256 Peek(size_t depth = 0) const;

    /**
     * Duplicate stack item
     *
     * @param depth Depth to duplicate (1-16)
     * @return True if successful
     */
    bool Dup(size_t depth);

    /**
     * Swap stack items
     *
     * @param depth Depth to swap with top (1-16)
     * @return True if successful
     */
    bool Swap(size_t depth);

    /**
     * Get stack size
     */
    size_t Size() const { return stack_.size(); }

    /**
     * Check if stack is empty
     */
    bool Empty() const { return stack_.empty(); }

    /**
     * Clear stack
     */
    void Clear() { stack_.clear(); }

private:
    std::vector<Word256> stack_;
};

/**
 * VM Memory
 *
 * Byte-addressable linear memory, expandable
 */
class Memory {
public:
    Memory() = default;

    /**
     * Read byte from memory
     *
     * @param offset Memory offset
     * @return Byte value
     */
    uint8_t ReadByte(size_t offset);

    /**
     * Write byte to memory
     *
     * @param offset Memory offset
     * @param value Byte value
     */
    void WriteByte(size_t offset, uint8_t value);

    /**
     * Read word from memory
     *
     * @param offset Memory offset
     * @return 256-bit word
     */
    Word256 ReadWord(size_t offset);

    /**
     * Write word to memory
     *
     * @param offset Memory offset
     * @param word 256-bit word
     */
    void WriteWord(size_t offset, const Word256& word);

    /**
     * Read bytes from memory
     *
     * @param offset Memory offset
     * @param length Number of bytes
     * @return Byte vector
     */
    std::vector<uint8_t> ReadBytes(size_t offset, size_t length);

    /**
     * Write bytes to memory
     *
     * @param offset Memory offset
     * @param data Byte data
     */
    void WriteBytes(size_t offset, const std::vector<uint8_t>& data);

    /**
     * Get memory size
     */
    size_t Size() const { return data_.size(); }

    /**
     * Clear memory
     */
    void Clear() { data_.clear(); }

private:
    /**
     * Expand memory if needed
     *
     * @param offset Required offset
     */
    void Expand(size_t offset);

    std::vector<uint8_t> data_;
};

/**
 * IntSC Virtual Machine
 *
 * Stack-based virtual machine for smart contract execution
 * with EVM compatibility and post-quantum cryptography support
 */
class IntSCVM {
public:
    struct Config {
        uint64_t max_gas{10000000};         // Maximum gas per execution
        uint64_t max_call_depth{1024};      // Maximum call stack depth
        bool enable_pqc_opcodes{true};      // Enable PQC opcodes
        bool strict_mode{true};             // Strict EVM compatibility
    };

    IntSCVM();
    explicit IntSCVM(const Config& config);
    explicit IntSCVM(std::shared_ptr<IStorage> storage);
    IntSCVM(const Config& config, std::shared_ptr<IStorage> storage);
    ~IntSCVM();

    /**
     * Execute bytecode
     *
     * @param bytecode Contract bytecode
     * @param context Execution context
     * @return Execution result
     */
    ExecutionResult Execute(
        const std::vector<uint8_t>& bytecode,
        const ExecutionContext& context
    );

    /**
     * Execute bytecode with output
     *
     * @param bytecode Contract bytecode
     * @param context Execution context
     * @param output [out] Return data
     * @return Execution result
     */
    ExecutionResult Execute(
        const std::vector<uint8_t>& bytecode,
        const ExecutionContext& context,
        std::vector<uint8_t>& output
    );

    /**
     * Get gas used
     */
    uint64_t GetGasUsed() const;

    /**
     * Get gas remaining
     */
    uint64_t GetGasRemaining() const;

    /**
     * Get return data
     */
    std::vector<uint8_t> GetReturnData() const;

    /**
     * Get logs
     */
    struct Log {
        std::string address;
        std::vector<Word256> topics;
        std::vector<uint8_t> data;
    };

    std::vector<Log> GetLogs() const;

    /**
     * Set storage backend
     *
     * @param storage Storage implementation
     */
    void SetStorage(std::shared_ptr<IStorage> storage);

    /**
     * Get configuration
     */
    Config GetConfig() const;

    /**
     * Set configuration
     *
     * @param config VM configuration
     */
    void SetConfig(const Config& config);

    /**
     * Reset VM state
     */
    void Reset();

    /**
     * Get opcode name
     *
     * @param opcode Opcode
     * @return Opcode name
     */
    static std::string GetOpcodeName(Opcode opcode);

    /**
     * Get opcode gas cost
     *
     * @param opcode Opcode
     * @return Gas cost
     */
    static uint64_t GetOpcodeGasCost(Opcode opcode);

    /**
     * Disassemble bytecode
     *
     * @param bytecode Contract bytecode
     * @return Human-readable assembly
     */
    static std::string Disassemble(const std::vector<uint8_t>& bytecode);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Helper functions
 */

/**
 * Convert Word256 to uint64_t
 */
uint64_t Word256ToUint64(const Word256& word);

/**
 * Convert uint64_t to Word256
 */
Word256 Uint64ToWord256(uint64_t value);

/**
 * Convert string to Word256
 */
Word256 StringToWord256(const std::string& str);

/**
 * Convert Word256 to hex string
 */
std::string Word256ToHex(const Word256& word);

/**
 * Convert hex string to Word256
 */
Word256 HexToWord256(const std::string& hex);

/**
 * Check if Word256 is zero
 */
bool IsZeroWord(const Word256& word);

/**
 * Compare Word256 values
 */
int CompareWord256(const Word256& a, const Word256& b);

/**
 * Add Word256 values
 */
Word256 AddWord256(const Word256& a, const Word256& b);

/**
 * Subtract Word256 values
 */
Word256 SubWord256(const Word256& a, const Word256& b);

/**
 * Multiply Word256 values
 */
Word256 MulWord256(const Word256& a, const Word256& b);

/**
 * Divide Word256 values
 */
Word256 DivWord256(const Word256& a, const Word256& b);

/**
 * Modulo Word256 values
 */
Word256 ModWord256(const Word256& a, const Word256& b);

/**
 * AND Word256 values
 */
Word256 AndWord256(const Word256& a, const Word256& b);

/**
 * OR Word256 values
 */
Word256 OrWord256(const Word256& a, const Word256& b);

/**
 * XOR Word256 values
 */
Word256 XorWord256(const Word256& a, const Word256& b);

/**
 * NOT Word256 value
 */
Word256 NotWord256(const Word256& a);

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_VM_H
