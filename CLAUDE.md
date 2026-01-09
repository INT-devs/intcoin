# INTcoin Claude Code Guide

**Project**: INTcoin - Quantum-Resistant Cryptocurrency with Smart Contracts
**Version**: v1.4.0-beta
**Last Updated**: January 9, 2026

This document provides guidance for AI assistants (Claude, etc.) working on the INTcoin codebase.

---

## Project Overview

INTcoin is a cryptocurrency implementing:
- **Post-quantum cryptography** (Dilithium3, Kyber768)
- **Smart contracts** (IntSC VM - EVM-compatible)
- **Lightning Network** integration
- **RandomX** proof-of-work
- **Qt desktop wallet**

**Current Status**: v1.4.0-beta released with smart contracts integration complete.

---

## Project Structure

```
/Users/neiladamson/Desktop/intcoin/
├── src/                    # C++ source files
│   ├── blockchain/         # Core blockchain (validation, blocks, transactions)
│   ├── consensus/          # Consensus rules
│   ├── crypto/             # PQC implementations (Dilithium, Kyber)
│   ├── network/            # P2P networking
│   ├── wallet/             # HD wallet implementation
│   ├── qt/                 # Qt GUI wallet
│   ├── rpc/                # JSON-RPC server
│   ├── mining/             # RandomX CPU miner
│   ├── lightning/          # Lightning Network
│   ├── contracts/          # Smart contracts (IntSC VM) ⭐ NEW v1.4.0
│   ├── ibd/                # Initial Block Download optimization ⭐ NEW v1.4.0
│   └── mempool/            # Transaction mempool (enhanced in v1.4.0)
├── include/intcoin/        # Public headers
├── tests/                  # Test suites (64 total)
├── docs/                   # Documentation
├── CMakeLists.txt          # Build configuration
└── README.md               # Project documentation
```

---

## Key Technologies

### Language & Standards
- **C++23** (ISO/IEC 14882:2023)
- Modern C++ features: `std::expected`, `std::mdspan`, ranges
- Strict compiler warnings: `-Wall -Wextra -Wpedantic -Werror`

### Dependencies
- **CMake** 4.2.0+
- **Boost** 1.89.0+
- **RocksDB** 10.7+ (persistent storage)
- **liboqs** 0.10.0+ (post-quantum crypto)
- **RandomX** 1.2.0+ (PoW algorithm)
- **Qt** 6.8+ (GUI wallet)
- **OpenSSL** 3.5.4+

### Platform Support
- **macOS**: 13 (Ventura)+
- **Linux**: Ubuntu 24.04+, Debian 12+, Fedora 40+, Arch
- **FreeBSD**: 13.0+
- **Windows**: 11+ (21H2 or later)

---

## Smart Contracts (v1.4.0)

### IntSC Virtual Machine
- **EVM-compatible**: 60+ standard opcodes
- **PQC opcodes**: 4 quantum-resistant operations
- **Gas metering**: EVM-compatible with PQC adjustments
- **Contract size limit**: 24 KB (24,576 bytes)
- **Block gas limit**: 30,000,000 gas
- **Mempool gas limit**: 60,000,000 gas

### Transaction Types
- **Type 0**: Standard UTXO transaction
- **Type 1**: Coinbase (mining reward)
- **Type 2**: Contract Deployment ⭐ NEW
- **Type 3**: Contract Call ⭐ NEW

### Contract Database
- **Backend**: RocksDB
- **Storage**: Per-contract key-value storage
- **Receipts**: Transaction execution results
- **Event Logs**: Indexed by block number
- **Performance**: 2.5M reads/sec, 242K writes/sec

### Key Files
- `src/contracts/vm.cpp` - IntSC VM implementation
- `src/contracts/database.cpp` - Contract state storage
- `src/contracts/validator.cpp` - Transaction validation
- `src/contracts/executor.cpp` - Contract execution engine
- `src/rpc/contracts_rpc.cpp` - 12 RPC methods
- `include/intcoin/contracts/` - Contract headers

---

## Development Guidelines

### Code Style
1. **Follow C++23 best practices**
   - Use RAII and smart pointers
   - Prefer `std::expected` for error handling
   - Use modern features where appropriate

2. **Memory Safety**
   - No raw pointers (use `std::unique_ptr`, `std::shared_ptr`)
   - Bounds checking on all arrays
   - RAII for resource management

3. **Cryptography**
   - Never implement custom crypto
   - Use liboqs for PQC operations
   - Zero memory after use (`OPENSSL_cleanse`)

4. **Error Handling**
   - Use `Result<T>` type (wraps `std::expected`)
   - Provide descriptive error messages
   - Never silently fail

### Testing Requirements
- **Test coverage**: ≥80%
- **All new features** must have tests
- **Current status**: 64 test suites, 100% passing
- Run tests before committing: `ctest`

### Smart Contract Testing
- **Integration tests**: `tests/test_contracts_integration.cpp` (7 tests)
- **Reorg tests**: `tests/test_contracts_reorg.cpp` (6 tests)
- **Benchmarks**: `tests/benchmark_contracts.cpp` (5 benchmarks)
- All tests pass at 100%

---

## Building the Project

### macOS/Linux
```bash
# Install dependencies
brew install cmake boost openssl@3 rocksdb liboqs randomx qt@6

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### FreeBSD
```bash
# Install dependencies
pkg install cmake boost-all openssl rocksdb qt6

# Build (same as above)
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows 11
```bash
# Use MSVC 2022 or MinGW-w64
# Dependencies via vcpkg or MSYS2
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

---

## Common Tasks

### Adding a New Test
1. Create test file in `tests/test_feature.cpp`
2. Add to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(test_feature test_feature.cpp)
   target_link_libraries(test_feature intcoin_core ${ROCKSDB_LIB})
   add_test(NAME FeatureTest COMMAND test_feature)
   ```
3. Install target in `tests/CMakeLists.txt`
4. Run `cmake --build build && ctest`

### Adding a New RPC Method
1. Add method to `src/rpc/contracts_rpc.cpp` (or relevant RPC file)
2. Register in `src/rpc/rpc_server.cpp`
3. Add documentation to `docs/RPC.md`
4. Write integration test

### Modifying Contract Validation
1. Update `src/contracts/validator.cpp`
2. Add validation test to `tests/test_contracts_integration.cpp`
3. Run all contract tests: `./tests/test_contracts_integration`
4. Check for performance impact with benchmarks

---

## Important Conventions

### Transaction Signing
- **All transactions** must be signed with Dilithium3
- Signature size: 3,309 bytes
- Use `DilithiumCrypto::GenerateKeyPair().GetValue()`
- Sign: `transaction.Sign(secret_key)`

### Contract Addresses
- Generated via: `SHA3-256(deployer_address || nonce)`
- Format: Bech32 with "int1" prefix
- Example: `int11udnsm9vhprweexy4uv924aw72s2nf026zkgv2j`

### Gas Limits
- **Deployment min**: 32,000 + 200 per byte
- **Call min**: 21,000 gas
- **Transaction max**: 30,000,000 gas
- **Block max**: 30,000,000 gas
- **Mempool max**: 60,000,000 gas

### Nonce Management
- Sequential per address
- Mempool rejects old nonces (replay prevention)
- Future nonces held until sequential

### Database Operations
- Use atomic batch operations:
  ```cpp
  db.BeginBatch();
  // ... multiple operations ...
  db.CommitBatch();  // On success
  db.DiscardBatch(); // On failure
  ```

---

## Performance Targets (v1.4.0)

### Smart Contracts
- **Deployment**: > 2,000/sec (measured: 2,188/sec) ✅
- **Calls**: > 2,000/sec (measured: 2,184/sec) ✅
- **Database reads**: > 2M/sec (measured: 2.5M/sec) ✅
- **Database writes**: > 200K/sec (measured: 242K/sec) ✅
- **Signature validation**: > 10K/sec (measured: 11,206/sec) ✅

### Block Validation
- **With contracts**: < 100ms per block ✅
- **IBD sync**: ~1000 blocks/sec (10x improvement) ✅

---

## Security Considerations

### Post-Quantum Cryptography
- All signatures use **Dilithium3**
- Key encapsulation uses **Kyber768**
- Hashing uses **SHA3-256**
- Future-proof against quantum computers

### Smart Contract Security
- **Gas limits** prevent DoS
- **Nonce validation** prevents replay
- **Bytecode size limits** (24 KB max)
- **RBF** requires 10% gas price increase
- **Isolated execution** in VM context

### Known Attack Vectors
1. **Gas griefing**: Mitigated by gas costs
2. **Storage bloat**: Mitigated by fees and size limits
3. **Nonce manipulation**: Mitigated by sequential validation
4. **Signature malleability**: Dilithium3 is non-malleable

---

## Documentation

### Key Documents
- `README.md` - Project overview and quick start
- `SECURITY.md` - Security policy and best practices
- `TECHNICAL_REQUIREMENTS.md` - Build and performance requirements
- `RELEASE_NOTES_v1.4.0-beta.md` - v1.4.0 changelog
- `docs/ARCHITECTURE.md` - System architecture
- `docs/SMART_CONTRACTS.md` - Smart contracts architecture
- `docs/CRYPTOGRAPHY.md` - PQC implementation details
- `docs/RPC.md` - RPC API reference

### API Documentation
- 12 contract RPC methods fully documented
- All public APIs have Doxygen comments
- Examples in `tests/contracts/` directory

---

## Common Issues & Solutions

### Build Errors

**Issue**: `fatal error: 'rocksdb/db.h' file not found`
**Solution**: Install RocksDB via `brew install rocksdb` or system package manager

**Issue**: `C++23 features not available`
**Solution**: Upgrade compiler (GCC 13+, Clang 16+, MSVC 2022 17.5+)

**Issue**: CMake can't find Qt6
**Solution**: Set `Qt6_DIR`: `export Qt6_DIR=/opt/homebrew/opt/qt@6/lib/cmake/Qt6`

### Test Failures

**Issue**: Contract tests fail with "Database not initialized"
**Solution**: Ensure test creates temp directory and calls `db.Open(path)`

**Issue**: Signature verification fails
**Solution**: Use `DilithiumCrypto::GenerateKeyPair().GetValue()` (note `.GetValue()`)

---

## Contact & Resources

- **Repository**: https://github.com/INT-devs/intcoin
- **Website**: https://international-coin.org
- **Documentation**: https://github.com/INT-devs/intcoin/tree/main/docs
- **Security**: security@international-coin.org
- **Discord**: https://discord.gg/jCy3eNgx
- **Twitter/X**: https://x.com/INTcoin_team

---

## Notes for AI Assistants

### When Adding Features
1. **Read existing code** first to understand patterns
2. **Follow C++23 conventions** used throughout codebase
3. **Write tests** for all new functionality
4. **Update documentation** (README, relevant docs)
5. **Check performance** impact with benchmarks

### When Fixing Bugs
1. **Write a regression test** that reproduces the bug
2. **Fix the bug** with minimal changes
3. **Verify all tests pass** including new regression test
4. **Update changelog** if user-facing bug

### When Refactoring
1. **Ensure all tests pass** before starting
2. **Make incremental changes** that can be tested
3. **Verify performance** hasn't degraded
4. **Update documentation** if APIs changed

### Smart Contract Development
- **Use `ContractDatabase` API** for all contract state
- **Atomic operations** via `BeginBatch()/CommitBatch()`
- **Gas tracking** mandatory for all VM operations
- **Event logs** should use indexed topics
- **Nonce increments** must be atomic

---

**Last Updated**: January 9, 2026
**Version**: v1.4.0-beta (Smart Contracts + IBD Optimization)
