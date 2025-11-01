# INTcoin Build Infrastructure

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

**Version**: 0.1.0
**Last Updated**: January 2025

---

## Overview

This document describes the complete build infrastructure for INTcoin, including automated build scripts, installer creation, and platform-specific build systems.

## Build System Architecture

### CMake-Based Build

INTcoin uses CMake 3.20+ as its build system, providing:

- **Cross-platform compatibility** (Windows, Linux, FreeBSD, macOS)
- **Modular component selection** (enable/disable features)
- **Multiple build configurations** (Release, Debug, RelWithDebInfo)
- **Package generation** (DEB, RPM, NSIS, ZIP, TGZ)
- **Automatic dependency detection**

### Build Configuration Files

```
intcoin/
├── CMakeLists.txt              # Root CMake configuration
├── src/
│   ├── crypto/CMakeLists.txt   # Cryptography module
│   ├── core/CMakeLists.txt     # Blockchain core
│   ├── network/CMakeLists.txt  # P2P networking
│   ├── consensus/CMakeLists.txt # Consensus engine
│   ├── wallet/CMakeLists.txt   # Wallet backend
│   ├── miner/CMakeLists.txt    # CPU miner
│   ├── rpc/CMakeLists.txt      # RPC server
│   └── qt/CMakeLists.txt       # Qt GUI wallet
├── external/
│   ├── randomx/CMakeLists.txt  # RandomX (future)
│   ├── dilithium/CMakeLists.txt # CRYSTALS-Dilithium
│   └── kyber/CMakeLists.txt    # CRYSTALS-Kyber
└── tests/CMakeLists.txt        # Test suite
```

---

## Automated Build Scripts

### build-linux.sh

**Location**: `./build-linux.sh`
**Purpose**: Automated Linux build with intelligent defaults

**Features**:
- Automatic dependency checking
- Color-coded status messages
- Parallel job control
- Package creation (.deb, .rpm, .tar.gz)
- Installation support
- Clean build option

**Usage**:
```bash
./build-linux.sh                # Standard release build
./build-linux.sh --clean        # Clean build
./build-linux.sh --debug        # Debug build
./build-linux.sh --no-qt        # Without GUI
./build-linux.sh --install      # Build and install
./build-linux.sh --package      # Create package
./build-linux.sh --jobs 4       # Use 4 threads
```

**Exit Codes**:
- `0` - Success
- `1` - Build/configuration failed

### build-freebsd.sh

**Location**: `./build-freebsd.sh`
**Purpose**: Automated FreeBSD build using gmake

**FreeBSD-Specific Features**:
- Uses `gmake` instead of `make`
- Uses `sysctl -n hw.ncpu` for CPU detection
- Compatible with FreeBSD 13.x and 14.x
- Supports both Clang (default) and GCC

**Usage**: Same as Linux script
```bash
./build-freebsd.sh
./build-freebsd.sh --help
```

### build-windows.ps1

**Location**: `.\build-windows.ps1`
**Purpose**: Automated Windows build with Visual Studio

**Features**:
- vcpkg integration
- Visual Studio 2022 support
- NSIS installer creation
- CPack package generation
- Parallel compilation control
- Clean build support

**Usage**:
```powershell
.\build-windows.ps1                         # Standard build
.\build-windows.ps1 -Clean                  # Clean build
.\build-windows.ps1 -BuildType Debug        # Debug build
.\build-windows.ps1 -WithInstaller          # Create installer
.\build-windows.ps1 -VcpkgPath "D:\vcpkg"  # Custom vcpkg
.\build-windows.ps1 -Jobs 8                 # 8 parallel jobs
```

**Parameters**:
- `-Clean` [switch] - Remove build directory
- `-BuildType` [Release|Debug] - Build configuration
- `-WithInstaller` [switch] - Create NSIS/ZIP packages
- `-VcpkgPath` [string] - vcpkg installation path (default: C:\vcpkg)
- `-Jobs` [int] - Parallel job count (default: 8)

---

## Installer Creation

### Windows Installer (NSIS)

**Script**: `installer.nsi`
**Output**: `INTcoin-Setup-0.1.0.exe`

**Components**:
1. **INTcoin Core** (required)
   - intcoind.exe
   - intcoin-cli.exe
   - intcoin-qt.exe

2. **CPU Miner** (optional)
   - intcoin-miner.exe

3. **Block Explorer** (optional)
   - intcoin-explorer.exe

4. **Configuration Files** (optional)
   - mainnet/testnet configs

5. **Documentation** (optional)
   - README, guides, docs

**Installation Features**:
- Start menu shortcuts
- Desktop shortcut
- Uninstaller
- Registry entries
- File associations

**Building the Installer**:
```powershell
# Method 1: Using build script
.\build-windows.ps1 -WithInstaller

# Method 2: Using NSIS directly
"C:\Program Files (x86)\NSIS\makensis.exe" installer.nsi

# Method 3: Using CPack
cd build
cpack -G NSIS -C Release
```

### Linux Packages

#### Debian/Ubuntu (.deb)
```bash
cd build
cpack -G DEB
sudo dpkg -i INTcoin-*.deb
```

**Package Contents**:
- Binaries → `/usr/local/bin/`
- Headers → `/usr/local/include/intcoin/`
- Config → `/usr/local/share/intcoin/`
- Docs → `/usr/local/share/doc/intcoin/`

#### Fedora/RHEL (.rpm)
```bash
cd build
cpack -G RPM
sudo rpm -i INTcoin-*.rpm
```

#### Generic (.tar.gz)
```bash
cd build
cpack -G TGZ
tar xzf INTcoin-*.tar.gz
```

### FreeBSD Package

```bash
cd build
cpack -G TGZ
sudo tar xzf INTcoin-*.tar.gz -C /usr/local
```

---

## Build Configurations

### Component Selection

CMake options for enabling/disabling components:

```cmake
# GUI Wallet
-DBUILD_QT_WALLET=ON|OFF          # Qt GUI wallet (default: ON)

# Core Components
-DBUILD_DAEMON=ON|OFF              # intcoind daemon (default: ON)
-DBUILD_CLI=ON|OFF                 # intcoin-cli tool (default: ON)

# Additional Tools
-DBUILD_MINER=ON|OFF               # CPU miner (default: ON)
-DBUILD_EXPLORER=ON|OFF            # Block explorer (default: ON)
-DBUILD_TESTS=ON|OFF               # Test suite (default: ON)

# Future Features
-DENABLE_LIGHTNING=ON|OFF          # Lightning Network (default: ON)
-DENABLE_SMART_CONTRACTS=ON|OFF    # Smart contracts (default: ON)
-DENABLE_CROSS_CHAIN=ON|OFF        # Cross-chain bridges (default: ON)
-DENABLE_WALLET=ON|OFF             # Wallet functionality (default: ON)
```

### Build Types

```cmake
-DCMAKE_BUILD_TYPE=Release         # Optimized, no debug symbols
-DCMAKE_BUILD_TYPE=Debug           # Debug symbols, no optimization
-DCMAKE_BUILD_TYPE=RelWithDebInfo  # Optimized + debug symbols
-DCMAKE_BUILD_TYPE=MinSizeRel      # Optimize for size
```

### Compiler Flags

**Release (default)**:
- GCC/Clang: `-O3 -DNDEBUG`
- MSVC: `/O2 /DNDEBUG`

**Debug**:
- GCC/Clang: `-O0 -g -DDEBUG`
- MSVC: `/Od /Zi /DDEBUG`

**Security Hardening**:
- GCC/Clang: `-fstack-protector-strong -fPIC`
- MSVC: `/GS`

---

## Platform-Specific Features

### Linux

**Supported Distributions**:
- Ubuntu 20.04, 22.04, 24.04
- Debian 11 (Bullseye), 12 (Bookworm)
- Fedora 38, 39, 40
- RHEL 8, 9 / Rocky Linux / AlmaLinux
- Arch Linux / Manjaro
- openSUSE Leap, Tumbleweed
- Gentoo

**Package Managers**:
- apt (Debian/Ubuntu)
- dnf (Fedora/RHEL)
- pacman (Arch)
- zypper (openSUSE)
- emerge (Gentoo)

**Build Tools**:
- GCC 11+ or Clang 14+
- CMake 3.20+
- GNU Make
- pkg-config

### FreeBSD

**Versions**: 13.x, 14.x

**Key Differences**:
- Uses `gmake` instead of `make`
- Uses `pkg` package manager
- Clang is default compiler
- Library paths in `/usr/local`
- Uses `sysctl` instead of `nproc`

**Special Features**:
- rc.d service script support
- DTrace integration
- Capsicum sandboxing (optional)
- ZFS optimization support
- Jail isolation support

### macOS

**Supported Versions**:
- macOS 11 (Big Sur)
- macOS 12 (Monterey)
- macOS 13 (Ventura)
- macOS 14 (Sonoma)

**Architectures**:
- x86_64 (Intel)
- arm64 (Apple Silicon)
- Universal binaries (both)

**Package Manager**: Homebrew

**Compilers**:
- Apple Clang (Xcode Command Line Tools)
- GCC (via Homebrew)

### Windows

**Versions**: Windows 10, 11 (x64)

**Compilers**:
- Visual Studio 2022 (MSVC 19.3+)
- Clang for Windows (optional)

**Package Manager**: vcpkg

**Architectures**:
- x64 (primary)
- ARM64 (experimental)

**Output Types**:
- NSIS installer (.exe)
- ZIP archive (portable)
- Development build (in-place)

---

## Dependency Management

### Linux (apt)

```bash
sudo apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libssl-dev \
    qtbase5-dev \
    qttools5-dev \
    libqt5svg5-dev \
    python3 \
    python3-pip
```

### FreeBSD (pkg)

```bash
pkg install -y \
    cmake \
    gmake \
    boost-all \
    openssl \
    qt5 \
    python3
```

### macOS (Homebrew)

```bash
brew install \
    cmake \
    boost \
    openssl \
    qt@5 \
    python3
```

### Windows (vcpkg)

```powershell
.\vcpkg install \
    boost:x64-windows \
    openssl:x64-windows \
    qt5:x64-windows
```

---

## Build Outputs

### Executables

| Binary | Description | Default Location |
|--------|-------------|------------------|
| `intcoind` | Blockchain daemon | `build/intcoind` |
| `intcoin-cli` | RPC client | `build/intcoin-cli` |
| `intcoin-qt` | GUI wallet | `build/intcoin-qt` |
| `intcoin-miner` | CPU miner | `build/intcoin-miner` |
| `intcoin-explorer` | Block explorer | `build/intcoin-explorer` |

### Libraries

| Library | Description |
|---------|-------------|
| `libintcoin_core.a` | Core blockchain library |
| `libcrypto.a` | Cryptography library |
| `libnetwork.a` | P2P networking library |

### Configuration Files

```
config/
├── mainnet/
│   └── intcoin.conf
└── testnet/
    └── intcoin.conf
```

---

## Performance Optimization

### Compiler Optimizations

```bash
# Native CPU optimization
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# Link-time optimization (LTO)
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# Profile-guided optimization (PGO)
cmake .. -DCMAKE_CXX_FLAGS="-fprofile-generate"
# Run and collect profiles
cmake .. -DCMAKE_CXX_FLAGS="-fprofile-use"
```

### Parallel Compilation

```bash
# Linux/macOS
make -j$(nproc)              # Use all cores
make -j4                     # Use 4 cores

# FreeBSD
gmake -j$(sysctl -n hw.ncpu)

# Windows
cmake --build . -j8
```

### Static vs Dynamic Linking

**Dynamic** (default):
- Smaller binaries
- Shared system libraries
- Faster build times

**Static** (Windows):
```powershell
cmake .. -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

---

## Testing Infrastructure

### Test Suites

1. **C++ Unit Tests** (`tests/`)
   - Cryptography tests
   - Blockchain tests
   - Network tests
   - Consensus tests

2. **Python Integration Tests**
   - RPC tests
   - P2P tests
   - Wallet tests

### Running Tests

```bash
# CMake/CTest
cd build
ctest                    # Run all tests
ctest -V                 # Verbose output
ctest -R crypto          # Run crypto tests only

# Python tests
cd tests
python3 -m pytest
pytest -v                # Verbose
pytest test_crypto.py    # Specific test
```

---

## Continuous Integration

### Supported CI Platforms

- GitHub Actions
- GitLab CI/CD
- Jenkins
- Travis CI
- CircleCI

### Example Workflow (GitHub Actions)

```yaml
name: Build

on: [push, pull_request]

jobs:
  linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - run: sudo apt install -y build-essential cmake libboost-all-dev
      - run: ./build-linux.sh
      - run: cd build && ctest

  windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - run: .\build-windows.ps1

  freebsd:
    runs-on: ubuntu-latest
    steps:
      - uses: vmactions/freebsd-vm@v1
        with:
          run: |
            pkg install -y cmake boost-all
            ./build-freebsd.sh
```

---

## Troubleshooting

### Common Build Issues

**CMake version too old**:
```bash
# Install newer CMake
pip3 install cmake --upgrade
```

**C++23 not supported**:
```bash
# Install newer compiler
sudo apt install gcc-13 g++-13
cmake .. -DCMAKE_CXX_COMPILER=g++-13
```

**Qt not found**:
```bash
# Specify Qt path
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5
```

**Out of memory**:
```bash
# Reduce parallel jobs
make -j2
```

**Boost linking errors**:
```bash
# Install all Boost libraries
sudo apt install libboost-all-dev
```

---

## Documentation

### Build Documentation

- [BUILD.md](BUILD.md) - Comprehensive build guide
- [docs/BUILD-WINDOWS.md](docs/BUILD-WINDOWS.md) - Windows guide
- [docs/INSTALL-LINUX.md](docs/INSTALL-LINUX.md) - Linux guide
- [docs/INSTALL-FREEBSD.md](docs/INSTALL-FREEBSD.md) - FreeBSD guide

### Quick Reference

- `./build-linux.sh --help` - Linux build options
- `./build-freebsd.sh --help` - FreeBSD build options
- `Get-Help .\build-windows.ps1` - Windows build help

---

## Summary

The INTcoin build infrastructure provides:

✅ **Cross-platform support** (Linux, FreeBSD, macOS, Windows)
✅ **Automated build scripts** (Linux, FreeBSD, Windows)
✅ **Package generation** (DEB, RPM, NSIS, TGZ, ZIP)
✅ **Modular component selection**
✅ **Multiple build configurations**
✅ **Comprehensive documentation**
✅ **CI/CD integration support**

All build tools are production-ready and tested on their respective platforms.

---

**Lead Developer**: Maddison Lane
**Contact**: team@international-coin.org
**Last Updated**: January 2025
