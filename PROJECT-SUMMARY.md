# INTcoin Project Summary

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

---

## 🎯 Project Overview

**INTcoin (INT)** is a next-generation cryptocurrency designed to be:

- **Quantum-Resistant**: Uses CRYSTALS-Dilithium & Kyber (NIST-approved)
- **ASIC-Resistant**: RandomX PoW algorithm (CPU-optimized)
- **Privacy-Focused**: Pseudonymous by default
- **Decentralized**: Pure PoW, no staking or governance
- **Feature-Rich**: Lightning Network, Smart Contracts, Cross-chain bridges

---

## 📊 Network Specifications

| Parameter | Value |
|-----------|-------|
| **Ticker** | INT |
| **Change Unit** | INTS |
| **Max Supply** | 221 Trillion INT |
| **Block Time** | 2 minutes (120 seconds) |
| **Consensus** | Proof of Work (PoW) |
| **Mining** | CPU-friendly (RandomX) |
| **Signatures** | CRYSTALS-Dilithium (quantum-safe) |
| **Key Exchange** | Kyber (quantum-safe) |

---

## 🏗️ Technology Stack

### Core
- **Language**: C++23
- **Build System**: CMake 4.0+
- **Testing**: Python 3.12
- **Cryptography**: NIST FIPS 204, 203, 202 compliant

### Dependencies
- **Boost**: 1.70+ (system, filesystem, thread, program_options)
- **OpenSSL**: 3.0+ (crypto operations)
- **Qt5**: 5.15+ (GUI wallet)
- **RandomX**: ASIC-resistant PoW
- **liboqs**: Quantum-safe cryptography (or NIST reference implementations)

### Platforms Supported
- ✅ macOS (Intel & Apple Silicon)
- ✅ Linux (Ubuntu, Debian, Fedora, Arch, etc.)
- ✅ FreeBSD
- ✅ Windows (via Visual Studio)
- ✅ Unix systems

---

## 📁 Project Structure

```
intcoin/
├── build/                  # Build artifacts (gitignored)
├── cmake/                  # CMake modules
├── config/                 # Network configurations
│   ├── mainnet/
│   └── testnet/
├── contrib/                # Additional tools and dependencies
├── docs/                   # Documentation
│   ├── BUILD-WINDOWS.md
│   ├── INSTALL-LINUX.md
│   └── INSTALL-FREEBSD.md
├── external/               # External dependencies
│   ├── randomx/           # RandomX (ASIC-resistant PoW)
│   ├── dilithium/         # CRYSTALS-Dilithium (signatures)
│   └── kyber/             # CRYSTALS-Kyber (key exchange)
├── include/intcoin/        # Public headers
│   ├── primitives.h       # Core types and constants
│   ├── block.h            # Block structures
│   ├── transaction.h      # Transaction & UTXO
│   ├── merkle.h           # Merkle trees
│   └── crypto.h           # Cryptography interfaces
├── scripts/                # Build and utility scripts
├── src/                    # Source code
│   ├── core/              # Blockchain core
│   ├── crypto/            # Cryptography implementations
│   ├── network/           # P2P networking
│   ├── consensus/         # Consensus rules
│   ├── wallet/            # Wallet backend
│   ├── miner/             # CPU miner
│   ├── lightning/         # Lightning Network
│   ├── contracts/         # Smart contracts VM
│   ├── bridge/            # Cross-chain bridges
│   ├── rpc/               # RPC server
│   ├── qt/                # Qt GUI wallet
│   ├── daemon/            # intcoind daemon
│   ├── cli/               # intcoin-cli tool
│   └── explorer/          # Block explorer
├── tests/                  # Test suite
│   ├── unit/              # C++ unit tests
│   ├── integration/       # Integration tests
│   └── functional/        # Functional tests (Python)
├── CMakeLists.txt          # Root CMake configuration
├── COPYING                 # MIT License
├── README.md               # Main README
├── ROADMAP.md              # 5-year development roadmap
├── DESIGN-DECISIONS.md     # Architecture decisions
├── REWARDS-PROGRAM.md      # Reward distribution options
├── REWARDS-4YEAR-HALVING.md # Bitcoin-style halving model
├── DEVELOPMENT-STATUS.md   # Current development status
├── header.py               # Copyright header tool
└── .gitignore             # Git ignore rules
```

---

## 💰 Reward Models

### Option 1: Multi-Phase Hybrid (60 years) - RECOMMENDED
- **Phase 1** (0-10y): 3,000,000 INT/block
- **Phase 2** (10-25y): 2,000,000 INT/block
- **Phase 3** (25-45y): 1,000,000 INT/block
- **Phase 4** (45-60y): 500,000 INT/block

### Option 2: 4-Year Halving (Bitcoin-style)
- **Initial**: 105,133,000 INT/block
- **Halving**: Every 1,051,200 blocks (~4 years)
- **Distribution**: 50% in 4 years, 99% in ~36 years

See [REWARDS-PROGRAM.md](REWARDS-PROGRAM.md) and [REWARDS-4YEAR-HALVING.md](REWARDS-4YEAR-HALVING.md) for full details.

---

## 🗺️ Development Roadmap

### Year 1 (2025): Foundation & Launch
- ✅ Q1: Core development (blockchain, crypto, PoW)
- Q2: Wallet & mining
- Q3: Lightning Network
- Q4: Security audits & **Mainnet Launch** 🚀

### Year 2 (2026): Growth & Smart Contracts
- Q1: Post-launch stability, mobile wallets
- Q2-Q3: Smart contract layer
- Q4: Enhanced privacy features

### Year 3 (2027): Interoperability
- Q1-Q2: Cross-chain bridges (BTC, ETH, etc.)
- Q3: Scalability improvements
- Q4: Mining ecosystem tools

### Year 4 (2028): Enterprise
- Q1: Enterprise features (multi-sig, payment processing)
- Q2: Governance framework (advisory only)
- Q3: Compliance tools
- Q4: Mobile ecosystem expansion

### Year 5 (2029-2030): Quantum Future
- Q1-Q2: Next-gen quantum resistance
- Q3: Protocol v2 upgrades
- Q4: Ecosystem maturity & future planning

See [ROADMAP.md](ROADMAP.md) for complete details.

---

## 🔐 Security Features

### Quantum Resistance
- **CRYSTALS-Dilithium** (NIST FIPS 204): Post-quantum signatures
- **Kyber** (NIST FIPS 203): Post-quantum key exchange
- **SHA3-256** (FIPS 202): Quantum-resistant hashing
- **Algorithm Agility**: Ready for future quantum algorithms

### ASIC Resistance
- **RandomX**: Memory-hard, CPU-optimized PoW
- **No ASICs**: Keeps mining decentralized
- **Fair Mining**: Anyone with a CPU can mine

### Privacy
- **Pseudonymous**: All transactions use pseudonymous addresses
- **No Tracking**: No built-in surveillance
- **Future**: Optional enhanced privacy (stealth addresses, etc.)

---

## 🔧 Build Instructions

### Quick Start (macOS - Already Done!)

```bash
# Dependencies already installed via Homebrew:
# - cmake 4.1.2
# - boost 1.89.0
# - openssl@3 3.6.0
# - qt@5 5.15.17
# - python@3.12

cd intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5
make -j$(sysctl -n hw.ncpu)
```

### Other Platforms
- **Linux**: See [INSTALL-LINUX.md](docs/INSTALL-LINUX.md)
- **FreeBSD**: See [INSTALL-FREEBSD.md](docs/INSTALL-FREEBSD.md)
- **Windows**: See [BUILD-WINDOWS.md](docs/BUILD-WINDOWS.md)

---

## 📝 What's Been Created

### ✅ Completed (Architecture Phase)

1. **Complete project structure** with organized directories
2. **CMake build system** with C++23 support and cross-platform configuration
3. **Core header files** defining all major data structures:
   - Primitives (types, constants)
   - Blocks and block headers
   - Transactions and UTXO model
   - Merkle trees
   - Quantum-resistant cryptography interfaces
4. **Configuration files** for mainnet and testnet
5. **Comprehensive documentation**:
   - README with overview
   - 5-year roadmap
   - Multiple reward distribution models
   - Build guides for Windows, Linux, FreeBSD
   - Development status tracking
6. **Tooling**: `header.py` for automated copyright management
7. **External dependency stubs** for RandomX, Dilithium, Kyber
8. **Dependencies installed** on your Mac

### 🔄 Next Phase: Implementation

The architecture is complete. Next steps:

1. **Integrate actual cryptographic libraries** (liboqs or NIST references)
2. **Implement core blockchain logic** (.cpp files)
3. **Build P2P networking layer**
4. **Create wallet backend**
5. **Develop Qt GUI**
6. **Write comprehensive tests**

See [DEVELOPMENT-STATUS.md](DEVELOPMENT-STATUS.md) for detailed implementation roadmap.

---

## 🧪 Testing

### Test Framework
- **Unit Tests**: C++ (Catch2 or Google Test)
- **Integration Tests**: Python 3
- **Functional Tests**: Python test framework
- **Performance Tests**: Benchmarking suite

### Running Tests
```bash
# C++ unit tests
cd build
ctest

# Python tests
cd tests
python3 -m pytest
```

---

## 📧 Contact Information

### Team
- **Lead Developer**: Maddison Lane
- **General Email**: team@international-coin.org
- **Security Email**: security@international-coin.org
- **Website**: https://international-coin.org

### GPG Keys
- **Admin**: `6E68 20D6 0277 879B 3EFA 62D1 EB1A 8F24 AB19 0CBC`
- **Team**: `85A2 19E7 98EE E017 2669 450B E7FC C378 2A41 8E33`
- **Security**: `50FA 6D8F 2359 DBD9 3BA7 6263 1916 8ED3 FF91 FF72`

### Repository
- **GitLab**: https://gitlab.com/intcoin/crypto
- **Wiki**: https://gitlab.com/intcoin/crypto/-/wikis/home
- **Issues**: https://gitlab.com/intcoin/crypto/-/issues

---

## 📜 License

INTcoin is released under the **MIT License**.

See [COPYING](COPYING) for full license text.

---

## 🎉 Current Status

**Phase**: Architecture & Planning Complete ✅

**Version**: 0.1.0-alpha (in development)

**What You Have**:
- Complete project structure
- Full architecture defined in headers
- Build system configured
- Dependencies installed
- Comprehensive documentation
- Multiple reward models designed
- 5-year roadmap
- Installation guides for all platforms

**What's Next**:
1. Integrate external cryptographic libraries
2. Implement core blockchain logic
3. Build out networking and consensus
4. Create functional wallet
5. Comprehensive testing
6. Security audits
7. Mainnet launch (Q4 2025 target)

---

## 🚀 Getting Started for Development

### For New Developers

1. **Read the documentation**:
   - [README.md](README.md) - Overview
   - [DESIGN-DECISIONS.md](DESIGN-DECISIONS.md) - Why we made certain choices
   - [DEVELOPMENT-STATUS.md](DEVELOPMENT-STATUS.md) - What's done and what's next

2. **Set up your environment**:
   - Follow install guide for your platform
   - Install all dependencies
   - Build the project

3. **Pick a task**:
   - Check [DEVELOPMENT-STATUS.md](DEVELOPMENT-STATUS.md) for "Not Yet Started" items
   - Start with cryptography or core blockchain components
   - Write tests for everything you implement

4. **Follow conventions**:
   - Use `header.py` to add copyright headers
   - Follow C++23 best practices
   - Write clear, documented code
   - Test thoroughly

---

## 📚 Key Documents

| Document | Purpose |
|----------|---------|
| [README.md](README.md) | Project overview and quick start |
| [ROADMAP.md](ROADMAP.md) | 5-year development plan |
| [DESIGN-DECISIONS.md](DESIGN-DECISIONS.md) | Architecture and design rationale |
| [REWARDS-PROGRAM.md](REWARDS-PROGRAM.md) | Distribution model options |
| [REWARDS-4YEAR-HALVING.md](REWARDS-4YEAR-HALVING.md) | Bitcoin-style halving details |
| [DEVELOPMENT-STATUS.md](DEVELOPMENT-STATUS.md) | Current progress and next steps |
| [BUILD-WINDOWS.md](docs/BUILD-WINDOWS.md) | Windows build instructions |
| [INSTALL-LINUX.md](docs/INSTALL-LINUX.md) | Linux installation guide |
| [INSTALL-FREEBSD.md](docs/INSTALL-FREEBSD.md) | FreeBSD installation guide |
| [COPYING](COPYING) | MIT License |

---

## 🌟 Vision

**INTcoin aims to be the world's leading quantum-resistant cryptocurrency**, providing secure, decentralized financial infrastructure that withstands the threats of both quantum computing and mining centralization, while remaining accessible to everyone.

**The future is quantum-safe. The future is decentralized. The future is INTcoin.**

---

**Last Updated**: January 2025
**Version**: 0.1.0-alpha
**Status**: Architecture Complete, Implementation in Progress
