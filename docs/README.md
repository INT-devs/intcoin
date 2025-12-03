# INTcoin Technical Documentation

**Version**: 1.0.0-alpha
**Last Updated**: December 2, 2025
**Status**: 90% Complete

This directory contains comprehensive technical documentation for INTcoin developers and contributors.

---

## 📚 Documentation Index

### Core Documentation
- [**README**](../README.md) - Project overview and quick start
- [**ARCHITECTURE**](ARCHITECTURE.md) - System architecture overview
- [**BUILD_STATUS**](../BUILD_STATUS.md) - Current build status and progress
- [**DEVELOPMENT_PLAN**](../DEVELOPMENT-PLAN.md) - Complete development roadmap

### Component Documentation
1. [**Cryptography**](CRYPTOGRAPHY.md) - Post-quantum cryptographic implementation
2. [**Consensus**](CONSENSUS.md) - RandomX PoW and Digishield V3 difficulty adjustment
3. [**Address Encoding**](ADDRESS_ENCODING.md) - Bech32 address format
4. [**Transactions**](TRANSACTIONS.md) - Transaction structure and validation
5. [**Blocks**](BLOCKS.md) - Block structure and Merkle trees
6. [**Blockchain**](BLOCKCHAIN.md) - Blockchain state management
7. [**Network**](NETWORK.md) - P2P networking protocol
8. [**Storage**](STORAGE.md) - RocksDB persistence layer
9. [**Wallet**](WALLET.md) - HD wallet with BIP32/39/44 and Dilithium3 ✅ **NEW**
10. [**RPC**](RPC.md) - JSON-RPC API documentation

### Build & Testing
- [**Building from Source**](BUILDING.md) - Complete build instructions
- [**Testing Guide**](TESTING.md) - Running and writing tests
- [**Copyright Script**](COPYRIGHT-SCRIPT-README.md) - Copyright header management

### Advanced Topics
- [**Lightning Network**](LIGHTNING.md) - Layer 2 scaling implementation
- [**Smart Contracts**](SMART_CONTRACTS.md) - Smart contract infrastructure
- [**Cross-Chain Bridges**](CROSS_CHAIN.md) - Cross-chain interoperability

---

## 🚀 Quick Links

### For Developers
- [API Reference](API_REFERENCE.md)
- [Code Style Guide](CODE_STYLE.md)
- [Contributing Guidelines](CONTRIBUTING.md)
- [Development Setup](DEVELOPMENT_SETUP.md)

### For Users
- [Wiki - User Guides](../wiki/Users/Home.md)
- [Installation Guide](../wiki/Users/Installation.md)
- [Wallet Guide](../wiki/Users/Wallet-Guide.md)
- [FAQ](../wiki/Users/FAQ.md)

---

## 📊 Project Status

### ✅ Completed (91%)
- ✅ Project structure and build system (CMake 4.2.0, C++23)
- ✅ Post-quantum cryptography (Dilithium3 + Kyber768 + SHA3-256)
- ✅ RandomX Proof-of-Work integration
- ✅ Bech32 address encoding ('int1' prefix)
- ✅ Digishield V3 difficulty adjustment
- ✅ Transaction/block serialization
- ✅ RocksDB storage layer (UTXO model, mempool)
- ✅ P2P networking protocol (1,200+ lines)
- ✅ RPC server (32+ methods, zero external JSON dependencies)
- ✅ Blockchain core (~15,000 lines)
- ✅ Comprehensive test suites (10/10 passing - 100%)
- ✅ **HD wallet backend (BIP32/39/44 adapted for Dilithium3)** ✨ **COMPLETE**
- ✅ **Wallet database (RocksDB persistence)** ✨ **COMPLETE**
- ✅ **UTXO scanning and balance tracking** ✨ **COMPLETE**
- ✅ **Transaction creation and signing** ✨ **COMPLETE**
- ✅ **Transaction history indexing** ✨ **COMPLETE**
- ✅ **intcoind daemon (blockchain node)** ✨ **NEW**

### 🔄 In Progress (0% - Phase 6 Starting)
- 🔄 Documentation completion

### ⏳ Planned (10% remaining)
- ⏳ Desktop wallet (Qt GUI)
- ⏳ CPU miner (RandomX)
- ⏳ Block explorer
- ⏳ Wallet encryption (Kyber768/AES-256) - optional enhancement
- ⏳ Additional integration tests

---

## 🔐 Security

- [Security Policy](SECURITY.md)
- [Vulnerability Reporting](VULNERABILITY_REPORTING.md)

---

## 📞 Contact & Support

- **Website**: https://international-coin.org
- **Email**: team@international-coin.org
- **GitLab**: https://gitlab.com/intcoin/crypto
- **Discord**: https://discord.gg/intcoin
- **Telegram**: https://t.me/intcoin

---

**Built with ❤️ for the quantum era**
