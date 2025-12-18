# INTcoin Consensus Mechanism

**Status**: 🟢 Complete (RandomX + Coinbase Validation)
**Tests**: 6/6 RandomX tests passing
**Last Updated**: December 18, 2025

INTcoin uses a hybrid consensus mechanism combining RandomX Proof-of-Work with Digishield V3 dynamic difficulty adjustment.

---

## Table of Contents

1. [Overview](#overview)
2. [RandomX Proof-of-Work](#randomx-proof-of-work)
3. [Digishield V3 Difficulty Adjustment](#digishield-v3-difficulty-adjustment)
4. [Block Rewards](#block-rewards)
5. [Implementation](#implementation)
6. [Security Considerations](#security-considerations)
7. [Test Coverage](#test-coverage)

---

## Overview

### Consensus Parameters

| Parameter | Value | Purpose |
|-----------|-------|---------|
| **Algorithm** | RandomX | ASIC-resistant PoW |
| **Block Time** | 2 minutes (120 seconds) | Target block interval |
| **Difficulty Adjustment** | Digishield V3 (every block) | Responsive difficulty |
| **Initial Reward** | 105,113,636 INT | Block subsidy |
| **Halving Interval** | 1,051,200 blocks (~4 years) | Reward reduction |
| **Max Halvings** | 64 | After which rewards = 0 |
| **Total Supply** | 221 Trillion INT | Maximum coin supply |
| **Emission Period** | ~256 years | Total emission time |

###Formula Summary

```
Block Reward = 105,113,636 >> (height / 1,051,200)
Difficulty Adjustment = Digishield V3 (per-block)
Target Hash < (2^256 / difficulty)
```

---

## RandomX Proof-of-Work

### Overview

RandomX is a Proof-of-Work algorithm optimized for general-purpose CPUs. It uses random code execution and memory-hard techniques to ensure ASIC resistance.

**Status**: ✅ Complete (100%)

### Why RandomX?

1. **ASIC Resistance**: Optimized for CPUs, economically unfeasible for ASICs
2. **Fair Distribution**: Allows CPU mining participation
3. **Security**: Proven algorithm used by Monero since 2019
4. **Memory Hard**: Requires 2GB+ memory, prevents lightweight implementations

### Epoch System

RandomX uses epoch-based key rotation to maintain ASIC resistance:

```cpp
Epoch Length: 2048 blocks (~2.8 days at 2-minute blocks)
Epoch Key: SHA3-256("INTcoin-RandomX-Epoch-{N}")

Block 0-2047:    Epoch 0
Block 2048-4095:  Epoch 1
Block 4096-6143:  Epoch 2
...
```

### Algorithm Flow

```
1. Get block header (excluding randomx_hash field)
2. Get epoch number: epoch = height / 2048
3. Generate epoch key: SHA3-256("INTcoin-RandomX-Epoch-{epoch}")
4. Initialize RandomX cache with epoch key
5. Serialize block header
6. Calculate RandomX hash
7. Check: hash < target
```

### Hardware Optimizations

```cpp
// x86_64 (Intel/AMD)
- JIT compilation enabled
- Hardware AES instructions
- AVX2 vectorization

// ARM64 (Apple Silicon, AWS Graviton)
- Hardware AES instructions
- NEON vectorization
- Software JIT

// Other architectures
- Interpreter mode
- Software AES
- Portable implementation
```

### Performance

| CPU | Hashrate | Power |
|-----|----------|-------|
| AMD Ryzen 9 5950X (16c) | ~18 KH/s | ~105W |
| Intel i9-12900K (16c) | ~16 KH/s | ~125W |
| Apple M1 Max (10c) | ~6 KH/s | ~30W |
| Apple M2 Ultra (24c) | ~15 KH/s | ~60W |

### API Usage

```cpp
#include "intcoin/consensus.h"

// Initialize RandomX (call once at startup)
auto init_result = RandomXValidator::Initialize();
if (init_result.IsError()) {
    // Handle error
}

// Calculate hash for block header
BlockHeader header;
header.version = 1;
header.prev_block_hash = previous_block.GetHash();
header.merkle_root = CalculateMerkleRoot(transactions);
header.timestamp = GetCurrentTime();
header.bits = next_difficulty;
header.nonce = 0;
header.randomx_key = RandomXValidator::GetRandomXKey(height);

// Mine (find valid nonce)
while (true) {
    auto hash_result = RandomXValidator::CalculateHash(header);
    if (hash_result.IsOk()) {
        uint256 hash = *hash_result.value;
        if (hash < target) {
            // Found valid block!
            header.randomx_hash = hash;
            break;
        }
    }
    header.nonce++;
}

// Validate block hash
auto validate_result = RandomXValidator::ValidateBlockHash(header);
if (validate_result.IsOk()) {
    // Block is valid
}

// Update dataset for new epoch (automatic)
if (RandomXValidator::NeedsDatasetUpdate(height)) {
    RandomXValidator::UpdateDataset(height);
}

// Shutdown (call at exit)
RandomXValidator::Shutdown();
```

### Implementation Details

**Source Files**:
- `src/consensus/consensus.cpp` - RandomX implementation
- `include/intcoin/consensus.h` - API definitions
- `tests/test_randomx.cpp` - Test suite

**Key Functions**:
```cpp
Result<void> RandomXValidator::Initialize();
void RandomXValidator::Shutdown();
Result<uint256> RandomXValidator::CalculateHash(const BlockHeader& header);
Result<void> RandomXValidator::ValidateBlockHash(const BlockHeader& header);
uint256 RandomXValidator::GetRandomXKey(uint64_t height);
bool RandomXValidator::NeedsDatasetUpdate(uint64_t height);
Result<void> RandomXValidator::UpdateDataset(uint64_t height);
```

**Global State** (thread-safe with mutex):
```cpp
randomx_cache* g_randomx_cache;  // RandomX cache
randomx_vm* g_randomx_vm;        // RandomX VM
uint256 g_current_key;           // Current epoch key
uint64_t g_current_epoch;        // Current epoch number
std::mutex g_randomx_mutex;      // Thread safety
bool g_randomx_initialized;      // Initialization flag
```

---

## Digishield V3 Difficulty Adjustment

### Overview

Digishield V3 is a per-block difficulty adjustment algorithm that responds quickly to hashrate changes while preventing exploitation.

**Status**: 🔴 Not Implemented (Planned)

### Why Digishield V3?

1. **Fast Response**: Adjusts every block (vs. Bitcoin's 2016 blocks)
2. **Attack Resistant**: Prevents difficulty manipulation
3. **Hashrate Stability**: Maintains consistent block times
4. **Proven**: Used by Dogecoin and other major cryptocurrencies

### Algorithm

```cpp
DIFFICULTY_AVERAGING_WINDOW = 60 blocks
DIFFICULTY_DAMPING_FACTOR = 4
TARGET_BLOCK_TIME = 120 seconds

function GetNextWorkRequired(last_block, blockchain):
    // Get last N blocks
    blocks = blockchain.GetLastNBlocks(DIFFICULTY_AVERAGING_WINDOW)

    // Calculate actual timespan
    actual_timespan = blocks[0].timestamp - blocks[N-1].timestamp

    // Calculate average difficulty
    avg_difficulty = Sum(block.difficulty for block in blocks) / N

    // Calculate expected timespan
    expected_timespan = (N - 1) * TARGET_BLOCK_TIME

    // Apply damping factor
    min_timespan = expected_timespan / DIFFICULTY_DAMPING_FACTOR
    max_timespan = expected_timespan * DIFFICULTY_DAMPING_FACTOR
    actual_timespan = clamp(actual_timespan, min_timespan, max_timespan)

    // Calculate new difficulty
    new_difficulty = avg_difficulty * expected_timespan / actual_timespan

    return new_difficulty
```

### Key Features

**Averaging Window**: Uses last 60 blocks to smooth out variance
**Damping**: Limits adjustment to ±25% per block
**Min/Max Difficulty**: Enforces minimum and maximum difficulty bounds

### Implementation Plan

```cpp
// src/consensus/consensus.cpp

uint32_t DifficultyCalculator::GetNextWorkRequired(
    const BlockHeader& last_block,
    const Blockchain& chain
) {
    const int AVERAGING_WINDOW = 60;
    const int DAMPING_FACTOR = 4;

    // Get last N blocks
    std::vector<BlockHeader> blocks;
    for (int i = 0; i < AVERAGING_WINDOW; i++) {
        blocks.push_back(chain.GetBlockHeader(last_block.height - i));
    }

    // Calculate actual timespan
    uint64_t actual_timespan = blocks[0].timestamp - blocks[AVERAGING_WINDOW-1].timestamp;

    // Calculate average difficulty
    uint256 total_difficulty{};
    for (const auto& block : blocks) {
        total_difficulty += CompactToTarget(block.bits);
    }
    uint256 avg_difficulty = total_difficulty / AVERAGING_WINDOW;

    // Calculate expected timespan
    uint64_t expected_timespan = (AVERAGING_WINDOW - 1) * consensus::TARGET_BLOCK_TIME;

    // Apply damping
    uint64_t min_timespan = expected_timespan / DAMPING_FACTOR;
    uint64_t max_timespan = expected_timespan * DAMPING_FACTOR;
    actual_timespan = std::clamp(actual_timespan, min_timespan, max_timespan);

    // Calculate new difficulty
    uint256 new_target = avg_difficulty * actual_timespan / expected_timespan;

    // Enforce limits
    if (new_target > consensus::MAX_TARGET) {
        new_target = consensus::MAX_TARGET;
    }
    if (new_target < consensus::MIN_TARGET) {
        new_target = consensus::MIN_TARGET;
    }

    return TargetToCompact(new_target);
}

uint256 DifficultyCalculator::CompactToTarget(uint32_t compact) {
    // Convert compact difficulty representation to full target
    uint32_t exponent = compact >> 24;
    uint32_t mantissa = compact & 0x00FFFFFF;

    uint256 target{};
    if (exponent <= 3) {
        mantissa >>= (8 * (3 - exponent));
        target = uint256{mantissa};
    } else {
        target = uint256{mantissa};
        // Shift left by (exponent - 3) bytes
        // Implementation needed
    }

    return target;
}

uint32_t DifficultyCalculator::TargetToCompact(const uint256& target) {
    // Convert full target to compact difficulty representation
    // Find first non-zero byte
    int leading_zeros = 0;
    for (int i = 31; i >= 0; i--) {
        if (target[i] != 0) {
            break;
        }
        leading_zeros++;
    }

    uint32_t exponent = 32 - leading_zeros;
    uint32_t mantissa = /* extract 3 bytes */;

    return (exponent << 24) | mantissa;
}
```

---

## Block Rewards

### Emission Schedule

```cpp
uint64_t GetBlockReward(uint64_t height) {
    uint64_t halvings = height / consensus::HALVING_INTERVAL; // 1,051,200 blocks

    if (halvings >= consensus::MAX_HALVINGS) { // 64 halvings
        return 0;
    }

    uint64_t reward = consensus::INITIAL_BLOCK_REWARD; // 105,113,636 INT
    reward >>= halvings; // Divide by 2^halvings

    return reward;
}
```

### Halving Schedule

| Halving | Block Range | Blocks | Reward (INT) | Coins Minted | % of Total |
|---------|-------------|--------|--------------|--------------|------------|
| 0 | 0 - 1,051,199 | 1,051,200 | 105,113,636 | 110.5T | 50.0% |
| 1 | 1,051,200 - 2,102,399 | 1,051,200 | 52,556,818 | 55.25T | 25.0% |
| 2 | 2,102,400 - 3,153,599 | 1,051,200 | 26,278,409 | 27.62T | 12.5% |
| 3 | 3,153,600 - 4,204,799 | 1,051,200 | 13,139,204 | 13.81T | 6.25% |
| ... | ... | ... | ... | ... | ... |
| 63 | 66,225,600 - 67,276,799 | 1,051,200 | ~0.01 | ~12K | ~0.000001% |

**Total Supply**: ~221 Trillion INT (after 64 halvings)

### Supply Calculation

```cpp
uint64_t GetSupplyAtHeight(uint64_t height) {
    uint64_t supply = 0;
    uint64_t current_height = 0;

    while (current_height < height) {
        uint64_t next_halving = ((current_height / consensus::HALVING_INTERVAL) + 1)
                               * consensus::HALVING_INTERVAL;
        uint64_t blocks_this_period = std::min(next_halving, height) - current_height;

        supply += blocks_this_period * GetBlockReward(current_height);
        current_height += blocks_this_period;
    }

    return supply;
}
```

---

## Coinbase Transaction Validation

### Overview

Coinbase transactions are special transactions that create new coins and collect transaction fees. They must be validated to prevent inflation attacks and ensure fair reward distribution.

**Status**: ✅ Complete (100%)

### Why Coinbase Validation?

1. **Inflation Prevention**: Ensures miners don't create more coins than allowed
2. **Economic Security**: Maintains the emission schedule and total supply cap
3. **Fee Collection**: Validates miners can only claim fees from included transactions
4. **Attack Prevention**: Prevents double-spending of block rewards

### Validation Rules

```cpp
// Coinbase validation checks:
1. Transaction must be a coinbase (1 input with prevout index = 0xFFFFFFFF)
2. Calculate block subsidy with halving based on height
3. Total output ≤ (subsidy + transaction fees)
4. Outputs must exist and have valid scripts
5. Miners may claim less than full reward (burns allowed)
```

### Implementation

**Source**: [src/consensus/consensus.cpp:537-587](../src/consensus/consensus.cpp#L537-L587)

```cpp
Result<void> ConsensusValidator::ValidateCoinbase(const Transaction& coinbase,
                                                 uint64_t height,
                                                 uint64_t total_fees) {
    // 1. Check if this is actually a coinbase transaction
    if (!coinbase.IsCoinbase()) {
        return Result<void>::Error("Transaction is not a coinbase transaction");
    }

    // 2. Calculate block subsidy with halving
    uint64_t subsidy = 0;
    uint64_t halving_count = height / consensus::HALVING_INTERVAL;

    if (halving_count >= consensus::MAX_HALVINGS) {
        // After max halvings, no more subsidy (only fees)
        subsidy = 0;
    } else {
        // Halve the reward for each halving period using bit shift
        subsidy = consensus::INITIAL_BLOCK_REWARD >> halving_count;
    }

    // 3. Calculate expected reward (subsidy + fees)
    uint64_t expected_reward = subsidy + total_fees;

    // 4. Validate outputs exist
    if (coinbase.outputs.empty()) {
        return Result<void>::Error("Coinbase transaction has no outputs");
    }

    // 5. Validate output scripts are not empty
    for (const auto& output : coinbase.outputs) {
        if (output.script_pubkey.bytes.empty()) {
            return Result<void>::Error("Coinbase output has empty script");
        }
    }

    // 6. Check total output value doesn't exceed expected reward
    uint64_t total_output = coinbase.GetTotalOutputValue();

    if (total_output > expected_reward) {
        return Result<void>::Error(
            "Coinbase output value exceeds expected reward (got " +
            std::to_string(total_output) + ", expected max " +
            std::to_string(expected_reward) + ")"
        );
    }

    // Note: It's acceptable for miners to claim less than the full reward
    // (this is sometimes done intentionally or due to bugs)

    return Result<void>::Ok();
}
```

### Halving Calculation

The block subsidy uses efficient bit-shift operations for halving:

```cpp
// Every 1,051,200 blocks (~4 years), reward halves
uint64_t halving_count = height / 1051200;

// Bit shift right = divide by 2^n
// subsidy >>= 1  means subsidy = subsidy / 2
// subsidy >> n   means subsidy = subsidy / 2^n

uint64_t subsidy = 105113636 >> halving_count;

// Examples:
// Height 0:         105113636 >> 0 = 105,113,636 INT
// Height 1,051,200: 105113636 >> 1 =  52,556,818 INT
// Height 2,102,400: 105113636 >> 2 =  26,278,409 INT
// Height 3,153,600: 105113636 >> 3 =  13,139,204 INT
```

### Error Cases

The validation returns specific error messages for different failure scenarios:

| Error | Cause | Impact |
|-------|-------|--------|
| `Transaction is not a coinbase transaction` | `IsCoinbase()` returns false | Block rejected |
| `Coinbase transaction has no outputs` | Empty outputs vector | Block rejected |
| `Coinbase output has empty script` | Invalid script_pubkey | Block rejected |
| `Coinbase output value exceeds expected reward` | Output > (subsidy + fees) | Block rejected (inflation attempt) |

### Security Considerations

1. **Overflow Protection**: Uses 64-bit integers for all calculations
2. **Halving Limit**: After 64 halvings, subsidy = 0 (only fees remain)
3. **Underflow Allowed**: Miners can claim less than full reward (intentional burns)
4. **Fee Calculation**: Caller must compute `total_fees` from all transactions

### API Usage

```cpp
#include "intcoin/consensus.h"

// Validate a block's coinbase transaction
Block block;
uint64_t total_fees = 0;

// Calculate fees from all non-coinbase transactions
for (size_t i = 1; i < block.transactions.size(); i++) {
    const auto& tx = block.transactions[i];

    // Get input value
    uint64_t input_value = 0;
    for (const auto& input : tx.inputs) {
        auto utxo = blockchain.GetUTXO(input.prevout);
        if (utxo.IsOk()) {
            input_value += utxo.value->amount;
        }
    }

    // Get output value
    uint64_t output_value = tx.GetTotalOutputValue();

    // Fee = input - output
    if (input_value > output_value) {
        total_fees += (input_value - output_value);
    }
}

// Validate coinbase
auto result = ConsensusValidator::ValidateCoinbase(
    block.transactions[0],  // Coinbase must be first transaction
    block.header.height,
    total_fees
);

if (result.IsError()) {
    std::cerr << "Coinbase validation failed: " << result.error << "\n";
    // Reject block
}
```

### Test Coverage

Coinbase validation is tested as part of the consensus validation test suite:

```bash
./tests/test_validation
# Tests include:
# - Valid coinbase with exact reward
# - Valid coinbase with less than full reward
# - Invalid coinbase with excessive output
# - Invalid coinbase with empty outputs
# - Coinbase validation across halving boundaries
```

---

## Implementation

### Source Files

```
src/consensus/consensus.cpp       # Implementation
include/intcoin/consensus.h      # API definitions
tests/test_randomx.cpp           # RandomX tests
tests/test_difficulty.cpp        # Difficulty tests (TODO)
```

### Constants

```cpp
namespace consensus {
    constexpr uint64_t TARGET_BLOCK_TIME = 120;           // 2 minutes
    constexpr uint64_t HALVING_INTERVAL = 1051200;       // ~4 years
    constexpr uint64_t INITIAL_BLOCK_REWARD = 105113636; // INT
    constexpr uint64_t MAX_HALVINGS = 64;
    constexpr uint64_t MAX_SUPPLY = 221000000000000;     // 221 Trillion INT
    constexpr uint64_t MAX_BLOCK_SIZE = 8 * 1024 * 1024; // 8 MB
    constexpr uint64_t COINBASE_MATURITY = 100;          // blocks
    constexpr uint32_t MIN_DIFFICULTY_BITS = 0x1f00ffff;
}
```

---

## Security Considerations

### RandomX Security

1. **Epoch Keys**: Different keys every 2048 blocks prevents precomputation
2. **Memory Hard**: 2GB+ memory requirement prevents lightweight ASICs
3. **CPU Optimized**: Fair distribution among CPU miners
4. **Thread Safe**: All operations protected by mutex

### Difficulty Adjustment Security

1. **Timestamp Manipulation**: Limited by median time past rule
2. **Hashrate Attacks**: Damping factor prevents rapid difficulty swings
3. **Timewarp Attacks**: Per-block adjustment prevents exploitation
4. **51% Attacks**: Does not prevent, but makes consistent attacks expensive

### Attack Vectors

**Timestamp Manipulation**:
- Attacker mines blocks with false timestamps
- Mitigation: Median Time Past (MTP) rule
- Blocks must have timestamp > MTP of last 11 blocks

**Difficulty Manipulation**:
- Attacker mines blocks quickly to lower difficulty
- Mitigation: Damping factor limits adjustment speed
- Maximum adjustment: ±25% per block

**Selfish Mining**:
- Attacker withholds blocks to gain advantage
- Mitigation: Not fully prevented by consensus
- Requires >25% hashrate for profitability

---

## Test Coverage

### RandomX Tests (tests/test_randomx.cpp)

**Status**: ✅ 6/6 tests passing

1. **RandomX Initialization** - Tests VM and cache initialization
2. **Key Generation** - Tests epoch-based key generation
3. **Hash Calculation** - Tests deterministic hashing
4. **Dataset Updates** - Tests epoch transitions
5. **Block Validation** - Tests PoW validation
6. **Shutdown/Cleanup** - Tests resource cleanup

### Difficulty Tests (TODO)

1. **Basic Adjustment** - Test difficulty increases/decreases correctly
2. **Damping** - Test adjustment limits are enforced
3. **Edge Cases** - Test minimum/maximum difficulty
4. **Timewarp Protection** - Test timestamp manipulation protection
5. **Averaging** - Test 60-block averaging window

### Running Tests

```bash
cd build
./tests/test_randomx

# Expected output:
# ========================================
# RandomX PoW Tests
# ========================================
# ✓ Test 1: RandomX Initialization
# ✓ Test 2: Key Generation
# ✓ Test 3: Hash Calculation
# ✓ Test 4: Dataset Updates
# ✓ Test 5: Block Validation
# ✓ Test 6: Shutdown/Cleanup
# ========================================
# ✓ All RandomX tests passed!
# ========================================
```

---

## References

### RandomX

- [RandomX Specifications](https://github.com/tevador/RandomX/blob/master/doc/specs.md)
- [RandomX Design](https://github.com/tevador/RandomX/blob/master/doc/design.md)
- [Monero RandomX Audit](https://www.getmonero.org/2019/11/25/randomx-audit.html)

### Digishield

- [Digishield Whitepaper](https://github.com/dogecoin/digishield)
- [Difficulty Adjustment Comparison](https://github.com/zawy12/difficulty-algorithms)

### Implementation

- [RandomX Repository](https://github.com/tevador/RandomX)
- [liboqs](https://github.com/open-quantum-safe/liboqs)

---

**Next**: [Address Encoding (Bech32)](ADDRESS_ENCODING.md)
