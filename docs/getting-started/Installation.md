# INTcoin Installation Guide

Complete installation guide for INTcoin on Linux, FreeBSD, macOS, and Windows.

## Table of Contents

- [System Requirements](#system-requirements)
- [Linux Installation](#linux-installation)
- [FreeBSD Installation](#freebsd-installation)
- [macOS Installation](#macos-installation)
- [Windows Installation](#windows-installation)
- [Building from Source](#building-from-source)
- [Verifying Installation](#verifying-installation)

---

## System Requirements

### Minimum Requirements
- **CPU**: 64-bit processor (x86_64 or ARM64)
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk**: 10 GB free space (blockchain grows over time)
- **Network**: Broadband internet connection

### Software Requirements
- **Compiler**: GCC 13+ or Clang 15+
- **CMake**: 3.20 or later
- **C++ Standard**: C++23
- **Operating System**: Linux, FreeBSD, macOS, or Windows 10+

### Dependencies
- **OpenSSL**: 3.0+ (for SHA3-256, networking)
- **Boost**: 1.75+ (optional, for some features)
- **RocksDB**: 6.0+ (for blockchain storage)
- **liboqs**: 0.8.0+ (for post-quantum cryptography)
- **RandomX**: 1.2.0+ (for proof-of-work)
- **Qt6**: 6.2+ (optional, for GUI wallet)

---

## Linux Installation

### Ubuntu/Debian

#### Using Installation Script

```bash
# Download and run the installation script
cd /path/to/intcoin
chmod +x install-linux.sh
sudo ./install-linux.sh
```

#### Manual Installation

**1. Install Dependencies**

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake git pkg-config

# Install required libraries
sudo apt install -y \
    libssl-dev \
    libboost-all-dev \
    librocksdb-dev

# Install optional dependencies
sudo apt install -y qt6-base-dev qt6-tools-dev
```

**2. Build and Install liboqs (Post-Quantum Crypto)**

```bash
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..
```

**3. Build and Install RandomX**

```bash
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..
```

**4. Build INTcoin**

```bash
cd intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

**5. Verify Installation**

```bash
intcoind --version
intcoin-cli --version
intcoin-miner --version
```

### Fedora/CentOS/RHEL

```bash
# Install dependencies
sudo dnf install -y gcc-c++ cmake git \
    openssl-devel boost-devel rocksdb-devel \
    qt6-qtbase-devel

# Build liboqs and RandomX (same as Ubuntu)
# Build INTcoin (same as Ubuntu)
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel cmake git \
    openssl boost rocksdb qt6-base

# Build liboqs and RandomX (same as Ubuntu)
# Build INTcoin (same as Ubuntu)
```

---

## FreeBSD Installation

### Using Installation Script

```bash
# Download and run the FreeBSD installation script
cd /path/to/intcoin
chmod +x install-freebsd.sh
sudo ./install-freebsd.sh
```

### Manual Installation

**1. Install Dependencies**

```bash
# Update ports tree
sudo portsnap fetch update

# Install from ports
sudo pkg install -y \
    cmake git \
    openssl boost-all \
    rocksdb qt6
```

**2. Build liboqs and RandomX**

```bash
# Same as Linux instructions above
```

**3. Build INTcoin**

```bash
cd intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

**4. Configure as Service**

```bash
# Copy rc.d script
sudo cp /path/to/intcoin/scripts/freebsd-rc.d/intcoind /usr/local/etc/rc.d/

# Enable service
sudo sysrc intcoind_enable="YES"

# Start service
sudo service intcoind start
```

---

## macOS Installation

### Using Homebrew

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl@3 boost rocksdb qt@6

# Build liboqs
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@3 ..
make -j$(sysctl -n hw.ncpu)
sudo make install
cd ../..

# Build RandomX
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
cd ../..

# Build INTcoin
cd intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@3 \
         -DQt6_DIR=/usr/local/opt/qt@6/lib/cmake/Qt6
make -j$(sysctl -n hw.ncpu)
sudo make install
```

---

## Windows Installation

### Using Pre-built Binaries

1. Download the latest release from the [Releases page](https://github.com/intcoin/intcoin/releases)
2. Extract the ZIP file
3. Run `intcoind.exe` from the extracted folder

### Building from Source

**Requirements:**
- Visual Studio 2022 (with C++ support)
- vcpkg (for dependency management)
- CMake 3.20+

**1. Install vcpkg**

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio
.\vcpkg integrate install
```

**2. Install Dependencies**

```powershell
.\vcpkg install openssl:x64-windows
.\vcpkg install boost:x64-windows
.\vcpkg install rocksdb:x64-windows
.\vcpkg install qt6:x64-windows
```

**3. Build liboqs and RandomX**

```powershell
# liboqs
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
cmake --install .
cd ..\..

# RandomX
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
cmake --install .
cd ..\..
```

**4. Build INTcoin**

```powershell
# Using PowerShell build script
cd intcoin
.\build-windows.ps1

# Or manually
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]\scripts\buildsystems\vcpkg.cmake `
         -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cmake --install .
```

**5. Cross-compile from Linux**

```bash
# Using MinGW cross-compiler
cd intcoin
chmod +x cross-build-windows.sh
./cross-build-windows.sh

# Output: intcoind.exe, intcoin-cli.exe, intcoin-qt.exe
```

---

## Building from Source

### Clone Repository

```bash
git clone https://github.com/intcoin/intcoin.git
cd intcoin
```

### Build Options

INTcoin supports several CMake build options:

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \      # Release or Debug
  -DBUILD_TESTS=ON \                # Build test suite
  -DBUILD_WALLET_QT=ON \            # Build Qt GUI wallet
  -DBUILD_DAEMON=ON \               # Build daemon (intcoind)
  -DBUILD_CLI=ON \                  # Build CLI client
  -DBUILD_MINER=ON \                # Build CPU miner
  -DBUILD_EXPLORER=ON \             # Build block explorer
  -DBUILD_FAUCET=ON \               # Build testnet faucet
  -DENABLE_LIGHTNING=OFF \          # Enable Lightning Network
  -DENABLE_TOR=OFF \                # Enable Tor support
  -DENABLE_I2P=OFF \                # Enable I2P support
  -DENABLE_GPU_MINING=OFF           # Enable GPU mining
```

### Build Commands

```bash
# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build all targets
make -j$(nproc)      # Linux/macOS
gmake -j$(sysctl -n hw.ncpu)  # FreeBSD

# Build specific target
make intcoind        # Daemon only
make intcoin-cli     # CLI only
make intcoin-qt      # Qt wallet only
make intcoin-miner   # Miner only
make intcoin-faucet  # Faucet only

# Install
sudo make install
```

### Testing

```bash
# Run all tests
ctest

# Run specific test
./tests/test_crypto
./tests/test_validation
./tests/test_wallet
```

---

## Verifying Installation

### Check Version

```bash
intcoind --version
# Output: INTcoin Daemon v1.0.0-alpha

intcoin-cli --version
# Output: INTcoin CLI v1.0.0-alpha

intcoin-miner --version
# Output: INTcoin Miner v1.0.0-alpha
```

### Test Daemon

```bash
# Start daemon
intcoind --testnet

# In another terminal, test RPC
intcoin-cli --testnet getinfo
```

### Test Qt Wallet

```bash
# Launch GUI wallet
intcoin-qt --testnet
```

### Test Miner

```bash
# Solo mine to address
intcoin-miner --solo --address=int1qxxxxxxxxxxx --threads=4
```

### Test Faucet

```bash
# Start faucet server
intcoin-faucet --testnet --port=2215

# Access web interface
# Open browser: http://localhost:2215/
```

---

## Common Issues

### Missing Dependencies

**Error**: `Could not find OpenSSL`

```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# macOS
brew install openssl@3
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl@3
```

### Compiler Too Old

**Error**: `requires at least C++23`

```bash
# Ubuntu 22.04+
sudo apt install gcc-13 g++-13
export CC=gcc-13
export CXX=g++-13
```

### RocksDB Not Found

```bash
# Build from source
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Port Already in Use

```bash
# Check what's using port 2210
sudo lsof -i :2210

# Use different port
intcoind --port=2220
```

---

## Next Steps

- [Quick Start Guide](Quick-Start.md)
- [Running intcoind](../user-guides/Running-Daemon.md)
- [Using the Wallet](../user-guides/Using-Wallet.md)
- [Mining Guide](../user-guides/Mining.md)
- [Testnet Faucet](../user-guides/Testnet-Faucet.md)

---

## Getting Help

- **Documentation**: https://intcoin.org/docs
- **Wiki**: https://gitlab.com/intcoin/crypto/-/wikis/
- **Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/7p4VmS2z
- **Telegram**: https://t.me/INTcoin_official
- **X (Twitter)**: https://x.com/INTcoin_team

---

*Last Updated: December 5, 2025*
