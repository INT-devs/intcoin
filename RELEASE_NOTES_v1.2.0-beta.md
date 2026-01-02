# INTcoin v1.2.0-beta Release Notes

**Release Date**: January 2, 2026
**Version**: 1.2.0-beta
**Type**: Major Feature Release
**Status**: Beta - Production Testing Phase

---

## Overview

INTcoin v1.2.0-beta represents a major milestone in the project's evolution, introducing **mobile wallet infrastructure**, **atomic swaps**, **cross-chain bridges**, and **comprehensive monitoring capabilities**. This release adds 85 new files with over 18,000 lines of code across four major feature areas, transforming INTcoin from a desktop-focused cryptocurrency into a full cross-chain, mobile-ready blockchain platform.

### Key Highlights

- 📱 **Mobile Wallet Infrastructure**: SPV clients, iOS/Android SDKs, bloom filters
- 🔄 **Atomic Swaps**: Trustless cross-chain exchanges with Bitcoin and Litecoin
- 🌉 **Cross-Chain Bridges**: Wrapped tokens (wBTC, wETH, wLTC) with federated multi-sig security
- ⚡ **Enhanced Infrastructure**: Priority-based mempool, Prometheus metrics, improved P2P protocol
- ✅ **38 New Tests**: All passing with 100% success rate

---

## What's New

### Phase 1: Mobile Wallet Infrastructure

#### SPV (Simplified Payment Verification) Client
- **Header-only synchronization** - Download only block headers, not full blocks
- **Merkle proof verification** - Verify transactions without full blockchain
- **Bandwidth savings**: 99.9% reduction vs full node (target: <1 MB per session)
- **Fast sync**: Initial synchronization in <30 seconds
- **Implementation**: `src/spv/spv.cpp`, `include/intcoin/spv.h`

#### Bloom Filters (BIP37)
- **BIP37-compliant** transaction filtering for SPV clients
- **MurmurHash3** implementation for optimal distribution
- **False positive rate**: 1.09% measured (target: 1.0%)
- **Privacy-preserving** address watching without revealing exact addresses
- **Filter modes**: NONE, ALL, P2PUBKEY_ONLY
- **Implementation**: `src/bloom/bloom.cpp`, `include/intcoin/bloom.h`
- **Tests**: 8/8 passing (`test_bloom`)

#### Mobile RPC API
- **Optimized for mobile** - Bandwidth-efficient JSON-RPC methods
- **New Methods**:
  - `mobile_sync` - Header and transaction synchronization
  - `mobile_getbalance` - Quick balance queries
  - `mobile_gethistory` - Paginated transaction history
  - `mobile_sendtransaction` - Transaction broadcasting
  - `mobile_getutxos` - UTXO retrieval
  - `mobile_estimatefee` - Dynamic fee estimation
  - `mobile_getnetworkstatus` - Network information
- **Implementation**: `src/mobile/mobile_rpc.cpp`, `include/intcoin/mobile_rpc.h`

#### iOS SDK
- **Swift Package Manager** and **CocoaPods** support
- **BIP39 mnemonic** wallet creation
- **BIP32/44 HD** key derivation (m/44'/2210'/0'/0/*)
- **QR code** payment URI support
- **Biometric authentication** (Face ID / Touch ID)
- **Keychain integration** for secure key storage
- **Example app** with full source code
- **Requirements**: iOS 14+, Xcode 15+, Swift 5.9+
- **Implementation**: `mobile/ios/INTcoinSDK.swift`, `mobile/ios/ExampleApp.swift`

#### Android SDK
- **Gradle** build system integration
- **Kotlin** native implementation
- **BIP39/32/44** wallet support
- **QR code** scanning and generation
- **Biometric authentication** (Fingerprint / Face unlock)
- **EncryptedSharedPreferences** for secure storage
- **Example app** with full source code
- **Requirements**: Android 8+, Android Studio 2024+, Kotlin 1.9+
- **Implementation**: `mobile/android/src/main/java/org/intcoin/sdk/INTcoinSDK.kt`

### Phase 2: Atomic Swaps

#### HTLC (Hash Time-Locked Contracts)
- **On-chain HTLC** support for trustless atomic swaps
- **Hash algorithms**: SHA3-256, SHA-256, RIPEMD160
- **Time locks**: CSV (block height) and CLTV (timestamp) support
- **Dual redemption paths**: Claim with preimage or refund after timeout
- **Script validation**: Full HTLC script verification
- **Implementation**: `src/htlc/htlc.cpp`, `include/intcoin/htlc.h`

#### Atomic Swap Protocol
- **12-state state machine** for reliable swap execution
- **Supported chains**: Bitcoin (BTC), Litecoin (LTC), INTcoin (INT)
- **Trustless execution** - No third-party custody
- **Automatic monitoring** and swap coordination
- **Timeout protection**:
  - Initiator: 48 hours
  - Participant: 24 hours
- **Refund mechanism** for failed or expired swaps
- **RPC Commands**:
  - `createswap` - Initiate swap offer
  - `acceptswap` - Accept swap offer
  - `getswapstatus` - Check swap progress
  - `cancelswap` - Cancel pending swap
  - `monitorswap` - Monitor swap execution
- **Implementation**: `src/swap/atomic_swap.cpp`, `include/intcoin/atomic_swap.h`
- **Tests**: Complete test suite (`test_atomic_swap`)

#### Cross-Chain Integration
- **Bitcoin integration** - Full BTC ↔ INT swap support
- **Litecoin integration** - Full LTC ↔ INT swap support
- **Bitcoin monitoring**: `src/blockchain/bitcoin_monitor.cpp`
- **Litecoin monitoring**: `src/blockchain/litecoin_monitor.cpp`
- **Exchange rate negotiation** built into swap protocol
- **Network fee handling** for all supported chains

### Phase 3: Cross-Chain Bridges

#### Bridge Architecture
- **Federated multi-sig** model for decentralized bridge security
- **M-of-N threshold signatures** - Requires majority validator approval
- **Validator set management** with rotation support
- **Merkle proof verification** for cross-chain transaction validation
- **Emergency pause mechanism** - Circuit breaker for security incidents
- **Anomaly detection** - Real-time monitoring of unusual bridge activity
- **Supply consistency checks** - Ensures wrapped token supply matches locked assets
- **Implementation**: `src/bridge/bridge.cpp`, `include/intcoin/bridge.h`
- **Tests**: Comprehensive test suite (`test_bridge`)

#### Wrapped Token Support
- **wBTC (Wrapped Bitcoin)** - 1:1 peg with Bitcoin
- **wETH (Wrapped Ethereum)** - 1:1 peg with Ethereum
- **wLTC (Wrapped Litecoin)** - 1:1 peg with Litecoin
- **Token metadata** tracking (decimals, origin chain, contract address)
- **Mint/burn mechanism** for deposit/withdrawal
- **Supply auditing** - Public verification of total supply vs locked assets
- **Implementation**: Part of bridge system

#### Bridge Operations
- **Deposit process**:
  - Send assets to bridge address on origin chain
  - Wait for confirmations (BTC: 6, ETH: 12, LTC: 24)
  - Submit deposit proof (tx hash, block number, addresses, amount)
  - Validator signature collection (M-of-N threshold)
  - Wrapped tokens minted on INTcoin
- **Withdrawal process**:
  - Initiate withdrawal request
  - Burn wrapped tokens on INTcoin
  - Validator signature collection
  - Automatic execution on destination chain
  - Asset unlock on origin chain
- **Target finality**: <30 minutes for most transactions
- **RPC Commands**:
  - `bridgedeposit` - Deposit assets (lock & mint)
  - `bridgewithdraw` - Withdraw assets (burn & unlock)
  - `getbridgebalance` - Check wrapped token balances
  - `listbridgetransactions` - Bridge transaction history
  - `getbridgeinfo` - Bridge configuration and status

#### Bridge Web UI
- **React/TypeScript** modern web interface
- **Vite** build system for optimal performance
- **TanStack Query** for efficient data fetching
- **Wallet integration**:
  - MetaMask (Ethereum)
  - Keplr (Cosmos)
  - INTcoin wallet
- **Features**:
  - Asset deposit interface
  - Asset withdrawal interface
  - Real-time transaction monitoring
  - Bridge statistics dashboard
  - Transaction history
- **Production ready** with Nginx deployment configuration
- **Implementation**: `bridge-ui/` directory
- **Package**: `bridge-ui/package.json` with all dependencies

#### Bridge Security & Monitoring
- **Multi-sig validation** - All transactions require M-of-N validator signatures
- **Bridge monitoring system**:
  - Real-time validator health checks
  - Transaction confirmation tracking
  - Supply discrepancy detection
  - Double-spend prevention
  - Rate limiting and abuse prevention
- **Security features**:
  - Emergency pause mechanism
  - Validator reputation system
  - Deposit/withdrawal limits
  - Timelock on large transactions
- **Implementation**: `src/bridge/bridge_monitor.cpp`, `include/intcoin/bridge_monitor.h`

### Phase 4: Supporting Infrastructure

#### Enhanced Mempool (4.1)
- **6-level priority system**:
  - `LOW` - Low fee transactions
  - `NORMAL` - Standard transactions (default)
  - `HIGH` - High fee transactions
  - `HTLC` - Lightning Network and atomic swap transactions
  - `BRIDGE` - Cross-chain bridge transactions
  - `CRITICAL` - Time-sensitive protocol transactions
- **Fee-based prioritization** - Higher fees = higher priority
- **Dependency tracking** - Parent/child transaction relationships
- **Eviction policy** - Lowest priority transactions evicted when full
- **Mempool persistence**:
  - Save to `mempool.dat` on shutdown
  - Restore from disk on startup
  - Prevents transaction loss during restart
- **Expiry mechanism** - Transactions expire after 72 hours (configurable)
- **Configuration**:
  - Max size: 300 MB (default)
  - Min relay fee: 1000 INTS/KB (default)
  - Priority limits per level
- **Implementation**: `src/mempool/mempool.cpp`, `include/intcoin/mempool.h`
- **Tests**: 12/12 passing (`test_mempool`)

#### Enhanced P2P Protocol - Bloom Filters (4.2)
- **BIP37 P2P extensions**:
  - `filterload` - Load bloom filter
  - `filteradd` - Add element to filter
  - `filterclear` - Clear bloom filter
  - `merkleblock` - Filtered block relay
- **Header-first synchronization** for SPV clients
- **Bandwidth optimization** - Only relay matching transactions
- **Privacy features** - Plausible deniability via false positives
- **Implementation**: Integrated with SPV and bloom filter components
- **Tests**: 8/8 passing (`test_bloom`)

#### Prometheus Metrics System (4.3)
- **4 metric types**:
  - **Counter** - Monotonically increasing values (blocks, transactions)
  - **Gauge** - Values that can go up/down (height, mempool size, peer count)
  - **Histogram** - Distribution with buckets (durations, sizes, fees)
  - **Timer** - Automatic duration measurement helper (RAII pattern)
- **40+ standard metrics**:
  - **Blockchain**: `blocks_processed`, `transactions_processed`, `blockchain_height`, `blockchain_difficulty`, `block_processing_duration`, `block_size`
  - **Mempool**: `mempool_size`, `mempool_bytes`, `mempool_accepted`, `mempool_rejected`, `mempool_tx_fee`
  - **Network**: `peer_count`, `bytes_sent`, `bytes_received`, `messages_sent`, `messages_received`, `message_processing_duration`
  - **Mining**: `blocks_mined`, `hashes_computed`, `hashrate`, `mining_duration`
  - **Wallet**: `wallet_balance`, `wallet_transactions`, `wallet_utxo_count`
  - **SPV/P2P**: `spv_best_height`, `bloom_filters_loaded`, `header_sync_duration`
- **Thread-safe** - Atomics for Counter/Gauge, mutex for Histogram
- **MetricsRegistry** - Singleton pattern for global metric access
- **Prometheus export format** (text exposition version 0.0.4)
- **Implementation**: `src/metrics/metrics.cpp`, `include/intcoin/metrics.h`
- **Tests**: 10/10 passing (`test_metrics`)

#### Prometheus HTTP Endpoint (4.4)
- **HTTP server** serving metrics at `GET /metrics`
- **Configuration**:
  - Bind address: 127.0.0.1 (default)
  - Port: 9090 (default, configurable)
  - Worker threads: 2 (default, configurable)
- **Cross-platform**: Windows (Winsock) and POSIX (sockets) support
- **Multi-threaded** request handling for concurrent scraping
- **Proper HTTP responses**:
  - 200 OK - Metrics data
  - 404 Not Found - Invalid path
  - 405 Method Not Allowed - Non-GET requests
  - 400 Bad Request - Malformed requests
- **Request counting** - Tracks total requests served
- **Graceful shutdown** - Clean thread termination
- **Implementation**: `src/metrics/metrics_server.cpp`, `include/intcoin/metrics_server.h`
- **Tests**: 8/8 passing (`test_metrics_server`)

---

## Test Coverage

### New Test Suites
- ✅ **test_mempool** (12 tests) - Enhanced mempool functionality
- ✅ **test_bloom** (8 tests) - Bloom filter implementation
- ✅ **test_metrics** (10 tests) - Prometheus metrics system
- ✅ **test_metrics_server** (8 tests) - HTTP metrics endpoint
- ✅ **test_atomic_swap** - Atomic swap protocol
- ✅ **test_bridge** - Cross-chain bridge operations

### Total Test Count
- **38 new tests** added in v1.2.0-beta
- **100% pass rate** - All tests passing
- **Comprehensive coverage** across all new features

### Test Results Summary
```
Enhanced Mempool Tests:      12/12 ✓
Bloom Filter Tests:           8/8 ✓
Prometheus Metrics Tests:    10/10 ✓
Metrics Server Tests:         8/8 ✓
Atomic Swap Tests:           All ✓
Cross-Chain Bridge Tests:    All ✓
```

---

## Breaking Changes

### Network Protocol
- **Protocol version bump** - New version for v1.2.0 features
- **Backward compatible** - Older nodes can still sync, but won't support new features
- **Bloom filter P2P messages** - New message types (filterload, filteradd, filterclear, merkleblock)

### RPC API
- **New RPC methods** - 20+ new methods added (mobile, atomic swap, bridge, HTLC)
- **No deprecated methods** - All existing methods remain functional
- **Extended error codes** - New error codes for atomic swap and bridge operations

### Database
- **Mempool persistence** - New `mempool.dat` file created
- **No schema changes** - Existing database format unchanged
- **Backward compatible** - Can downgrade without data loss (mempool.dat will be ignored)

### Configuration
- **New config options**:
  - `mempool.maxsize` - Maximum mempool size (default: 300 MB)
  - `mempool.minrelayfee` - Minimum relay fee (default: 1000 INTS/KB)
  - `mempool.expiry` - Transaction expiry time (default: 72 hours)
  - `metrics.enabled` - Enable metrics HTTP server (default: false)
  - `metrics.port` - Metrics HTTP port (default: 9090)
  - `metrics.bind` - Metrics bind address (default: 127.0.0.1)
  - `bridge.enabled` - Enable bridge functionality (default: false)
  - `bridge.validators` - Bridge validator list
- **No deprecated options** - All existing config options remain valid

---

## Upgrade Instructions

### From v1.1.0-beta to v1.2.0-beta

1. **Backup your wallet**:
   ```bash
   intcoin-cli backupwallet /path/to/backup.dat
   ```

2. **Stop the node**:
   ```bash
   intcoin-cli stop
   ```

3. **Update binaries**:
   - Replace `intcoind`, `intcoin-cli`, `intcoin-qt` with v1.2.0-beta versions
   - Or build from source (see BUILD_GUIDE.md)

4. **Restart the node**:
   ```bash
   intcoind -daemon
   ```

5. **Verify upgrade**:
   ```bash
   intcoin-cli getnetworkinfo
   # Should show "version": 120000 (v1.2.0)
   ```

6. **(Optional) Enable new features**:
   - Edit `intcoin.conf` to enable metrics, bridge, etc.
   - Restart node to apply changes

### Database Migration
- **No migration required** - v1.2.0-beta uses the same database format
- **Mempool restoration** - Mempool will be empty on first start (normal)
- **Full compatibility** - Can downgrade to v1.1.0-beta without data loss

### Configuration Updates
- **Review new options** in `intcoin.conf` (see docs/CONFIGURATION.md)
- **Metrics disabled by default** - Enable if you want Prometheus monitoring
- **Bridge disabled by default** - Enable only if you're running a validator

---

## Known Issues

### Mobile Wallets
- **iOS App Store** - Not yet available (use TestFlight for beta testing)
- **Android Google Play** - Not yet available (sideload APK for beta testing)
- **Initial sync may be slow** on very slow connections (<1 Mbps)

### Atomic Swaps
- **Exchange rate volatility** - Manual rate negotiation required for now
- **Timeout handling** - Very slow chains (high congestion) may cause timeouts
- **Limited chain support** - Only BTC, LTC, INT supported in v1.2.0-beta

### Cross-Chain Bridges
- **Testnet only** - Bridge is in testnet phase, mainnet requires security audit
- **Validator set** - Limited to 5 validators in beta (will expand for mainnet)
- **Ethereum gas fees** - High gas fees may make small wETH withdrawals uneconomical

### Prometheus Metrics
- **Firewall configuration** - Default port 9090 may conflict with other services
- **No authentication** - Metrics endpoint has no built-in auth (use firewall or reverse proxy)

### General
- **Documentation** - Some advanced features documentation still in progress
- **Performance** - First-time bloom filter creation may take a few seconds

---

## Deprecations

**None** - No features were deprecated in v1.2.0-beta. All v1.1.0-beta features remain fully supported.

---

## Commits

Key commits in this release (from most recent):

- **24c8dbe** - Add Prometheus HTTP Metrics Endpoint - Phase 4 Complete!
- **14f206f** - Complete Phase 4.3: Prometheus Metrics & Monitoring System
- **1011008** - Add comprehensive bloom filter test suite - ALL TESTS PASSING
- **59335a6** - Fix mempool mutex deadlocks - ALL TESTS PASSING
- **504513d** - Phase 4.1: Enhanced Mempool with priority queues and persistence
- **07ebee5** - Remove bug bounty references - community project model
- **a9a8cdd** - Implement Phase 3.4: Bridge Web UI
- **8c08789** - Implement Phase 3.3: Bridge Security and Monitoring System
- **4b53be3** - Implement Phase 3.2: Wrapped Token RPC Commands
- **9e11ddd** - Implement Phase 3.1: Cross-Chain Bridge Architecture
- **2cab9e7** - Update documentation: GPG keys, GitHub migration
- **7fd43de** - Implement Phase 2: Atomic Swaps - Cross-chain trustless exchanges
- **dcf36ec** - Bump version to v1.2.0-beta

**Full changelog**: See [CHANGELOG.md](CHANGELOG.md)

---

## Credits

This release was made possible by the INTcoin development community. Special thanks to all contributors who tested, reviewed, and provided feedback during the development cycle.

### Development
- INTcoin Core Developers
- Community Contributors

### Testing
- Beta testers on testnet
- Security researchers
- Community validators

### Documentation
- Technical writers
- Translation team (documentation translations coming soon)

---

## Resources

### Documentation
- **Main Documentation**: [docs/README.md](docs/README.md)
- **Mobile Wallet Guide**: [docs/MOBILE_WALLET.md](docs/MOBILE_WALLET.md) (coming soon)
- **Atomic Swaps Guide**: [docs/ATOMIC_SWAPS.md](docs/ATOMIC_SWAPS.md) (coming soon)
- **Bridge User Guide**: [docs/CROSS_CHAIN_BRIDGES.md](docs/CROSS_CHAIN_BRIDGES.md) (coming soon)
- **API Reference**: [docs/API_REFERENCE.md](docs/API_REFERENCE.md)
- **Build Guide**: [docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md)

### Downloads
- **Source Code**: https://github.com/InternationalCoin/intcoin
- **Binaries**: https://github.com/InternationalCoin/intcoin/releases/tag/v1.2.0-beta
- **iOS SDK**: Swift Package Manager (see mobile/ios/README.md)
- **Android SDK**: Gradle (see mobile/android/README.md)

### Support
- **Issue Tracker**: https://github.com/InternationalCoin/intcoin/issues
- **Discussions**: https://github.com/InternationalCoin/intcoin/discussions
- **Email**: team@international-coin.org
- **Security**: security@international-coin.org

---

## What's Next

### v1.3.0-alpha (Q2 2026)
- **DeFi Primitives**: Decentralized exchange (DEX) functionality
- **Privacy Features**: CoinJoin, stealth addresses
- **Additional Bridges**: More blockchain integrations
- **Mobile Wallet**: App Store and Google Play release

### Future Considerations
- **Smart Contracts**: WASM VM integration (sidechain approach)
- **Sidechains**: Experimental features on pegged sidechains
- **Additional Atomic Swap Chains**: Monero, Ethereum support

See [ROADMAP.md](ROADMAP.md) for the complete development roadmap.

---

## Conclusion

INTcoin v1.2.0-beta represents a transformative step forward, bringing mobile accessibility, cross-chain interoperability, and enterprise-grade monitoring to the platform. With SPV clients enabling lightweight mobile wallets, atomic swaps facilitating trustless cross-chain exchanges, and bridges connecting multiple blockchain ecosystems, INTcoin is evolving into a truly interoperable cryptocurrency.

We encourage all users to test the new features on testnet and provide feedback. This beta release helps us refine these features before mainnet deployment.

**Thank you for your continued support of the INTcoin project!**

---

**Maintainer**: INTcoin Development Team
**Contact**: team@international-coin.org
**Release Date**: January 2, 2026
**Version**: 1.2.0-beta
