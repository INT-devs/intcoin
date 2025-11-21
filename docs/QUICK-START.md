# INTcoin Quick Start Guide

**Version**: 1.1.0
**Last Updated**: November 14, 2025
**Network Launch**: January 1, 2026

## Overview

INTcoin is the world's first production-ready quantum-resistant cryptocurrency. This guide will help you get started with INTcoin in minutes.

## What You Need

- **Operating System**: Linux, macOS, FreeBSD, or Windows
- **RAM**: Minimum 2GB, Recommended 4GB+
- **Disk Space**: Minimum 20GB for blockchain data
- **Network**: Stable internet connection

## Quick Installation

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake git libssl-dev libboost-all-dev

# Clone repository
git clone https://gitlab.com/intcoin/intcoin.git
cd intcoin

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Install
sudo cmake --install build
```

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake openssl boost rocksdb qt@6

# Clone repository
git clone https://gitlab.com/intcoin/intcoin.git
cd intcoin

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_QT_WALLET=ON
cmake --build build -j$(sysctl -n hw.ncpu)

# Binaries available in build/ directory
```

### Windows

Use the automated installer script:

```powershell
# Run as Administrator
.\scripts\install-windows.ps1
```

Or see [BUILD-WINDOWS.md](BUILD-WINDOWS.md) for manual build instructions.

## First Steps

### 1. Start the Daemon

The daemon is the core node software that connects to the INTcoin network:

```bash
# Start the daemon
./build/intcoind

# Or with custom data directory
./build/intcoind -datadir=/path/to/data
```

Default ports:
- **P2P**: 9333 (mainnet), 19333 (testnet)
- **RPC**: 9334 (mainnet), 19334 (testnet)

### 2. Create a Wallet

Using the command-line wallet tool:

```bash
# Create new wallet
./build/intcoin-wallet create

# Or using the CLI
./build/intcoin-cli createwallet "mywallet"
```

Using the Qt GUI wallet:

```bash
# Launch GUI wallet
./build/src/qt/intcoin-qt
```

The GUI will guide you through:
1. Creating a new wallet
2. Setting a strong password
3. Backing up your recovery phrase (24 words)

**IMPORTANT**: Write down your recovery phrase and store it safely!

### 3. Get Your First Address

```bash
# Get a receiving address
./build/intcoin-cli getnewaddress

# Example output:
# intcoin1q2x3y4z5a6b7c8d9e0f1g2h3i4j5k6l7m8n9o0p1q2r3s4t5u6
```

### 4. Check Your Balance

```bash
# Check wallet balance
./build/intcoin-cli getbalance

# Check blockchain sync status
./build/intcoin-cli getblockchaininfo
```

## Network Information

### Mainnet (Production)

- **Launch Date**: January 1, 2026
- **Genesis Hash**: `000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9`
- **P2P Port**: 9333
- **RPC Port**: 9334
- **Magic Bytes**: `0x494E5443` ("INTC")

### Seed Nodes

Primary active seed nodes:
- `74.208.112.43:9333`
- `51.155.97.192:9333`

Geographic seed nodes will be activated at launch.

### Testnet

- **P2P Port**: 19333
- **RPC Port**: 19334
- **Magic Bytes**: `0x54494E54` ("TINT")

To use testnet:

```bash
./build/intcoind -testnet
```

## Basic Operations

### Send INTcoin

```bash
# Send 100 INT to an address
./build/intcoin-cli sendtoaddress "intcoin1q..." 100

# Send with custom fee
./build/intcoin-cli sendtoaddress "intcoin1q..." 100 "" "" false 1000
```

### Check Transaction

```bash
# Get transaction details
./build/intcoin-cli gettransaction <txid>

# List recent transactions
./build/intcoin-cli listtransactions
```

### Mine Blocks (Testnet)

```bash
# Start CPU mining
./build/intcoin-miner

# Or use RPC
./build/intcoin-cli setgenerate true 4  # Use 4 CPU threads
```

## Security Best Practices

### Wallet Security

1. **Strong Password**: Use a password with 16+ characters
2. **Backup Recovery Phrase**: Write down your 24-word recovery phrase
3. **Encrypt Wallet**: Always encrypt your wallet file
4. **Regular Backups**: Backup wallet.dat regularly

```bash
# Encrypt existing wallet
./build/intcoin-cli encryptwallet "your-secure-password"

# Backup wallet
./build/intcoin-cli backupwallet "/secure/location/wallet-backup.dat"
```

### Node Security

1. **Firewall**: Allow only P2P port 9333
2. **RPC Access**: Restrict RPC to localhost only
3. **Updates**: Keep your node software updated

Edit `~/.intcoin/intcoin.conf`:

```ini
# Restrict RPC to localhost
rpcallowip=127.0.0.1

# Enable only P2P port
listen=1
upnp=0

# Strong RPC credentials
rpcuser=your_username
rpcpassword=your_strong_password
```

## Quantum-Resistant Features

INTcoin uses NIST-approved post-quantum cryptography:

- **Digital Signatures**: CRYSTALS-Dilithium5 (ML-DSA-87, FIPS 204)
- **Key Exchange**: CRYSTALS-Kyber1024 (ML-KEM-1024, FIPS 203)
- **Hashing**: SHA3-256 (FIPS 202)
- **Proof of Work**: SHA-256

Your funds are protected against both classical and quantum computer attacks!

## Network Specifications

- **Block Time**: ~5 minutes (300 seconds target)
- **Block Reward**: 105,113,636 INT (initial)
- **Halving**: Every 1,051,200 blocks (~4 years)
- **Max Supply**: 221,000,000,000,000 INT (221 Trillion)
- **Difficulty Adjustment**: Every 2016 blocks

## GUI Wallet Features

The Qt GUI wallet (`intcoin-qt`) includes:

- **Dashboard**: View balance and recent transactions
- **Send/Receive**: Easy payment interface
- **Transaction History**: Detailed transaction list
- **Address Book**: Manage contacts
- **Mining Tab**: Control mining operations
- **Network Tab**: Monitor P2P connections
- **Console**: Advanced RPC commands

## Command Reference

### Essential Commands

```bash
# Node Control
./build/intcoind                    # Start daemon
./build/intcoin-cli stop            # Stop daemon
./build/intcoin-cli getinfo         # Node information

# Wallet Operations
./build/intcoin-cli getnewaddress   # Get new address
./build/intcoin-cli getbalance      # Check balance
./build/intcoin-cli sendtoaddress   # Send coins
./build/intcoin-cli listtransactions # List transactions

# Blockchain Info
./build/intcoin-cli getblockcount   # Current block height
./build/intcoin-cli getblockhash    # Get block hash
./build/intcoin-cli getblock        # Get block details

# Network Info
./build/intcoin-cli getpeerinfo     # Connected peers
./build/intcoin-cli getnetworkinfo  # Network status
./build/intcoin-cli addnode         # Connect to peer
```

For complete RPC API documentation, see [RPC-API.md](RPC-API.md).

## Troubleshooting

### Node Won't Start

```bash
# Check logs
tail -f ~/.intcoin/debug.log

# Try with clean data directory
./build/intcoind -reindex
```

### Can't Connect to Network

```bash
# Manually add seed nodes
./build/intcoin-cli addnode "74.208.112.43:9333" "add"
./build/intcoin-cli addnode "51.155.97.192:9333" "add"

# Check firewall
sudo ufw allow 9333/tcp
```

### Wallet Issues

```bash
# Unlock wallet for 60 seconds
./build/intcoin-cli walletpassphrase "your-password" 60

# Rescan blockchain for wallet transactions
./build/intcoin-cli rescanblockchain
```

## Getting Help

- **Documentation**: [docs/README.md](README.md)
- **Website Documentation**: https://international-coin.org/docs/
- **Getting Started Guide**: https://international-coin.org/docs/getting-started.html
- **Whitepaper**: https://international-coin.org/docs/whitepaper.html
- **RPC API**: [docs/RPC-API.md](RPC-API.md)
- **Security**: [docs/SECURITY-AUDIT.md](SECURITY-AUDIT.md)
- **Website**: https://international-coin.org
- **Email**: team@international-coin.org

## Advanced Topics

For more advanced usage, see:

- [TOR-GUIDE.md](TOR-GUIDE.md) - Running node over Tor
- [INSTALL-LINUX.md](INSTALL-LINUX.md) - Detailed Linux installation
- [INSTALL-FREEBSD.md](INSTALL-FREEBSD.md) - FreeBSD installation
- [BUILD-WINDOWS.md](BUILD-WINDOWS.md) - Windows build guide
- [TESTING.md](TESTING.md) - Running test suite
- [CRYPTOGRAPHY-DESIGN.md](CRYPTOGRAPHY-DESIGN.md) - Cryptographic design

## What's Next?

1. **Join the Community**: Connect with other INTcoin users
2. **Run a Full Node**: Help secure the network
3. **Set Up Mining**: Contribute to block production
4. **Explore**: Learn about quantum-resistant cryptography

## Important Dates

- **Genesis Block Mined**: November 14, 2025
- **Mainnet Launch**: January 1, 2026
- **First Halving**: ~4 years after launch

---

**Welcome to the Quantum-Resistant Future of Cryptocurrency!**

*INTcoin - Built to Last in the Quantum Era*
