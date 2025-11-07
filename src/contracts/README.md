# INTcoin Smart Contracts

## Overview

INTcoin Smart Contracts enable decentralized applications (dApps) to run on the INTcoin blockchain. The platform provides a secure, gas-metered virtual machine with built-in protection against common vulnerabilities.

## Features

✅ **Secure Virtual Machine**
- Gas metering to prevent infinite loops
- Stack depth limits (prevents overflow)
- Memory limits (prevents DOS attacks)
- Integer overflow protection
- Input validation on all operations

✅ **Security Protections**
- SafeMath for all arithmetic operations
- Reentrancy attack prevention
- Bounds checking on all array/memory access
- Divide-by-zero protection
- Buffer overflow protection

✅ **Standard Interfaces**
- ERC20-compatible tokens
- NFT (Non-Fungible Token) support
- Custom contract templates

✅ **Development Tools**
- Security analyzer
- Gas estimation
- Bytecode validation
- Contract templates

## Architecture

```
┌────────────────────────────────────┐
│     Contract Manager               │
│  - Deploy contracts                │
│  - Execute calls                   │
│  - Manage state                    │
└──────────┬─────────────────────────┘
           │
           │
┌──────────▼─────────────────────────┐
│     Virtual Machine                │
│  - Bytecode execution              │
│  - Gas metering                    │
│  - Stack management                │
│  - Memory management               │
└──────────┬─────────────────────────┘
           │
    ┌──────▼──────┐    ┌─────────────┐
    │  Contract   │    │   Storage   │
    │   State     │◄───┤   (Key/Val) │
    └─────────────┘    └─────────────┘
```

## Security Model

### Integer Overflow Protection

All arithmetic operations use SafeMath:

```cpp
// Safe addition with overflow check
std::optional<uint64_t> result = SafeMath::safe_add(a, b);
if (!result) {
    return ExecutionResult::INTEGER_OVERFLOW;
}

// Safe multiplication with overflow check
std::optional<uint64_t> product = SafeMath::safe_mul(x, y);
if (!product) {
    return ExecutionResult::INTEGER_OVERFLOW;
}

// Safe division with divide-by-zero check
std::optional<uint64_t> quotient = SafeMath::safe_div(a, b);
if (!quotient) {
    return ExecutionResult::DIVIDE_BY_ZERO;
}
```

### Input Validation

All external inputs are validated:

```cpp
// Validate bytecode before execution
bool VirtualMachine::validate_bytecode(const std::vector<uint8_t>& bytecode) const {
    // Check size limits
    if (bytecode.empty() || bytecode.size() > MAX_BYTECODE_SIZE) {
        return false;
    }

    // Validate opcodes
    for (size_t i = 0; i < bytecode.size(); ) {
        OpCode op = static_cast<OpCode>(bytecode[i]);
        if (!is_valid_opcode(op)) {
            return false;
        }
        i += get_opcode_size(op);
    }

    return true;
}

// Validate contract call data
bool Contract::validate_call_data(const std::vector<uint8_t>& data) const {
    // Check size (prevent DOS)
    if (data.size() > MAX_CALL_DATA_SIZE) {
        return false;
    }

    // Additional validation...
    return true;
}
```

### Memory Safety

All memory operations are bounds-checked:

```cpp
// Write to memory with bounds checking
bool VirtualMachine::write_memory(size_t offset, const std::vector<uint8_t>& data) {
    // Check for buffer overflow
    if (offset + data.size() > max_memory_size_) {
        return false;
    }

    // Expand memory if needed
    if (offset + data.size() > memory_.size()) {
        memory_.resize(offset + data.size(), 0);
    }

    // Safe copy
    std::copy(data.begin(), data.end(), memory_.begin() + offset);
    return true;
}

// Read from memory with validation
std::optional<std::vector<uint8_t>> VirtualMachine::read_memory(
    size_t offset, size_t length) const {

    // Validate bounds
    if (offset + length > memory_.size()) {
        return std::nullopt;
    }

    // Safe read
    return std::vector<uint8_t>(
        memory_.begin() + offset,
        memory_.begin() + offset + length
    );
}
```

### Stack Safety

Stack operations prevent overflow/underflow:

```cpp
// Push with overflow check
bool VirtualMachine::push(uint64_t value) {
    if (stack_.size() >= max_stack_size_) {
        return false;  // Stack overflow
    }
    stack_.push(value);
    return true;
}

// Pop with underflow check
std::optional<uint64_t> VirtualMachine::pop() {
    if (stack_.empty()) {
        return std::nullopt;  // Stack underflow
    }
    uint64_t value = stack_.top();
    stack_.pop();
    return value;
}
```

## Usage

### Deploying a Contract

```cpp
#include <intcoin/contracts/contract.h>

using namespace intcoin::contracts;

// Prepare deployment
ContractDeployment deployment;
deployment.deployer = deployer_address;
deployment.bytecode = compiled_bytecode;
deployment.constructor_args = constructor_data;
deployment.value = 0;
deployment.gas_limit = 1000000;
deployment.gas_price = 1;

// Validate before deployment
if (!deployment.validate()) {
    std::cerr << "Invalid deployment" << std::endl;
    return;
}

// Check bytecode safety
if (!deployment.is_safe_bytecode()) {
    std::cerr << "Unsafe bytecode detected" << std::endl;
    return;
}

// Deploy contract
ContractManager manager;
auto contract_address = manager.deploy_contract(deployment, current_block);

if (contract_address) {
    std::cout << "Contract deployed at: "
              << contract_address->to_string() << std::endl;
}
```

### Calling a Contract

```cpp
// Prepare contract call
ContractCall call;
call.from = caller_address;
call.to = contract_address;
call.data = function_call_data;
call.value = 0;
call.gas_limit = 100000;
call.gas_price = 1;

// Validate call
if (!call.validate()) {
    std::cerr << "Invalid call" << std::endl;
    return;
}

// Execute call with safety checks
std::vector<uint8_t> return_data;
uint64_t gas_used;

ExecutionResult result = manager.call_contract(call, return_data, gas_used);

if (result == ExecutionResult::SUCCESS) {
    std::cout << "Call succeeded" << std::endl;
    std::cout << "Gas used: " << gas_used << std::endl;
    std::cout << "Return data size: " << return_data.size() << std::endl;
} else {
    std::cerr << "Call failed: " << static_cast<int>(result) << std::endl;
}
```

### Creating a Token Contract

```cpp
// Create ERC20-compatible token
Hash256 owner = /* owner address */;
uint64_t initial_supply = 1000000 * COIN;  // 1 million tokens

TokenContract token(owner, initial_supply);

// Transfer tokens with overflow protection
bool success = token.transfer(recipient, 100 * COIN);
if (success) {
    std::cout << "Transfer successful" << std::endl;
} else {
    std::cout << "Transfer failed (insufficient balance or overflow)" << std::endl;
}

// Check balance
uint64_t balance = token.balance_of(recipient);
std::cout << "Balance: " << balance << std::endl;
```

### Security Analysis

```cpp
// Analyze contract bytecode for vulnerabilities
auto report = SecurityAnalyzer::analyze(bytecode);

if (report.is_safe()) {
    std::cout << "Contract passed security analysis" << std::endl;
} else {
    std::cerr << "Security issues detected:" << std::endl;

    if (report.has_integer_overflow_risk) {
        std::cerr << "  - Integer overflow risk" << std::endl;
    }
    if (report.has_reentrancy_risk) {
        std::cerr << "  - Reentrancy vulnerability" << std::endl;
    }
    if (report.has_unbounded_loops) {
        std::cerr << "  - Unbounded loops detected" << std::endl;
    }

    for (const auto& warning : report.warnings) {
        std::cerr << "  - " << warning << std::endl;
    }
}
```

## Opcodes

### Arithmetic (with overflow protection)

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x01 | ADD | Safe addition (a + b) | 3 |
| 0x02 | SUB | Safe subtraction (a - b) | 3 |
| 0x03 | MUL | Safe multiplication (a * b) | 5 |
| 0x04 | DIV | Safe division (a / b) | 5 |
| 0x05 | MOD | Safe modulo (a % b) | 5 |

### Comparison

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x10 | LT | Less than (a < b) | 3 |
| 0x11 | GT | Greater than (a > b) | 3 |
| 0x12 | EQ | Equal (a == b) | 3 |

### Stack

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x30 | PUSH | Push value to stack | 3 |
| 0x31 | POP | Pop value from stack | 2 |
| 0x32 | DUP | Duplicate stack top | 3 |
| 0x33 | SWAP | Swap top two items | 3 |

### Storage

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x40 | SLOAD | Load from storage | 200 |
| 0x41 | SSTORE | Store to storage | 20000/5000 |

### Control Flow

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x50 | JUMP | Unconditional jump | 8 |
| 0x51 | JUMPI | Conditional jump | 10 |
| 0x52 | JUMPDEST | Jump destination | 1 |
| 0x53 | RETURN | Return from contract | 0 |
| 0x54 | REVERT | Revert state changes | 0 |
| 0x55 | STOP | Stop execution | 0 |

### Blockchain Access

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x60 | ADDRESS | Get contract address | 2 |
| 0x61 | BALANCE | Get balance | 400 |
| 0x62 | CALLER | Get caller address | 2 |
| 0x63 | CALLVALUE | Get call value | 2 |
| 0x64 | BLOCKNUMBER | Get block number | 2 |
| 0x65 | TIMESTAMP | Get timestamp | 2 |

### Cryptography

| Opcode | Name | Description | Gas |
|--------|------|-------------|-----|
| 0x70 | SHA256 | SHA-256 hash | 60 |
| 0x71 | DILITHIUM_VERIFY | Verify Dilithium signature | 3000 |

## Gas Costs

### Base Operations
- Zero operation: 0 gas
- Base operation: 2 gas
- Very low operation: 3 gas
- Low operation: 5 gas
- Mid operation: 8 gas
- High operation: 10 gas

### Storage Operations
- SLOAD: 200 gas
- SSTORE (new slot): 20,000 gas
- SSTORE (modify): 5,000 gas
- SSTORE (delete): 5,000 gas (15,000 refund)

### Contract Operations
- CALL: 700 gas base + value transfer costs
- CREATE: 32,000 gas + deployment costs

### Memory Expansion
- First 724 bytes: 3 gas per byte
- After 724 bytes: Quadratic cost increase

## Security Best Practices

### 1. Use SafeMath for Arithmetic

```cpp
// ❌ UNSAFE: Can overflow
uint64_t result = a + b;

// ✅ SAFE: Checked arithmetic
auto result = SafeMath::safe_add(a, b);
if (!result) {
    return ExecutionResult::INTEGER_OVERFLOW;
}
```

### 2. Validate All Inputs

```cpp
// ✅ Always validate external inputs
bool Contract::execute(...) {
    // Validate call data
    if (!validate_call_data(call_data)) {
        return ExecutionResult::INVALID_INPUT;
    }

    // Validate addresses
    if (!validate_address(caller)) {
        return ExecutionResult::INVALID_INPUT;
    }

    // Validate amounts
    if (!validate_amount(value)) {
        return ExecutionResult::INVALID_INPUT;
    }

    // Proceed with execution...
}
```

### 3. Check Array Bounds

```cpp
// ✅ Always check bounds before accessing
std::optional<uint8_t> read_byte(size_t index) {
    if (index >= data.size()) {
        return std::nullopt;  // Out of bounds
    }
    return data[index];
}
```

### 4. Limit Resource Usage

```cpp
// ✅ Enforce limits to prevent DOS
static constexpr size_t MAX_STACK_SIZE = 1024;
static constexpr size_t MAX_MEMORY_SIZE = 1024 * 1024;  // 1MB
static constexpr size_t MAX_STORAGE_SIZE = 1024 * 1024;  // 1MB

if (stack_.size() >= MAX_STACK_SIZE) {
    return ExecutionResult::STACK_OVERFLOW;
}
```

### 5. Gas Metering

```cpp
// ✅ Always meter gas to prevent infinite loops
bool consume_gas(uint64_t amount) {
    if (gas_used_ + amount > gas_limit_) {
        return false;  // Out of gas
    }
    gas_used_ += amount;
    return true;
}
```

## Common Vulnerabilities and Mitigations

### Integer Overflow/Underflow

**Vulnerability:**
```cpp
// ❌ Unsafe
uint64_t balance = balances[account];
balance += amount;  // Can overflow!
```

**Mitigation:**
```cpp
// ✅ Safe
auto new_balance = SafeMath::safe_add(balance, amount);
if (!new_balance) {
    return false;  // Overflow prevented
}
balance = *new_balance;
```

### Reentrancy

**Vulnerability:**
```cpp
// ❌ Checks-Effects-Interactions violated
function withdraw(uint amount) {
    require(balance[msg.sender] >= amount);
    msg.sender.call.value(amount)();  // External call before state update
    balance[msg.sender] -= amount;    // Attacker can reenter!
}
```

**Mitigation:**
```cpp
// ✅ Checks-Effects-Interactions pattern
function withdraw(uint amount) {
    require(balance[msg.sender] >= amount);
    balance[msg.sender] -= amount;    // Update state first
    msg.sender.call.value(amount)();  // Then external call
}

// ✅ Reentrancy guard
bool locked = false;
function withdraw(uint amount) {
    require(!locked);
    locked = true;
    // ... withdrawal logic ...
    locked = false;
}
```

### Buffer Overflow

**Vulnerability:**
```cpp
// ❌ No bounds checking
void write(size_t offset, uint8_t value) {
    memory[offset] = value;  // Can overflow!
}
```

**Mitigation:**
```cpp
// ✅ Bounds checked
bool write(size_t offset, uint8_t value) {
    if (offset >= memory.size()) {
        return false;  // Out of bounds
    }
    memory[offset] = value;
    return true;
}
```

## Testing

```bash
# Run VM tests
./test_vm

# Run contract tests
./test_contracts

# Run security analyzer tests
./test_security_analyzer

# Integration tests
./test_contract_integration
```

## Future Enhancements

1. **Advanced Features**
   - Solidity compiler support
   - WebAssembly (WASM) VM
   - Formal verification tools
   - Enhanced debugger

2. **Performance**
   - JIT compilation
   - Bytecode optimization
   - Parallel execution
   - State compression

3. **Security**
   - Formal verification
   - Automated security audits
   - Fuzzing tools
   - Symbolic execution

4. **Developer Tools**
   - IDE integration
   - Visual debugger
   - Profiling tools
   - Gas optimizer

## References

- [Ethereum Yellow Paper](https://ethereum.github.io/yellowpaper/paper.pdf)
- [EVM Opcodes](https://ethereum.org/en/developers/docs/evm/opcodes/)
- [Smart Contract Security](https://consensys.github.io/smart-contract-best-practices/)
- [SafeMath Library](https://docs.openzeppelin.com/contracts/2.x/api/math)

## License

MIT License - See LICENSE file for details

## Authors

INTcoin Core Development Team
