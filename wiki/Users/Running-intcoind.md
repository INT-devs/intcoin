# Running the INTcoin Daemon (intcoind)

**Last Updated**: December 3, 2025
**Version**: 1.0.0-alpha

The **intcoind** daemon is the core blockchain node that powers the INTcoin network. It maintains the blockchain, validates transactions, participates in P2P networking, and provides an RPC interface for other applications.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Command-Line Options](#command-line-options)
- [Configuration File](#configuration-file)
- [Running on Mainnet](#running-on-mainnet)
- [Running on Testnet](#running-on-testnet)
- [RPC Authentication](#rpc-authentication)
- [Data Directory](#data-directory)
- [Monitoring Status](#monitoring-status)
- [Stopping the Daemon](#stopping-the-daemon)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

Before running intcoind, ensure you have:

1. **Built INTcoin from source** (see [Building from Source](../../docs/BUILDING.md))
2. **Sufficient disk space** (minimum 10 GB recommended for mainnet)
3. **Network connectivity** (ports 2210 for P2P, 2211 for RPC)
4. **RocksDB installed** (required for blockchain storage)

---

## Quick Start

### 1. Check Installation

First, verify intcoind is installed correctly:

```bash
./intcoind --version
```

**Expected output:**
```
INTcoin Daemon v1.0.0
Post-Quantum Cryptocurrency
```

### 2. View Help

See all available options:

```bash
./intcoind --help
```

### 3. Run on Mainnet

Start the daemon with default settings:

```bash
./intcoind
```

This will:
- Create data directory at `./data`
- Initialize blockchain database
- Start P2P network on port **2210**
- Start RPC server on port **2211** (localhost only)
- Begin syncing with the network

---

## Command-Line Options

### Network Options

| Option | Description | Default |
|--------|-------------|---------|
| `-port=<port>` | P2P network port | 2210 (mainnet)<br>12210 (testnet) |
| `-testnet` | Run on testnet network | Disabled |

### RPC Options

| Option | Description | Default |
|--------|-------------|---------|
| `-rpcport=<port>` | RPC server port | 2211 (mainnet)<br>12211 (testnet) |
| `-rpcuser=<user>` | RPC username | None (⚠️ not secure) |
| `-rpcpassword=<pass>` | RPC password | None (⚠️ not secure) |

### Data Options

| Option | Description | Default |
|--------|-------------|---------|
| `-datadir=<dir>` | Data directory path | `./data` |

### Information Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-v, --version` | Show version information |

---

## Configuration File

Instead of command-line options, you can create a configuration file at:

```
./data/intcoin.conf
```

### Example Configuration

```ini
# INTcoin Configuration File

# Network
port=2210
testnet=0

# RPC
rpcport=2211
rpcuser=yourusername
rpcpassword=your_strong_password_here

# Data
datadir=/path/to/your/data

# P2P
maxconnections=125
listen=1

# Add seed nodes (optional)
addnode=seed1.international-coin.org:2210
addnode=seed2.international-coin.org:2210
```

**⚠️ Security Warning:** Protect your configuration file with proper permissions:

```bash
chmod 600 ./data/intcoin.conf
```

---

## Running on Mainnet

### Basic Mainnet Setup

```bash
# Create data directory
mkdir -p /var/intcoin/mainnet

# Run with custom data directory
./intcoind -datadir=/var/intcoin/mainnet \
           -rpcuser=myuser \
           -rpcpassword=mypassword
```

### Running as Background Service

**Using systemd (Linux):**

Create `/etc/systemd/system/intcoind.service`:

```ini
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=simple
User=intcoin
ExecStart=/usr/local/bin/intcoind -datadir=/var/intcoin/mainnet
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl enable intcoind
sudo systemctl start intcoind
sudo systemctl status intcoind
```

**View logs:**

```bash
sudo journalctl -u intcoind -f
```

---

## Running on Testnet

Testnet is useful for development and testing without using real INT coins.

```bash
./intcoind -testnet \
           -datadir=./testnet-data \
           -rpcuser=testuser \
           -rpcpassword=testpass
```

**Testnet Specifications:**
- P2P Port: **12210**
- RPC Port: **12211**
- Network Magic: **0xA1B2C3D5**
- Separate blockchain from mainnet

---

## RPC Authentication

The RPC interface allows applications (like wallets and miners) to communicate with intcoind.

### Setting Up Authentication

**⚠️ IMPORTANT:** Always set RPC credentials in production:

```bash
./intcoind -rpcuser=secureuser \
           -rpcpassword=$(openssl rand -base64 32)
```

### Allowing External Connections

By default, RPC only listens on `127.0.0.1` (localhost). To allow external connections:

**⚠️ WARNING:** Only do this behind a firewall or VPN!

```bash
./intcoind -rpcuser=user \
           -rpcpassword=pass \
           -rpcallowip=192.168.1.0/24
```

---

## Data Directory

### Structure

```
data/
├── blockchain/         # RocksDB blockchain database
│   ├── CURRENT
│   ├── LOCK
│   ├── LOG
│   └── *.sst          # Data files
└── intcoin.conf       # Configuration file (optional)
```

### Disk Space Requirements

| Network | Initial Size | After 1 Year | After 5 Years |
|---------|--------------|--------------|---------------|
| Mainnet | ~100 MB | ~10 GB | ~50 GB |
| Testnet | ~50 MB | ~5 GB | ~25 GB |

### Backup

To backup your blockchain:

```bash
# Stop daemon first
pkill -SIGTERM intcoind

# Backup
tar czf blockchain-backup-$(date +%Y%m%d).tar.gz data/blockchain/

# Restart daemon
./intcoind
```

---

## Monitoring Status

### Console Output

intcoind displays status every 60 seconds:

```
[Status] Height: 12345 | Peers: 8 | Mempool: 42
```

- **Height**: Current blockchain height
- **Peers**: Number of connected peers
- **Mempool**: Unconfirmed transactions

### Checking via RPC

Use `intcoin-cli` (once implemented) or `curl`:

```bash
curl --user rpcuser:rpcpass \
     --data-binary '{"jsonrpc":"2.0","id":"1","method":"getblockcount","params":[]}' \
     -H 'content-type: text/plain;' \
     http://127.0.0.1:2211/
```

### Monitoring Tools

- **htop/top**: Monitor CPU and memory usage
- **nethogs**: Monitor network bandwidth
- **iotop**: Monitor disk I/O
- **journalctl**: View logs (systemd)

---

## Stopping the Daemon

### Graceful Shutdown

Press `Ctrl+C` in the terminal, or send SIGTERM:

```bash
pkill -SIGTERM intcoind
```

**Expected output:**
```
Received signal 15, shutting down...

Shutting down...
Stopping RPC server...
Stopping P2P network...
Closing blockchain...
Shutdown complete.
Goodbye!
```

### Force Kill (⚠️ Not Recommended)

Only if graceful shutdown fails:

```bash
pkill -SIGKILL intcoind
```

**⚠️ WARNING:** Force killing may corrupt the database. Always try graceful shutdown first.

---

## Troubleshooting

### Port Already in Use

**Error:** `Failed to start P2P network: Address already in use`

**Solution:** Check if another instance is running:

```bash
lsof -i :2210
# or
netstat -tulpn | grep 2210
```

Kill the other instance or use a different port:

```bash
./intcoind -port=2220
```

### RocksDB Database Error

**Error:** `Failed to open database: IO error`

**Solution:** Check permissions and disk space:

```bash
# Check permissions
ls -la data/blockchain/

# Check disk space
df -h

# Fix permissions
chmod -R 755 data/
```

### Cannot Connect to Peers

**Symptoms:** Peers count stays at 0

**Solutions:**

1. **Check firewall:**
```bash
# Allow P2P port
sudo ufw allow 2210/tcp
```

2. **Manually add nodes:**
```bash
./intcoind -addnode=seed1.international-coin.org:2210 \
           -addnode=seed2.international-coin.org:2210
```

3. **Check network connectivity:**
```bash
nc -zv seed1.international-coin.org 2210
```

### High Memory Usage

**Normal:** intcoind uses 500-1000 MB of RAM initially, growing with blockchain size.

**If excessive:** Check for memory leaks and report on GitHub.

**Reduce memory:**
- Close other applications
- Use a VPS with more RAM
- Consider pruned mode (when implemented)

### Blockchain Corruption

**Symptoms:** Crashes, validation errors, unexpected behavior

**Solution:** Resync from genesis:

```bash
# Backup wallet if you have one
cp -r data/wallet data/wallet-backup

# Remove blockchain data
rm -rf data/blockchain

# Restart daemon
./intcoind
```

---

## Performance Tuning

### SSD vs HDD

- **SSD recommended:** 10-100x faster blockchain sync
- **HDD acceptable:** Slower but works for low-traffic nodes

### Network Bandwidth

| Activity | Download | Upload |
|----------|----------|--------|
| Initial sync | High (100+ MB/hr) | Low |
| Normal operation | Medium (10-50 MB/hr) | Medium (10-50 MB/hr) |
| With many peers | Higher | Higher |

### CPU Usage

- **Initial sync:** High CPU usage (PoW verification)
- **Normal operation:** Low CPU usage
- **Peak usage:** During block validation

---

## Security Best Practices

### 1. Firewall Configuration

```bash
# Allow P2P only
sudo ufw allow 2210/tcp

# Don't expose RPC to internet
# (keep 2211 blocked from external access)
```

### 2. Strong RPC Credentials

```bash
# Generate strong password
RPC_PASS=$(openssl rand -base64 32)

# Use it
./intcoind -rpcuser=admin -rpcpassword=$RPC_PASS
```

### 3. Run as Non-Root User

```bash
# Create dedicated user
sudo useradd -r -m -s /bin/bash intcoin

# Run as that user
sudo -u intcoin ./intcoind
```

### 4. Keep Software Updated

```bash
# Check for updates regularly
git pull origin main
make clean
make intcoind
```

### 5. Monitor Logs

```bash
# Watch for suspicious activity
tail -f /var/log/syslog | grep intcoind
```

---

## Next Steps

Once intcoind is running:

- [Use intcoin-cli](intcoin-cli-Guide.md) - Command-line RPC client
- [Create a Wallet](Creating-Wallet.md) - Set up your first wallet
- [Mining Guide](Mining-Guide.md) - Start mining INT coins
- [Run a Node 24/7](Running-Node.md) - Production setup guide

---

## Additional Resources

- [RPC API Documentation](../../docs/RPC.md)
- [P2P Protocol](../../docs/NETWORK.md)
- [Blockchain Architecture](../../docs/ARCHITECTURE.md)
- [Troubleshooting Guide](Troubleshooting.md)

---

## Support

If you encounter issues:

1. Check this guide's [Troubleshooting](#troubleshooting) section
2. Search [GitHub Issues](https://github.com/intcoin/intcoin/issues)
3. Ask on [Discord](https://discord.gg/intcoin)
4. Post on [Telegram](https://t.me/intcoin)

---

**Happy node running! 🚀**
