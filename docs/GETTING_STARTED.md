# Getting Started with INTcoin

Welcome to INTcoin! This guide will help you get started with building, installing, and using INTcoin.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Building INTcoin](#building-intcoin)
3. [Installing INTcoin](#installing-intcoin)
4. [First Run](#first-run)
5. [Basic Usage](#basic-usage)
6. [Next Steps](#next-steps)

---

## System Requirements

### Minimum Requirements

- **CPU**: Dual-core processor (2.0 GHz or faster)
- **RAM**: 4 GB
- **Storage**: 50 GB free space
- **Network**: Broadband internet connection

### Recommended Requirements

- **CPU**: Quad-core processor (3.0 GHz or faster)
- **RAM**: 8 GB or more
- **Storage**: 100 GB+ SSD
- **Network**: Stable broadband connection

### Supported Operating Systems

✅ **Linux** (Ubuntu 20.04+, Debian 11+, Fedora 35+)
✅ **FreeBSD** (13.0+)
✅ **macOS** (12.0 Monterey or later)
✅ **Windows** (Windows 10/11 via WSL2 or native build)

---

## Building INTcoin

### Prerequisites

#### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake git libssl-dev \
    libboost-all-dev libqrencode-dev libsodium-dev \
    qt6-base-dev qt6-tools-dev
```

#### FreeBSD

```bash
sudo pkg install cmake git openssl boost-all libqrencode \
    libsodium qt6-base qt6-tools
```

#### macOS

```bash
brew install cmake openssl boost libqrencode libsodium qt@6
```

### Clone Repository

```bash
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
```

### Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (use -j for parallel compilation)
make -j$(nproc)  # Linux/FreeBSD
make -j$(sysctl -n hw.ncpu)  # macOS
```

**Build time**: 5-15 minutes depending on your system

### Verify Build

```bash
# Check binaries were created
ls -lh bin/

# Should see:
# - intcoind (daemon)
# - intcoin-cli (command-line interface)
# - intcoin-qt (GUI wallet)
# - intcoin-tx (transaction utility)
```

---

## Installing INTcoin

### Linux Installation

```bash
# Install to /usr/local
sudo make install

# Or install to custom location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/intcoin
make install
```

### FreeBSD Installation

```bash
# Install to /usr/local
sudo make install

# Create rc.d service script
sudo cp contrib/init/intcoind.freebsd /usr/local/etc/rc.d/intcoind
sudo chmod +x /usr/local/etc/rc.d/intcoind

# Enable service
sudo sysrc intcoind_enable="YES"
```

### Manual Installation

If you prefer not to use `make install`:

```bash
# Copy binaries
sudo cp bin/* /usr/local/bin/

# Create data directory
mkdir -p ~/.intcoin

# Set permissions
chmod 700 ~/.intcoin
```

---

## First Run

### Starting the Daemon

```bash
# Start INTcoin daemon
intcoind -daemon

# Check status
intcoin-cli getblockchaininfo
```

**First sync**: The initial blockchain sync will take several hours depending on your connection and system performance.

### Configuration

Create a configuration file at `~/.intcoin/intcoin.conf`:

```ini
# Server options
server=1
daemon=1
listen=1

# RPC settings
rpcuser=your_username
rpcpassword=your_secure_password
rpcallowip=127.0.0.1

# Network settings
maxconnections=50

# Mining (optional)
gen=0  # Set to 1 to enable mining

# Lightning Network (optional)
lightning=1
lightningdir=~/.intcoin/lightning
```

**Important**: Replace `your_username` and `your_secure_password` with secure credentials.

### Generating a Wallet

```bash
# Create new wallet
intcoin-cli createwallet "mywallet"

# Get receiving address
intcoin-cli getnewaddress

# Check balance
intcoin-cli getbalance
```

---

## Basic Usage

### Command-Line Interface

#### Check Node Status

```bash
# Get blockchain info
intcoin-cli getblockchaininfo

# Get network info
intcoin-cli getnetworkinfo

# Get peer connections
intcoin-cli getpeerinfo
```

#### Wallet Operations

```bash
# List wallets
intcoin-cli listwallets

# Get new address
intcoin-cli getnewaddress

# Send coins
intcoin-cli sendtoaddress "INT_ADDRESS" 10.5

# List transactions
intcoin-cli listtransactions

# Get balance
intcoin-cli getbalance
```

#### Mining

```bash
# Start mining (solo)
intcoin-cli setgenerate true 1

# Stop mining
intcoin-cli setgenerate false

# Get mining info
intcoin-cli getmininginfo
```

### GUI Wallet (intcoin-qt)

Launch the GUI wallet:

```bash
intcoin-qt
```

**Features**:
- Visual wallet management
- Transaction history
- Address book
- Lightning Network interface
- Built-in mining support
- QR code generation/scanning
- Hardware wallet integration

---

## Next Steps

### Learn More

- **[User Guide](USER_GUIDE.md)** - Comprehensive usage documentation
- **[Lightning Network Guide](LIGHTNING_GUIDE.md)** - Using Lightning payments
- **[Mining Guide](MINING_GUIDE.md)** - Solo and pool mining
- **[API Documentation](API_REFERENCE.md)** - RPC API reference

### Key Features

#### Lightning Network V2

INTcoin includes advanced Lightning Network features:

- **Multi-Path Payments (MPP)** - Split large payments across multiple routes
- **Atomic Multi-Path (AMP)** - Enhanced privacy and reliability
- **Watchtower Service** - 24/7 channel breach protection
- **Submarine Swaps** - Trustless on-chain ↔ Lightning exchanges

See [LIGHTNING_V2.md](LIGHTNING_V2.md) for technical details.

#### Advanced IBD

Fast initial blockchain sync with:

- **Parallel Validation** - Multi-threaded block processing
- **AssumeUTXO** - Skip historical validation with signed snapshots
- **Optimized Consensus** - RandomX PoW + Digishield difficulty

#### Desktop Wallet Features

- **QR Code Support** - Generate and scan QR codes for addresses
- **Hardware Wallets** - Ledger and Trezor integration
- **PDF Invoices** - Professional invoice generation
- **Built-in Mining** - Solo and pool mining support

### Community

- **Website**: https://intcoin.org
- **GitHub**: https://github.com/INT-devs/intcoin
- **Discord**: https://discord.gg/intcoin
- **Twitter**: @intcoin_project
- **Reddit**: r/intcoin

### Support

If you need help:

1. Check the [FAQ](FAQ.md)
2. Read the [User Guide](USER_GUIDE.md)
3. Search [GitHub Issues](https://github.com/INT-devs/intcoin/issues)
4. Ask on [Discord](https://discord.gg/intcoin)
5. Create a [GitHub Issue](https://github.com/INT-devs/intcoin/issues/new)

---

## Quick Reference

### Essential Commands

```bash
# Daemon
intcoind -daemon                  # Start daemon
intcoin-cli stop                  # Stop daemon
intcoin-cli help                  # List all commands
intcoin-cli help <command>        # Get help for specific command

# Wallet
intcoin-cli createwallet "name"   # Create wallet
intcoin-cli loadwallet "name"     # Load wallet
intcoin-cli getbalance            # Check balance
intcoin-cli getnewaddress         # Get receiving address
intcoin-cli sendtoaddress <addr> <amount>  # Send coins

# Blockchain
intcoin-cli getblockchaininfo     # Chain status
intcoin-cli getblockcount         # Current height
intcoin-cli getblock <hash>       # Get block details
intcoin-cli gettxout <txid> <n>   # Get UTXO

# Network
intcoin-cli getnetworkinfo        # Network status
intcoin-cli getpeerinfo           # Connected peers
intcoin-cli addnode <node> add    # Connect to peer

# Mining
intcoin-cli setgenerate true 1    # Start mining
intcoin-cli getmininginfo         # Mining status
intcoin-cli getblocktemplate      # Get block template
```

### File Locations

| File/Directory | Linux/FreeBSD | macOS | Description |
|----------------|---------------|-------|-------------|
| Data directory | `~/.intcoin` | `~/Library/Application Support/INTcoin` | Blockchain data |
| Config file | `~/.intcoin/intcoin.conf` | `~/Library/Application Support/INTcoin/intcoin.conf` | Configuration |
| Wallet | `~/.intcoin/wallets/` | `~/Library/Application Support/INTcoin/wallets/` | Wallet files |
| Debug log | `~/.intcoin/debug.log` | `~/Library/Application Support/INTcoin/debug.log` | Debug output |
| Lightning | `~/.intcoin/lightning/` | `~/Library/Application Support/INTcoin/lightning/` | LN channels |

### Port Numbers

| Service | Port | Protocol | Description |
|---------|------|----------|-------------|
| P2P Network | 8333 | TCP | Node-to-node communication |
| RPC | 8332 | TCP | JSON-RPC API |
| Lightning | 9735 | TCP | Lightning Network protocol |
| Watchtower | 9911 | TCP | Watchtower service |

---

**Welcome to INTcoin!** 🚀

For detailed usage information, continue to the [User Guide](USER_GUIDE.md).
