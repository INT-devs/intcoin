# INTcoin v1.3.0-beta - FreeBSD Installation Guide

**Version**: 1.3.0-beta
**Platform**: FreeBSD 13.x, 14.x
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
- **OS**: FreeBSD 13.0 or later
- **CPU**: 64-bit processor (amd64 or arm64)
- **RAM**: 2GB minimum (4GB recommended)
- **Disk**: 10GB for pruned node, 500GB for full node
- **Network**: Broadband internet connection

### Recommended Requirements
- **OS**: FreeBSD 14.0+
- **CPU**: 4+ cores
- **RAM**: 8GB+
- **Disk**: 1TB SSD
- **Network**: Unmetered connection

---

## Building from Source

### 1. Install Dependencies

```sh
# Update package repository
pkg update

# Install build tools
pkg install \
    git \
    cmake \
    pkgconf \
    autoconf \
    automake \
    libtool \
    gmake

# Install Clang 16+ (if not already installed)
pkg install llvm16

# Install dependencies
pkg install \
    qt6-base \
    qt6-tools \
    qt6-svg \
    openssl \
    boost-libs \
    rocksdb \
    libevent \
    miniupnpc \
    libnatpmp \
    libzmq4 \
    libqrencode \
    protobuf
```

### 2. Clone Repository

```sh
# Clone the repository
git clone https://github.com/intcoin/intcoin.git
cd intcoin

# Checkout v1.3.0-beta
git checkout v1.3.0-beta

# View available tags
git tag -l
```

### 3. Build

```sh
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++16 \
    -DCMAKE_C_COMPILER=/usr/bin/clang16 \
    -DBUILD_QT_WALLET=ON \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON

# Build (use all CPU cores)
cmake --build . -j$(sysctl -n hw.ncpu)

# Run tests (optional but recommended)
ctest --output-on-failure

# View build results
ls -lh intcoind intcoin-cli intcoin-qt tests/test_*
```

### 4. Install

```sh
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

```sh
# Minimal build (daemon only)
cmake .. -DBUILD_QT_WALLET=OFF -DBUILD_TESTS=OFF

# With Lightning Network
cmake .. -DBUILD_LIGHTNING=ON

# With Tor support
cmake .. -DBUILD_TOR=ON

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# View all options
cmake .. -LAH
```

---

## Post-Installation Setup

### 1. Create Configuration File

```sh
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

### 2. Create rc.d Service Script (Optional)

```sh
# Create service script
sudo tee /usr/local/etc/rc.d/intcoind > /dev/null <<'EOF'
#!/bin/sh

# PROVIDE: intcoind
# REQUIRE: DAEMON cleanvar
# KEYWORD: shutdown

. /etc/rc.subr

name="intcoind"
rcvar="${name}_enable"
command="/usr/local/bin/intcoind"
pidfile="/var/run/${name}.pid"
intcoind_user="${name}"
intcoind_group="${name}"
intcoind_datadir="/var/db/intcoin"
intcoind_config="${intcoind_datadir}/intcoin.conf"

command_args="-daemon -conf=${intcoind_config} -datadir=${intcoind_datadir} -pid=${pidfile}"
start_precmd="${name}_prestart"
stop_cmd="${name}_stop"

intcoind_prestart()
{
    if [ ! -d "${intcoind_datadir}" ]; then
        install -d -o ${intcoind_user} -g ${intcoind_group} ${intcoind_datadir}
    fi
    if [ ! -f "${intcoind_config}" ]; then
        echo "Config file ${intcoind_config} not found"
        return 1
    fi
}

intcoind_stop()
{
    /usr/local/bin/intcoin-cli stop
    sleep 5
}

load_rc_config $name
run_rc_command "$1"
EOF

# Make executable
sudo chmod +x /usr/local/etc/rc.d/intcoind

# Create user and group
sudo pw useradd intcoind -d /var/db/intcoin -s /usr/sbin/nologin -c "INTcoin daemon"

# Create data directory
sudo mkdir -p /var/db/intcoin
sudo chown intcoind:intcoind /var/db/intcoin

# Enable service
sudo sysrc intcoind_enable="YES"

# Start service
sudo service intcoind start

# Check status
sudo service intcoind status
```

### 3. Firewall Configuration (pf)

```sh
# Edit /etc/pf.conf and add:
# Allow INTcoin P2P
pass in proto tcp to port 8333

# Apply firewall rules
sudo pfctl -f /etc/pf.conf
```

---

## Running INTcoin

### GUI Wallet

```sh
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

```sh
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

```sh
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

```sh
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

### Issue: "Shared library not found"

**Solution**:
```sh
# Check missing libraries
ldd /usr/local/bin/intcoin-qt

# Install missing Qt6 libraries
pkg install qt6-base qt6-tools

# Update library cache
ldconfig -R
```

### Issue: Build fails with C++23 error

**Solution**:
```sh
# Install Clang 16+
pkg install llvm16

# Configure with Clang 16
cmake .. \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++16 \
    -DCMAKE_C_COMPILER=/usr/bin/clang16
```

### Issue: RocksDB not found

**Solution**:
```sh
# Install RocksDB
pkg install rocksdb

# Or specify path
cmake .. -DRocksDB_ROOT=/usr/local
```

### Issue: Qt6 not found

**Solution**:
```sh
# Install Qt6
pkg install qt6-base qt6-tools qt6-svg

# Set Qt6 path
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/lib/qt6/cmake
```

### Issue: Permission denied on startup

**Solution**:
```sh
# Fix permissions
chmod +x /usr/local/bin/intcoin*

# Or reinstall
cd build
sudo cmake --install .
```

### Issue: Cannot connect to peers

**Solution**:
```sh
# Check firewall
sudo pfctl -sr | grep 8333

# Add manual peers (edit intcoin.conf)
addnode=node1.intcoin.org
addnode=node2.intcoin.org

# Restart daemon
intcoin-cli stop
intcoind -daemon
```

### Issue: High memory usage

**Solution**:
```sh
# Reduce database cache (intcoin.conf)
dbcache=1024

# Reduce max mempool size
maxmempool=150

# Enable pruning
prune=10000

# Restart daemon
```

### Getting Help

- **FreeBSD Forums**: https://forums.freebsd.org
- **INTcoin Documentation**: https://docs.intcoin.org
- **GitHub Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/intcoin

---

## FreeBSD-Specific Notes

### Using jails

```sh
# Create jail for INTcoin
sudo iocage create -r 14.0-RELEASE -n intcoin
sudo iocage start intcoin
sudo iocage console intcoin

# Inside jail, install INTcoin
pkg install intcoin
```

### ZFS Optimization

```sh
# Create ZFS dataset for blockchain
sudo zfs create -o mountpoint=/var/db/intcoin zroot/intcoin
sudo zfs set compression=lz4 zroot/intcoin
sudo zfs set atime=off zroot/intcoin
sudo zfs set recordsize=16k zroot/intcoin

# Set ownership
sudo chown -R $USER:$USER /var/db/intcoin
```

### Resource Limits

```sh
# Increase file descriptors (in /etc/login.conf)
# Add to default class:
# :openfiles-cur=4096:\
# :openfiles-max=8192:

# Rebuild login database
sudo cap_mkdb /etc/login.conf

# Verify
ulimit -n
```

---

## Security Recommendations

1. **Firewall Configuration**:
```sh
# Edit /etc/pf.conf
# Allow P2P, block RPC
pass in proto tcp to port 8333
block in proto tcp to port 8332
```

2. **Encrypt Wallet**:
```sh
intcoin-cli encryptwallet "your-secure-password"
```

3. **Regular Backups**:
```sh
# Backup wallet
intcoin-cli backupwallet /secure/location/wallet-backup.dat

# Or use ZFS snapshots
sudo zfs snapshot zroot/intcoin@backup-$(date +%Y%m%d)
```

4. **Run as Non-Root**:
```sh
# Never run intcoind as root
# Use dedicated user (see rc.d setup above)
```

---

## Upgrading

### From v1.2.0 to v1.3.0-beta

```sh
# 1. Backup wallet
intcoin-cli backupwallet ~/intcoin-wallet-backup.dat

# 2. Stop old version
intcoin-cli stop
# Or: sudo service intcoind stop

# 3. Build and install new version from source (see Building from Source section)

# 4. Start new version
intcoind -daemon
# Or: sudo service intcoind start

# 5. Verify upgrade
intcoin-cli getblockchaininfo | grep version
```

---

## Uninstalling

### Source Installation

```sh
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
**Platform**: FreeBSD
**Last Updated**: January 8, 2026
