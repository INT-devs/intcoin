# INTcoin Build Status & Documentation

**Date**: November 26, 2025
**Version**: 1.0.0-alpha
**Status**: ✅ Clean Build - Core Foundation Complete
**CMake**: 4.2.0+
**C++ Standard**: C++20
**Architecture**: 64-bit only

---

## Table of Contents

1. [Current Status](#current-status)
2. [Build System](#build-system)
3. [Platform Builds](#platform-builds)
4. [Dependencies](#dependencies)
5. [Build Options](#build-options)
6. [Troubleshooting](#troubleshooting)
7. [Development Roadmap](#development-roadmap)

---

## Current Status

### ✅ Completed Work

#### 1. Project Structure
- ✅ Complete directory structure created
- ✅ CMake build system configured (CMake 3.25+, 64-bit only)
- ✅ Ports updated to 2210-2220 range (non-conflicting)
- ✅ .gitignore updated for wiki and symlinks
- ✅ File organization (header.py renamed, symlinks removed)
- ✅ Wiki structure created (Users/ and Developers/ sections)

#### 2. Header Files (include/intcoin/)
All header files created with comprehensive API definitions:

- ✅ **types.h** - Core type definitions (uint256, PublicKey, SecretKey, Signature, Result<T>)
- ✅ **util.h** - Utility functions (string, encoding, file I/O, serialization, logging)
- ✅ **crypto.h** - Quantum-resistant cryptography (Dilithium3, Kyber768, SHA3-256, Bech32)
- ✅ **script.h** - Script system (opcodes, P2PKH, P2PK, OP_RETURN)
- ✅ **transaction.h** - Transactions (TxIn, TxOut, OutPoint, Transaction, TransactionBuilder)
- ✅ **block.h** - Blocks (BlockHeader, Block, Merkle trees, genesis block)
- ✅ **blockchain.h** - Blockchain state (Blockchain class, validators, iterators)
- ✅ **consensus.h** - Consensus rules (RandomX, Digishield V3, block rewards, validation)
- ✅ **network.h** - P2P networking (messages, peers, discovery)
- ✅ **storage.h** - Database layer (RocksDB, UTXO set, mempool, wallet DB)
- ✅ **intcoin.h** - Master header (includes all components + version info)

#### 3. Stub Implementation Files (src/)
Stub implementations created with TODO markers:

- ✅ src/core/intcoin.cpp
- ✅ src/util/types.cpp
- ✅ src/util/util.cpp
- ✅ src/blockchain/block.cpp
- ✅ src/blockchain/transaction.cpp
- ✅ src/blockchain/script.cpp
- ✅ src/crypto/crypto.cpp
- ✅ src/consensus/consensus.cpp

#### 4. Build Configuration
- ✅ CMakeLists.txt updated to CMake 4.2.0+
- ✅ 64-bit system requirement enforced
- ✅ Optional dependency handling (Boost 1.89.0+, OpenSSL 3.5.4+, RocksDB 10.7+, liboqs 0.15.0+, RandomX 1.2.1+)
- ✅ Source files list updated to match actual files
- ✅ Executables (daemon, CLI, miner) commented out until implemented
- ✅ All conditional includes fixed for NOTFOUND dependencies

#### 5. Compilation Fixes (All Resolved ✅)

**All compilation errors have been fixed!**

1. **Header Includes** ✅
   - Added `<functional>` to blockchain.h for std::function
   - Added `<iostream>` to util.cpp for std::cout
   - Added `<algorithm>` to transaction.cpp and block.cpp for std::all_of
   - Added `#include "intcoin/consensus.h"` to block.cpp

2. **Type Alignment** ✅
   - Aligned uint256 implementation with type alias definition (std::array<uint8_t, 32>)
   - Removed class-style operators (std::array already provides them)
   - Fixed all .data member accesses to use iterators
   - Replaced method calls (ToString, IsZero, Serialize) with appropriate alternatives

3. **ScriptType Enum** ✅
   - Added ScriptType enum to script.h with UNKNOWN, P2PKH, P2PK, OP_RETURN, MULTISIG

4. **Function Conflicts** ✅
   - Removed duplicate IntsToInt/IntToInts from util.cpp (already inline in types.h)
   - Fixed Result<T> API usage (value field access, Ok/Error methods)

#### 6. Build Artifacts

**Current Build**: `libintcoin_core.a` (129 KB)
**Build Time**: ~15 seconds
**All Source Files**: Compiling cleanly ✅

```bash
# Build verified with:
cd build && rm -rf * && cmake .. && make
# Result: [100%] Built target intcoin_core
```

#### 7. Cryptography Implementation ✅ **COMPLETED**

**SHA3-256 Hashing**:
- ✅ Fully implemented using OpenSSL 3.5.4+ EVP interface
- ✅ Test vectors verified (empty string, "abc", custom messages)
- ✅ All tests passing

**Dilithium3 (ML-DSA-65) Digital Signatures**:
- ✅ Key generation (1952-byte public key, 4032-byte secret key)
- ✅ Message signing (3309-byte signatures)
- ✅ Signature verification
- ✅ Invalid signature detection
- ✅ All tests passing (5/5)

**Kyber768 (ML-KEM-768) Key Exchange**:
- ✅ Key generation (1184-byte public key, 2400-byte secret key)
- ✅ Encapsulation (32-byte shared secret + 1088-byte ciphertext)
- ✅ Decapsulation (shared secret recovery)
- ✅ Shared secret matching verified
- ✅ All tests passing (5/5)

**Dependencies Installed**:
- ✅ liboqs 0.15.0 (Open Quantum Safe library)
- ✅ OpenSSL 3.6.0 (SHA3-256 support)

#### 8. RandomX Proof-of-Work ✅ **COMPLETED**

**RandomX Integration**:
- ✅ RandomX 1.2.1 library installed (~/.local/)
- ✅ CMakeLists.txt updated with RandomX linking
- ✅ Compilation with RandomX successful (library: 129KB)

**RandomX Implementation**:
- ✅ `RandomXValidator::Initialize()` - Initialize RandomX cache and VM
- ✅ `RandomXValidator::Shutdown()` - Cleanup resources
- ✅ `RandomXValidator::CalculateHash()` - Calculate RandomX hash for block headers
- ✅ `RandomXValidator::ValidateBlockHash()` - Validate PoW hash meets difficulty
- ✅ `RandomXValidator::GetRandomXKey()` - Generate epoch-based RandomX key
- ✅ `RandomXValidator::UpdateDataset()` - Update cache for new epochs
- ✅ `RandomXValidator::NeedsDatasetUpdate()` - Check if epoch changed

**Epoch Management**:
- ✅ 2048-block epochs (~2.8 days at 2-minute blocks)
- ✅ Epoch-based key generation: `SHA3-256("INTcoin-RandomX-Epoch-{N}")`
- ✅ Automatic cache updates when crossing epoch boundaries
- ✅ Keys stored in block headers for validation

**Hardware Optimizations**:
- ✅ JIT compilation enabled on x86_64 platforms
- ✅ Hardware AES support on x86_64 and ARM64
- ✅ Thread-safe implementation with mutex protection

**Test Results** (tests/test_randomx):
- ✅ Test 1: RandomX initialization (2/2 passed)
- ✅ Test 2: Key generation (4/4 passed)
- ✅ Test 3: Hash calculation (3/3 passed)
- ✅ Test 4: Dataset updates (3/3 passed)
- ✅ Test 5: Block validation (1/1 passed)
- ✅ Test 6: Shutdown/cleanup (3/3 passed)

**Total**: 6/6 test suites passing ✅

#### 9. Bech32 Address Encoding ✅ **COMPLETED**

**Bech32 Implementation**:
- ✅ Complete Bech32 encoding algorithm with 'int1' HRP (Human-Readable Part)
- ✅ Bech32 decoding with checksum validation
- ✅ Address validation functions
- ✅ 8-bit to 5-bit conversion for base32 encoding
- ✅ BCH checksum calculation using polymod algorithm
- ✅ Case-insensitive address support (uppercase/lowercase)

**Implementation Details**:
- ✅ `AddressEncoder::EncodeAddress()` - Encode uint256 pubkey hash to Bech32 address
- ✅ `AddressEncoder::DecodeAddress()` - Decode Bech32 address back to pubkey hash
- ✅ `AddressEncoder::ValidateAddress()` - Validate address format and checksum
- ✅ `PublicKeyToHash()` - SHA3-256 hash of public key
- ✅ `PublicKeyHashToAddress()` - Convert pubkey hash to Bech32 address
- ✅ `PublicKeyToAddress()` - Convenience function for direct conversion

**Address Format**:
- ✅ Prefix: `int1` (Human-Readable Part)
- ✅ Separator: `1`
- ✅ Version byte: `0` (mainnet P2PKH)
- ✅ Payload: 32-byte pubkey hash (SHA3-256)
- ✅ Checksum: 6 characters (BCH code)
- ✅ Example: `int11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqegtg5n`

**Error Detection**:
- ✅ Single-character error detection (59/59 detected in test)
- ✅ Invalid character rejection (no 0, 1, b, i, o)
- ✅ Checksum verification prevents typos
- ✅ Mixed case detection (must be all uppercase or all lowercase)
- ✅ Invalid HRP rejection
- ✅ Invalid version byte rejection

**Test Results** (tests/test_bech32):
- ✅ Test 1: Encode/decode round-trip (3/3 passed)
- ✅ Test 2: Different hashes produce different addresses (2/2 passed)
- ✅ Test 3: Address validation (5/5 passed)
- ✅ Test 4: Case insensitivity (2/2 passed)
- ✅ Test 5: Checksum error detection (1/1 passed)
- ✅ Test 6: Public key to address conversion (5/5 passed)
- ✅ Test 7: Edge cases (4/4 passed)
- ✅ Test 8: Invalid input handling (1/1 passed)

**Total**: 8/8 test suites passing ✅

### 📊 Overall Progress

- **Headers/API Design**: 100% ✅
- **Stub Implementations**: 100% ✅
- **Build System**: 100% ✅
- **File Organization**: 100% ✅
- **Wiki Structure**: 100% ✅
- **Cryptography**: 100% ✅
- **RandomX PoW**: 100% ✅
- **Bech32 Addresses**: 100% ✅ **NEW**
- **Core Implementation**: 15% 🟡
- **Testing**: 25% 🟡 (Crypto + RandomX + Bech32 complete)
- **Documentation**: 50% 🟡

**Total Project Completion**: ~30%

### 🎯 Current Status Summary

| Component | Status | Notes |
|-----------|--------|-------|
| **Build System** | ✅ Working | Clean compilation, 129KB library |
| **Type System** | ✅ Complete | All type aliases properly implemented |
| **Cryptography** | ✅ Complete | SHA3-256, Dilithium3, Kyber768 fully working |
| **RandomX PoW** | ✅ Complete | Full implementation with epoch management |
| **Bech32 Addresses** | ✅ Complete | Full encode/decode with 'int1' prefix ⭐ NEW |
| **Blockchain Core** | ✅ Defined | Needs full implementation |
| **Network Layer** | ✅ Defined | Needs implementation |
| **Storage Layer** | ✅ Defined | Needs RocksDB integration |
| **Tests** | 🟡 In Progress | Crypto (5/5) + RandomX (6/6) + Bech32 (8/8) ⭐ NEW |
| **Documentation** | 🟡 In Progress | Updated with crypto + RandomX + Bech32 |

---

## Build System

### System Requirements

#### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | macOS 10.15+, Linux (kernel 4.x+), FreeBSD 12+, Windows 10+ |
| **Architecture** | 64-bit (x86_64 or ARM64) |
| **CMake** | 4.2.0 or higher |
| **Compiler** | GCC 15.2+, Clang 17+, MSVC 2022+, Apple Clang 17+ |
| **RAM** | 4 GB minimum, 8 GB recommended |
| **Disk** | 2 GB for build, 50+ GB for full node |

#### Network Ports

INTcoin uses the **2210-2220** port range to avoid conflicts:

| Network | P2P Port | RPC Port | Purpose |
|---------|----------|----------|---------|
| **Mainnet** | 2210 | 2211 | Production network |
| **Testnet** | 2212 | 2213 | Testing network |
| **Regtest** | 2214 | 2215 | Regression testing |

### Build Configuration

#### Basic Build Process

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Build
cmake --build . -j$(nproc)

# 4. Install (optional)
sudo cmake --install .
```

#### Build Types

```bash
# Debug (with debug symbols, no optimization)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release (optimized, no debug symbols) - RECOMMENDED
cmake -DCMAKE_BUILD_TYPE=Release ..

# RelWithDebInfo (optimized + debug symbols)
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# MinSizeRel (optimized for size)
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
```

#### Compiler Flags

Auto-configured for GCC/Clang:
- `-Wall -Wextra -Wpedantic`: Enable warnings
- `-Werror`: Treat warnings as errors
- `-Wno-unused-parameter`: Suppress unused parameter warnings
- `-march=native`: Optimize for local CPU
- `-O3`: Maximum optimization (Release)

---

## Platform Builds

### macOS

#### Install Dependencies

```bash
# Install Homebrew (if needed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install base dependencies
brew install cmake boost openssl@3 rocksdb qt@6 zeromq libevent

# Install liboqs (Post-Quantum Cryptography)
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install

# Install RandomX (ASIC-resistant PoW)
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

#### Build INTcoin

```bash
cd /path/to/intcoin
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/opt/homebrew \
      -DCMAKE_INSTALL_PREFIX=/usr/local ..

make -j$(sysctl -n hw.ncpu)
sudo make install
```

#### Create macOS .app Bundle

```bash
# Build Qt wallet
cmake -DBUILD_WALLET_QT=ON ..
make -j$(sysctl -n hw.ncpu)

# Create .app bundle
macdeployqt intcoin-qt.app

# Create DMG installer
hdiutil create -volname "INTcoin" -srcfolder intcoin-qt.app \
               -ov -format UDZO INTcoin-1.0.0-macOS.dmg
```

---

### Linux (Ubuntu/Debian)

#### Install Dependencies

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake git

# Install core dependencies
sudo apt install -y \
    libboost-all-dev \
    libssl-dev \
    librocksdb-dev \
    qt6-base-dev \
    libzmq3-dev \
    libevent-dev \
    pkg-config

# Install liboqs 0.15.0
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig

# Install RandomX 1.2.1
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

#### Build INTcoin

```bash
cd /path/to/intcoin
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local ..

make -j$(nproc)
sudo make install
```

#### Create .deb Package

```bash
# Configure with packaging
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Create Debian package
cpack -G DEB

# Install package
sudo dpkg -i INTcoin-1.0.0-Linux.deb
```

---

### FreeBSD

#### Install Dependencies

```bash
# Install from ports
sudo pkg install cmake boost-all openssl rocksdb qt6 \
                 zeromq libevent pkgconf

# Build liboqs 0.15.0
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install

# Build RandomX 1.2.1
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

#### Build INTcoin

```bash
cd /path/to/intcoin
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local ..

gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

---

### Windows

#### Install Dependencies (vcpkg)

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install boost:x64-windows openssl:x64-windows ^
                rocksdb:x64-windows qt6:x64-windows ^
                zeromq:x64-windows libevent:x64-windows

# Build liboqs manually
# Download from https://github.com/open-quantum-safe/liboqs/releases

# Build RandomX manually
# Download from https://github.com/tevador/RandomX/releases
```

#### Build INTcoin

```powershell
# Open "x64 Native Tools Command Prompt for VS 2022"
cd C:\path\to\intcoin
mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake ^
      -DCMAKE_BUILD_TYPE=Release ..

cmake --build . --config Release -j

# Install
cmake --install . --config Release
```

#### Create Windows .exe Installer

```powershell
# Build with NSIS
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cpack -G NSIS

# Output: INTcoin-1.0.0-win64.exe
```

---

## Dependencies

### Required Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **CMake** | 3.25+ | Build system |
| **C++ Compiler** | See requirements | Compilation |
| **Threads** | System | Multi-threading |

### Core Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **Boost** | 1.89.0+ | Utilities, filesystem, threading |
| **OpenSSL** | 3.5.4+ | SSL/TLS, cryptography |
| **RocksDB** | 10.7+ | Blockchain database |
| **liboqs** | 0.15.0+ | Post-quantum crypto (Dilithium, Kyber) |
| **RandomX** | 1.2.1+ | ASIC-resistant PoW |

### Network Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **ZeroMQ** | 4.3+ | RPC message queue |
| **libevent** | 2.1+ | Event-driven networking |

### Optional Dependencies

| Library | Version | Purpose | Flag |
|---------|---------|---------|------|
| **Qt6** | 6.8+ | Desktop wallet GUI | `BUILD_WALLET_QT=ON` |
| **libtor** | Latest | Tor integration | `ENABLE_TOR=ON` |
| **libi2pd** | Latest | I2P integration | `ENABLE_I2P=ON` |
| **CUDA/OpenCL** | - | GPU mining | `ENABLE_GPU_MINING=ON` |

---

## Build Options

### Configuration Options

Set via `-D` flag: `cmake -D<OPTION>=<VALUE> ..`

#### Component Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build test suite |
| `BUILD_DAEMON` | `ON` | Build intcoind daemon (TODO) |
| `BUILD_CLI` | `ON` | Build intcoin-cli (TODO) |
| `BUILD_MINER` | `ON` | Build intcoin-miner (TODO) |
| `BUILD_WALLET_QT` | `ON` | Build Qt wallet (TODO) |
| `BUILD_EXPLORER` | `ON` | Build block explorer (TODO) |

#### Feature Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_LIGHTNING` | `ON` | Lightning Network support |
| `ENABLE_TOR` | `OFF` | Tor integration |
| `ENABLE_I2P` | `OFF` | I2P integration |
| `ENABLE_GPU_MINING` | `OFF` | GPU mining support |

### Example Builds

#### Minimal Build (daemon only)

```bash
cmake -DBUILD_TESTS=OFF \
      -DBUILD_WALLET_QT=OFF \
      -DBUILD_MINER=OFF \
      -DBUILD_EXPLORER=OFF \
      -DENABLE_LIGHTNING=OFF ..
```

#### Full Build (all features)

```bash
cmake -DBUILD_TESTS=ON \
      -DBUILD_WALLET_QT=ON \
      -DBUILD_MINER=ON \
      -DBUILD_EXPLORER=ON \
      -DENABLE_LIGHTNING=ON \
      -DENABLE_TOR=ON \
      -DENABLE_I2P=ON \
      -DENABLE_GPU_MINING=ON ..
```

---

## Troubleshooting

### Common Issues

#### CMake Version Too Old

```
CMake Error: CMake 3.25 or higher is required
```

**Solution**:
```bash
# macOS
brew install cmake

# Ubuntu
sudo apt install cmake

# Or download from https://cmake.org/download/
```

#### Not a 64-bit System

```
CMake Error: INTcoin requires a 64-bit system
```

**Solution**: Use a 64-bit OS and compiler.

#### Boost Not Found

```
Could NOT find Boost (missing: system filesystem thread program_options)
```

**Solution**:
```bash
# Install Boost
brew install boost  # macOS
sudo apt install libboost-all-dev  # Linux

# Or specify path
cmake -DBOOST_ROOT=/path/to/boost ..
```

#### liboqs Not Found

```
Could NOT find OQS library
```

**Solution**: Build and install liboqs, or specify path:
```bash
cmake -DOQS_INCLUDE_DIR=/path/to/oqs/include \
      -DOQS_LIB=/path/to/oqs/lib/liboqs.a ..
```

#### RandomX Not Found

```
Could NOT find RandomX library
```

**Solution**: Build and install RandomX, or specify path:
```bash
cmake -DRANDOMX_INCLUDE_DIR=/path/to/randomx/include \
      -DRANDOMX_LIB=/path/to/randomx/lib/librandomx.a ..
```

#### Compilation Errors

**Solution**:
1. Clean rebuild:
   ```bash
   rm -rf build && mkdir build && cd build && cmake ..
   ```

2. Check compiler version:
   ```bash
   gcc --version    # Should be 11+
   clang --version  # Should be 14+
   ```

3. Disable `-Werror`:
   ```bash
   cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra" ..
   ```

### Performance Tips

1. **Use Ninja** (faster than Make):
   ```bash
   cmake -G Ninja ..
   ninja
   ```

2. **Enable ccache** (speeds up recompilation):
   ```bash
   cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache ..
   ```

3. **Parallel builds** (use all CPU cores):
   ```bash
   make -j$(nproc)  # Linux
   make -j$(sysctl -n hw.ncpu)  # macOS
   ```

4. **LTO** (Link-Time Optimization):
   ```bash
   cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
   ```

---

## Development Roadmap

### Phase 1: Fix Compilation (IMMEDIATE)
1. Fix header includes
2. Resolve type conflicts
3. Remove duplicate function definitions
4. Verify clean compilation

### Phase 2: Core Implementation (Week 1-2)

**Cryptography** (crypto.cpp):
- Integrate liboqs for Dilithium3 and Kyber768
- Implement SHA3-256 hashing
- Implement Bech32 address encoding

**Script System** (script.cpp):
- Implement script interpreter
- Add P2PKH and P2PK script types
- Add script validation

**Transactions** (transaction.cpp):
- Implement serialization/deserialization
- Add signature creation and verification
- Implement transaction validation

### Phase 3: Blockchain Core (Week 3-4)

**Blocks** (block.cpp):
- Complete Merkle tree implementation
- Add block validation
- Implement genesis block

**Consensus** (consensus.cpp):
- Integrate RandomX library
- Implement Digishield V3 difficulty adjustment
- Add PoW validation

**Storage** (storage.cpp - NEW):
- RocksDB integration
- UTXO set implementation
- Blockchain database

### Phase 4: Networking & Applications (Week 5-6)

**Network** (network.cpp - NEW):
- P2P protocol implementation
- Peer discovery
- Message handling

**Blockchain Management** (blockchain.cpp - NEW):
- Chain reorganization
- Block/transaction propagation
- Mempool management

**Executables**:
- intcoind (daemon)
- intcoin-cli (CLI tool)
- intcoin-miner (mining software)

### Phase 5: Testing & Documentation

1. Create unit tests (C++ and Python)
2. Create functional tests (Python)
3. Create fuzz tests
4. Write comprehensive wiki documentation
5. Build for all platforms
6. Create installers (.deb, .app, .exe)

---

## Key Requirements from DEVELOPMENT-PLAN.md

### Build System
- [x] CMake 3.25+
- [x] 64-bit only support
- [x] Ports in 2210-2220 range

### Code Organization
- [x] Rename add_copyright.py to header.py
- [x] Remove symlinks (desktop-wallet, mobile-wallet, web-wallet, website)
- [ ] Move test-copyright-update.md and copyright-script-readme.md to docs/
- [x] Create wiki/ folder (added to .gitignore)
- [x] Organize wiki into Users and Developers sections

### Documentation
- [ ] Update all documentation based on current progress
- [ ] Create comprehensive user guides in wiki
- [ ] Create developer guides in wiki
- [ ] Document all APIs

### Testing
- [ ] Write unit tests (C++ and Python)
- [ ] Write functional tests (Python)
- [ ] Write fuzz tests
- [ ] Achieve >80% code coverage

---

## Notes

- All header files follow consistent naming and documentation standards
- Code uses C++20 features throughout
- All functions have TODO markers where implementation is needed
- Quantum-resistant cryptography APIs are well-defined and ready for implementation
- Network protocol uses unique ports (2210-2220) to avoid conflicts with other cryptocurrencies
- Result<T> type provides clean error handling pattern
- Build system gracefully handles optional dependencies
- Cross-platform support for macOS, Linux, FreeBSD, and Windows

---

## Resources

- **CMake Documentation**: https://cmake.org/documentation/
- **C++20 Reference**: https://en.cppreference.com/w/cpp/20
- **liboqs (PQC)**: https://github.com/open-quantum-safe/liboqs
- **RandomX**: https://github.com/tevador/RandomX
- **Boost**: https://www.boost.org/
- **RocksDB**: https://rocksdb.org/

---

**Last Updated**: November 25, 2025 17:30 UTC
**Build Status**: ✅ All compilation errors resolved
**Next Steps**: Core implementation (cryptography, consensus, storage)
**Maintained by**: INTcoin Team (Neil Adamson)
**License**: MIT
