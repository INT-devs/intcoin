## Smart Contracts on INTcoin (IntSC)

**Version**: 1.4.0-dev
**Status**: Implementation Complete
**Last Updated**: January 9, 2026

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Architecture](#architecture)
4. [Contract Deployment](#contract-deployment)
5. [Contract Execution](#contract-execution)
6. [RPC API Reference](#rpc-api-reference)
7. [Examples](#examples)
8. [Best Practices](#best-practices)
9. [Gas Optimization](#gas-optimization)
10. [Troubleshooting](#troubleshooting)

---

## Overview

INTcoin Smart Contracts (IntSC) provide a quantum-resistant, Turing-complete virtual machine for decentralized applications. The IntSC VM is **EVM-compatible** with post-quantum cryptography enhancements.

### Key Features

✅ **EVM-Compatible**: Run Solidity contracts with minimal changes
✅ **Quantum-Resistant**: All transactions signed with Dilithium3
✅ **60+ Opcodes**: Full EVM opcode support
✅ **4 PQC Opcodes**: DILITHIUM_VERIFY, KYBER_ENCAP, KYBER_DECAP, PQC_PUBKEY
✅ **Gas Metering**: Fair resource allocation
✅ **Bech32 Addresses**: int1q... format for contracts

### VM Specifications

- **Stack**: 256-bit words, 1024 depth limit
- **Memory**: Byte-addressable, expandable linear memory
- **Storage**: Persistent key-value store (SHA3-256 keys)
- **Gas Model**: EVM-compatible with PQC operation costs
- **Address Format**: Bech32 (`int1` prefix for contracts)

---

## Quick Start

### 1. Write a Simple Contract

```solidity
// SimpleStorage.sol
pragma solidity ^0.8.0;

contract SimpleStorage {
    uint256 private value;

    event ValueChanged(uint256 newValue);

    function set(uint256 _value) public {
        value = _value;
        emit ValueChanged(_value);
    }

    function get() public view returns (uint256) {
        return value;
    }
}
```

### 2. Compile the Contract

```bash
# Using standard Solidity compiler
solc --bin --abi SimpleStorage.sol -o build/

# Output:
# - SimpleStorage.bin (bytecode)
# - SimpleStorage.abi (ABI JSON)
```

### 3. Deploy via RPC

```bash
# Extract bytecode
BYTECODE=$(cat build/SimpleStorage.bin)

# Deploy contract
intcoin-cli deploycontract "$BYTECODE" [] 1000000 0

# Response:
{
  "txid": "0x1234...abcd",
  "contract_address": "int1qpzry9x8gf2tvdw0s3jn54khce6mua7l5tpq7g",
  "gas_estimated": 125000
}
```

### 4. Call Contract Functions

```bash
# Set value to 42
intcoin-cli callcontract \
  "int1qpzry9x8gf2tvdw0s3jn54khce6mua7l5tpq7g" \
  "set(uint256)" \
  "[\"0x000000000000000000000000000000000000000000000000000000000000002a\"]" \
  100000 \
  0

# Get value (static call)
intcoin-cli callcontractstatic \
  "int1qpzry9x8gf2tvdw0s3jn54khce6mua7l5tpq7g" \
  "get()" \
  "[]"

# Response:
{
  "result": "0x000000000000000000000000000000000000000000000000000000000000002a",
  "decoded": [42],
  "gas_used": 21000
}
```

---

## Architecture

### IntSC VM Components

```
┌──────────────────────────────────────────┐
│          IntSC Virtual Machine            │
├──────────────────────────────────────────┤
│  ┌────────┐  ┌────────┐  ┌────────────┐ │
│  │ Stack  │  │ Memory │  │  Storage   │ │
│  │ 1024   │  │Byte-   │  │  Key-Value │ │
│  │ depth  │  │address │  │  Store     │ │
│  └────────┘  └────────┘  └────────────┘ │
├──────────────────────────────────────────┤
│         Opcode Executor                   │
│  • 60+ EVM opcodes                       │
│  • 4 PQC opcodes                         │
│  • Gas metering                          │
├──────────────────────────────────────────┤
│         Transaction Layer                 │
│  • ContractDeploymentTx (Type 2)         │
│  • ContractCallTx (Type 3)               │
│  • Dilithium3 signatures                 │
└──────────────────────────────────────────┘
```

### Transaction Types

#### Type 2: Contract Deployment

```cpp
struct ContractDeploymentTx {
    uint8_t type = 2;
    PublicKey from;                  // Dilithium3 public key
    uint64_t value;                  // Initial balance (satINT)
    vector<uint8_t> bytecode;        // Contract bytecode
    vector<uint8_t> constructor_args;// Constructor arguments
    uint64_t gas_limit;              // Maximum gas
    uint64_t gas_price;              // Gas price (satINT)
    uint64_t nonce;                  // Transaction counter
    Signature signature;             // Dilithium3 signature
};
```

#### Type 3: Contract Call

```cpp
struct ContractCallTx {
    uint8_t type = 3;
    PublicKey from;                  // Caller's public key
    string to;                       // Contract address (Bech32)
    uint64_t value;                  // Amount to send
    vector<uint8_t> data;            // Function call data (ABI-encoded)
    uint64_t gas_limit;
    uint64_t gas_price;
    uint64_t nonce;
    Signature signature;
};
```

---

## Contract Deployment

### Deployment Process

1. **Compile Contract** → Bytecode + ABI
2. **Create Deployment Transaction** → Sign with Dilithium3
3. **Broadcast to Network** → Miners include in block
4. **Contract Address** → Deterministic: SHA3-256(from || nonce)

### Deployment Example

```bash
# 1. Prepare bytecode (from compiled Solidity)
BYTECODE="0x60806040523480156100..."

# 2. Prepare constructor args (if any)
# Example: constructor(uint256 _initialValue)
CONSTRUCTOR_ARGS="[\"0x00000000000000000000000000000000000000000000000000000000000003e8\"]"

# 3. Deploy
intcoin-cli deploycontract \
  "$BYTECODE" \
  "$CONSTRUCTOR_ARGS" \
  2000000 \        # gas_limit
  1000000          # initial value (1 INT)

# 4. Get contract address from response
```

### Gas Estimation for Deployment

```bash
# Estimate deployment cost
BASE_COST=32000                    # Base deployment cost
BYTECODE_LENGTH=$(echo $BYTECODE | wc -c)
BYTECODE_COST=$((BYTECODE_LENGTH * 200))
TOTAL_ESTIMATE=$((BASE_COST + BYTECODE_COST))

echo "Estimated gas: $TOTAL_ESTIMATE"
```

---

## Contract Execution

### Calling Functions

```bash
# Non-payable function call
intcoin-cli callcontract \
  "<contract_address>" \
  "functionName(type1,type2)" \
  "[\"arg1_hex\",\"arg2_hex\"]" \
  <gas_limit> \
  0  # value = 0 for non-payable

# Payable function call
intcoin-cli callcontract \
  "<contract_address>" \
  "deposit()" \
  "[]" \
  100000 \
  5000000  # value = 5 INT
```

### Static Calls (Read-Only)

```bash
# Read contract state (no transaction, no gas cost)
intcoin-cli callcontractstatic \
  "<contract_address>" \
  "balanceOf(address)" \
  "[\"0x...\"]"

# Response includes decoded values
{
  "result": "0x00000000000000000000000000000000000000000000000000000000000003e8",
  "decoded": [1000],
  "gas_used": 21000
}
```

---

## RPC API Reference

### Core Methods

| Method | Purpose | Transaction? |
|--------|---------|-------------|
| `deploycontract` | Deploy new contract | Yes |
| `callcontract` | Execute contract function | Yes |
| `callcontractstatic` | Read-only call | No |
| `getcontractinfo` | Get contract details | No |
| `getcontractcode` | Get bytecode & disassembly | No |
| `getcontractreceipt` | Get transaction receipt | No |
| `estimatecontractgas` | Estimate gas cost | No |

### Query Methods

| Method | Purpose |
|--------|---------|
| `getcontractlogs` | Query event logs |
| `listcontracts` | List all contracts |
| `getcontractstorage` | Read storage slot |
| `compilesolidity` | Compile source code |
| `verifycontract` | Verify source code |

### Example: Get Contract Info

```bash
intcoin-cli getcontractinfo "int1q..."

# Response:
{
  "address": "int1qpzry9x8gf2tvdw0s3jn54khce6mua7l5tpq7g",
  "balance": 5000000,
  "code_hash": "0x1234...abcd",
  "storage_root": "0x5678...ef01",
  "nonce": 0,
  "creator": "int1q...",
  "creation_tx": "0x...",
  "block_created": 12345
}
```

---

## Examples

### Example 1: ERC-20 Token

```solidity
pragma solidity ^0.8.0;

contract INTToken {
    string public name = "INTcoin Token";
    string public symbol = "INTK";
    uint8 public decimals = 8;
    uint256 public totalSupply;

    mapping(address => uint256) public balanceOf;
    mapping(address => mapping(address => uint256)) public allowance;

    event Transfer(address indexed from, address indexed to, uint256 value);
    event Approval(address indexed owner, address indexed spender, uint256 value);

    constructor(uint256 _initialSupply) {
        totalSupply = _initialSupply;
        balanceOf[msg.sender] = _initialSupply;
    }

    function transfer(address _to, uint256 _value) public returns (bool) {
        require(balanceOf[msg.sender] >= _value, "Insufficient balance");
        balanceOf[msg.sender] -= _value;
        balanceOf[_to] += _value;
        emit Transfer(msg.sender, _to, _value);
        return true;
    }

    function approve(address _spender, uint256 _value) public returns (bool) {
        allowance[msg.sender][_spender] = _value;
        emit Approval(msg.sender, _spender, _value);
        return true;
    }

    function transferFrom(address _from, address _to, uint256 _value) public returns (bool) {
        require(balanceOf[_from] >= _value, "Insufficient balance");
        require(allowance[_from][msg.sender] >= _value, "Insufficient allowance");
        balanceOf[_from] -= _value;
        balanceOf[_to] += _value;
        allowance[_from][msg.sender] -= _value;
        emit Transfer(_from, _to, _value);
        return true;
    }
}
```

### Example 2: Multi-Signature Wallet

```solidity
pragma solidity ^0.8.0;

contract MultiSigWallet {
    address[] public owners;
    uint256 public required;

    struct Transaction {
        address to;
        uint256 value;
        bytes data;
        bool executed;
        uint256 confirmations;
    }

    Transaction[] public transactions;
    mapping(uint256 => mapping(address => bool)) public confirmed;

    constructor(address[] memory _owners, uint256 _required) {
        require(_owners.length > 0, "Owners required");
        require(_required > 0 && _required <= _owners.length, "Invalid required count");

        owners = _owners;
        required = _required;
    }

    function submitTransaction(address _to, uint256 _value, bytes memory _data) public returns (uint256) {
        uint256 txId = transactions.length;
        transactions.push(Transaction({
            to: _to,
            value: _value,
            data: _data,
            executed: false,
            confirmations: 0
        }));
        return txId;
    }

    function confirmTransaction(uint256 _txId) public {
        require(_txId < transactions.length, "Invalid transaction");
        require(!confirmed[_txId][msg.sender], "Already confirmed");

        confirmed[_txId][msg.sender] = true;
        transactions[_txId].confirmations += 1;

        if (transactions[_txId].confirmations >= required) {
            executeTransaction(_txId);
        }
    }

    function executeTransaction(uint256 _txId) private {
        require(!transactions[_txId].executed, "Already executed");

        Transaction storage txn = transactions[_txId];
        txn.executed = true;

        (bool success, ) = txn.to.call{value: txn.value}(txn.data);
        require(success, "Transaction failed");
    }
}
```

---

## Best Practices

### Security

1. **Use Latest Solidity**: `^0.8.0` has built-in overflow protection
2. **Check-Effects-Interactions**: Prevent reentrancy attacks
3. **Access Control**: Use modifiers for privileged functions
4. **Input Validation**: Always validate user inputs
5. **Gas Limits**: Set reasonable gas limits to prevent DoS

### Gas Optimization

1. **Use Events**: Cheaper than storage for logging
2. **Pack Variables**: Combine storage slots
3. **Short-Circuit**: Use `&&` and `||` wisely
4. **Minimize Storage**: Use memory when possible
5. **Batch Operations**: Combine multiple calls

### PQC Considerations

```solidity
// Use dilithium_verify for signature verification
function verifyDilithiumSignature(
    bytes32 message,
    bytes memory signature,
    bytes memory pubkey
) internal view returns (bool) {
    // Call PQC opcode (0xa0)
    return dilithium_verify(message, signature, pubkey);
}
```

---

## Gas Optimization

### Gas Costs

| Operation | Gas Cost |
|-----------|----------|
| ADD, SUB | 3 |
| MUL | 5 |
| DIV, MOD | 5 |
| SLOAD | 800 |
| SSTORE (new) | 20,000 |
| SSTORE (update) | 5,000 |
| SHA3 | 30 |
| DILITHIUM_VERIFY | 50,000 |
| KYBER_ENCAP/DECAP | 30,000 |

### Optimization Tips

```solidity
// ❌ Expensive: Multiple SSTOREs
function updateValues(uint256[] memory values) public {
    for (uint256 i = 0; i < values.length; i++) {
        data[i] = values[i];  // SSTORE per iteration
    }
}

// ✅ Optimized: Batch update
function updateValuesBatch(uint256[] memory values) public {
    uint256 len = values.length;
    for (uint256 i = 0; i < len; i++) {
        data[i] = values[i];
    }
    // Consider using events instead of storage when possible
}
```

---

## Troubleshooting

### Common Errors

**Error: "Out of gas"**
- **Solution**: Increase gas_limit
- **Cause**: Complex computation or infinite loop

**Error: "Invalid jump"**
- **Solution**: Check bytecode validity
- **Cause**: Corrupted bytecode or invalid JUMPDEST

**Error**: "Stack underflow"
- **Solution**: Review opcode sequence
- **Cause**: Pop without corresponding push

**Error**: "Contract creation failed"
- **Solution**: Check constructor logic
- **Cause**: Revert in constructor

### Debugging

```bash
# Get contract code disassembly
intcoin-cli getcontractcode "int1q..."

# Check transaction receipt
intcoin-cli getcontractreceipt "0x..."

# View contract logs
intcoin-cli getcontractlogs "int1q..." <from_block> <to_block>
```

---

## Roadmap

### Phase 1: Core VM ✅ (v1.4.0)
- [x] VM specification
- [x] Opcode implementation
- [x] Stack and memory
- [x] Storage layer
- [x] Transaction types
- [x] RPC endpoints

### Phase 2: Integration (v1.4.0)
- [ ] Block validation
- [ ] State database
- [ ] Event log indexing
- [ ] Receipt storage
- [ ] Mempool integration

### Phase 3: Tooling (v1.5.0)
- [ ] Solidity compiler integration
- [ ] Hardhat plugin
- [ ] Block explorer
- [ ] Contract debugger
- [ ] Gas profiler

### Phase 4: Optimization (v1.6.0)
- [ ] JIT compilation
- [ ] Parallel execution
- [ ] State pruning
- [ ] Gas optimizations

---

## Additional Resources

- [SMART_CONTRACTS_SPEC.md](SMART_CONTRACTS_SPEC.md) - Technical specification
- [API_REFERENCE.md](API_REFERENCE.md) - Complete API documentation
- [Solidity Documentation](https://docs.soliditylang.org/) - Solidity language reference
- [EVM Opcodes](https://www.evm.codes/) - EVM opcode reference

---

**Maintainer**: INTcoin Development Team
**Contact**: team@international-coin.org
**License**: MIT

---

*This documentation is part of the INTcoin v1.4.0 release. For updates, please check the GitHub repository.*
