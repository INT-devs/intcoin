# INTcoin Smart Contract Specification

**Version**: 1.0
**Date**: January 9, 2026
**Status**: Draft

---

## Overview

INTcoin Smart Contracts (IntSC) provide a quantum-resistant, Turing-complete virtual machine for decentralized applications. The IntSC VM is EVM-compatible with post-quantum cryptography enhancements.

## Design Principles

1. **Quantum-Resistant**: All cryptographic operations use PQC algorithms (Dilithium3, Kyber768)
2. **EVM-Compatible**: Solidity contracts can be compiled with minimal changes
3. **Gas Efficient**: Optimized for post-quantum signature verification
4. **Deterministic**: No undefined behavior across nodes
5. **Security-First**: Built-in protection against common vulnerabilities

---

## Virtual Machine Architecture

### Execution Model

The IntSC VM is a stack-based virtual machine similar to the EVM:

- **Stack**: 256-bit words, maximum depth 1024
- **Memory**: Byte-addressable linear memory, expandable
- **Storage**: Persistent key-value store (SHA3-256 keys)
- **Code**: Immutable bytecode stored on-chain

### Address Format

Contract addresses use Bech32 format:
- Prefix: `int1` (contracts), `int` (accounts)
- Example: `int1qpzry9x8gf2tvdw0s3jn54khce6mua7l5tpq7g`

### Account Model

```cpp
struct Account {
    uint64_t balance;        // Balance in satINT
    uint64_t nonce;          // Transaction counter
    bytes32 code_hash;       // Hash of contract code (empty for EOA)
    bytes32 storage_root;    // Merkle root of storage trie
};
```

---

## Opcodes

### Standard EVM Opcodes

All standard EVM opcodes are supported with identical semantics:

| Opcode | Mnemonic | Gas | Description |
|--------|----------|-----|-------------|
| 0x00 | STOP | 0 | Halt execution |
| 0x01 | ADD | 3 | Addition |
| 0x02 | MUL | 5 | Multiplication |
| 0x03 | SUB | 3 | Subtraction |
| 0x04 | DIV | 5 | Integer division |
| 0x05 | SDIV | 5 | Signed division |
| 0x06 | MOD | 5 | Modulo |
| 0x07 | SMOD | 5 | Signed modulo |
| 0x08 | ADDMOD | 8 | Modular addition |
| 0x09 | MULMOD | 8 | Modular multiplication |
| 0x0a | EXP | 10 | Exponentiation |
| 0x10 | LT | 3 | Less than |
| 0x11 | GT | 3 | Greater than |
| 0x12 | SLT | 3 | Signed less than |
| 0x13 | SGT | 3 | Signed greater than |
| 0x14 | EQ | 3 | Equality |
| 0x15 | ISZERO | 3 | Is zero |
| 0x16 | AND | 3 | Bitwise AND |
| 0x17 | OR | 3 | Bitwise OR |
| 0x18 | XOR | 3 | Bitwise XOR |
| 0x19 | NOT | 3 | Bitwise NOT |
| 0x20 | SHA3 | 30 | SHA3-256 hash |
| 0x30 | ADDRESS | 2 | Current contract address |
| 0x31 | BALANCE | 400 | Account balance |
| 0x32 | ORIGIN | 2 | Transaction origin |
| 0x33 | CALLER | 2 | Message sender |
| 0x34 | CALLVALUE | 2 | Message value |
| 0x35 | CALLDATALOAD | 3 | Load call data |
| 0x36 | CALLDATASIZE | 2 | Size of call data |
| 0x37 | CALLDATACOPY | 3 | Copy call data |
| 0x38 | CODESIZE | 2 | Size of code |
| 0x39 | CODECOPY | 3 | Copy code |
| 0x50 | POP | 2 | Remove stack item |
| 0x51 | MLOAD | 3 | Load word from memory |
| 0x52 | MSTORE | 3 | Store word to memory |
| 0x53 | MSTORE8 | 3 | Store byte to memory |
| 0x54 | SLOAD | 800 | Load from storage |
| 0x55 | SSTORE | 20000 | Store to storage |
| 0x56 | JUMP | 8 | Unconditional jump |
| 0x57 | JUMPI | 10 | Conditional jump |
| 0x58 | PC | 2 | Program counter |
| 0x59 | MSIZE | 2 | Size of memory |
| 0x5a | GAS | 2 | Remaining gas |
| 0x5b | JUMPDEST | 1 | Jump destination |
| 0x60-0x7f | PUSH1-PUSH32 | 3 | Push N bytes |
| 0x80-0x8f | DUP1-DUP16 | 3 | Duplicate stack item |
| 0x90-0x9f | SWAP1-SWAP16 | 3 | Swap stack items |
| 0xf0 | CREATE | 32000 | Create contract |
| 0xf1 | CALL | 700 | Call contract |
| 0xf2 | CALLCODE | 700 | Call with code |
| 0xf3 | RETURN | 0 | Return from call |
| 0xf4 | DELEGATECALL | 700 | Delegate call |
| 0xf5 | CREATE2 | 32000 | Create contract (deterministic) |
| 0xfa | STATICCALL | 700 | Static call |
| 0xfd | REVERT | 0 | Revert state changes |
| 0xfe | INVALID | 0 | Invalid opcode |
| 0xff | SELFDESTRUCT | 5000 | Destroy contract |

### Post-Quantum Cryptography Opcodes

New opcodes for quantum-resistant operations:

| Opcode | Mnemonic | Gas | Description |
|--------|----------|-----|-------------|
| 0xa0 | DILITHIUM_VERIFY | 50000 | Verify Dilithium3 signature |
| 0xa1 | KYBER_ENCAP | 30000 | Kyber768 key encapsulation |
| 0xa2 | KYBER_DECAP | 30000 | Kyber768 key decapsulation |
| 0xa3 | PQC_PUBKEY | 100 | Get PQC public key |

#### DILITHIUM_VERIFY (0xa0)

Verifies a Dilithium3 signature.

**Stack Input**:
- `message_offset` (32 bytes) - Memory offset of message
- `message_length` (32 bytes) - Length of message
- `signature_offset` (32 bytes) - Memory offset of signature
- `pubkey_offset` (32 bytes) - Memory offset of public key

**Stack Output**:
- `result` (1 if valid, 0 if invalid)

**Gas**: 50,000

---

## Gas Costs

### Standard Operations

Same as EVM with minor adjustments:

- Arithmetic: 3-10 gas
- Memory expansion: 3 gas per word + quadratic component
- Storage: 20,000 gas (new), 5,000 gas (update)
- Transaction base: 21,000 gas

### Post-Quantum Operations

Higher costs due to computational complexity:

- **DILITHIUM_VERIFY**: 50,000 gas
- **KYBER_ENCAP**: 30,000 gas
- **KYBER_DECAP**: 30,000 gas
- **SHA3**: 60 gas (cheaper than KECCAK256)

### Contract Deployment

- Base: 32,000 gas
- Code storage: 200 gas per byte
- Contract creation: 32,000 gas

---

## Storage Model

### Key-Value Store

Contracts have persistent storage implemented as a key-value store:

```cpp
mapping(bytes32 => bytes32) storage;
```

- Keys: SHA3-256 hash (32 bytes)
- Values: 32-byte words
- Cost: 20,000 gas (SSTORE new), 5,000 gas (SSTORE update)

### Storage Trie

Storage is organized in a Merkle Patricia Trie:

- Root hash stored in account state
- Allows efficient proofs of storage values
- Supports light clients

---

## Transaction Types

### Contract Deployment (Type 2)

```cpp
struct ContractDeploymentTx {
    uint8_t type = 2;
    PublicKey from;
    uint64_t value;
    uint64_t gas_limit;
    uint64_t gas_price;
    bytes bytecode;
    bytes constructor_args;
    uint64_t nonce;
    Signature signature;
};
```

### Contract Call (Type 3)

```cpp
struct ContractCallTx {
    uint8_t type = 3;
    PublicKey from;
    Address to;
    uint64_t value;
    uint64_t gas_limit;
    uint64_t gas_price;
    bytes data;
    uint64_t nonce;
    Signature signature;
};
```

---

## ABI (Application Binary Interface)

### Function Selector

First 4 bytes of SHA3-256 hash of function signature:

```cpp
bytes4 selector = SHA3("transfer(address,uint256)")[:4];
```

### Encoding Rules

- Fixed types: Padded to 32 bytes
- Dynamic types: Offset + length + data
- Arrays: Length + elements
- Strings: Length + UTF-8 bytes

### Example

```solidity
function transfer(address to, uint256 amount) external returns (bool);
```

Call data:
```
0xa9059cbb                                         // selector
0000000000000000000000001234567890abcdef1234567890abcdef12345678  // to (padded)
00000000000000000000000000000000000000000000000000000000000003e8  // amount (1000)
```

---

## Solidity Compatibility

### Supported Features

✅ All Solidity language features (v0.8.x)
✅ Events and logs
✅ Modifiers
✅ Inheritance
✅ Libraries
✅ Interfaces
✅ Abstract contracts
✅ Enums and structs

### Differences from Ethereum

1. **Address Format**: Bech32 instead of hex (internal conversion)
2. **Cryptography**: `ecrecover` replaced with `dilithium_verify`
3. **Gas Costs**: Different for PQC operations
4. **Block Time**: ~5 minutes (vs ~12 seconds)

### Migration Guide

Minimal changes needed:

```solidity
// Ethereum
function verify(bytes32 hash, bytes memory sig) public view returns (address) {
    return ecrecover(hash, v, r, s);
}

// INTcoin
function verify(bytes32 hash, bytes memory sig, bytes memory pubkey) public view returns (bool) {
    return dilithium_verify(hash, sig, pubkey);
}
```

---

## Security Considerations

### Reentrancy Protection

All external calls use the checks-effects-interactions pattern:

```solidity
function withdraw(uint256 amount) external {
    require(balances[msg.sender] >= amount);  // Checks
    balances[msg.sender] -= amount;           // Effects
    payable(msg.sender).transfer(amount);     // Interactions
}
```

### Integer Overflow

Solidity 0.8+ has built-in overflow checks:

```solidity
uint256 a = type(uint256).max;
uint256 b = a + 1;  // Reverts with overflow error
```

### Access Control

Use OpenZeppelin-style access control:

```solidity
import "@openzeppelin/contracts/access/Ownable.sol";

contract MyContract is Ownable {
    function sensitiveOperation() external onlyOwner {
        // ...
    }
}
```

---

## Standard Library

### Precompiled Contracts

| Address | Name | Description |
|---------|------|-------------|
| int1q0000000000000000000000000000000000000001 | ECRecover | ECDSA recovery (legacy) |
| int1q0000000000000000000000000000000000000002 | SHA256 | SHA-256 hash |
| int1q0000000000000000000000000000000000000003 | RIPEMD160 | RIPEMD-160 hash |
| int1q0000000000000000000000000000000000000004 | Identity | Identity function |
| int1q0000000000000000000000000000000000000005 | ModExp | Modular exponentiation |
| int1q0000000000000000000000000000000000000010 | Dilithium | Dilithium3 verify |
| int1q0000000000000000000000000000000000000011 | Kyber | Kyber768 operations |

---

## Development Tools

### Compiler

IntSC uses standard Solidity compiler with custom target:

```bash
solc --target intsc --optimize MyContract.sol
```

### Testing

Use Hardhat/Truffle with IntSC provider:

```javascript
module.exports = {
  networks: {
    intcoin: {
      url: "http://localhost:8332",
      chainId: 1337
    }
  }
};
```

### Deployment

Deploy via intcoin-cli:

```bash
intcoin-cli deploycontract <bytecode> <constructor_args> --gas-limit 1000000
```

---

## Example Contracts

### Simple Storage

```solidity
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

### ERC-20 Token

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
        require(balanceOf[msg.sender] >= _value);
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
        require(balanceOf[_from] >= _value);
        require(allowance[_from][msg.sender] >= _value);
        balanceOf[_from] -= _value;
        balanceOf[_to] += _value;
        allowance[_from][msg.sender] -= _value;
        emit Transfer(_from, _to, _value);
        return true;
    }
}
```

---

## Roadmap

### Phase 1: Core VM (v1.4.0)
- ✅ VM specification
- [ ] Basic opcode implementation
- [ ] Stack and memory management
- [ ] Storage layer

### Phase 2: Contract Deployment (v1.4.0)
- [ ] Transaction types
- [ ] Contract deployment
- [ ] Contract execution
- [ ] Event logs

### Phase 3: Tooling (v1.5.0)
- [ ] Solidity compiler integration
- [ ] Hardhat plugin
- [ ] Block explorer
- [ ] Debugger

### Phase 4: Optimization (v1.6.0)
- [ ] JIT compilation
- [ ] Gas optimizations
- [ ] Parallel execution
- [ ] State pruning

---

## Conclusion

The IntSC smart contract platform brings Turing-complete programmability to INTcoin with quantum-resistant security. EVM compatibility ensures easy migration of existing Solidity contracts, while new PQC opcodes enable post-quantum applications.

**Next Steps**:
1. Review and finalize specification
2. Implement core VM
3. Add contract transaction support
4. Integrate Solidity compiler
5. Deploy example contracts
6. Launch testnet

---

**Document Version**: 1.0
**Last Updated**: January 9, 2026
**Author**: INTcoin Core Development Team
