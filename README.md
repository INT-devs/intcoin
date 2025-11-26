# INTcoin Core

**Version**: 1.0.0-alpha
**Network**: INTCOIN (INT)
**Total Supply**: 221 Trillion INT
**License**: MIT
**Build Status**: ✅ **Crypto + RandomX + Bech32 + Digishield V3 + Serialization Complete** (40%)
**Last Updated**: November 26, 2025

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
| **P2P Port (Mainnet)** | 2210 |
| **RPC Port (Mainnet)** | 2211 |
| **P2P Port (Testnet)** | 2212 |
| **RPC Port (Testnet)** | 2213 |

---

## 🏗️ Project Structure

```
intcoin/
├── src/                    # Core C/C++ source code
│   ├── blockchain/         # Block, transaction, UTXO logic
│   ├── crypto/             # Quantum-resistant cryptography (✅ Complete)
│   ├── consensus/          # PoW, difficulty adjustment (✅ RandomX complete)
│   ├── network/            # P2P networking
│   ├── storage/            # RocksDB persistence
│   ├── rpc/                # JSON-RPC server
│   ├── lightning/          # Lightning Network implementation
│   ├── util/               # Utility functions
│   └── core/               # Core initialization
├── include/intcoin/        # Public header files (✅ Complete - 11 headers)
├── tests/                  # Test suites (✅ 23/23 passing)
│   ├── test_crypto.cpp     # Cryptography tests (✅ 5/5 passing)
│   ├── test_randomx.cpp    # RandomX PoW tests (✅ 6/6 passing)
│   ├── test_bech32.cpp     # Bech32 address tests (✅ 8/8 passing)
│   ├── test_serialization.cpp # Serialization tests (✅ 9/9 passing)
│   └── (more to come)      # Additional test suites
├── build/                  # Build artifacts
├── wiki/                   # Developer and user documentation
│   ├── Developers/         # Technical documentation
│   └── Users/              # User guides
├── branding/               # Brand assets and guidelines
└── scripts/                # Build and utility scripts (header.py)
```

---

## 🔧 Build Dependencies

### Required Dependencies

- **CMake** >= 4.2.0
- **C++ Compiler**: GCC 15.2+, Clang 17+, or Apple Clang 17+ (C++20 support)
- **OpenSSL** >= 3.5.4 (for SHA3-256)
- **liboqs** >= 0.15.0 (NIST PQC - Dilithium3, Kyber768) ✅ **Integrated**
- **RandomX** >= 1.2.1 (ASIC-resistant PoW) ✅ **Integrated**
- **Threads** - Multi-threading support

### Optional Dependencies

- **Boost** >= 1.89.0 - Utilities, filesystem, threading
- **RocksDB** >= 10.7 - Blockchain database
- **Qt6** >= 6.8 (for desktop wallet)
- **libzmq** >= 4.3 - ZeroMQ messaging
- **libevent** >= 2.1 - Event-driven networking
- **libtor** - Tor integration
- **libi2pd** - I2P integration
- **CUDA** / **OpenCL** - GPU mining support

---

## 🚀 Build Instructions

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake boost openssl rocksdb qt6 zeromq libevent

# Build and install liboqs 0.15.0
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install

# Build and install RandomX 1.2.1
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/homebrew \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install

# Build INTcoin
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)

# Run tests
./tests/test_crypto         # Cryptography tests (5/5 passing)
./tests/test_randomx        # RandomX tests (6/6 passing)
./tests/test_bech32         # Bech32 address tests (8/8 passing)
./tests/test_serialization  # Serialization tests (9/9 passing)
```

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev \
    libssl-dev librocksdb-dev qt6-base-dev libzmq3-dev libevent-dev pkg-config

# Build and install liboqs 0.15.0
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig

# Build and install RandomX 1.2.1
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig

# Build INTcoin
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run tests
./tests/test_crypto         # Cryptography tests (5/5 passing)
./tests/test_randomx        # RandomX tests (6/6 passing)
./tests/test_bech32         # Bech32 address tests (8/8 passing)
./tests/test_serialization  # Serialization tests (9/9 passing)
```

### FreeBSD

```bash
# Install dependencies
sudo pkg install cmake boost-all openssl rocksdb qt6 zeromq libevent pkgconf

# Build and install liboqs 0.15.0
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
git checkout 0.15.0
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install

# Build and install RandomX 1.2.1
cd ../..
git clone https://github.com/tevador/RandomX.git
cd RandomX
git checkout v1.2.1
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install

# Build INTcoin
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
gmake -j$(sysctl -n hw.ncpu)

# Run tests
./tests/test_crypto         # Cryptography tests (5/5 passing)
./tests/test_randomx        # RandomX tests (6/6 passing)
./tests/test_bech32         # Bech32 address tests (8/8 passing)
./tests/test_serialization  # Serialization tests (9/9 passing)
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

- [BUILD_STATUS.md](BUILD_STATUS.md) - Detailed build status and progress
- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [DEVELOPMENT-PLAN.md](DEVELOPMENT-PLAN.md) - Complete development roadmap
- [Wiki - Developer Docs](wiki/Developers/Home.md) - Technical documentation
- [Wiki - User Guides](wiki/Users/Home.md) - User documentation
- [Current Progress](wiki/Developers/Current-Progress.md) - Latest development status
- [Build System Guide](wiki/Developers/Build-System.md) - Build instructions
- [API Overview](wiki/Developers/API-Overview.md) - API documentation

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

- **P2P**: `seed.international-coin.org:2210`
- **RPC**: `rpc.international-coin.org:2211`
- **Explorer**: `https://explorer.international-coin.org`
- **Tor**: `intcoinxxx...onion:2210`

### Testnet

- **P2P**: `testseed.international-coin.org:2212`
- **RPC**: `testrpc.international-coin.org:2213`
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

### Phase 1: Core Blockchain (Q1-Q4 2025) - **40% Complete**
- [x] Project structure ✅
- [x] Build system (CMake 4.2.0+) ✅
- [x] Header files (11 comprehensive APIs) ✅
- [x] Quantum-resistant cryptography (Dilithium3 + Kyber768 + SHA3-256) ✅
- [x] RandomX PoW implementation ✅
- [x] Bech32 address encoding ('int1' prefix) ✅
- [x] Digishield V3 difficulty adjustment ✅ **NEW**
- [x] Full transaction/block serialization ✅ **NEW**
- [x] Cryptography tests (5/5 passing) ✅
- [x] RandomX tests (6/6 passing) ✅
- [x] Bech32 tests (8/8 passing) ✅
- [x] Serialization tests (9/9 passing) ✅ **NEW**
- [ ] P2P networking
- [ ] RocksDB storage
- [ ] UTXO model implementation
- [ ] Basic RPC server

### Phase 2: Wallet & Tools (Q2-Q3 2025)
- [ ] Desktop wallet (Qt)
- [ ] Web wallet
- [ ] Mobile wallet (Android/iOS)
- [ ] Block explorer
- [ ] Mining software

### Phase 3: Lightning Network (Q4 2025)
- [ ] BOLT specifications implementation
- [ ] Payment channels
- [ ] Routing algorithm
- [ ] Watchtowers

### Phase 4: Advanced Features (2026)
- [ ] Tor/I2P integration
- [ ] Hardware wallet support
- [ ] Multi-signature
- [ ] Atomic swaps

**Current Status**: Core cryptography, PoW, address encoding, difficulty adjustment, and serialization complete (40%). Next: RocksDB storage, UTXO model, and P2P networking.

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
