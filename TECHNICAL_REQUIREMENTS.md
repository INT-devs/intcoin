# INTcoin Technical Requirements

**Last Updated**: December 9, 2025
**Current Status**: 97% Complete (Phases 1-10)

## Development Standards

### Language Standards
- **C++ Standard**: C++23 (ISO/IEC 14882:2023)
- **C Standard**: C23 (ISO/IEC 9899:2024) where applicable
- **Compiler Requirements**: Full C++23 support required
- **Extensions**: Disabled (strict ISO compliance)

### Dependency Management
- **Primary Package Manager**: Homebrew (macOS/Linux)
- All dependencies must be installable via `brew install`
- Document fallback installation methods for systems without Homebrew

### Platform Support

#### Supported Platforms
- **macOS**: 13 (Ventura) and later
- **Linux**: Ubuntu 22.04+, Debian 12+, Fedora 38+, Arch (rolling)
- **FreeBSD**: 13.0 and later
- **Unix-like**: Any system with modern C++23 compiler

#### Explicitly NOT Supported
- **Windows 10**: End of life (October 14, 2025)
- **Windows 8.1 and earlier**: End of life
- **macOS 12 and earlier**: End of support
- **Ubuntu 20.04 and earlier**: Outdated toolchain
- Any operating system no longer receiving security updates from original creator

#### Windows Support
- **Minimum**: Windows 11 (21H2 or later)
- **Compiler**: MSVC 2022 with C++23 support or MinGW-w64 with GCC 13+

## Completed Features (v1.0)

The following features have been completed for v1.0:

### ✅ Core Infrastructure (100%)
- **Blockchain Core**: Full validation, UTXO model, consensus rules
- **P2P Networking**: Peer discovery, block/transaction propagation
- **Storage**: RocksDB persistence with LRU cache and Bloom filters
- **Qt Desktop Wallet**: Full-featured GUI with transaction history
- **Block Explorer**: Real-time blockchain explorer backend
- **Lightning Network**: Foundation complete (BOLT-compatible)
- **Machine Learning**: Anomaly detection and fee estimation
- **RPC Server**: JSON-RPC 2.0 with HTTP Basic Auth
- **CPU Miner**: RandomX miner with pool support
- **Mining Pool**: Stratum protocol foundation
- **Testnet Faucet**: Automated testnet coin distribution

## Deferred Features

The following features are deferred to post-1.0 releases:

### Deferred to v2.0+
- **Mobile Wallet** (Android/iOS native apps - separate projects)
- **Web Wallet** (Browser-based wallet - separate project)
- **Lightning Network Full BOLT Compliance** (v1.0 has foundation)
- **Smart Contracts Layer** (Turing-complete VM)
- **Privacy Features** (Ring signatures, stealth addresses)

## Core Technology Stack

### Required Dependencies
```bash
# Homebrew installation (macOS/Linux)
brew install cmake
brew install boost
brew install openssl@3
brew install rocksdb
brew install liboqs          # Post-quantum cryptography
brew install randomx         # PoW algorithm
brew install zeromq
brew install libevent
brew install qt@6            # Desktop wallet
```

### Optional Dependencies
```bash
brew install tor             # Privacy (optional)
brew install i2pd            # Privacy (optional)
```

## Build Requirements

### Minimum Versions
- **CMake**: 4.2.0+
- **Boost**: 1.89.0+
- **OpenSSL**: 3.5.4+
- **Qt**: 6.8+
- **RocksDB**: 10.7+
- **liboqs**: 0.10.0+
- **RandomX**: 1.2.0+

### Compiler Support
- **GCC**: 13.0+ (full C++23 support)
- **Clang**: 16.0+ (full C++23 support)
- **MSVC**: 2022 17.5+ (full C++23 support)
- **Apple Clang**: 15.0+ (Xcode 15+)

## Code Quality Standards

### Compiler Warnings
```cmake
-Wall          # All warnings
-Wextra        # Extra warnings
-Wpedantic     # ISO C++ compliance
-Werror        # Warnings as errors
```

### Static Analysis
- Use `clang-tidy` with C++23 checks
- Use `cppcheck` for additional validation
- All code must pass static analysis before merge

### Code Style
- Follow ISO C++23 best practices
- Use modern C++23 features where appropriate:
  - `std::expected` for error handling
  - `std::mdspan` for multi-dimensional arrays
  - Deducing `this` for CRTP elimination
  - `std::print` for formatted output
  - Ranges and views

## Security Requirements

### Cryptography
- **Quantum-Resistant**: NIST PQC algorithms only
- **No deprecated algorithms**: No MD5, SHA-1, RSA < 3072 bits
- **Constant-time operations**: All crypto operations must be timing-safe

### Memory Safety
- **No use-after-free**: Strict ownership rules
- **No buffer overflows**: Bounds checking
- **No memory leaks**: RAII and smart pointers
- **Sanitizers**: ASan, UBSan, MSan in development builds

### Network Security
- **TLS 1.3+**: All encrypted connections
- **Certificate validation**: Strict certificate checks
- **DDoS protection**: Rate limiting, connection limits

## Testing Requirements

### Test Coverage
- **Unit tests**: ≥80% code coverage
- **Integration tests**: All major features
- **Regression tests**: All fixed bugs
- **Performance tests**: Benchmarks for critical paths

### Continuous Integration
- Build on all supported platforms
- Run all tests on every commit
- Static analysis on every PR
- Benchmarks on performance-critical changes

## Documentation Requirements

### Code Documentation
- **Public API**: Doxygen comments for all public interfaces
- **Complex algorithms**: Inline comments explaining logic
- **References**: Citations for cryptographic implementations

### User Documentation
- Installation guide for each platform
- Configuration reference
- API documentation
- Troubleshooting guide

## Release Requirements

### Version Numbering
- **Semantic Versioning**: MAJOR.MINOR.PATCH
- **Alpha/Beta**: Append -alpha/-beta for pre-releases
- **Release Candidates**: Append -rc.N

### Pre-Release Checklist
- [ ] All tests passing
- [ ] Documentation updated
- [ ] Changelog updated
- [ ] Performance benchmarks acceptable
- [ ] Builds on all supported platforms

## Performance Requirements

### Block Validation
- **Target**: < 100ms per block
- **Maximum**: < 500ms per block

### Transaction Validation
- **Target**: < 10ms per transaction
- **Maximum**: < 50ms per transaction

### Sync Performance
- **Initial sync**: < 24 hours for full blockchain
- **Bandwidth**: < 100 GB for full sync

### Memory Usage
- **Node**: < 4 GB RAM typical, < 8 GB maximum
- **Wallet**: < 1 GB RAM typical, < 2 GB maximum

## Backward Compatibility

### Network Protocol
- Protocol version included in all messages
- Graceful degradation for unknown message types
- Minimum supported protocol version documented

### Database Schema
- Migration scripts for all schema changes
- Backward-compatible reads where possible
- Clear upgrade path documented

---

**Note**: These requirements are subject to change as the project evolves. All changes must be documented and communicated to contributors.
