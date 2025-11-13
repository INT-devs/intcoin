# INTcoin Mainnet Genesis Block

**Status**: Mining in Progress
**Target Launch**: January 1, 2026
**Last Updated**: November 13, 2025

## Overview

The INTcoin mainnet genesis block represents the foundation of the quantum-resistant blockchain. This document specifies the exact parameters and process for creating the official mainnet genesis block.

## Genesis Block Parameters

### Header Fields

```
Version:           1
Timestamp:         1735689600 (January 1, 2025 00:00:00 UTC)
Previous Hash:     0000000000000000000000000000000000000000000000000000000000000000
Difficulty (bits): 0x1d00ffff
Nonce:             [MINING IN PROGRESS]
```

### Coinbase Transaction

**Message**: `"The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins"`

This message commemorates the beginning of the post-quantum cryptography era in cryptocurrency, marking January 1, 2025 as the symbolic start of quantum-resistant blockchain technology.

**Initial Reward**: 50 INT

### Merkle Root

```
b35d55ec6717339d4d8c8a8b4c44f1b67ea87595c005a8f773c1091861a4c33c
```

The merkle root is derived from the hash of the single coinbase transaction in the genesis block.

## Mining Process

### Tool

The genesis block is being mined using the `mine_genesis` tool located in [tools/mine_genesis.cpp](../tools/mine_genesis.cpp).

### Compilation

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target mine_genesis
```

### Execution

```bash
./build/tools/mine_genesis
```

### Expected Performance

- **Hash Rate**: ~2.5 million hashes/second (on Apple Silicon M-series)
- **Difficulty**: 0x1d00ffff (Bitcoin's initial difficulty)
- **Expected Time**: Variable (depends on luck and hardware)

### Mining Status

Currently mining genesis block with the following configuration:

- **Start Time**: November 13, 2025
- **Current Progress**: ~271 million hashes attempted
- **Hash Rate**: 2.49 MH/s (sustained)
- **Status**: In Progress

## Verification

Once mining is complete, the genesis block will be verified using:

1. **Proof of Work**: Block hash must be less than or equal to the target derived from difficulty bits
2. **Merkle Root**: Must match the calculated merkle root of transactions
3. **Coinbase**: First transaction must be a valid coinbase with correct reward
4. **Timestamp**: Must be January 1, 2025 00:00:00 UTC (1735689600)

## Implementation

### Current Code (block.cpp)

```cpp
Block GenesisBlock::create_mainnet() {
    const std::string message = "The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins";
    const uint64_t timestamp = 1735689600;  // January 1, 2025 00:00:00 UTC
    const uint64_t nonce = 0;  // Will be updated with mined nonce
    const uint32_t bits = 0x1d00ffff;  // Initial difficulty

    return create_genesis(message, timestamp, nonce, bits);
}
```

### After Mining Completion

Once a valid nonce is found, the code will be updated to:

```cpp
Block GenesisBlock::create_mainnet() {
    const std::string message = "The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins";
    const uint64_t timestamp = 1735689600;  // January 1, 2025 00:00:00 UTC
    const uint64_t nonce = [MINED_NONCE]ULL;  // Mined nonce
    const uint32_t bits = 0x1d00ffff;  // Initial difficulty

    return create_genesis(message, timestamp, nonce, bits);
}
```

## Quantum-Resistant Features

The genesis block implements post-quantum cryptography from the very first block:

- **Signatures**: CRYSTALS-Dilithium5 (ML-DSA-87, NIST FIPS 204)
- **Key Exchange**: CRYSTALS-Kyber1024 (ML-KEM-1024, NIST FIPS 203)
- **Hashing**: SHA3-256 (NIST FIPS 202)
- **Proof of Work**: SHA-256 (for RandomX compatibility)

## Network Parameters

### Mainnet

- **Magic Bytes**: `0x494E5443` ("INTC")
- **Default Port**: 8333
- **RPC Port**: 8332
- **Genesis Hash**: [To be determined after mining]

### Testnet

- **Magic Bytes**: `0x54494E54` ("TINT")
- **Default Port**: 18333
- **RPC Port**: 18332
- **Genesis Hash**: [To be determined]

## Checkpoints

The genesis block will serve as checkpoint 0:

```cpp
checkpoint_system_.add_checkpoint(0, genesis_hash);
```

All subsequent blocks must build upon this genesis block.

## Emission Schedule

Starting from the genesis block, INTcoin follows a halving-based emission schedule:

- **Initial Reward**: 105,113,636 INT per block
- **Block Time**: 2 minutes (120 seconds)
- **Halving Interval**: 1,051,200 blocks (~4 years)
- **Max Supply**: 221 Trillion INT
- **Blocks Per Year**: 262,800

## Historical Context

The genesis block message references the beginning of the quantum-resistant cryptocurrency era, symbolically dated January 1, 2025. This mirrors Bitcoin's tradition of embedding a newspaper headline, but marks a new chapter in blockchain technology—one that addresses the quantum computing threat.

## Next Steps

1. **Complete Mining**: Finish mining the mainnet genesis block
2. **Update Code**: Hard-code the mined nonce in `src/core/block.cpp`
3. **Verify Block**: Run comprehensive validation tests
4. **Document Hash**: Record the final genesis block hash
5. **Prepare Launch**: Set up seed nodes and prepare for mainnet launch

## References

- **Genesis Miner**: [tools/mine_genesis.cpp](../tools/mine_genesis.cpp)
- **Block Implementation**: [src/core/block.cpp](../src/core/block.cpp)
- **Block Header**: [include/intcoin/block.h](../include/intcoin/block.h)
- **Consensus Parameters**: [include/intcoin/primitives.h](../include/intcoin/primitives.h)

---

**Lead Developer**: Maddison Lane
**Documentation**: Claude AI (Anthropic)
**Date**: November 13, 2025
