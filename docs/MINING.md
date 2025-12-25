# INTcoin Mining Documentation

**Version**: 1.0.0-alpha
**Last Updated**: December 25, 2025
**Status**: Complete - Production Ready

This document describes the mining system for INTcoin, including solo mining, pool mining, and the CPU miner implementation.

---

## Table of Contents

- [Overview](#overview)
- [Mining Algorithm](#mining-algorithm)
- [Architecture](#architecture)
- [Solo Mining](#solo-mining)
- [Pool Mining](#pool-mining)
- [Configuration](#configuration)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)

---

## Overview

INTcoin uses **RandomX** as its Proof-of-Work (PoW) algorithm, providing ASIC-resistant mining that favors CPUs with large caches and efficient random code execution.

**Key Features**:
- ✅ CPU-optimized (RandomX)
- ✅ ASIC-resistant
- ✅ Multi-threaded mining
- ✅ Solo and pool mining support
- ✅ Stratum protocol
- ✅ Real-time statistics

**Note**: GPU mining is not supported. RandomX is specifically designed to be CPU-optimal and GPU-resistant, making GPU mining unprofitable compared to CPU mining.

**Network Parameters**:
- **Block Time**: 2 minutes (120 seconds)
- **Initial Reward**: 105,113,636 INT
- **Halving Interval**: 1,051,200 blocks (~4 years)
- **Difficulty Adjustment**: Digishield V3 (every block)
- **Max Supply**: 221 Trillion INT

---

## Mining Algorithm

### RandomX

RandomX is a proof-of-work algorithm optimized for general-purpose CPUs. It uses:

- **Random code execution**: Generates random programs for each hash
- **Large dataset**: 2+ GB dataset for light mode, 8+ GB for fast mode
- **Memory-hard**: Requires significant RAM to operate efficiently
- **Cache-friendly**: Benefits from large CPU caches (L3)

**Advantages**:
- ASIC-resistant (economically unfeasible to build ASICs)
- Decentralized mining (anyone with a CPU can mine)
- Fair distribution
- Energy-efficient compared to SHA-256

**System Requirements**:
- **Minimum**: 2 GB RAM, 2-core CPU
- **Recommended**: 4 GB+ RAM, 4+ core CPU
- **Optimal**: 8 GB+ RAM, 8+ core CPU with large L3 cache

---

## Architecture

### Components

```
┌─────────────────────────────────────────────────┐
│              Mining Manager                      │
│  - Coordinates mining threads                    │
│  - Manages jobs and results                     │
│  - Statistics tracking                          │
└──────────────┬──────────────────────────────────┘
               │
    ┌──────────┴──────────┐
    │                     │
┌───▼────┐           ┌───▼────┐
│ Thread │           │ Thread │
│   1    │    ...    │   N    │
└────────┘           └────────┘
```

### Miner Thread

Each thread:
1. Receives mining job (block template)
2. Tries range of nonces
3. Computes RandomX hash for each nonce
4. Checks if hash meets difficulty target
5. Reports results (block found or hashes done)

### Mining Manager

Responsibilities:
- Initialize RandomX cache/dataset
- Create and manage worker threads
- Distribute jobs to threads
- Collect and report statistics
- Handle block/share submissions

---

## Solo Mining

Solo mining means mining directly to your own wallet without a pool.

### Prerequisites

1. **Running Node**: `intcoind` must be running
2. **Mining Address**: Valid INT address to receive rewards
3. **RPC Access**: RPC credentials for intcoind

### Start Solo Mining

```bash
# Method 1: Using standalone miner (recommended)
./intcoin-miner \\
    --solo \\
    --address=int1qyouraddress... \\
    --rpc-user=myuser \\
    --rpc-password=mypass \\
    --threads=4

# Method 2: Using getblocktemplate RPC directly
./intcoin-cli -rpcuser=myuser -rpcpassword=mypass \\
    getblocktemplate '{"rules":["segwit"]}'

# The miner uses getblocktemplate internally to fetch work
# and submitblock to submit solved blocks
```

### Configuration

Create `miner.conf`:

```conf
# Solo Mining Configuration
mining-address=int1qyouraddress...
threads=4
daemon-host=127.0.0.1
daemon-port=2211
rpc-user=myuser
rpc-password=mypass
update-interval=5
```

### Solo Mining Rewards

- Block reward goes directly to your address
- No pool fees
- Variance: May take longer to find blocks
- Full reward when block is found

**Example**:
```
Block 1234 found!
Reward: 105,113,636 INT
Address: int1qyouraddress...
Time: 2025-12-03 14:30:00
```

---

## Pool Mining

Pool mining combines hashrate with other miners for more consistent rewards.

### Benefits

- ✅ Regular, predictable payouts
- ✅ Lower variance
- ✅ No need to run full node
- ✅ Suitable for smaller miners

### Pool Selection

Consider:
- **Fee**: Typical 1-3%
- **Location**: Choose nearby pool for lower latency
- **Size**: Balance between stability and decentralization
- **Payout**: Minimum payout threshold
- **Reputation**: Established pools preferred

### Start Pool Mining

```bash
./intcoin-miner \\
    --pool \\
    --pool-host=pool.international-coin.org \\
    --pool-port=3333 \\
    --pool-user=yourworker \\
    --pool-pass=x \\
    -t 4
```

### Stratum Protocol

INTcoin uses the Stratum mining protocol:

**Features**:
- Efficient communication
- Low latency
- Share-based rewards
- Difficulty adjustment per worker
- Job notifications in real-time

**Message Flow**:
```
Miner → Pool: mining.subscribe
Pool → Miner: session_id, extranonce

Miner → Pool: mining.authorize
Pool → Miner: authorization result

Pool → Miner: mining.notify (job)
Miner: [mines job]
Miner → Pool: mining.submit (share)
Pool → Miner: share accepted/rejected
```

---

## Configuration

### Mining Config Options

| Option | Description | Default |
|--------|-------------|---------|
| `-t, --threads` | Number of threads | Auto-detect |
| `-a, --address` | Mining address | Required (solo) |
| `--testnet` | Use testnet | false |
| `--daemon-host` | intcoind host | 127.0.0.1 |
| `--daemon-port` | intcoind RPC port | 2211 |
| `--rpc-user` | RPC username | - |
| `--rpc-password` | RPC password | - |
| `--pool` | Enable pool mining | false |
| `--pool-host` | Pool hostname | - |
| `--pool-port` | Pool port | 3333 |
| `--pool-user` | Pool username/worker | - |
| `--pool-pass` | Pool password | x |
| `--affinity` | CPU affinity | false |
| `--batch-size` | Nonces per batch | 100 |
| `--update-interval` | Stats interval (sec) | 5 |

### Thread Configuration

**Auto-detect** (recommended):
```bash
./intcoin-miner -a int1qaddr...
# Uses all available CPU threads
```

**Manual**:
```bash
./intcoin-miner -a int1qaddr... -t 4
# Uses exactly 4 threads
```

**Optimal thread count**:
- Desktop: All cores (8-16 threads typical)
- Server: All cores (may be 32-128 threads)
- Laptop: Half cores (to prevent overheating)
- Shared system: Leave 1-2 cores free

### CPU Affinity

Binding threads to specific CPU cores can improve performance:

```bash
./intcoin-miner -a int1qaddr... --affinity
```

**Benefits**:
- Better cache utilization
- Reduced context switching
- More consistent hashrate

---

## Performance

### Hashrate Expectations

**CPU Performance** (approximate):

| CPU | Cores | Cache | Hashrate |
|-----|-------|-------|----------|
| Intel i5-10400 | 6 | 12 MB | 1.2-1.5 KH/s |
| Intel i7-10700K | 8 | 16 MB | 2.0-2.5 KH/s |
| AMD Ryzen 5 5600X | 6 | 32 MB | 3.5-4.5 KH/s |
| AMD Ryzen 7 5800X | 8 | 32 MB | 5.0-6.5 KH/s |
| AMD Ryzen 9 5950X | 16 | 64 MB | 10-13 KH/s |
| AMD EPYC 7763 | 64 | 256 MB | 40-50 KH/s |

**Factors affecting hashrate**:
- CPU architecture (newer = better)
- Cache size (larger L3 = much better)
- RAM speed (DDR4-3200+ recommended)
- Thermal throttling (good cooling essential)
- Background processes (close unnecessary apps)

### Optimization Tips

**1. Hardware**:
- Enable XMP/DOCP for faster RAM
- Ensure good cooling (CPUs run hot while mining)
- Use performance power plan (not balanced/power saver)
- Consider undervolting for efficiency

**2. Software**:
- Close unnecessary applications
- Disable Windows background apps
- Use performance mode in BIOS
- Update CPU microcode/BIOS

**3. Mining Config**:
```bash
# Optimal configuration example
./intcoin-miner \\
    -a int1qaddr... \\
    -t 16 \\              # All cores
    --affinity \\        # CPU affinity
    --batch-size=500 \\  # Larger batches
    --update-interval=30 # Less frequent updates
```

### Power Consumption

Mining is CPU-intensive and consumes significant power:

| System | Power Draw | Cost/Day (USD) |
|--------|------------|----------------|
| Desktop (100W) | 2.4 kWh | $0.30-0.50 |
| Workstation (250W) | 6.0 kWh | $0.75-1.25 |
| Server (500W) | 12.0 kWh | $1.50-2.50 |

*Assumes $0.12/kWh electricity cost*

**Calculate profitability**:
```
Daily Revenue = (Your Hashrate / Network Hashrate) × Daily Blocks × Block Reward
Daily Profit = Daily Revenue - Power Cost
```

---

## Troubleshooting

### Low Hashrate

**Problem**: Hashrate lower than expected

**Solutions**:
1. Check CPU temperature (thermal throttling)
2. Close background applications
3. Verify correct number of threads
4. Enable CPU affinity
5. Update BIOS/microcode
6. Check RAM speed (enable XMP)

### High Rejected Shares (Pool Mining)

**Problem**: Many shares rejected by pool

**Causes**:
- High network latency
- Stale jobs (too slow to submit)
- Wrong difficulty

**Solutions**:
1. Choose closer pool (lower ping)
2. Reduce batch size
3. Check internet connection
4. Verify pool configuration

### Miner Crashes

**Problem**: Miner stops unexpectedly

**Solutions**:
1. Check RAM (RandomX needs 2-8 GB)
2. Verify intcoind is running (solo mining)
3. Check RPC credentials
4. Review error logs
5. Ensure stable power supply

### Connection Issues

**Problem**: Cannot connect to pool/daemon

**Solutions**:
1. Verify hostname/IP address
2. Check port number (2211 for daemon, 3333 typical for pool)
3. Verify firewall settings
4. Test with telnet/nc
5. Check RPC credentials

---

## Statistics

### Real-time Stats

The miner displays:

```
========================================
[Mining] Hashrate: 5.23 KH/s | Blocks: 0 | Uptime: 3600s
========================================
```

**Metrics**:
- **Hashrate**: Current hash rate
- **Blocks**: Blocks found (solo mining)
- **Shares**: Shares submitted (pool mining)
- **Accepted/Rejected**: Share acceptance rate
- **Uptime**: Total mining time

### Final Statistics

When stopping:

```
========================================
Mining Statistics:
========================================
Total Hashes: 18,828,000
Blocks Found: 1
Shares Submitted: 1,234
Shares Accepted: 1,220
Shares Rejected: 14
Average Hashrate: 5.23 KH/s
Uptime: 3600 seconds
========================================
```

---

## Implementation Status

**Current Status**: ✅ 100% Complete - Production Ready

**Completed** ✅:
- Mining header (mining.h) with full API
- Multi-threaded mining manager architecture
- Stratum client implementation
- Mining statistics tracking
- Standalone miner executable (intcoin-miner.cpp)
- **GetBlockTemplate RPC** - Block template generation with coinbase creation
- **Solo Mining** - Direct mining to daemon using getblocktemplate/submitblock
- **Pool Mining** - Full Stratum v1 protocol support
- **Mining Pool Server** - Complete pool implementation (see POOL_SETUP.md)
- CMake build integration
- Documentation

**Ready for Deployment**:
- ✅ Solo miners can mine blocks to their addresses
- ✅ Pool operators can run mining pools
- ✅ Pool miners can connect and receive work
- ✅ Block rewards distributed correctly
- ✅ Variable difficulty working
- ✅ Share validation functional

---

## See Also

- [CONSENSUS.md](CONSENSUS.md) - RandomX and difficulty adjustment
- [RPC.md](RPC.md) - RPC API for mining commands
- [Mining Guide](../wiki/Mining-Guide.md) - User-friendly mining guide

---

**Last Updated**: December 25, 2025
**Status**: Complete - GetBlockTemplate, solo mining, and pool server fully operational
