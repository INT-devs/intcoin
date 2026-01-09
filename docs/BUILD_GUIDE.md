# INTcoin Build Guide
**Version**: 1.2.0-beta
**Date**: January 2, 2026
**Status**: Production Beta
**Platforms**: Ubuntu, Debian, Fedora, Arch, macOS, FreeBSD, Windows

## What's New in v1.2.0-beta

- 📊 **Enhanced Mempool**: 6-level priority system with RocksDB persistence
- 📈 **Prometheus Metrics**: HTTP endpoint for monitoring (port 9090)
- 🔄 **Atomic Swaps**: HTLC-based cross-chain trading
- 🌉 **Cross-Chain Bridges**: ETH, BTC, BSC support
- 📱 **Mobile SPV**: Bloom filter support for mobile wallets
- ✅ **17 Test Suites**: Comprehensive testing (all passing)

---

## Table of Contents

- [Quick Start](#quick-start)
- [System Requirements](#system-requirements)
- [Dependencies](#dependencies)
- [Platform-Specific Instructions](#platform-specific-instructions)
- [Build Options](#build-options)
- [Mobile SDK Build](#mobile-sdk-build)
- [Troubleshooting](#troubleshooting)
- [Cross-Compilation](#cross-compilation)

---

## Quick Start

**For Experienced Developers** (Ubuntu 24.04+):

```bash
# Install system dependencies
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake ninja-build git \
  libboost-all-dev libssl-dev librocksdb-dev libqt6charts6-dev

# Build liboqs (post-quantum crypto)
git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git
cd liboqs && mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$HOME/liboqs -DCMAKE_BUILD_TYPE=Release ..
ninja && ninja install

# Build RandomX (ASIC-resistant PoW)
cd ../../
git clone --branch v1.2.1 https://github.com/tevador/RandomX.git
cd RandomX && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/randomx -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) && make install

# Build INTcoin
cd ../../
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
cmake -B build -DCMAKE_PREFIX_PATH="$HOME/liboqs;$HOME/randomx" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure
```

---

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **CPU** | 64-bit processor (x86_64 or ARM64) |
| **RAM** | 4 GB (8 GB recommended for building) |
| **Disk** | 10 GB free space |
| **OS** | Ubuntu 24.04+, Debian 13+, macOS 13+, FreeBSD 13+, Windows 11 |

### Recommended Requirements

| Component | Recommendation |
|-----------|----------------|
| **CPU** | 4+ cores for parallel builds |
| **RAM** | 16 GB for comfortable development |
| **Disk** | SSD with 50+ GB for full blockchain |
| **Network** | Stable internet for P2P synchronization |

---

## Dependencies

### Required Build Tools

- **Compiler**: GCC 11+ or Clang 13+ (C++23 support)
- **CMake**: 3.28 or higher
- **Build System**: Ninja (recommended) or Make
- **Git**: 2.30+ for source control

### Required Libraries

| Library | Version | Purpose |
|---------|---------|---------|
| **Boost** | 1.83.0+ | System utilities, threading, filesystem |
| **OpenSSL** | 3.0+ | SSL/TLS for network connections |
| **RocksDB** | 8.0+ | High-performance key-value database |
| **liboqs** | 0.10.0+ | Post-quantum cryptography (Dilithium3, Kyber768) |
| **RandomX** | 1.2.1+ | ASIC-resistant proof-of-work |
| **Qt** | 6.4+ | GUI wallet (optional, use `-DBUILD_WALLET_QT=ON`) |

---

## Platform-Specific Instructions

### Ubuntu / Debian

**Ubuntu 24.04 | Debian 13**

```bash
# Update package lists
sudo apt-get update

# Install build tools
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config

# Install required libraries
sudo apt-get install -y \
  libboost-all-dev \
  libssl-dev \
  librocksdb-dev

# Install Qt6 for GUI wallet (optional)
sudo apt-get install -y \
  qt6-base-dev \
  libqt6charts6-dev

# Build liboqs
git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git ~/liboqs-src
cd ~/liboqs-src && mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$HOME/liboqs -DCMAKE_BUILD_TYPE=Release ..
ninja && ninja install

# Build RandomX
git clone --branch v1.2.1 https://github.com/tevador/RandomX.git ~/randomx-src
cd ~/randomx-src && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/randomx -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) && make install

# Build INTcoin
cd ~
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
cmake -B build \
  -DCMAKE_PREFIX_PATH="$HOME/liboqs;$HOME/randomx" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_WALLET_QT=ON
cmake --build build -j$(nproc)

# Install (optional)
sudo cmake --install build
```

---

### macOS

**macOS 12 (Monterey)+, Intel & Apple Silicon (M1/M2/M3)**

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies via Homebrew
brew install cmake ninja boost openssl@3 rocksdb qt@6 git

# Build liboqs
git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git ~/liboqs-src
cd ~/liboqs-src && mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$HOME/liboqs -DCMAKE_BUILD_TYPE=Release ..
ninja && ninja install

# Build RandomX
git clone --branch v1.2.1 https://github.com/tevador/RandomX.git ~/randomx-src
cd ~/randomx-src && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/randomx -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu) && make install

# Build INTcoin
cd ~
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
cmake -B build \
  -DCMAKE_PREFIX_PATH="$HOME/liboqs;$HOME/randomx;$(brew --prefix qt@6)" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_WALLET_QT=ON \
  -DQt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
cmake --build build -j$(sysctl -n hw.ncpu)
```

**Apple Silicon Note**: All dependencies build natively on ARM64.

---

### FreeBSD

**FreeBSD 13, 14**

```bash
# Install dependencies
sudo pkg install -y \
  cmake ninja boost-all openssl rocksdb qt6 git

# Build liboqs
git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git ~/liboqs-src
cd ~/liboqs-src && mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$HOME/liboqs -DCMAKE_BUILD_TYPE=Release ..
ninja && ninja install

# Build RandomX
git clone --branch v1.2.1 https://github.com/tevador/RandomX.git ~/randomx-src
cd ~/randomx-src && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/randomx -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu) && gmake install

# Build INTcoin
cd ~
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
cmake -B build \
  -DCMAKE_PREFIX_PATH="$HOME/liboqs;$HOME/randomx" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON
cmake --build build -j$(sysctl -n hw.ncpu)
```

---

### Windows

**Windows 10/11 (64-bit)**

#### Option 1: WSL2 (Recommended)

Follow Ubuntu instructions inside WSL2.

#### Option 2: Native Build (MSVC)

```powershell
# Install Visual Studio 2022 with C++ workload
# Install vcpkg for dependencies
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install boost:x64-windows openssl:x64-windows rocksdb:x64-windows qt6:x64-windows

# Build liboqs
git clone https://github.com/open-quantum-safe/liboqs.git C:\liboqs-src
cd C:\liboqs-src
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=C:\liboqs ..
cmake --build . --config Release
cmake --install .

# Build RandomX
git clone https://github.com/tevador/RandomX.git C:\randomx-src
cd C:\randomx-src
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=C:\randomx ..
cmake --build . --config Release
cmake --install .

# Build INTcoin
git clone https://github.com/INT-devs/intcoin.git C:\intcoin
cd C:\intcoin
cmake -B build ^
  -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\vcpkg\installed\x64-windows;C:\liboqs;C:\randomx" ^
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DBUILD_TESTS=ON
cmake --build build --config Release
```

---

## Build Options

### CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build test suite (17 tests) |
| `BUILD_WALLET_QT` | OFF | Build Qt6 GUI wallet |
| `BUILD_BENCH` | OFF | Build performance benchmarks |
| `CMAKE_BUILD_TYPE` | Release | Build type: Release, Debug, RelWithDebInfo |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation directory |
| `ENABLE_HARDENING` | ON | Enable security hardening flags |
| `ENABLE_LTO` | OFF | Enable Link Time Optimization |
| `WARNINGS_AS_ERRORS` | ON | Treat warnings as errors |
| `RELAXED_DEPENDENCIES` | OFF | Allow older dependency versions (for CI) |

### Build Type Comparison

| Build Type | Optimizations | Debug Info | Use Case |
|------------|---------------|------------|----------|
| **Release** | `-O3` | No | Production, performance testing |
| **Debug** | `-O0 -g` | Full | Development, debugging |
| **RelWithDebInfo** | `-O2 -g` | Yes | Profiling, production debugging |
| **MinSizeRel** | `-Os` | No | Embedded systems, size-constrained |

### Example Configurations

**Development Build**:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DWARNINGS_AS_ERRORS=OFF
```

**Production Build**:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_WALLET_QT=ON \
  -DENABLE_LTO=ON
```

**CI Build**:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DRELAXED_DEPENDENCIES=ON \
  -DWARNINGS_AS_ERRORS=OFF
```

---

## Mobile SDK Build

INTcoin v1.2.0-beta includes native mobile SDKs for iOS and Android.

### iOS SDK (Swift)

**Requirements**:
- macOS 13+ (Ventura)
- Xcode 15+
- Swift 5.9+
- CocoaPods or Swift Package Manager

**Build Instructions**:

```bash
# Clone repository
cd mobile/ios

# Install dependencies with CocoaPods
pod install

# Open in Xcode
open INTcoinKit.xcworkspace

# Build framework
xcodebuild -workspace INTcoinKit.xcworkspace \
  -scheme INTcoinKit \
  -configuration Release \
  -destination 'generic/platform=iOS' \
  -derivedDataPath .build

# Run tests
xcodebuild test -workspace INTcoinKit.xcworkspace \
  -scheme INTcoinKit \
  -destination 'platform=iOS Simulator,name=iPhone 15'
```

**Swift Package Manager**:

```swift
// Package.swift
dependencies: [
    .package(url: "https://github.com/INT-devs/intcoin-ios.git", from: "1.2.0")
]
```

See: [Mobile SDK Documentation](MOBILE_SDK.md)

---

### Android SDK (Kotlin)

**Requirements**:
- JDK 17+
- Android SDK 34+ (API Level 34)
- Kotlin 1.9+
- Gradle 8.5+

**Build Instructions**:

```bash
# Clone repository
cd mobile/android

# Build SDK
./gradlew assembleRelease

# Run tests
./gradlew test

# Generate AAR
./gradlew bundleReleaseAar

# Built SDK location
ls build/outputs/aar/intcoin-sdk-release.aar
```

**Gradle Dependency**:

```kotlin
// build.gradle.kts
dependencies {
    implementation("org.intcoin:intcoin-sdk:1.2.0")
}
```

**Maven**:

```xml
<dependency>
    <groupId>org.intcoin</groupId>
    <artifactId>intcoin-sdk</artifactId>
    <version>1.2.0</version>
</dependency>
```

See: [Mobile SDK Documentation](MOBILE_SDK.md)

---

## Troubleshooting

### Common Issues

#### 1. "CMake version too old"

**Error**: `CMake 3.28 or higher is required`

**Solution**:
```bash
# Ubuntu/Debian: Use Kitware's official repository
sudo apt-get install -y software-properties-common
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update && sudo apt-get install cmake

# macOS
brew upgrade cmake

# From source
wget https://cmake.org/files/v3.28/cmake-3.28.0.tar.gz
tar xzf cmake-3.28.0.tar.gz && cd cmake-3.28.0
./bootstrap && make -j$(nproc) && sudo make install
```

#### 2. "Boost version too old"

**Error**: `Boost 1.83.0 or higher required`

**Solution**:
```bash
# Build Boost from source
wget https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.gz
tar xzf boost_1_83_0.tar.gz && cd boost_1_83_0
./bootstrap.sh --prefix=$HOME/boost
./b2 -j$(nproc) install
```

Then build INTcoin with:
```bash
cmake -B build -DCMAKE_PREFIX_PATH="$HOME/boost;$HOME/liboqs;$HOME/randomx"
```

#### 3. "liboqs not found"

**Error**: `Could NOT find liboqs`

**Solution**:
Ensure liboqs is installed and CMake can find it:
```bash
# Verify installation
ls $HOME/liboqs/lib/liboqs.* 

# Specify path explicitly
cmake -B build -Dliboqs_DIR=$HOME/liboqs/lib/cmake/liboqs
```

#### 4. "Qt6 not found" (GUI wallet)

**Error**: `Could NOT find Qt6`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev libqt6charts6-dev

# macOS
cmake -B build -DQt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6

# Or disable GUI wallet
cmake -B build -DBUILD_WALLET_QT=OFF
```

#### 5. Linker errors with liboqs

**Error**: `undefined reference to 'pqcrystals_dilithium3_ref_keypair'`

**Solution**:
liboqs may not be building all algorithms. Rebuild with:
```bash
cd ~/liboqs-src/build
cmake .. -DOQS_BUILD_ONLY_LIB=OFF
ninja && ninja install
```

### Build Performance

**Slow Builds**:
- Use Ninja instead of Make: `-GNinja`
- Enable parallel builds: `-j$(nproc)` or `-j8`
- Use ccache: `export CMAKE_CXX_COMPILER_LAUNCHER=ccache`
- Build in Release mode for faster subsequent rebuilds

**Out of Memory**:
- Reduce parallel jobs: `-j2` instead of `-j8`
- Add swap space (Linux)
- Close other applications

---

## Cross-Compilation

### ARM64 on x86_64

```bash
# Install cross-compilation toolchain
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build for ARM64
cmake -B build-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64
```

### Static Linking

For portable binaries:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_STATIC=ON \
  -DCMAKE_FIND_LIBRARY_SUFFIXES=".a"
cmake --build build
```

---

## Next Steps

After successful build:

1. **Run Tests**: `cd build && ctest --output-on-failure`
2. **Install**: `sudo cmake --install build`
3. **Run Node**: `intcoind --daemon`
4. **Use CLI**: `intcoin-cli getblockcount`
5. **Start GUI Wallet**: `intcoin-qt`

---

## Documentation

**Core Documentation**:
- [Architecture](ARCHITECTURE.md)
- [API Reference](API_REFERENCE.md)
- [RPC Commands](RPC.md)
- [Testing Guide](TESTING.md)

**New Features (v1.2.0)**:
- [Enhanced Mempool](ENHANCED_MEMPOOL.md)
- [Prometheus Metrics](PROMETHEUS_METRICS.md)
- [Atomic Swaps](ATOMIC_SWAPS.md)
- [Cross-Chain Bridges](CROSS_CHAIN_BRIDGES.md)
- [SPV & Bloom Filters](SPV_AND_BLOOM_FILTERS.md)
- [Mobile SDK](MOBILE_SDK.md)
- [Grafana Dashboards](GRAFANA_DASHBOARDS.md)

**General**:
- [Contributing](../CONTRIBUTING.md)
- [Security Policy](../SECURITY.md)

---

**Maintainer**: INTcoin Development Team
**Contact**: team@international-coin.org
**Last Updated**: January 2, 2026
