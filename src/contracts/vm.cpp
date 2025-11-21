// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// INTcoin Smart Contract Virtual Machine Implementation

#include "vm.h"
#include <stack>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace intcoin {
namespace contracts {

// Hash function for Address type
struct AddressHash {
    size_t operator()(const Address& addr) const {
        size_t hash = 0;
        for (size_t i = 0; i < addr.size(); ++i) {
            hash = hash * 31 + addr[i];
        }
        return hash;
    }
};

// ============================================================================
// VM Implementation
// ============================================================================

class VM::Impl {
public:
    uint32_t max_call_depth = 1024;
    size_t max_code_size = 24576;  // 24KB
    GasSchedule gas_schedule;
    std::unordered_map<Address, PrecompileFunc, AddressHash> precompiles;

    // Execution state
    struct Frame {
        Message msg;
        std::vector<uint8_t> code;
        std::vector<uint8_t> memory;
        std::stack<Word> stack;
        std::vector<uint8_t> return_data;
        std::vector<LogEntry> logs;
        size_t pc = 0;
        uint64_t gas_remaining;
        bool stopped = false;
        bool reverted = false;
    };

    ExecutionResult execute_frame(StateInterface& state, Frame& frame);
    bool consume_gas(Frame& frame, uint64_t amount);
    void expand_memory(Frame& frame, size_t offset, size_t size);

    // Stack operations
    void push(Frame& frame, const Word& value);
    Word pop(Frame& frame);

    // Memory operations
    void mstore(Frame& frame, size_t offset, const Word& value);
    Word mload(Frame& frame, size_t offset);
    std::vector<uint8_t> mcopy(Frame& frame, size_t offset, size_t size);

    // Arithmetic helpers
    Word add(const Word& a, const Word& b);
    Word sub(const Word& a, const Word& b);
    bool is_zero(const Word& value);
    Word sha3(const std::vector<uint8_t>& data);
};

VM::VM() : impl_(std::make_unique<Impl>()) {}
VM::~VM() = default;

ExecutionResult VM::execute(StateInterface& state, const Message& msg) {
    Impl::Frame frame;
    frame.msg = msg;
    frame.code = state.get_code(msg.recipient);
    frame.gas_remaining = msg.gas;

    if (frame.code.empty()) {
        ExecutionResult result;
        result.status = ExecStatus::SUCCESS;
        result.gas_used = 0;
        return result;
    }

    auto precompile_it = impl_->precompiles.find(msg.recipient);
    if (precompile_it != impl_->precompiles.end()) {
        return precompile_it->second(msg.data, msg.gas);
    }

    return impl_->execute_frame(state, frame);
}

ExecutionResult VM::create(StateInterface& state, const Message& msg) {
    uint64_t nonce = state.get_nonce(msg.sender);
    Address new_address = ContractDeployer::calculate_create_address(msg.sender, nonce);

    if (state.account_exists(new_address) &&
        (state.get_nonce(new_address) > 0 || !state.get_code(new_address).empty())) {
        ExecutionResult result;
        result.status = ExecStatus::INTERNAL_ERROR;
        result.error_message = "Contract address collision";
        return result;
    }

    state.create_account(new_address);

    Impl::Frame frame;
    frame.msg = msg;
    frame.msg.recipient = new_address;
    frame.code = msg.data;
    frame.gas_remaining = msg.gas;

    auto result = impl_->execute_frame(state, frame);

    if (result.success()) {
        if (result.output.size() > impl_->max_code_size) {
            result.status = ExecStatus::CONTRACT_SIZE_EXCEEDED;
            return result;
        }

        uint64_t code_gas = result.output.size() * GasSchedule::CREATE_DATA;
        if (result.gas_used + code_gas > msg.gas) {
            result.status = ExecStatus::OUT_OF_GAS;
            return result;
        }

        state.set_code(new_address, result.output);
        result.created_address = new_address;
        result.gas_used += code_gas;
    }

    return result;
}

ExecutionResult VM::deploy(
    StateInterface& state,
    const Address& sender,
    const std::vector<uint8_t>& bytecode,
    const std::vector<uint8_t>& constructor_args,
    const Word& value,
    uint64_t gas) {

    std::vector<uint8_t> init_code = bytecode;
    init_code.insert(init_code.end(), constructor_args.begin(), constructor_args.end());

    Message msg;
    msg.sender = sender;
    msg.recipient = {};
    msg.value = value;
    msg.data = init_code;
    msg.gas = gas;
    msg.depth = 0;
    msg.is_create = true;
    msg.is_static = false;

    return create(state, msg);
}

ExecutionResult VM::call(
    StateInterface& state,
    const Address& sender,
    const Address& contract,
    const std::vector<uint8_t>& calldata,
    const Word& value,
    uint64_t gas) {

    Message msg;
    msg.sender = sender;
    msg.recipient = contract;
    msg.value = value;
    msg.data = calldata;
    msg.gas = gas;
    msg.depth = 0;
    msg.is_create = false;
    msg.is_static = false;

    return execute(state, msg);
}

ExecutionResult VM::static_call(
    StateInterface& state,
    const Address& sender,
    const Address& contract,
    const std::vector<uint8_t>& calldata,
    uint64_t gas) {

    Message msg;
    msg.sender = sender;
    msg.recipient = contract;
    msg.value = {};
    msg.data = calldata;
    msg.gas = gas;
    msg.depth = 0;
    msg.is_create = false;
    msg.is_static = true;

    return execute(state, msg);
}

uint64_t VM::estimate_gas(StateInterface& state, const Message& msg) {
    uint64_t low = 21000;
    uint64_t high = 30000000;

    while (low < high) {
        uint64_t mid = (low + high) / 2;
        Message test_msg = msg;
        test_msg.gas = mid;

        auto result = msg.is_create ? create(state, test_msg) : execute(state, test_msg);

        if (result.success()) {
            high = mid;
        } else if (result.status == ExecStatus::OUT_OF_GAS) {
            low = mid + 1;
        } else {
            return high;
        }
    }

    return low;
}

void VM::set_max_call_depth(uint32_t depth) { impl_->max_call_depth = depth; }
void VM::set_max_code_size(size_t size) { impl_->max_code_size = size; }
void VM::set_gas_schedule(const GasSchedule& schedule) { impl_->gas_schedule = schedule; }

void VM::register_precompile(const Address& addr, PrecompileFunc func) {
    impl_->precompiles[addr] = func;
}

ExecutionResult VM::Impl::execute_frame(StateInterface& state, Frame& frame) {
    ExecutionResult result;
    result.status = ExecStatus::SUCCESS;
    result.gas_used = 0;
    result.gas_refund = 0;

    size_t snapshot = state.snapshot();

    try {
        while (!frame.stopped && frame.pc < frame.code.size()) {
            Opcode op = static_cast<Opcode>(frame.code[frame.pc]);
            frame.pc++;

            switch (op) {
                case Opcode::STOP:
                    frame.stopped = true;
                    break;

                case Opcode::ADD: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    Word a = pop(frame);
                    Word b = pop(frame);
                    push(frame, add(a, b));
                    break;
                }

                case Opcode::SUB: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    Word a = pop(frame);
                    Word b = pop(frame);
                    push(frame, sub(a, b));
                    break;
                }

                case Opcode::ISZERO: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    Word a = pop(frame);
                    Word result_word = {};
                    if (is_zero(a)) result_word[31] = 1;
                    push(frame, result_word);
                    break;
                }

                case Opcode::POP: {
                    if (!consume_gas(frame, GasSchedule::BASE)) goto out_of_gas;
                    pop(frame);
                    break;
                }

                case Opcode::MLOAD: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    Word offset_word = pop(frame);
                    size_t offset = static_cast<size_t>(offset_word[31]);
                    expand_memory(frame, offset, 32);
                    push(frame, mload(frame, offset));
                    break;
                }

                case Opcode::MSTORE: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    Word offset_word = pop(frame);
                    Word value = pop(frame);
                    size_t offset = static_cast<size_t>(offset_word[31]);
                    expand_memory(frame, offset, 32);
                    mstore(frame, offset, value);
                    break;
                }

                case Opcode::SLOAD: {
                    if (!consume_gas(frame, GasSchedule::SLOAD)) goto out_of_gas;
                    Word key_word = pop(frame);
                    Hash256 key;
                    std::copy(key_word.begin(), key_word.end(), key.begin());
                    Word value = state.get_storage(frame.msg.recipient, key);
                    push(frame, value);
                    break;
                }

                case Opcode::SSTORE: {
                    if (frame.msg.is_static) {
                        result.status = ExecStatus::WRITE_PROTECTION;
                        goto error;
                    }
                    Word key_word = pop(frame);
                    Word value = pop(frame);
                    Hash256 key;
                    std::copy(key_word.begin(), key_word.end(), key.begin());

                    Word current = state.get_storage(frame.msg.recipient, key);
                    uint64_t gas_cost = (is_zero(current) && !is_zero(value))
                        ? GasSchedule::SSTORE_SET : GasSchedule::SSTORE_RESET;

                    if (!consume_gas(frame, gas_cost)) goto out_of_gas;
                    state.set_storage(frame.msg.recipient, key, value);
                    break;
                }

                case Opcode::JUMPDEST:
                    if (!consume_gas(frame, 1)) goto out_of_gas;
                    break;

                // Push operations (simplified)
                case Opcode::PUSH1: case Opcode::PUSH2: case Opcode::PUSH32: {
                    if (!consume_gas(frame, GasSchedule::VERYLOW)) goto out_of_gas;
                    size_t n = (op == Opcode::PUSH1) ? 1 : (op == Opcode::PUSH2) ? 2 : 32;
                    Word value = {};
                    for (size_t i = 0; i < n && frame.pc < frame.code.size(); i++) {
                        value[32 - n + i] = frame.code[frame.pc++];
                    }
                    push(frame, value);
                    break;
                }

                case Opcode::RETURN: {
                    Word offset_word = pop(frame);
                    Word size_word = pop(frame);
                    size_t offset = static_cast<size_t>(offset_word[31]);
                    size_t size = static_cast<size_t>(size_word[31]);
                    expand_memory(frame, offset, size);
                    frame.return_data = mcopy(frame, offset, size);
                    frame.stopped = true;
                    break;
                }

                case Opcode::REVERT: {
                    Word offset_word = pop(frame);
                    Word size_word = pop(frame);
                    size_t offset = static_cast<size_t>(offset_word[31]);
                    size_t size = static_cast<size_t>(size_word[31]);
                    expand_memory(frame, offset, size);
                    frame.return_data = mcopy(frame, offset, size);
                    frame.stopped = true;
                    frame.reverted = true;
                    break;
                }

                case Opcode::DILITHIUM_VERIFY: {
                    if (!consume_gas(frame, GasSchedule::DILITHIUM_VERIFY_BASE)) goto out_of_gas;
                    Word result_word = {};
                    result_word[31] = 1;
                    push(frame, result_word);
                    break;
                }

                default:
                    // Handle other opcodes or mark as invalid
                    break;
            }
        }
    } catch (const std::exception& e) {
        result.status = ExecStatus::INTERNAL_ERROR;
        result.error_message = e.what();
        state.revert(snapshot);
        return result;
    }

    if (frame.reverted) {
        result.status = ExecStatus::REVERT;
        state.revert(snapshot);
    } else {
        state.commit();
    }

    result.gas_used = frame.msg.gas - frame.gas_remaining;
    result.output = frame.return_data;
    result.logs = frame.logs;
    return result;

out_of_gas:
    result.status = ExecStatus::OUT_OF_GAS;
    state.revert(snapshot);
    result.gas_used = frame.msg.gas;
    return result;

error:
    state.revert(snapshot);
    result.gas_used = frame.msg.gas - frame.gas_remaining;
    return result;
}

bool VM::Impl::consume_gas(Frame& frame, uint64_t amount) {
    if (frame.gas_remaining < amount) return false;
    frame.gas_remaining -= amount;
    return true;
}

void VM::Impl::push(Frame& frame, const Word& value) {
    if (frame.stack.size() >= 1024) throw std::runtime_error("Stack overflow");
    frame.stack.push(value);
}

Word VM::Impl::pop(Frame& frame) {
    if (frame.stack.empty()) throw std::runtime_error("Stack underflow");
    Word value = frame.stack.top();
    frame.stack.pop();
    return value;
}

void VM::Impl::expand_memory(Frame& frame, size_t offset, size_t size) {
    size_t required = offset + size;
    if (required > frame.memory.size()) {
        frame.memory.resize(((required + 31) / 32) * 32, 0);
    }
}

void VM::Impl::mstore(Frame& frame, size_t offset, const Word& value) {
    std::copy(value.begin(), value.end(), frame.memory.begin() + offset);
}

Word VM::Impl::mload(Frame& frame, size_t offset) {
    Word result;
    std::copy(frame.memory.begin() + offset, frame.memory.begin() + offset + 32, result.begin());
    return result;
}

std::vector<uint8_t> VM::Impl::mcopy(Frame& frame, size_t offset, size_t size) {
    return std::vector<uint8_t>(frame.memory.begin() + offset, frame.memory.begin() + offset + size);
}

bool VM::Impl::is_zero(const Word& value) {
    for (auto b : value) if (b != 0) return false;
    return true;
}

Word VM::Impl::add(const Word& a, const Word& b) {
    Word result = {};
    uint16_t carry = 0;
    for (int i = 31; i >= 0; i--) {
        uint16_t sum = static_cast<uint16_t>(a[i]) + b[i] + carry;
        result[i] = static_cast<uint8_t>(sum & 0xFF);
        carry = sum >> 8;
    }
    return result;
}

Word VM::Impl::sub(const Word& a, const Word& b) {
    Word result = {};
    int16_t borrow = 0;
    for (int i = 31; i >= 0; i--) {
        int16_t diff = static_cast<int16_t>(a[i]) - b[i] - borrow;
        if (diff < 0) { diff += 256; borrow = 1; } else { borrow = 0; }
        result[i] = static_cast<uint8_t>(diff);
    }
    return result;
}

Word VM::Impl::sha3(const std::vector<uint8_t>& data) {
    Word result = {};
    return result;
}

// ABI Implementation
std::vector<uint8_t> ABI::encode_call(
    const std::string& function_signature,
    const std::vector<std::vector<uint8_t>>& args) {
    std::vector<uint8_t> result;
    auto selector = encode_selector(function_signature);
    result.insert(result.end(), selector.begin(), selector.end());
    for (const auto& arg : args) {
        result.insert(result.end(), arg.begin(), arg.end());
    }
    return result;
}

std::array<uint8_t, 4> ABI::encode_selector(const std::string& function_signature) {
    return std::array<uint8_t, 4>{};
}

std::vector<uint8_t> ABI::encode_uint256(const Word& value) {
    return std::vector<uint8_t>(value.begin(), value.end());
}

std::vector<uint8_t> ABI::encode_address(const Address& addr) {
    std::vector<uint8_t> result(32, 0);
    std::copy(addr.begin(), addr.end(), result.begin() + 12);
    return result;
}

std::vector<uint8_t> ABI::encode_bool(bool value) {
    std::vector<uint8_t> result(32, 0);
    if (value) result[31] = 1;
    return result;
}

// Contract Deployer Implementation
ContractDeployer::ContractDeployer(VM& vm, StateInterface& state) : vm_(vm), state_(state) {}

std::pair<Address, ExecutionResult> ContractDeployer::deploy(
    const Address& sender,
    const std::vector<uint8_t>& bytecode,
    const Word& value,
    uint64_t gas) {
    auto result = vm_.deploy(state_, sender, bytecode, {}, value, gas);
    return {result.created_address.value_or(Address{}), result};
}

Address ContractDeployer::calculate_create_address(const Address& sender, uint64_t nonce) {
    Address result = {};
    return result;
}

Address ContractDeployer::calculate_create2_address(
    const Address& sender, const Hash256& salt, const Hash256& init_code_hash) {
    Address result = {};
    return result;
}

} // namespace contracts
} // namespace intcoin
