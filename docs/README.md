# INTcoin Documentation

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

Welcome to the INTcoin documentation. This directory contains comprehensive guides for building, installing, and using INTcoin.

## Table of Contents

### Getting Started

- [**Quick Start Guide**](QUICK-START.md) - **Get started in minutes** ⚡
- [Genesis Block](GENESIS_BLOCK.md) - Official mainnet genesis block details
- [README](../README.md) - Project overview and features
- [Project Status](../PROJECT-STATUS.md) - Current development status

### Building & Installation

- [**BUILD.md**](../BUILD.md) - **Comprehensive build guide for all platforms**
- [Windows Build Guide](BUILD-WINDOWS.md) - Detailed Windows build instructions
- [Linux Installation Guide](INSTALL-LINUX.md) - Linux build and install instructions
- [FreeBSD Installation Guide](INSTALL-FREEBSD.md) - FreeBSD build and install instructions

### Technical Documentation

- [Cryptography Design](CRYPTOGRAPHY-DESIGN.md) - Quantum-resistant cryptography design
- [RPC API Reference](RPC-API.md) - Complete JSON-RPC API documentation
- [Design Decisions](../DESIGN-DECISIONS.md) - Architectural design decisions
- [Roadmap](../ROADMAP.md) - Five-year development roadmap
- [Testing Guide](TESTING.md) - Comprehensive testing documentation
- [Fuzz Testing](FUZZ-TESTING.md) - Fuzzing infrastructure and tests
- [Python Tests](PYTHON-TESTS.md) - Python-based integration tests

### Project Information

- [Implementation Summary](../IMPLEMENTATION-SUMMARY.md) - Current implementation status
- [Development Status](../DEVELOPMENT-STATUS.md) - Development progress
- [Project Summary](../PROJECT-SUMMARY.md) - High-level project overview
- [Q4 2025 Completion Report](Q4_2025_COMPLETION_REPORT.md) - Recent achievements
- [Production Readiness](PRODUCTION-READINESS.md) - Launch preparation status

---

## Quick Reference

### Build Scripts

INTcoin provides automated build scripts for easy compilation:

**Linux:**
```bash
./build-linux.sh          # Standard release build
./build-linux.sh --help   # View all options
```

**FreeBSD:**
```bash
./build-freebsd.sh        # Standard release build
./build-freebsd.sh --help # View all options
```

**Windows:**
```powershell
.\build-windows.ps1               # Standard release build
.\build-windows.ps1 -WithInstaller # Build with installer
```

### Platform-Specific Quick Starts

#### Ubuntu/Debian
```bash
sudo apt install build-essential cmake libboost-all-dev libssl-dev qtbase5-dev
git clone https://gitlab.com/intcoin/crypto.git
cd crypto && ./build-linux.sh
```

#### Fedora/RHEL
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake boost-devel openssl-devel qt5-qtbase-devel
git clone https://gitlab.com/intcoin/crypto.git
cd crypto && ./build-linux.sh
```

#### FreeBSD
```bash
pkg install cmake boost-all openssl qt5
git clone https://gitlab.com/intcoin/crypto.git
cd crypto && ./build-freebsd.sh
```

#### macOS
```bash
brew install cmake boost openssl qt@5
git clone https://gitlab.com/intcoin/crypto.git
cd crypto && mkdir build && cd build && cmake .. && make
```

#### Windows
```powershell
# Install Visual Studio 2022 and vcpkg first
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
.\build-windows.ps1
```

---

## Documentation by Category

### For Users

- **Getting Started**: [Quick Start](QUICK-START.md) | [Web Guide](https://international-coin.org/docs/getting-started.html)
- **Installation**: [BUILD.md](../BUILD.md)
- **RPC API**: [RPC-API.md](RPC-API.md)
- **Project Overview**: [README](../README.md)
- **Whitepaper**: [View Online](https://international-coin.org/docs/whitepaper.html) | [Download PDF](https://international-coin.org/docs/whitepaper.pdf)
- **Network Info**: [Seed Nodes](SEED_NODES.md), [Tor Guide](TOR-GUIDE.md)
- **Security**: [Security Audit](SECURITY-AUDIT.md), [GPG Signing](GPG-SIGNING.md)

### For Developers

- **Building**: [BUILD.md](../BUILD.md)
- **Design**: [DESIGN-DECISIONS.md](../DESIGN-DECISIONS.md)
- **Cryptography**: [CRYPTOGRAPHY-DESIGN.md](CRYPTOGRAPHY-DESIGN.md)
- **Implementation**: [IMPLEMENTATION-SUMMARY.md](../IMPLEMENTATION-SUMMARY.md)
- **Roadmap**: [ROADMAP.md](../ROADMAP.md)

### For System Administrators

- **Linux Installation**: [INSTALL-LINUX.md](INSTALL-LINUX.md)
- **FreeBSD Installation**: [INSTALL-FREEBSD.md](INSTALL-FREEBSD.md)
- **Windows Installation**: [BUILD-WINDOWS.md](BUILD-WINDOWS.md)
- **RPC API**: [RPC-API.md](RPC-API.md)
- **Network Setup**: [Seed Nodes](SEED_NODES.md)
- **Privacy**: [Tor Guide](TOR-GUIDE.md), [Tor Quick Start](TOR-QUICKSTART.md)

---

## Key Features

### Quantum-Resistant Cryptography
- **CRYSTALS-Dilithium5** (signatures) - NIST Level 5
- **CRYSTALS-Kyber1024** (key exchange) - NIST Level 5
- **SHA3-256** (transactions, merkle trees)
- **SHA-256** (PoW mining - becomes ASIC-resistant in quantum era)

### Core Components
- ✅ Blockchain core with UTXO model
- ✅ P2P networking layer
- ✅ Transaction mempool with fee prioritization
- ✅ Multi-threaded CPU miner
- ✅ HD Wallet with BIP39 mnemonics
- ✅ Qt GUI wallet
- ✅ JSON-RPC 2.0 API (25+ methods)
- 🔄 Lightning Network (planned Q3 2025)
- 🔄 Smart Contracts (planned 2026)

### Supported Platforms
- ✅ Linux (Ubuntu, Debian, Fedora, RHEL, Arch, Gentoo, openSUSE)
- ✅ FreeBSD 13.x, 14.x
- ✅ macOS (Intel and Apple Silicon)
- ✅ Windows 10/11 (x64)

---

## Build Outputs

After building, you'll have the following executables:

### Core Components
- **intcoind** - Blockchain daemon (headless node)
- **intcoin-cli** - Command-line interface for RPC calls
- **intcoin-qt** - Qt GUI wallet (if Qt enabled)

### Additional Tools
- **intcoin-miner** - Standalone CPU miner
- **intcoin-explorer** - Block explorer (planned)

### Configuration Files
- `config/mainnet/intcoin.conf` - Mainnet configuration
- `config/testnet/intcoin.conf` - Testnet configuration

---

## Support & Contact

- **Email**: team@international-coin.org
- **Security**: security@international-coin.org
- **GitLab**: https://gitlab.com/intcoin/crypto
- **Website**: https://international-coin.org
- **Documentation**: https://international-coin.org/docs/
- **Getting Started**: https://international-coin.org/docs/getting-started.html
- **Whitepaper**: https://international-coin.org/docs/whitepaper.html

### GPG Keys
- **Admin**: `6E68 20D6 0277 879B 3EFA 62D1 EB1A 8F24 AB19 0CBC`
- **Team**: `85A2 19E7 98EE E017 2669 450B E7FC C378 2A41 8E33`
- **Security**: `50FA 6D8F 2359 DBD9 3BA7 6263 1916 8ED3 FF91 FF72`

---

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

Before contributing:
1. Read the [Design Decisions](../DESIGN-DECISIONS.md)
2. Check the [Roadmap](../ROADMAP.md)
3. Review [Implementation Summary](../IMPLEMENTATION-SUMMARY.md)

---

## License

INTcoin is released under the terms of the MIT license. See [COPYING](../COPYING) for more information.

---

**Last Updated**: November 15, 2025
