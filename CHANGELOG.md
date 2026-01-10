# Changelog

All notable changes to INTcoin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.4.0-beta] - 2026-01-09

**Status**: Production Beta - Major Feature Release
**Test Coverage**: 100% (64 test suites, all passing)
**Codename**: "Quantum Contracts"

### Added

#### Smart Contracts (IntSC VM)
- **EVM-Compatible Virtual Machine**: Full EVM instruction set (60+ opcodes)
- **PQC Opcodes**: 4 quantum-resistant operations (DILITHIUM_VERIFY, KYBER_ENCAP, KYBER_DECAP, PQC_PUBKEY)
- **Contract Deployment**: Type 2 transactions for deploying bytecode
- **Contract Calls**: Type 3 transactions for executing contract functions
- **Gas Metering**: EVM-compatible gas costs with PQC adjustments
- **Contract Storage**: RocksDB-backed persistent key-value storage per contract
- **Event Logs**: Indexed by block number with topic filtering
- **Transaction Receipts**: Complete execution results with gas tracking
- **12 New RPC Methods**:
  - `deploycontract`, `callcontract`, `getcontractinfo`, `getcontractcode`
  - `getcontractstorage`, `getreceipt`, `getlogs`, `estimategas`
  - `getgasprice`, `listcontracts`, `getcontractbalance`, `getstoragehash`
- **Performance**: 2,188 deployments/sec, 2,184 calls/sec
- Implementation: `src/contracts/`

#### Initial Block Download (IBD) Optimization
- **Parallel Validation**: Multi-threaded block validation (utilizes all CPU cores)
- **Assume-UTXO**: Fast sync from Dilithium3-signed UTXO snapshots
- **Sync Speed**: ~1000 blocks/second (10x improvement)
- Implementation: `src/ibd/`

#### Bitcoin-Style Address Manager
- **Netgroup Bucketing**: /16 subnet grouping for Sybil attack prevention
- **NEW/TRIED Buckets**: Address verification state tracking
- **Weighted Selection**: Random peer selection based on connection history
- **peers.dat v2**: Extended format with connection metrics
- Implementation: `src/network/network.cpp`

#### Mempool Enhancements
- **Contract Transaction Routing**: Dedicated handling for deployment and calls
- **Nonce Tracking**: Per-address sequential nonce management
- **Replace-By-Fee (RBF)**: 10% minimum gas price increase required
- **Gas Limits**: 60M mempool limit, 30M per block

#### CI/CD Improvements
- **Multi-Platform Builds**: Ubuntu 24.04, macOS, Fedora 40, Debian 13
- **Security Scanning**: CodeQL, Trivy dependency scanning
- **Workflow Permissions**: Least privilege model for all workflows

### Changed
- Replaced satoshi/sats terminology with INT/ints throughout codebase
- Updated network protocol for v1.4.0 features
- Mempool now supports contract transactions alongside regular UTXO transfers
- Block validation includes contract execution pipeline
- Transaction structure extended with `contract_data` field

### Fixed
- Mempool nonce tracking edge cases
- Gas limit overflow in block validation
- Event log query pagination
- Contract address generation for high nonces
- GCC type-limits warnings in test files
- Qt6 compatibility issues (checkbox signal handling)
- hidapi library detection on multiple platforms

### Security
- Added workflow-level permissions to CI workflows
- CodeQL cleartext transmission alerts addressed with documentation
- All contract transactions use Dilithium3 signatures (3,309 bytes)

### Commits (selected)
- c0520a0 - Fix GitHub code scanning security alerts
- 7155dd1 - Add Bitcoin-style address manager for peer discovery
- 288c324 - Replace satoshi/sats terminology with INT/ints
- db8a6b9 - Add Debian 13 (Trixie) build to CI workflow
- b39d616 - Fix Fedora container checkout and add Windows 11 build

---

## [1.3.0-beta] - 2026-01-06

**Status**: Production Beta - Infrastructure Release
**Test Coverage**: 100% (51 test suites, all passing)
**Codename**: "Foundation"

### Added

#### DeFi Infrastructure
- **Lending Protocol**: Collateralized lending with liquidation
- **Mempool Analytics**: Transaction pattern analysis and fee prediction
- **ML Anomaly Detection**: Machine learning-based threat detection

#### QR Code Support
- **libqrencode Integration**: Native QR code generation
- **Address QR Codes**: Payment URIs with amount and label
- **Lightning Invoice QR**: Support for BOLT #11 invoices
- **SVG Generation**: Vector QR code output

### Changed
- Updated dependency versions for Ubuntu 24.04 compatibility
- Enhanced build system with better platform detection

### Fixed
- Mempool analytics compiler errors
- Missing `<algorithm>` include in defi/lending.cpp
- Qt version-based conditional compilation

---

## [1.2.0-beta] - 2026-01-02

**Status**: Production Beta - Major Feature Release
**Test Coverage**: 100% (38 new tests, all passing)
**Codename**: "CrossChain"

### Added

#### Phase 1: Mobile Wallet Infrastructure
- **SPV (Simplified Payment Verification) Client**
  - Header-only synchronization for lightweight clients
  - Merkle proof verification without full blockchain
  - 99.9% bandwidth reduction vs full node
  - Target sync time: <30 seconds
  - Implementation: `src/spv/spv.cpp`

- **Bloom Filters (BIP37)**
  - BIP37-compliant transaction filtering for SPV clients
  - MurmurHash3 implementation
  - False positive rate: 1.09% (target: 1.0%)
  - Privacy-preserving address watching
  - Filter update modes: NONE, ALL, P2PUBKEY_ONLY
  - 8/8 tests passing
  - Implementation: `src/bloom/bloom.cpp`

- **Mobile RPC API** (7 new methods)
  - `mobile_sync` - Header and transaction synchronization
  - `mobile_getbalance` - Quick balance queries
  - `mobile_gethistory` - Paginated transaction history
  - `mobile_sendtransaction` - Transaction broadcasting
  - `mobile_getutxos` - UTXO retrieval
  - `mobile_estimatefee` - Dynamic fee estimation
  - `mobile_getnetworkstatus` - Network information
  - Implementation: `src/mobile/mobile_rpc.cpp`

- **iOS SDK**
  - Swift Package Manager and CocoaPods support
  - BIP39 mnemonic wallet creation
  - BIP32/44 HD key derivation (m/44'/2210'/0'/0/*)
  - QR code payment URI support
  - Biometric authentication (Face ID / Touch ID)
  - Keychain secure storage
  - Example app with full source
  - Requirements: iOS 14+, Xcode 15+, Swift 5.9+
  - Implementation: `mobile/ios/INTcoinSDK.swift`

- **Android SDK**
  - Gradle build system integration
  - Kotlin native implementation
  - BIP39/32/44 wallet support
  - QR code scanning and generation
  - Biometric authentication (Fingerprint / Face unlock)
  - EncryptedSharedPreferences for secure storage
  - Example app with full source
  - Requirements: Android 8+, Android Studio 2024+, Kotlin 1.9+
  - Implementation: `mobile/android/src/main/java/org/intcoin/sdk/INTcoinSDK.kt`

#### Phase 2: Atomic Swaps
- **HTLC (Hash Time-Locked Contracts)**
  - On-chain HTLC support for trustless atomic swaps
  - Hash algorithms: SHA3-256, SHA-256, RIPEMD160
  - Time locks: CSV (block height) and CLTV (timestamp)
  - Dual redemption paths: claim with preimage or refund after timeout
  - Full HTLC script verification
  - Implementation: `src/htlc/htlc.cpp`

- **Atomic Swap Protocol**
  - 12-state state machine for reliable swap execution
  - Supported chains: Bitcoin (BTC), Litecoin (LTC), INTcoin (INT)
  - Trustless execution with no third-party custody
  - Automatic monitoring and swap coordination
  - Timeout protection (Initiator: 48h, Participant: 24h)
  - Refund mechanism for failed/expired swaps
  - Implementation: `src/swap/atomic_swap.cpp`

- **Atomic Swap RPC** (5 new methods)
  - `createswap` - Initiate swap offer
  - `acceptswap` - Accept swap offer
  - `getswapstatus` - Check swap progress
  - `cancelswap` - Cancel pending swap
  - `monitorswap` - Monitor swap execution

- **Cross-Chain Integration**
  - Bitcoin integration (full BTC ↔ INT swap support)
  - Litecoin integration (full LTC ↔ INT swap support)
  - Bitcoin monitoring: `src/blockchain/bitcoin_monitor.cpp`
  - Litecoin monitoring: `src/blockchain/litecoin_monitor.cpp`
  - Exchange rate negotiation built into protocol
  - Network fee handling for all supported chains

#### Phase 3: Cross-Chain Bridges
- **Bridge Architecture**
  - Federated multi-sig model for decentralized security
  - M-of-N threshold signatures (requires majority validator approval)
  - Validator set management with rotation support
  - Merkle proof verification for cross-chain transactions
  - Emergency pause mechanism (circuit breaker)
  - Anomaly detection and real-time monitoring
  - Supply consistency checks for wrapped tokens
  - Implementation: `src/bridge/bridge.cpp`

- **Wrapped Token Support**
  - wBTC (Wrapped Bitcoin) - 1:1 peg with Bitcoin
  - wETH (Wrapped Ethereum) - 1:1 peg with Ethereum
  - wLTC (Wrapped Litecoin) - 1:1 peg with Litecoin
  - Token metadata tracking (decimals, origin chain, contract address)
  - Mint/burn mechanism for deposit/withdrawal
  - Public supply auditing (total supply vs locked assets)

- **Bridge Operations**
  - Deposit: Lock assets on origin chain → Mint wrapped tokens on INTcoin
  - Withdrawal: Burn wrapped tokens → Unlock assets on origin chain
  - Confirmation requirements: BTC: 6, ETH: 12, LTC: 24
  - Target finality: <30 minutes
  - Validator signature collection (M-of-N threshold)

- **Bridge RPC** (5 new methods)
  - `bridgedeposit` - Deposit assets (lock & mint)
  - `bridgewithdraw` - Withdraw assets (burn & unlock)
  - `getbridgebalance` - Check wrapped token balances
  - `listbridgetransactions` - Bridge transaction history
  - `getbridgeinfo` - Bridge configuration and status

- **Bridge Web UI**
  - React/TypeScript modern web interface
  - Vite build system
  - TanStack Query for data fetching
  - Wallet integration: MetaMask, Keplr, INTcoin wallet
  - Asset deposit/withdrawal interface
  - Real-time transaction monitoring
  - Bridge statistics dashboard
  - Production ready with Nginx deployment
  - Implementation: `bridge-ui/` directory

- **Bridge Security & Monitoring**
  - Multi-sig validation for all transactions
  - Real-time validator health checks
  - Transaction confirmation tracking
  - Supply discrepancy detection
  - Double-spend prevention
  - Rate limiting and abuse prevention
  - Validator reputation system
  - Implementation: `src/bridge/bridge_monitor.cpp`

#### Phase 4: Supporting Infrastructure
- **Enhanced Mempool (4.1)**
  - 6-level priority system: LOW, NORMAL, HIGH, HTLC, BRIDGE, CRITICAL
  - Fee-based prioritization (higher fees = higher priority)
  - Dependency tracking for parent/child transactions
  - Eviction policy (lowest priority evicted when full)
  - Mempool persistence to `mempool.dat` (save on shutdown, restore on startup)
  - Transaction expiry after 72 hours (configurable)
  - Configuration: max size 300 MB, min relay fee 1000 INTS/KB
  - 12/12 tests passing
  - Implementation: `src/mempool/mempool.cpp`

- **Enhanced P2P Protocol - Bloom Filters (4.2)**
  - BIP37 P2P extensions: filterload, filteradd, filterclear, merkleblock
  - Header-first synchronization for SPV clients
  - Bandwidth optimization (only relay matching transactions)
  - Privacy via false positives (plausible deniability)
  - 8/8 tests passing

- **Prometheus Metrics System (4.3)**
  - 4 metric types: Counter, Gauge, Histogram, Timer
  - 40+ standard metrics:
    - Blockchain: blocks_processed, transactions_processed, blockchain_height, blockchain_difficulty, block_processing_duration, block_size
    - Mempool: mempool_size, mempool_bytes, mempool_accepted, mempool_rejected, mempool_tx_fee
    - Network: peer_count, bytes_sent, bytes_received, messages_sent, messages_received, message_processing_duration
    - Mining: blocks_mined, hashes_computed, hashrate, mining_duration
    - Wallet: wallet_balance, wallet_transactions, wallet_utxo_count
    - SPV/P2P: spv_best_height, bloom_filters_loaded, header_sync_duration
  - Thread-safe implementation (atomics for Counter/Gauge, mutex for Histogram)
  - MetricsRegistry singleton for global metric access
  - Prometheus text exposition format (version 0.0.4)
  - 10/10 tests passing
  - Implementation: `src/metrics/metrics.cpp`

- **Prometheus HTTP Endpoint (4.4)**
  - HTTP server serving metrics at GET /metrics
  - Configuration: bind 127.0.0.1:9090 (default, configurable)
  - Cross-platform: Windows (Winsock) and POSIX sockets
  - Multi-threaded request handling (2 worker threads default)
  - Proper HTTP responses: 200 OK, 404 Not Found, 405 Method Not Allowed, 400 Bad Request
  - Request counting and statistics
  - Graceful shutdown with thread cleanup
  - 8/8 tests passing
  - Implementation: `src/metrics/metrics_server.cpp`

### Changed
- Network protocol version bumped for v1.2.0 features
- Mempool now uses priority queues instead of simple queue
- P2P protocol extended with bloom filter messages
- Enhanced mempool priority for Lightning/bridge transactions

### Fixed
- Mempool mutex deadlocks in GetBlockTemplate and RemoveTransaction (59335a6)
- Mempool fee calculation now properly computes fees based on tx size
- Thread safety improvements across all new components

### Technical Details
- **New Files**: 85 files added
- **Lines of Code**: +18,338 additions
- **New Tests**: 38 tests (all passing)
- **New RPC Methods**: 20+ methods
- **Supported Platforms**: Linux, FreeBSD, Windows, iOS 14+, Android 8+

### Commits
- 24c8dbe - Add Prometheus HTTP Metrics Endpoint - Phase 4 Complete!
- 14f206f - Complete Phase 4.3: Prometheus Metrics & Monitoring System
- 1011008 - Add comprehensive bloom filter test suite - ALL TESTS PASSING
- 59335a6 - Fix mempool mutex deadlocks - ALL TESTS PASSING
- 504513d - Phase 4.1: Enhanced Mempool with priority queues and persistence
- 07ebee5 - Remove bug bounty references - community project model
- a9a8cdd - Implement Phase 3.4: Bridge Web UI
- 8c08789 - Implement Phase 3.3: Bridge Security and Monitoring System
- 4b53be3 - Implement Phase 3.2: Wrapped Token RPC Commands
- 9e11ddd - Implement Phase 3.1: Cross-Chain Bridge Architecture
- 2cab9e7 - Update documentation: GPG keys, GitHub migration
- 7fd43de - Implement Phase 2: Atomic Swaps - Cross-chain trustless exchanges
- dcf36ec - Bump version to v1.2.0-beta

### Known Issues
- iOS/Android apps not yet on App Store/Google Play (use TestFlight/sideload)
- Bridge in testnet phase (mainnet requires security audit)
- Atomic swap exchange rate negotiation is manual
- Metrics endpoint has no built-in authentication (use firewall/reverse proxy)

### Upgrade Path
1. Backup wallet: `intcoin-cli backupwallet /path/to/backup.dat`
2. Stop node: `intcoin-cli stop`
3. Update binaries to v1.2.0-beta
4. Restart node: `intcoind -daemon`
5. Verify: `intcoin-cli getnetworkinfo` (should show version 120000)

No database migration required. Fully backward compatible with v1.1.0-beta.

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
| **1.4.0-beta** | 2026-01-09 | **Current** | 64/64 (100%) | Smart Contracts, IBD Optimization, Address Manager |
| 1.3.0-beta | 2026-01-06 | Stable | 51/51 (100%) | DeFi Infrastructure, QR Codes |
| 1.2.0-beta | 2026-01-02 | Stable | 38/38 (100%) | Mobile SDKs, Atomic Swaps, Cross-Chain Bridges |
| 1.0.0-beta | 2026-01-06 | Stable | 13/13 (100%) | Lightning Network, Enhanced RPC |
| 1.0.0-alpha | 2025-12-25 | Superseded | 12/13 (92%) | Core blockchain, Mining, Wallets |

---

## Upgrade Path

### From v1.3.0-beta to v1.4.0-beta

**Required Steps**:
1. Backup your wallet: `intcoin-cli backupwallet /path/to/backup.dat`
2. Stop node: `intcoin-cli stop`
3. Update binaries to v1.4.0-beta
4. Restart node: `intcoind -daemon`
5. Verify: `intcoin-cli getnetworkinfo` (should show version 140000)

**Breaking Changes**: None - Fully backward compatible

**New Features Available**:
- Smart contract deployment and execution
- 12 new contract RPC methods
- 10x faster initial block download
- Bitcoin-style peer address management

### From v1.2.0-beta to v1.3.0-beta

**Required Steps**:
1. Stop node and backup wallet
2. Update binaries
3. Restart - new databases initialize automatically

**New Features**: DeFi lending, QR code generation

### From v1.0.0-alpha to v1.0.0-beta

**Required Steps**:
1. Backup your wallet and blockchain data
2. Stop all running INTcoin services
3. Upgrade binaries to v1.0.0-beta
4. Restart services

**New Features Available**:
- Lightning Network channels and payments
- Enhanced RPC methods (47+ total)

See [RELEASE_NOTES_v1.4.0-beta.md](RELEASE_NOTES_v1.4.0-beta.md) for detailed upgrade instructions.

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
**Last Updated**: January 10, 2026
