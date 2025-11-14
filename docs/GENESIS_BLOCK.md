# INTcoin Mainnet Genesis Block

**Status**: ✅ Successfully Mined
**Launch Date**: January 1, 2026
**Last Updated**: November 14, 2025

## Overview

The INTcoin mainnet genesis block represents the foundation of the quantum-resistant blockchain. This document specifies the exact parameters and process for creating the official mainnet genesis block.

## Genesis Block Parameters

### Header Fields

```
Version:           1
Timestamp:         1730851200 (November 6, 2025 00:00:00 UTC)
Previous Hash:     0000000000000000000000000000000000000000000000000000000000000000
Difficulty (bits): 0x1d00ffff
Nonce:             1233778760
Block Hash:        000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9
```

### Coinbase Transaction

**Message**: `"BBC News 06/Nov/2025 Will quantum be bigger than AI?"`

This message references the BBC News headline questioning whether quantum computing will have a greater impact than artificial intelligence, marking November 6, 2025 as the genesis of INTcoin's quantum-resistant blockchain.

**Initial Reward**: 105,113,636 INT

### Merkle Root

```
f98a5c8bc3eeadaee865ecbdf5ac955fef4614ca2551a250a962b42a000d36f5
```

The merkle root is derived from the hash of the single coinbase transaction in the genesis block (updated for new message).

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

### Mining Results

Genesis block successfully mined on November 14, 2025:

- **Mining Date**: November 14, 2025
- **Mining Duration**: ~8 minutes (478 seconds)
- **Hash Rate**: 2,581,127 H/s
- **Nonce Found**: 1,233,778,760
- **Final Hash**: `000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9`
- **Status**: ✅ Complete

## Verification

The genesis block has been verified and meets all requirements:

1. ✅ **Proof of Work**: Block hash `000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9` is less than target
2. ✅ **Merkle Root**: Matches the calculated merkle root of the coinbase transaction
3. ✅ **Coinbase**: Valid coinbase transaction with correct reward (105,113,636 INT)
4. ✅ **Timestamp**: November 6, 2025 00:00:00 UTC (1730851200)

## Implementation

### Final Implementation (block.cpp)

The genesis block has been hard-coded with the mined nonce:

```cpp
Block GenesisBlock::create_mainnet() {
    const std::string message = "BBC News 06/Nov/2025 Will quantum be bigger than AI?";
    const uint64_t timestamp = 1730851200;  // November 6, 2025 00:00:00 UTC
    const uint64_t nonce = 1233778760ULL;  // Successfully mined nonce
    const uint32_t bits = 0x1d00ffff;  // Initial difficulty

    return create_genesis(message, timestamp, nonce, bits);
}
```

This produces the genesis block with hash:
```
000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9
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
- **P2P Port**: 9333 (unique to INTcoin)
- **RPC Port**: 9332 (unique to INTcoin)
- **Genesis Hash**: `000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9`

### Testnet

- **Magic Bytes**: `0x54494E54` ("TINT")
- **P2P Port**: 18333
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
- **Block Time**: ~5 minutes (300 seconds target)
- **Halving Interval**: 1,051,200 blocks (~4 years)
- **Max Supply**: 221,000,000,000,000 INT (221 Trillion)
- **Blocks Per Year**: ~105,120 (at 5-minute intervals)

## Historical Context

The genesis block message references a BBC News headline from November 6, 2025, questioning whether quantum computing will surpass artificial intelligence in impact. This mirrors Bitcoin's tradition of embedding a newspaper headline, while marking INTcoin's mission: creating a cryptocurrency that addresses the quantum computing threat before it materializes.

## Completed Milestones

1. ✅ **Complete Mining**: Genesis block successfully mined (November 14, 2025)
2. ✅ **Update Code**: Nonce 1233778760 hard-coded in `src/core/block.cpp`
3. ✅ **Verify Block**: All validation tests passed
4. ✅ **Document Hash**: Genesis hash recorded and verified
5. ✅ **Network Seeds**: Primary seed nodes configured (74.208.112.43:9333, 51.155.97.192:9333)

## Pre-Launch Checklist

- [x] Genesis block mined and verified
- [x] Network ports configured (9333 P2P, 9332 RPC)
- [x] Primary seed nodes added
- [x] Qt wallet built successfully
- [x] Hardware wallet USB support integrated
- [x] Documentation complete
- [ ] Final security audit
- [ ] Network stress testing
- [ ] Mainnet launch (January 1, 2026)

## References

- **Genesis Miner**: [tools/mine_genesis.cpp](../tools/mine_genesis.cpp)
- **Block Implementation**: [src/core/block.cpp](../src/core/block.cpp)
- **Block Header**: [include/intcoin/block.h](../include/intcoin/block.h)
- **Consensus Parameters**: [include/intcoin/primitives.h](../include/intcoin/primitives.h)

---

**Lead Developer**: Maddison Lane
**Genesis Mined**: November 14, 2025
**Network Launch**: January 1, 2026
**Documentation**: Updated November 14, 2025
