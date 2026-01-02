# INTcoin Migration Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

This guide helps you migrate from INTcoin v1.0.0-alpha to v1.2.0-beta.

---

## Table of Contents

- [Overview](#overview)
- [Breaking Changes](#breaking-changes)
- [Migration Steps](#migration-steps)
- [Configuration Changes](#configuration-changes)
- [Database Migration](#database-migration)
- [RPC API Changes](#rpc-api-changes)
- [Mobile Wallet Migration](#mobile-wallet-migration)
- [Rollback Procedure](#rollback-procedure)

---

## Overview

### What's New in v1.2.0-beta

INTcoin v1.2.0-beta introduces major new features:

- 📊 **Enhanced Mempool**: 6-level priority system (LOW, NORMAL, HIGH, HTLC, BRIDGE, CRITICAL)
- 📈 **Prometheus Metrics**: HTTP endpoint on port 9090 for monitoring
- 🔄 **Atomic Swaps**: HTLC-based cross-chain trading (BTC, LTC, XMR)
- 🌉 **Cross-Chain Bridges**: Lock-and-mint bridges for ETH, BTC, BSC
- 📱 **Mobile SPV Wallets**: Native iOS and Android wallets with Bloom filters
- ✅ **Enhanced Testing**: 17 test suites (up from 12)

### Migration Difficulty

| User Type | Difficulty | Estimated Time |
|-----------|-----------|----------------|
| **Full Node Operator** | Easy | 15-30 minutes |
| **Mining Pool Operator** | Medium | 1-2 hours |
| **Desktop Wallet User** | Easy | 10 minutes |
| **Exchange/Service** | Medium | 2-4 hours |

---

## Breaking Changes

### ⚠️ Important: Mempool Database Format

The mempool storage format has changed in v1.2.0-beta to support priority levels. Your existing mempool will be automatically migrated on first startup.

**Action Required**: None (automatic migration)

### ⚠️ New RPC Methods

23+ new RPC methods have been added. If you're using RPC automation, review [RPC.md](RPC.md) for the full list.

**Action Required**: Update RPC scripts to use new methods

### ⚠️ Prometheus Metrics Port

A new HTTP server listens on port 9090 by default for Prometheus metrics. If you're running a firewall, you may need to adjust rules.

**Action Required**: Update firewall rules if exposing metrics externally

### Non-Breaking Changes

✅ **Wallet Format**: No changes - existing wallets work without modification
✅ **Blockchain Data**: Fully compatible - no re-sync required
✅ **P2P Protocol**: Backward compatible with v1.0.0 nodes
✅ **Address Format**: No changes to Bech32 address encoding

---

## Migration Steps

### Step 1: Backup

**Critical**: Always backup before upgrading!

```bash
# Stop the node
intcoin-cli stop

# Wait for shutdown
sleep 10

# Backup wallet
cp ~/.intcoin/wallet.dat ~/intcoin-wallet-backup-$(date +%Y%m%d).dat

# Backup config
cp ~/.intcoin/intcoin.conf ~/intcoin-conf-backup-$(date +%Y%m%d).conf

# Optional: Backup entire data directory
tar -czf ~/intcoin-datadir-backup-$(date +%Y%m%d).tar.gz ~/.intcoin/
```

### Step 2: Update Binary

**From Pre-Built Binaries**:
```bash
# Download v1.2.0-beta
wget https://github.com/INT-devs/intcoin/releases/download/v1.2.0-beta/intcoin-1.2.0-linux-x86_64.tar.gz

# Extract
tar -xzf intcoin-1.2.0-linux-x86_64.tar.gz

# Install (requires sudo)
sudo install -m 0755 -o root -g root -t /usr/local/bin intcoin-1.2.0/bin/*

# Verify version
intcoind --version
# Should output: INTcoin Core version v1.2.0-beta
```

**From Source**:
```bash
# Pull latest code
cd ~/intcoin
git fetch origin
git checkout v1.2.0-beta

# Rebuild
cmake --build build --clean-first -j$(nproc)

# Install
sudo cmake --install build
```

### Step 3: Update Configuration

Add new optional configuration for v1.2.0 features:

```bash
# Edit config
nano ~/.intcoin/intcoin.conf
```

Add these lines:

```ini
# Enhanced Mempool (optional)
mempool.persist=1                  # Enable mempool persistence
mempool.persist_interval=300       # Save every 5 minutes

# Prometheus Metrics (optional)
metrics.enabled=1                  # Enable metrics HTTP server
metrics.bind=127.0.0.1            # Bind to localhost only
metrics.port=9090                  # Metrics port

# SPV Support (for mobile wallets connecting to this node)
spv.enabled=1                      # Enable SPV mode support
spv.bloom_filters=1                # Allow bloom filter connections
```

### Step 4: Restart Node

```bash
# Start node
intcoind -daemon

# Watch logs for migration messages
tail -f ~/.intcoin/debug.log
```

**Expected Log Output**:
```
2026-01-02 12:00:00 INTcoin Core version v1.2.0-beta
2026-01-02 12:00:01 Loading mempool from disk...
2026-01-02 12:00:01 Migrating mempool to v1.2.0 format...
2026-01-02 12:00:02 Mempool migration complete: 1523 transactions
2026-01-02 12:00:02 Starting Prometheus metrics server on 127.0.0.1:9090
2026-01-02 12:00:03 P2P listening on 0.0.0.0:9333
```

### Step 5: Verify Migration

```bash
# Check version
intcoin-cli getnetworkinfo | grep subversion

# Check blockchain height (should match pre-upgrade)
intcoin-cli getblockcount

# Check wallet balance (should match pre-upgrade)
intcoin-cli getbalance

# Test new RPC methods
intcoin-cli getmempoolinfo
intcoin-cli getmetricsinfo

# Test Prometheus endpoint
curl http://localhost:9090/metrics
```

---

## Configuration Changes

### New Configuration Options (v1.2.0)

#### Enhanced Mempool
```ini
mempool.persist=1                  # Enable mempool persistence (default: 0)
mempool.persist_interval=300       # Persist interval in seconds (default: 300)
mempool.max_size=300               # Max mempool size in MB (default: 300)
```

#### Prometheus Metrics
```ini
metrics.enabled=1                  # Enable metrics HTTP server (default: 0)
metrics.bind=127.0.0.1            # Bind address (default: 127.0.0.1)
metrics.port=9090                  # HTTP port (default: 9090)
metrics.threads=2                  # Worker threads (default: 2)
```

#### SPV Support
```ini
spv.enabled=1                      # Enable SPV support (default: 0)
spv.bloom_filters=1                # Allow bloom filters (default: 1 if spv.enabled)
spv.max_bloom_filters=1000         # Max bloom filters per connection (default: 1000)
```

#### Atomic Swaps
```ini
atomic_swaps.enabled=1             # Enable atomic swap support (default: 0)
atomic_swaps.chains=BTC,LTC,XMR   # Supported chains (default: all)
```

#### Bridges
```ini
bridges.enabled=1                  # Enable bridge support (default: 0)
bridges.chains=ETH,BTC,BSC        # Supported bridges (default: all)
```

---

## Database Migration

### Mempool Database

The mempool database format has changed to include priority information.

**Migration Process**:
1. **Automatic**: On first startup, v1.2.0 reads the old mempool format
2. **Assignment**: All existing transactions are assigned NORMAL priority
3. **Re-save**: Mempool is saved in new format with priority fields

**Rollback Note**: If you downgrade to v1.0.0, the mempool will be cleared (transactions will re-enter from network).

### Blockchain Database

**No changes** - blockchain database format is unchanged. No re-indexing required.

### Wallet Database

**No changes** - wallet.dat format is unchanged. Existing wallets work without modification.

---

## RPC API Changes

### New RPC Methods

**Enhanced Mempool**:
- `getmempoolinfo` - Enhanced with priority breakdown
- `setmempoolpriority <txid> <priority>` - Change transaction priority
- `savemempool` - Persist mempool to disk

**Prometheus Metrics**:
- `getmetricsinfo` - Metrics server status
- `getprometheusmetrics` - Get metrics in Prometheus format

**Atomic Swaps**:
- `initiate_atomic_swap` - Start atomic swap
- `redeem_atomic_swap` - Redeem swap
- `refund_atomic_swap` - Refund expired swap
- `list_atomic_swaps` - List active swaps
- `get_atomic_swap_status` - Get swap status

**Bridges**:
- `bridge_deposit` - Deposit to bridge
- `bridge_withdraw` - Withdraw from bridge
- `bridge_status` - Get bridge operation status
- `list_bridge_operations` - List operations

**SPV**:
- `loadbloomfilter` - Load bloom filter
- `getheaders` - Get block headers
- `getmerkleproof` - Get Merkle proof for transaction

See: [RPC.md](RPC.md), [API_REFERENCE.md](API_REFERENCE.md)

### Modified RPC Methods

**`getmempoolinfo`**: Now includes `priority_breakdown` field:
```json
{
  "size": 1523,
  "bytes": 456789,
  "priority_breakdown": {
    "LOW": 100,
    "NORMAL": 1200,
    "HIGH": 200,
    "HTLC": 15,
    "BRIDGE": 5,
    "CRITICAL": 3
  }
}
```

### Deprecated Methods

**None** - All v1.0.0 RPC methods remain functional.

---

## Mobile Wallet Migration

### New Mobile Wallets

INTcoin v1.2.0-beta introduces native mobile SPV wallets:

**iOS (Swift)**:
```swift
import INTcoinKit

// Migration from v1.0.0 seed
let mnemonic = "your 24 word seed phrase..."
let wallet = try Wallet.restore(mnemonic: mnemonic)

// Start syncing
wallet.startSync()
```

**Android (Kotlin)**:
```kotlin
import org.intcoin.sdk.Wallet

// Migration from v1.0.0 seed
val mnemonic = "your 24 word seed phrase..."
val wallet = Wallet.restore(mnemonic)

// Start syncing
wallet.startSync()
```

See: [Mobile SDK Documentation](MOBILE_SDK.md)

---

## Rollback Procedure

If you need to rollback to v1.0.0:

### Step 1: Stop Node
```bash
intcoin-cli stop
```

### Step 2: Restore Binary
```bash
# Reinstall v1.0.0 binary
sudo install -m 0755 -o root -g root -t /usr/local/bin ~/intcoin-1.0.0/bin/*
```

### Step 3: Restore Config
```bash
# Remove v1.2.0 config options
cp ~/intcoin-conf-backup-YYYYMMDD.conf ~/.intcoin/intcoin.conf
```

### Step 4: Clear Mempool (Optional)
```bash
# Remove v1.2.0 mempool
rm ~/.intcoin/mempool.dat
```

### Step 5: Restart
```bash
intcoind -daemon
```

**Note**: Wallet and blockchain data are compatible - no restoration needed.

---

## Troubleshooting

### Issue: "Mempool migration failed"

**Symptoms**: Error on startup about mempool corruption

**Solution**:
```bash
# Stop node
intcoin-cli stop

# Remove mempool
rm ~/.intcoin/mempool.dat

# Restart (mempool will repopulate from network)
intcoind -daemon
```

### Issue: "Port 9090 already in use"

**Symptoms**: Metrics server won't start

**Solution**:
```bash
# Check what's using port 9090
lsof -i :9090

# Change metrics port in config
echo "metrics.port=9091" >> ~/.intcoin/intcoin.conf

# Restart
intcoin-cli stop && intcoind -daemon
```

### Issue: "RPC method not found"

**Symptoms**: RPC automation scripts failing

**Solution**:
- Verify you're running v1.2.0: `intcoin-cli getnetworkinfo`
- Check RPC method exists: See [RPC.md](RPC.md)
- Update scripts to use new method names

---

## Additional Resources

- [Release Notes](../RELEASE_NOTES.md)
- [Changelog](../CHANGELOG.md)
- [RPC Documentation](RPC.md)
- [Configuration Guide](CONFIGURATION.md)
- [Prometheus Metrics](PROMETHEUS_METRICS.md)

---

## Support

**Questions?** Create an issue: https://github.com/INT-devs/intcoin/issues

**Security Issues?** See: [SECURITY.md](../SECURITY.md)

---

**Last Updated**: January 2, 2026
**Next Version**: v1.3.0 (Q2 2026)
