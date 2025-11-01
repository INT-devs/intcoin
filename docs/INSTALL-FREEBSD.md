# Installing INTcoin on FreeBSD

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This guide covers building and installing INTcoin on FreeBSD systems.

## Quick Start

```bash
# Install dependencies
pkg install cmake boost-all openssl qt5 python3 git

# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
gmake -j$(sysctl -n hw.ncpu)

# Install
sudo gmake install

# Run
intcoin-qt  # GUI wallet
# or
intcoind    # Daemon
```

---

## Detailed Installation

### FreeBSD 13.x / 14.x

#### Update System

```bash
# Update package repository
sudo pkg update
sudo pkg upgrade
```

#### Install Build Tools

```bash
# Install essential build tools
sudo pkg install -y \
    cmake \
    git \
    gmake \
    pkgconf \
    autoconf \
    automake \
    libtool
```

#### Install Dependencies

```bash
# Core dependencies
sudo pkg install -y \
    boost-all \
    openssl \
    libevent \
    db5 \
    miniupnpc \
    libzmq4

# Qt5 for GUI wallet
sudo pkg install -y \
    qt5-core \
    qt5-gui \
    qt5-widgets \
    qt5-network \
    qt5-buildtools \
    qt5-qmake \
    qt5-svg

# Python for tests
sudo pkg install -y \
    python3 \
    py39-pip

# Python test dependencies
pip install --user pytest pytest-asyncio
```

---

## Building INTcoin

### Standard Build

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Initialize submodules
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (use gmake on FreeBSD, not make)
gmake -j$(sysctl -n hw.ncpu)

# Run tests (optional)
ctest

# Install system-wide (optional)
sudo gmake install
```

### FreeBSD-Specific Considerations

#### Use gmake Instead of make

FreeBSD's default `make` is BSD make. Use `gmake` (GNU make) for building:

```bash
# Always use gmake
gmake -j$(sysctl -n hw.ncpu)

# Or set up alias
echo 'alias make=gmake' >> ~/.shrc
```

#### Clang vs GCC

FreeBSD uses Clang by default. To use GCC:

```bash
# Install GCC
sudo pkg install gcc13

# Configure with GCC
cmake .. -DCMAKE_C_COMPILER=gcc13 \
         -DCMAKE_CXX_COMPILER=g++13
```

#### Library Paths

FreeBSD installs packages to `/usr/local`. CMake usually finds them automatically, but if not:

```bash
cmake .. -DCMAKE_PREFIX_PATH=/usr/local \
         -DBoost_INCLUDE_DIR=/usr/local/include \
         -DOPENSSL_ROOT_DIR=/usr/local
```

### Build Options

```bash
# Minimal build (daemon only)
cmake .. -DBUILD_QT_WALLET=OFF \
         -DBUILD_MINER=OFF \
         -DBUILD_EXPLORER=OFF

# Full build
cmake .. -DBUILD_QT_WALLET=ON \
         -DBUILD_DAEMON=ON \
         -DBUILD_CLI=ON \
         -DBUILD_MINER=ON \
         -DBUILD_EXPLORER=ON

# Custom install location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/intcoin
```

---

## Installation

### System-wide Installation

```bash
cd build
sudo gmake install

# Verify
which intcoind
intcoind --version
```

Default install locations:
- Binaries: `/usr/local/bin/`
- Headers: `/usr/local/include/intcoin/`
- Config: `/usr/local/share/intcoin/`
- Documentation: `/usr/local/share/doc/intcoin/`

### User Installation

```bash
# Install to home directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
gmake install

# Add to PATH
echo 'setenv PATH $HOME/.local/bin:$PATH' >> ~/.cshrc
# or for sh/bash:
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.shrc
```

### Create FreeBSD Package

```bash
cd build
cpack -G TGZ

# Extract and install
tar xzf INTcoin-*.tar.gz -C /usr/local
```

---

## Running INTcoin

### First Run

```bash
# Create data directory
mkdir -p ~/.intcoin

# Copy sample config
cp /usr/local/share/intcoin/config/mainnet/intcoin.conf ~/.intcoin/

# Edit config (optional)
ee ~/.intcoin/intcoin.conf
# or
vi ~/.intcoin/intcoin.conf
```

### Start Daemon

```bash
# Start daemon
intcoind -daemon

# Check status
intcoin-cli getblockchaininfo

# Stop daemon
intcoin-cli stop
```

### Start GUI Wallet

```bash
intcoin-qt
```

If you get Qt library errors:

```bash
# Set library path
export LD_LIBRARY_PATH=/usr/local/lib/qt5
intcoin-qt
```

### Testnet

```bash
# Start testnet
intcoind -testnet -daemon

# Or with config
cp /usr/local/share/intcoin/config/testnet/intcoin.conf ~/.intcoin/intcoin-testnet.conf
intcoind -testnet -daemon -conf=$HOME/.intcoin/intcoin-testnet.conf
```

---

## RC Script (Auto-start on Boot)

Create an rc.d script for automatic startup:

```bash
# Create rc script
sudo ee /usr/local/etc/rc.d/intcoind
```

Add this content:

```bash
#!/bin/sh

# PROVIDE: intcoind
# REQUIRE: DAEMON cleanvar
# KEYWORD: shutdown

. /etc/rc.subr

name="intcoind"
rcvar="${name}_enable"

load_rc_config $name

: ${intcoind_enable:=NO}
: ${intcoind_user:=intcoin}
: ${intcoind_group:=intcoin}
: ${intcoind_datadir:=/var/db/intcoin}
: ${intcoind_config:=/usr/local/etc/intcoin/intcoin.conf}

command="/usr/local/bin/intcoind"
command_args="-daemon -conf=${intcoind_config} -datadir=${intcoind_datadir}"
pidfile="${intcoind_datadir}/intcoind.pid"

start_precmd="${name}_prestart"
stop_cmd="${name}_stop"

intcoind_prestart()
{
    if [ ! -d "${intcoind_datadir}" ]; then
        mkdir -p "${intcoind_datadir}"
        chown ${intcoind_user}:${intcoind_group} "${intcoind_datadir}"
    fi
}

intcoind_stop()
{
    su -m ${intcoind_user} -c "/usr/local/bin/intcoin-cli stop"
}

run_rc_command "$1"
```

Make executable and configure:

```bash
# Make executable
sudo chmod +x /usr/local/etc/rc.d/intcoind

# Create user
sudo pw useradd intcoin -d /var/db/intcoin -s /usr/sbin/nologin -c "INTcoin daemon"

# Create config directory
sudo mkdir -p /usr/local/etc/intcoin
sudo cp config/mainnet/intcoin.conf /usr/local/etc/intcoin/
sudo chown -R intcoin:intcoin /usr/local/etc/intcoin

# Create data directory
sudo mkdir -p /var/db/intcoin
sudo chown -R intcoin:intcoin /var/db/intcoin

# Enable service
sudo sysrc intcoind_enable=YES

# Start service
sudo service intcoind start

# Check status
sudo service intcoind status

# View logs
tail -f /var/db/intcoin/debug.log
```

---

## CPU Mining

### From GUI Wallet

1. Open `intcoin-qt`
2. Settings → Options → Mining
3. Enable CPU mining
4. Set threads (recommended: CPU cores - 1)
5. Start Mining

### Standalone Miner

```bash
# Mine with 4 threads
intcoin-miner -t 4

# Mine with all cores except 1
intcoin-miner -t $(expr $(sysctl -n hw.ncpu) - 1)

# Mine to specific address
intcoin-miner -t 4 -a INT1YourAddressHere
```

---

## Performance Tuning

### Compiler Optimizations

```bash
# Native CPU optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# Link-time optimization
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Kernel Tuning

For better network performance:

```bash
# Edit /etc/sysctl.conf
sudo ee /etc/sysctl.conf
```

Add:

```conf
# Increase network buffers
kern.ipc.maxsockbuf=16777216
net.inet.tcp.sendspace=8388608
net.inet.tcp.recvspace=8388608

# Increase max open files
kern.maxfiles=204800
kern.maxfilesperproc=200000

# Shared memory (for RandomX mining)
kern.ipc.shmmax=2147483648
kern.ipc.shmall=524288
```

Apply:

```bash
sudo sysctl -f /etc/sysctl.conf
```

### Large Page Support (for mining)

```bash
# Enable superpages
sudo sysctl vm.pmap.pg_ps_enabled=1

# Make permanent
echo 'vm.pmap.pg_ps_enabled=1' | sudo tee -a /etc/sysctl.conf
```

---

## Jails (Optional Isolation)

Run INTcoin in a FreeBSD jail for security:

```bash
# Create jail
sudo ezjail-admin create intcoin 'lo1|10.0.0.1'

# Install dependencies in jail
sudo ezjail-admin console intcoin
pkg install cmake boost-all openssl

# Copy INTcoin build into jail
sudo cp -R /path/to/intcoin /usr/jails/intcoin/usr/local/

# Start jail
sudo ezjail-admin start intcoin
```

---

## Troubleshooting

### Qt Libraries Not Found

```bash
# Set Qt path
export CMAKE_PREFIX_PATH=/usr/local/lib/qt5/cmake

# Or install qt5-buildtools
sudo pkg install qt5-buildtools
```

### Boost Version Issues

```bash
# FreeBSD 13+ has Boost 1.84+, which should work
# If needed, specify version:
cmake .. -DBoost_INCLUDE_DIR=/usr/local/include/boost
```

### OpenSSL Compatibility

```bash
# FreeBSD 13+ uses OpenSSL 1.1.1 or 3.0
# Force specific version if needed:
cmake .. -DOPENSSL_ROOT_DIR=/usr/local
```

### Permission Denied on /dev/crypto

```bash
# Add user to _crypto group for hardware crypto acceleration
sudo pw groupmod _crypto -m yourusername

# Or load crypto module
sudo kldload cryptodev
```

### Out of Memory During Build

```bash
# Limit parallel jobs
gmake -j2  # Instead of -j$(sysctl -n hw.ncpu)

# Or add swap
sudo dd if=/dev/zero of=/swapfile bs=1M count=4096
sudo chmod 600 /swapfile
sudo mdconfig -a -t vnode -f /swapfile -u 0
sudo swapon /dev/md0
```

---

## ZFS Optimizations

If using ZFS for blockchain data:

```bash
# Create dedicated dataset
sudo zfs create -o compression=lz4 \
                -o atime=off \
                -o recordsize=128k \
                zroot/intcoin

# Set mount point
sudo zfs set mountpoint=/var/db/intcoin zroot/intcoin

# Enable snapshots for backup
sudo zfs snapshot zroot/intcoin@backup-$(date +%Y%m%d)
```

---

## Security Recommendations

### Firewall (pf)

Edit `/etc/pf.conf`:

```conf
# Allow INTcoin P2P
pass in on $ext_if proto tcp from any to any port 8333

# Allow RPC only from localhost
pass in on lo0 proto tcp from 127.0.0.1 to any port 8332
block in on $ext_if proto tcp from any to any port 8332
```

Reload:

```bash
sudo pfctl -f /etc/pf.conf
```

### Wallet Security

```bash
# Encrypt wallet
intcoin-cli encryptwallet "secure-passphrase"

# Backup wallet
intcoin-cli backupwallet ~/intcoin-backup.dat

# Set permissions
chmod 600 ~/.intcoin/wallet.dat
```

---

## Uninstallation

### Manual Removal

```bash
# Remove binaries
sudo rm /usr/local/bin/intcoin*

# Remove libraries and headers
sudo rm -rf /usr/local/include/intcoin
sudo rm -rf /usr/local/lib/libintcoin*

# Remove config and data (WARNING: includes wallet!)
sudo rm -rf /usr/local/etc/intcoin
sudo rm -rf ~/.intcoin
sudo rm -rf /var/db/intcoin
```

### Remove Service

```bash
# Disable and remove service
sudo sysrc intcoind_enable=NO
sudo service intcoind stop
sudo rm /usr/local/etc/rc.d/intcoind
```

---

## FreeBSD-Specific Features

### DTrace Integration

Monitor INTcoin with DTrace:

```bash
# Monitor syscalls
sudo dtrace -n 'syscall:::entry /execname == "intcoind"/ { @[probefunc] = count(); }'

# Monitor file I/O
sudo dtrace -n 'io:::start /execname == "intcoind"/ { @[args[2]->fi_pathname] = count(); }'
```

### Capsicum Sandboxing

INTcoin can leverage FreeBSD's Capsicum for enhanced security (if compiled with support).

---

## Additional Resources

- FreeBSD Handbook: https://docs.freebsd.org/
- FreeBSD Forums: https://forums.freebsd.org/
- INTcoin Support: team@international-coin.org

---

**Last Updated**: January 2025
