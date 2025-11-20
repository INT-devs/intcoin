# INTcoin Build Guide

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

Complete build instructions for all supported platforms.

---

## Table of Contents

- [System Requirements](#system-requirements)
- [Dependencies](#dependencies)
- [Linux Build](#linux-build)
- [macOS Build](#macos-build)
- [FreeBSD Build](#freebsd-build)
- [Windows Build](#windows-build)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements

- **CPU**: 64-bit processor (x86_64 or ARM64)
- **RAM**: 4 GB
- **Disk**: 10 GB free space
- **OS**: Linux, macOS 10.15+, FreeBSD 12+, Windows 10+

### Recommended

- **CPU**: Multi-core processor (4+ cores)
- **RAM**: 8 GB or more
- **Disk**: 50 GB SSD
- **Network**: Broadband internet connection

**Note**: INTcoin requires 64-bit architecture. 32-bit systems are NOT supported.

---

## Dependencies

### Core Dependencies

| Dependency | Minimum Version | Purpose |
|------------|----------------|---------|
| **CMake** | 4.0 | Build system |
| **C++ Compiler** | GCC 11+, Clang 14+, MSVC 2022+ | C++23 support required |
| **Boost** | 1.70.0 | Utility libraries |
| **OpenSSL** | 3.0 | Cryptography (AES-256-GCM) |
| **RocksDB** | Latest | Database engine |
| **liboqs** | Latest | Post-quantum cryptography |

### Optional Dependencies

| Dependency | Purpose |
|------------|---------|
| **Qt5/Qt6** | GUI wallet (Qt6 6.2+ or Qt5 5.9+) |
| **libusb** | Hardware wallet support |
| **hidapi** | Hardware wallet HID protocol |
| **Python 3** | Build scripts and tools |

---

## Linux Build

### Quick Install (Ubuntu/Debian)

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Run installation script
./scripts/install_linux.sh
```

### Manual Build

#### 1. Install Dependencies

**Ubuntu/Debian**:
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libssl-dev \
    librocksdb-dev \
    liboqs-dev \
    qtbase5-dev \
    qttools5-dev \
    python3 \
    libusb-1.0-0-dev \
    libhidapi-dev
```

**Fedora/RHEL/CentOS**:
```bash
sudo dnf install -y \
    gcc gcc-c++ \
    cmake \
    git \
    boost-devel \
    openssl-devel \
    rocksdb-devel \
    liboqs-devel \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    python3 \
    libusb-devel \
    hidapi-devel
```

**Arch Linux**:
```bash
sudo pacman -Syu \
    base-devel \
    cmake \
    git \
    boost \
    openssl \
    rocksdb \
    liboqs \
    qt5-base \
    qt5-tools \
    python \
    libusb \
    hidapi
```

#### 2. Build

```bash
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LIGHTNING=ON \
    -DENABLE_I2P=ON \
    -DENABLE_ML=ON

make -j$(nproc)
```

#### 3. Install

```bash
sudo make install
```

#### 4. Configuration

```bash
mkdir -p ~/.intcoin
cat > ~/.intcoin/intcoin.conf <<EOF
[rpc]
user=intcoin
password=$(openssl rand -hex 32)
EOF
```

---

## macOS Build

### Using Homebrew

#### 1. Install Dependencies

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake boost openssl@3 qt@6 python3 liboqs libusb hidapi
```

#### 2. Build

```bash
cd intcoin
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 \
    -DENABLE_LIGHTNING=ON \
    -DENABLE_I2P=ON \
    -DENABLE_ML=ON

make -j$(sysctl -n hw.ncpu)
```

#### 3. Run

```bash
./intcoind
```

### macOS-Specific Notes

- **Apple Silicon (M1/M2)**: Fully supported, use ARM64 binaries
- **Intel (x86_64)**: Fully supported
- **Qt Location**: Qt6 is at `/opt/homebrew/opt/qt@6` (ARM) or `/usr/local/opt/qt@6` (Intel)
- **OpenSSL**: Use `openssl@3` from Homebrew, not system OpenSSL

---

## FreeBSD Build

### Quick Install

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Run installation script
./scripts/install_freebsd.sh
```

### Manual Build

#### 1. Install Dependencies

```bash
pkg update
pkg install -y \
    cmake \
    git \
    boost-all \
    openssl \
    rocksdb \
    liboqs \
    qt5-buildtools \
    qt5-qmake \
    qt5-core \
    qt5-gui \
    qt5-widgets \
    python3 \
    libusb \
    hidapi
```

#### 2. Build

```bash
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LIGHTNING=ON \
    -DENABLE_I2P=ON \
    -DENABLE_ML=ON

gmake -j$(sysctl -n hw.ncpu)
```

#### 3. Install

```bash
gmake install
```

### FreeBSD-Specific Notes

- Use `gmake` (GNU Make), not `make` (BSD Make)
- Default prefix: `/usr/local`
- Service script: `/usr/local/etc/rc.d/intcoind`
- Enable at boot: `sysrc intcoind_enable=YES`

---

## Windows Build

### Option 1: Cross-Compile from Linux/macOS

```bash
# Install MinGW-w64
# macOS:
brew install mingw-w64

# Linux:
sudo apt install mingw-w64

# Build for Windows
./scripts/build_windows.sh
```

This creates `intcoin-v1.2.0-windows-x64.zip` with all binaries.

### Option 2: Native Windows Build (MSVC)

#### 1. Install Visual Studio 2022

Download from: https://visualstudio.microsoft.com/

Select "Desktop development with C++" workload.

#### 2. Install vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

#### 3. Install Dependencies

```powershell
.\vcpkg install boost:x64-windows openssl:x64-windows rocksdb:x64-windows qt5:x64-windows
```

**Note**: liboqs must be built separately (see [liboqs Windows instructions](https://github.com/open-quantum-safe/liboqs)).

#### 4. Build

```powershell
mkdir build
cd build

cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DENABLE_LIGHTNING=ON ^
    -DENABLE_I2P=ON ^
    -DENABLE_ML=ON

cmake --build . --config Release
```

#### 5. Package

```powershell
cmake --install . --prefix install
```

### Windows-Specific Notes

- Configuration directory: `%APPDATA%\INTcoin\`
- Use forward slashes in config paths: `C:/Users/Name/...`
- Firewall: Allow `intcoind.exe` and `intcoin-qt.exe`
- Run as Administrator for first-time setup

---

## Build Options

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Release, Debug, RelWithDebInfo) |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation directory |
| `BUILD_TESTS` | ON | Build test suite |
| `BUILD_QT_WALLET` | ON | Build Qt GUI wallet |
| `BUILD_DAEMON` | ON | Build intcoind daemon |
| `BUILD_CLI` | ON | Build intcoin-cli |
| `BUILD_MINER` | ON | Build CPU miner |
| `BUILD_EXPLORER` | ON | Build block explorer |
| `ENABLE_LIGHTNING` | OFF | Enable Lightning Network |
| `ENABLE_I2P` | ON | Enable I2P network |
| `ENABLE_ML` | ON | Enable Machine Learning |
| `ENABLE_WALLET` | ON | Enable wallet functionality |
| `ENABLE_SMART_CONTRACTS` | OFF | Enable smart contracts |
| `ENABLE_CROSS_CHAIN` | OFF | Enable cross-chain bridge |

### Build Examples

**Minimal Build** (daemon only):
```bash
cmake .. \
    -DBUILD_QT_WALLET=OFF \
    -DBUILD_CLI=OFF \
    -DBUILD_MINER=OFF \
    -DBUILD_EXPLORER=OFF \
    -DENABLE_I2P=OFF \
    -DENABLE_ML=OFF
```

**Full Build** (all features):
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LIGHTNING=ON \
    -DENABLE_I2P=ON \
    -DENABLE_ML=ON \
    -DBUILD_TESTS=ON
```

**Debug Build**:
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address"
```

---

## Troubleshooting

### Common Issues

#### CMake version error

```
Error: CMake 4.0 or higher is required
```

**Solution**:
```bash
# Install CMake 4.0+
pip3 install cmake --upgrade
```

#### Missing liboqs

```
Error: liboqs is required but not found
```

**Solution**:
- Linux: Run `scripts/install_liboqs_debian.sh`
- macOS: Run `scripts/install_liboqs_macos.sh`
- FreeBSD: Run `scripts/install_liboqs_freebsd.sh`

#### Qt not found

```
Error: Could not find Qt5 or Qt6
```

**Solution**:
```bash
# Specify Qt path
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt

# macOS with Homebrew:
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6
```

#### C++23 support error

```
Error: The compiler does not support C++23
```

**Solution**: Upgrade your compiler:
- GCC 11+ or Clang 14+
- MSVC 2022+

#### Boost version too old

```
Error: Boost 1.70.0 or higher is required
```

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# macOS
brew install boost

# Build from source if needed
```

#### Out of memory during build

```
Error: c++: fatal error: Killed signal terminated program cc1plus
```

**Solution**: Reduce parallel jobs:
```bash
make -j2  # Use only 2 cores instead of all
```

#### Windows: MSVC not found

```
Error: No CMAKE_CXX_COMPILER could be found
```

**Solution**: Install Visual Studio 2022 with C++ workload.

---

## Testing

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

### Run Specific Tests

```bash
# Crypto tests
./tests/test_crypto

# Lightning Network tests
./tests/test_lightning

# Blockchain tests
./tests/test_blockchain
```

### Expected Output

```
╔════════════════════════════════════════════╗
║   INTcoin Quantum Cryptography Tests     ║
╚════════════════════════════════════════════╝

✓ SHA3-256 tests passed
✓ All Dilithium tests passed
✓ All Kyber tests passed
✓ All address tests passed
✓ Random generation tests passed
✓ HKDF tests passed
✓ Mnemonic tests passed

╔════════════════════════════════════════════╗
║      ✓ ALL TESTS PASSED ✓                ║
╚════════════════════════════════════════════╝
```

---

## Build Performance

### Compilation Times (Reference Hardware)

| Hardware | Full Build | Incremental |
|----------|------------|-------------|
| M2 Max (12 cores) | 2-3 min | 30 sec |
| AMD Ryzen 9 (16 cores) | 3-4 min | 45 sec |
| Intel i7 (8 cores) | 5-6 min | 1 min |
| Raspberry Pi 4 | 30-40 min | 5 min |

### Disk Space

- Source code: ~50 MB
- Build directory: ~500 MB
- Installed binaries: ~100 MB
- Total (including tests): ~650 MB

---

## Platform-Specific Build Scripts

### Available Scripts

| Script | Platform | Description |
|--------|----------|-------------|
| `scripts/install_linux.sh` | Linux | Automated Linux installation |
| `scripts/install_freebsd.sh` | FreeBSD | Automated FreeBSD installation |
| `scripts/build_windows.sh` | Linux/macOS | Cross-compile for Windows |
| `scripts/install_liboqs_debian.sh` | Ubuntu/Debian | Install liboqs |
| `scripts/install_liboqs_macos.sh` | macOS | Install liboqs |
| `scripts/install_liboqs_freebsd.sh` | FreeBSD | Install liboqs |

### Usage Examples

```bash
# Linux automated install
sudo ./scripts/install_linux.sh --system

# FreeBSD with custom prefix
./scripts/install_freebsd.sh --prefix=/opt/intcoin

# Windows cross-compile
./scripts/build_windows.sh --no-gui
```

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build INTcoin

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y cmake libboost-all-dev libssl-dev
      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          make -j$(nproc)
      - name: Test
        run: cd build && ctest --output-on-failure
```

---

## Cross-Compilation

### ARM64 (aarch64)

```bash
# Install ARM64 cross-compiler
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build
cmake .. -DCMAKE_SYSTEM_NAME=Linux \
         -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
         -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++

make -j$(nproc)
```

### RISC-V

```bash
# RISC-V support is experimental
# Contact development team for assistance
```

---

## Additional Resources

- **Quick Start**: [QUICK-START.md](../QUICK-START.md)
- **Network Ports**: [NETWORK-PORTS.md](NETWORK-PORTS.md)
- **I2P Integration**: [I2P-INTEGRATION.md](I2P-INTEGRATION.md)
- **Machine Learning**: [MACHINE-LEARNING.md](MACHINE-LEARNING.md)
- **Security Audit**: [SECURITY-AUDIT.md](SECURITY-AUDIT.md)

---

## Getting Help

If you encounter build issues:

1. Check this documentation
2. Search closed issues: https://gitlab.com/intcoin/crypto/-/issues
3. Ask on community channels
4. Email: team@international-coin.org

---

**Last Updated**: November 20, 2025
**Version**: 1.2.0
**Status**: Complete ✅
