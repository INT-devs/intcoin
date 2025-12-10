# INTcoin Quick Start Guide

Get up and running with INTcoin in 5 minutes.

## Prerequisites

- 64-bit operating system (Linux, macOS, FreeBSD, or Windows)
- 4 GB RAM minimum
- 10 GB free disk space
- Internet connection

## Installation

### Linux/macOS

```bash
# Clone repository
git clone https://github.com/intcoin/intcoin.git
cd intcoin

# Run installation script (Linux)
chmod +x install-linux.sh
sudo ./install-linux.sh

# Build from source
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Windows

1. Download pre-built binaries from [Releases](https://github.com/intcoin/intcoin/releases)
2. Extract ZIP file
3. Run `intcoind.exe`

## Running the Daemon

### Testnet (Recommended for First-time Users)

```bash
# Start daemon on testnet
intcoind --testnet

# Check status
intcoin-cli --testnet getinfo
```

### Mainnet

```bash
# Start daemon on mainnet
intcoind

# Check status
intcoin-cli getinfo
```

## Creating a Wallet

### Using CLI

```bash
# Create new wallet
intcoin-cli createwallet "my_wallet"

# Get new address
intcoin-cli getnewaddress

# Check balance
intcoin-cli getbalance
```

### Using Qt GUI

```bash
# Launch GUI wallet
intcoin-qt --testnet

# Follow on-screen instructions to:
# 1. Create new wallet
# 2. Backup mnemonic phrase (24 words)
# 3. Receive coins
```

## Getting Testnet Coins

### Using Faucet

```bash
# Start faucet server (if you're running one)
intcoin-faucet --testnet --port=2215

# Or visit public faucet
# Open browser: http://testnet-faucet.international-coin.org
# Enter your address and request coins
```

### Using Miner

```bash
# Mine testnet coins to your address
intcoin-miner --testnet \
  --address=int1qyour_address_here \
  --threads=4
```

## Sending Coins

### Using CLI

```bash
# Send 1 INT to address
intcoin-cli sendtoaddress "int1qrecipient_address" 1.0

# Send with custom fee
intcoin-cli sendtoaddress "int1qrecipient_address" 1.0 "" "" 0.0001
```

### Using Qt GUI

1. Click "Send" tab
2. Enter recipient address
3. Enter amount
4. Click "Send"
5. Confirm transaction

## Mining

### Solo Mining

```bash
# Mine to your address
intcoin-miner --solo \
  --address=int1qyour_address \
  --threads=8 \
  --rpcuser=user \
  --rpcpassword=pass
```

### Pool Mining

```bash
# Mine to pool
intcoin-miner --pool \
  --pool-url=stratum+tcp://pool.international-coin.org:3333 \
  --pool-user=int1qyour_address \
  --threads=8
```

## Configuration

### Config File Location

- **Linux**: `~/.intcoin/intcoin.conf`
- **macOS**: `~/Library/Application Support/INTcoin/intcoin.conf`
- **Windows**: `%APPDATA%\INTcoin\intcoin.conf`
- **FreeBSD**: `~/.intcoin/intcoin.conf`

### Sample Configuration

```ini
# Network
testnet=1
port=12210

# RPC
rpcuser=intcoinrpc
rpcpassword=changeme
rpcport=12211
rpcallowip=127.0.0.1

# Mining
gen=0
genproclimit=4

# Connection
maxconnections=125
```

## Checking Sync Status

```bash
# Get blockchain info
intcoin-cli getblockchaininfo

# Output:
# {
#   "chain": "test",
#   "blocks": 12345,
#   "headers": 12345,
#   "bestblockhash": "...",
#   "difficulty": 1234.56,
#   "verificationprogress": 0.99,
#   "chainwork": "...",
#   "pruned": false
# }
```

## Common Commands

### Blockchain Info

```bash
# Get block count
intcoin-cli getblockcount

# Get best block hash
intcoin-cli getbestblockhash

# Get block info
intcoin-cli getblock <blockhash>

# Get difficulty
intcoin-cli getdifficulty
```

### Wallet Operations

```bash
# List wallets
intcoin-cli listwallets

# Get wallet info
intcoin-cli getwalletinfo

# List transactions
intcoin-cli listtransactions

# Get transaction
intcoin-cli gettransaction <txid>
```

### Network Info

```bash
# Get network info
intcoin-cli getnetworkinfo

# Get peer info
intcoin-cli getpeerinfo

# Get connection count
intcoin-cli getconnectioncount
```

## Port Reference

| Service | Mainnet Port | Testnet Port |
|---------|-------------|--------------|
| P2P Network | 2210 | 12210 |
| RPC Server | 2211 | 12211 |
| Block Explorer | 2212 | 12212 |
| Lightning P2P | 2213 | 12213 |
| Lightning RPC | 2214 | 12214 |
| Testnet Faucet | N/A | 2215 |

## Stopping the Daemon

```bash
# Graceful shutdown
intcoin-cli stop

# Or send SIGTERM
killall intcoind
```

## Backup Your Wallet

### Backup Mnemonic

**CRITICAL**: Write down your 24-word mnemonic phrase and store it securely offline. This is the ONLY way to recover your wallet.

### Backup Wallet File

```bash
# Backup wallet.dat
intcoin-cli backupwallet /path/to/backup/wallet_backup.dat

# Or manually copy
cp ~/.intcoin/wallets/wallet.dat ~/backup/
```

## Security Best Practices

1. **Never share your mnemonic phrase** with anyone
2. **Use strong RPC password** in configuration
3. **Firewall RPC port** (only allow 127.0.0.1)
4. **Encrypt wallet** with strong passphrase
5. **Keep software updated** to latest version
6. **Backup regularly** and test restore procedure
7. **Use testnet first** before handling real value

## Getting Help

### Documentation

- [Installation Guide](Installation.md)
- [Running Daemon](../user-guides/Running-Daemon.md)
- [Using Wallet](../user-guides/Using-Wallet.md)
- [Mining Guide](../user-guides/Mining.md)

### Community

- **Discord**: https://discord.gg/7p4VmS2z
- **Telegram**: https://t.me/INTcoin_official
- **Reddit**: https://reddit.com/r/intcoin
- **X (Twitter)**: https://x.com/INTcoin_team

### Support

- **Documentation**: https://international-coin.org/docs
- **Wiki**: https://gitlab.com/intcoin/crypto/-/wikis/
- **Issues**: https://github.com/intcoin/intcoin/issues
- **Email**: support@international-coin.org

## Next Steps

Now that you have INTcoin running:

1. **Explore the GUI** - Try `intcoin-qt` for a graphical interface
2. **Mine Some Coins** - Follow the [Mining Guide](../user-guides/Mining.md)
3. **Run a Faucet** - Help the testnet with [Faucet Guide](../user-guides/Testnet-Faucet.md)
4. **Develop Apps** - Check [Developer Guides](../developer-guides/)
5. **Join Community** - Connect with other INT users

---

*Last Updated: December 5, 2025*
