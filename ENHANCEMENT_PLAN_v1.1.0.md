# INTcoin v1.1.0-beta Enhancement Plan
**Version**: 1.1.0-beta  
**Date**: December 26, 2025  
**Status**: Planning  
**Based On**: v1.0.0-beta (Releasing January 6, 2026)

---

## Table of Contents

- [Overview](#overview)
- [Priority Classification](#priority-classification)
- [Phase 1: Lightning Network Production Hardening](#phase-1-lightning-network-production-hardening)
- [Phase 2: Mining Pool Completion](#phase-2-mining-pool-completion)
- [Phase 3: Storage & Database Enhancements](#phase-3-storage--database-enhancements)
- [Phase 4: Core Blockchain Improvements](#phase-4-core-blockchain-improvements)
- [Phase 5: RPC & API Enhancements](#phase-5-rpc--api-enhancements)
- [Phase 6: Security & Performance](#phase-6-security--performance)
- [Timeline & Milestones](#timeline--milestones)

---

## Overview

This document outlines all remaining features and TODO items identified in the INTcoin codebase for completion in v1.1.0-beta. The plan is organized by priority and component, focusing on production-readiness and feature completion.

**Current Status** (v1.0.0-beta):
- ✅ 100% test coverage (13/13 tests)
- ✅ Lightning Network core implementation
- ✅ Mining pool basic functionality
- ✅ 47+ RPC methods
- ✅ Complete documentation

**Target Status** (v1.1.0-beta):
- 🎯 Production-ready Lightning Network
- 🎯 Complete mining pool server (Stratum)
- 🎯 Enhanced database functionality
- 🎯 Improved performance & security
- 🎯 Additional RPC methods

---

## Priority Classification

| Priority | Description | Timeline |
|----------|-------------|----------|
| **P0 - Critical** | Required for production use | Week 1-2 |
| **P1 - High** | Important for reliability | Week 3-4 |
| **P2 - Medium** | Enhances functionality | Week 5-6 |
| **P3 - Low** | Nice to have | Week 7-8 |

---

## Phase 1: Lightning Network Production Hardening

**Priority**: P0 - Critical  
**Goal**: Make Lightning Network production-ready with proper cryptography and security

### 1.1 Cryptographic Security (P0)

**Current Issue**: Watchtower encryption uses placeholder/insecure methods

**Tasks**:
- [ ] **Implement AES-256-GCM encryption** for watchtower data
  - Location: `src/lightning/lightning.cpp`
  - Replace: `TODO: Use proper AES-256-GCM encryption` (line ~1150)
  - Replace: `TODO: Use proper AES-256-GCM decryption` (line ~1180)
  - Add OpenSSL EVP functions for AES-256-GCM
  - Implement key derivation from channel secrets
  - Test vectors from BOLT specifications

- [ ] **Implement proper key derivation (BIP32)**
  - Location: `src/lightning/lightning.cpp`
  - Replace: `TODO: Implement proper key derivation (BIP32)` (line ~1850)
  - Replace: `TODO: Derive proper funding key` (line ~660)
  - Implement HD wallet integration for Lightning keys
  - Use BIP32 derivation paths: m/44'/COIN'/0'/2/*
  - Ensure deterministic key generation

- [ ] **Implement proper hashing (SHA3-256)**
  - Location: `src/lightning/lightning.cpp`
  - Replace: `TODO: Use proper hashing (SHA3-256)` (line ~1250)
  - Use SHA3-256 for all Lightning hashes (consistent with chain)
  - Update commitment transaction hashing
  - Update HTLC preimage verification

**Estimated Effort**: 40 hours  
**Dependencies**: None  
**Tests Required**: Cryptography test suite addition

### 1.2 BOLT #3 Script Implementation (P0)

**Current Issue**: Commitment transaction scripts are placeholders

**Tasks**:
- [ ] **Implement full BOLT #3 to_local script**
  - Location: `src/lightning/lightning.cpp` (line ~510)
  - Include: CSV delay, revocation path
  - Implement proper script construction
  - Add signature verification

- [ ] **Implement full BOLT #3 HTLC scripts**
  - Location: `src/lightning/lightning.cpp` (line ~550)
  - Include: timeout/success paths
  - Include: revocation paths  
  - Implement proper unlocking scripts
  - Test with various HTLC scenarios

- [ ] **Implement proper 2-of-2 multisig script**
  - Location: `src/lightning/lightning.cpp` (line ~730)
  - Use both funding pubkeys
  - Ensure compatibility with BOLT #3
  - Test funding transaction creation

**Estimated Effort**: 60 hours  
**Dependencies**: Key derivation  
**Tests Required**: BOLT #3 script validation tests

### 1.3 Transaction Signing & Broadcasting (P0)

**Current Issue**: Many signature operations are placeholders

**Tasks**:
- [ ] **Implement commitment transaction signing**
  - Replace all: `TODO: Actually sign` placeholders
  - Location: `src/lightning/lightning.cpp` (multiple locations)
  - Sign commitment transactions with Dilithium3
  - Sign HTLC outputs
  - Verify counterparty signatures

- [ ] **Implement broadcasting to blockchain**
  - Replace: `TODO: Broadcast local commitment transaction`
  - Replace: `TODO: Actually broadcast funding_tx`
  - Integrate with blockchain RPC
  - Handle broadcast failures
  - Monitor transaction confirmations

- [ ] **Implement proper closing scripts**
  - Replace: `TODO: Use proper closing script` (multiple)
  - Generate P2PKH/P2SH closing addresses
  - Support custom scriptpubkeys
  - Calculate proper closing fees

**Estimated Effort**: 50 hours  
**Dependencies**: BOLT #3 scripts  
**Tests Required**: Signing and broadcast integration tests

### 1.4 Fee & Value Calculations (P1)

**Current Issue**: Fee calculations are hardcoded or missing

**Tasks**:
- [ ] **Implement dynamic fee estimation**
  - Replace: `TODO: Get actual feerate from mempool` (line ~680)
  - Integrate with fee estimation RPC
  - Use mempool statistics
  - Adjust based on confirmation target

- [ ] **Implement proper fee calculations**
  - Replace: `TODO: Calculate based on feerate` (line ~810)
  - Replace: `TODO: Subtract transaction fee` (line ~1240)
  - Calculate closing transaction fees
  - Account for witness size
  - Ensure fee fairness in mutual close

**Estimated Effort**: 20 hours  
**Dependencies**: Fee estimation RPC  
**Tests Required**: Fee calculation unit tests

### 1.5 Watchtower Enhancements (P1)

**Current Issue**: Watchtower configuration incomplete

**Tasks**:
- [ ] **Implement watchtower operator configuration**
  - Replace: `TODO: Configure watchtower operator destination` (2 locations)
  - Add configuration file support
  - Support multiple watchtower endpoints
  - Implement watchtower authentication

- [ ] **Implement revocation signature creation**
  - Replace: `TODO: Create proper unlocking script with revocation signature`
  - Generate penalty transaction signatures
  - Store revocation secrets securely
  - Automate penalty transaction broadcast

**Estimated Effort**: 30 hours  
**Dependencies**: AES-256-GCM encryption  
**Tests Required**: Watchtower breach detection tests

### 1.6 Error Handling & Encoding (P2)

**Current Issue**: Error encoding is placeholder

**Tasks**:
- [ ] **Implement BOLT #4 error encoding**
  - Replace: `TODO: Proper error encoding` (multiple)
  - Implement onion error packet format
  - Support all BOLT #4 failure codes
  - Add error propagation for forwarded HTLCs

- [ ] **Implement payment detail verification**
  - Replace: `TODO: In production, verify payment details against invoice`
  - Verify invoice amounts
  - Check invoice expiration
  - Validate payment hash

**Estimated Effort**: 25 hours  
**Dependencies**: None  
**Tests Required**: Error handling tests

### 1.7 Network Graph Enhancements (P2)

**Current Issue**: Missing signature verification and iteration methods

**Tasks**:
- [ ] **Implement BOLT #7 signature verification**
  - Replace: `TODO: Verify signatures (BOLT #7 signature verification)` (3 locations)
  - Verify node announcements
  - Verify channel announcements
  - Verify channel updates
  - Use Dilithium3 for verification

- [ ] **Implement network graph iteration**
  - Add: `GetAllNodes()` method
  - Add: `GetAllChannels()` method
  - Support efficient graph traversal
  - Enable RPC queries

- [ ] **Implement channel verification on blockchain**
  - Replace: `TODO: Verify channel exists on blockchain`
  - Query blockchain for funding transaction
  - Verify confirmation depth
  - Update channel status

**Estimated Effort**: 35 hours  
**Dependencies**: Blockchain RPC integration  
**Tests Required**: Network graph tests

---

## Phase 2: Mining Pool Completion

**Priority**: P0-P1  
**Goal**: Complete Stratum protocol implementation for production mining

### 2.1 Stratum Protocol Implementation (P0)

**Current Issue**: Stratum message handling incomplete

**Tasks**:
- [ ] **Implement mining.subscribe handler**
  - Location: `src/pool/mining_pool_server.cpp`
  - Replace: `TODO: Handle mining.subscribe`
  - Send subscription response
  - Assign extranonce1 to miner
  - Set up worker session

- [ ] **Implement mining.authorize handler**
  - Replace: `TODO: Handle mining.authorize`
  - Validate worker credentials
  - Store worker information
  - Send authorization response

- [ ] **Implement mining.submit handler**
  - Replace: `TODO: Handle mining.submit`
  - Validate share submission
  - Check share difficulty
  - Update worker statistics
  - Credit shares to account

- [ ] **Implement Stratum notifications**
  - Replace: `TODO: Send mining.notify`
  - Replace: `TODO: Send mining.set_difficulty`
  - Broadcast new block templates
  - Send difficulty updates
  - Handle VarDiff adjustments

- [ ] **Implement Stratum message routing**
  - Replace: `TODO: Parse and route Stratum message`
  - Parse JSON-RPC Stratum messages
  - Route to appropriate handlers
  - Handle malformed requests

**Estimated Effort**: 60 hours  
**Dependencies**: Pool database  
**Tests Required**: Stratum protocol conformance tests

### 2.2 Payout Systems (P0)

**Current Issue**: Payout calculations not implemented

**Tasks**:
- [ ] **Implement PPLNS payout calculation**
  - Location: `src/pool/mining_pool_server.cpp`
  - Replace: `TODO: Implement PPLNS payout calculation`
  - Calculate share windows (N last shares)
  - Proportional distribution
  - Handle block maturity

- [ ] **Implement PPS payout calculation**
  - Replace: `TODO: Implement PPS payout calculation`
  - Fixed payout per share
  - Pool variance risk
  - Instant payout support

- [ ] **Implement payout processing**
  - Replace: `TODO: Implement payout processing`
  - Generate payout transactions
  - Batch payments for efficiency
  - Track payment history
  - Handle failed payments

- [ ] **Implement proper wallet integration**
  - Replace: `TODO: Use proper wallet/keypair for pool rewards`
  - Integrate with INTcoin wallet
  - Secure pool private keys
  - Automate reward distribution

**Estimated Effort**: 50 hours  
**Dependencies**: Wallet integration  
**Tests Required**: Payout calculation tests

### 2.3 Pool Statistics & Monitoring (P1)

**Current Issue**: Statistics incomplete

**Tasks**:
- [ ] **Implement aggregated worker statistics**
  - Location: `src/pool/pool_database.cpp`
  - Replace: `TODO: Implement aggregated worker stats`
  - Total hashrate per worker
  - Share submission rates
  - Rejection rates
  - Historical data

- [ ] **Implement pool-wide statistics**
  - Replace: `TODO: Populate statistics` (mining_pool_server.cpp)
  - Network vs pool hashrate
  - Block finding rate
  - Payout totals
  - Active miners count

- [ ] **Implement HTTP API statistics**
  - Location: `src/pool/http_api.cpp`
  - Replace: `TODO: Get actual finder` (block finder tracking)
  - Replace: `TODO: Check actual status` (block status)
  - Real-time pool dashboard data
  - JSON API endpoints

**Estimated Effort**: 30 hours  
**Dependencies**: Database implementation  
**Tests Required**: Statistics accuracy tests

### 2.4 Difficulty Adjustment (P1)

**Current Issue**: Difficulty updates incomplete

**Tasks**:
- [ ] **Implement Stratum difficulty broadcast**
  - Location: `src/pool/pool.cpp`
  - Replace: `TODO: Send difficulty update via Stratum`
  - Broadcast to all connected miners
  - Per-worker difficulty
  - VarDiff algorithm integration

- [ ] **Implement dynamic difficulty broadcast**
  - Location: `src/pool/mining_pool_server.cpp`
  - Replace: `TODO: Implement Stratum broadcast`
  - Multicast to worker connections
  - Handle disconnected workers
  - Queue updates during reconnection

**Estimated Effort**: 20 hours  
**Dependencies**: Stratum protocol  
**Tests Required**: Difficulty adjustment tests

---

## Phase 3: Storage & Database Enhancements

**Priority**: P1-P2  
**Goal**: Complete database functionality for scalability

### 3.1 UTXO Set Management (P1)

**Current Issue**: UTXO operations incomplete

**Tasks**:
- [ ] **Implement UTXO set loading from database**
  - Location: `src/storage/storage.cpp`
  - Replace: `TODO: Load UTXOs from database`
  - Efficient batch loading
  - Progressive loading for large sets
  - Cache management

- [ ] **Implement address UTXO filtering**
  - Replace: `TODO: Implement address filtering based on script_pubkey`
  - Index UTXOs by address
  - Support address queries
  - Optimize lookup performance

**Estimated Effort**: 25 hours  
**Dependencies**: None  
**Tests Required**: UTXO set tests

### 3.2 Transaction Indexing (P1)

**Current Issue**: Transaction lookup not implemented

**Tasks**:
- [ ] **Implement transaction indexing**
  - Location: `src/storage/storage.cpp`
  - Replace: `TODO: Implement transaction indexing`
  - Index transactions by hash
  - Support txid lookup
  - Enable tx-by-block queries

- [ ] **Implement address transaction lookup**
  - Replace: `TODO: Implement address transaction lookup`
  - Index transactions by involved addresses
  - Support address history queries
  - Pagination for large histories

**Estimated Effort**: 35 hours  
**Dependencies**: Database schema updates  
**Tests Required**: Transaction index tests

### 3.3 Block Management (P2)

**Current Issue**: Block operations incomplete

**Tasks**:
- [ ] **Implement block pruning**
  - Location: `src/storage/storage.cpp`
  - Replace: `TODO: Implement block pruning`
  - Prune old block data
  - Keep headers and UTXO set
  - Configurable pruning depth

- [ ] **Implement block reversion for reorgs**
  - Replace: `TODO: Implement block reversion`
  - Reverse UTXO changes
  - Restore spent outputs
  - Handle deep reorganizations

**Estimated Effort**: 40 hours  
**Dependencies**: UTXO set management  
**Tests Required**: Blockchain reorganization tests

### 3.4 Database Utilities (P2)

**Current Issue**: Database utilities missing

**Tasks**:
- [ ] **Implement database size calculation**
  - Location: `src/storage/storage.cpp`
  - Replace: `TODO: Implement database size calculation`
  - Calculate total DB size
  - Report per-component sizes
  - Disk space monitoring

- [ ] **Implement database verification**
  - Replace: `TODO: Implement database verification`
  - Verify block chain integrity
  - Check UTXO set consistency
  - Detect corruption

- [ ] **Implement database backup**
  - Replace: `TODO: Implement database backup`
  - Hot backup support
  - Snapshot creation
  - Restore functionality

- [ ] **Implement cache flushing**
  - Replace: `TODO: Flush cache to database`
  - Periodic cache writes
  - Graceful shutdown flush
  - WAL (Write-Ahead Logging)

**Estimated Effort**: 30 hours  
**Dependencies**: None  
**Tests Required**: Database utility tests

---

## Phase 4: Core Blockchain Improvements

**Priority**: P1-P2  
**Goal**: Enhance blockchain validation and handling

### 4.1 Validation Enhancements (P1)

**Current Issue**: Additional validation needed

**Tasks**:
- [ ] **Add comprehensive block validation**
  - Location: `src/blockchain/blockchain.cpp`
  - Replace: `TODO: Add more validation`
  - Validate merkle root
  - Check block weight limits
  - Verify coinbase maturity
  - Validate witness data

- [ ] **Implement script validation**
  - Location: `src/blockchain/validation.cpp`
  - Replace: `TODO: Validate script is well-formed`
  - Check script syntax
  - Validate opcodes
  - Prevent malformed scripts

**Estimated Effort**: 25 hours  
**Dependencies**: None  
**Tests Required**: Extended validation tests

### 4.2 Orphan Block Handling (P1)

**Current Issue**: No orphan pool implementation

**Tasks**:
- [ ] **Implement orphan block pool**
  - Location: `src/network/network.cpp`
  - Replace: `TODO: Implement orphan pool to handle out-of-order blocks`
  - Store orphan blocks temporarily
  - Request missing parent blocks
  - Process orphans when parents arrive
  - Prune old orphans

**Estimated Effort**: 30 hours  
**Dependencies**: None  
**Tests Required**: Out-of-order block tests

### 4.3 Transaction Relay (P2)

**Current Issue**: Transaction relay incomplete

**Tasks**:
- [ ] **Implement transaction relay to peers**
  - Location: `src/network/network.cpp`
  - Replace: `TODO: Relay transaction to other peers`
  - Broadcast valid transactions
  - Implement inv/getdata protocol
  - Prevent relay loops
  - Rate limiting

**Estimated Effort**: 20 hours  
**Dependencies**: Network protocol  
**Tests Required**: Transaction propagation tests

### 4.4 Fee Calculation (P2)

**Current Issue**: Fee calculation incomplete

**Tasks**:
- [ ] **Implement total fee calculation in blocks**
  - Location: `src/blockchain/block.cpp`
  - Replace: `TODO: Calculate total fees (requires UTXO set)`
  - Sum all transaction fees
  - Verify coinbase amount
  - Expose via RPC

- [ ] **Implement transaction fee calculation**
  - Location: `src/rpc/rpc.cpp`
  - Replace: `TODO: Calculate actual fee`
  - Calculate: inputs - outputs
  - Access UTXO set for input values
  - Return fee in RPC responses

**Estimated Effort**: 15 hours  
**Dependencies**: UTXO set  
**Tests Required**: Fee calculation tests

### 4.5 UTXO Set Integration (P1)

**Current Issue**: UTXO loading and restoration needed

**Tasks**:
- [ ] **Implement UTXO set loading**
  - Location: `src/blockchain/blockchain.cpp`
  - Replace: `TODO: Implement UTXO set loading`
  - Replace: `TODO: Implement UTXOSet properly`
  - Load on startup
  - Optimize for large sets
  - Progress reporting

- [ ] **Implement spent UTXO restoration for reorgs**
  - Replace: `TODO: Need to restore spent UTXOs from database`
  - Store spent outputs temporarily
  - Restore on chain reorganization
  - Garbage collect old spent outputs

- [ ] **Implement address UTXO lookup**
  - Replace: `TODO: Implement address UTXO lookup`
  - Query UTXOs by address
  - Support balance calculation
  - Efficient indexing

**Estimated Effort**: 35 hours  
**Dependencies**: Database improvements  
**Tests Required**: UTXO management tests

---

## Phase 5: RPC & API Enhancements

**Priority**: P2-P3  
**Goal**: Extend RPC functionality

### 5.1 Lightning Network RPC (P2)

**Current Issue**: Network graph RPC incomplete

**Tasks**:
- [ ] **Implement GetAllNodes() method**
  - Location: `src/rpc/rpc.cpp`
  - Add to NetworkGraph class
  - Return all Lightning nodes
  - Include node metadata
  - Expose via RPC

- [ ] **Implement GetAllChannels() method**
  - Add to NetworkGraph class  
  - Return all Lightning channels
  - Include channel capacity and fees
  - Expose via RPC

**Estimated Effort**: 15 hours  
**Dependencies**: Network graph enhancements  
**Tests Required**: RPC integration tests

### 5.2 Pool Server RPC (P3)

**Current Issue**: Some pool server tasks incomplete

**Tasks**:
- [ ] **Implement blockchain RPC client**
  - Location: `src/pool/intcoin-pool-server.cpp`
  - Replace: `TODO: Create blockchain RPC client`
  - Connect to intcoind RPC
  - Handle RPC failures
  - Reconnection logic

- [ ] **Implement pool server creation**
  - Replace: `TODO: Create and start pool server`
  - Initialize pool server
  - Start Stratum listener
  - Start HTTP API server

- [ ] **Implement periodic statistics printing**
  - Replace: `TODO: Print periodic statistics`
  - Log pool statistics
  - Display to console
  - Export to monitoring systems

**Estimated Effort**: 20 hours  
**Dependencies**: Pool completion  
**Tests Required**: Pool server integration tests

---

## Phase 6: Security & Performance

**Priority**: P0-P1  
**Goal**: Production hardening

### 6.1 Logging System (P1)

**Current Issue**: No logging infrastructure

**Tasks**:
- [ ] **Implement logging system**
  - Location: Multiple (`src/consensus/consensus.cpp`, `src/blockchain/blockchain.cpp`)
  - Replace: `TODO: Add logging system` (2 locations)
  - Structured logging
  - Multiple log levels (debug, info, warn, error)
  - Log rotation
  - Async logging for performance

**Estimated Effort**: 25 hours  
**Dependencies**: None  
**Tests Required**: Logging tests

### 6.2 Wallet Encryption (P0)

**Current Issue**: Wallet encryption timeout not implemented

**Tasks**:
- [ ] **Implement wallet auto-lock timeout**
  - Location: `src/wallet/wallet.cpp`
  - Replace: `TODO: Handle timeout_seconds for auto-locking`
  - Auto-lock after timeout
  - Configurable timeout
  - Clear keys from memory

**Estimated Effort**: 10 hours  
**Dependencies**: None  
**Tests Required**: Wallet security tests

### 6.3 Performance Optimizations (P2)

**Tasks**:
- [ ] **Optimize UTXO set loading**
  - Batch database reads
  - Parallel loading
  - Memory-mapped files

- [ ] **Optimize signature verification**
  - Batch Dilithium3 verification
  - Multi-threading
  - Caching

- [ ] **Optimize network message processing**
  - Async I/O
  - Connection pooling
  - Buffer management

**Estimated Effort**: 60 hours  
**Dependencies**: All core features  
**Tests Required**: Performance benchmarks

---

## Timeline & Milestones

### Week 1-2: Lightning Network Security (P0)
- ✅ AES-256-GCM encryption
- ✅ Key derivation (BIP32)
- ✅ SHA3-256 hashing
- ✅ Basic signing infrastructure

**Deliverable**: Secure watchtower implementation

### Week 3-4: Lightning Network Completion (P0)
- ✅ BOLT #3 scripts
- ✅ Transaction signing
- ✅ Broadcasting
- ✅ Fee calculations

**Deliverable**: Production-ready Lightning Network

### Week 5-6: Mining Pool (P0-P1)
- ✅ Stratum protocol
- ✅ PPLNS/PPS payouts
- ✅ Statistics
- ✅ Difficulty management

**Deliverable**: Complete mining pool server

### Week 7-8: Database & Storage (P1)
- ✅ UTXO operations
- ✅ Transaction indexing
- ✅ Block management
- ✅ Database utilities

**Deliverable**: Scalable database layer

### Week 9-10: Blockchain Improvements (P1-P2)
- ✅ Enhanced validation
- ✅ Orphan handling
- ✅ Transaction relay
- ✅ UTXO integration

**Deliverable**: Robust blockchain core

### Week 11-12: Polish & Testing (P2-P3)
- ✅ RPC enhancements
- ✅ Logging system
- ✅ Performance optimization
- ✅ Security hardening
- ✅ Complete test coverage

**Deliverable**: v1.1.0-beta release

---

## Success Criteria

### Functional Requirements
- [ ] All P0 items complete
- [ ] 95%+ of P1 items complete
- [ ] Lightning Network fully functional in production
- [ ] Mining pool supporting 100+ concurrent miners
- [ ] Database handling 1M+ UTXOs efficiently

### Quality Requirements
- [ ] 100% test coverage maintained (13/13+ tests)
- [ ] Zero memory leaks (Valgrind clean)
- [ ] All sanitizers passing
- [ ] Performance benchmarks met:
  - Block validation: < 100ms
  - Signature verification: > 1000 ops/sec
  - UTXO lookup: < 1ms
  - Network latency: < 50ms

### Security Requirements
- [ ] All cryptographic operations production-ready
- [ ] Security audit recommendations addressed
- [ ] No TODO items in security-critical code
- [ ] Proper error handling throughout

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Lightning crypto complexity | Medium | High | Early focus, extensive testing |
| Stratum protocol compatibility | Low | Medium | Test with real miners |
| Database performance | Medium | Medium | Profiling, optimization |
| UTXO set memory usage | Medium | High | Implement pruning, optimize |
| Schedule delays | Medium | Medium | Prioritize P0/P1 items |

---

## Dependencies

### External Dependencies
- OpenSSL (for AES-256-GCM)
- RocksDB (database)
- Boost (utilities)
- liboqs (Dilithium3)

### Internal Dependencies
- Wallet HD key derivation
- Fee estimation RPC
- Blockchain RPC client
- Network protocol

---

## Testing Strategy

### Unit Tests
- Add tests for each new feature
- Cover edge cases
- Test error conditions

### Integration Tests
- End-to-end Lightning payment tests
- Mining pool workflow tests
- Blockchain reorganization tests

### Performance Tests
- Benchmark critical paths
- Memory profiling
- Network stress testing

### Security Tests
- Cryptographic verification
- Input validation
- DoS resistance

---

## Release Criteria

v1.1.0-beta will be released when:

1. ✅ All P0 items complete
2. ✅ 95%+ P1 items complete
3. ✅ 100% test coverage maintained
4. ✅ All tests passing
5. ✅ Security audit complete
6. ✅ Performance benchmarks met
7. ✅ Documentation updated
8. ✅ 2 weeks of community testing

**Target Release**: Q1 2026 (8-12 weeks after v1.0.0-beta)

---

## Resources

### Team
- Core Developers: 2-3 developers
- Security Reviewers: 1 developer
- Testing: 1 developer
- Documentation: 1 developer

### Time Estimate
- Total Development: 600-800 hours
- Testing & QA: 200 hours
- Documentation: 100 hours
- **Total**: 900-1100 hours (~12 weeks with 3 developers)

---

## Contact

**Questions or Suggestions**:
- GitHub Issues: https://github.com/INT-devs/intcoin/issues
- Discord: https://discord.gg/jCy3eNgx
- Email: team@international-coin.org

---

**Version**: 1.1.0-beta Enhancement Plan  
**Author**: INTcoin Development Team  
**Last Updated**: December 26, 2025  
**Next Review**: January 6, 2026 (after v1.0.0-beta release)
