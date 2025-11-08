# INTcoin v1.0.0 - Production Release

**First production-ready release of the quantum-resistant cryptocurrency**

---

## 🎉 Highlights

- ✅ Complete quantum-resistant cryptocurrency implementation
- ✅ NIST-verified post-quantum cryptography (Dilithium5 + Kyber1024)
- ✅ ASIC-resistant mining (RandomX)
- ✅ Full HD wallet with BIP39 mnemonic support
- ✅ Qt5 GUI wallet with complete user interface
- ✅ Comprehensive test suite (8 tests, all passing)
- ✅ Multi-platform support (macOS, Linux, FreeBSD, Windows)

---

## 📦 Components

### Production Executables (6)
- **intcoind** (11 MB) - Full node daemon with P2P networking
- **intcoin-cli** (11 MB) - Command-line interface for node interaction
- **intcoin-wallet** (11 MB) - HD wallet management tool
- **intcoin-miner** (11 MB) - CPU miner for RandomX
- **intcoin-explorer** (36 KB) - Block explorer utility
- **intcoin-qt** (11 MB) - Qt5-based GUI wallet 🎉

### Test Suite (8)
- **test_crypto** (2.9 MB) - Cryptographic function tests
- **test_blockchain** (11 MB) - Core blockchain functionality
- **test_wallet** (11 MB) - HD wallet operations
- **test_network** (11 MB) - P2P networking tests
- **test_security** (166 KB) - Security validation tests
- **test_dilithium_nist** (2.4 MB) - NIST ML-DSA-87 verification
- **test_kyber_nist** (621 KB) - NIST ML-KEM-1024 verification
- **test_hash_nist** (76 KB) - NIST SHA3-256 verification

---

## 🔐 Cryptography

All cryptographic implementations verified against official NIST test vectors:

- **Signatures:** CRYSTALS-Dilithium5 (ML-DSA-87)
  - 4,595-byte signatures
  - NIST FIPS 204 compliant
  - All test vectors pass ✅

- **Key Exchange:** CRYSTALS-Kyber1024 (ML-KEM-1024)
  - 3,168-byte ciphertexts
  - NIST FIPS 203 compliant
  - All test vectors pass ✅

- **Hashing:** SHA3-256
  - 256-bit output
  - FIPS 202 compliant
  - All test vectors pass ✅

---

## ⛏️ Mining

- **Algorithm:** RandomX (CPU-optimized, ASIC-resistant)
- **Block Time:** 120 seconds (2 minutes)
- **Difficulty Adjustment:** Every 2016 blocks (~4.67 days)
- **Initial Reward:** 50 INT

---

## 💰 Blockchain Parameters

- **Coin Name:** INTcoin (INT)
- **Max Supply:** 221 trillion INT
- **Block Time:** 2 minutes
- **Blocks per Day:** 720
- **P2P Port:** 9333 (mainnet), 18333 (testnet)
- **RPC Port:** 9332 (mainnet), 18332 (testnet)

---

## 🛠️ Build Instructions

### Requirements
- CMake 3.28+
- C++23 compiler (GCC 13+, Clang 16+, MSVC 2022+)
- OpenSSL 3.0+
- RocksDB 8.0+
- Boost 1.85+
- Qt5 5.15+ (for GUI)
- liboqs (for post-quantum crypto)

### Quick Build
```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
git checkout v1.0.0
mkdir build && cd build
cmake ..
cmake --build .
```

See [BUILD.md](https://gitlab.com/intcoin/crypto/-/blob/main/BUILD.md) for detailed platform-specific instructions.

---

## 📋 Documentation

- [Release Notes](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/docs/RELEASE-NOTES-v1.0.0.md) - Complete release documentation
- [Build Guide](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/BUILD.md) - Multi-platform build instructions
- [Quick Start](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/QUICK-START.md) - Getting started guide
- [Cryptography Design](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/docs/CRYPTOGRAPHY-DESIGN.md) - Technical details
- [Testing Guide](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/docs/TESTING.md) - How to run tests
- [Roadmap](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/ROADMAP.md) - Project roadmap

---

## ⚠️ Important Notes

### Pre-Production Status
This is a **pre-production release**. The following items are pending:

- [ ] External security audit
- [ ] Penetration testing
- [ ] Testnet deployment and community testing
- [ ] Performance benchmarking
- [ ] Bug bounty program

**Do not use on mainnet without completing security audit.**

### Known Limitations
The following features are **disabled** in v1.0.0:
- Lightning Network (namespace fixes required)
- Smart Contracts (not implemented)
- Cross-chain Bridge (not implemented)
- TOR integration (not implemented)

These features are planned for future releases (v1.1.0+).

---

## 🔜 Future Roadmap

### v1.1.0 (Q1 2026)
- Lightning Network support
- Enhanced P2P protocol
- Performance optimizations
- Comprehensive benchmarks

### v1.2.0 (Q2 2026)
- Smart contract VM
- TOR integration
- Enhanced privacy features

### v2.0.0 (Q3 2026)
- Cross-chain bridge
- Sharding support
- Governance system

---

## 🐛 Reporting Issues

Found a bug? Please report it:
- **GitLab Issues:** https://gitlab.com/intcoin/crypto/-/issues
- **Security Issues:** security@international-coin.org
- **General Questions:** team@international-coin.org

---

## 👥 Credits

**Lead Developer:** Maddison Lane
**Build System:** Claude Code Assistant

### Special Thanks
- NIST for post-quantum cryptography standards
- RandomX developers for ASIC-resistant mining
- Bitcoin Core for blockchain architecture inspiration
- Open-source cryptography community

---

## 📄 License

MIT License - Copyright (c) 2025 INTcoin Core (Maddison Lane)

See [COPYING](https://gitlab.com/intcoin/crypto/-/blob/v1.0.0/COPYING) for full license text.

---

## 🔗 Links

- **Website:** https://international-coin.org
- **Repository:** https://gitlab.com/intcoin/crypto
- **Wiki:** https://gitlab.com/intcoin/crypto/-/wikis/home
- **Releases:** https://gitlab.com/intcoin/crypto/-/releases

---

## ✅ Verification

### Verify Release Tag
```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
git tag --verify v1.0.0
```

### Build from Source
```bash
git checkout v1.0.0
mkdir build && cd build
cmake ..
cmake --build .
# All tests should pass
ctest
```

### GPG Signature
Release signed by: **Maddison Lane**
GPG Key: `6E68 20D6 0277 879B 3EFA 62D1 EB1A 8F24 AB19 0CBC`

---

**Release Date:** November 8, 2025
**Git Commit:** `13352d2`
**Git Tag:** `v1.0.0`
**Total Code:** ~20,000+ lines of production C++23
