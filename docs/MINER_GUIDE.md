# INTcoin Standalone Miner Guide

## Overview

The INTcoin standalone miner is a high-performance CPU miner using RandomX proof-of-work algorithm. It supports both solo and pool mining with advanced features for optimal performance.

---

## Features

### Core Features
✅ **Multi-threaded mining** - Auto-detects optimal thread count
✅ **Solo mining** - Mine directly to intcoind
✅ **Pool mining** - Stratum protocol support
✅ **Pool failover** - Automatic failover to backup pools
✅ **Configuration files** - Easy setup with .conf files
✅ **Enhanced logging** - Detailed logs with timestamps
✅ **Real-time statistics** - Hashrate, shares, acceptance rate
✅ **CPU affinity** - Pin threads to specific cores
✅ **Benchmark mode** - Test hardware performance

### Performance Features
- **RandomX optimizations** - Hardware-specific tuning
- **Huge pages support** - Improved cache efficiency
- **Batch processing** - Configurable nonce batching
- **Priority control** - CPU priority management

---

## Installation

### Build from Source

```bash
cd intcoin
mkdir build && cd build
cmake ..
make intcoin-miner
```

Binary location: `build/bin/intcoin-miner`

---

## Quick Start

### Solo Mining

```bash
./intcoin-miner \
  --address=int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh \
  --rpc-user=user \
  --rpc-password=pass \
  --threads=4
```

### Pool Mining

```bash
./intcoin-miner \
  --pool \
  --pool-host=pool.international-coin.org \
  --pool-port=3333 \
  --pool-user=worker1 \
  --threads=4
```

### Using Configuration File

```bash
# Generate sample config
./intcoin-miner --generate-config miner.conf

# Edit miner.conf with your settings

# Run miner
./intcoin-miner --config=miner.conf
```

---

## Configuration File

### Sample miner.conf

```ini
# INTcoin Miner Configuration

# General Settings
threads=4
address=int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh
testnet=false
update_interval=5
verbose=false
log_file=miner.log

# Solo Mining
daemon_host=127.0.0.1
daemon_port=2211
rpc_user=user
rpc_password=pass

# Pool Mining
pool=true
pool_url=pool.international-coin.org:3333
pool_url=backup.international-coin.org:3333
pool_user=worker1
pool_pass=x

# Performance
batch_size=100
affinity=true
priority=0
huge_pages=false

# Advanced
benchmark=false
```

---

## Command Line Options

### General Options
```
-h, --help              Show help message
-v, --version           Show version
-c, --config=<file>     Load configuration file
--generate-config       Generate sample config file
-t, --threads=<n>       Number of threads (0=auto)
-a, --address=<addr>    Mining address
--testnet               Use testnet
--verbose               Enable verbose logging
--log=<file>            Log file path
```

### Solo Mining Options
```
--daemon-host=<host>    intcoind RPC host (default: 127.0.0.1)
--daemon-port=<port>    intcoind RPC port (default: 2211)
--rpc-user=<user>       RPC username
--rpc-password=<pass>   RPC password
--retry-delay=<sec>     Retry delay on connection failure (default: 5)
```

### Pool Mining Options
```
--pool                  Enable pool mining
--pool-host=<host>      Pool hostname
--pool-port=<port>      Pool port (default: 3333)
--pool-user=<user>      Worker name
--pool-pass=<pass>      Worker password (default: x)
--failover              Enable pool failover (default: true)
--keepalive=<sec>       Pool keepalive interval (default: 60)
```

### Performance Options
```
--affinity              Enable CPU affinity
--batch-size=<n>        Nonces per batch (default: 100)
--priority=<n>          CPU priority (0=normal, 1=below, 2=idle)
--huge-pages            Enable huge pages (requires root)
--update-interval=<n>   Stats update interval in seconds (default: 5)
```

### Advanced Options
```
--benchmark             Benchmark mode (60 seconds)
--benchmark-duration=<n> Benchmark duration in seconds
--max-retries=<n>       Maximum connection retries (default: 3)
--retry-pause=<n>       Pause between retries in seconds (default: 5)
```

---

## Pool Mining

### Supported Pool Software
- **intcoin-pool** - Official INTcoin pool software
- **Stratum compatible** - Works with standard Stratum protocol

### Pool URL Format
```
pool.example.com:3333
192.168.1.100:3333
```

### Multiple Pools (Failover)
Add multiple pool_url entries in config file:
```ini
pool_url=pool1.international-coin.org:3333
pool_url=pool2.international-coin.org:3333
pool_url=pool3.international-coin.org:3333
```

The miner will automatically failover to the next pool if connection is lost.

---

## Performance Tuning

### Thread Count

**Auto-detect** (recommended):
```bash
--threads=0
```

**Manual (CPU cores minus 1)**:
```bash
# For 8-core CPU
--threads=7
```

### CPU Affinity

Pins threads to specific CPU cores for better cache locality:
```bash
--affinity
```

### Huge Pages

Improves RandomX performance (requires root/administrator):

**Linux**:
```bash
sudo sysctl -w vm.nr_hugepages=3072
sudo ./intcoin-miner --huge-pages ...
```

**Windows** (Run as Administrator):
```
intcoin-miner.exe --huge-pages ...
```

### Priority Settings

```bash
--priority=0  # Normal priority (default)
--priority=1  # Below normal (less CPU impact)
--priority=2  # Idle priority (background mining)
```

### Batch Size

Larger batch sizes reduce overhead but increase latency:
```bash
--batch-size=100  # Default, good balance
--batch-size=1000 # Better for high-end CPUs
--batch-size=10   # Better for low-end CPUs
```

---

## Benchmarking

Test your hardware performance:

```bash
./intcoin-miner --benchmark --threads=4

# Or via config
benchmark=true
benchmark_duration=60
```

Output example:
```
========================================
Benchmark Results
========================================
Duration: 60 seconds
Threads: 4
Total Hashes: 3,600,000
Average Hashrate: 60.00 KH/s
Peak Hashrate: 62.45 KH/s
========================================
```

---

## Monitoring

### Real-time Statistics

The miner displays periodic updates:

```
[2025-12-23 15:30:45.123] [INFO]    Hashrate: 58.32 KH/s | Avg: 57.89 KH/s | Total: 3.45M hashes
[2025-12-23 15:30:50.456] [INFO]    Share accepted | Total: 42/45 (93.3%)
```

### Log Files

Enable logging to file:
```bash
--log=miner.log
```

Log format:
```
[2025-12-23 15:30:45.123] [INFO]    Starting miner...
[2025-12-23 15:30:45.456] [INFO]    Connected to pool: pool.international-coin.org
[2025-12-23 15:30:45.789] [DEBUG]   New job: abc123 | Height: 12345
[2025-12-23 15:31:12.345] [INFO]    Share accepted | Total: 1/1 (100.0%)
```

---

## Troubleshooting

### Connection Issues

**Problem**: "Failed to connect to daemon"
```bash
# Check if intcoind is running
./intcoind getinfo

# Check RPC credentials
./intcoind getinfo --rpcuser=user --rpcpassword=pass
```

**Problem**: "Failed to connect to pool"
```bash
# Test pool connectivity
telnet pool.international-coin.org 3333

# Check firewall settings
# Ensure port 3333 is open
```

### Performance Issues

**Low hashrate**:
1. Enable CPU affinity: `--affinity`
2. Enable huge pages: `--huge-pages`
3. Adjust thread count: `--threads=<optimal>`
4. Increase batch size: `--batch-size=1000`

**High rejection rate**:
1. Check network latency to pool
2. Reduce batch size: `--batch-size=10`
3. Increase update interval: `--update-interval=10`

### Error Messages

**"Invalid mining address"**
- Verify address format starts with `int1` (mainnet) or `intc1` (testnet)
- Check address checksum

**"Unauthorized"**
- Verify RPC credentials
- Check intcoind RPC settings

**"Job timeout"**
- Pool may be overloaded
- Try failover pool
- Check network connection

---

## Security

### Best Practices

1. **Use strong RPC passwords**
   ```ini
   rpc_password=veryStrongPassword123!@#
   ```

2. **Restrict RPC access**
   ```bash
   # intcoind configuration
   rpcallowip=127.0.0.1
   rpcbind=127.0.0.1
   ```

3. **Use encrypted connections**
   - Enable SSL/TLS if pool supports it

4. **Monitor resource usage**
   ```bash
   # Linux
   top -p $(pgrep intcoin-miner)

   # Windows
   Task Manager > Details > intcoin-miner.exe
   ```

5. **Keep software updated**
   ```bash
   git pull
   cmake --build build --target intcoin-miner
   ```

---

## Examples

### Home Mining Setup

```ini
# miner.conf
threads=6
address=int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh
pool=true
pool_url=pool.international-coin.org:3333
pool_user=home-miner-1
affinity=true
priority=1
update_interval=10
log_file=miner.log
```

### Mining Farm Setup

```ini
# farm-miner.conf
threads=0  # Auto-detect
pool=true
pool_url=pool1.company.com:3333
pool_url=pool2.company.com:3333
pool_url=pool3.company.com:3333
pool_user=farm-worker-001
affinity=true
huge_pages=true
batch_size=1000
update_interval=5
verbose=true
log_file=/var/log/intcoin/miner.log
```

### Testnet Mining

```bash
./intcoin-miner \
  --testnet \
  --address=intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh \
  --daemon-port=12211 \
  --rpc-user=test \
  --rpc-password=test \
  --threads=2
```

---

## API Reference

### Enhanced Logger API

```cpp
#include "miner_logger.h"

MinerLogger logger("miner.log", true);  // verbose=true

logger.Info("Starting miner");
logger.Debug("Detailed debug info");
logger.Warning("Pool connection slow");
logger.Error("Failed to connect");

logger.LogHashrate(1000000, 58.3, 57.9);
logger.LogBlockFound(12345, "abc123...", 54321);
logger.LogShareAccepted(42, 3);
logger.LogPoolConnected("pool.example.com", "worker1");
```

### Configuration API

```cpp
#include "miner_config.h"

MinerConfig config;

// Load from file
if (!config.LoadFromFile("miner.conf")) {
    std::cerr << "Failed to load config\n";
}

// Generate sample
MinerConfig::GenerateSampleConfig("miner-sample.conf");

// Save current config
config.SaveToFile("miner.conf");
```

---

## Performance Benchmarks

### Hardware Performance (RandomX)

| CPU | Cores | Hashrate | Power | Efficiency |
|-----|-------|----------|-------|------------|
| AMD Ryzen 9 5950X | 16 | ~18 KH/s | 142W | 126 H/W |
| Intel i9-12900K | 16 | ~16 KH/s | 125W | 128 H/W |
| AMD Ryzen 7 5800X | 8 | ~10 KH/s | 105W | 95 H/W |
| Intel i7-11700K | 8 | ~8 KH/s | 125W | 64 H/W |
| AMD Ryzen 5 5600X | 6 | ~7 KH/s | 65W | 108 H/W |

*Note: Actual performance varies based on RAM speed, configuration, and system load*

---

## FAQ

**Q: Can I mine on a laptop?**
A: Yes, but use lower priority and fewer threads to avoid overheating.

**Q: Does GPU mining work?**
A: RandomX is optimized for CPUs. GPUs are not efficient for RandomX.

**Q: What's the minimum hardware requirement?**
A: 4GB RAM, dual-core CPU minimum. 16GB RAM recommended for optimal performance.

**Q: How much can I earn?**
A: Earnings depend on network hashrate, block reward, and your hardware. Use a mining calculator.

**Q: Can I mine to an exchange address?**
A: Not recommended. Use a personal wallet for better security.

**Q: What's the difference between solo and pool mining?**
A: Solo mining: Mine blocks directly, infrequent but larger rewards.
   Pool mining: Share work with others, frequent smaller rewards.

---

## Support

- **Documentation**: https://github.com/intcoin/intcoin/tree/main/docs
- **Discord**: https://discord.gg/intcoin
- **Forum**: https://forum.international-coin.org
- **Issue Tracker**: https://github.com/intcoin/intcoin/issues

---

**Last Updated**: December 23, 2025
**Miner Version**: 1.0.0-alpha
**Protocol**: Stratum v1
