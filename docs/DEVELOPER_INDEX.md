# INTcoin Developer Documentation Index
**Version**: 1.0.0-beta  
**Last Updated**: December 26, 2025

---

## Quick Links

- 🚀 [Quick Start](#quick-start)
- 📚 [Core Documentation](#core-documentation)
- 🛠️ [Development Guides](#development-guides)
- 🧪 [Testing](#testing)
- 🔍 [API References](#api-references)

---

## Quick Start

**New Developers**:
1. Read [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
2. Follow [BUILD_GUIDE.md](BUILD_GUIDE.md) - Build the project
3. Review [CODE_STYLE.md](CODE_STYLE.md) - Coding standards
4. Study [ARCHITECTURE.md](ARCHITECTURE.md) - System design

**Experienced Developers**:
- [API_REFERENCE.md](API_REFERENCE.md) - Internal API documentation
- [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) - Comprehensive development guide
- [TEST_ENHANCEMENT_PLAN.md](TEST_ENHANCEMENT_PLAN.md) - Testing strategy

---

## Core Documentation

### Essential Reading

| Document | Description | Audience |
|----------|-------------|----------|
| [README.md](../README.md) | Project overview and features | Everyone |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | How to contribute to INTcoin | Contributors |
| [LICENSE](../LICENSE) | MIT License terms | Everyone |
| [RELEASE_NOTES.md](../RELEASE_NOTES.md) | v1.0.0-beta release details | Everyone |

### Technical Documentation

| Document | Description | Audience |
|----------|-------------|----------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System architecture and design | Developers |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Comprehensive development guide | Developers |
| [BUILD_GUIDE.md](BUILD_GUIDE.md) | Platform-specific build instructions | Developers |
| [CODE_STYLE.md](CODE_STYLE.md) | Coding standards and conventions | Contributors |
| [API_REFERENCE.md](API_REFERENCE.md) | Internal API documentation | Developers |

---

## Development Guides

### Getting Started

1. **[BUILD_GUIDE.md](BUILD_GUIDE.md)** - Complete build instructions
   - System requirements
   - Dependency installation
   - Platform-specific guides (Ubuntu, macOS, FreeBSD, Windows)
   - Build options and configurations
   - Troubleshooting

2. **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - Development workflow
   - Development environment setup
   - Code organization
   - Best practices
   - Debugging techniques
   - Performance profiling

3. **[CODE_STYLE.md](CODE_STYLE.md)** - Coding standards
   - C++23 style guide
   - Naming conventions
   - Code formatting
   - Documentation requirements

### Architecture & Design

4. **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture
   - Component overview
   - Data flow diagrams
   - Module dependencies
   - Design patterns
   - Post-quantum cryptography implementation

5. **[SECURITY.md](../SECURITY.md)** - Security considerations
   - Vulnerability reporting
   - Security best practices
   - Cryptographic implementations

### Specialized Topics

6. **[LIGHTNING.md](LIGHTNING.md)** - Lightning Network
   - BOLT specification implementation
   - Channel management
   - HTLC operations
   - Routing and payments
   - Watchtower integration

7. **[POOL_SETUP.md](POOL_SETUP.md)** - Mining pool
   - Pool server configuration
   - Stratum protocol
   - VarDiff algorithm
   - Payout systems

8. **[RPC.md](RPC.md)** - RPC API
   - 47+ RPC methods
   - Method categories
   - Usage examples
   - Error codes

9. **[CI-CD-PIPELINE.md](CI-CD-PIPELINE.md)** - CI/CD
   - GitHub Actions workflows
   - Build matrix
   - Testing automation
   - Release process

---

## Testing

### Test Documentation

| Document | Description |
|----------|-------------|
| [TEST_ENHANCEMENT_PLAN.md](TEST_ENHANCEMENT_PLAN.md) | Testing roadmap and strategy |
| Test suites in [../tests/](../tests/) | 13 test suites (100% passing) |

### Test Coverage

**Current Status**: 13/13 tests passing (100%)

| Test Suite | Coverage Area | Status |
|------------|---------------|--------|
| test_crypto | Post-quantum cryptography | ✅ PASS |
| test_randomx | Mining algorithm | ✅ PASS |
| test_bech32 | Address encoding | ✅ PASS |
| test_serialization | Data serialization | ✅ PASS |
| test_storage | Database operations | ✅ PASS |
| test_validation | Transaction validation | ✅ PASS (FIXED) |
| test_genesis | Genesis block | ✅ PASS |
| test_network | P2P protocol | ✅ PASS |
| test_ml | Machine learning | ✅ PASS |
| test_wallet | HD wallet | ✅ PASS |
| test_fuzz | Robustness | ✅ PASS |
| test_integration | End-to-end | ✅ PASS |
| test_lightning | Lightning Network | ✅ PASS (10/10) |

### Running Tests

```bash
# All tests
cd build && ctest --output-on-failure

# Specific test
./build/tests/test_lightning

# With coverage
cmake -B build -DENABLE_COVERAGE=ON
cmake --build build
cd build && ctest
# Generate coverage report
```

---

## API References

### Internal APIs

1. **[API_REFERENCE.md](API_REFERENCE.md)** - Complete API documentation
   - Core classes and interfaces
   - Blockchain API
   - Wallet API
   - Network API
   - RPC API
   - Lightning API

### External References

- [Bitcoin Developer Docs](https://developer.bitcoin.org/) - Bitcoin protocol reference
- [BOLT Specifications](https://github.com/lightning/bolts) - Lightning Network specs
- [NIST PQC](https://csrc.nist.gov/projects/post-quantum-cryptography) - Post-quantum crypto
- [RandomX](https://github.com/tevador/RandomX) - Mining algorithm

---

## Project Structure

```
intcoin/
├── src/                    # Implementation files
│   ├── blockchain/         # Blockchain core (blocks, transactions, validation)
│   ├── crypto/             # Post-quantum cryptography (Dilithium3, Kyber768)
│   ├── lightning/          # Lightning Network implementation
│   ├── network/            # P2P networking
│   ├── rpc/                # JSON-RPC server
│   ├── storage/            # RocksDB database layer
│   └── wallet/             # HD wallet, UTXO management
│
├── include/intcoin/        # Public header files
│   ├── blockchain.h
│   ├── crypto.h
│   ├── lightning.h
│   ├── network.h
│   ├── rpc.h
│   ├── storage.h
│   └── wallet.h
│
├── tests/                  # Test suites (13 total)
│   ├── test_crypto.cpp
│   ├── test_lightning.cpp
│   └── ...
│
├── docs/                   # Documentation
│   ├── DEVELOPER_INDEX.md  # This file
│   ├── BUILD_GUIDE.md
│   ├── DEVELOPER_GUIDE.md
│   ├── ARCHITECTURE.md
│   ├── RPC.md
│   ├── LIGHTNING.md
│   └── ...
│
├── cmake/                  # CMake modules and scripts
├── .github/workflows/      # GitHub Actions CI/CD
└── CONTRIBUTING.md         # Contribution guidelines
```

---

## Development Workflow

### 1. Setup
```bash
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
# Follow BUILD_GUIDE.md
```

### 2. Create Feature Branch
```bash
git checkout -b feature/your-feature-name
```

### 3. Develop
- Follow CODE_STYLE.md
- Write tests for new functionality
- Update documentation

### 4. Test
```bash
cd build && ctest --output-on-failure
```

### 5. Submit PR
```bash
git push origin feature/your-feature-name
# Create Pull Request on GitHub
```

See [CONTRIBUTING.md](../CONTRIBUTING.md) for detailed workflow.

---

## Key Technologies

| Technology | Purpose | Version |
|------------|---------|---------|
| **C++23** | Primary language | ISO/IEC 14882:2023 |
| **CMake** | Build system | 3.28+ |
| **Boost** | Utilities, threading | 1.90.0+ |
| **RocksDB** | Database | 6.11.4+ |
| **liboqs** | Post-quantum crypto | 0.15.0+ |
| **RandomX** | PoW algorithm | 1.2.1+ |
| **Qt 6** | GUI framework | 6.2+ |

---

## Community Resources

### Communication

- **GitHub Issues**: https://github.com/INT-devs/intcoin/issues
- **GitHub Discussions**: https://github.com/INT-devs/intcoin/discussions
- **Discord**: https://discord.gg/jCy3eNgx
- **Reddit**: https://www.reddit.com/r/INTcoin
- **Twitter/X**: https://x.com/INTcoin_team
- **Wiki**: https://github.com/INT-devs/intcoin/wiki

### Getting Help

1. Search existing issues and discussions
2. Check documentation in [docs/](.)
3. Ask on Discord #development channel
4. Open GitHub Discussion

---

## Release Information

**Current Version**: v1.0.0-beta  
**Release Date**: December 26, 2025  
**Status**: Production Testing  
**Next Milestone**: v1.0.0 Mainnet (Q1 2026)

See [RELEASE_NOTES.md](../RELEASE_NOTES.md) for complete release details.

---

## Contributing

We welcome contributions! Please read:

1. [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
2. [CODE_STYLE.md](CODE_STYLE.md) - Coding standards
3. [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) - Development practices

**Quick Contribution Checklist**:
- [ ] Read CONTRIBUTING.md
- [ ] Fork repository
- [ ] Create feature branch
- [ ] Follow code style
- [ ] Write tests
- [ ] Update documentation
- [ ] Submit Pull Request

---

## License

MIT License - see [LICENSE](../LICENSE)

By contributing, you agree to license your contributions under the MIT License.

---

**Maintainer**: INTcoin Development Team  
**Contact**: team@international-coin.org  
**Wiki**: https://github.com/INT-devs/intcoin/wiki

---

_This documentation index is maintained as part of the INTcoin project. For updates, please submit a Pull Request._
