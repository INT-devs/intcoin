# INTcoin Mempool Design & Operations

**Version:** 1.0
**Status:** Production Ready
**Last Updated:** November 25, 2025

## Overview

The mempool (memory pool) is the collection of unconfirmed transactions waiting to be included in blocks. This document describes INTcoin's mempool design, operation, and optimization strategies.

## Table of Contents

- [Mempool Architecture](#mempool-architecture)
- [Transaction Ordering](#transaction-ordering)
- [Fee Estimation](#fee-estimation)
- [Validation & Limits](#validation--limits)
- [Mempool Monitoring](#mempool-monitoring)
- [Performance Optimization](#performance-optimization)
- [Fuzzing & Testing](#fuzzing--testing)

## Mempool Architecture

### Core Components

```
┌─────────────────────────────────────────┐
│    Mempool Manager                      │
│  ┌───────────────────────────────────┐  │
│  │ Transaction Index                 │  │
│  │ - TXID → Transaction mapping      │  │
│  │ - Fast lookup and retrieval        │  │
│  │ - O(1) average access time        │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │ Priority Queue                    │  │
│  │ - Ordered by fee rate             │  │
│  │ - Fast block template generation  │  │
│  │ - Fee-based prioritization        │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │ Ancestor/Descendant Tracking      │  │
│  │ - Parent-child relationships      │  │
│  │ - Total fee & size calculations   │  │
│  │ - Transaction eviction ordering   │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │ RBF (Replace-By-Fee) Manager      │  │
│  │ - Tracks conflicting inputs       │  │
│  │ - Validates RBF rules             │  │
│  │ - Atomic replacement              │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

### Data Structures

**Transaction Entry:**
```cpp
struct MempoolEntry {
    Transaction tx;           // Transaction object
    int64_t fee;             // Total fee in satoshis
    uint32_t fees_per_byte;  // Fee rate
    uint64_t time;           // Insertion time
    uint32_t count_down;     // Expiration counter
    bool is_spent_coin;      // UTXO status
};
```

**Priority Ordering:**
```
Fee Rate (satoshis/byte) = Total Fee / Transaction Size
- Higher fee rate = Higher priority
- Used for block template generation
- Updated when children transactions added
```

## Transaction Ordering

### Fee Rate Based Ordering

**Standard Priority:**
```
Priority Score = Fee Rate × Ancestor Count × Descendant Fee
- Transactions with higher fee rates included first
- Accounts for transaction dependencies
- CPFP (Child Pays For Parent) support
```

**Ancestor Set Scoring:**
```
Include entire ancestor set if:
- Total ancestor fee rate > threshold
- Reduces need for low-fee parents
- Enables efficient block building
```

**Descendant Set Handling:**
```
When including transaction:
- Check all descendant transactions
- If descendant fee-sensitive: Include descendants
- Maximizes total block fees
- Prevents orphaned transactions in block
```

### Replace-By-Fee (RBF)

**RBF Validation Rules:**
1. New transaction spends same inputs as original
2. New transaction has higher total fee
3. New transaction size ≤ 110% of replaced transaction
4. Replacement doesn't exceed 25 conflicts
5. Signaling via sequence numbers (BIP 125)

**RBF Process:**
```
1. Receive replacement transaction
2. Validate fee increase (minimum +0.01 INT)
3. Check conflict count (≤25)
4. Remove original transaction
5. Add replacement transaction
6. Update fee/size statistics
7. Notify RBF subscribers
```

## Fee Estimation

### Dynamic Fee Calculation

**Base Fee Determination:**
```
1. Analyze mempool transaction fee rates
2. Group by age (recent vs historical)
3. Calculate percentiles:
   - 25th percentile: Economy fee
   - 50th percentile: Standard fee
   - 75th percentile: Fast fee
   - 95th percentile: Instant fee

Formula:
Economy: p25 * 0.8
Standard: p50 * 1.0
Fast: p75 * 1.5
Instant: p95 * 2.0
```

**Size-Based Variations:**
```
Small Tx (<100 bytes):
- Applies flat rate minimum
- Prevents dust attacks

Standard Tx (100-1000 bytes):
- Uses calculated rates
- Most common transactions

Large Tx (>1000 bytes):
- May use higher rates
- Accounts for propagation delays
```

### Confirmation Time Predictions

**Predictive Algorithm:**
```
Estimated Time = Base Time + Size Factor + Congestion Factor

Base Time:
- 5 minutes (1 block average) = 100% probability
- 10 minutes (2 blocks) = 95% probability
- 30 minutes (6 blocks) = 75% probability

Size Factor (bytes):
- Each 1,000 bytes adds 1-2 seconds latency

Congestion Factor:
- Mempool size <50 MB: +0 seconds
- Mempool size 50-150 MB: +5 seconds
- Mempool size 150-300 MB: +30 seconds
- Mempool size >300 MB: +60 seconds
```

## Validation & Limits

### Mempool Limits

| Metric | Limit | Reason |
|--------|-------|--------|
| **Max Mempool Size** | 300 MB | Memory management |
| **Max Orphan Tx** | 100 MB | Child-without-parent handling |
| **Max Tx Size** | 1 MB | Large tx rejection |
| **Min Relay Fee** | 0.0001 INT/byte | Dust prevention |
| **Max Ancestors** | 25 | Evaluation complexity |
| **Max Descendants** | 25 | Eviction management |

### Transaction Validation

**Pre-Mempool Checks:**
1. Serialization validity
2. Output script validity (not too large)
3. Input references existing UTXO
4. All inputs ≥ outputs (no negative fees)
5. Not double-spending mempool tx
6. Signature validation (Dilithium5)

**Standard Policy Checks:**
1. Transaction size < 1 MB
2. Inputs/outputs < 10,000 each
3. Dust output rejection (< 0.00001 INT)
4. Lock time not in far future
5. Sequence number checks (CSV)
6. Script size < 10,000 bytes

### Eviction Policy

**When Mempool Size Exceeded:**

```
1. Sort by ancestor fee rate (lowest first)
2. Calculate eviction set:
   - Target eviction: 10% of mempool
   - Cumulative removal until under limit
   - Group related transactions (ancestors)
3. Remove eviction set
4. Log evicted transactions
5. Notify affected users
```

**Eviction Protection:**
- Recent transactions (< 10 minutes): Protected
- High-fee transactions: Protected
- Strategic transactions: May be protected

## Mempool Monitoring

### Real-Time Statistics

**Tracked Metrics:**
```
Total Transactions: N
Total Bytes: M
Average Fee Rate: X sat/byte
Median Fee Rate: Y sat/byte
Min Fee Rate: A sat/byte
Max Fee Rate: B sat/byte

Age Statistics:
- Min Age: X seconds
- Max Age: Y seconds
- Median Age: Z seconds
```

**Activity Metrics:**
```
Transactions/Second: TPS
Bytes/Second: BPS
Evictions/Hour: E
RBF Updates/Hour: R
Orphan Handling: O
```

### Alerts & Notifications

**High Mempool Congestion** (>200 MB):
- Alert threshold: 200 MB
- Notification: Email + SMS
- Recommendation: Use Fast fee level

**Low Fee Network** (<0.0001 INT/byte):
- Alert threshold: Economic level
- Recommendation: Batch transactions
- Delay acceptable transactions

**Unusual Activity:**
- Large transaction pattern
- Rapid transaction rate
- Fee spike detection
- Orphan transaction surge

## Performance Optimization

### Mempool Operations

**Insertion Performance (O(log n)):**
```
1. Validate transaction (O(n) for inputs)
2. Add to index (O(1) average)
3. Insert into priority queue (O(log n))
4. Update ancestor/descendant tracking (O(1) average)

Total: O(n + log n) = O(n) for large tx
```

**Block Template Generation (O(n)):**
```
1. Iterate priority queue (O(n) transactions)
2. Check transaction validity (O(1) average)
3. Add to template (O(1))
4. Calculate total fees (O(1))
5. Optimal block found in O(n) average

Optimization: Cache top N transactions
- Reuse if no mempool changes
- Reduce CPU usage for frequent mining
```

**Transaction Lookup (O(1) average):**
```
TXID → Index lookup: O(1)
Fast confirmation queries
Orphan resolution
RBF conflict detection
```

### Memory Usage Optimization

**Dynamic Sizing:**
```
Available Memory Detection:
- Query system free memory
- Target 50% of free memory maximum
- Auto-scale mempool size
- Prevent system instability
```

**Transaction Compression:**
```
Option: Store serialized form only
Benefit: ~20% memory reduction
Cost: Deserialization on access
Tradeoff: Enabled for >200 MB mempool
```

## Fuzzing & Testing

### Mempool Fuzz Testing

**Target: `fuzz_mempool.cpp`**

**Fuzz Coverage (48 lines):**
- Malformed transaction insertion
- Invalid input references
- Duplicate transaction handling
- Fee calculation with edge cases
- Eviction logic validation
- RBF replacement attempts
- Orphan transaction handling
- Ancestor/descendant tracking

**Test Scenarios:**
```
1. Random transaction insertion (valid)
2. Malformed transaction input
3. Negative fee (output > input)
4. Missing input reference
5. Circular dependencies
6. Maximum ancestor chains
7. RBF with insufficient fee increase
8. Duplicate transactions
9. Transaction size attacks
10. Rapid insertion/eviction
```

**Test Infrastructure:**
```
Corpus Directory: tests/fuzz/corpus/mempool/
Seed Samples: 50+ representative transactions
Mutation Strategy: libFuzzer auto-generation
Execution Time: 10+ million iterations
Sanitizers: ASan, UBSan, MSan enabled
```

### Performance Benchmarks

**Benchmark Categories:**
```
1. Insertion Performance
   - Measure: insertions/second
   - Target: 1,000+ ops/sec
   - Payload: Various tx sizes

2. Lookup Performance
   - Measure: lookups/second
   - Target: 5,000+ ops/sec
   - Query: By TXID

3. Block Template Generation
   - Measure: templates/second
   - Target: 100+ ops/sec
   - Complexity: Full evaluation

4. Memory Usage
   - Measure: Bytes per transaction
   - Target: <1 KB per entry
   - Metric: Efficient data structures
```

### Integration Tests

**Test Coverage:**
- Deposit processing (mempool → confirmation)
- Withdrawal processing (RBF scenarios)
- High-load stress testing (100+ tx/sec)
- Network synchronization
- Blockchain reorganization

## Consensus Rules

### Mempool vs. Block Rules

**Permissive in Mempool:**
- RBF allowed (BIP 125)
- Replace-by-fee signals accepted
- Higher size limits during validation
- Better error reporting

**Strict in Block:**
- No transaction replacement
- Size limits enforced
- No RBF signals required
- Deterministic validation

### Transaction Expiration

**Default Expiration:**
- Time to live: 72 hours (432,000 blocks)
- Calculation: Continuous countdown
- Removal: Automatic at expiration
- Notification: User informed of expiry

**Rebroadcast Policy:**
```
If TX Not In Block After:
- 1 hour: No action (normal)
- 24 hours: Optional rebroadcast
- 48 hours: Rebroadcast recommended
- 72 hours: Automatic expiration
```

## Best Practices

### For Exchange Operators

1. **Monitor Mempool Size**
   - Track daily patterns
   - Adjust fee estimates accordingly
   - Plan for congestion peaks

2. **Batch Transactions**
   - Combine multiple withdrawals
   - Reduce overall network load
   - Lower per-transaction fees

3. **RBF Strategy**
   - Use RBF for time-sensitive tx
   - Increase fee incrementally
   - Maintain customer communication

4. **Orphan Prevention**
   - Ensure parent transactions confirmed
   - CPFP (Child Pays For Parent) support
   - Timely transaction rebroadcast

### For Miners

1. **Fee Rate Optimization**
   - Include highest-fee transactions first
   - Respect ancestor/descendant relationships
   - Monitor network-wide fee distribution

2. **Block Space Allocation**
   - Reserve space for priority transactions
   - Include at least 500 KB of standard txs
   - Optimize for maximum fee collection

## Resources

- [Transaction Validation](../src/transaction_validation.h)
- [Mempool Manager Implementation](../src/mempool_manager.cpp)
- [Fuzz Testing Guide](FUZZ-TESTING.md)
- [RPC API - Mempool Methods](RPC-API.md#mempool)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Initial mempool design documentation with fuzz testing coverage |

---

**License**: MIT License
**Copyright**: (c) 2025 INTcoin Core
