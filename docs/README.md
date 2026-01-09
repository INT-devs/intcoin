# INTcoin Documentation

**Version**: 1.4.0-beta
**Last Updated**: January 9, 2026
**Code Name**: "Quantum Contracts"

Complete documentation for INTcoin - the world's first quantum-resistant cryptocurrency with EVM-compatible smart contracts.

---

## What's New in v1.4.0 🎉

- **[Smart Contracts](SMART_CONTRACTS.md)** - EVM-compatible IntSC VM with post-quantum signatures
- **[IBD Optimization](IBD_OPTIMIZATION.md)** - 10x faster sync with parallel validation
- **[Mempool Enhancements](ENHANCED_MEMPOOL.md)** - Contract transaction support with gas metering

---

## Quick Start

### New Users
1. **[Getting Started](GETTING_STARTED.md)** - Complete introduction to INTcoin
2. **[Quick Start](QUICK_START.md)** - Get running in 5 minutes
3. **[Building from Source](BUILDING.md)** - Compile INTcoin yourself

### Platform-Specific Installation
- **[Linux Installation](INSTALL_LINUX.md)** - Ubuntu, Debian, Fedora, Arch
- **[FreeBSD Installation](INSTALL_FREEBSD.md)** - FreeBSD 13.0+
- **[Windows Installation](BUILD_WINDOWS.md)** - Windows 11 build guide
- **[macOS Build Guide](BUILD_GUIDE.md)** - macOS 13+ (Ventura)

---

## Core Documentation

### Architecture & Design
- **[Architecture](ARCHITECTURE.md)** - System architecture and components
- **[Consensus Rules](CONSENSUS.md)** - Blockchain consensus and validation
- **[Cryptography](CRYPTOGRAPHY.md)** - Post-quantum cryptographic implementation
- **[Address Encoding](ADDRESS_ENCODING.md)** - Bech32 address format

### Smart Contracts (v1.4.0)
- **[Smart Contracts Overview](SMART_CONTRACTS.md)** - IntSC VM architecture
- **[Smart Contracts Specification](SMART_CONTRACTS_SPEC.md)** - Technical specification
- **[Contracts Integration](CONTRACTS_INTEGRATION.md)** - Integration implementation details
- **[v1.4.0 Integration Status](V1.4.0_INTEGRATION_STATUS.md)** - Phase 1-3 completion report
- **[v1.4.0 Test Summary](V1.4.0_TEST_SUMMARY.md)** - Comprehensive test results

### Blockchain Features
- **[Lightning Network](LIGHTNING.md)** - Layer 2 payment channels
- **[Lightning v2](LIGHTNING_V2.md)** - Enhanced Lightning features
- **[Atomic Swaps](ATOMIC_SWAPS.md)** - Cross-chain trustless exchanges
- **[Cross-Chain Bridges](CROSS_CHAIN_BRIDGES.md)** - Interoperability protocol
- **[SPV & Bloom Filters](SPV_AND_BLOOM_FILTERS.md)** - Lightweight clients
- **[Privacy Features](PRIVACY.md)** - Network privacy and anonymity

### Node Operation
- **[Mining Guide](MINING.md)** - CPU mining with RandomX
- **[Miner Guide](MINER_GUIDE.md)** - Advanced mining configuration
- **[Mining Pool Setup](POOL_SETUP.md)** - Run your own mining pool
- **[Mining Pool Stratum](Mining-Pool-Stratum.md)** - Stratum protocol implementation
- **[Block Explorer](BLOCK_EXPLORER.md)** - Blockchain explorer backend

---

## Developer Resources

### API & RPC
- **[RPC API Reference](RPC.md)** - Complete JSON-RPC API (includes 12 contract methods)
- **[API Reference](API_REFERENCE.md)** - General API documentation
- **[Developer Index](DEVELOPER_INDEX.md)** - Developer resource index

### Development
- **[Code Style Guide](CODE_STYLE.md)** - C++23 coding standards
- **[Testing Guide](TESTING.md)** - Writing and running tests
- **[Build System](BUILDING.md)** - CMake build configuration
- **[Compatibility](COMPATIBILITY.md)** - Platform compatibility matrix

### Security
- **[Security Audit Checklist](SECURITY_AUDIT_CHECKLIST.md)** - Security review guidelines
- **[Security Sanitization](SECURITY_SANITIZATION.md)** - Address sanitizers and memory safety

---

## User Guides

### Wallets
- **[Qt Wallet Guide](QT_WALLET_GUIDE.md)** - Desktop GUI wallet
- **[Wallet Overview](WALLET.md)** - Wallet functionality
- **[Wallet Features](WALLET_FEATURES.md)** - HD wallet, BIP32/BIP39
- **[Mobile Wallet](MOBILE_WALLET.md)** - Mobile wallet architecture
- **[Mobile SDK](MOBILE_SDK.md)** - Mobile development SDK

### General Use
- **[User Guide](USER_GUIDE.md)** - Complete user documentation
- **[Migration Guide](MIGRATION_GUIDE.md)** - Upgrading from previous versions

---

## Tutorials

- **[Tutorial: Atomic Swaps](TUTORIAL_ATOMIC_SWAPS.md)** - Step-by-step atomic swap guide
- **[Tutorial: Monitoring](TUTORIAL_MONITORING.md)** - Node monitoring with Prometheus/Grafana

---

## Operations & Monitoring

### Monitoring & Metrics
- **[Prometheus Metrics](PROMETHEUS_METRICS.md)** - Metrics collection and export
- **[Grafana Dashboards](GRAFANA_DASHBOARDS.md)** - Visualization dashboards

### Infrastructure
- **[CI/CD Pipeline](CI-CD-PIPELINE.md)** - Continuous integration setup
- **[OS Support Policy](OS_SUPPORT_POLICY.md)** - Operating system support matrix
- **[Enhanced Mempool](ENHANCED_MEMPOOL.md)** - Mempool architecture (v1.4.0 updated)

---

## Reference Documents

### Specifications
- **[BOLT Specification](BOLT_SPECIFICATION.md)** - Lightning Network BOLT compliance

### Technical Requirements
- **[Technical Requirements](../TECHNICAL_REQUIREMENTS.md)** - Build and performance requirements
- **[Security Policy](../SECURITY.md)** - Security best practices and reporting

---

## Version History

### v1.4.0-beta (Current - January 9, 2026)
- **Smart Contracts**: IntSC VM with EVM compatibility
- **IBD Optimization**: 10x faster sync with parallel validation
- **Mempool Enhancements**: Contract transaction support
- **Test Suite**: 64 total test suites (13 new for contracts)
- **Performance**: 2,000+ contract TPS

### v1.3.0-beta (January 2, 2026)
- Lightning Network enhancements
- Mempool analytics and persistence
- Prometheus metrics integration

### v1.2.0-beta (December 2025)
- Mobile wallet foundation (SPV, Bloom filters)
- Atomic swaps implementation
- Cross-chain bridges

### v1.0.0-beta (Initial Release)
- Core blockchain implementation
- Post-quantum cryptography (Dilithium3, Kyber768)
- RandomX proof-of-work
- Qt desktop wallet
- Basic Lightning Network foundation

---

## Key Features

### Post-Quantum Security
- **Dilithium3** signatures (NIST-standardized)
- **Kyber768** key encapsulation
- **SHA3-256** hashing
- Future-proof against quantum computers

### Smart Contracts (v1.4.0)
- **EVM-compatible** VM with 60+ opcodes
- **4 PQC opcodes** for quantum-resistant operations
- **Gas metering** with EVM-compatible costs
- **2,000+ TPS** for contract transactions
- **Quantum-resistant** contract signatures

### Performance
- **Contract deployment**: 2,188 ops/sec
- **Contract calls**: 2,184 ops/sec
- **Database reads**: 2.5M ops/sec
- **Database writes**: 242K ops/sec
- **IBD sync**: ~1000 blocks/sec (10x improvement)

---

## Community & Support

### Official Channels
- **Website**: https://international-coin.org
- **Twitter/X**: https://x.com/INTcoin_team
- **Discord**: https://discord.gg/jCy3eNgx
- **Reddit**: https://www.reddit.com/r/INTcoin
- **GitHub**: https://github.com/INT-devs/intcoin

### Support
- **GitHub Issues**: https://github.com/INT-devs/intcoin/issues
- **General Inquiries**: admin@international-coin.org
- **Security**: security@international-coin.org (PGP preferred)

---

## Contributing

We welcome contributions! Please see:
- **[Code Style Guide](CODE_STYLE.md)** - Coding standards
- **[Testing Guide](TESTING.md)** - Writing tests
- **[CLAUDE.md](../CLAUDE.md)** - Guide for AI assistants working on the codebase

---

## License

INTcoin is released under the MIT License. See [LICENSE](../LICENSE) for details.

---

**INTcoin v1.4.0-beta** - Securing value in the quantum era with smart contracts

*Released: January 9, 2026*
