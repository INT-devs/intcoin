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
**Status**: Foundation implemented, full spec deferred
**Files**: `src/lightning/lightning.cpp:604`

#### Current Implementation
- ✅ BOLT #8: Encrypted transport (partial)
- ✅ Basic channel establishment
- ✅ Simple payment routing
- ✅ Invoice generation/parsing (simplified)
- ✅ Post-quantum cryptography integration

#### Deferred to v2.0
- [ ] **BOLT #1**: Base Protocol - Message framing and extensions
- [ ] **BOLT #2**: Peer Protocol - Channel management lifecycle
- [ ] **BOLT #3**: Bitcoin Transaction and Script Formats (adapted for INTcoin)
- [ ] **BOLT #4**: Onion Routing Protocol - Complete implementation
- [ ] **BOLT #5**: Recommendations for On-chain Transaction Handling
- [ ] **BOLT #7**: P2P Node and Channel Discovery
- [ ] **BOLT #9**: Assigned Feature Flags
- [ ] **BOLT #10**: DNS Bootstrap and Assisted Node Location
- [ ] **BOLT #11**: Invoice Protocol - Full Bech32 encoding
- [ ] Multi-path payments (MPP/AMP)
- [ ] Watchtower integration
- [ ] Submarine swaps
- [ ] Dual-funded channels
- [ ] Anchor outputs
- [ ] Taproot channels (adapted for Dilithium)

#### Effort Estimate
**Complexity**: Very High
**Estimated Development**: 6-12 months
**Lines of Code**: ~15,000-25,000
**Testing Required**: Extensive interoperability testing

#### Why Defer?
1. **Foundation First**: Core blockchain must be stable before Layer 2
2. **Quantum Adaptation**: BOLT specs assume ECDSA, we use Dilithium3
3. **Testing Complexity**: Requires extensive network testing
4. **Resource Intensive**: Needs dedicated Lightning Network team
5. **Standards Evolution**: BOLT specs are still evolving

#### Implementation Plan (v2.0+)
1. **Phase 1** (3 months): Complete BOLT #1, #2, #3
2. **Phase 2** (2 months): BOLT #4 (Onion Routing)
3. **Phase 3** (2 months): BOLT #7, #10 (P2P Discovery)
4. **Phase 4** (2 months): BOLT #11 (Full Invoice Protocol)
5. **Phase 5** (2 months): MPP/AMP, Watchtowers
6. **Phase 6** (3 months): Testing, optimization, security audit

**Rationale**: Current implementation provides basic Lightning functionality for early testing. Full BOLT compliance requires extensive development, testing, and adaptation for post-quantum cryptography. Network must mature before deploying full Layer 2 solution.

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
