# INTcoin Consensus Engine & PoW Validation

**Version:** 1.0
**Status:** Production Ready
**Last Updated:** November 25, 2025

## Overview

The consensus engine is the core system that ensures all nodes agree on the valid state of the blockchain. INTcoin uses Proof-of-Work (PoW) with ASIC-resistant mining and supports multiple consensus validation layers.

## Table of Contents

- [Consensus Architecture](#consensus-architecture)
- [Proof-of-Work Validation](#proof-of-work-validation)
- [Difficulty Adjustment](#difficulty-adjustment)
- [Block Validation](#block-validation)
- [Transaction Validation](#transaction-validation)
- [Consensus Rules](#consensus-rules)
- [Fuzzing & Testing](#fuzzing--testing)
- [Security Analysis](#security-analysis)

## Consensus Architecture

### Layers of Validation

```
┌──────────────────────────────┐
│  Network Layer               │
│  - Message propagation       │
│  - Peer synchronization      │
│  - Block/TX relay            │
└────────────┬─────────────────┘
             │
┌────────────▼─────────────────┐
│  Mempool Layer               │
│  - TX validation             │
│  - Fee estimation            │
│  - UTXO tracking             │
└────────────┬─────────────────┘
             │
┌────────────▼─────────────────┐
│  Block Validation            │
│  - Header validation         │
│  - TX validation in block    │
│  - Consensus rules check     │
└────────────┬─────────────────┘
             │
┌────────────▼─────────────────┐
│  Consensus Engine            │
│  - PoW verification          │
│  - Difficulty check          │
│  - Chain selection           │
└────────────┬─────────────────┘
             │
┌────────────▼─────────────────┐
│  State Update                │
│  - UTXO set update           │
│  - Balance tracking          │
│  - Database persistence      │
└──────────────────────────────┘
```

### Consensus Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Target Block Time** | 5 minutes | 300 seconds |
| **Difficulty Interval** | 2,016 blocks | 2 weeks |
| **Max Block Size** | 4 MB | Including witness data |
| **Max Transaction Size** | 1 MB | Individual transaction |
| **Block Reward** | Initial: 50 INT | Halves every 210,000 blocks |
| **Total Supply** | 21 Million INT | Hard cap |
| **Coinbase Maturity** | 100 blocks | Confirmation requirement |
| **Max Reorg Depth** | 100 blocks | Security limit |

## Proof-of-Work Validation

### PoW Algorithm

**Algorithm: ASIC-Resistant PoW (Custom)**

```
Format:
Block Header Hash (SHA3-256 over block header)
  Input: Version (4) + Previous Hash (32) + Merkle Root (32) +
          Timestamp (4) + Difficulty (4) + Nonce (4) +
          Quantum Proof (256) = 340 bytes

Target Calculation:
  Difficulty D = Maximum Hash (2^256 - 1) / Target
  Target = Max Hash / Difficulty
  
Valid PoW:
  Block Hash ≤ Target
  Block Hash < (2^256 / Difficulty)
```

**Nonce Range:**
```
Standard: 0 to 2^32 - 1 (4.3 billion)
Extended: Support for extra nonce in coinbase (unlimited)
Extra Nonce: 1-32 bytes in coinbase script
```

**Quantum Proof Component:**
```
Requirement: 256-byte post-quantum signature
Purpose: Prove miner's quantum-safe identity
Verification: Dilithium5 signature validation
Benefit: Future-proof mining validation
```

### PoW Verification Process

**Step 1: Header Parsing**
```cpp
1. Read block header (340 bytes)
2. Verify serialization format
3. Check byte ordering (little-endian)
4. Validate field ranges
```

**Step 2: Difficulty Validation**
```cpp
1. Parse difficulty value
2. Check within valid range (1 to max difficulty)
3. Validate matches current chain difficulty
4. Reject if below minimum acceptable
```

**Step 3: Hash Calculation**
```cpp
1. Serialize block header consistently
2. SHA3-256 hash twice for security
3. Result: 256-bit block hash
4. Compare against target
```

**Step 4: Target Verification**
```cpp
1. Calculate target from difficulty
2. Convert block hash to integer
3. Check: hash_value ≤ target_value
4. Accept if passes, reject if fails
```

**Step 5: Quantum Proof Validation**
```cpp
1. Extract 256-byte quantum proof
2. Parse miner's Dilithium5 public key
3. Verify signature over block header hash
4. Reject if signature invalid (NIST Level 5)
```

### Mining Difficulty

**Difficulty Representation:**
```
Compact Format: 0x1d00ffff
  MSB (1 byte): Exponent (number of bytes)
  Lower 3 bytes: Mantissa (value)
  
Conversion to Target:
  Mantissa = 0x00ffff
  Exponent = 0x1d
  
  Target = Mantissa * 2^(8*(Exponent-3))
         = 0x00ffff * 2^(8*26)
         = 0x00000000ffff0000000000000000000000000000000000000000000000000000
```

**Difficulty Adjustment:**
```
New Difficulty = Old Difficulty * (Actual Time / Target Time)

Example:
- Target blocks: 2,016 over 2 weeks (1,209,600 seconds)
- Actual time for 2,016 blocks: 1,500,000 seconds (24% slower)
- New Difficulty = Old * (1,500,000 / 1,209,600) = Old * 1.24x
- Mining becomes 24% harder
```

## Difficulty Adjustment

### Algorithm Overview

**Adjustment Interval:**
- Every 2,016 blocks
- Approximately 2 weeks (14 days)
- Recalculated after every interval

**Adjustment Formula:**
```
NewDifficulty = OldDifficulty × (ActualTime / TargetTime)

Where:
- TargetTime = 2,016 blocks × 5 minutes = 1,209,600 seconds
- ActualTime = Timestamp(block 2,016) - Timestamp(block 0)
- Constraint: 0.25x ≤ Multiplier ≤ 4.0x (prevents extreme shifts)
```

### Adjustment Safety Limits

**Maximum Change:**
```
Multiplier Range: [0.25x, 4.0x]

If Actual Time / Target Time:
- < 0.25: Cap at 0.25x (4x harder)
- > 4.0: Cap at 4.0x (4x easier)

Purpose: Prevent gaming and stability
```

**Example Adjustments:**
```
Scenario 1: Blocks too slow (10-minute average)
- Target: 5 minutes
- Ratio: 10 / 5 = 2.0
- Adjustment: Difficulty ÷ 2 (made 2x easier)

Scenario 2: Blocks too fast (3-minute average)
- Target: 5 minutes
- Ratio: 3 / 5 = 0.6 (capped at 0.25)
- Adjustment: Difficulty × 4 (made 4x harder)

Scenario 3: Perfect timing (5-minute average)
- Ratio: 5 / 5 = 1.0
- Adjustment: No change (difficulty remains same)
```

### Adjustment Triggering

**Trigger Conditions:**
```
1. Block number % 2,016 == 0
2. Sufficient block history available
3. Valid timestamp progression
4. No consensus rule violations
```

**Timestamp Validation:**
```
Block Timestamp Constraints:
- Must be > median of previous 11 blocks
- Must be ≤ current network time + 2 hours
- Prevents timestamp manipulation
```

## Block Validation

### Block Header Validation

**Header Format (80 bytes):**
```
0-3:   Version (4 bytes)
4-35:  Previous Block Hash (32 bytes)
36-67: Merkle Root (32 bytes)
68-71: Timestamp (4 bytes, Unix time)
72-75: Difficulty (4 bytes, compact format)
76-79: Nonce (4 bytes)
```

**Header Validation Steps:**
```
1. Version Check
   - Valid version range (1-10)
   - Reject invalid versions

2. Previous Hash Verification
   - Must reference known block
   - Check chain continuity
   - Reject orphan headers

3. Merkle Root Validation
   - Calculate from transactions
   - Must match header value
   - Prevent TX modification

4. Timestamp Validation
   - > median of previous 11 blocks
   - ≤ network time + 2 hours
   - Prevent time warp attacks

5. Difficulty Check
   - Matches current target
   - Within valid range
   - Proper adjustment if interval

6. Nonce Validation
   - Used for PoW calculation
   - Range: 0 to 2^32-1
   - Extra nonce: coinbase script
```

### Block Transaction Validation

**Pre-Block Checks:**
```
1. Block size limits
   - Max 4 MB (including witness data)
   - Min 80 bytes (header only)

2. Transaction count
   - First TX must be coinbase
   - Remaining TXs not coinbase
   - < 10,000 transactions

3. Serialization validity
   - All TXs valid format
   - No parsing errors
   - Proper encoding
```

**Per-Transaction Validation:**
```
1. Coinbase validation (if first TX)
   - Height encoding (BIP 34)
   - Valid reward amount
   - No inputs referenced

2. Standard TX validation
   - All inputs reference existing UTXOs
   - Inputs not spent in same block
   - Signatures valid (Dilithium5)
   - Fees non-negative

3. Script validation
   - Script syntax valid
   - Opcodes permissible
   - Script evaluation succeeds
```

### Merkle Root Calculation

**Process:**
```
1. Collect all block transactions
2. Hash each transaction (SHA3-256)
3. If odd number: duplicate last hash
4. Pair consecutive hashes and combine
5. Repeat until single hash remains
6. Result: Merkle root

Validation:
- Calculate merkle root from TXs
- Compare with header merkle root
- Must match exactly
```

## Transaction Validation

### Transaction Structure Validation

**Input Validation:**
```
1. Previous Output Verification
   - UTXO exists in UTXO set
   - Not already spent
   - Correct amount
   - Script type supported

2. Script Validation
   - ScriptSig unlocks ScriptPubKey
   - Signature verification succeeds
   - Stack operations valid

3. Sequence Number Check
   - If sequence < 0xfffffffe: CLTV applies
   - If sequence < 0xffffffff: CSV applies
```

**Output Validation:**
```
1. Amount Validation
   - Non-negative amount
   - Total ≤ max supply (21M INT)
   - Sum of inputs ≥ sum of outputs

2. Script Validation
   - Script size < 10,000 bytes
   - Script not OP_RETURN with invalid structure
   - Proper address format

3. Dust Detection
   - Outputs < 0.00001 INT rejected
   - Exception: OP_RETURN allowed
```

### Signature Verification

**Dilithium5 Signature Validation:**
```
Process:
1. Extract Dilithium5 public key
2. Hash transaction (SHA3-256)
3. Verify signature against hash
4. Signature size: 4,627 bytes

Security:
- NIST Level 5 (256-bit quantum)
- Constant-time verification
- No timing attacks
```

## Consensus Rules

### Chain Selection Rule

**Most-Work Rule:**
```
Primary Rule: Highest cumulative proof-of-work wins
Formula: Select chain with maximum accumulated difficulty

Backup Rules:
1. Checkpoint enforcement
2. Genesis block validation
3. Reorg depth limit (100 blocks)
4. Selfish mining detection
```

### Validation Constants

| Rule | Value | Reason |
|------|-------|--------|
| **Max Reorg Depth** | 100 blocks | Security/finality |
| **Coinbase Maturity** | 100 blocks | Prevents double-spend |
| **Block Size** | 4 MB | Network capacity |
| **Tx Size** | 1 MB | Propagation speed |
| **Max Script Size** | 10,000 bytes | Resource protection |
| **Max Stack Size** | 1,000 items | Memory safety |
| **Max Ops Per Script** | 201 | CPU protection |

## Fuzzing & Testing

### Consensus Engine Fuzz Testing

**Target: `fuzz_consensus.cpp`**

**Fuzz Coverage (61 lines):**
- PoW validation with various difficulty levels
- Block header parsing and validation
- Merkle root calculation and verification
- Difficulty adjustment calculations
- Nonce validation and edge cases
- Timestamp validation rules
- Transaction validation in blocks
- Consensus rule enforcement
- Edge cases in hash calculations
- Blockchain reorg scenarios

**Test Scenarios:**
```
1. Valid block header fuzzing
2. Invalid difficulty values
3. Extreme nonce values (0, max)
4. Merkle root mismatches
5. Timestamp edge cases
6. Transaction validation failures
7. Serialization errors
8. Consensus rule violations
9. Reorg depth testing
10. Selfish mining patterns
```

**Test Infrastructure:**
```
Corpus Directory: tests/fuzz/corpus/consensus/
Seed Samples: 50+ representative blocks
Mutation Strategy: libFuzzer auto-generation
Execution Time: 10+ million iterations
Sanitizers: ASan, UBSan, MSan enabled
Coverage Target: 90%+ code coverage
```

### Integration Tests

**Test Scenarios:**
```
1. Block acceptance workflow
   - Parse header
   - Validate PoW
   - Validate transactions
   - Update chain

2. Difficulty adjustment
   - Track block times
   - Trigger adjustment at interval
   - Verify calculation
   - Confirm new difficulty applied

3. Blockchain reorganization
   - Receive competing block
   - Compare work totals
   - Reorg chain if necessary
   - Update UTXO set

4. Consensus violation detection
   - Reject invalid blocks
   - Prevent consensus splits
   - Automatic ban of violators
```

### Performance Benchmarks

**PoW Verification:**
- Target: 100,000+ verifications/second
- Includes: Hash calculation + difficulty check + signature validation
- Memory usage: < 1 KB per verification

**Block Validation:**
- Target: 1,000+ blocks/second
- Includes: Full TX validation
- Chain with 100k blocks: < 10 minutes

**Difficulty Adjustment:**
- Target: Microsecond precision
- Calculation: Sub-millisecond overhead
- Accuracy: No rounding errors allowed

## Security Analysis

### Attack Prevention

**Double-Spend Prevention:**
- Transaction included in block
- Block confirmed (6 confirmations)
- Reorg limit: 100 blocks (8 hours)
- Probability: Exponentially decreases with depth

**51% Attack:**
- Requires control of network hash power
- Current network size prevents economically
- Defense: Monitor for unusual patterns
- Response: Automatic alert + manual intervention

**Selfish Mining Detection:**
- Tracks unusual block patterns
- Detects rapid block succession
- Monitors duplicate heights
- Automatic scoring system

**Eclipse Attack:**
- Network layer protection
- Multiple peer connections
- BGP security recommended
- Monitor for network partitions

### Quantum-Safety

**Post-Quantum Components:**
- Dilithium5 block signing: NIST Level 5
- Kyber1024 future key exchange: NIST Level 5
- SHA3-256 hashing: Quantum-resistant
- No ECDSA used in consensus

**Migration Path:**
- All new blocks: Dilithium5 signatures
- Legacy blocks: Supported during transition
- Eventually: Historical ECDSA blocks marked as legacy
- Timeline: 5-year deprecation window

## Resources

- [Consensus Validation Implementation](../include/intcoin/consensus_validation.h)
- [Block Validator Code](../src/consensus_validation.cpp)
- [Fuzz Testing Guide](FUZZ-TESTING.md)
- [Security Audit](SECURITY-AUDIT.md#consensus-security)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Initial consensus engine documentation with fuzz testing coverage |

---

**License**: MIT License
**Copyright**: (c) 2025 INTcoin Core
