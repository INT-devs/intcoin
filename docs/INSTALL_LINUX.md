# INTcoin v1.3.0-beta - Linux Installation Guide

**Version**: 1.3.0-beta
**Platform**: Linux (Debian, Ubuntu, Fedora, Arch, etc.)
**Last Updated**: January 8, 2026

---

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Building from Source](#building-from-source)
3. [Post-Installation Setup](#post-installation-setup)
4. [Running INTcoin](#running-intcoin)
5. [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements
- **OS**: Linux kernel 5.4+ (Ubuntu 20.04+, Debian 11+, Fedora 35+)
- **CPU**: 64-bit processor (x86_64 or ARM64)
- **RAM**: 2GB minimum (4GB recommended)
- **Disk**: 10GB for pruned node, 500GB for full node
- **Network**: Broadband internet connection

### Recommended Requirements
- **OS**: Linux kernel 6.0+
- **CPU**: 4+ cores
- **RAM**: 8GB+
- **Disk**: 1TB SSD
- **Network**: Unmetered connection

---

## Building from Source

### 1. Install Dependencies

#### Debian / Ubuntu

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    autoconf \
    libtool \
    bison

# Install dependencies
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6svg6-dev \
    libssl-dev \
    libboost-all-dev \
    librocksdb-dev \
    libevent-dev \
    libminiupnpc-dev \
    libnatpmp-dev \
    libzmq3-dev \
    libqrencode-dev \
    libprotobuf-dev \
    protobuf-compiler

# Install GCC 13+ (if not available)
sudo apt install -y gcc-13 g++-13
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
```

#### Fedora / RHEL / CentOS

```bash
# Install build tools
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake git pkgconfig autoconf libtool bison

# Install dependencies
sudo dnf install -y \
    qt6-qtbase-devel \
    qt6-qttools-devel \
    qt6-qtsvg-devel \
    openssl-devel \
    boost-devel \
    rocksdb-devel \
    libevent-devel \
    miniupnpc-devel \
    libnatpmp-devel \
    zeromq-devel \
    qrencode-devel \
    protobuf-devel \
    protobuf-compiler

# Install GCC 13+ (Fedora 38+)
sudo dnf install -y gcc-c++
```

#### Arch Linux

```bash
# Install build tools and dependencies
sudo pacman -S --needed \
    base-devel \
    cmake \
    git \
    qt6-base \
    qt6-tools \
    qt6-svg \
    openssl \
    boost \
    rocksdb \
    libevent \
    miniupnpc \
    libnatpmp \
    zeromq \
    qrencode \
    protobuf
```

### 2. Clone Repository

```bash
# Clone the repository
git clone https://github.com/intcoin/intcoin.git
cd intcoin

# Checkout v1.3.0-beta
git checkout v1.3.0-beta

# View tags (optional)
git tag -l
```

### 3. Build

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_QT_WALLET=ON \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON

# Build (use all CPU cores)
cmake --build . -j$(nproc)

# Run tests (optional but recommended)
ctest --output-on-failure

# View build summary
ls -lh intcoind intcoin-cli intcoin-qt tests/test_*
```

### 4. Install

```bash
# Install system-wide
sudo cmake --install .

# Or install to custom location
cmake --install . --prefix=$HOME/.local

# Verify installation
intcoin --version
intcoind --version
intcoin-cli --version
```

### Build Options

```bash
# Minimal build (daemon only)
cmake .. -DBUILD_QT_WALLET=OFF -DBUILD_TESTS=OFF

# With Lightning Network
cmake .. -DBUILD_LIGHTNING=ON

# With Tor support
cmake .. -DBUILD_TOR=ON

# With I2P support
cmake .. -DBUILD_I2P=ON

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# View all options
cmake .. -LAH
```

---

## Post-Installation Setup

### 1. Create Configuration File

```bash
# Create data directory
mkdir -p ~/.intcoin

# Create configuration file
cat > ~/.intcoin/intcoin.conf <<EOF
# INTcoin Configuration

# Network
listen=1
maxconnections=125

# RPC (for intcoin-cli)
server=1
rpcuser=intcoinrpc
rpcpassword=$(openssl rand -hex 32)
rpcallowip=127.0.0.1

# Logging
debug=0
logtimestamps=1

# Performance
dbcache=4096
maxmempool=300

# Pruning (optional - saves disk space)
# prune=10000

# AssumeUTXO (fast sync)
assumevalid=1

EOF

# Secure the config file
chmod 600 ~/.intcoin/intcoin.conf
```

### 2. Create Systemd Service (Optional)

```bash
# Create service file
sudo tee /etc/systemd/system/intcoind.service > /dev/null <<EOF
[Unit]
Description=INTcoin daemon
After=network.target

[Service]
Type=forking
User=$USER
Group=$USER
ExecStart=/usr/local/bin/intcoind -daemon -conf=/home/$USER/.intcoin/intcoin.conf -datadir=/home/$USER/.intcoin
ExecStop=/usr/local/bin/intcoin-cli stop
Restart=on-failure
TimeoutStopSec=600
KillMode=process

# Hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
PrivateDevices=true

[Install]
WantedBy=multi-user.target
EOF

# Reload systemd
sudo systemctl daemon-reload

# Enable service
sudo systemctl enable intcoind

# Start service
sudo systemctl start intcoind

# Check status
sudo systemctl status intcoind

# View logs
sudo journalctl -u intcoind -f
```

### 3. Create Desktop Entry (GUI)

```bash
# Create desktop entry
cat > ~/.local/share/applications/intcoin.desktop <<EOF
[Desktop Entry]
Type=Application
Name=INTcoin Wallet
Comment=INTcoin cryptocurrency wallet
Icon=intcoin
Exec=intcoin-qt
Terminal=false
Categories=Finance;Network;
Keywords=cryptocurrency;bitcoin;wallet;
EOF

# Update desktop database
update-desktop-database ~/.local/share/applications/
```

---

## Running INTcoin

### GUI Wallet

```bash
# Start GUI wallet
intcoin-qt

# Start on testnet
intcoin-qt -testnet

# Start with specific data directory
intcoin-qt -datadir=/path/to/custom/dir

# Enable debug logging
intcoin-qt -debug=all
```

### Daemon (Background Service)

```bash
# Start daemon
intcoind -daemon

# Check if running
intcoin-cli getblockchaininfo

# Stop daemon
intcoin-cli stop

# View help
intcoind --help
```

### Command Line Interface

```bash
# Get wallet balance
intcoin-cli getbalance

# Get new receiving address
intcoin-cli getnewaddress

# Send transaction
intcoin-cli sendtoaddress <address> <amount>

# Get network info
intcoin-cli getnetworkinfo

# Get blockchain info
intcoin-cli getblockchaininfo

# List all commands
intcoin-cli help
```

### Lightning Network

```bash
# Open Lightning channel
intcoin-cli lightning openchannel <node_pubkey> <amount>

# List channels
intcoin-cli lightning listchannels

# Pay Lightning invoice
intcoin-cli lightning payinvoice <invoice>

# Create Lightning invoice
intcoin-cli lightning createinvoice <amount> <description>

# Lightning help
intcoin-cli help lightning
```

---

## Troubleshooting

### Issue: "Permission denied" when running

**Solution**:
```bash
# Make binary executable
chmod +x /path/to/intcoin-qt

# Or reinstall with correct permissions
sudo chmod +x /usr/local/bin/intcoin*
```

### Issue: "Library not found" errors

**Solution**:
```bash
# Check missing libraries
ldd /usr/local/bin/intcoin-qt | grep "not found"

# Install missing libraries (Debian/Ubuntu)
sudo apt install libqt6core6 libqt6gui6 libqt6widgets6

# Update library cache
sudo ldconfig
```

### Issue: Build fails with "C++23 required"

**Solution**:
```bash
# Install newer GCC
sudo apt install gcc-13 g++-13

# Or use Clang 16+
sudo apt install clang-16

# Configure with specific compiler
cmake .. -DCMAKE_CXX_COMPILER=g++-13
```

### Issue: Qt6 not found

**Solution**:
```bash
# Debian/Ubuntu
sudo apt install qt6-base-dev qt6-tools-dev

# Set Qt6 path explicitly
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
```

### Issue: RocksDB too old

**Solution**:
```bash
# Build RocksDB from source
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
git checkout v9.0.0
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# Point CMake to it
cmake .. -DRocksDB_ROOT=/usr/local
```

### Issue: Blockchain won't sync

**Solution**:
```bash
# Check connections
intcoin-cli getconnectioncount

# Add nodes manually (edit intcoin.conf)
addnode=node1.intcoin.org
addnode=node2.intcoin.org

# Try AssumeUTXO fast sync
intcoin-cli loadutxo snapshot.dat

# Reindex blockchain
intcoind -reindex
```

### Issue: High CPU usage during sync

**Solution**:
```bash
# Reduce validation threads (intcoin.conf)
par=2

# Reduce database cache
dbcache=1024

# Or wait - initial sync is CPU-intensive
```

### Issue: Out of disk space

**Solution**:
```bash
# Enable pruning (intcoin.conf)
prune=10000

# Or use external drive
intcoind -datadir=/mnt/external/intcoin
```

### Getting Help

- **Documentation**: https://docs.intcoin.org
- **GitHub Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/intcoin
- **Reddit**: https://reddit.com/r/intcoin

---

## Security Recommendations

1. **Firewall Configuration**:
```bash
# Allow INTcoin P2P port
sudo ufw allow 8333/tcp

# Allow RPC only from localhost (default)
sudo ufw deny 8332/tcp
```

2. **Encrypt Wallet**:
```bash
# In GUI: Settings > Encrypt Wallet
# Or via CLI:
intcoin-cli encryptwallet "your-secure-password"
```

3. **Regular Backups**:
```bash
# Backup wallet
intcoin-cli backupwallet /secure/location/wallet-backup.dat

# Backup entire data directory
tar -czf intcoin-backup-$(date +%Y%m%d).tar.gz ~/.intcoin/
```

4. **Use Strong RPC Password**:
```bash
# Generate strong password
openssl rand -hex 32

# Update in intcoin.conf
rpcpassword=<generated-password>
```

---

## Upgrading

### From v1.2.0 to v1.3.0-beta

```bash
# 1. Backup wallet
intcoin-cli backupwallet ~/intcoin-wallet-backup.dat

# 2. Stop old version
intcoin-cli stop

# 3. Install new version (see Quick Install above)

# 4. Start new version
intcoind -daemon

# 5. Verify upgrade
intcoin-cli getblockchaininfo | grep version
```

---

## Uninstalling

### Source Installation

```bash
# From build directory
cd build
sudo cmake --build . --target uninstall

# Or manually
sudo rm /usr/local/bin/intcoin*
sudo rm -rf /usr/local/share/intcoin

# Remove data directory (WARNING: deletes wallet!)
rm -rf ~/.intcoin
```

---

**Installation Guide Version**: 1.1
**For INTcoin**: v1.3.0-beta
**Platform**: Linux
**Last Updated**: January 8, 2026
