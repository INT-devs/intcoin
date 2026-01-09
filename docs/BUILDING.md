# Building INTcoin from Source

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

Complete guide for building INTcoin on all supported platforms.

## What's New in v1.2.0-beta

- 📊 **Enhanced Mempool**: 6-level priority system with persistence
- 📈 **Prometheus Metrics**: Monitoring endpoint on port 9090
- 🔄 **Atomic Swaps & Bridges**: Cross-chain functionality
- 📱 **Mobile SDKs**: iOS (Swift) and Android (Kotlin) support
- ✅ **17 Test Suites**: Comprehensive test coverage

**Note**: Automated installation scripts are available in `scripts/` for Linux and FreeBSD. See [Automated Installation](#automated-installation) below.

**Detailed Build Guide**: [BUILD_GUIDE.md](BUILD_GUIDE.md)

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
| **OS** | macOS 12.0+ (Monterey), Linux 5.x+, FreeBSD 13+, Windows 10 22H2+/Windows 11+ |
| **Architecture** | 64-bit (x86_64 or ARM64) |
| **CMake** | 3.28 or higher (latest stable from Kitware) |
| **Compiler** | GCC 13+, Clang 16+, MSVC 2022+, Apple Clang 15+ |
| **RAM** | 4 GB minimum, 8 GB recommended |
| **Disk Space** | 2 GB for build, 50+ GB for full node |
| **C++ Standard** | C++23 (required) |

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
| **CMake** | ≥ 3.28 | Build system (latest from Kitware) |
| **OpenSSL** | ≥ 3.5.4 | SHA3-256 hashing (built from source) |
| **liboqs** | ≥ 0.12.0 | Post-quantum cryptography (Dilithium3, Kyber768) |
| **RandomX** | ≥ 1.2.1 | ASIC-resistant Proof-of-Work |
| **Threads** | System | Multi-threading support |

### Optional Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **Boost** | ≥ 1.83.0 | Utilities, filesystem, threading |
| **RocksDB** | ≥ 10.7 | Blockchain database |
| **Qt6** | ≥ 6.8 | Desktop wallet GUI |
| **libzmq** | ≥ 4.3 | ZeroMQ messaging |
| **libevent** | ≥ 2.1 | Event-driven networking |
| **libtor** | Latest | Tor integration |
| **libi2pd** | Latest | I2P integration |

---

## macOS Build

**Supported Versions**:
- macOS 12 (Monterey) or later
- macOS 13 (Ventura)
- macOS 14 (Sonoma)
- macOS 15 (Sequoia)

**Note**: macOS 11 (Big Sur) and earlier are end-of-life and not supported.

### Install Dependencies

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake

# Install optional dependencies
brew install boost openssl rocksdb qt6 zeromq libevent
```

### Build liboqs 0.12.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.12.0

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
git clone https://github.com/INT-devs/intcoin.git intcoin
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

### Build liboqs 0.12.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.12.0

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
git clone https://github.com/INT-devs/intcoin.git intcoin
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

### Build liboqs 0.12.0

```bash
# Clone and checkout
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.12.0

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
git clone https://github.com/INT-devs/intcoin.git intcoin
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
git clone https://github.com/INT-devs/intcoin.git intcoin
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

### Building Windows .exe Executables

After building the project, create standalone Windows executables:

#### Build All Executables

```powershell
# From the build directory
cd build

# Build the daemon executable
cmake --build . --config Release --target intcoind

# Build the CLI tool
cmake --build . --config Release --target intcoin-cli

# Build the Qt wallet GUI
cmake --build . --config Release --target intcoin-qt

# Build the miner
cmake --build . --config Release --target intcoin-miner

# Build the pool server
cmake --build . --config Release --target intcoin-pool-server

# Build the wallet utility
cmake --build . --config Release --target wallet-tool
```

#### Locate Built Executables

The `.exe` files will be in:
```
build\bin\Release\
├── intcoind.exe              (~8.5 MB)  - Full node daemon
├── intcoin-cli.exe           (~850 KB)  - Command-line interface
├── intcoin-qt.exe            (~12 MB)   - Desktop wallet GUI
├── intcoin-miner.exe         (~8.2 MB)  - CPU miner
├── intcoin-pool-server.exe   (~9.0 MB)  - Mining pool server
└── wallet-tool.exe           (~1.2 MB)  - Wallet management utility
```

#### Create Distribution Package

```powershell
# Create distribution directory
mkdir ..\intcoin-windows-dist
cd ..\intcoin-windows-dist

# Copy executables
copy ..\build\bin\Release\*.exe .

# Copy required DLLs from vcpkg
copy C:\vcpkg\installed\x64-windows\bin\Qt6Core.dll .
copy C:\vcpkg\installed\x64-windows\bin\Qt6Gui.dll .
copy C:\vcpkg\installed\x64-windows\bin\Qt6Widgets.dll .
copy C:\vcpkg\installed\x64-windows\bin\libcrypto-3-x64.dll .
copy C:\vcpkg\installed\x64-windows\bin\libssl-3-x64.dll .
copy C:\liboqs\bin\oqs.dll .
copy C:\RandomX\bin\randomx.dll .

# Copy configuration template
copy ..\intcoin.conf.example intcoin.conf

# Copy documentation
mkdir docs
copy ..\README.md docs\
copy ..\LICENSE docs\
copy ..\docs\WALLET.md docs\
copy ..\docs\MINER_GUIDE.md docs\
```

#### Create Windows Installer (Optional)

Using [Inno Setup](https://jrsoftware.org/isinfo.php):

1. Install Inno Setup Compiler
2. Create installer script `intcoin-setup.iss`:

```ini
[Setup]
AppName=INTcoin Core
AppVersion=1.0.0-alpha
DefaultDirName={pf}\INTcoin
DefaultGroupName=INTcoin
OutputDir=.
OutputBaseFilename=intcoin-1.0.0-alpha-win64-setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
LicenseFile=LICENSE
InfoBeforeFile=README.md

[Files]
Source: "*.exe"; DestDir: "{app}\bin"
Source: "*.dll"; DestDir: "{app}\bin"
Source: "intcoin.conf"; DestDir: "{userappdata}\INTcoin"
Source: "docs\*"; DestDir: "{app}\docs"; Flags: recursesubdirs

[Icons]
Name: "{group}\INTcoin Wallet"; Filename: "{app}\bin\intcoin-qt.exe"
Name: "{group}\INTcoin Daemon"; Filename: "{app}\bin\intcoind.exe"
Name: "{group}\Documentation"; Filename: "{app}\docs"
Name: "{userdesktop}\INTcoin Wallet"; Filename: "{app}\bin\intcoin-qt.exe"

[Run]
Filename: "{app}\bin\intcoin-qt.exe"; Description: "Launch INTcoin Wallet"; Flags: postinstall nowait skipifsilent
```

3. Compile installer:
```powershell
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" intcoin-setup.iss
```

This creates `intcoin-1.0.0-alpha-win64-setup.exe` (~25 MB)

#### Testing Windows Executables

```powershell
# Test daemon
.\intcoind.exe --version
.\intcoind.exe -testnet -daemon

# Test CLI
.\intcoin-cli.exe -testnet getblockchaininfo

# Test Qt wallet (opens GUI)
.\intcoin-qt.exe -testnet

# Test miner
.\intcoin-miner.exe --address=intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh --threads=2

# Stop daemon
.\intcoin-cli.exe -testnet stop
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

## Automated Installation

**Status**: ✅ Available for Windows, Linux, and FreeBSD

INTcoin provides automated installation scripts that handle all dependency installation, building from source, and system configuration.

### Windows Build Script

**Script**: `scripts/build-windows.ps1`

**Supported Versions**:
- Windows 10 22H2 or later (64-bit)
- Windows 11 (64-bit)

**Note**: Windows 10 versions older than 22H2 and Windows 7/8/8.1 are end-of-life and not supported.

**Prerequisites**:
- Visual Studio 2022 with C++ workload
- Git for Windows
- CMake 3.28+

**Features**:
- Sets up vcpkg for dependency management
- Installs all required dependencies via vcpkg (RocksDB, OpenSSL, zlib, curl, nlohmann-json, sqlite3)
- Builds and installs liboqs 0.15.0 (post-quantum crypto)
- Builds and installs RandomX 1.2.1 (ASIC-resistant PoW)
- Builds all INTcoin executables
- Creates distributable package with all required DLLs
- Generates README for distribution

**Usage**:
```powershell
# Run in PowerShell (as Administrator recommended)
cd C:\path\to\intcoin
.\scripts\build-windows.ps1

# Installation process:
# 1. Checks prerequisites (Visual Studio, CMake, Git)
# 2. Sets up vcpkg and installs dependencies (30-60 minutes first run)
# 3. Builds liboqs 0.15.0 from source
# 4. Builds RandomX 1.2.1 from source
# 5. Configures and builds INTcoin
# 6. Creates distribution package in dist-windows/

# Custom build options:
.\scripts\build-windows.ps1 -BuildType Debug          # Debug build
.\scripts\build-windows.ps1 -SkipDependencies         # Skip dependency build
.\scripts\build-windows.ps1 -CleanBuild               # Clean build from scratch
.\scripts\build-windows.ps1 -VcpkgRoot "C:\vcpkg"     # Custom vcpkg location

# After build:
cd dist-windows
.\intcoind.exe --help      # Run INTcoin daemon
.\intcoin-qt.exe           # Run Qt wallet
```

**Distribution Package**:
The script creates a `dist-windows/` folder containing:
- All .exe executables (intcoind.exe, intcoin-cli.exe, etc.)
- Required DLLs (rocksdb.dll, OpenSSL, VC++ runtime)
- README.txt with quick start instructions

To create a distributable ZIP:
```powershell
Compress-Archive -Path 'dist-windows\*' -DestinationPath 'intcoin-windows-x64.zip'
```

### Linux Installation Script

**Script**: `scripts/install-linux.sh`

**Supported Distributions**:
- Ubuntu 24.04 LTS
- Debian 12 (Bookworm)
- Fedora 40+
- CentOS/RHEL 9+, Rocky Linux 9+, AlmaLinux 9+
- Arch Linux/Manjaro (rolling release)

**Note**: Ubuntu 22.04, Debian 11, and CentOS 8 are outdated and not supported.

**Features**:
- Installs latest CMake 3.28+ from Kitware repositories
- Builds and installs OpenSSL 3.5.4 from source
- Builds and installs liboqs 0.15.0 (post-quantum crypto)
- Builds and installs RandomX 1.2.1 (ASIC-resistant PoW)
- Builds INTcoin from source
- Creates systemd service for intcoind
- Sets up configuration files and directories

**Usage**:
```bash
cd /path/to/intcoin
sudo ./scripts/install-linux.sh

# Installation process:
# 1. Detects Linux distribution automatically
# 2. Installs build tools and dependencies
# 3. Downloads and builds OpenSSL 3.5.4
# 4. Downloads and builds liboqs 0.15.0
# 5. Downloads and builds RandomX 1.2.1
# 6. Builds INTcoin with all features
# 7. Runs test suite
# 8. Installs binaries to /usr/local/bin
# 9. Creates systemd service
# 10. Sets up config in /etc/intcoin/

# After installation:
sudo systemctl enable intcoind  # Enable service
sudo systemctl start intcoind   # Start daemon
sudo systemctl status intcoind  # Check status
```

### FreeBSD Installation Script

**Script**: `scripts/install-freebsd.sh`

**Supported Versions**:
- FreeBSD 13.x
- FreeBSD 14.x

**Note**: FreeBSD 12.x is end-of-life and no longer supported.

**Features**:
- Installs dependencies via pkg
- Builds OpenSSL 3.5.4 from source
- Builds liboqs 0.15.0 and RandomX 1.2.1
- Builds INTcoin from source
- Creates rc.d service for intcoind
- Sets up configuration and log directories

**Usage**:
```bash
cd /path/to/intcoin
sudo ./scripts/install-freebsd.sh

# Installation process similar to Linux script
# Uses FreeBSD-specific tools:
# - pkg instead of apt/dnf
# - gmake instead of make
# - fetch instead of wget
# - rc.d instead of systemd

# After installation:
sudo sysrc intcoind_enable=YES  # Enable service
sudo service intcoind start     # Start daemon
sudo service intcoind status    # Check status
```

### Script Configuration

Both scripts support environment variables for customization:

```bash
# Set custom install prefix (default: /usr/local)
export INSTALL_PREFIX=/opt/intcoin

# Set build parallelism (default: all CPU cores)
export BUILD_JOBS=4

# Set build type (default: Release)
export BUILD_TYPE=Release

# Run installation
sudo -E ./scripts/install-linux.sh
```

### What Gets Installed

After running the installation scripts:

**Binaries** (`/usr/local/bin/`):
- `intcoind` - Full node daemon (7.2 MB)
- `intcoin-cli` - Command-line interface (73 KB)
- `intcoin-qt` - Qt desktop wallet (180 KB)
- `intcoin-miner` - CPU miner (7.0 MB)
- `intcoin-pool-server` - Mining pool server (8.5 MB)
- `wallet-tool` - Wallet management utility

**Libraries** (`/usr/local/lib/`):
- `libintcoin_core.a` - Core library
- `liboqs.a` - Post-quantum crypto (0.15.0)
- `librandomx.a` - RandomX PoW (1.2.1)
- `libssl.so.3` - OpenSSL 3.5.4
- `libcrypto.so.3` - OpenSSL crypto

**Configuration** (Linux: `/etc/intcoin/`, FreeBSD: `/usr/local/etc/intcoin/`):
- `intcoin.conf` - Default configuration file

**Data Directories** (Linux: `/var/lib/intcoin`, FreeBSD: `/var/db/intcoin`):
- Blockchain data
- Wallet databases
- Transaction index

**Logs** (`/var/log/intcoin/`):
- `debug.log` - Daemon logs
- `error.log` - Error logs

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
