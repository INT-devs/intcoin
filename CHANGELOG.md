# Changelog

All notable changes to INTcoin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0-beta] - 2026-01-06

**Status**: Production Beta - Supersedes v1.0.0-alpha  
**Test Coverage**: 100% (13/13 tests passing)  
**Codename**: "Lightning Strike"

### Added

#### Lightning Network (100% Complete)
- Full BOLT (Basis of Lightning Technology) implementation
- Channel management (BOLT #2) - Open, close, manage payment channels
- HTLC lifecycle - Add, fulfill, fail HTLCs with timeout handling
- Multi-hop payment routing with automatic pathfinding
- BOLT #11 invoice creation and payment
- Network gossip protocol (BOLT #7) for node discovery
- Watchtower integration (BOLT #13) for channel monitoring
- Onion routing (BOLT #4) for payment privacy
- Post-quantum channel announcements

#### RPC API Enhancements (47+ Methods)
- **Lightning RPC** (7 methods):
  - `lightning_openchannel` - Open payment channel
  - `lightning_closechannel` - Close payment channel
  - `lightning_sendpayment` - Send Lightning payment
  - `lightning_createinvoice` - Create BOLT #11 invoice
  - `lightning_payinvoice` - Pay BOLT #11 invoice
  - `lightning_listchannels` - List all channels
  - `lightning_getchannelinfo` - Get channel details

- **Pool Server RPC** (4 methods):
  - `pool_getstats` - Pool statistics and metrics
  - `pool_getworkers` - Active worker information
  - `pool_getpayments` - Payment history
  - `pool_gettopminers` - Top miner rankings

- **Fee Estimation RPC** (3 methods):
  - `estimatesmartfee` - Smart fee estimation
  - `estimaterawfee` - Raw fee estimation
  - `estimatefee` - Simple fee estimation

- **Enhanced Blockchain RPC** (4 methods):
  - `getblockstats` - Block statistics
  - `getrawmempool` - Mempool with verbose option
  - `gettxoutsetinfo` - UTXO set information
  - `getblockheader` - Block header details

#### Core Improvements
- Complete UTXOSet implementation (10 methods, thread-safe)
- In-memory UTXO cache with RocksDB persistence
- Block application and reversion support
- Address-based UTXO queries

#### Testing
- Lightning Network test suite (10/10 tests, 713 lines)
- 100% test coverage (13/13 tests passing)
- Fixed ValidationTest P2PKH signing issue
- Comprehensive integration tests

#### Documentation
- Complete developer documentation suite (5 new files, 2,500+ lines)
- BUILD_GUIDE.md - Platform-specific build instructions
- CODE_STYLE.md - C++23 coding standards
- DEVELOPER_INDEX.md - Documentation hub
- TEST_ENHANCEMENT_PLAN.md - Testing roadmap
- DOCUMENTATION_SUMMARY.md - Complete documentation overview
- Updated CONTRIBUTING.md to v1.0.0-beta
- Updated all documentation with GitHub repository links

#### CI/CD
- GitHub Actions workflows for all platforms
- Multi-platform builds (Ubuntu, macOS, Windows)
- Automated testing on all platforms
- Security scanning (CodeQL, Trivy)
- Documentation validation
- Dependency vulnerability scanning

### Changed
- Upgraded Boost requirement to 1.90.0+
- Upgraded CMake requirement to 3.28+
- Upgraded RocksDB to 6.11.4+
- Repository migrated from GitLab to GitHub
- Updated community links (Discord, Reddit, Twitter/X, Wiki)
- Enhanced RELEASE_NOTES.md with comprehensive beta details
- Updated BETA_ROADMAP.md - All phases complete

### Fixed
- ValidationTest P2PKH signing issue (13/13 tests now passing)
- UTXOSet::GetCount() implementation
- Network hashrate calculation
- Signature verification edge cases
- Memory leaks in Lightning channel management
- Race conditions in pool server worker management

### Security
- Post-quantum channel security
- Enhanced cryptographic verification
- Improved input validation across all RPC methods
- Security audit preparation

---

## [1.0.0-alpha] - 2025-12-25

**Status**: Alpha Release - Superseded by v1.0.0-beta  
**Test Coverage**: 92% (12/13 tests passing)

### Added
- Core blockchain functionality
- Post-quantum cryptography (Dilithium3, Kyber768)
- RandomX proof-of-work mining
- HD wallet with BIP39/BIP44 support
- Qt desktop wallet
- Mining pool server (Stratum protocol)
- RPC API (35+ methods)
- P2P networking
- Machine learning anomaly detection
- Genesis block and initial distribution

### Known Issues (Fixed in Beta)
- ValidationTest P2PKH signing failure
- Missing UTXOSet implementation
- Incomplete Lightning Network
- Limited RPC methods

---

## Version History Summary

| Version | Release Date | Status | Tests | Major Features |
|---------|-------------|--------|-------|----------------|
| **1.0.0-beta** | 2026-01-06 | **Current** | 13/13 (100%) | Lightning Network, Enhanced RPC, Complete Docs |
| 1.0.0-alpha | 2025-12-25 | Superseded | 12/13 (92%) | Core blockchain, Mining, Wallets |

---

## Upgrade Path

### From v1.0.0-alpha to v1.0.0-beta

**Required Steps**:
1. Backup your wallet and blockchain data
2. Stop all running INTcoin services
3. Upgrade binaries to v1.0.0-beta
4. Update configuration files (see RELEASE_NOTES.md)
5. Restart services
6. Verify synchronization

**Breaking Changes**: None - Fully backward compatible with v1.0.0-alpha blockchain data

**New Features Available**:
- Lightning Network channels and payments
- Enhanced RPC methods (47+ total)
- Improved fee estimation
- Pool server statistics

See [RELEASE_NOTES.md](RELEASE_NOTES.md) for detailed upgrade instructions.

---

## Links

- **Repository**: https://github.com/INT-devs/intcoin
- **Issues**: https://github.com/INT-devs/intcoin/issues
- **Discussions**: https://github.com/INT-devs/intcoin/discussions
- **Wiki**: https://github.com/INT-devs/intcoin/wiki
- **Discord**: https://discord.gg/jCy3eNgx
- **Reddit**: https://www.reddit.com/r/INTcoin
- **Twitter/X**: https://x.com/INTcoin_team

---

**Maintainer**: INTcoin Development Team  
**Contact**: team@international-coin.org  
**Last Updated**: December 26, 2025
