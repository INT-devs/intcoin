# Future Enhancements (v2.0+)

This document tracks features and improvements that are deferred to future versions of INTcoin.

## Overview

The following features have been intentionally left for future development:
- They require significant architectural changes
- They are optimizations for production scale
- They are enhancements beyond the MVP feature set
- They will be added as the network matures

---

## Blockchain & Consensus

### Script Engine (v2.0)
**Status**: Foundation implemented, full engine deferred  
**Files**: `src/blockchain/script.cpp`

- [ ] Complete P2PKH/P2PK script execution engine
- [ ] Script signature validation (opcodes)
- [ ] Human-readable script representation
- [ ] Script debugging tools

**Rationale**: Current transaction validation uses Dilithium3 signatures directly. Full script engine adds flexibility for smart contracts but is not required for basic operations.

### Block Validation Optimizations (v2.1)
**Status**: Basic validation complete, optimizations deferred  
**Files**: `src/blockchain/block.cpp`, `src/blockchain/validation.cpp`

- [ ] Full merkle tree building and proof generation
- [ ] Merkle branch verification
- [ ] Median time past validation
- [ ] Enhanced fee calculation from transactions

**Rationale**: Current validation is sufficient for network operation. These optimizations improve performance at scale.

### Checkpoints (Ongoing)
**Status**: Infrastructure complete, checkpoints added as network matures  
**Files**: `src/consensus/consensus.cpp:616`

- [ ] Add weekly checkpoints (~5040 blocks)
- [ ] Add monthly checkpoints (~21600 blocks)

**Rationale**: Checkpoints are added after blocks are confirmed stable. Network must run before checkpoints can be established.

---

## Storage & Database

### Advanced UTXO Indexing (v2.0)
**Status**: Basic UTXO operations complete  
**Files**: `src/storage/storage.cpp:625`

- [ ] Address UTXO lookup with index
- [ ] Transaction indexing for faster queries
- [ ] Address transaction history lookup

**Rationale**: Requires additional database indices. Current implementation is sufficient for smaller chain sizes.

### Database Management (v2.1)
**Status**: Core operations complete  
**Files**: `src/storage/storage.cpp:749`

- [ ] Block pruning for reduced storage
- [ ] Database verification and repair tools
- [ ] Automated backup functionality
- [ ] Database size optimization

**Rationale**: Important for long-term node operation but not critical for initial network launch.

---

## Network & P2P

### Orphan Block Handling (v2.0)
**Status**: Basic handling implemented  
**Files**: `src/network/network.cpp:1445`

- [ ] Orphan block pool for out-of-order blocks
- [ ] Request missing parent blocks automatically
- [ ] Orphan cleanup and limits

**Rationale**: Current implementation processes blocks in order. Orphan pool improves sync resilience.

### Transaction Relay Optimization (v2.0)
**Status**: Basic relay implemented  
**Files**: `src/network/network.cpp:1543`

- [ ] Transaction inventory announcements
- [ ] Bloom filters for SPV clients
- [ ] Compact block relay (BIP 152)

**Rationale**: Optimizations for bandwidth efficiency at scale.

### Ping/Pong Nonce Verification (v2.1)
**Status**: Basic keep-alive implemented  
**Files**: `src/network/network.cpp:1900`

- [ ] Add `last_ping_nonce` field to Peer structure
- [ ] Store sent PING nonce
- [ ] Verify PONG nonce matches
- [ ] Detect and ban misbehaving peers

**Rationale**: Requires structural changes to Peer class. Current implementation provides basic keep-alive functionality.

---

## Wallet & User Interface

### Wallet Enhancements (v2.0)
**Status**: Core wallet features complete  
**Files**: `src/wallet/wallet.cpp:2079`

- [ ] Auto-lock timeout implementation
- [ ] Multi-signature wallets
- [ ] Hardware wallet support (Ledger, Trezor)

**Rationale**: Advanced features for improved security and usability.

---

## Lightning Network

### Full BOLT Specification (v2.0)
**Status**: Foundation implemented  
**Files**: `src/lightning/lightning.cpp:604`

- [ ] Complete BOLT #2 (Peer Protocol)
- [ ] Multi-path payments (MPP)
- [ ] Atomic Multi-Path (AMP)
- [ ] Watchtower integration

**Rationale**: Current implementation provides basic Lightning functionality. Full BOLT compliance requires extensive testing.

---

## Build System

### C++26 Upgrade (v3.0)
**Status**: Currently using C++23  
**Files**: `CMakeLists.txt:31`

- [ ] Upgrade to C++26 when compiler support is widespread
- [ ] Utilize new language features
- [ ] Update build system

**Rationale**: C++26 not yet finalized. Will upgrade when stable and widely supported.

---

## Logging System

### Centralized Logging (v2.0)
**Status**: Console output implemented  
**Files**: `src/consensus/consensus.cpp:654`, `src/blockchain/blockchain.cpp:593`

- [ ] Implement proper logging framework (spdlog or similar)
- [ ] Log levels (DEBUG, INFO, WARN, ERROR)
- [ ] Log rotation and archival
- [ ] Performance metrics logging

**Rationale**: Current console output is sufficient for development. Production deployment needs structured logging.

---

## Priority & Timeline

### High Priority (v2.0 - Q2 2026)
1. Script engine completion
2. Advanced UTXO indexing
3. Orphan block pool
4. Centralized logging

### Medium Priority (v2.1 - Q3 2026)
1. Database pruning and management
2. Transaction relay optimizations
3. Wallet enhancements

### Low Priority (v2.2+ - Q4 2026)
1. Full BOLT specification
2. C++26 upgrade

---

## Contributing

Want to help implement these features? See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

## Notes

- All features marked for v2.0+ are tracked in GitLab issues
- Community feedback will prioritize development
- Security and stability take precedence over new features
- Features may be promoted or demoted based on network needs

---

**Last Updated**: December 23, 2025  
**Next Review**: After mainnet launch (Q1 2026)
