# Building INTcoin from Source

**Version**: 1.0.0-alpha
**Last Updated**: November 26, 2025

Complete guide for building INTcoin on all supported platforms.

---

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Dependencies](#dependencies)
3. [macOS Build](#macos-build)
4. [Linux Build](#linux-build)
5. [FreeBSD Build](#freebsd-build)
6. [Windows Build](#windows-build)
7. [Cross-Compilation](#cross-compilation)
8. [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | macOS 10.15+, Linux 4.x+, FreeBSD 12+, Windows 10+ |
| **Architecture** | 64-bit (x86_64 or ARM64) |
| **CMake** | 4.2.0 or higher |
| **Compiler** | GCC 15.2+, Clang 17+, MSVC 2022+, Apple Clang 17+ |
| **RAM** | 4 GB minimum, 8 GB recommended |
| **Disk Space** | 2 GB for build, 50+ GB for full node |
| **C++ Standard** | C++20 |

### Recommended Specifications

- **CPU**: 4+ cores
- **RAM**: 16 GB
- **Storage**: SSD recommended
- **Network**: Broadband connection

---

## Dependencies

### Required Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **CMake** | ≥ 4.2.0 | Build system |
| **OpenSSL** | ≥ 3.5.4 | SHA3-256 hashing |
| **liboqs** | ≥ 0.15.0 | Post-quantum cryptography (Dilithium3, Kyber768) |
| **RandomX** | ≥ 1.2.1 | ASIC-resistant Proof-of-Work |
| **Threads** | System | Multi-threading support |

### Optional Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **Boost** | ≥ 1.89.0 | Utilities, filesystem, threading |
| **RocksDB** | ≥ 10.7 | Blockchain database |
| **Qt6** | ≥ 6.8 | Desktop wallet GUI |
| **libzmq** | ≥ 4.3 | ZeroMQ messaging |
| **libevent** | ≥ 2.1 | Event-driven networking |
| **libtor** | Latest | Tor integration |
| **libi2pd** | Latest | I2P integration |

---

## macOS Build

### Install Dependencies

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake

# Install optional dependencies
brew install boost openssl rocksdb qt6 zeromq libevent
```

### Build liboqs 0.15.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Build RandomX 1.2.1

```bash
# Clone and checkout
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Build INTcoin

```bash
# Clone repository
cd /path/to/intcoin
git clone https://gitlab.com/intcoin/crypto.git intcoin
cd intcoin

# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(sysctl -n hw.ncpu)

# Output: build/libintcoin_core.a (146 KB)
```

### Run Tests

```bash
cd build

# Run all tests
./tests/test_crypto     # Cryptography (5/5)
./tests/test_randomx    # RandomX PoW (6/6)
./tests/test_bech32     # Bech32 addresses (8/8)

# All tests: 19/19 passing
```

---

## Linux Build

### Ubuntu/Debian

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake pkg-config git

# Install optional dependencies
sudo apt install -y \
    libboost-all-dev \
    libssl-dev \
    librocksdb-dev \
    qt6-base-dev \
    libzmq3-dev \
    libevent-dev
```

### Build liboqs 0.15.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Build RandomX 1.2.1

```bash
# Clone and checkout
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Build INTcoin

```bash
# Clone repository
cd /path/to
git clone https://gitlab.com/intcoin/crypto.git intcoin
cd intcoin

# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Run tests
./tests/test_crypto
./tests/test_randomx
./tests/test_bech32
```

### Fedora/CentOS/RHEL

```bash
# Install build tools
sudo dnf install -y gcc-c++ cmake make git pkg-config

# Install optional dependencies
sudo dnf install -y \
    boost-devel \
    openssl-devel \
    rocksdb-devel \
    qt6-qtbase-devel \
    zeromq-devel \
    libevent-devel

# Build liboqs and RandomX (same as Ubuntu)
# Build INTcoin (same as Ubuntu)
```

---

## FreeBSD Build

### Install Dependencies

```bash
# Install build tools
sudo pkg install cmake git pkgconf gmake

# Install optional dependencies
sudo pkg install \
    boost-all \
    openssl \
    rocksdb \
    qt6 \
    zeromq \
    libevent
```

### Build liboqs 0.15.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

### Build RandomX 1.2.1

```bash
# Clone and checkout
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1

# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

### Build INTcoin

```bash
# Clone repository
cd /path/to
git clone https://gitlab.com/intcoin/crypto.git intcoin
cd intcoin

# Create build directory
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
gmake -j$(sysctl -n hw.ncpu)

# Run tests
./tests/test_crypto
./tests/test_randomx
./tests/test_bech32
```

---

## Windows Build

### Prerequisites

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/)
   - Workload: "Desktop development with C++"
   - Components: C++ CMake tools, C++ Clang tools

2. Install [vcpkg](https://github.com/microsoft/vcpkg)
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

### Install Dependencies

```powershell
# Install dependencies via vcpkg
.\vcpkg install boost openssl rocksdb qt6 zeromq libevent

# Integrate with Visual Studio
.\vcpkg integrate install
```

### Build liboqs

```powershell
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0

# Build
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

# Install
cmake --install . --prefix "C:\liboqs"
```

### Build RandomX

```powershell
# Clone and checkout
cd ..\..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1

# Build
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

# Install
cmake --install . --prefix "C:\RandomX"
```

### Build INTcoin

```powershell
# Clone repository
cd \path\to
git clone https://gitlab.com/intcoin/crypto.git intcoin
cd intcoin

# Create build directory
mkdir build
cd build

# Configure
cmake -G "Visual Studio 17 2022" -A x64 \
      -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake \
      ..

# Build
cmake --build . --config Release

# Run tests
ctest -C Release --output-on-failure
```

---

## Cross-Compilation

### Build Windows Binary on Linux

```bash
# Install MinGW cross-compiler
sudo apt install -y mingw-w64

# Configure for cross-compilation
mkdir build-windows && cd build-windows
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64.cmake \
      -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)
```

### Build ARM Binary on x86_64

```bash
# Install cross-compilation tools
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Configure for ARM64
mkdir build-arm64 && cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux.cmake \
      -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)
```

---

## Troubleshooting

### Common Issues

**1. CMake version too old**
```bash
# Error: CMake 3.x or older found, 4.2.0+ required

# Solution: Install CMake 4.2.0+
# macOS:
brew upgrade cmake

# Linux:
wget https://github.com/Kitware/CMake/releases/download/v4.2.0/cmake-4.2.0.tar.gz
tar xzf cmake-4.2.0.tar.gz
cd cmake-4.2.0
./bootstrap && make && sudo make install
```

**2. liboqs not found**
```bash
# Error: Could not find liboqs

# Solution: Ensure liboqs is installed in standard location
# Check: /opt/homebrew/lib/liboqs.a (macOS)
#        /usr/local/lib/liboqs.a (Linux)

# Re-install if necessary (see platform-specific instructions above)
```

**3. RandomX not found**
```bash
# Error: Could not find RandomX

# Solution: Install to user directory if sudo not available
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make install

# Update CMakeLists.txt to include ~/.local
```

**4. Compilation errors**
```bash
# Error: C++20 features not supported

# Solution: Update compiler
# GCC: sudo apt install gcc-15
# Clang: sudo apt install clang-17
# macOS: xcode-select --install
```

**5. Linking errors**
```bash
# Error: undefined reference to `OQS_SIG_alg_ml_dsa_65'

# Solution: Rebuild liboqs with correct flags
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release ..
```

### Build Flags

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release with debug symbols
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Optimized build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable specific features
cmake -DENABLE_LIGHTNING=ON \
      -DENABLE_TOR=ON \
      -DENABLE_I2P=ON \
      -DBUILD_TESTS=ON \
      ..
```

### Performance Optimization

```bash
# Enable native CPU optimizations
cmake -DCMAKE_CXX_FLAGS="-march=native -O3" ..

# Enable LTO (Link Time Optimization)
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..

# Profile-guided optimization (advanced)
# 1. Build with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make

# 2. Run tests/benchmarks to generate profile data
./tests/benchmark

# 3. Rebuild with profile data
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make
```

---

## Verification

### Verify Build

```bash
# Check library size
ls -lh build/libintcoin_core.a
# Expected: ~146 KB

# Check test binaries
ls -lh tests/test_*
# Expected: test_crypto, test_randomx, test_bech32

# Run all tests
for test in tests/test_*; do
    echo "Running $test..."
    ./$test
done
```

### Verify Installation

```bash
# Check installed files
ls -la /usr/local/lib/libintcoin_core.a
ls -la /usr/local/include/intcoin/

# Check executables (when implemented)
which intcoind
which intcoin-cli
which intcoin-qt
```

---

## Next Steps

After successful build:

1. [Run Tests](TESTING.md) - Verify functionality
2. [Development Setup](DEVELOPMENT_SETUP.md) - Configure development environment
3. [API Reference](API_REFERENCE.md) - Learn the API
4. [Contributing](CONTRIBUTING.md) - Start contributing

---

**Previous**: [Documentation Home](README.md)
**Next**: [Testing Guide](TESTING.md)
