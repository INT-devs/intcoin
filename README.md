# INTcoin (INT)

**Quantum-Resistant, ASIC-Resistant Cryptocurrency**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Python 3](https://img.shields.io/badge/python-3.x-blue.svg)](https://www.python.org/)

## Overview

INTcoin is a next-generation cryptocurrency designed from the ground up to be resistant to both quantum computing attacks and ASIC mining centralization. Built with privacy, decentralization, and long-term security as core principles.

### Key Features

- ✅ **Quantum-Resistant Cryptography**: CRYSTALS-Dilithium5 and Kyber1024 (NIST Level 5 post-quantum algorithms)
- ✅ **ASIC-Resistant Mining**: SHA-256 PoW (becomes ASIC-resistant in quantum era)
- ✅ **Privacy-Focused**: Pseudonymous transactions by default
- ✅ **HD Wallet**: BIP39 mnemonic phrases with hierarchical deterministic key generation
- ✅ **Multi-threaded CPU Miner**: Optimized SHA-256 mining with auto-thread detection
- ✅ **P2P Network**: Distributed peer-to-peer networking
- ✅ **Transaction Mempool**: Fee-based transaction prioritization
- ✅ **Qt GUI Wallet**: Professional cross-platform graphical interface
- ✅ **JSON-RPC API**: Complete remote control capabilities
- ✅ **Lightning Network**: Fast, low-cost Layer 2 payment channels
- ✅ **Smart Contracts**: Secure VM with gas metering and SafeMath
- ✅ **Cross-Chain Bridges**: Atomic swaps with Bitcoin and Ethereum
- ✅ **Merkle Tree Structure**: SHA3-256 based efficient transaction verification
- ✅ **Pure PoW**: No staking, governance, or centralization mechanisms
- ✅ **Mining Pool Support**: Stratum protocol V1 for pooled mining

### Specifications

| Parameter | Value |
|-----------|-------|
| Ticker | INT |
| Change Unit | INTS |
| Max Supply | 221 Trillion INT |
| Block Time | 2 minutes (120 seconds) |
| Consensus | Proof of Work (PoW) |
| Mining Algorithm | SHA-256 (quantum-era ASIC-resistant) |
| Hash Function | SHA3-256 (transactions, merkle trees) |
| Signature Scheme | CRYSTALS-Dilithium5 (NIST Level 5) |
| Key Exchange | CRYSTALS-Kyber1024 (NIST Level 5) |
| Initial Reward | 105,113,636 INT |
| Halving Method | 50% every 4 years (traditional) |
| Halving Interval | 1,051,200 blocks (~4 years) |
| Total Emission Period | ~256 years (64 halvings) |
| Address Format | Base58Check |

## Project Information

- **Website**: https://international-coin.org
- **Repository**: https://gitlab.com/intcoin/crypto
- **Wiki**: https://gitlab.com/intcoin/crypto/-/wikis/home
- **Lead Developer**: Maddison Lane
- **License**: MIT

### Contact

- **General**: team@international-coin.org
- **Security**: security@international-coin.org

### GPG Keys

- **Admin**: `6E68 20D6 0277 879B 3EFA 62D1 EB1A 8F24 AB19 0CBC`
- **Team**: `85A2 19E7 98EE E017 2669 450B E7FC C378 2A41 8E33`
- **Security**: `50FA 6D8F 2359 DBD9 3BA7 6263 1916 8ED3 FF91 FF72`

## Building from Source

### Prerequisites

#### macOS
```bash
brew install cmake boost openssl qt@5 python3
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libboost-all-dev libssl-dev \
    qtbase5-dev qttools5-dev libqt5svg5-dev python3 python3-pip
```

#### FreeBSD
```bash
pkg install cmake boost-all openssl qt5 python3
```

#### Windows
- Install Visual Studio 2022 or later
- Install CMake 3.20+
- Install vcpkg and dependencies:
```powershell
vcpkg install boost openssl qt5
```

### Build Instructions

#### Quick Build (Automated Scripts)

**Linux:**
```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
./build-linux.sh          # Standard release build
./build-linux.sh --help   # See all options
```

**FreeBSD:**
```bash
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
./build-freebsd.sh        # Standard release build
./build-freebsd.sh --help # See all options
```

**Windows (PowerShell):**
```powershell
git clone https://gitlab.com/intcoin/crypto.git
cd crypto
.\build-windows.ps1               # Standard release build
.\build-windows.ps1 -WithInstaller # Build with installer
```

#### Manual Build

```bash
# Clone the repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Install (optional)
sudo cmake --install .
```

### Installation Scripts

Automated installation scripts are available for various platforms:

**Linux:**
- Debian/Ubuntu: `./scripts/install-debian.sh`
- Fedora/RHEL: `./scripts/install-fedora.sh`
- Arch Linux: `./scripts/install-arch.sh`

**FreeBSD:**
- `./scripts/install-freebsd.sh`

**Windows:**
- PowerShell installer: `.\scripts\install-windows.ps1`

For detailed platform-specific instructions, see:
- Windows: [docs/BUILD-WINDOWS.md](docs/BUILD-WINDOWS.md)
- Linux: [docs/INSTALL-LINUX.md](docs/INSTALL-LINUX.md)
- FreeBSD: [docs/INSTALL-FREEBSD.md](docs/INSTALL-FREEBSD.md)

### Build Options

```bash
# Disable Qt wallet
cmake .. -DBUILD_QT_WALLET=OFF

# Disable Lightning Network
cmake .. -DENABLE_LIGHTNING=OFF

# Build with tests
cmake .. -DBUILD_TESTS=ON
```

## Quick Start

### Running the GUI Wallet

```bash
# Start the Qt wallet
./build/intcoin-qt

# Or with specific data directory
./build/intcoin-qt --datadir=/path/to/data
```

### Running the Daemon

```bash
# Start the daemon
./build/intcoind

# Run in background
./build/intcoind --daemon

# Specify testnet
./build/intcoind --testnet
```

### Using the CLI

```bash
# Get blockchain info
./build/intcoin-cli getblockchaininfo

# Create new address
./build/intcoin-cli getnewaddress "My Address"

# Check balance
./build/intcoin-cli getbalance

# Send transaction
./build/intcoin-cli sendtoaddress "INT1..." 10.5

# Start mining
./build/intcoin-cli startmining 4

# Stop mining
./build/intcoin-cli stopmining
```

### First Time Setup

1. **Create a wallet**:
```bash
./build/intcoin-qt
# Click "File" > "New Wallet"
# Save your mnemonic phrase securely!
```

2. **Get your first address**:
```bash
./build/intcoin-cli getnewaddress "Main"
```

3. **Start mining** (testnet recommended):
```bash
./build/intcoin-cli --testnet startmining
```

## Running INTcoin

### Start the daemon
```bash
./intcoind
```

### Start with testnet
```bash
./intcoind -testnet
```

### Use the Qt wallet
```bash
./intcoin-qt
```

### Use the CLI
```bash
./intcoin-cli help
./intcoin-cli getblockchaininfo
./intcoin-cli getbalance
```

### CPU Mining
```bash
# Mine from wallet
./intcoin-qt  # Enable mining in GUI

# Or use standalone miner
./intcoin-miner -t 4  # Use 4 threads
```

## Advanced Features

### Lightning Network

INTcoin includes a fully-featured Lightning Network implementation for instant, low-cost transactions:

**Current Features:**
- Payment channels with HTLC support
- Multi-hop routing with onion encryption
- BOLT #11 invoice support
- Channel management (open, close, force-close)
- Real-time fee estimation

**Future Enhancements:**
- **Watchtowers**: Third-party monitoring services for channel security
- **Submarine Swaps**: Seamless on-chain ↔ off-chain conversions
- **Atomic Multi-Path Payments (AMP)**: Split large payments across multiple routes
- **Trampoline Routing**: Lightweight routing for mobile clients
- **Channel Factories**: Batch channel creation for efficiency
- **Splicing**: Dynamic channel capacity adjustments without closing
- **Dual-Funded Channels**: Both parties contribute to initial funding

See [src/lightning/README.md](src/lightning/README.md) for complete documentation.

### Smart Contracts

Secure virtual machine with comprehensive safety features:
- Gas metering to prevent infinite loops
- SafeMath for overflow protection
- Memory and stack safety
- ERC20-compatible token support
- Security analyzer for vulnerability detection

See [src/contracts/README.md](src/contracts/README.md) for contract development guide.

### Cross-Chain Bridges

Trustless atomic swaps with other blockchains:
- Bitcoin bridge with SPV proofs
- Ethereum bridge with smart contract integration
- Hash Time Locked Contracts (HTLC)
- Multi-signature security

See [src/bridge/README.md](src/bridge/README.md) for usage examples.

## Testing

```bash
# Run Python test suite
cd tests
python3 -m pytest

# Run C++ unit tests
./build/tests/intcoin-tests

# Run Lightning Network tests
./build/tests/test_lightning

# Run smart contract VM tests
./build/tests/test_contracts

# Run bridge tests
./build/tests/test_bridge
```

## Network Types

### Mainnet
Production network with real value.

### Testnet
Testing network with test coins (no real value).
```bash
./intcoind -testnet
```

## Documentation

See the [docs](docs/) directory for detailed documentation:
- [Architecture](docs/architecture.md)
- [API Reference](docs/api.md)
- [Mining Guide](docs/mining.md)
- [Wallet Guide](docs/wallet.md)
- [Lightning Network](src/lightning/README.md) - Layer 2 payment channels
- [Smart Contracts](src/contracts/README.md) - Secure VM and contract development
- [Cross-Chain Bridges](src/bridge/README.md) - Atomic swaps with other chains
- [Mining Pools](src/pool/README.md) - Stratum protocol implementation

## Security

### Security Features Implemented

INTcoin implements comprehensive security measures throughout the codebase:

- ✅ **Input validation on all external data**: All RPC commands, P2P messages, and contract calls validate inputs
- ✅ **Integer overflow protection**: SafeMath library for all arithmetic operations (returns `std::optional`)
- ✅ **Memory safety**: Bounds checking on all array/memory access, no buffer overflows

### Security Reporting

If you discover a security vulnerability, please email security@international-coin.org with details. Do not open a public issue.

All security communications should be encrypted with our GPG key:
```
50FA 6D8F 2359 DBD9 3BA7 6263 1916 8ED3 FF91 FF72
```

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Known Issues and TODOs

The following items need to be addressed before production release:

- ⚠️ **Block serialization is placeholder**: Current implementation needs proper versioning and validation
- ⚠️ **Wallet encryption is placeholder XOR**: Must implement AES-256-GCM encryption before mainnet
- ⚠️ **Full reorg support needs undo data**: Blockchain reorganization requires transaction undo information

### Current Development Phase

- **Phase 12**: Full P2P Implementation (in progress)
  - Enhanced peer discovery
  - Connection management improvements
  - Network protocol optimizations

- **Phase 13**: Testing & Production Readiness (planned)
  - Comprehensive security audits
  - Stress testing and load testing
  - Testnet deployment and validation
  - Production hardening

## Roadmap

See [ROADMAP.md](ROADMAP.md) for our five-year development plan.

## Design Decisions

See [DESIGN-DECISIONS.md](DESIGN-DECISIONS.md) for detailed explanations of architectural choices.

## Standards Compliance

INTcoin adheres to the following standards:
- **NIST**: Post-Quantum Cryptography Standards
- **FIPS 140-3**: Cryptographic Module Validation
- **FIPS 186-5**: Digital Signature Standard
- **FIPS 202**: SHA-3 Standard
- **RFC 8391**: XMSS (Extended Merkle Signature Scheme)

## License

INTcoin is released under the terms of the MIT license. See [COPYING](COPYING) for more information.

---

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**
