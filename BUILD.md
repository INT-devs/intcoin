# Building INTcoin

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This document provides comprehensive build instructions for all supported platforms.

## Table of Contents

- [Quick Start](#quick-start)
- [Prerequisites](#prerequisites)
- [Automated Build Scripts](#automated-build-scripts)
- [Manual Build](#manual-build)
- [Platform-Specific Guides](#platform-specific-guides)
- [Build Options](#build-options)
- [Creating Installers](#creating-installers)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

### Linux

```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
./build-linux.sh
```

### FreeBSD

```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
./build-freebsd.sh
```

### macOS

```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
brew install cmake boost openssl qt@5 python3
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### Windows

```powershell
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
.\build-windows.ps1
```

---

## Prerequisites

### All Platforms

- **CMake** 3.20 or later
- **C++23 compiler** (GCC 11+, Clang 14+, MSVC 2022+)
- **Git** for version control
- **Python** 3.9+ (for tests)

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libssl-dev \
    qtbase5-dev \
    qttools5-dev \
    libqt5svg5-dev \
    python3 \
    python3-pip
```

### Linux (Fedora/RHEL)

```bash
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y \
    cmake \
    git \
    boost-devel \
    openssl-devel \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    python3 \
    python3-pip
```

### Linux (Arch)

```bash
sudo pacman -S \
    base-devel \
    cmake \
    git \
    boost \
    openssl \
    qt5-base \
    qt5-tools \
    python \
    python-pip
```

### FreeBSD

```bash
pkg install -y \
    cmake \
    git \
    gmake \
    boost-all \
    openssl \
    qt5 \
    python3
```

### macOS

```bash
brew install cmake boost openssl qt@5 python3
```

### Windows

1. **Visual Studio 2022** or later (Community Edition)
   - Download: https://visualstudio.microsoft.com/downloads/
   - Select "Desktop development with C++"

2. **CMake** 3.20+
   - Download: https://cmake.org/download/

3. **vcpkg** (recommended)
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   .\vcpkg install boost:x64-windows openssl:x64-windows qt5:x64-windows
   ```

---

## Automated Build Scripts

We provide automated build scripts for convenience:

### build-linux.sh

```bash
# Standard release build
./build-linux.sh

# Clean build
./build-linux.sh --clean

# Debug build without GUI
./build-linux.sh --debug --no-qt

# Build with installation
./build-linux.sh --install

# Create .deb or .rpm package
./build-linux.sh --package

# Custom job count
./build-linux.sh --jobs 4

# View all options
./build-linux.sh --help
```

**Options:**
- `-c, --clean` - Clean build directory before building
- `-d, --debug` - Build in Debug mode (default: Release)
- `-t, --no-tests` - Skip building tests
- `-q, --no-qt` - Build without Qt GUI wallet
- `-i, --install` - Install after building
- `-p, --package` - Create .deb or .rpm package
- `-j N, --jobs N` - Use N parallel jobs

### build-freebsd.sh

Same options as Linux. FreeBSD uses `gmake` instead of `make`.

```bash
./build-freebsd.sh
./build-freebsd.sh --help
```

### build-windows.ps1

```powershell
# Standard release build
.\build-windows.ps1

# Clean build
.\build-windows.ps1 -Clean

# Debug build
.\build-windows.ps1 -BuildType Debug

# Create installer
.\build-windows.ps1 -WithInstaller

# Custom vcpkg path
.\build-windows.ps1 -VcpkgPath "D:\vcpkg"

# Custom job count
.\build-windows.ps1 -Jobs 4
```

**Parameters:**
- `-Clean` - Clean build directory before building
- `-BuildType` - Release or Debug (default: Release)
- `-WithInstaller` - Create NSIS installer and packages
- `-VcpkgPath` - Path to vcpkg installation (default: C:\vcpkg)
- `-Jobs` - Number of parallel jobs (default: 8)

---

## Manual Build

### Standard Build Process

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

# Build (adjust based on platform)
# Linux/macOS:
make -j$(nproc)
# FreeBSD:
gmake -j$(sysctl -n hw.ncpu)
# Windows:
cmake --build . --config Release -j8

# Run tests (optional)
make test  # or: ctest

# Install (optional)
sudo make install
```

### Cross-Platform CMake

CMake is the same across all platforms:

```bash
mkdir build && cd build
cmake .. [OPTIONS]
cmake --build . --config Release
```

---

## Build Options

CMake configuration options:

### Component Selection

```bash
# Disable Qt wallet GUI
cmake .. -DBUILD_QT_WALLET=OFF

# Disable daemon
cmake .. -DBUILD_DAEMON=OFF

# Disable CLI tool
cmake .. -DBUILD_CLI=OFF

# Disable CPU miner
cmake .. -DBUILD_MINER=OFF

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Disable block explorer
cmake .. -DBUILD_EXPLORER=OFF
```

### Feature Flags

```bash
# Disable Lightning Network support
cmake .. -DENABLE_LIGHTNING=OFF

# Disable smart contracts
cmake .. -DENABLE_SMART_CONTRACTS=OFF

# Disable cross-chain bridges
cmake .. -DENABLE_CROSS_CHAIN=OFF

# Disable wallet functionality
cmake .. -DENABLE_WALLET=OFF
```

### Build Type

```bash
# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug build (with symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Minimal size release
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

### Install Location

```bash
# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/intcoin

# Install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
```

### Compiler Selection

```bash
# Use specific GCC version
cmake .. -DCMAKE_C_COMPILER=gcc-13 \
         -DCMAKE_CXX_COMPILER=g++-13

# Use Clang
cmake .. -DCMAKE_C_COMPILER=clang \
         -DCMAKE_CXX_COMPILER=clang++
```

### Performance Optimization

```bash
# Native CPU optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# Link-time optimization (LTO)
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# Static linking (Windows)
cmake .. -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

---

## Creating Installers

### Linux Packages

#### Debian/Ubuntu (.deb)

```bash
cd build
cpack -G DEB

# Install
sudo dpkg -i INTcoin-*.deb

# Uninstall
sudo apt remove intcoin
```

#### Fedora/RHEL (.rpm)

```bash
cd build
cpack -G RPM

# Install
sudo rpm -i INTcoin-*.rpm

# Uninstall
sudo rpm -e intcoin
```

#### Generic (.tar.gz)

```bash
cd build
cpack -G TGZ

# Extract and use
tar xzf INTcoin-*.tar.gz
```

### FreeBSD Package

```bash
cd build
cpack -G TGZ
tar xzf INTcoin-*.tar.gz -C /usr/local
```

### Windows Installer

#### Using CPack (Built-in)

```powershell
cd build
cpack -G NSIS -C Release
# Creates: INTcoin-0.1.0-win64.exe

cpack -G ZIP -C Release
# Creates: INTcoin-0.1.0-win64.zip (portable)
```

#### Using NSIS Directly

```powershell
# Install NSIS from https://nsis.sourceforge.io/Download
# Then:
"C:\Program Files (x86)\NSIS\makensis.exe" installer.nsi
# Creates: INTcoin-Setup-0.1.0.exe
```

---

## Platform-Specific Guides

For detailed platform-specific instructions, see:

- **Windows**: [docs/BUILD-WINDOWS.md](docs/BUILD-WINDOWS.md)
- **Linux**: [docs/INSTALL-LINUX.md](docs/INSTALL-LINUX.md)
- **FreeBSD**: [docs/INSTALL-FREEBSD.md](docs/INSTALL-FREEBSD.md)

---

## Troubleshooting

### CMake Can't Find Dependencies

**Qt5:**
```bash
# Linux
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5

# macOS
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5

# Windows
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64
```

**Boost:**
```bash
cmake .. -DBoost_INCLUDE_DIR=/usr/local/include/boost
```

**OpenSSL:**
```bash
cmake .. -DOPENSSL_ROOT_DIR=/usr/local
```

### Compiler Errors

**C++23 not supported:**
```bash
# Update compiler
# Ubuntu:
sudo apt install gcc-11 g++-11
cmake .. -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_CXX_COMPILER=g++-11

# Or disable C++23 features (not recommended)
# Edit CMakeLists.txt: set(CMAKE_CXX_STANDARD 20)
```

**Out of memory during build:**
```bash
# Reduce parallel jobs
make -j2  # instead of -j$(nproc)
```

### Linking Errors

**Undefined reference to Boost:**
```bash
# Ensure Boost libraries are installed
# Ubuntu:
sudo apt install libboost-all-dev

# Or specify Boost directory
cmake .. -DBoost_LIBRARY_DIR=/usr/lib/x86_64-linux-gnu
```

### Qt Build Failures

**Disable Qt if not needed:**
```bash
cmake .. -DBUILD_QT_WALLET=OFF
```

**Install Qt development packages:**
```bash
# Ubuntu:
sudo apt install qtbase5-dev qttools5-dev libqt5svg5-dev

# Fedora:
sudo dnf install qt5-qtbase-devel qt5-qttools-devel

# FreeBSD:
pkg install qt5
```

### Windows vcpkg Issues

**Triplet mismatch:**
```powershell
# Ensure x64 architecture
.\vcpkg install boost:x64-windows openssl:x64-windows qt5:x64-windows

# For static linking
.\vcpkg install boost:x64-windows-static openssl:x64-windows-static
```

---

## Build Variants

### Minimal Build (Daemon Only)

```bash
cmake .. \
    -DBUILD_QT_WALLET=OFF \
    -DBUILD_MINER=OFF \
    -DBUILD_EXPLORER=OFF \
    -DBUILD_TESTS=OFF \
    -DENABLE_LIGHTNING=OFF \
    -DENABLE_SMART_CONTRACTS=OFF
```

### Full Build (All Features)

```bash
cmake .. \
    -DBUILD_QT_WALLET=ON \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_MINER=ON \
    -DBUILD_EXPLORER=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_LIGHTNING=ON \
    -DENABLE_SMART_CONTRACTS=ON \
    -DENABLE_CROSS_CHAIN=ON
```

### Server Build (No GUI)

```bash
cmake .. \
    -DBUILD_QT_WALLET=OFF \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON
```

---

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y build-essential cmake libboost-all-dev libssl-dev qt5-default
      - name: Build
        run: |
          ./build-linux.sh
      - name: Test
        run: |
          cd build && ctest

  windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Setup vcpkg
        run: |
          git clone https://github.com/Microsoft/vcpkg.git
          .\vcpkg\bootstrap-vcpkg.bat
          .\vcpkg\vcpkg integrate install
          .\vcpkg\vcpkg install boost:x64-windows openssl:x64-windows qt5:x64-windows
      - name: Build
        run: |
          .\build-windows.ps1
```

---

## Additional Resources

- **CMake Documentation**: https://cmake.org/documentation/
- **vcpkg**: https://vcpkg.io/
- **Qt Documentation**: https://doc.qt.io/
- **INTcoin GitLab**: https://gitlab.com/intcoin/crypto
- **Support**: team@international-coin.org

---

**Last Updated**: January 2025
