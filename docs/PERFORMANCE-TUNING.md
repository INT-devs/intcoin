# INTcoin Performance Tuning Guide

**Version:** 1.0
**Status:** Production Ready
**Last Updated:** November 25, 2025

## Overview

This guide provides comprehensive performance optimization strategies for INTcoin nodes and mining operations.

## Table of Contents

- [Performance Benchmarking](#performance-benchmarking)
- [Optimization Targets](#optimization-targets)
- [Configuration Tuning](#configuration-tuning)
- [Hardware Requirements](#hardware-requirements)
- [Monitoring & Profiling](#monitoring--profiling)

## Performance Benchmarking

### 8 Benchmark Suites

**1. Cryptographic Operations**
```
Target Metrics:
- Dilithium5 Sign: 500+ ops/sec
- Dilithium5 Verify: 1,000+ ops/sec
- Kyber1024 Encap: 800+ ops/sec
- Kyber1024 Decap: 800+ ops/sec
- SHA3-256 Hash: 50,000+ ops/sec

Typical Results:
- Modern CPU (AVX2): 600-800 sign/sec
- With optimization: 1,000+ verify/sec
```

**2. Transaction Processing**
```
Target Metrics:
- Parse: 100,000+ ops/sec
- Validate: 50,000+ ops/sec
- Fee Calc: 200,000+ ops/sec

Optimization:
- Inline validation checks
- Cache UTXO lookups
- Batch fee calculations
```

**3. Mining Performance**
```
Target Metrics:
- PoW Check: 100,000+ checks/sec
- Difficulty Calc: <1 ms
- Block Template: 100+ templates/sec
```

**4. Smart Contract Execution**
```
Target Metrics:
- Gas: 1,000+ units/sec
- Opcode: 10,000+ ops/sec
- State Update: 5,000+ ops/sec
```

**5. Lightning Network**
```
Target Metrics:
- Channel Open: 100+ ops/sec
- Payment Routing: 1,000+ routes/sec
- HTLC Resolution: 500+ ops/sec
```

**6. Bridge Operations**
```
Target Metrics:
- Atomic Swap: 50+ ops/sec
- Proof Verify: 100+ ops/sec
```

**7. Network Performance**
```
Target Metrics:
- P2P Throughput: 100+ MB/sec
- Connection: <500ms establishment
- Message Propagation: <100ms average
```

**8. Memory Usage**
```
Target Metrics:
- Blockchain Sync: 2 GB baseline
- UTXO Set: <500 MB
- Mempool: <300 MB
- Cache: <100 MB

Total: ~3 GB typical
```

## Optimization Targets

### Priority 1: Critical Path

**Consensus Validation:**
```
Target: Process 1 block/5 sec (network speed)
Current: 10+ blocks/sec (achievable on modern CPU)
Optimization: Focus on P2P propagation, not validation

Key Metrics:
- Block validation: <50ms (95th percentile)
- TX validation: <1ms per TX
- PoW check: <1ms per block
```

**Transaction Validation:**
```
Target: 1,000+ TXs/sec throughput
Current: 2,000+ TXs/sec (achievable)
Optimization: Cache UTXO access, batch signature verification

Key Metrics:
- UTXO lookup: O(1) average
- Signature verify: <1ms per signature
- Script execution: <10ms max
```

### Priority 2: Network Performance

**P2P Message Propagation:**
```
Target: <100ms average to all peers
Current: 50-200ms typical
Optimization: Message compression, connection pooling

Key Metrics:
- Block propagation: <1 second network-wide
- TX propagation: <500ms network-wide
- Peer connection: <2 seconds average
```

### Priority 3: Storage & Memory

**Database Performance:**
```
Target: 1,000+ operations/sec
Current: 5,000+ operations/sec achievable
Optimization: RocksDB tuning, cache size

Key Metrics:
- Read latency: <1ms (p99)
- Write latency: <10ms (p99)
- Compaction: Background process only
```

## Configuration Tuning

### Database Optimization

```ini
# RocksDB Configuration
[database]
# Cache settings
block_cache_size = 134217728          # 128 MB
write_buffer_size = 67108864          # 64 MB
max_write_buffer_number = 2           # Allow 2 in-memory buffers

# Compaction
level0_file_num_compaction_trigger = 4
max_bytes_for_level_base = 268435456  # 256 MB
target_file_size_base = 67108864      # 64 MB

# Performance
enable_compression = true
compression_type = lz4                # Fast compression

# Tuning
use_direct_reads = true               # Bypass OS cache
use_direct_io_for_flush_and_compaction = true
```

### Network Optimization

```ini
[network]
# Connection limits
max_inbound_connections = 125
max_outbound_connections = 8
max_connections_per_ip = 8

# Message handling
max_message_size = 33554432           # 32 MB
message_buffer_size = 2097152         # 2 MB

# Rate limiting
rate_limit_mbps = 100                 # 100 MB/sec per connection
messages_per_second = 100             # Per-peer limit

# Timeouts
connection_timeout = 30000            # 30 seconds
read_timeout = 120000                 # 120 seconds
write_timeout = 120000                # 120 seconds
```

### Memory Pool Configuration

```ini
[mempool]
# Size limits
max_mempool_size = 314572800          # 300 MB
max_orphan_pool_size = 104857600      # 100 MB

# Transaction limits
max_tx_size = 1048576                 # 1 MB
min_relay_fee = 0.0001                # Per satoshi

# Eviction
eviction_percentage = 10              # Remove 10% when full
min_eviction_age = 600                # Don't evict recent TXs

# Performance
use_ancestor_index = true             # Speed up lookups
ancestor_limit = 25
descendant_limit = 25
```

### Consensus Optimization

```ini
[consensus]
# Caching
pow_cache_size = 134217728            # 128 MB
block_validity_cache = 67108864       # 64 MB

# Validation
parallel_validation = true
validation_threads = 8

# Difficulty
difficulty_cache_interval = 10        # Cache for 10 blocks
```

### Mining Optimization

```ini
[mining]
# Template generation
block_template_cache_size = 10        # Cache 10 templates
template_reuse_valid_for = 5000       # 5 seconds

# Submission
submit_stale_blocks = false
submit_minimum_difficulty = 0.0001

# Pool
pool_connection_timeout = 60000       # 60 seconds
```

## Hardware Requirements

### Minimum Specifications

**CPU:**
- Cores: 2+ cores
- Speed: 2 GHz+
- Instructions: SSE2 (or better for crypto)
- Example: Intel Core i5 or AMD Ryzen 3

**Memory:**
- RAM: 4 GB minimum
- Swap: 2 GB recommended
- Allocation: 2 GB for node, 2 GB free

**Storage:**
- Type: SSD (SATA or NVMe)
- Capacity: 50 GB minimum (blockchain growth)
- Speed: 500+ MB/sec read/write
- Example: 500GB SSD

**Network:**
- Bandwidth: 1 Mbps minimum
- Connection: Stable, low-latency preferred
- Ports: 8338-8344 for P2P + RPC

### Recommended Specifications

**CPU:**
- Cores: 4+ cores
- Speed: 3+ GHz
- Instructions: AVX2 (for cryptography)
- Example: Intel Core i7/i9 or AMD Ryzen 5/7

**Memory:**
- RAM: 8-16 GB
- Allocation: 2 GB for node, rest available

**Storage:**
- Type: NVMe SSD (PCIe 3.0+)
- Capacity: 500 GB - 1 TB
- Speed: 2,000+ MB/sec
- Example: Samsung 970 EVO or similar

**Network:**
- Bandwidth: 10+ Mbps
- Connection: High-speed, low-latency
- Dedicated line recommended

### High-Performance Setup

**CPU:**
- Cores: 8+ cores
- Speed: 4+ GHz
- Instructions: AVX-512 (if available)
- Example: Intel Xeon or AMD Ryzen Threadripper

**Memory:**
- RAM: 32+ GB
- SSD Cache: 10+ GB available

**Storage:**
- Type: Enterprise NVMe SSD
- Capacity: 2 TB+
- Speed: 5,000+ MB/sec
- Setup: RAID for redundancy

**Network:**
- Bandwidth: 100+ Mbps
- Connection: Fiber or dedicated circuit
- Multiple connections for failover

## Monitoring & Profiling

### Performance Metrics

**CPU Utilization:**
```
Monitor with: top, htop, sar
Target: <80% average
Alert: >95% for >10 minutes

Optimization: Profile and optimize hot paths
```

**Memory Usage:**
```
Monitor with: free, top, ps
Target: <50% of available
Alert: >80% usage

Optimization: Increase cache sizes, reduce mempool
```

**Disk I/O:**
```
Monitor with: iostat, iotop, dstat
Target: <50% utilization
Alert: >90% I/O wait

Optimization: Use faster SSD, increase cache
```

**Network I/O:**
```
Monitor with: iftop, nethogs, vnstat
Target: <50% bandwidth capacity
Alert: >80% usage

Optimization: Reduce peer connections, compress messages
```

### Profiling Tools

**CPU Profiling:**
```bash
# Using perf
sudo perf record -g ./intcoind
sudo perf report

# Using gperftools
env CPUPROFILE=cpu.prof ./intcoind
pprof ./intcoind cpu.prof

# Using Valgrind
valgrind --tool=callgrind ./intcoind
kcachegrind callgrind.out.*
```

**Memory Profiling:**
```bash
# Using Valgrind
valgrind --leak-check=full ./intcoind

# Using massif
valgrind --tool=massif ./intcoind
ms_print massif.out.*
```

**Network Analysis:**
```bash
# Packet capture
tcpdump -i any -w intcoin.pcap 'port 8338'
wireshark intcoin.pcap

# Connection analysis
ss -s                    # Summary
ss -tnap | grep intcoind # Active connections
```

### Performance Dashboard

**Key Metrics to Track:**
```
Real-time:
- TPS (Transactions per second)
- Blocks/hour
- Mempool size
- P2P connections
- CPU/Memory/Disk/Network utilization

Historical:
- Average TPS by hour/day/week
- Network growth
- Hardware performance trends
- Optimization impact
```

## Optimization Workflow

### 1. Baseline Measurement
```
1. Run benchmarks without optimization
2. Record CPU/Memory/Disk/Network metrics
3. Identify bottlenecks
4. Profile hot code paths
```

### 2. Optimization
```
1. Apply configuration changes (non-code)
2. Run benchmarks
3. If improvement: keep change, else revert
4. Repeat for all parameters

5. If needed: code optimization
   - Profile to identify hot paths
   - Optimize algorithm/implementation
   - Measure improvement
```

### 3. Validation
```
1. Run full benchmark suite
2. Compare with baseline
3. Ensure no regressions
4. Test with realistic load
```

### 4. Deployment
```
1. Deploy to testnet
2. Monitor for 1 week
3. If stable: deploy to mainnet
4. Continue monitoring
```

## Resources

- [Benchmark Source Code](../tests/benchmark/)
- [Performance Results](STRESS_TEST_RESULTS.md)
- [Testing Guide](TESTING.md)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Initial performance tuning guide with 8 benchmark suites |

---

**License**: MIT License
**Copyright**: (c) 2025 INTcoin Core
