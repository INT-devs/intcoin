# INTcoin Core

**Version 1.4.0-beta** | **Release: January 9, 2026** | **License: MIT** | **Status: Production Beta**

A next-generation cryptocurrency built on post-quantum cryptography (Dilithium3) with Lightning Network support and **EVM-compatible smart contracts**, designed to secure value in the quantum computing era.

> **Note**: v1.4.0-beta introduces **IntSC smart contracts** with quantum-resistant signatures, building on the mobile wallet infrastructure, atomic swaps, and cross-chain bridges from v1.2.0-beta.

---

## Overview

INTcoin is a fully-featured cryptocurrency implementing:

- **Post-Quantum Cryptography**: Dilithium3 digital signatures (NIST FIPS 204 standard)
- **Smart Contracts**: EVM-compatible IntSC VM with post-quantum cryptography ⭐ **NEW**
- **Quantum-Resistant Hashing**: SHA3-256 (NIST FIPS 202)
- **CPU-Optimized Mining**: RandomX proof-of-work algorithm (ASIC-resistant)
- **Layer 2 Scaling**: Complete Lightning Network (BOLT #1-12 specification)
- **Privacy Features**: Tor/I2P integration for anonymous transactions
- **User-Friendly Wallets**: Qt desktop wallet with BIP39 mnemonic seeds

### Why INTcoin?

Traditional cryptocurrencies like Bitcoin use ECDSA cryptography, which will be broken by quantum computers. **INTcoin uses Dilithium3**, a NIST-standardized post-quantum signature scheme, ensuring your assets remain secure for decades to come.

**World's First**: INTcoin is the first cryptocurrency to combine **EVM-compatible smart contracts** with **post-quantum cryptographic signatures**, making it the most future-proof platform for decentralized applications.

---

## Key Features

### Core Blockchain

- **Consensus**: Proof-of-Work (RandomX algorithm)
- **Block Time**: 2 minutes
- **Total Supply**: 221,000,000,000,000 INT (221 Trillion)
- **Address Format**: Bech32 (int1 for mainnet, tint1 for testnet, lint1 for Lightning)
- **Transaction Model**: UTXO with account-based contracts
- **Scripting**: Bitcoin-style transaction scripts + smart contracts

### Smart Contracts (v1.4.0) ⭐ **NEW**

**IntSC Virtual Machine** - EVM-compatible with quantum-resistant security:

- **VM**: Stack-based execution engine (60+ opcodes)
- **PQC Opcodes**: 4 quantum-resistant operations (DILITHIUM_VERIFY, KYBER_ENCAP, KYBER_DECAP, PQC_PUBKEY)
- **Gas Metering**: EVM-compatible gas costs with PQC adjustments
- **Languages**: Solidity compatible (compile with standard solc)
- **Signatures**: All contract transactions signed with Dilithium3 (quantum-resistant)
- **Database**: RocksDB-backed persistent contract state
- **Event Logs**: Indexed by block number with topic filtering
- **Performance**: 2,000+ contract transactions per second

**Gas Economics**:
- Block Gas Limit: 30,000,000 gas per block
- Deployment Gas: 32,000 base + 200 gas/byte
- Call Gas: 21,000-30,000,000 gas range
- Min Gas Price: 1 satINT/gas

**Transaction Types**:
1. Standard (UTXO transfers)
2. Coinbase (mining rewards)
3. Contract Deployment (deploy new contracts) ⭐ **NEW**
4. Contract Call (execute contract functions) ⭐ **NEW**

### Lightning Network

Full implementation of BOLT (Basis of Lightning Technology) specifications:

- **BOLT #1-12**: Complete protocol implementation
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
- **Quantum Resistance**: Dilithium3 signatures, SHA3-256 hashing

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
- **Contract Interaction**: Deploy and call smart contracts ⭐ **NEW**

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

### Smart Contracts (NEW)

```bash
# Deploy a contract
intcoin-cli deploycontract \
  "$(cat SimpleStorage.bin)" \
  "" \
  100000 \
  10

# Call a contract function
intcoin-cli callcontract \
  "int1contract_address" \
  "0x60fe47b1000000000000000000000000000000000000000000000000000000000000002a" \
  0 \
  50000 \
  10

# Get contract information
intcoin-cli getcontractinfo "int1contract_address"

# Query event logs
intcoin-cli getlogs "int1contract_address" 0 100
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
- **[Smart Contracts Guide](docs/SMART_CONTRACTS.md)** ⭐ **NEW**

### Technical Documentation
- [Architecture](docs/ARCHITECTURE.md)
- [Consensus Rules](docs/CONSENSUS.md)
- [Cryptography](docs/CRYPTOGRAPHY.md)
- [RPC API](docs/RPC.md)
- [Security](docs/SECURITY_SANITIZATION.md)
- [Testing](docs/TESTING.md)
- **[Smart Contracts Integration](docs/CONTRACTS_INTEGRATION.md)** ⭐ **NEW**
- **[IBD Optimization](docs/IBD_OPTIMIZATION.md)** ⭐ **NEW**

### Developer Resources
- [API Documentation](docs/RPC.md)
- [Contributing Guide](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)

---

## Project Status

**Development Progress: v1.4.0-beta - Smart Contracts Beta**

| Component | Status | Tests | Notes |
|-----------|--------|-------|-------|
| Core Blockchain | ✅ 100% | ✅ | UTXO, consensus, validation, reorg support complete |
| Cryptography | ✅ 100% | ✅ | Dilithium3 (signatures), Kyber768 (encryption), SHA3-256 |
| Mining | ✅ 100% | ✅ | RandomX PoW, solo + pool mining, GetBlockTemplate |
| Mining Pool Server | ✅ 100% | ✅ | Stratum protocol, PPLNS/PPS payouts, VarDiff |
| Lightning Network | ✅ 100% | ✅ | Full BOLT #1-13 implementation, watchtowers, routing |
| Qt Wallet | ✅ 100% | ✅ | Full-featured desktop wallet with Lightning support |
| RPC Server | ✅ 100% | ✅ | JSON-RPC 2.0, 82+ methods including contracts |
| P2P Network | ✅ 100% | ✅ | Peer discovery, relay, sync, bloom filters (BIP37) |
| Mobile Wallet | ✅ 100% | ✅ | SPV client, iOS/Android SDKs, bloom filters |
| Atomic Swaps | ✅ 100% | ✅ | HTLC, BTC/LTC cross-chain trustless exchanges |
| Cross-Chain Bridges | ✅ 100% | ✅ | wBTC/wETH/wLTC, federated multi-sig, bridge UI |
| Enhanced Mempool | ✅ 100% | ✅ | 6-level priority, persistence, contract tx support |
| Prometheus Metrics | ✅ 100% | ✅ | 40+ metrics, HTTP endpoint at :9090/metrics |
| **Smart Contracts** | ✅ 100% | ✅ 13/13 | **IntSC VM, Solidity support, PQC signatures** ⭐ |
| **Contract RPC** | ✅ 100% | ✅ | **12 RPC methods for contract deployment/interaction** ⭐ |
| **IBD Optimization** | ✅ 100% | ✅ | **Parallel validation, assume-UTXO fast sync** ⭐ |
| Security | ✅ 100% | ✅ | Sanitization, fuzzing, post-quantum cryptography |
| Testing | ✅ 100% | ✅ | **64 test suites, all passing (13 new in v1.4.0)** |
| Documentation | ✅ 100% | N/A | Complete documentation including smart contracts |
| CI/CD | ✅ 100% | ✅ | Automated builds, tests, multi-platform support |

**v1.4.0-beta**: Smart contracts (IntSC VM), IBD optimization, mempool enhancements
**Next Milestone**: v1.5.0 (DeFi primitives, additional privacy features - Q2 2026)

---

## Architecture

```
intcoin/
├── src/
│   ├── blockchain/      # Block and transaction processing
│   ├── consensus/       # Consensus rules and validation
│   ├── crypto/          # Dilithium3, SHA3, RandomX
│   ├── contracts/       # Smart contracts VM and database ⭐ NEW
│   ├── lightning/       # Lightning Network (BOLT specs)
│   ├── mining/          # PoW mining and pool
│   ├── network/         # P2P networking
│   ├── wallet/          # HD wallet, key management
│   ├── rpc/             # JSON-RPC server
│   ├── qt/              # Desktop GUI wallet
│   ├── daemon/          # intcoind (full node)
│   └── cli/             # intcoin-cli (RPC client)
├── include/intcoin/     # Public headers
├── tests/               # Test suites (64 total)
│   └── contracts/       # Smart contract test files ⭐ NEW
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
- **Transaction Model**: UTXO + Account-based contracts
- **Script System**: Bitcoin-compatible opcodes + IntSC VM
- **Address Encoding**: Bech32 (int1 mainnet / tint1 testnet / lint1 Lightning)
- **Block Gas Limit**: 30,000,000 gas (for contract execution) ⭐ **NEW**

### Cryptography

- **Signatures**: Dilithium3 (NIST FIPS 204)
  - Public Key: 1952 bytes
  - Signature: 3309 bytes
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

### Smart Contracts (NEW)

- **VM**: IntSC (EVM-compatible stack machine)
- **Opcodes**: 60+ standard EVM opcodes + 4 PQC opcodes
- **Gas Limit**: 30M gas per block
- **Contract Size**: 24 KB maximum
- **Storage**: Key-value store (RocksDB backend)
- **Performance**: 2,188 deployments/sec, 2,184 calls/sec
- **Database Reads**: 2.5M ops/sec
- **Database Writes**: 242K ops/sec
- **Signature Verification**: 11,206 validations/sec (Dilithium3)

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
# Run all tests (64 test suites)
cd build
ctest --output-on-failure

# Run specific test suites
./tests/test_crypto
./tests/test_blockchain
./tests/test_lightning
./tests/test_contracts_integration      # NEW: Smart contracts
./tests/test_contracts_reorg            # NEW: Reorg handling
./tests/benchmark_contracts             # NEW: Performance benchmarks
```

### Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Merge Request

---

## Performance Metrics (v1.4.0)

### Smart Contracts Benchmarks

Measured on Apple Silicon (macOS 15):

- **Contract Deployment**: 2,188 deployments/second (33,200 gas/op avg)
- **Contract Calls**: 2,184 calls/second (21,136 gas/op avg)
- **Database Writes**: 241,978 writes/second
- **Database Reads**: 2,532,287 reads/second
- **Transaction Validation**: 11,206 validations/second (Dilithium3 PQC)

### Blockchain Performance

- **Block Validation**: < 100ms per block
- **Transaction Throughput**: 2,000+ contract TPS
- **P2P Sync**: ~1000 blocks/second (fast sync with parallel validation)
- **Lightning Payments**: < 1 second end-to-end

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
- **Ethereum Foundation**: EVM design and Solidity compiler
- **Bitcoin Core**: Reference implementation and best practices
- **Open Source Community**: Libraries and tools

---

## Security

Found a security issue? Please email security@international-coin.org or see [SECURITY.md](SECURITY.md) for our responsible disclosure policy.

**Community Project**: Everyone can participate in improving INTcoin security

---

## Roadmap

See [ROADMAP.md](ROADMAP.md) for development direction and focus areas (2026-2031).

**Recent Development:**
- Smart Contracts (IntSC VM) - EVM-compatible with PQC opcodes
- IBD Optimization - Parallel validation and Assume-UTXO
- Lightning Network Integration - Post-quantum secure channels
- Cross-chain Atomic Swaps - BTC and LTC support

---

**Maintained by**: INTcoin Core Development Team
**Lead Developer**: Neil Adamson
**Status**: Active Development

*"Securing value in the quantum era with smart contracts"*

---

*Last Updated: January 9, 2026 - v1.4.0-beta Smart Contracts Beta*
