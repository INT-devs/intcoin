# Enhanced Mempool Documentation

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production

---

## Overview

The Enhanced Mempool in INTcoin v1.2.0-beta introduces a **6-level priority system** with **persistence**, enabling efficient transaction management for diverse transaction types including Lightning Network HTLCs and cross-chain bridge operations.

### Key Features

- 🎯 **6-Level Priority System** - Intelligent transaction categorization
- 💾 **Persistence** - Mempool survives node restarts
- 💰 **Fee-Based Sorting** - Higher fees get higher priority
- 🔗 **Dependency Tracking** - Parent/child transaction relationships
- ⏱️ **Automatic Expiry** - Cleanup old transactions (72 hours default)
- 📊 **Statistics Tracking** - Size, fees, acceptance rate

---

## Priority Levels

### LOW (Priority Level 0)

**Usage**: Transactions with very low fees, non-urgent

**Characteristics**:
- Fee per byte < 0.0001 INT
- May wait hours for confirmation
- First to be evicted when mempool full
- Suitable for: Consolidation, non-urgent transfers

**Example**:
```bash
intcoin-cli sendtoaddress <address> <amount> "" "" false 1
# 1 sat/byte (LOW priority)
```

### NORMAL (Priority Level 1) - Default

**Usage**: Standard transactions

**Characteristics**:
- Fee per byte: 0.0001-0.0005 INT
- Typical confirmation: 10-30 minutes
- Default for most wallet transactions
- Suitable for: Regular payments, wallet transfers

**Example**:
```bash
intcoin-cli sendtoaddress <address> <amount>
# Uses normal priority by default
```

### HIGH (Priority Level 2)

**Usage**: Urgent transactions

**Characteristics**:
- Fee per byte: > 0.0005 INT
- Priority confirmation: 5-15 minutes
- Higher fees = faster processing
- Suitable for: Time-sensitive payments, exchange deposits

**Example**:
```bash
intcoin-cli sendtoaddress <address> <amount> "" "" false 500
# 500 sat/byte (HIGH priority)
```

### HTLC (Priority Level 3)

**Usage**: Lightning Network and Atomic Swap transactions

**Characteristics**:
- Reserved for HTLC transactions
- Time-sensitive (must confirm before timeout)
- Elevated priority even with standard fees
- Automatic assignment for HTLC txs

**Identified by**: Transaction script contains HTLC patterns (OP_IF, OP_SHA256, OP_CHECKLOCKTIMEVERIFY)

### BRIDGE (Priority Level 4)

**Usage**: Cross-chain bridge operations

**Characteristics**:
- Reserved for bridge deposits/withdrawals
- Critical for cross-chain operations
- High priority to prevent timeout issues
- Automatic assignment for bridge txs

**Identified by**: Transaction to/from bridge contract addresses

### CRITICAL (Priority Level 5)

**Usage**: Protocol-critical transactions

**Characteristics**:
- Highest priority
- Reserved for emergency operations
- Requires special authorization
- Examples: Emergency pause, validator rotation, critical patches

**Access**: Restricted to protocol administrators

---

## Configuration

### mempool.conf Settings

Edit `intcoin.conf` or `~/.intcoin/intcoin.conf`:

```conf
# Maximum mempool size in MB
mempool.maxsize=300

# Minimum relay fee per KB in INTS
mempool.minrelayfee=1000

# Transaction expiry time in hours
mempool.expiry=72

# Enable mempool persistence
mempool.persist=1

# Mempool persistence file location
mempool.datadir=/var/lib/intcoin

# Priority level limits (max txs per level)
mempool.limit.low=1000
mempool.limit.normal=5000
mempool.limit.high=10000
mempool.limit.htlc=2000
mempool.limit.bridge=1000
mempool.limit.critical=100

# Eviction policy
mempool.eviction=lowest_priority_first
```

### Runtime Configuration

Via RPC:

```bash
# View current mempool config
intcoin-cli getmempoolinfo

# Set minimum relay fee
intcoin-cli setmempoolconfig '{"minrelayfee": 1000}'

# Clear mempool (requires restart)
intcoin-cli clearmempool
```

---

## Mempool Persistence

### How It Works

**On Shutdown**:
1. Mempool serialized to `mempool.dat`
2. All valid transactions saved
3. Transaction metadata preserved (priority, fees, timestamps)
4. File written atomically (crash-safe)

**On Startup**:
1. `mempool.dat` loaded if exists
2. Transactions validated (signatures, timelock)
3. Expired transactions discarded
4. Valid transactions restored to mempool with original priority

### File Format

Location: `~/.intcoin/mempool.dat` (or configured datadir)

**Structure**:
```
Header (magic bytes, version, checksum)
Transaction count
For each transaction:
  - Transaction data (serialized)
  - Priority level
  - Fee information
  - Entry timestamp
  - Dependencies (parent txs)
```

### Benefits

- 💾 **No transaction loss** on restart/crash
- 🚀 **Faster restart** (no re-propagation needed)
- 📊 **Preserves priority** across restarts
- 🔄 **Seamless upgrades** (maintain mempool across version upgrades)

### Persistence Management

**Manual persistence**:
```bash
intcoin-cli savemempool
```

**Disable persistence** (not recommended):
```conf
mempool.persist=0
```

**Clear persistence file**:
```bash
rm ~/.intcoin/mempool.dat
# Mempool will start empty on next boot
```

---

## Fee-Based Prioritization

### Fee Calculation

For each transaction:

```
fee = total_inputs - total_outputs
fee_per_byte = fee / transaction_size_bytes
```

### Priority Assignment

1. **Manual Priority**: If specified by user
2. **Transaction Type**: HTLC, Bridge → elevated priority
3. **Fee-Based**: fee_per_byte determines LOW/NORMAL/HIGH
4. **Dynamic Adjustment**: Reprioritized if fee increases (RBF)

### Fee Thresholds

| Priority | Minimum Fee/Byte |
|----------|------------------|
| LOW | < 0.0001 INT |
| NORMAL | 0.0001-0.0005 INT |
| HIGH | > 0.0005 INT |
| HTLC | N/A (type-based) |
| BRIDGE | N/A (type-based) |
| CRITICAL | N/A (admin-only) |

---

## Dependency Tracking

### Parent-Child Transactions

Mempool tracks:
- **Parents**: Transactions this tx depends on (spends outputs from)
- **Children**: Transactions that depend on this tx

**Ordering**:
- Parent must be included in block before child
- CPFP (Child Pays For Parent): Child with high fee can pull parent

**Example**:
```
Tx A (low fee) → creates output
  ↓
Tx B (high fee) → spends Tx A output

Result: Both included together due to Tx B's high fee
```

### Query Dependencies

```bash
# Get transaction dependencies
intcoin-cli getmempoolentry <txid>
```

Response includes:
```json
{
  "depends": ["parent_tx_id_1", "parent_tx_id_2"],
  "spentby": ["child_tx_id_1", "child_tx_id_2"],
  "ancestorcount": 2,
  "descendantcount": 2
}
```

---

## Eviction Policy

### When Eviction Occurs

Mempool evicts transactions when:
- Size exceeds `maxsize` (default: 300 MB)
- Priority level exceeds limit
- Memory pressure detected

### Eviction Order

1. **Expired transactions** (older than `expiry` hours)
2. **Lowest priority first** (LOW → NORMAL → HIGH)
3. **Within priority, lowest fee** first
4. **Orphan transactions** (missing parents)

**Protected from eviction**:
- HTLC transactions (time-sensitive)
- Bridge transactions (critical for cross-chain)
- CRITICAL priority transactions
- Transactions with children (dependency chains)

### Manual Eviction

```bash
# Remove specific transaction
intcoin-cli removemempoolentry <txid>

# Remove all transactions below fee threshold
intcoin-cli evictmempool '{"minfee": 0.0001}'
```

---

## Statistics and Monitoring

### Get Mempool Stats

```bash
intcoin-cli getmempoolinfo
```

Response:
```json
{
  "size": 1523,
  "bytes": 456789,
  "usage": 1234567,
  "maxmempool": 314572800,
  "mempoolminfee": 0.00001000,
  "minrelaytxfee": 0.00001000,
  "priority_stats": {
    "LOW": {"count": 100, "bytes": 25000},
    "NORMAL": {"count": 1200, "bytes": 400000},
    "HIGH": {"count": 200, "bytes": 30000},
    "HTLC": {"count": 15, "bytes": 1500},
    "BRIDGE": {"count": 7, "bytes": 289},
    "CRITICAL": {"count": 1, "bytes": 250}
  },
  "total_fees": "15.5"
}
```

### Monitor Mempool Size

```bash
watch -n 10 intcoin-cli getmempoolinfo
```

### Export Mempool Data

```bash
# Get all mempool transactions
intcoin-cli getrawmempool false

# Get detailed mempool entries
intcoin-cli getrawmempool true > mempool_detailed.json
```

---

## RPC Methods

### getmempoolinfo

Get mempool statistics.

**Returns**: Overall stats including size, bytes, fees, priority breakdown

### getrawmempool

Get list of transactions in mempool.

**Parameters**:
- `verbose` (bool): false = txids only, true = detailed info

### getmempoolentry

Get detailed info for specific transaction.

**Parameters**:
- `txid` (string): Transaction ID

**Returns**: Priority, fees, dependencies, size, time

### getmempoolancestors / getmempooldescendants

Get parent or child transactions.

**Parameters**:
- `txid` (string): Transaction ID
- `verbose` (bool): Detailed info

### savemempool

Manually save mempool to disk.

**Note**: Happens automatically on shutdown

### clearmempool

Clear all transactions from mempool (requires restart).

**Warning**: Destructive operation, use with caution

---

## Best Practices

### For Users

1. **Set appropriate fees** based on urgency
2. **Use HIGH priority** for time-sensitive transactions
3. **Monitor mempool** during high congestion
4. **Use RBF** (Replace-By-Fee) to increase stuck transaction fees
5. **Be patient** with LOW priority (may take hours)

### For Node Operators

1. **Allocate sufficient memory** (default 300 MB is reasonable)
2. **Enable persistence** to avoid transaction loss
3. **Monitor mempool size** - if consistently full, may need to increase
4. **Set appropriate min relay fee** to prevent spam
5. **Regular restarts** to clear expired transactions (if persistence disabled)

### For Developers

1. **Use mempool RPC** for fee estimation
2. **Check transaction priority** before broadcasting
3. **Handle rejected transactions** gracefully
4. **Implement transaction retry** with higher fees
5. **Monitor mempool** for confirmation timing

---

## Troubleshooting

### Transaction Not Accepted to Mempool

**Reasons**:
- Fee too low (below min relay fee)
- Transaction already in mempool
- Spending non-existent outputs
- Script validation failed
- Mempool full (LOW priority rejected)

**Check**:
```bash
intcoin-cli getmempoolentry <txid>
# If not found, check error logs
```

### Mempool Full

**Symptoms**: LOW priority transactions rejected

**Solutions**:
1. Increase `mempool.maxsize`
2. Increase min relay fee to reduce spam
3. Clear expired transactions
4. Use higher priority for your transactions

### Transaction Stuck in Mempool

**Causes**: Low fee, dependency not confirmed

**Solutions**:
1. Use RBF to increase fee
2. Wait for low-congestion period
3. Use CPFP (create child transaction with high fee)
4. Check parent transactions are confirmed

### Mempool Not Persisting

**Check**:
1. `mempool.persist=1` in config
2. Write permissions on datadir
3. Disk space available
4. Check logs for persistence errors

---

## Performance Tuning

### Memory Optimization

**Reduce memory usage**:
```conf
mempool.maxsize=100  # Reduce from 300 MB
mempool.limit.low=100  # Reduce low priority limit
```

**Increase for high-volume node**:
```conf
mempool.maxsize=1000  # 1 GB for high-traffic nodes
```

### CPU Optimization

**Reduce validation overhead**:
```conf
mempool.expiry=24  # Shorter expiry (less to validate on restart)
```

### Disk I/O

**Persistence optimization**:
```conf
mempool.datadir=/fast/ssd/path  # Use SSD for faster persistence
```

**Disable persistence** (not recommended):
```conf
mempool.persist=0  # Reduces disk I/O but loses txs on restart
```

---

## Integration Examples

### Priority-Aware Transaction Creation

```python
import json
from bitcoinrpc.authproxy import AuthServiceProxy

rpc = AuthServiceProxy("http://user:pass@127.0.0.1:8332")

# Create HIGH priority transaction
tx = rpc.createrawtransaction(inputs, outputs)
signed = rpc.signrawtransactionwithwallet(tx)
txid = rpc.sendrawtransaction(signed['hex'], 500)  # 500 sat/byte
```

### Mempool Monitoring

```python
while True:
    info = rpc.getmempoolinfo()
    if info['size'] > 10000:
        print(f"WARNING: Mempool large ({info['size']} txs)")
        print(f"Consider using HIGH priority fees")
    time.sleep(60)
```

---

## Resources

- **Source Code**: [src/mempool/mempool.cpp](../src/mempool/mempool.cpp)
- **Header File**: [include/intcoin/mempool.h](../include/intcoin/mempool.h)
- **Tests**: [tests/test_mempool.cpp](../tests/test_mempool.cpp) (12/12 passing)
- **RPC Reference**: [docs/RPC.md](RPC.md)

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
