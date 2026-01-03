# INTcoin Core

**Version 1.2.0-beta** | **Release: January 2, 2026** | **License: MIT** | **Status: Production Beta**

A next-generation cryptocurrency built on post-quantum cryptography (Dilithium3) with full Lightning Network support, designed to secure value in the quantum computing era.

> **Note**: v1.2.0-beta includes mobile wallet infrastructure, atomic swaps, and cross-chain bridges. Built on v1.1.0-beta foundation.

---

## Overview

INTcoin is a fully-featured cryptocurrency implementing:

- **Post-Quantum Cryptography**: Dilithium3 digital signatures (NIST FIPS 204 standard)
- **Quantum-Resistant Hashing**: SHA3-256 (NIST FIPS 202)
- **CPU-Optimized Mining**: RandomX proof-of-work algorithm (ASIC-resistant)
- **Layer 2 Scaling**: Complete Lightning Network (BOLT #1-12 specification)
- **Privacy Features**: Tor/I2P integration for anonymous transactions
- **User-Friendly Wallets**: Qt desktop wallet with BIP39 mnemonic seeds

### Why INTcoin?

Traditional cryptocurrencies like Bitcoin use ECDSA cryptography, which will be broken by quantum computers. INTcoin uses Dilithium3, a NIST-standardized post-quantum signature scheme, ensuring your assets remain secure for decades to come.

---

## Key Features

### Core Blockchain

- **Consensus**: Proof-of-Work (RandomX algorithm)
- **Block Time**: 2 minutes
- **Total Supply**: 221,000,000,000,000 INT (221 Trillion)
- **Address Format**: Bech32 (int1 for mainnet, tint1 for testnet, lint1 for Lightning)
- **Transaction Model**: UTXO (Unspent Transaction Output)
- **Scripting**: Bitcoin-style transaction scripts

### Lightning Network

Full implementation of BOLT (Basis of Lightning Technology) specifications:

- **BOLT #1**: Base Protocol - Message framing, initialization, error handling
- **BOLT #2**: Peer Protocol - Channel lifecycle management
- **BOLT #3**: Transaction Formats - Commitment and HTLC transactions
- **BOLT #4**: Onion Routing - Multi-hop payment routing via Sphinx protocol
- **BOLT #5**: On-chain Handling - Penalty transactions and monitoring
- **BOLT #7**: P2P Discovery - Gossip protocol and channel announcements
- **BOLT #8**: Encrypted Transport - Noise_XK handshake, ChaCha20-Poly1305 encryption
- **BOLT #9**: Feature Flags - Protocol compatibility management
- **BOLT #10**: DNS Bootstrap - Lightning node discovery
- **BOLT #11**: Invoice Protocol - Bech32-encoded payment requests
- **BOLT #12**: Offers Protocol - Reusable payment requests

Features:
- Instant payments (sub-second confirmation)
- Minimal fees (fractions of a cent)
- Payment channels with HTLCs
- Multi-path payments (MPP)
- Watchtower services for channel security

### Security

- **Input Sanitization**: Protection against 90+ attack vectors
- **Fuzzing**: 500+ malicious input patterns tested
- **RPC Security**: HTTP Basic Authentication
- **Encryption**: AES-256-GCM wallet encryption with PBKDF2
- **Privacy**: Tor proxy support, stream isolation

### Mining

- **Algorithm**: RandomX (CPU-optimized, ASIC-resistant)
- **Solo Mining**: GetBlockTemplate RPC for direct mining
- **Pool Support**: Full Stratum v1 protocol with VarDiff
- **Pool Server**: Built-in mining pool server with HTTP API
- **Difficulty**: Digishield V3 adjustment (every block, 60-block window)

### Wallet

- **BIP39 Mnemonic**: 24-word recovery phrase
- **HD Wallet**: Hierarchical deterministic key derivation
- **Encryption**: AES-256-GCM with passphrase protection
- **UI**: Qt6 cross-platform desktop application
- **CLI Tools**: Command-line wallet management

---

## Quick Start

### Installation

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake git libboost-all-dev libssl-dev \
  librocksdb-dev qt6-base-dev libzmq3-dev libevent-dev
git clone https://github.com/INT-devs/intcoin.git intcoin
cd intcoin
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**macOS:**
```bash
brew install cmake boost openssl rocksdb qt@6 zeromq libevent
git clone https://github.com/INT-devs/intcoin.git intcoin
cd intcoin
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

**Windows:** See `docs/BUILDING.md` for detailed instructions.

### Running a Node

```bash
# Start daemon (mainnet)
./build/bin/intcoind

# Start daemon (testnet)
./build/bin/intcoind -testnet

# Start Qt wallet
./build/bin/intcoin-qt

# Start mining pool server
./build/bin/intcoin-pool-server --config=pool.conf
```

### Mining

```bash
# Solo mining (using GetBlockTemplate RPC)
./build/bin/intcoin-miner \
  --solo \
  --address=int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh6vqhvn \
  --rpc-user=user \
  --rpc-password=pass \
  --threads=4

# Pool mining (Stratum protocol)
./build/bin/intcoin-miner \
  --pool \
  --pool-host=pool.international-coin.org \
  --pool-port=3333 \
  --pool-user=worker1.miner01 \
  --pool-password=x \
  --threads=4

# Run a mining pool
./build/bin/intcoin-pool-server \
  --stratum-port=3333 \
  --http-port=8080 \
  --payout-threshold=1000000000
```

---

## Documentation

### Getting Started
- [Quick Start Guide](docs/getting-started/Quick-Start.md)
- [Installation Guide](docs/getting-started/Installation.md)
- [Building from Source](docs/BUILDING.md)

### User Guides
- [Wallet Guide](docs/WALLET.md)
- [Mining Guide](docs/MINING.md)
- [Pool Setup Guide](docs/POOL_SETUP.md)
- [Privacy Guide](docs/PRIVACY.md)

### Technical Documentation
- [Architecture](docs/ARCHITECTURE.md)
- [Consensus Rules](docs/CONSENSUS.md)
- [Cryptography](docs/CRYPTOGRAPHY.md)
- [RPC API](docs/RPC.md)
- [Security](docs/SECURITY_SANITIZATION.md)
- [Testing](docs/TESTING.md)

### Developer Resources
- [API Documentation](docs/RPC.md)
- [Contributing Guide](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)

---

## Project Status

**Development Progress: v1.2.0-beta Production Beta**

| Component | Status | Tests | Notes |
|-----------|--------|-------|-------|
| Core Blockchain | ✅ 100% | ✅ | UTXO, consensus, validation, reorg support complete |
| Cryptography | ✅ 100% | ✅ | Dilithium3 (signatures), Kyber768 (encryption), SHA3-256 |
| Mining | ✅ 100% | ✅ | RandomX PoW, solo + pool mining, GetBlockTemplate |
| Mining Pool Server | ✅ 100% | ✅ | Stratum protocol, PPLNS/PPS payouts, VarDiff |
| Lightning Network | ✅ 100% | ✅ | Full BOLT #1-13 implementation, watchtowers, routing |
| Qt Wallet | ✅ 100% | ✅ | Full-featured desktop wallet with Lightning support |
| RPC Server | ✅ 100% | ✅ | JSON-RPC 2.0, 70+ methods including mobile/swap/bridge |
| P2P Network | ✅ 100% | ✅ | Peer discovery, relay, sync, bloom filters (BIP37) |
| **Mobile Wallet** | ✅ 100% | ✅ 8/8 | **SPV client, iOS/Android SDKs, bloom filters** |
| **Atomic Swaps** | ✅ 100% | ✅ | **HTLC, BTC/LTC cross-chain trustless exchanges** |
| **Cross-Chain Bridges** | ✅ 100% | ✅ | **wBTC/wETH/wLTC, federated multi-sig, bridge UI** |
| **Enhanced Mempool** | ✅ 100% | ✅ 12/12 | **6-level priority, persistence, fee-based sorting** |
| **Prometheus Metrics** | ✅ 100% | ✅ 18/18 | **40+ metrics, HTTP endpoint at :9090/metrics** |
| Security | ✅ 100% | ✅ | Sanitization, fuzzing, post-quantum cryptography |
| Testing | ✅ 100% | ✅ | **51 test suites, all passing (38 new in v1.2.0)** |
| Documentation | ⏳ 95% | N/A | Core docs complete, v1.2.0 feature docs in progress |
| CI/CD | ✅ 100% | ✅ | Automated builds, tests, multi-platform support |

**v1.2.0-beta**: Mobile wallets, atomic swaps, cross-chain bridges, enhanced infrastructure
**Next Milestone**: v1.3.0-alpha (DeFi primitives, additional privacy features - Q2 2026)

---

## Architecture

```
intcoin/
├── src/
│   ├── blockchain/      # Block and transaction processing
│   ├── consensus/       # Consensus rules and validation
│   ├── crypto/          # Dilithium3, SHA3, RandomX
│   ├── lightning/       # Lightning Network (BOLT specs)
│   ├── mining/          # PoW mining and pool
│   ├── network/         # P2P networking
│   ├── wallet/          # HD wallet, key management
│   ├── rpc/             # JSON-RPC server
│   ├── qt/              # Desktop GUI wallet
│   ├── daemon/          # intcoind (full node)
│   └── cli/             # intcoin-cli (RPC client)
├── include/intcoin/     # Public headers
├── tests/               # Test suites
├── docs/                # Documentation
└── scripts/             # Build and deployment scripts
```

---

## Technical Specifications

### Blockchain

- **Algorithm**: RandomX (CPU-optimized Proof-of-Work)
- **Block Time**: 2 minutes (120 seconds)
- **Block Reward**: 105,113,636 INT (halving every 1,051,200 blocks / ~4 years)
- **Total Supply**: 221,000,000,000,000 INT (221 Trillion)
- **Difficulty Adjustment**: Digishield V3 (every block, 60 block averaging window)
- **Transaction Model**: UTXO
- **Script System**: Bitcoin-compatible opcodes
- **Address Encoding**: Bech32 (int1 mainnet / tint1 testnet / lint1 Lightning)

### Cryptography

- **Signatures**: Dilithium3 (NIST FIPS 204)
  - Public Key: 1952 bytes
  - Signature: 3293 bytes
  - Security Level: NIST Level 3 (equivalent to AES-192)
  - Quantum Resistance: Yes (lattice-based)

- **Hashing**: SHA3-256 (NIST FIPS 202)
  - Output: 256 bits (32 bytes)
  - Collision Resistance: 2^128 operations
  - Quantum Resistance: Yes (Grover's algorithm only provides quadratic speedup)

- **Mining**: RandomX
  - ASIC-Resistant: Yes
  - CPU-Optimized: Yes
  - Memory-Hard: Yes (2 GB recommended)

### Network

- **P2P Port**: 2210 (mainnet), 12210 (testnet)
- **RPC Port**: 2211 (mainnet), 12211 (testnet)
- **Lightning Port**: 2213 (mainnet), 12213 (testnet)
- **Protocol**: Custom binary protocol
- **Peer Discovery**: DNS seeds, hardcoded seeds
- **Max Connections**: 125 outbound, 125 inbound

---

## Development

### Building

**Prerequisites:**
- C++23 compiler (GCC 14+, Clang 19+, MSVC 2022+)
- CMake 3.28+
- Dependencies: Boost 1.87+, OpenSSL 3.5.4+, RocksDB 9.8+, Qt6.8+

**Build Steps:**
```bash
git clone https://github.com/INT-devs/intcoin.git intcoin
cd intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suite
./tests/test_crypto
./tests/test_blockchain
./tests/test_lightning
```

### Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Merge Request

---

## Community

Stay connected with the INTcoin community:

- **Website**: https://international-coin.org
- **Repository**: https://github.com/INT-devs/intcoin
- **Twitter/X**: https://x.com/INTcoin_team
- **Reddit**: https://www.reddit.com/r/INTcoin
- **Discord**: https://discord.gg/jCy3eNgx
- **Email**: team@international-coin.org

---

## License

INTcoin Core is released under the MIT License. See [LICENSE](LICENSE) for details.

---

## Acknowledgments

- **NIST**: Post-Quantum Cryptography standardization
- **RandomX Team**: ASIC-resistant mining algorithm
- **Lightning Labs**: Lightning Network specification (BOLT)
- **Bitcoin Core**: Reference implementation and best practices
- **Open Source Community**: Libraries and tools

---

## Security

Found a security issue? Please email security@international-coin.org or see [SECURITY.md](SECURITY.md) for our responsible disclosure policy.

**Community Project**: Everyone can participate in improving INTcoin security

---

## Roadmap

See [ROADMAP.md](ROADMAP.md) for our 10-year development plan.

**Upcoming Milestones:**
- Q1 2026: Mainnet Launch
- Q2 2026: Exchange Listings
- Q3 2026: Lightning Network Activation
- 2027: Smart Contracts
- 2028+: Mass Adoption

---

**Maintained by**: INTcoin Core Development Team
**Lead Developer**: Neil Adamson
**Status**: Active Development

*"Securing value in the quantum era"*

---

*Last Updated: January 2, 2026 - v1.2.0-beta Production Beta*
