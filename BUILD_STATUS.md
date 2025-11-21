# INTcoin v1.2.0 Build Status

**Date**: November 21, 2025
**Version**: 1.2.0

---

## Build Summary

### ✅ Successful Platforms

#### macOS ARM64 (Apple Silicon)
- **OS**: macOS 15.1 (Darwin 25.1.0)
- **Compiler**: AppleClang 17.0.0
- **Architecture**: arm64
- **Build Type**: Release
- **C++ Standard**: C++23

**Built Executables**:
- `intcoind` (3.1 MB) - Full node daemon
- `intcoin-cli` (3.1 MB) - RPC command-line tool
- `intcoin-wallet` (2.9 MB) - Wallet CLI
- `intcoin-miner` (2.8 MB) - CPU miner
- `intcoin-explorer` (36 KB) - Block explorer

---

## Enabled Features

| Feature | Status |
|---------|--------|
| Core Blockchain | ✅ Enabled |
| Wallet | ✅ Enabled |
| Mining | ✅ Enabled |
| RPC Server | ✅ Enabled |
| Lightning Network | ✅ Enabled |
| Block Explorer | ✅ Enabled |
| Qt Wallet GUI | ⏸️  Not built (requires Qt6) |
| Smart Contracts | ⚠️ Disabled (SDK incomplete) |
| I2P Support | ⏸️ Pending (src/i2p not implemented) |
| ML Features | ⏸️ Pending (src/ml not implemented) |

---

## Smart Contract Status

**Implementation**: Partial (v1.2.0)

### ✅ Completed
- VM header with EVM opcodes + quantum extensions
- Bytecode specification (0x00-0xFF standard, 0x100-0x1FF quantum)
- Gas mechanism and full GasSchedule
- VM execution engine (vm.cpp)
- Contract deployment framework
- ABI encoding structure

### ⚠️  Incomplete
- SDK implementation (StateInterface methods)
- MockState full implementation
- Testing framework
- Contract templates
- CLI tools

**Note**: Smart contracts can be enabled with `cmake -DENABLE_SMART_CONTRACTS=ON` but SDK requires additional development to compile.

---

## Platform Support

### Tested
- ✅ macOS ARM64 (Apple Silicon) - Build successful

### Planned
- 🔄 Linux x86_64 (Ubuntu/Debian/Fedora)
- 🔄 Linux ARM64
- 🔄 Linux RISC-V
- 🔄 FreeBSD x86_64
- 🔄 Windows x64 (.exe)

---

## Build Instructions

### macOS (Current Platform)
```bash
mkdir -p build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Linux
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### FreeBSD
```bash
mkdir -p build && cd build
cmake ..
gmake -j$(sysctl -n hw.ncpu)
```

### Windows (Cross-compile or native)
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

---

## Dependencies

| Package | Version | Status |
|---------|---------|--------|
| CMake | 4.0+ | ✅ |
| C++ Compiler | C++23 | ✅ |
| OpenSSL | 3.0+ | ✅ |
| Boost | 1.70+ | ✅ |
| RocksDB | Latest | ✅ |
| liboqs | Latest | ✅ |
| Qt6 | 6.2+ | ⏸️ (for GUI) |

---

## Recent Changes (v1.2.0)

1. **Smart Contract Infrastructure** (Nov 21)
   - Added vm.h and vm.cpp with EVM compatibility
   - Quantum extension opcodes implemented
   - SDK framework created (incomplete)

2. **Build System Fixes** (Nov 21)
   - Fixed CMakeLists.txt to handle missing directories
   - Removed duplicate test targets
   - Added Address hash function for unordered_map support

3. **Documentation** (Nov 21)
   - Created wiki/Smart-Contracts.md
   - Updated ROADMAP.md Q2 2026 section
   - Updated wiki/home.md with smart contract status

---

## Known Issues

1. **Smart Contract SDK**: Incomplete StateInterface implementation
2. **Test Suite**: Some tests failing due to API changes
3. **I2P Module**: Not yet implemented (src/i2p)
4. **ML Module**: Not yet implemented (src/ml)

---

## Next Steps

1. Complete SDK StateInterface methods
2. Implement contract testing framework
3. Build for Linux platforms
4. Build for FreeBSD
5. Cross-compile Windows .exe
6. Complete I2P integration
7. Complete ML features

---

**Maintainer**: Maddison Lane
**Repository**: https://gitlab.com/intcoin/crypto
**License**: MIT
