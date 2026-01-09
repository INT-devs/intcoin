# INTcoin Quick Start Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Difficulty**: Beginner
**Time Required**: 30 minutes

Welcome to INTcoin! This guide will help you get started with INTcoin in just 30 minutes.

---

## What is INTcoin?

INTcoin is a **post-quantum cryptocurrency** that uses:
- 🔐 **Quantum-Resistant Cryptography** (Dilithium3, Kyber768)
- ⛏️ **CPU Mining** (RandomX - ASIC-resistant)
- 📱 **Mobile Wallets** (iOS & Android SPV wallets)
- 🔄 **Cross-Chain Features** (Atomic swaps, bridges)

---

## Choose Your Path

Select the path that best fits your needs:

### Option 1: Mobile Wallet (Easiest) 📱
**Best for**: Everyday users who want to send/receive INT
**Time**: 5 minutes
**Requirements**: iPhone (iOS 15+) or Android (8.0+)

[→ Go to Mobile Wallet Setup](#option-1-mobile-wallet)

### Option 2: Desktop Wallet 💻
**Best for**: Users who want full control and features
**Time**: 15 minutes
**Requirements**: Computer (Windows/Mac/Linux)

[→ Go to Desktop Wallet Setup](#option-2-desktop-wallet)

### Option 3: Full Node Operator 🖥️
**Best for**: Advanced users, miners, developers
**Time**: 30+ minutes
**Requirements**: Server or desktop with 50+ GB storage

[→ Go to Full Node Setup](#option-3-full-node)

---

## Option 1: Mobile Wallet

### iOS (iPhone/iPad)

**Step 1: Install App**
```
1. Open App Store
2. Search "INTcoin Wallet"
3. Tap "Get" to install
4. Open INTcoin Wallet
```

**Step 2: Create Wallet**
```
1. Tap "Create New Wallet"
2. Write down your 24-word recovery phrase
   ⚠️ CRITICAL: Store this phrase safely!
3. Verify recovery phrase
4. Set PIN code
5. Enable biometric unlock (optional)
```

**Step 3: Receive INT**
```
1. Tap "Receive"
2. Share your INTcoin address or QR code
3. Wait for confirmation (2-3 minutes)
```

**Step 4: Send INT**
```
1. Tap "Send"
2. Scan recipient's QR code or paste address
3. Enter amount
4. Review transaction
5. Confirm with PIN/biometrics
```

**Backup Your Wallet**:
```
Settings → Backup → Export Recovery Phrase
Write down 24 words on paper (not digitally!)
```

See: [Mobile SDK Documentation](MOBILE_SDK.md)

### Android

**Step 1: Install App**
```
1. Open Google Play Store
2. Search "INTcoin Wallet"
3. Tap "Install"
4. Open INTcoin Wallet
```

**Step 2-4**: Same as iOS above

---

## Option 2: Desktop Wallet

### Windows

**Step 1: Download**
```
1. Visit: https://intcoin.org/downloads
2. Download "INTcoin-1.2.0-win64.exe"
3. Run installer
4. Click "Next" → "Install" → "Finish"
```

**Step 2: Launch Wallet**
```
1. Open "INTcoin Wallet" from Start Menu
2. Choose data directory (default: C:\Users\YourName\AppData\Roaming\INTcoin)
3. Wait for initial sync (may take 30 minutes)
```

**Step 3: Encrypt Wallet**
```
1. Click "Settings" → "Encrypt Wallet"
2. Enter strong passphrase
3. Write down passphrase securely
4. Click "OK"
5. Wallet will restart
```

**Step 4: Backup Wallet**
```
1. Click "File" → "Backup Wallet"
2. Save to USB drive or external storage
3. Label file: "intcoin-wallet-backup-2026-01-02"
```

**Step 5: Receive INT**
```
1. Click "Receive" tab
2. Click "Request payment"
3. Optional: Enter label and amount
4. Copy address or show QR code
```

### macOS

**Step 1: Download**
```
1. Visit: https://intcoin.org/downloads
2. Download "INTcoin-1.2.0-macos.dmg"
3. Open DMG file
4. Drag "INTcoin" to Applications folder
```

**Step 2-5**: Same as Windows above

**Data Directory**: `~/Library/Application Support/INTcoin/`

### Linux (Ubuntu/Debian)

**Step 1: Install via PPA** (Easiest)
```bash
# Add repository
sudo add-apt-repository ppa:intcoin/stable
sudo apt update

# Install
sudo apt install intcoin-qt

# Launch
intcoin-qt
```

**Step 2-5**: Same as Windows above

**Data Directory**: `~/.intcoin/`

---

## Option 3: Full Node

### Ubuntu 24.04+ (Recommended)

**Step 1: Install Dependencies**
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build \
  libboost-all-dev libssl-dev librocksdb-dev git
```

**Step 2: Build liboqs (Post-Quantum Crypto)**
```bash
git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git ~/liboqs-src
cd ~/liboqs-src && mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$HOME/liboqs -DCMAKE_BUILD_TYPE=Release ..
ninja && ninja install
```

**Step 3: Build RandomX**
```bash
git clone --branch v1.2.1 https://github.com/tevador/RandomX.git ~/randomx-src
cd ~/randomx-src && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/randomx -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) && make install
```

**Step 4: Build INTcoin**
```bash
git clone https://github.com/INT-devs/intcoin.git ~/intcoin
cd ~/intcoin
cmake -B build \
  -DCMAKE_PREFIX_PATH="$HOME/liboqs;$HOME/randomx" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
```

**Step 5: Install**
```bash
sudo cmake --install build
```

**Step 6: Create Configuration**
```bash
mkdir -p ~/.intcoin
nano ~/.intcoin/intcoin.conf
```

Add this configuration:
```ini
# Network
listen=1
maxconnections=125

# RPC
server=1
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1

# Prometheus metrics (optional)
metrics.enabled=1
metrics.bind=127.0.0.1
metrics.port=9090
```

**Step 7: Start Node**
```bash
# Start as daemon
intcoind -daemon

# Check status
intcoin-cli getblockchaininfo
```

**Step 8: Create Wallet**
```bash
# Create new address
intcoin-cli getnewaddress "My Main Address"

# Get balance
intcoin-cli getbalance

# Backup wallet
intcoin-cli backupwallet ~/intcoin-wallet-backup-$(date +%Y%m%d).dat
```

See: [BUILD_GUIDE.md](BUILD_GUIDE.md)

---

## Getting Your First INT

### Testnet Faucet (Free)
```
1. Visit: https://testnet-faucet.intcoin.org
2. Enter your testnet address (starts with TINT1)
3. Complete CAPTCHA
4. Receive 10 test INT
```

**Switch to Testnet**:
```bash
# Add to intcoin.conf
testnet=1

# Or start with flag
intcoind -testnet
```

### Buy INT (Mainnet)

**Exchanges**:
- ExchangeA (coming soon)
- ExchangeB (coming soon)
- DEX: Use atomic swaps (see [ATOMIC_SWAPS.md](ATOMIC_SWAPS.md))

### Mine INT

See: [Mining Tutorial](#mining-quick-start) below

---

## Mining Quick Start

### Solo CPU Mining (Beginner)

**Step 1: Ensure Node is Running**
```bash
intcoin-cli getblockchaininfo
```

**Step 2: Start Mining**
```bash
# Mine to your address
intcoin-cli generatetoaddress 1 "INT1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh"
```

**Step 3: Check Mining Progress**
```bash
# Get mining info
intcoin-cli getmininginfo

# Check your balance
intcoin-cli getbalance
```

**Expected Hashrate**:
- Intel Core i5: ~1-2 KH/s
- Intel Core i7: ~3-5 KH/s
- AMD Ryzen 5: ~2-4 KH/s
- AMD Ryzen 9: ~6-10 KH/s

### Pool Mining (Recommended)

**Step 1: Choose a Pool**
- pool.intcoin.org (Official)
- mining-pool-1.com (Community)
- mining-pool-2.com (Community)

**Step 2: Download Miner**
```bash
# XMRig (supports RandomX)
wget https://github.com/xmrig/xmrig/releases/download/v6.21.0/xmrig-6.21.0-linux-x64.tar.gz
tar -xzf xmrig-6.21.0-linux-x64.tar.gz
cd xmrig-6.21.0
```

**Step 3: Configure**
```bash
nano config.json
```

Edit pool settings:
```json
{
  "pools": [
    {
      "url": "pool.intcoin.org:3333",
      "user": "INT1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
      "pass": "x",
      "keepalive": true
    }
  ]
}
```

**Step 4: Start Mining**
```bash
./xmrig
```

See: [MINING.md](MINING.md), [POOL_SETUP.md](POOL_SETUP.md)

---

## Common Tasks

### Check Balance
```bash
intcoin-cli getbalance
```

### Send INT
```bash
intcoin-cli sendtoaddress "INT1receiver..." 10.5
```

### List Transactions
```bash
intcoin-cli listtransactions "*" 10
```

### Get New Address
```bash
intcoin-cli getnewaddress "Label"
```

### Check Sync Status
```bash
intcoin-cli getblockchaininfo | grep -E "blocks|headers"
```

### Export Private Key (Backup)
```bash
# Get address private key
intcoin-cli dumpprivkey "INT1qxy..."

# ⚠️ Keep this secret!
```

---

## Troubleshooting

### Node won't start

**Check logs**:
```bash
tail -f ~/.intcoin/debug.log
```

**Common issues**:
- Port 9333 already in use → Change port in config
- Corrupt database → Delete and resync: `rm -rf ~/.intcoin/blocks ~/.intcoin/chainstate`

### Wallet shows 0 balance

**Wait for sync**:
```bash
intcoin-cli getblockchaininfo
# Check: blocks == headers (fully synced)
```

### Transaction not confirming

**Check mempool**:
```bash
intcoin-cli getrawmempool
```

**Increase fee** (if needed):
```bash
intcoin-cli settxfee 0.0001  # Higher fee
```

### Can't connect to peers

**Add seed nodes**:
```bash
# Add to intcoin.conf
addnode=seed1.intcoin.org
addnode=seed2.intcoin.org
addnode=seed3.intcoin.org
```

---

## Security Best Practices

### Wallet Security
1. ✅ **Encrypt wallet** with strong passphrase
2. ✅ **Backup regularly** to multiple locations (USB, paper)
3. ✅ **Never share** private keys or seed phrases
4. ✅ **Use hardware wallets** for large amounts (coming soon)
5. ✅ **Verify addresses** before sending (check twice!)

### Node Security
1. ✅ **Firewall**: Only allow port 9333 (P2P)
2. ✅ **RPC**: Bind to 127.0.0.1 only (no external access)
3. ✅ **Strong RPC password**: Use long random password
4. ✅ **Keep updated**: Install security patches promptly
5. ✅ **Monitor logs**: Watch for suspicious activity

### Privacy Tips
1. ✅ **Use Tor**: Route connections through Tor for anonymity
2. ✅ **New addresses**: Use a new address for each transaction
3. ✅ **Avoid address reuse**: Don't publish addresses publicly
4. ✅ **Mobile privacy**: Use high FPR for bloom filters

See: [PRIVACY.md](PRIVACY.md), [SECURITY.md](../SECURITY.md)

---

## Next Steps

**Learn More**:
- [Architecture](ARCHITECTURE.md) - How INTcoin works
- [Cryptography](CRYPTOGRAPHY.md) - Post-quantum security
- [Mining Guide](MINING.md) - Advanced mining
- [RPC Commands](RPC.md) - Full command reference

**New Features (v1.2.0)**:
- [Atomic Swaps](ATOMIC_SWAPS.md) - Trade with other blockchains
- [Cross-Chain Bridges](CROSS_CHAIN_BRIDGES.md) - Bridge to ETH/BTC
- [Prometheus Metrics](PROMETHEUS_METRICS.md) - Monitor your node
- [Mobile Wallets](MOBILE_SDK.md) - Build mobile apps

**Community**:
- Website: https://intcoin.org
- Forum: https://forum.intcoin.org
- Discord: https://discord.gg/intcoin
- GitHub: https://github.com/INT-devs/intcoin
- Twitter: @INTcoin

---

## Getting Help

**Documentation**: All docs in `docs/` folder

**Report Issues**: https://github.com/INT-devs/intcoin/issues

**Security Issues**: See [SECURITY.md](../SECURITY.md)

**FAQ**: https://intcoin.org/faq

---

**Congratulations!** 🎉 You're now part of the INTcoin network!

**Last Updated**: January 2, 2026
