# INTcoin Core

**Version**: 1.0.0-alpha
**Network**: INTCOIN (INT)
**Total Supply**: 221 Trillion INT
**License**: MIT

---

## 🎯 Project Overview

INTcoin is a quantum-resistant cryptocurrency designed for long-term security in the post-quantum era. Built from the ground up with NIST-approved post-quantum cryptographic algorithms and ASIC-resistant mining.

### Key Features

- ✅ **Quantum-Resistant**: Dilithium (signatures) + Kyber (key exchange)
- ✅ **ASIC-Resistant**: RandomX Proof-of-Work algorithm
- ✅ **Lightning Network**: Layer 2 scaling solution
- ✅ **Cross-Platform**: macOS, Windows, Linux, FreeBSD
- ✅ **Multi-Wallet**: Desktop (Qt), Web, Mobile (Android/iOS)
- ✅ **Block Explorer**: Real-time blockchain explorer
- ✅ **Privacy**: Tor/I2P support

---

## 📊 Network Specifications

| Parameter | Value |
|-----------|-------|
| **Coin Name** | INTCOIN |
| **Ticker** | INT |
| **Base Unit** | INT |
| **Sub Unit** | INTS (1 INT = 1,000,000 INTS) |
| **Total Supply** | 221,000,000,000,000 INT (221 Trillion) |
| **Initial Block Reward** | 105,113,636 INT |
| **Block Time** | 2 minutes (120 seconds) |
| **Halving Interval** | 1,051,200 blocks (~4 years) |
| **Difficulty Adjustment** | Every block (Digishield V3) |
| **PoW Algorithm** | RandomX |
| **Signature Algorithm** | Dilithium3 (NIST PQC) |
| **Key Exchange** | Kyber768 (NIST PQC) |
| **Address Format** | Bech32 (int1...) |
| **P2P Port** | 9333 |
| **RPC Port** | 9334 |

---

## 🏗️ Project Structure

```
intcoin/
├── src/                    # Core C/C++ source code
│   ├── blockchain/         # Block, transaction, UTXO logic
│   ├── crypto/             # Quantum-resistant cryptography
│   ├── consensus/          # PoW, difficulty adjustment
│   ├── network/            # P2P networking
│   ├── storage/            # RocksDB persistence
│   ├── rpc/                # JSON-RPC server
│   ├── lightning/          # Lightning Network implementation
│   └── miner/              # RandomX miner
├── include/                # Public header files
├── lib/                    # Third-party libraries
├── tests/                  # Test suites
│   ├── unit/               # Unit tests (Python/C++)
│   ├── functional/         # Functional tests (Python)
│   └── fuzz/               # Fuzz tests (libFuzzer)
├── docs/                   # Documentation
├── branding/               # Brand assets and guidelines
├── website/                # Official website (symlink)
├── web-wallet/             # Web wallet (symlink)
├── desktop-wallet/         # Desktop wallet (symlink)
├── mobile-wallet/          # Mobile wallet (symlink)
│   ├── android/            # Android app
│   └── ios/                # iOS app
├── explorer/               # Block explorer
└── scripts/                # Build and utility scripts
```

---

## 🔧 Build Dependencies

### Core Dependencies

- **CMake** >= 3.20
- **C++ Compiler**: GCC 11+ or Clang 14+ (C++20 support)
- **Boost** >= 1.75
- **OpenSSL** >= 3.0
- **RocksDB** >= 7.0
- **liboqs** (Open Quantum Safe) - PQC algorithms
- **RandomX** - ASIC-resistant PoW
- **Qt6** >= 6.5 (for desktop wallet)
- **libzmq** - ZeroMQ messaging
- **libevent** - Event-driven networking

### Optional Dependencies

- **libtor** - Tor integration
- **libi2pd** - I2P integration
- **CUDA** / **OpenCL** - GPU mining support

---

## 🚀 Build Instructions

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake boost openssl rocksdb qt6 zeromq libevent

# Clone and build liboqs
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew ..
make && sudo make install

# Clone and build RandomX
git clone https://github.com/tevador/RandomX.git
cd RandomX && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew ..
make && sudo make install

# Build INTcoin
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)

# Run tests
ctest --output-on-failure

# Install
sudo make install
```

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev \
    libssl-dev librocksdb-dev qt6-base-dev libzmq3-dev libevent-dev

# Build liboqs and RandomX (same as macOS)

# Build INTcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
ctest --output-on-failure
sudo make install
```

### FreeBSD

```bash
# Install dependencies
pkg install cmake boost-all openssl rocksdb qt6 zeromq4 libevent

# Build liboqs and RandomX
# (similar to above)

# Build INTcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
ctest --output-on-failure
sudo gmake install
```

### Windows

```bash
# Use vcpkg for dependencies
vcpkg install boost openssl rocksdb qt6 zeromq libevent

# Build with Visual Studio 2022
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

# Run tests
ctest -C Release --output-on-failure
```

---

## 🧪 Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suites
ctest -R unit       # Unit tests only
ctest -R functional # Functional tests only
ctest -R fuzz       # Fuzz tests only

# Run Python functional tests directly
cd ../tests/functional
python3 -m pytest -v

# Generate coverage report
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make coverage
```

---

## 📚 Documentation

- [Architecture Overview](docs/architecture.md)
- [Quantum Cryptography](docs/quantum-crypto.md)
- [RandomX Mining](docs/randomx-mining.md)
- [Lightning Network](docs/lightning.md)
- [RPC API Reference](docs/rpc-api.md)
- [Wallet Development](docs/wallet-dev.md)
- [Block Explorer API](docs/explorer-api.md)

---

## 🎮 Usage

### Running a Full Node

```bash
# Start intcoind daemon
intcoind --daemon

# Check sync status
intcoin-cli getblockchaininfo

# Generate new address
intcoin-cli getnewaddress

# Send transaction
intcoin-cli sendtoaddress int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh 100.0
```

### Mining

```bash
# Solo mining (CPU)
intcoin-miner --threads 8 --address int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh

# Pool mining
intcoin-miner --pool stratum+tcp://pool.intcoin.org:3333 --user your_address --pass x
```

### Lightning Network

```bash
# Open Lightning channel
intcoin-lightning openchannel <node_pubkey> 1000000

# Send Lightning payment
intcoin-lightning sendpayment <invoice>

# Close channel
intcoin-lightning closechannel <channel_id>
```

---

## 🌐 Network Endpoints

### Mainnet

- **P2P**: `seed.international-coin.org:9333`
- **RPC**: `rpc.international-coin.org:9334`
- **Explorer**: `https://explorer.international-coin.org`
- **Tor**: `intcoinxxx...onion:9333`

### Testnet

- **P2P**: `testseed.international-coin.org:19333`
- **RPC**: `testrpc.international-coin.org:19334`
- **Explorer**: `https://testnet-explorer.international-coin.org`

---

## 🔐 Security

### Responsible Disclosure

Found a security vulnerability? Please report it privately to:
- **Email**: security@international-coin.org
- **GPG Key**: [Download](https://international-coin.org/security.asc)

**Do not** create public GitHub/GitLab issues for security vulnerabilities.

### Security Audits

- [ ] **Cryptography**: Pending third-party audit
- [ ] **Consensus**: Pending third-party audit
- [ ] **Network**: Pending third-party audit

---

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes with signed commits
4. Write tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

### Code Style

- C++: Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Python: Follow [PEP 8](https://pep8.org/)
- Run `clang-format` before committing

---

## 📋 Roadmap

### Phase 1: Core Blockchain (Q1 2025) ✅
- [x] Project structure
- [ ] Core blockchain logic
- [ ] Quantum-resistant cryptography integration
- [ ] RandomX PoW implementation
- [ ] P2P networking
- [ ] RocksDB storage
- [ ] Basic RPC server

### Phase 2: Wallet & Tools (Q2 2025)
- [ ] Desktop wallet (Qt)
- [ ] Web wallet
- [ ] Mobile wallet (Android/iOS)
- [ ] Block explorer
- [ ] Mining software

### Phase 3: Lightning Network (Q3 2025)
- [ ] BOLT specifications implementation
- [ ] Payment channels
- [ ] Routing algorithm
- [ ] Watchtowers

### Phase 4: Advanced Features (Q4 2025)
- [ ] Tor/I2P integration
- [ ] Hardware wallet support
- [ ] Multi-signature
- [ ] Atomic swaps

---

## 📜 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

```
Copyright (c) 2025 INTcoin Team (Neil Adamson)
```

---

## 🙏 Acknowledgments

- **NIST PQC**: For post-quantum cryptographic standards
- **RandomX**: For ASIC-resistant PoW algorithm
- **Bitcoin Core**: For blockchain architecture inspiration
- **Lightning Labs**: For Lightning Network specifications
- **Open Quantum Safe**: For liboqs library

---

## 📞 Contact

- **Website**: https://international-coin.org
- **Email**: team@international-coin.org
- **GitLab**: https://gitlab.com/intcoin/crypto
- **Discord**: https://discord.gg/intcoin
- **Telegram**: https://t.me/intcoin
- **Twitter**: @intcoin_official

---

**Built with ❤️ for the quantum era**
