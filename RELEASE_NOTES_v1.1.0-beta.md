# INTcoin v1.1.0-beta Release Notes

**Release Date**: December 27, 2025
**Branch**: main
**Tag**: v1.1.0-beta
**Commits**: 35 new commits since previous release

---

## 🎉 Release Highlights

This release represents a major advancement in INTcoin's core functionality, bringing significant improvements to blockchain validation, network protocol, Lightning Network integration, and security features.

### Major Feature Categories

#### 1. Core Blockchain Improvements (Phase 4)
- **Comprehensive Validation** (Phase 4.1)
  - P2PKH script validation with full signature verification
  - P2SH support with script hash validation
  - Multisig script support (1-of-N to N-of-N)
  - Merkle root verification
  - Witness data validation

- **Orphan Block Handling** (Phase 4.2)
  - LRU-evicted orphan pool (100 block limit)
  - 10-minute timeout for orphan blocks
  - Automatic parent block requests via GETDATA
  - Recursive orphan processing when parents arrive

- **Transaction Relay** (Phase 4.3)
  - Bitcoin-compatible INV/GETDATA protocol
  - Relay loop prevention (excludes sender peer)
  - Automatic transaction broadcasting after mempool acceptance

- **Accurate Fee Calculation** (Phase 4.4)
  - Transaction fee calculation using UTXO lookups
  - Block total fees (sum of all non-coinbase transaction fees)
  - RPC fee reporting in getrawmempool

- **UTXO Set Integration** (Phase 4.5)
  - Database-backed UTXO set with RocksDB persistence
  - Spent output tracking for reorganization support
  - Address-based UTXO lookups for wallet queries
  - In-memory cache with periodic flushing

#### 2. RPC & API Enhancements (Phase 5)
- **Lightning Network RPC** (Phase 5.1)
  - `lightning_getnetworkgraph` RPC method
  - Complete node and channel serialization
  - Dilithium3 public key hex encoding (1952-byte keys)
  - Network topology exposure for routing

#### 3. Security & Performance (Phase 6)
- **Wallet Auto-Lock** (Phase 6.2 - P0 Priority)
  - Configurable auto-lock timeout in seconds
  - Background monitoring thread
  - Secure memory clearing with OPENSSL_cleanse
  - Thread-safe operations with mutex
  - Usage: `wallet.Unlock(passphrase, 300)` for 5-minute timeout

- **Logging System** (Phase 6.1 - P1 Priority)
  - Deep reorganization warning logs
  - Security monitoring for potential 51% attacks
  - Blockchain state change audit trail
  - Printf-style LogF() with multiple log levels

#### 4. Mining Pool Enhancements (Phase 2-3)
- Pool statistics and monitoring
- Stratum broadcast implementation
- Database utilities for pool management

---

## 📊 Statistics

### Code Changes
- **Files Modified**: 31 files
- **Lines Added**: 5,541
- **Lines Removed**: 3,365
- **Net Addition**: +2,176 lines
- **Commits**: 35

### Test Coverage
- **Test Suites**: 13/13 passing (100%)
- **Test Status**: All tests green ✅

**Test Suites**:
1. CryptoTest - Post-quantum cryptography
2. RandomXTest - Mining algorithm validation
3. Bech32Test - Address encoding/decoding
4. SerializationTest - Data structure serialization
5. StorageTest - RocksDB operations
6. ValidationTest - Transaction validation
7. GenesisTest - Genesis block verification
8. NetworkTest - P2P protocol
9. MLTest - Machine learning components
10. WalletTest - HD wallet functionality
11. FuzzTest - Robustness testing
12. IntegrationTest - End-to-end testing
13. LightningTest - Lightning Network

---

## 🔑 Key Features

### 1. Orphan Block Pool
Handles out-of-order block reception with intelligent caching and parent requests.

**Features**:
- Hash-based storage for O(1) lookups
- Parent-hash indexing for efficient retrieval
- LRU eviction (100 block maximum)
- 10-minute timeout
- Automatic GETDATA parent requests
- Recursive orphan processing

**Location**: [src/network/network.cpp:952-1082](src/network/network.cpp#L952-L1082)

### 2. Transaction Relay Protocol
Bitcoin-compatible transaction propagation with relay loop prevention.

**Features**:
- INV message broadcasting
- Relay loop prevention (skip sender)
- BroadcastTransaction() with peer exclusion
- Mempool integration

**Location**: [src/network/network.cpp:1830-1838](src/network/network.cpp#L1830-L1838)

### 3. UTXO Set Integration
Database-backed unspent transaction output tracking with reorganization support.

**Features**:
- RocksDB persistence
- Spent output restoration for reorgs
- Address-based UTXO queries
- In-memory caching
- Batch database operations

**Location**: [src/blockchain/blockchain.cpp:75-170](src/blockchain/blockchain.cpp#L75-L170)

### 4. Wallet Auto-Lock Security
Automatic wallet locking after configurable idle timeout.

**Features**:
- Configurable timeout (0 = disabled)
- Background monitoring thread
- Secure memory clearing (OPENSSL_cleanse)
- Thread-safe with mutex protection
- Auto-stops on wallet close

**Location**: [src/wallet/wallet.cpp:1678-1743](src/wallet/wallet.cpp#L1678-L1743)

**Usage**:
```cpp
// Unlock with 5-minute auto-lock
wallet.Unlock(passphrase, 300);

// Unlock without auto-lock
wallet.Unlock(passphrase, 0);

// Manual lock
wallet.Lock();
```

### 5. Lightning Network Graph RPC
Complete network topology exposure via RPC.

**Features**:
- Node information (ID, alias, channels)
- Channel metadata (capacity, fees, status)
- Dilithium3 public key hex encoding
- JSON serialization

**Location**: [src/rpc/rpc.cpp:2256-2323](src/rpc/rpc.cpp#L2256-L2323)

**Usage**:
```bash
intcoin-cli lightning_getnetworkgraph
```

### 6. Security Logging
Blockchain security monitoring and audit trail.

**Features**:
- Deep reorganization warnings
- Fork detection logging
- Security-critical event tracking
- Operator awareness

**Locations**:
- [src/consensus/consensus.cpp:654](src/consensus/consensus.cpp#L654)
- [src/blockchain/blockchain.cpp:786](src/blockchain/blockchain.cpp#L786)

---

## 🔧 Technical Details

### Dependencies
- **liboqs**: Post-quantum cryptography (Dilithium3, Kyber768)
- **RandomX**: ASIC-resistant Proof-of-Work
- **RocksDB**: High-performance database
- **OpenSSL 3.0+**: AES-256-GCM, PBKDF2
- **Qt6**: Desktop wallet GUI
- **Boost**: C++ libraries
- **ZeroMQ**: Message queue
- **libevent**: Event notification

### Compatibility
- **Breaking Changes**: None
- **Backward Compatibility**: Fully compatible with v1.0.0-beta
- **Database Format**: Compatible (automatic migration)
- **Network Protocol**: Extended (backward compatible)

### Platform Support
- **Linux**: Ubuntu 22.04+, Debian 11+, Fedora 36+
- **macOS**: 12+ (Intel & ARM/Apple Silicon)
- **FreeBSD**: 13+, 14
- **Windows**: 10/11 (WSL2 & native)

---

## 📝 Detailed Change Log

### Phase 4.1: Block & Script Validation
**Files**: `src/blockchain/validation.cpp`, `src/blockchain/script.cpp`
**Commits**: 1fcaf2c

- Implemented P2PKH script validation
- Added P2SH support with script hash verification
- Multisig script validation (1-of-N to N-of-N)
- Enhanced TxValidator with comprehensive checks
- Merkle root verification
- Witness data validation

### Phase 4.2: Orphan Block Handling
**Files**: `src/network/network.cpp`, `include/intcoin/network.h`
**Commits**: 1895477

- OrphanBlock structure with peer tracking
- AddOrphanBlock() with LRU eviction
- ProcessOrphans() for recursive handling
- PruneOrphans() for timeout cleanup
- HandleBlock() integration with parent requests

### Phase 4.3: Transaction Relay
**Files**: `src/network/network.cpp`, `include/intcoin/network.h`
**Commits**: 24456f1

- BroadcastTransaction() with peer exclusion
- BroadcastMessage() overload for selective broadcasting
- HandleTx() integration with relay logic
- Relay loop prevention

### Phase 4.4: Fee Calculation
**Files**: `src/blockchain/transaction.cpp`, `src/blockchain/block.cpp`, `src/rpc/rpc.cpp`
**Commits**: a5585f2

- Transaction::GetTotalInputValue() using UTXO set
- Transaction::GetFee() as inputs - outputs
- Block::GetTotalFees() summing all transaction fees
- RPC getrawmempool fee reporting

### Phase 4.5: UTXO Set Integration
**Files**: `src/blockchain/blockchain.cpp`, `src/storage/storage.cpp`
**Commits**: cb61ebf

- LoadUTXOSet() from database
- ApplyBlockToUTXO() with spent tracking
- RevertBlockFromUTXO() for reorganizations
- GetUTXOsForAddress() for wallet queries
- UTXOSet class with database persistence

### Phase 5.1: Lightning Network RPC
**Files**: `src/rpc/rpc.cpp`
**Commits**: 0f57c8a

- lightning_getnetworkgraph implementation
- Node and channel serialization
- PublicKey hex encoding via BytesToHex()
- JSON response formatting

### Phase 6.2: Wallet Auto-Lock
**Files**: `src/wallet/wallet.cpp`
**Commits**: 6cfe80c

- Auto-lock timer infrastructure
- Background monitoring thread (1-second checks)
- StartAutoLock() and StopAutoLock() methods
- Secure memory clearing on lock
- Thread-safe operations with mutex

### Phase 6.1: Logging System
**Files**: `src/consensus/consensus.cpp`, `src/blockchain/blockchain.cpp`
**Commits**: 176443d

- Deep reorganization warning logs
- LogF() integration at critical points
- Security monitoring for chain events
- Operational visibility improvements

---

## 🚀 Upgrade Instructions

### From v1.0.0-beta

1. **Backup Your Data**:
   ```bash
   # Backup wallet
   cp ~/.intcoin/wallet.dat ~/.intcoin/wallet.dat.backup

   # Backup blockchain data (optional)
   cp -r ~/.intcoin/blocks ~/.intcoin/blocks.backup
   ```

2. **Stop Running Node**:
   ```bash
   intcoin-cli stop
   ```

3. **Update Code**:
   ```bash
   cd /path/to/intcoin
   git fetch origin
   git checkout main
   git pull origin main
   ```

4. **Rebuild**:
   ```bash
   cd build
   make clean
   cmake ..
   make -j$(nproc)
   ```

5. **Run Tests** (Optional):
   ```bash
   ctest --output-on-failure
   ```

6. **Restart Node**:
   ```bash
   intcoind -daemon
   ```

7. **Verify**:
   ```bash
   intcoin-cli getblockchaininfo
   ```

### First-Time Installation

See [BUILDING.md](BUILDING.md) for complete build instructions.

---

## 🐛 Known Issues

None reported for this release.

---

## 🔜 Future Enhancements

See [ENHANCEMENT_PLAN_v1.1.0.md](ENHANCEMENT_PLAN_v1.1.0.md) for planned features:

- **Phase 5.2**: Pool Server RPC completion
- **Phase 6.3**: Performance optimizations
- **Phase 7**: Mobile wallet infrastructure
- **Phase 8**: Atomic swaps
- **Phase 9**: Cross-chain bridges

---

## 🙏 Acknowledgments

### Contributors
- **Claude Sonnet 4.5**: AI Development Assistant
- **Neil Adamson**: Project Lead & Core Developer

### Special Thanks
- Open Quantum Safe project (liboqs)
- RandomX developers
- Bitcoin Core team (protocol inspiration)
- Lightning Network specification authors

---

## 📧 Contact & Support

- **GitHub**: https://github.com/INT-devs/intcoin
- **Issues**: https://github.com/INT-devs/intcoin/issues
- **Discord**: https://discord.gg/jCy3eNgx
- **Email**: team@international-coin.org
- **Wiki**: https://github.com/INT-devs/intcoin/wiki

---

## 📜 License

MIT License - See [LICENSE](LICENSE) for details

---

**Build with confidence. Deploy with security. Scale with INTcoin.**
