// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/contracts/vm.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace intcoin {
namespace contracts {

//
// Stack Implementation
//

bool Stack::Push(const Word256& word) {
    if (stack_.size() >= MAX_DEPTH) {
        return false;
    }
    stack_.push_back(word);
    return true;
}

Word256 Stack::Pop() {
    if (stack_.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    Word256 word = stack_.back();
    stack_.pop_back();
    return word;
}

Word256 Stack::Peek(size_t depth) const {
    if (depth >= stack_.size()) {
        throw std::runtime_error("Stack underflow");
    }
    return stack_[stack_.size() - 1 - depth];
}

bool Stack::Dup(size_t depth) {
    if (depth == 0 || depth > 16 || depth > stack_.size()) {
        return false;
    }
    if (stack_.size() >= MAX_DEPTH) {
        return false;
    }
    Word256 word = stack_[stack_.size() - depth];
    stack_.push_back(word);
    return true;
}

bool Stack::Swap(size_t depth) {
    if (depth == 0 || depth > 16 || depth >= stack_.size()) {
        return false;
    }
    size_t top = stack_.size() - 1;
    size_t swap_idx = top - depth;
    std::swap(stack_[top], stack_[swap_idx]);
    return true;
}

//
// Memory Implementation
//

void Memory::Expand(size_t offset) {
    if (offset >= data_.size()) {
        data_.resize(offset + 1, 0);
    }
}

uint8_t Memory::ReadByte(size_t offset) {
    if (offset >= data_.size()) {
        return 0;
    }
    return data_[offset];
}

void Memory::WriteByte(size_t offset, uint8_t value) {
    Expand(offset);
    data_[offset] = value;
}

Word256 Memory::ReadWord(size_t offset) {
    Word256 word{};
    for (size_t i = 0; i < 32; ++i) {
        word[i] = ReadByte(offset + i);
    }
    return word;
}

void Memory::WriteWord(size_t offset, const Word256& word) {
    for (size_t i = 0; i < 32; ++i) {
        WriteByte(offset + i, word[i]);
    }
}

std::vector<uint8_t> Memory::ReadBytes(size_t offset, size_t length) {
    std::vector<uint8_t> result(length);
    for (size_t i = 0; i < length; ++i) {
        result[i] = ReadByte(offset + i);
    }
    return result;
}

void Memory::WriteBytes(size_t offset, const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        WriteByte(offset + i, data[i]);
    }
}

//
// IntSCVM Implementation
//

class IntSCVM::Impl {
public:
    Config config_;
    std::shared_ptr<IStorage> storage_;

    // Execution state
    Stack stack_;
    Memory memory_;
    uint64_t gas_used_{0};
    uint64_t gas_remaining_{0};
    uint64_t pc_{0};  // Program counter
    std::vector<uint8_t> return_data_;
    std::vector<Log> logs_;
    std::vector<uint8_t> bytecode_;
    ExecutionContext context_;

    Impl() : config_(), storage_(nullptr) {}

    explicit Impl(const Config& config) : config_(config), storage_(nullptr) {}

    explicit Impl(std::shared_ptr<IStorage> storage)
        : config_(), storage_(std::move(storage)) {}

    Impl(const Config& config, std::shared_ptr<IStorage> storage)
        : config_(config), storage_(std::move(storage)) {}

    void Reset() {
        stack_.Clear();
        memory_.Clear();
        gas_used_ = 0;
        gas_remaining_ = 0;
        pc_ = 0;
        return_data_.clear();
        logs_.clear();
        bytecode_.clear();
    }

    ExecutionResult Execute(
        const std::vector<uint8_t>& bytecode,
        const ExecutionContext& context,
        std::vector<uint8_t>& output
    ) {
        Reset();
        bytecode_ = bytecode;
        context_ = context;
        gas_remaining_ = context.gas_limit;

        if (bytecode_.empty()) {
            return ExecutionResult::SUCCESS;
        }

        // Execute opcodes
        while (pc_ < bytecode_.size()) {
            // Check gas
            if (gas_remaining_ == 0) {
                return ExecutionResult::OUT_OF_GAS;
            }

            // Fetch opcode
            Opcode opcode = static_cast<Opcode>(bytecode_[pc_]);

            // Execute opcode
            ExecutionResult result = ExecuteOpcode(opcode);
            if (result != ExecutionResult::SUCCESS) {
                return result;
            }

            // Advance PC (unless jump instruction modified it)
            if (opcode != Opcode::JUMP && opcode != Opcode::JUMPI) {
                pc_++;
            }

            // Check for STOP or RETURN
            if (opcode == Opcode::STOP || opcode == Opcode::RETURN) {
                output = return_data_;
                return ExecutionResult::SUCCESS;
            }

            // Check for REVERT
            if (opcode == Opcode::REVERT) {
                output = return_data_;
                return ExecutionResult::REVERT;
            }
        }

        output = return_data_;
        return ExecutionResult::SUCCESS;
    }

    ExecutionResult ExecuteOpcode(Opcode opcode) {
        // Charge gas
        uint64_t gas_cost = IntSCVM::GetOpcodeGasCost(opcode);
        if (gas_cost > gas_remaining_) {
            return ExecutionResult::OUT_OF_GAS;
        }
        gas_remaining_ -= gas_cost;
        gas_used_ += gas_cost;

        switch (opcode) {
            case Opcode::STOP:
                return ExecutionResult::SUCCESS;

            // Arithmetic operations
            case Opcode::ADD: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = AddWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::MUL: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = MulWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::SUB: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = SubWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::DIV: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                if (IsZeroWord(b)) {
                    // Division by zero returns 0
                    if (!stack_.Push(Word256{})) return ExecutionResult::STACK_OVERFLOW;
                } else {
                    Word256 result = DivWord256(a, b);
                    if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                }
                break;
            }

            case Opcode::MOD: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                if (IsZeroWord(b)) {
                    if (!stack_.Push(Word256{})) return ExecutionResult::STACK_OVERFLOW;
                } else {
                    Word256 result = ModWord256(a, b);
                    if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                }
                break;
            }

            // Comparison operations
            case Opcode::LT: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                bool less = CompareWord256(a, b) < 0;
                Word256 result = less ? Uint64ToWord256(1) : Word256{};
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::GT: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                bool greater = CompareWord256(a, b) > 0;
                Word256 result = greater ? Uint64ToWord256(1) : Word256{};
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::EQ: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                bool equal = CompareWord256(a, b) == 0;
                Word256 result = equal ? Uint64ToWord256(1) : Word256{};
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::ISZERO: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                bool zero = IsZeroWord(a);
                Word256 result = zero ? Uint64ToWord256(1) : Word256{};
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            // Bitwise operations
            case Opcode::AND: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = AndWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::OR: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = OrWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::XOR: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 b = stack_.Pop();
                Word256 result = XorWord256(a, b);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::NOT: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                Word256 a = stack_.Pop();
                Word256 result = NotWord256(a);
                if (!stack_.Push(result)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            // Stack operations
            case Opcode::POP: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                stack_.Pop();
                break;
            }

            // PUSH operations (0x60-0x7f)
            case Opcode::PUSH1: case Opcode::PUSH2: case Opcode::PUSH3: case Opcode::PUSH4:
            case Opcode::PUSH5: case Opcode::PUSH6: case Opcode::PUSH7: case Opcode::PUSH8:
            case Opcode::PUSH9: case Opcode::PUSH10: case Opcode::PUSH11: case Opcode::PUSH12:
            case Opcode::PUSH13: case Opcode::PUSH14: case Opcode::PUSH15: case Opcode::PUSH16:
            case Opcode::PUSH17: case Opcode::PUSH18: case Opcode::PUSH19: case Opcode::PUSH20:
            case Opcode::PUSH21: case Opcode::PUSH22: case Opcode::PUSH23: case Opcode::PUSH24:
            case Opcode::PUSH25: case Opcode::PUSH26: case Opcode::PUSH27: case Opcode::PUSH28:
            case Opcode::PUSH29: case Opcode::PUSH30: case Opcode::PUSH31: case Opcode::PUSH32: {
                uint8_t num_bytes = static_cast<uint8_t>(opcode) - 0x5f;
                Word256 value{};
                for (uint8_t i = 0; i < num_bytes && (pc_ + 1 + i) < bytecode_.size(); ++i) {
                    value[32 - num_bytes + i] = bytecode_[pc_ + 1 + i];
                }
                if (!stack_.Push(value)) return ExecutionResult::STACK_OVERFLOW;
                pc_ += num_bytes;  // Skip the pushed bytes
                break;
            }

            // DUP operations (0x80-0x8f)
            case Opcode::DUP1: case Opcode::DUP2: case Opcode::DUP3: case Opcode::DUP4:
            case Opcode::DUP5: case Opcode::DUP6: case Opcode::DUP7: case Opcode::DUP8:
            case Opcode::DUP9: case Opcode::DUP10: case Opcode::DUP11: case Opcode::DUP12:
            case Opcode::DUP13: case Opcode::DUP14: case Opcode::DUP15: case Opcode::DUP16: {
                uint8_t depth = static_cast<uint8_t>(opcode) - 0x7f;
                if (!stack_.Dup(depth)) {
                    return ExecutionResult::STACK_UNDERFLOW;
                }
                break;
            }

            // SWAP operations (0x90-0x9f)
            case Opcode::SWAP1: case Opcode::SWAP2: case Opcode::SWAP3: case Opcode::SWAP4:
            case Opcode::SWAP5: case Opcode::SWAP6: case Opcode::SWAP7: case Opcode::SWAP8:
            case Opcode::SWAP9: case Opcode::SWAP10: case Opcode::SWAP11: case Opcode::SWAP12:
            case Opcode::SWAP13: case Opcode::SWAP14: case Opcode::SWAP15: case Opcode::SWAP16: {
                uint8_t depth = static_cast<uint8_t>(opcode) - 0x8f;
                if (!stack_.Swap(depth)) {
                    return ExecutionResult::STACK_UNDERFLOW;
                }
                break;
            }

            // Memory operations
            case Opcode::MLOAD: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                Word256 value = memory_.ReadWord(offset);
                if (!stack_.Push(value)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::MSTORE: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                Word256 value = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                memory_.WriteWord(offset, value);
                break;
            }

            case Opcode::MSTORE8: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                Word256 value_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                uint8_t value = value_word[31];  // Last byte
                memory_.WriteByte(offset, value);
                break;
            }

            // Storage operations
            case Opcode::SLOAD: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                if (!storage_) return ExecutionResult::INTERNAL_ERROR;
                Word256 key = stack_.Pop();
                Word256 value = storage_->Load(context_.address, key);
                if (!stack_.Push(value)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::SSTORE: {
                if (context_.mode == ExecutionMode::STATIC) {
                    return ExecutionResult::REVERT;
                }
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                if (!storage_) return ExecutionResult::INTERNAL_ERROR;
                Word256 key = stack_.Pop();
                Word256 value = stack_.Pop();
                storage_->Store(context_.address, key, value);
                break;
            }

            // Control flow
            case Opcode::JUMP: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                Word256 dest_word = stack_.Pop();
                uint64_t dest = Word256ToUint64(dest_word);
                if (dest >= bytecode_.size()) {
                    return ExecutionResult::INVALID_JUMP;
                }
                // Verify JUMPDEST
                if (bytecode_[dest] != static_cast<uint8_t>(Opcode::JUMPDEST)) {
                    return ExecutionResult::INVALID_JUMP;
                }
                pc_ = dest;
                break;
            }

            case Opcode::JUMPI: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 dest_word = stack_.Pop();
                Word256 cond = stack_.Pop();
                if (!IsZeroWord(cond)) {
                    uint64_t dest = Word256ToUint64(dest_word);
                    if (dest >= bytecode_.size()) {
                        return ExecutionResult::INVALID_JUMP;
                    }
                    if (bytecode_[dest] != static_cast<uint8_t>(Opcode::JUMPDEST)) {
                        return ExecutionResult::INVALID_JUMP;
                    }
                    pc_ = dest;
                } else {
                    pc_++;
                }
                break;
            }

            case Opcode::JUMPDEST:
                // Valid jump destination, no-op
                break;

            case Opcode::PC: {
                Word256 pc_word = Uint64ToWord256(pc_);
                if (!stack_.Push(pc_word)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::MSIZE: {
                Word256 size = Uint64ToWord256(memory_.Size());
                if (!stack_.Push(size)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::GAS: {
                Word256 gas = Uint64ToWord256(gas_remaining_);
                if (!stack_.Push(gas)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            // Environment operations
            case Opcode::ADDRESS: {
                Word256 addr = StringToWord256(context_.address);
                if (!stack_.Push(addr)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CALLER: {
                Word256 caller = StringToWord256(context_.caller);
                if (!stack_.Push(caller)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CALLVALUE: {
                Word256 value = Uint64ToWord256(context_.value);
                if (!stack_.Push(value)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CALLDATASIZE: {
                Word256 size = Uint64ToWord256(context_.calldata.size());
                if (!stack_.Push(size)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CALLDATALOAD: {
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                Word256 data{};
                for (size_t i = 0; i < 32; ++i) {
                    if (offset + i < context_.calldata.size()) {
                        data[i] = context_.calldata[offset + i];
                    }
                }
                if (!stack_.Push(data)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CALLDATACOPY: {
                if (stack_.Size() < 3) return ExecutionResult::STACK_UNDERFLOW;
                Word256 dest_offset_word = stack_.Pop();
                Word256 offset_word = stack_.Pop();
                Word256 length_word = stack_.Pop();
                uint64_t dest_offset = Word256ToUint64(dest_offset_word);
                uint64_t offset = Word256ToUint64(offset_word);
                uint64_t length = Word256ToUint64(length_word);

                for (uint64_t i = 0; i < length; ++i) {
                    uint8_t byte = 0;
                    if (offset + i < context_.calldata.size()) {
                        byte = context_.calldata[offset + i];
                    }
                    memory_.WriteByte(dest_offset + i, byte);
                }
                break;
            }

            // Block operations
            case Opcode::BLOCKHASH: {
                // Simplified: return zero hash
                if (stack_.Size() < 1) return ExecutionResult::STACK_UNDERFLOW;
                stack_.Pop();  // Block number
                if (!stack_.Push(Word256{})) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::COINBASE: {
                Word256 coinbase = StringToWord256(context_.block_coinbase);
                if (!stack_.Push(coinbase)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::TIMESTAMP: {
                Word256 timestamp = Uint64ToWord256(context_.block_timestamp);
                if (!stack_.Push(timestamp)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::NUMBER: {
                Word256 number = Uint64ToWord256(context_.block_number);
                if (!stack_.Push(number)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::DIFFICULTY: {
                Word256 difficulty = Uint64ToWord256(context_.block_difficulty);
                if (!stack_.Push(difficulty)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::GASLIMIT: {
                Word256 gaslimit = Uint64ToWord256(context_.block_gas_limit);
                if (!stack_.Push(gaslimit)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            case Opcode::CHAINID: {
                Word256 chainid = Uint64ToWord256(context_.chain_id);
                if (!stack_.Push(chainid)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            // Return operation
            case Opcode::RETURN: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                Word256 length_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                uint64_t length = Word256ToUint64(length_word);
                return_data_ = memory_.ReadBytes(offset, length);
                return ExecutionResult::SUCCESS;
            }

            case Opcode::REVERT: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                Word256 length_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                uint64_t length = Word256ToUint64(length_word);
                return_data_ = memory_.ReadBytes(offset, length);
                return ExecutionResult::REVERT;
            }

            // Hash operations
            case Opcode::SHA3: {
                if (stack_.Size() < 2) return ExecutionResult::STACK_UNDERFLOW;
                Word256 offset_word = stack_.Pop();
                Word256 length_word = stack_.Pop();
                uint64_t offset = Word256ToUint64(offset_word);
                uint64_t length = Word256ToUint64(length_word);

                std::vector<uint8_t> data = memory_.ReadBytes(offset, length);
                // Use SHA3-256 hash
                uint256 hash_result = SHA3::Hash(data.data(), data.size());
                Word256 hash;
                std::copy(hash_result.begin(), hash_result.end(), hash.begin());
                if (!stack_.Push(hash)) return ExecutionResult::STACK_OVERFLOW;
                break;
            }

            // Post-Quantum Cryptography opcodes
            case Opcode::DILITHIUM_VERIFY:
            case Opcode::KYBER_ENCAP:
            case Opcode::KYBER_DECAP:
            case Opcode::PQC_PUBKEY:
                // TODO: Implement PQC operations
                return ExecutionResult::INVALID_OPCODE;

            default:
                return ExecutionResult::INVALID_OPCODE;
        }

        return ExecutionResult::SUCCESS;
    }
};

//
// IntSCVM Public API
//

IntSCVM::IntSCVM() : pimpl_(std::make_unique<Impl>()) {}

IntSCVM::IntSCVM(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

IntSCVM::IntSCVM(std::shared_ptr<IStorage> storage)
    : pimpl_(std::make_unique<Impl>(std::move(storage))) {}

IntSCVM::IntSCVM(const Config& config, std::shared_ptr<IStorage> storage)
    : pimpl_(std::make_unique<Impl>(config, std::move(storage))) {}

IntSCVM::~IntSCVM() = default;

ExecutionResult IntSCVM::Execute(
    const std::vector<uint8_t>& bytecode,
    const ExecutionContext& context
) {
    std::vector<uint8_t> output;
    return pimpl_->Execute(bytecode, context, output);
}

ExecutionResult IntSCVM::Execute(
    const std::vector<uint8_t>& bytecode,
    const ExecutionContext& context,
    std::vector<uint8_t>& output
) {
    return pimpl_->Execute(bytecode, context, output);
}

uint64_t IntSCVM::GetGasUsed() const {
    return pimpl_->gas_used_;
}

uint64_t IntSCVM::GetGasRemaining() const {
    return pimpl_->gas_remaining_;
}

std::vector<uint8_t> IntSCVM::GetReturnData() const {
    return pimpl_->return_data_;
}

std::vector<IntSCVM::Log> IntSCVM::GetLogs() const {
    return pimpl_->logs_;
}

void IntSCVM::SetStorage(std::shared_ptr<IStorage> storage) {
    pimpl_->storage_ = std::move(storage);
}

IntSCVM::Config IntSCVM::GetConfig() const {
    return pimpl_->config_;
}

void IntSCVM::SetConfig(const Config& config) {
    pimpl_->config_ = config;
}

void IntSCVM::Reset() {
    pimpl_->Reset();
}

std::string IntSCVM::GetOpcodeName(Opcode opcode) {
    switch (opcode) {
        case Opcode::STOP: return "STOP";
        case Opcode::ADD: return "ADD";
        case Opcode::MUL: return "MUL";
        case Opcode::SUB: return "SUB";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";
        case Opcode::LT: return "LT";
        case Opcode::GT: return "GT";
        case Opcode::EQ: return "EQ";
        case Opcode::ISZERO: return "ISZERO";
        case Opcode::AND: return "AND";
        case Opcode::OR: return "OR";
        case Opcode::XOR: return "XOR";
        case Opcode::NOT: return "NOT";
        case Opcode::POP: return "POP";
        case Opcode::MLOAD: return "MLOAD";
        case Opcode::MSTORE: return "MSTORE";
        case Opcode::SLOAD: return "SLOAD";
        case Opcode::SSTORE: return "SSTORE";
        case Opcode::JUMP: return "JUMP";
        case Opcode::JUMPI: return "JUMPI";
        case Opcode::JUMPDEST: return "JUMPDEST";
        case Opcode::RETURN: return "RETURN";
        case Opcode::REVERT: return "REVERT";
        case Opcode::SHA3: return "SHA3";
        case Opcode::DILITHIUM_VERIFY: return "DILITHIUM_VERIFY";
        case Opcode::KYBER_ENCAP: return "KYBER_ENCAP";
        case Opcode::KYBER_DECAP: return "KYBER_DECAP";
        case Opcode::PQC_PUBKEY: return "PQC_PUBKEY";
        default: {
            if (opcode >= Opcode::PUSH1 && opcode <= Opcode::PUSH32) {
                uint8_t n = static_cast<uint8_t>(opcode) - 0x5f;
                return "PUSH" + std::to_string(n);
            }
            if (opcode >= Opcode::DUP1 && opcode <= Opcode::DUP16) {
                uint8_t n = static_cast<uint8_t>(opcode) - 0x7f;
                return "DUP" + std::to_string(n);
            }
            if (opcode >= Opcode::SWAP1 && opcode <= Opcode::SWAP16) {
                uint8_t n = static_cast<uint8_t>(opcode) - 0x8f;
                return "SWAP" + std::to_string(n);
            }
            return "UNKNOWN";
        }
    }
}

uint64_t IntSCVM::GetOpcodeGasCost(Opcode opcode) {
    switch (opcode) {
        case Opcode::STOP: return 0;
        case Opcode::ADD: case Opcode::SUB: return 3;
        case Opcode::MUL: return 5;
        case Opcode::DIV: case Opcode::MOD: return 5;
        case Opcode::LT: case Opcode::GT: case Opcode::EQ: return 3;
        case Opcode::ISZERO: return 3;
        case Opcode::AND: case Opcode::OR: case Opcode::XOR: case Opcode::NOT: return 3;
        case Opcode::POP: return 2;
        case Opcode::MLOAD: case Opcode::MSTORE: return 3;
        case Opcode::SLOAD: return 800;
        case Opcode::SSTORE: return 20000;
        case Opcode::JUMP: return 8;
        case Opcode::JUMPI: return 10;
        case Opcode::JUMPDEST: return 1;
        case Opcode::SHA3: return 30;
        case Opcode::DILITHIUM_VERIFY: return 50000;
        case Opcode::KYBER_ENCAP: case Opcode::KYBER_DECAP: return 30000;
        case Opcode::PQC_PUBKEY: return 100;
        default:
            if (opcode >= Opcode::PUSH1 && opcode <= Opcode::PUSH32) {
                return 3;
            }
            if (opcode >= Opcode::DUP1 && opcode <= Opcode::DUP16) {
                return 3;
            }
            if (opcode >= Opcode::SWAP1 && opcode <= Opcode::SWAP16) {
                return 3;
            }
            return 1;
    }
}

std::string IntSCVM::Disassemble(const std::vector<uint8_t>& bytecode) {
    std::stringstream ss;
    size_t pc = 0;

    while (pc < bytecode.size()) {
        Opcode opcode = static_cast<Opcode>(bytecode[pc]);
        ss << std::setw(6) << std::setfill('0') << pc << " ";
        ss << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(bytecode[pc]) << " ";
        ss << std::setfill(' ') << std::setw(20) << std::left
           << GetOpcodeName(opcode);

        // Show push data
        if (opcode >= Opcode::PUSH1 && opcode <= Opcode::PUSH32) {
            uint8_t num_bytes = static_cast<uint8_t>(opcode) - 0x5f;
            ss << " 0x";
            for (uint8_t i = 0; i < num_bytes && (pc + 1 + i) < bytecode.size(); ++i) {
                ss << std::setw(2) << std::setfill('0') << std::hex
                   << static_cast<int>(bytecode[pc + 1 + i]);
            }
            pc += num_bytes;
        }

        ss << "\n";
        pc++;
    }

    return ss.str();
}

//
// Helper Functions
//

uint64_t Word256ToUint64(const Word256& word) {
    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i) {
        result = (result << 8) | word[24 + i];
    }
    return result;
}

Word256 Uint64ToWord256(uint64_t value) {
    Word256 word{};
    for (int i = 7; i >= 0; --i) {
        word[24 + i] = value & 0xff;
        value >>= 8;
    }
    return word;
}

Word256 StringToWord256(const std::string& str) {
    Word256 word{};
    size_t len = std::min(str.size(), size_t(32));
    std::memcpy(word.data() + (32 - len), str.data(), len);
    return word;
}

std::string Word256ToHex(const Word256& word) {
    std::stringstream ss;
    ss << "0x";
    for (uint8_t byte : word) {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
    }
    return ss.str();
}

Word256 HexToWord256(const std::string& hex) {
    Word256 word{};
    std::string hex_str = hex;
    if (hex_str.substr(0, 2) == "0x") {
        hex_str = hex_str.substr(2);
    }

    for (size_t i = 0; i < hex_str.size() && i < 64; i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        word[i / 2] = std::stoi(byte_str, nullptr, 16);
    }

    return word;
}

bool IsZeroWord(const Word256& word) {
    for (uint8_t byte : word) {
        if (byte != 0) return false;
    }
    return true;
}

int CompareWord256(const Word256& a, const Word256& b) {
    for (size_t i = 0; i < 32; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

Word256 AddWord256(const Word256& a, const Word256& b) {
    Word256 result{};
    uint16_t carry = 0;
    for (int i = 31; i >= 0; --i) {
        uint16_t sum = a[i] + b[i] + carry;
        result[i] = sum & 0xff;
        carry = sum >> 8;
    }
    return result;
}

Word256 SubWord256(const Word256& a, const Word256& b) {
    Word256 result{};
    int16_t borrow = 0;
    for (int i = 31; i >= 0; --i) {
        int16_t diff = a[i] - b[i] - borrow;
        if (diff < 0) {
            diff += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result[i] = diff & 0xff;
    }
    return result;
}

Word256 MulWord256(const Word256& a, const Word256& b) {
    // Simplified multiplication (not full 256-bit precision)
    uint64_t a_val = Word256ToUint64(a);
    uint64_t b_val = Word256ToUint64(b);
    return Uint64ToWord256(a_val * b_val);
}

Word256 DivWord256(const Word256& a, const Word256& b) {
    // Simplified division
    uint64_t a_val = Word256ToUint64(a);
    uint64_t b_val = Word256ToUint64(b);
    if (b_val == 0) return Word256{};
    return Uint64ToWord256(a_val / b_val);
}

Word256 ModWord256(const Word256& a, const Word256& b) {
    // Simplified modulo
    uint64_t a_val = Word256ToUint64(a);
    uint64_t b_val = Word256ToUint64(b);
    if (b_val == 0) return Word256{};
    return Uint64ToWord256(a_val % b_val);
}

Word256 AndWord256(const Word256& a, const Word256& b) {
    Word256 result{};
    for (size_t i = 0; i < 32; ++i) {
        result[i] = a[i] & b[i];
    }
    return result;
}

Word256 OrWord256(const Word256& a, const Word256& b) {
    Word256 result{};
    for (size_t i = 0; i < 32; ++i) {
        result[i] = a[i] | b[i];
    }
    return result;
}

Word256 XorWord256(const Word256& a, const Word256& b) {
    Word256 result{};
    for (size_t i = 0; i < 32; ++i) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

Word256 NotWord256(const Word256& a) {
    Word256 result{};
    for (size_t i = 0; i < 32; ++i) {
        result[i] = ~a[i];
    }
    return result;
}

} // namespace contracts
} // namespace intcoin
