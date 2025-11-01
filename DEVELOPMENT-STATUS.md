# INTcoin Development Status

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

**Version**: 0.1.0-alpha
**Last Updated**: January 2025

---

## Project Overview

INTcoin is a quantum-resistant, ASIC-resistant cryptocurrency built with C++23, Python 3, and CMake. This document tracks the current development status and outlines next steps.

---

## Completed Components

### ✅ Project Infrastructure

- [x] Complete folder structure
- [x] CMake build system (v3.20+)
- [x] C++23 configuration
- [x] Git repository structure
- [x] .gitignore configuration
- [x] MIT license (COPYING file)
- [x] Version management system
- [x] Build configuration system

### ✅ Documentation

- [x] README.md with project overview
- [x] COPYING (MIT License)
- [x] DESIGN-DECISIONS.md
- [x] ROADMAP.md (5-year plan)
- [x] REWARDS-PROGRAM.md (multiple distribution models)
- [x] REWARDS-4YEAR-HALVING.md (Bitcoin-style halving)
- [x] BUILD-WINDOWS.md (complete Windows build guide)
- [x] INSTALL-LINUX.md (all major distros)
- [x] INSTALL-FREEBSD.md (comprehensive FreeBSD guide)
- [x] Development status tracking (this file)

### ✅ Core Headers & Architecture

- [x] primitives.h - Fundamental types and constants
- [x] block.h - Block and block header structures
- [x] transaction.h - Transaction and UTXO model
- [x] merkle.h - Merkle tree implementation
- [x] crypto.h - Quantum-resistant cryptography interfaces
- [x] Version information headers (version.h.in)
- [x] Build configuration (config.h.in)

### ✅ Configuration Files

- [x] Mainnet configuration (config/mainnet/intcoin.conf)
- [x] Testnet configuration (config/testnet/intcoin.conf)
- [x] Network parameters defined
- [x] Block reward schedules designed

### ✅ Tooling

- [x] header.py - Automated copyright header management
  - Handles C++ and Python files
  - Updates copyright year ranges automatically
  - Ignores build artifacts and dependencies

### ✅ External Dependencies Setup

- [x] RandomX integration stub (ASIC-resistant PoW)
- [x] CRYSTALS-Dilithium stub (quantum-resistant signatures)
- [x] CRYSTALS-Kyber stub (quantum-resistant key exchange)
- [x] CMake integration for all external libraries

### ✅ macOS Dependencies

- [x] Homebrew packages installed:
  - cmake 4.1.2
  - boost 1.89.0
  - openssl@3 3.6.0
  - qt@5 5.15.17
  - python@3.12

---

## In Progress Components

### 🔄 Cryptography Implementation

**Status**: Header files complete, implementations needed

**Next Steps**:
1. Integrate actual CRYSTALS-Dilithium library
   - Option A: Use liboqs (https://github.com/open-quantum-safe/liboqs)
   - Option B: Use NIST reference implementation
2. Integrate actual CRYSTALS-Kyber library
3. Implement SHA3-256 using OpenSSL or Keccak reference
4. Implement address generation (Base58Check encoding)
5. Implement secure random number generation
6. Implement HKDF key derivation
7. Implement BIP39-style mnemonic generation

**Files to Create**:
- src/crypto/sha3.cpp
- src/crypto/dilithium.cpp
- src/crypto/kyber.cpp
- src/crypto/address.cpp
- src/crypto/random.cpp
- src/crypto/hkdf.cpp
- src/crypto/mnemonic.cpp

---

## Not Yet Started

### 🔲 Core Blockchain

**Priority**: HIGH

**Components**:
- [ ] Block serialization/deserialization
- [ ] Block validation logic
- [ ] Genesis block creation
- [ ] Block reward calculation
- [ ] Blockchain state management
- [ ] Chain reorganization handling
- [ ] Block storage (LevelDB/RocksDB)

**Files to Create**:
- src/core/block.cpp
- src/core/blockchain.cpp
- src/core/blockindex.cpp
- src/core/validation.cpp
- src/core/genesis.cpp
- include/intcoin/blockchain.h

### 🔲 Transaction Pool (Mempool)

**Priority**: HIGH

**Components**:
- [ ] Transaction validation
- [ ] UTXO set management
- [ ] Transaction serialization
- [ ] Fee calculation
- [ ] Replace-by-fee logic
- [ ] Transaction prioritization

**Files to Create**:
- src/core/transaction.cpp
- src/core/mempool.cpp
- src/core/utxo.cpp
- include/intcoin/mempool.h

### 🔲 Merkle Tree Implementation

**Priority**: MEDIUM

**Components**:
- [ ] Merkle tree building
- [ ] Merkle proof generation
- [ ] Merkle proof verification
- [ ] SPV client support

**Files to Create**:
- src/core/merkle.cpp

### 🔲 Consensus Engine

**Priority**: HIGH

**Components**:
- [ ] RandomX PoW integration
- [ ] Difficulty adjustment algorithm
- [ ] Block time targeting
- [ ] Chain work calculation
- [ ] Fork detection and resolution

**Files to Create**:
- src/consensus/pow.cpp
- src/consensus/difficulty.cpp
- src/consensus/validation.cpp
- include/intcoin/consensus.h

### 🔲 P2P Network Layer

**Priority**: HIGH

**Components**:
- [ ] Peer discovery
- [ ] Connection management
- [ ] Message protocol
- [ ] Block propagation
- [ ] Transaction relay
- [ ] Inventory system
- [ ] Bloom filters (SPV)

**Files to Create**:
- src/network/net.cpp
- src/network/peer.cpp
- src/network/protocol.cpp
- src/network/messages.cpp
- include/intcoin/network.h

### 🔲 Wallet Backend

**Priority**: HIGH

**Components**:
- [ ] Key management
- [ ] Address generation
- [ ] Transaction creation
- [ ] Transaction signing
- [ ] Balance calculation
- [ ] Coin selection
- [ ] HD wallet (BIP32-style)

**Files to Create**:
- src/wallet/wallet.cpp
- src/wallet/keystore.cpp
- src/wallet/coinselection.cpp
- include/intcoin/wallet.h

### 🔲 RPC Server

**Priority**: MEDIUM

**Components**:
- [ ] JSON-RPC server
- [ ] Command routing
- [ ] Authentication
- [ ] API endpoints
- [ ] Help system

**Files to Create**:
- src/rpc/server.cpp
- src/rpc/blockchain.cpp
- src/rpc/wallet.cpp
- src/rpc/mining.cpp
- src/rpc/network.cpp

### 🔲 Qt GUI Wallet

**Priority**: MEDIUM

**Components**:
- [ ] Main window
- [ ] Send/receive forms
- [ ] Transaction history
- [ ] Address book
- [ ] Settings dialog
- [ ] Mining interface
- [ ] Light/dark themes
- [ ] Cross-platform support

**Files to Create**:
- src/qt/main.cpp
- src/qt/mainwindow.cpp
- src/qt/sendcoins.cpp
- src/qt/receivecoins.cpp
- src/qt/transactionlist.cpp
- src/qt/addressbook.cpp
- src/qt/settings.cpp

### 🔲 CPU Miner

**Priority**: MEDIUM

**Components**:
- [ ] RandomX integration
- [ ] Thread management
- [ ] Hash rate calculation
- [ ] Pool protocol (Stratum)
- [ ] Solo mining support

**Files to Create**:
- src/miner/miner.cpp
- src/miner/main.cpp
- include/intcoin/miner.h

### 🔲 Lightning Network

**Priority**: LOW (Phase 2)

**Components**:
- [ ] Payment channels
- [ ] HTLC implementation
- [ ] Routing protocol
- [ ] Invoice generation
- [ ] Channel management

**Files to Create**:
- src/lightning/channel.cpp
- src/lightning/htlc.cpp
- src/lightning/routing.cpp

### 🔲 Smart Contracts

**Priority**: LOW (Phase 2)

**Components**:
- [ ] VM design
- [ ] Bytecode interpreter
- [ ] Gas mechanism
- [ ] Contract storage
- [ ] Event system

**Files to Create**:
- src/contracts/vm.cpp
- src/contracts/interpreter.cpp
- src/contracts/storage.cpp

### 🔲 Cross-Chain Bridge

**Priority**: LOW (Phase 3)

**Components**:
- [ ] Atomic swaps
- [ ] SPV proofs
- [ ] Relay contracts
- [ ] Bridge validators

**Files to Create**:
- src/bridge/swap.cpp
- src/bridge/spv.cpp
- src/bridge/relay.cpp

### 🔲 Block Explorer

**Priority**: LOW

**Components**:
- [ ] Web interface
- [ ] Block viewing
- [ ] Transaction lookup
- [ ] Address history
- [ ] Network stats

**Files to Create**:
- src/explorer/main.cpp
- src/explorer/api.cpp

### 🔲 Testing

**Priority**: HIGH

**Components**:
- [ ] Unit tests (C++)
- [ ] Integration tests (Python)
- [ ] Functional tests (Python)
- [ ] Regression tests
- [ ] Fuzz testing

**Directories to Populate**:
- tests/unit/
- tests/integration/
- tests/functional/

---

## Immediate Next Steps (Priority Order)

### Week 1: Core Cryptography
1. ✅ ~~Install dependencies~~ (COMPLETED)
2. Integrate liboqs for Dilithium and Kyber
3. Implement SHA3-256 wrapper
4. Implement address generation
5. Write crypto unit tests

### Week 2: Basic Blockchain
1. Implement block serialization
2. Implement transaction serialization
3. Create genesis block
4. Implement basic validation
5. Write blockchain unit tests

### Week 3: UTXO & Mempool
1. Implement UTXO set
2. Implement mempool
3. Transaction validation
4. Coin selection
5. Write transaction tests

### Week 4: RandomX Integration
1. Integrate RandomX library
2. Implement PoW validation
3. Implement difficulty adjustment
4. Mining algorithm tests
5. Benchmark performance

### Week 5-6: P2P Networking
1. Message protocol
2. Peer discovery
3. Block propagation
4. Transaction relay
5. Network tests

### Week 7-8: Wallet & RPC
1. Wallet backend
2. RPC server
3. Key management
4. Transaction creation
5. Integration tests

### Week 9-10: Qt GUI
1. Main window UI
2. Send/receive forms
3. Transaction history
4. Settings/preferences
5. Mining interface

### Week 11-12: Testing & Polish
1. Comprehensive test suite
2. Bug fixes
3. Documentation updates
4. Performance optimization
5. Security audit preparation

---

## External Library Integration Guide

### RandomX (ASIC-Resistant PoW)

```bash
cd external/randomx
git clone https://github.com/tevador/RandomX.git .
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### liboqs (Quantum-Safe Cryptography)

```bash
# Install liboqs
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install

# Update CMakeLists.txt to link against liboqs
target_link_libraries(intcoin_core PUBLIC oqs)
```

### Alternative: Manual CRYSTALS Integration

```bash
# Dilithium
cd external/dilithium
git clone https://github.com/pq-crystals/dilithium.git src
cd src/ref
make

# Kyber
cd external/kyber
git clone https://github.com/pq-crystals/kyber.git src
cd src/ref
make
```

---

## Build System Status

### Current State

The CMake build system is configured with:
- C++23 standard requirement
- Cross-platform support (macOS, Linux, FreeBSD, Windows)
- Modular source organization
- Optional component building
- CPack packaging support

### Known Issues

1. External libraries (RandomX, Dilithium, Kyber) are stubs
2. Source files (.cpp) need to be created
3. CMakeLists.txt in subdirectories need completion
4. Platform-specific flags may need tuning

---

## Testing Strategy

### Unit Tests (C++ with Catch2 or Google Test)

```cpp
// Example: tests/unit/test_crypto.cpp
#include <catch2/catch.hpp>
#include <intcoin/crypto.h>

TEST_CASE("SHA3-256 hashing", "[crypto]") {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto hash = intcoin::crypto::SHA3_256::hash(data);
    REQUIRE(hash.size() == 32);
}
```

### Integration Tests (Python)

```python
# Example: tests/integration/test_blockchain.py
import pytest
from test_framework import IntcoinTestFramework

class BlockchainTest(IntcoinTestFramework):
    def run_test(self):
        # Generate some blocks
        self.nodes[0].generate(100)

        # Verify blockchain
        info = self.nodes[0].getblockchaininfo()
        assert info['blocks'] == 100
```

---

## Performance Goals

### Target Metrics

- **Block Time**: 2 minutes (120 seconds) average
- **Transaction Throughput**: 100+ TPS (base layer)
- **Block Propagation**: <5 seconds (95th percentile)
- **Memory Usage**: <2GB for full node
- **Sync Speed**: 1000+ blocks/second
- **Mining Hash Rate**: Optimized for CPU (RandomX)

---

## Security Checklist

- [ ] All cryptographic operations use constant-time algorithms
- [ ] Private keys are zeroed after use
- [ ] Input validation on all external data
- [ ] Integer overflow protection
- [ ] Memory safety (no buffer overflows)
- [ ] Secure random number generation
- [ ] Protection against timing attacks
- [ ] Regular security audits scheduled

---

## Contributors

**Lead Developer**: Maddison Lane

**Contact**:
- Email: team@international-coin.org
- Security: security@international-coin.org
- Repository: https://gitlab.com/intcoin/crypto

---

## License

INTcoin is released under the MIT License. See [COPYING](COPYING) for details.

---

**Next Update**: Weekly (every Monday)
**Next Milestone**: Q1 2025 - Alpha Release (v0.1.0)
