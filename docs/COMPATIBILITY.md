# INTcoin Compatibility Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

This document outlines compatibility requirements, version dependencies, and interoperability for INTcoin v1.2.0-beta.

---

## Table of Contents

- [Version Compatibility](#version-compatibility)
- [Operating System Support](#operating-system-support)
- [Dependency Requirements](#dependency-requirements)
- [Network Protocol](#network-protocol)
- [Wallet Compatibility](#wallet-compatibility)
- [RPC API Compatibility](#rpc-api-compatibility)
- [Mobile Platform Support](#mobile-platform-support)
- [Known Issues](#known-issues)

---

## Version Compatibility

### Node Versions

| Your Version | Can Connect To | Data Format Compatible |
|--------------|----------------|------------------------|
| **v1.2.0-beta** | v1.0.0-alpha, v1.1.x, v1.2.x | ✅ Yes (all versions) |
| **v1.0.0-alpha** | v1.0.0-alpha, v1.1.x, v1.2.x | ✅ Yes |

**Network Protocol Version**: 70015 (unchanged from v1.0.0)

### P2P Protocol Compatibility

**Fully Compatible**:
- v1.2.0-beta nodes can sync from v1.0.0-alpha nodes
- v1.0.0-alpha nodes can sync from v1.2.0-beta nodes
- Mixed-version networks are fully supported

**New Features** (backward compatible):
- SPV nodes (v1.2.0+) can connect to any full node
- Bloom filters supported on v1.2.0+ nodes
- Legacy nodes ignore unknown messages gracefully

### Blockchain Data

**Database Compatibility**:

| Component | v1.0.0 → v1.2.0 | v1.2.0 → v1.0.0 |
|-----------|-----------------|-----------------|
| **Blockchain DB** | ✅ Compatible (no migration) | ✅ Compatible |
| **Wallet** | ✅ Compatible (no migration) | ✅ Compatible |
| **Mempool** | ✅ Auto-migrated | ⚠️ Cleared on downgrade |
| **Peer DB** | ✅ Compatible | ✅ Compatible |

---

## Operating System Support

### Desktop/Server Platforms

| OS | Architecture | v1.2.0 Support | Notes |
|----|-------------|----------------|-------|
| **Ubuntu 24.04 LTS** | x86_64, ARM64 | ✅ Fully Supported | Recommended |
| **Ubuntu 22.04 LTS** | x86_64, ARM64 | ❌ Deprecated | Outdated toolchain |
| **Debian 11 (Bullseye)** | x86_64, ARM64 | ✅ Supported | |
| **Debian 12 (Bookworm)** | x86_64, ARM64 | ✅ Fully Supported | Recommended |
| **Fedora 38+** | x86_64, ARM64 | ✅ Supported | |
| **Arch Linux** | x86_64, ARM64 | ✅ Supported | Rolling release |
| **macOS 12 (Monterey)** | Intel, Apple Silicon | ✅ Supported | |
| **macOS 13 (Ventura)** | Intel, Apple Silicon | ✅ Fully Supported | |
| **macOS 14 (Sonoma)** | Intel, Apple Silicon | ✅ Fully Supported | Recommended |
| **macOS 15 (Sequoia)** | Intel, Apple Silicon | ✅ Fully Supported | Recommended |
| **FreeBSD 13** | x86_64 | ✅ Supported | |
| **FreeBSD 14** | x86_64 | ✅ Fully Supported | |
| **Windows 10 22H2** | x86_64 | ✅ Supported | Via WSL2 recommended |
| **Windows 11** | x86_64, ARM64 | ✅ Supported | Native or WSL2 |

**End of Life** (not supported):
- Ubuntu 22.04 and earlier
- Debian 10 and earlier
- macOS 11 (Big Sur) and earlier
- FreeBSD 12 and earlier
- Windows 7, 8, 8.1, 10 (pre-22H2)

### Mobile Platforms

| Platform | Minimum Version | Recommended | Notes |
|----------|----------------|-------------|-------|
| **iOS** | 15.0 | iOS 17+ | Native SPV wallet |
| **Android** | 8.0 (API 26) | Android 13+ (API 33) | Native SPV wallet |

---

## Dependency Requirements

### Required Dependencies

| Library | Minimum Version | Recommended | Purpose |
|---------|----------------|-------------|---------|
| **CMake** | 3.28.0 | Latest stable | Build system |
| **C++ Compiler** | - | - | See below |
| **OpenSSL** | 3.5.4 | Latest 3.x | SHA3-256 hashing |
| **liboqs** | 0.12.0 | 0.15.0+ | Post-quantum crypto |
| **RandomX** | 1.2.1 | Latest | ASIC-resistant PoW |

### Compiler Requirements

| Compiler | Minimum Version | Recommended | C++ Standard |
|----------|----------------|-------------|--------------|
| **GCC** | 11.0 | 13.0+ | C++23 |
| **Clang** | 13.0 | 16.0+ | C++23 |
| **MSVC** | VS 2022 17.0 | Latest | C++23 |
| **Apple Clang** | 14.0 | 15.0+ | C++23 |

**C++23 Features Used**:
- `std::expected` (error handling)
- `std::print` (formatted output)
- `import std` (modules - optional)
- Deducing `this` (CRTP simplification)

### Optional Dependencies

| Library | Minimum Version | Purpose | Required For |
|---------|----------------|---------|--------------|
| **Boost** | 1.89.0 | Utilities, threading | Core features |
| **RocksDB** | 10.7 | Blockchain database | Full node |
| **Qt6** | 6.8 | Desktop GUI | Qt wallet |
| **ZeroMQ** | 4.3.0 | Messaging | ZMQ notifications |
| **libevent** | 2.1.0 | Networking | HTTP RPC |
| **libtor** | Latest | Tor integration | Privacy mode |
| **libi2pd** | Latest | I2P integration | Privacy mode |

### Mobile SDK Dependencies

**iOS**:
- Xcode 15.0+
- Swift 5.9+
- iOS Deployment Target: 15.0+

**Android**:
- Android Studio Hedgehog (2023.1.1)+
- Kotlin 1.9+
- Gradle 8.5+
- Android SDK 34+ (API Level 34)
- NDK 26+ (for native crypto libraries)

---

## Network Protocol

### Protocol Version

**Current**: 70015 (unchanged since v1.0.0)

### Message Compatibility

| Message Type | v1.0.0 | v1.2.0 | Backward Compatible |
|--------------|--------|--------|---------------------|
| `version` | ✅ | ✅ | ✅ Yes |
| `verack` | ✅ | ✅ | ✅ Yes |
| `addr` | ✅ | ✅ | ✅ Yes |
| `inv` | ✅ | ✅ | ✅ Yes |
| `getdata` | ✅ | ✅ | ✅ Yes |
| `block` | ✅ | ✅ | ✅ Yes |
| `tx` | ✅ | ✅ | ✅ Yes |
| `getblocks` | ✅ | ✅ | ✅ Yes |
| `getheaders` | ✅ | ✅ | ✅ Yes |
| `headers` | ✅ | ✅ | ✅ Yes |
| `filterload` | ❌ | ✅ | ✅ Yes (ignored by v1.0.0) |
| `filteradd` | ❌ | ✅ | ✅ Yes (ignored by v1.0.0) |
| `filterclear` | ❌ | ✅ | ✅ Yes (ignored by v1.0.0) |
| `merkleblock` | ❌ | ✅ | ✅ Yes (ignored by v1.0.0) |

**New in v1.2.0**:
- Bloom filter messages (BIP37): `filterload`, `filteradd`, `filterclear`, `merkleblock`
- SPV nodes can request filtered blocks from full nodes
- Full nodes running v1.0.0 will reject bloom filter requests

---

## Wallet Compatibility

### Wallet Formats

| Wallet Type | v1.0.0 | v1.2.0 | Cross-Compatible |
|-------------|--------|--------|------------------|
| **wallet.dat** (Berkeley DB) | ✅ | ✅ | ✅ Yes |
| **HD Wallet (BIP32/39/44)** | ✅ | ✅ | ✅ Yes |
| **Watch-Only** | ✅ | ✅ | ✅ Yes |
| **Multi-Sig** | ✅ | ✅ | ✅ Yes |

### Seed Phrase Compatibility

**BIP39 Mnemonics**:
- ✅ 12-word seeds: Compatible across all versions
- ✅ 24-word seeds: Compatible across all versions
- ✅ Passphrase protection: Fully compatible

**Derivation Paths** (BIP44):
```
m/44'/2211'/0'/0/0  # Standard INTcoin path (unchanged)
```

### Address Formats

| Format | v1.0.0 | v1.2.0 | Example |
|--------|--------|--------|---------|
| **Bech32 (Mainnet)** | ✅ | ✅ | `INT1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh` |
| **Bech32 (Testnet)** | ✅ | ✅ | `TINT1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh` |

**No changes** - all addresses generated by v1.0.0 work in v1.2.0 and vice versa.

---

## RPC API Compatibility

### Versioning

**RPC API Version**: 1.2.0

### Backward Compatibility

**All v1.0.0 RPC methods** work unchanged in v1.2.0:
- ✅ No breaking changes
- ✅ Same parameter formats
- ✅ Same response formats

### New RPC Methods (v1.2.0)

**Enhanced Mempool** (3 new):
- `getmempoolinfo` (enhanced)
- `setmempoolpriority`
- `savemempool`

**Prometheus Metrics** (2 new):
- `getmetricsinfo`
- `getprometheusmetrics`

**Atomic Swaps** (5 new):
- `initiate_atomic_swap`
- `redeem_atomic_swap`
- `refund_atomic_swap`
- `list_atomic_swaps`
- `get_atomic_swap_status`

**Bridges** (4 new):
- `bridge_deposit`
- `bridge_withdraw`
- `bridge_status`
- `list_bridge_operations`

**SPV** (3 new):
- `loadbloomfilter`
- `getheaders`
- `getmerkleproof`

See: [RPC.md](RPC.md), [API_REFERENCE.md](API_REFERENCE.md)

---

## Mobile Platform Support

### iOS Compatibility

| iOS Version | Support Status | Notes |
|-------------|----------------|-------|
| **iOS 17** | ✅ Fully Supported | Recommended |
| **iOS 16** | ✅ Supported | |
| **iOS 15** | ✅ Supported | Minimum version |
| **iOS 14 and earlier** | ❌ Not Supported | |

**Device Requirements**:
- iPhone 7 or later
- iPad (5th generation) or later
- 64-bit architecture

**Swift Compatibility**:
- Swift 5.9+
- Objective-C bridging supported

### Android Compatibility

| Android Version | API Level | Support Status | Notes |
|----------------|-----------|----------------|-------|
| **Android 14** | 34 | ✅ Fully Supported | Recommended |
| **Android 13** | 33 | ✅ Fully Supported | Recommended |
| **Android 12** | 31-32 | ✅ Supported | |
| **Android 11** | 30 | ✅ Supported | |
| **Android 10** | 29 | ✅ Supported | |
| **Android 9** | 28 | ✅ Supported | |
| **Android 8.1** | 27 | ✅ Supported | |
| **Android 8.0** | 26 | ✅ Supported | Minimum version |
| **Android 7.1 and earlier** | ≤25 | ❌ Not Supported | |

**Device Requirements**:
- ARMv8 (64-bit) or x86_64 architecture
- 2 GB RAM minimum (4 GB recommended)

**Kotlin Compatibility**:
- Kotlin 1.9+
- Java 8+ (for Java interop)

---

## Known Issues

### Platform-Specific Issues

#### macOS Apple Silicon (M1/M2/M3)

**Status**: ✅ Fully Supported (native ARM64 builds)

**Known Issue**: Rosetta 2 not required

#### Windows Native Build

**Status**: ⚠️ Partial Support

**Known Issue**: Native MSVC build requires specific vcpkg versions
**Workaround**: Use WSL2 for best experience

#### FreeBSD

**Status**: ✅ Supported

**Known Issue**: Qt6 wallet requires manual Qt build on FreeBSD 13
**Workaround**: Use pre-built Qt6 from pkg on FreeBSD 14

### Feature Compatibility

#### Atomic Swaps

**Compatibility**:
- ✅ BTC: Fully tested
- ✅ LTC: Fully tested
- ⚠️ XMR: Experimental (use with caution)

#### Cross-Chain Bridges

**Compatibility**:
- ✅ ETH: Production ready
- ✅ BTC: Production ready
- ⚠️ BSC: Beta (thorough testing recommended)

#### Mobile SPV

**Known Limitation**: Mobile SPV wallets require at least one v1.2.0+ full node to connect to for bloom filter support. Connection to v1.0.0 nodes will work but without bloom filtering.

---

## Version Support Policy

### Long-Term Support (LTS)

| Version | Release Date | Support Until | Status |
|---------|--------------|---------------|--------|
| **v1.2.0-beta** | January 2026 | January 2027 | Current |
| **v1.0.0-alpha** | December 2025 | June 2026 | Legacy |

### Deprecation Policy

**Notice Period**: 6 months minimum before deprecating features

**Current Deprecations**: None

**Planned Deprecations**: None announced

---

## Testing Compatibility

### Test Suite Compatibility

| Test Suite | v1.0.0 | v1.2.0 | Compatible |
|------------|--------|--------|------------|
| Unit Tests | ✅ | ✅ | ✅ Yes |
| Integration Tests | ✅ | ✅ | ✅ Yes |
| RPC Tests | ✅ | ✅ | ✅ Yes (new tests added) |

**Test Coverage**:
- v1.0.0: 12 test suites
- v1.2.0: 17 test suites
- All v1.0.0 tests pass on v1.2.0

---

## Upgrade Recommendations

### For Node Operators

**Upgrade Path**: v1.0.0-alpha → v1.2.0-beta
**Difficulty**: Easy
**Downtime**: ~5 minutes
**Migration**: Automatic

See: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)

### For Exchanges

**Testing Period**: 2-4 weeks recommended
**RPC Changes**: Review new methods, all old methods work
**Wallet Format**: No changes

### For Developers

**API Changes**: 23 new RPC methods, 0 breaking changes
**SDK Updates**: New mobile SDKs (iOS, Android)
**Documentation**: Updated for all new features

---

## Resources

- [Migration Guide](MIGRATION_GUIDE.md)
- [Build Guide](BUILD_GUIDE.md)
- [RPC Documentation](RPC.md)
- [Mobile SDK](MOBILE_SDK.md)

---

**Questions?** Create an issue: https://github.com/INT-devs/intcoin/issues

**Last Updated**: January 2, 2026
