# INTcoin Mining Guide

**Version**: 1.0
**Last Updated**: December 3, 2025
**For INTcoin v1.0.0+**

Welcome to INTcoin mining! This guide will help you start mining INTcoin using your computer's CPU.

---

## Table of Contents

- [Introduction](#introduction)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Getting Started](#getting-started)
- [Solo Mining](#solo-mining)
- [Pool Mining](#pool-mining)
- [Optimizing Performance](#optimizing-performance)
- [Monitoring Your Miner](#monitoring-your-miner)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

---

## Introduction

### What is Mining?

Mining is the process of validating transactions and creating new blocks on the INTcoin blockchain. Miners use computing power to solve complex cryptographic puzzles, and successful miners are rewarded with newly created INTcoin.

### RandomX Algorithm

INTcoin uses **RandomX**, a CPU-optimized proof-of-work algorithm that is:
- **ASIC-resistant**: Designed to favor CPUs over specialized mining hardware
- **Fair**: Anyone with a regular computer can participate
- **Secure**: Provides strong blockchain security through computational work

### Block Rewards

- **Initial Reward**: 105,113,636 INT per block
- **Block Time**: 2 minutes
- **Halving**: Every 1,051,200 blocks (~4 years)
- **Total Supply**: 221 Trillion INT over ~256 years

---

## System Requirements

### Minimum Requirements

| Component | Specification |
|-----------|--------------|
| **CPU** | 2-core processor |
| **RAM** | 2 GB available |
| **Storage** | 10 GB free space |
| **Internet** | Stable connection |
| **OS** | macOS 11+, Linux, FreeBSD, Windows 10+ |

### Recommended Requirements

| Component | Specification |
|-----------|--------------|
| **CPU** | 4+ core processor with large L3 cache |
| **RAM** | 4-8 GB available |
| **Storage** | 50 GB+ free space |
| **Internet** | Fast, reliable connection |
| **Cooling** | Good airflow/cooling for sustained loads |

### Best CPUs for Mining

RandomX performs best on CPUs with large caches. Here are some examples:

**Budget-Friendly:**
- AMD Ryzen 5 5600X
- Intel Core i5-10400

**High Performance:**
- AMD Ryzen 9 5950X
- AMD Ryzen 7 5800X3D
- AMD Threadripper series

**Professional:**
- AMD EPYC server CPUs

**Note**: AMD Ryzen processors generally outperform Intel for RandomX due to larger L3 caches.

---

## Installation

### macOS Installation

1. **Download INTcoin**:
   ```bash
   # Download from official website or build from source
   git clone https://gitlab.com/intcoin/crypto.git
   cd crypto
   ```

2. **Install Dependencies**:
   ```bash
   brew install cmake boost rocksdb randomx liboqs
   ```

3. **Build INTcoin**:
   ```bash
   mkdir build && cd build
   cmake .. -DBUILD_MINER=ON
   make -j$(sysctl -n hw.ncpu)
   ```

4. **Verify Installation**:
   ```bash
   ./intcoin-miner --version
   ```

### Linux Installation

1. **Install Dependencies** (Ubuntu/Debian):
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libboost-all-dev \
                    librocksdb-dev git pkg-config
   ```

2. **Build RandomX**:
   ```bash
   git clone https://github.com/tevador/RandomX.git
   cd RandomX
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   sudo make install
   ```

3. **Build INTcoin**:
   ```bash
   git clone https://gitlab.com/intcoin/crypto.git
   cd crypto
   mkdir build && cd build
   cmake .. -DBUILD_MINER=ON
   make -j$(nproc)
   ```

### FreeBSD Installation

1. **Install Dependencies**:
   ```bash
   sudo pkg install cmake boost-all rocksdb git
   ```

2. **Follow Linux build steps** (same process)

### Windows Installation

1. **Use WSL2** (recommended):
   - Install Windows Subsystem for Linux 2
   - Follow Linux installation instructions

2. **Native Windows**:
   - Install Visual Studio 2022 with C++ support
   - Install vcpkg for dependencies
   - Follow Windows-specific build instructions

---

## Getting Started

### Choose Your Mining Method

**Solo Mining**:
- ✅ Keep 100% of block rewards
- ✅ No pool fees
- ❌ High variance (may take long time to find blocks)
- ❌ Requires running full node

**Pool Mining**:
- ✅ Regular, predictable payouts
- ✅ Lower variance
- ✅ No need to run full node
- ❌ Pool fees (typically 1-3%)

**Recommendation**:
- **Small miners (<50 KH/s)**: Use pool mining for consistent rewards
- **Large operations (>100 KH/s)**: Solo mining may be more profitable

---

## Solo Mining

### Step 1: Run INTcoin Node

First, start the INTcoin daemon:

```bash
./intcoind -daemon
```

Wait for the blockchain to sync. Check status:

```bash
./intcoin-cli getinfo
```

### Step 2: Create Mining Address

Create or get your INTcoin address:

```bash
./intcoin-cli getnewaddress
```

Example output: `int1q9xj4hdm8ywgkf3vry6qp7zsf4jm9ktc5x2n8a7`

### Step 3: Start Mining

Start the CPU miner:

```bash
./intcoin-miner \
    -a int1q9xj4hdm8ywgkf3vry6qp7zsf4jm9ktc5x2n8a7 \
    --rpc-user=yourusername \
    --rpc-password=yourpassword \
    -t 4
```

**Parameters**:
- `-a`: Your mining address (where rewards will go)
- `--rpc-user`: RPC username (set in intcoin.conf)
- `--rpc-password`: RPC password
- `-t`: Number of threads (use number of CPU cores)

### Step 4: Configure intcoin.conf

Create `~/.intcoin/intcoin.conf`:

```conf
# RPC configuration
rpcuser=yourusername
rpcpassword=your_secure_password_here
rpcport=2211

# Network
listen=1
server=1

# Mining
gen=0
```

**Security Note**: Use a strong, random password!

### Example Solo Mining Setup

Complete example:

```bash
# 1. Start daemon
./intcoind -daemon

# 2. Wait for sync
./intcoin-cli getinfo

# 3. Start mining with 8 threads
./intcoin-miner \
    -a int1q9xj4hdm8ywgkf3vry6qp7zsf4jm9ktc5x2n8a7 \
    --rpc-user=miner \
    --rpc-password=SecurePass123! \
    -t 8 \
    --affinity
```

---

## Pool Mining

### Step 1: Choose a Mining Pool

**Factors to Consider**:
- **Fee**: Look for 1-3% fees
- **Location**: Choose a pool close to you for lower latency
- **Size**: Balance between stability (large) and decentralization (small)
- **Reputation**: Use established pools with good track records
- **Payout**: Check minimum payout threshold

**Example Pools** (check official INTcoin website for current list):
- pool.intcoin.org
- us.intcoin.pool
- eu.intcoin.pool

### Step 2: Create Pool Account

1. Visit your chosen pool's website
2. Create an account
3. Create a worker (e.g., `myworker01`)
4. Get your mining credentials

### Step 3: Start Pool Mining

```bash
./intcoin-miner \
    --pool \
    --pool-host=pool.intcoin.org \
    --pool-port=3333 \
    --pool-user=yourusername.worker01 \
    --pool-pass=x \
    -t 8
```

**Parameters**:
- `--pool`: Enable pool mining mode
- `--pool-host`: Pool server hostname
- `--pool-port`: Pool server port (usually 3333)
- `--pool-user`: Your username.worker_name
- `--pool-pass`: Usually just `x` (password not needed)
- `-t`: Number of threads

### Example Pool Mining Setup

```bash
# Mine to pool with optimized settings
./intcoin-miner \
    --pool \
    --pool-host=pool.intcoin.org \
    --pool-port=3333 \
    --pool-user=alice.rig01 \
    --pool-pass=x \
    -t 16 \
    --affinity \
    --batch-size=500 \
    --update-interval=30
```

---

## Optimizing Performance

### 1. Thread Configuration

**Auto-detect** (recommended):
```bash
./intcoin-miner -a <address> -t 0
```

**Manual configuration**:
- Desktop: Use all cores (e.g., `-t 16` for 16-core CPU)
- Laptop: Use 50-75% of cores to prevent overheating
- Shared system: Leave 1-2 cores free for other tasks

### 2. CPU Affinity

Enable CPU affinity to bind threads to specific cores:

```bash
./intcoin-miner -a <address> --affinity
```

**Benefits**:
- Better CPU cache utilization
- Reduced context switching
- 5-15% hashrate improvement

### 3. Hardware Optimizations

**BIOS/UEFI Settings**:
- Enable **XMP/DOCP** for faster RAM
- Set power plan to **High Performance**
- Disable **C-States** (prevents CPU throttling)
- Ensure **virtualization** is enabled

**Cooling**:
- Ensure good airflow in case
- Clean dust from CPU cooler
- Consider upgrading CPU cooler if temps are high
- Keep CPU temps under 80°C (176°F)

**RAM Speed**:
- RandomX loves fast RAM
- DDR4-3200+ recommended
- Dual-channel mode essential

### 4. Operating System

**Linux**:
```bash
# Increase max locked memory
sudo sysctl vm.nr_hugepages=1280

# Set CPU governor to performance
sudo cpupower frequency-set -g performance
```

**macOS**:
```bash
# Set high performance mode
sudo pmset -a perfmode 1
```

### 5. Batch Size

Adjust batch size based on your hardware:

```bash
# Slower CPU (lower latency)
--batch-size=100

# Fast CPU (better efficiency)
--batch-size=1000
```

### 6. Monitoring Tools

**htop** (Linux/macOS):
```bash
htop
```

**Activity Monitor** (macOS):
- Applications → Utilities → Activity Monitor

**Task Manager** (Windows):
- Ctrl + Shift + Esc

---

## Monitoring Your Miner

### Real-Time Statistics

While mining, you'll see output like:

```
========================================
INTcoin CPU Miner v1.0.0
Post-Quantum Cryptocurrency Miner
RandomX Proof-of-Work
========================================

Configuration:
  Mode: Solo Mining
  Network: Mainnet
  Threads: 16
  Mining Address: int1q9xj4hdm8ywgkf3vry6qp7zsf4jm9ktc5x2n8a7

========================================
Starting miner...
Press Ctrl+C to stop
========================================

[Mining] Hashrate: 5.23 KH/s | Blocks: 0 | Uptime: 3600s
[Mining] Hashrate: 5.31 KH/s | Blocks: 0 | Uptime: 3605s
[Mining] Hashrate: 5.28 KH/s | Blocks: 1 | Uptime: 3610s

*** BLOCK FOUND! ***
Block Hash: a7c3f...8d92e
Height: 123456
Nonce: 987654321
```

### Understanding Statistics

- **Hashrate**: Your mining speed (higher is better)
  - H/s = Hashes per second
  - KH/s = Thousands of hashes per second
  - MH/s = Millions of hashes per second

- **Blocks**: Number of blocks you've found (solo mining)
- **Shares**: Number of shares submitted to pool (pool mining)
- **Uptime**: How long miner has been running

### Expected Hashrates

| CPU | Approximate Hashrate |
|-----|---------------------|
| Intel i5-10400 | 1.2-1.5 KH/s |
| Intel i7-10700K | 2.0-2.5 KH/s |
| AMD Ryzen 5 5600X | 3.5-4.5 KH/s |
| AMD Ryzen 7 5800X | 5.0-6.5 KH/s |
| AMD Ryzen 9 5950X | 10-13 KH/s |
| AMD EPYC 7763 (64c) | 40-50 KH/s |

### Pool Dashboard

Most pools provide web dashboards showing:
- Current hashrate
- Estimated earnings
- Unpaid balance
- Worker status
- Recent shares

Check your pool's website for your dashboard.

---

## Troubleshooting

### Low Hashrate

**Problem**: Hashrate is lower than expected

**Solutions**:
1. Check CPU temperature (thermal throttling?)
2. Close other applications
3. Enable CPU affinity (`--affinity`)
4. Check RAM speed (enable XMP in BIOS)
5. Verify all CPU cores are being used
6. Update BIOS/CPU microcode

### Miner Crashes

**Problem**: Miner stops unexpectedly

**Solutions**:
1. Check available RAM (RandomX needs 2-8 GB)
2. Verify intcoind is running (solo mining)
3. Check RPC credentials
4. Review error logs
5. Test with fewer threads
6. Check system stability (run memtest)

### Connection Issues (Pool Mining)

**Problem**: Cannot connect to pool

**Solutions**:
1. Verify pool hostname and port
2. Check firewall settings
3. Test connectivity: `ping pool.intcoin.org`
4. Try alternate pool server
5. Check pool's status page

### High Rejected Shares (Pool Mining)

**Problem**: Many shares rejected by pool

**Solutions**:
1. Choose closer pool (lower latency)
2. Reduce batch size
3. Check internet connection stability
4. Verify system time is correct
5. Update to latest miner version

### Overheating

**Problem**: CPU temperature too high

**Solutions**:
1. Reduce number of threads (e.g., `-t 12` instead of `-t 16`)
2. Improve case airflow
3. Clean CPU cooler
4. Upgrade CPU cooler
5. Undervolt CPU (advanced)
6. Limit power consumption in BIOS

### No Payouts (Pool Mining)

**Problem**: Haven't received any payouts

**Reasons**:
1. **Below minimum**: Haven't reached minimum payout threshold
2. **New miner**: Need to mine for a while before first payout
3. **Wrong address**: Verify your payout address in pool settings
4. **Pool issue**: Check pool's payment status

**Check**:
- Pool dashboard for unpaid balance
- Pool's payment history
- Minimum payout threshold in pool settings

---

## FAQ

### Q: How much can I earn mining INTcoin?

**A**: Earnings depend on:
- Your hashrate
- Network difficulty
- Block reward (decreases over time)
- Pool fees (if pool mining)
- Electricity costs

**Example Calculation**:
```
Your Hashrate: 5 KH/s
Network Hashrate: 50 MH/s (50,000 KH/s)
Block Reward: 105,113,636 INT
Block Time: 2 minutes

Your Share: 5 / 50,000 = 0.01%
Blocks per Day: 720
Your Blocks per Day: 720 × 0.01% = 0.072
Daily Earnings: 0.072 × 105,113,636 = ~7.5 million INT
```

Use pool calculators for more accurate estimates.

### Q: Should I mine solo or in a pool?

**A**:
- **Small miners (<50 KH/s)**: Pool mining for regular payouts
- **Large operations (>100 KH/s)**: Solo mining to avoid fees
- **Learning/Testing**: Solo mining to understand the process

### Q: How much electricity will mining use?

**A**:
- Desktop (100W): ~2.4 kWh/day ≈ $0.30-0.50/day
- Workstation (250W): ~6.0 kWh/day ≈ $0.75-1.25/day
- Server (500W): ~12.0 kWh/day ≈ $1.50-2.50/day

Calculate your costs using local electricity rates.

### Q: Will mining damage my computer?

**A**: No, if done properly:
- Keep temperatures reasonable (<80°C)
- Ensure good cooling
- Don't overclock excessively
- Use quality power supply

Mining puts sustained load on CPU but won't damage it if temperatures are safe.

### Q: Can I mine on a laptop?

**A**: Yes, but with caution:
- Use 50-75% of CPU cores (not all)
- Monitor temperatures closely
- Ensure good ventilation
- Accept lower hashrates
- Consider laptop cooling pad

Laptops have limited cooling; extended mining may reduce lifespan.

### Q: What if I find a block while solo mining?

**A**: Congratulations!
1. Block is broadcast to network
2. Network validates block
3. After 100 confirmations, reward is spendable
4. Check your wallet: `./intcoin-cli getbalance`

### Q: Can I mine while using my computer?

**A**: Yes:
- Leave 1-2 CPU cores free (e.g., `-t 6` on 8-core CPU)
- Performance of other applications may be reduced
- Consider mining only when idle

### Q: How do I stop mining?

**A**: Press `Ctrl+C` in the terminal

The miner will:
1. Stop mining threads
2. Disconnect from pool (if applicable)
3. Display final statistics
4. Exit cleanly

### Q: Can I mine with GPU?

**A**: Not yet. GPU mining support is planned for future releases. RandomX is CPU-optimized, so GPUs don't offer significant advantages.

### Q: What is the minimum to start mining?

**A**:
- **Hardware**: Any CPU (2+ cores recommended)
- **RAM**: 2 GB available
- **Software**: Free (open source)
- **Time**: 15-30 minutes setup

There's no minimum purchase or registration fee!

### Q: Is INTcoin mining profitable?

**A**: Profitability depends on:
- Hardware efficiency (hash per watt)
- Electricity costs
- INTcoin price
- Network difficulty

Calculate your profitability:
```
Daily Revenue = (Your Hashrate / Network Hashrate) × Daily Blocks × Block Reward × INT Price
Daily Profit = Daily Revenue - Electricity Cost
```

Mining may be done for supporting the network even if not highly profitable.

---

## Getting Help

### Community Resources

- **Discord**: https://discord.gg/intcoin
- **Telegram**: https://t.me/intcoin
- **Forum**: https://forum.intcoin.org
- **Reddit**: https://reddit.com/r/intcoin

### Technical Support

- **GitHub Issues**: https://github.com/intcoin/intcoin/issues
- **GitLab Issues**: https://gitlab.com/intcoin/crypto/-/issues
- **Email**: support@intcoin.org

### Documentation

- **Technical Docs**: docs/MINING.md
- **API Reference**: docs/RPC.md
- **Wiki**: https://wiki.intcoin.org

---

## Safety and Security

### Best Practices

1. **Download Only Official Software**
   - Use official website or GitHub/GitLab
   - Verify signatures when possible

2. **Secure Your Wallet**
   - Use strong passwords
   - Backup wallet regularly
   - Encrypt wallet file

3. **Monitor Your System**
   - Watch temperatures
   - Check for unusual CPU usage
   - Use antivirus software

4. **Pool Mining Safety**
   - Research pool reputation
   - Use established pools
   - Don't use same password as wallet

5. **Electricity Safety**
   - Use quality power supply
   - Don't overload circuits
   - Ensure proper grounding

### Warning Signs

Stop mining if you notice:
- CPU temperature >90°C (194°F)
- System crashes or freezes
- Burning smell
- Unusual fan noise
- Corrupted files

---

## Next Steps

### Advanced Topics

Once comfortable with basic mining, explore:

1. **Mining Rig Building**
   - Dedicated mining systems
   - Multi-CPU configurations
   - Professional setups

2. **Optimization**
   - Custom kernel patches
   - Memory tuning
   - Advanced cooling solutions

3. **Running a Mining Pool**
   - Pool software setup
   - Community building
   - Payment systems

4. **Contributing to Development**
   - Mining software improvements
   - Bug fixes
   - Feature suggestions

---

## Conclusion

Congratulations on setting up INTcoin mining! Remember:

✅ **Start small** and learn the basics
✅ **Monitor your system** regularly
✅ **Join the community** for support
✅ **Be patient** - mining rewards take time
✅ **Have fun** and support the network!

Happy Mining! 🚀

---

**Document Version**: 1.0
**Last Updated**: December 3, 2025
**License**: MIT License
**Generated with**: [Claude Code](https://claude.com/claude-code)
