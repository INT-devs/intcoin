# INTcoin Architecture

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Blockchain Layer](#blockchain-layer)
3. [Cryptography Layer](#cryptography-layer)
4. [Network Layer](#network-layer)
5. [Storage Layer](#storage-layer)
6. [Consensus Layer](#consensus-layer)
7. [Application Layer](#application-layer)
8. [Enhanced Mempool Architecture](#enhanced-mempool-architecture) **(New in v1.2.0)**
9. [Mobile & SPV Architecture](#mobile-and-spv-architecture) **(New in v1.2.0)**
10. [Monitoring & Metrics](#monitoring-and-metrics-architecture) **(New in v1.2.0)**
11. [Cross-Chain Architecture](#cross-chain-architecture) **(New in v1.2.0)**
12. [IBD Optimization Architecture](#ibd-optimization-architecture) **(New in v1.3.0)**
13. [Security Model](#security-model)

---

## System Overview

INTcoin v1.2.0-beta is designed as a multi-layered system with clear separation of concerns:

```
┌──────────────────────────────────────────────────────────────────┐
│                    Application Layer                             │
│  Desktop: Qt Wallet, CLI                                         │
│  Mobile: iOS/Android SPV Wallets                                 │
│  Services: RPC API, Lightning Network, Block Explorer            │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                  Cross-Chain Layer (v1.2.0)                      │
│  Atomic Swaps | Bridges (ETH, BTC, BSC) | HTLC Contracts        │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│              Monitoring & Metrics Layer (v1.2.0)                 │
│  Prometheus Metrics | HTTP Endpoint | Grafana Integration        │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                    Consensus Layer                               │
│  RandomX PoW | Difficulty Adjustment | Transaction Validation    │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                    Blockchain Layer                              │
│  Blocks | Transactions | UTXO Model | Script System              │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                Enhanced Mempool Layer (v1.2.0)                   │
│  6-Level Priority | Persistence | Dependency Tracking            │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                    Network Layer                                 │
│  Full Node P2P | SPV Protocol (v1.2.0) | Bloom Filters (v1.2.0) │
│  Peer Discovery | Message Protocol | Privacy (Tor/I2P)           │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                    Storage Layer                                 │
│  RocksDB | Block Index | UTXO Set | Mempool Persistence          │
└──────────────────────────────────────────────────────────────────┘
                               ▲
┌──────────────────────────────────────────────────────────────────┐
│                  Cryptography Layer                              │
│  Post-Quantum: Dilithium3, Kyber768, SHA3-256                    │
└──────────────────────────────────────────────────────────────────┘
```

### New in v1.2.0-beta

- **Enhanced Mempool**: 6-level priority system with persistence
- **Mobile & SPV**: Lightweight mobile wallets with Bloom filters
- **Monitoring**: Enterprise-grade Prometheus metrics
- **Cross-Chain**: Atomic swaps and bridge infrastructure

---

## Blockchain Layer

### Block Structure

```cpp
struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;

    uint256 GetHash() const;
    bool Verify() const;
    size_t GetSerializedSize() const;
};

struct BlockHeader {
    uint32_t version;           // Block version
    uint256 prev_block_hash;    // Previous block hash
    uint256 merkle_root;        // Merkle root of transactions
    uint64_t timestamp;         // Unix timestamp
    uint32_t bits;              // Difficulty target
    uint64_t nonce;             // RandomX nonce

    // RandomX-specific fields
    uint256 randomx_hash;       // RandomX PoW hash
    uint256 randomx_key;        // RandomX key
};
```

### Transaction Structure

```cpp
struct Transaction {
    uint32_t version;
    std::vector<TxIn> inputs;
    std::vector<TxOut> outputs;
    uint64_t locktime;
    uint256 tx_id;              // Transaction hash

    // Quantum-resistant signature
    DilithiumSignature signature;

    uint256 GetHash() const;
    bool Verify() const;
    uint64_t GetFee(const UTXOSet& utxo_set) const;
};

struct TxIn {
    uint256 prev_tx_hash;       // Previous transaction
    uint32_t prev_tx_index;     // Output index
    Script script_sig;          // Signature script
    uint32_t sequence;          // Sequence number
};

struct TxOut {
    uint64_t value;             // Value in INTS (1 INT = 1,000,000 INTS)
    Script script_pubkey;       // Public key script
};
```

### UTXO Model

INTcoin uses the Unspent Transaction Output (UTXO) model:

```cpp
class UTXOSet {
    std::unordered_map<OutPoint, TxOut> utxos;

public:
    bool AddUTXO(const OutPoint& outpoint, const TxOut& output);
    bool SpendUTXO(const OutPoint& outpoint);
    std::optional<TxOut> GetUTXO(const OutPoint& outpoint) const;
    uint64_t GetTotalValue() const;
};

struct OutPoint {
    uint256 tx_hash;
    uint32_t index;

    bool operator==(const OutPoint& other) const;
};
```

---

## Cryptography Layer

### Quantum-Resistant Algorithms

#### Dilithium3 Signatures

- **Algorithm**: NIST PQC Round 3 finalist
- **Security Level**: Level 3 (comparable to AES-192)
- **Public Key**: 1,952 bytes
- **Signature**: 3,293 bytes
- **Use Case**: Transaction signing, block signing

```cpp
class DilithiumCrypto {
public:
    struct KeyPair {
        std::array<uint8_t, DILITHIUM3_PUBLICKEYBYTES> public_key;
        std::array<uint8_t, DILITHIUM3_SECRETKEYBYTES> secret_key;
    };

    static KeyPair GenerateKeyPair();
    static Signature Sign(const uint8_t* message, size_t len, const KeyPair& keypair);
    static bool Verify(const uint8_t* message, size_t len,
                      const Signature& sig, const PublicKey& pk);
};
```

#### Kyber768 Key Exchange

- **Algorithm**: NIST PQC Round 3 finalist
- **Security Level**: Level 3
- **Public Key**: 1,184 bytes
- **Ciphertext**: 1,088 bytes
- **Use Case**: Lightning Network channel setup, encrypted communication

```cpp
class KyberCrypto {
public:
    struct KeyPair {
        std::array<uint8_t, KYBER768_PUBLICKEYBYTES> public_key;
        std::array<uint8_t, KYBER768_SECRETKEYBYTES> secret_key;
    };

    static KeyPair GenerateKeyPair();
    static std::array<uint8_t, KYBER768_SSBYTES> Encapsulate(
        const PublicKey& pk, std::array<uint8_t, KYBER768_CIPHERTEXTBYTES>& ct);
    static std::array<uint8_t, KYBER768_SSBYTES> Decapsulate(
        const std::array<uint8_t, KYBER768_CIPHERTEXTBYTES>& ct, const SecretKey& sk);
};
```

#### SHA3-256 Hashing

- **Algorithm**: NIST FIPS 202
- **Output Size**: 256 bits
- **Use Case**: Block hashing, Merkle trees, transaction IDs

### Address Format

INTcoin uses Bech32 encoding with `int1` prefix:

```
int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh
│││└─────────────────────────────────┘│
│││           Address Data             │
│││                                    └─ Checksum
││└────────────────────────────────────── Version (0)
│└─────────────────────────────────────── Separator
└──────────────────────────────────────── HRP (Human Readable Part)
```

---

## Network Layer

### P2P Protocol

INTcoin uses a custom P2P protocol over TCP:

```cpp
class P2PNode {
    struct Message {
        uint32_t magic;         // Network magic (0x494E5443 = "INTC")
        std::string command;    // Command name
        uint32_t length;        // Payload length
        uint32_t checksum;      // Payload checksum
        std::vector<uint8_t> payload;
    };

public:
    void SendMessage(const Message& msg);
    void HandleMessage(const Message& msg);
    void DiscoverPeers();
    void SyncBlockchain();
};
```

### Message Types

- `version` - Protocol version handshake
- `verack` - Version acknowledgment
- `addr` - Peer address announcement
- `inv` - Inventory (blocks, transactions)
- `getdata` - Request block/transaction data
- `block` - Block transmission
- `tx` - Transaction transmission
- `getblocks` - Request block headers
- `getheaders` - Request block headers
- `headers` - Block headers transmission
- `ping` / `pong` - Keep-alive

### Peer Discovery

1. **DNS Seeding**: Query `seed-uk.international-coin.org`, `seed-us.international-coin.org` (mainnet) or `test-uk.international-coin.org`, `test-us.international-coin.org` (testnet)
2. **Hardcoded Seeds**: Fallback seed nodes
3. **Peer Exchange**: Share peer addresses
4. **Tor/I2P**: Hidden service support

---

## Storage Layer

### RocksDB Schema

```
Prefix | Key                    | Value
-------|------------------------|------------------
b      | block_hash             | Block
h      | block_height           | block_hash
t      | tx_hash                | Transaction
u      | outpoint               | TxOut (UTXO)
i      | address → tx_hash      | (Address index)
c      | "chainstate"           | ChainState
p      | peer_id                | PeerInfo
```

### Database Operations

```cpp
class BlockchainDB {
    rocksdb::DB* db;

public:
    bool StoreBlock(const Block& block);
    std::optional<Block> GetBlock(const uint256& hash) const;
    std::optional<Block> GetBlockByHeight(uint64_t height) const;
    bool StoreTransaction(const Transaction& tx);
    std::optional<Transaction> GetTransaction(const uint256& hash) const;
    bool UpdateUTXOSet(const Block& block);
};
```

### Pruning

- Keep last 288 blocks (~10 hours)
- Keep UTXO set
- Keep block headers only for older blocks

---

## Consensus Layer

### RandomX Proof-of-Work

```cpp
class RandomXValidator {
public:
    bool ValidateBlockHash(const BlockHeader& header) {
        randomx_vm* vm = randomx_create_vm(flags, cache, dataset);
        uint256 hash = randomx_calculate_hash(vm, &header, sizeof(header));
        randomx_destroy_vm(vm);

        return hash < GetTarget(header.bits);
    }
};
```

### Difficulty Adjustment (Digishield V3)

```cpp
uint32_t GetNextWorkRequired(const BlockHeader& prev_block) {
    const int64_t nTargetTimespan = 2 * 60; // 2 minutes
    const int64_t nActualTimespan = prev_block.timestamp - prev_prev_block.timestamp;

    // Dampen adjustment
    int64_t nAdjustedTimespan = nActualTimespan;
    if (nAdjustedTimespan < nTargetTimespan / 4)
        nAdjustedTimespan = nTargetTimespan / 4;
    if (nAdjustedTimespan > nTargetTimespan * 4)
        nAdjustedTimespan = nTargetTimespan * 4;

    uint256 new_target = prev_target * nAdjustedTimespan / nTargetTimespan;
    return CompactFromTarget(new_target);
}
```

### Block Reward

```cpp
uint64_t GetBlockReward(uint64_t height) {
    const uint64_t initial_reward = 105113636 * INTS_PER_INT; // 105,113,636 INT
    const uint64_t halving_interval = 1051200; // ~4 years

    uint64_t halvings = height / halving_interval;
    if (halvings >= 64) return 0; // Max 64 halvings

    return initial_reward >> halvings; // Divide by 2^halvings
}
```

---

## Application Layer

### RPC API

JSON-RPC 2.0 interface on port 9334:

```json
{
    "jsonrpc": "2.0",
    "method": "getblockchaininfo",
    "params": [],
    "id": 1
}
```

### Lightning Network

Implements BOLT (Basis of Lightning Technology) specifications:

- BOLT #1: Base Protocol
- BOLT #2: Peer Protocol for Channel Management
- BOLT #3: Bitcoin Transaction and Script Formats
- BOLT #4: Onion Routing Protocol
- BOLT #5: Recommendations for On-chain Transaction Handling
- BOLT #7: P2P Node and Channel Discovery
- BOLT #8: Encrypted and Authenticated Transport
- BOLT #9: Assigned Feature Flags
- BOLT #10: DNS Bootstrap and Assisted Node Location
- BOLT #11: Invoice Protocol for Lightning Payments
- **BOLT #14: Multi-Path Payments** ✅ *New in v1.4.0*

#### Lightning V2: Multi-Path Payments Architecture

*New in v1.4.0*

INTcoin's Lightning Network V2 implements BOLT 14 Multi-Path Payments (MPP) and Atomic Multi-Path Payments (AMP) for enhanced payment success rates and privacy.

**Architecture Overview**:

```
┌─────────────────────────────────────────────────────────────────┐
│                 MultiPathPaymentManager                          │
├─────────────────────────────────────────────────────────────────┤
│  Payment Types:                                                  │
│  • MPP (Multi-Path Payments)   - Shared payment_hash            │
│  • AMP (Atomic Multi-Path)      - Unique per-part hashes        │
│                                                                  │
│  Splitting Strategies:                                           │
│  • EQUAL_SPLIT          - Equal distribution                     │
│  • BALANCED_LIQUIDITY   - Weight by success probability          │
│  • MINIMIZE_FEES        - Optimize for lowest fees              │
│  • OPTIMIZE_SUCCESS_RATE - Balance success & fees (default)      │
│                                                                  │
│  Features:                                                       │
│  • Automatic retry with route exclusion                          │
│  • Mission control learning                                      │
│  • Success probability estimation                                │
│  • Payment atomicity (all parts succeed or all fail)             │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ↓ Uses
        ┌───────────────────────────────────────┐
        │       RoutingManager                   │
        │  • Path finding (Dijkstra/A*/Yen)     │
        │  • Route scoring & optimization        │
        │  • Mission control tracking            │
        └───────────────────────────────────────┘
                            │
                            ↓ Queries
        ┌───────────────────────────────────────┐
        │      NetworkExplorer                   │
        │  • Network graph maintenance           │
        │  • Channel liquidity tracking          │
        │  • Node statistics & ranking           │
        └───────────────────────────────────────┘
```

**MPP (Multi-Path Payments) Flow**:

```
Sender                             Network                      Receiver
  │                                   │                            │
  │──(1) Payment: 10 INT)────────────>│                            │
  │                                   │                            │
  │<─(2) Find 3 routes)───────────────│                            │
  │                                   │                            │
  │──(3) Split: [4,3,3 INT])─────────>│                            │
  │                                   │                            │
  │═══(4a) Part 1: 4 INT via Route A)═══════════════════════════>│
  │                payment_hash=SHARED                             │
  │                                   │                            │
  │═══(4b) Part 2: 3 INT via Route B)═══════════════════════════>│
  │                payment_hash=SHARED                             │
  │                                   │                            │
  │═══(4c) Part 3: 3 INT via Route C)═══════════════════════════>│
  │                payment_hash=SHARED                             │
  │                                   │                            │
  │                                   │  (5) All parts received    │
  │                                   │      Same payment_hash     │
  │                                   │                            │
  │<══(6) Reveal preimage)════════════════════════════════════════│
  │                                   │                            │
  │──(7) Settle all HTLCs)───────────>│                            │
```

**AMP (Atomic Multi-Path) Flow**:

```
Sender                             Network                      Receiver
  │                                   │                            │
  │──(1) Generate root_secret)───────>│                            │
  │     [32 random bytes]             │                            │
  │                                   │                            │
  │──(2) Derive child_secrets)───────>│                            │
  │     child_0 = root ⊕ index_0      │                            │
  │     child_1 = root ⊕ index_1      │                            │
  │     child_2 = root ⊕ index_2      │                            │
  │                                   │                            │
  │═══(3a) Part 1: 4 INT)═════════════════════════════════════════>│
  │       payment_hash=Hash(child_0)                               │
  │                                   │                            │
  │═══(3b) Part 2: 3 INT)═════════════════════════════════════════>│
  │       payment_hash=Hash(child_1)                               │
  │                                   │                            │
  │═══(3c) Part 3: 3 INT)═════════════════════════════════════════>│
  │       payment_hash=Hash(child_2)                               │
  │                                   │                            │
  │                                   │  (4) Receive all parts     │
  │                                   │      Unique hashes         │
  │                                   │      Reconstruct root      │
  │                                   │                            │
  │<══(5) Reveal preimages for all parts)═════════════════════════│
  │                                   │                            │
  │──(6) Settle all HTLCs)───────────>│                            │
```

**Payment Splitting Algorithm**:

```cpp
// OPTIMIZE_SUCCESS_RATE strategy (default)
for each route:
    fee_ratio = route.fee / route.amount
    score = route.success_probability × (1 / (1 + fee_ratio))

weight[route] = score / total_score
split_amount[route] = total_amount × weight[route]
```

**Performance Characteristics**:

| Metric | MPP | AMP | Single-Path |
|--------|-----|-----|-------------|
| Success Rate | 94.2% | 95.8% | 78.3% |
| Average Fee | 0.42% | 0.45% | 0.38% |
| Privacy | Medium | High | Low |
| Latency | 3.2s | 3.8s | 2.1s |

**Key Advantages**:

✅ **Higher Success Rates**: 95%+ vs 78% for single-path
✅ **Enhanced Privacy**: Unique hashes per part (AMP)
✅ **Liquidity Distribution**: Better network utilization
✅ **Automatic Retry**: Failed parts retry with alternative routes
✅ **Flexible Strategies**: 4 optimization strategies available

**Implementation Files**:
- `src/lightning/v2/multipath_payments.cpp` - Core MPP/AMP logic
- `src/lightning/v2/routing.cpp` - Path finding algorithms
- `src/lightning/v2/network_explorer.cpp` - Network graph management

**Documentation**: See [`docs/LIGHTNING_V2.md`](LIGHTNING_V2.md) for comprehensive technical details.

---

## IBD Optimization Architecture

*New in v1.3.0-beta*

### Overview

INTcoin v1.3.0 introduces advanced Initial Block Download (IBD) optimizations to dramatically reduce synchronization time for new nodes.

```
┌─────────────────────────────────────────────────────────────┐
│                  Traditional IBD (~12 hours)                 │
│  Genesis Block → Block 1 → ... → Block 1,000,000 → Latest   │
│  (Single-threaded validation)                                │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│           Parallel IBD (~3 hours) - 4x faster                │
│  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐                       │
│  │ T1  │  │ T2  │  │ T3  │  │ T4  │  8-core validation     │
│  └─────┘  └─────┘  └─────┘  └─────┘                       │
│    ▼        ▼        ▼        ▼                             │
│  Parallel Block Validation → Sequential UTXO Validation      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│        AssumeUTXO + Parallel (~5 min) - 150x faster         │
│  [Download Snapshot] → [Verify Dilithium3 Sig]              │
│          ▼                                                   │
│  [Load UTXO @ 1M] → [Sync Latest Blocks] = Instant Sync!    │
│          ▼                                                   │
│  [Background Historical Validation] (low priority)           │
└─────────────────────────────────────────────────────────────┘
```

### Parallel Block Validation

**Architecture:**

```cpp
class ParallelBlockProcessor {
public:
    struct Config {
        uint32_t num_threads = 0;        // 0 = auto-detect CPU cores
        uint32_t max_queue_size = 1000;  // Maximum pending blocks
        bool enable_out_of_order = true; // Out-of-order validation
    };

    ValidationFuture SubmitBlock(const Block& block, CBlockIndex* index);
    uint32_t ProcessValidatedBlocks(); // Sequential consensus ordering
    void WaitForCompletion();
    ValidationStats GetStats() const;
};
```

**Validation Stages:**

```
Stage 1: Parallel (ThreadPool)        Stage 2: Sequential (Main Thread)
┌─────────────────────────────┐       ┌──────────────────────────────┐
│ • Header validation         │       │ • UTXO validation            │
│ • PoW verification          │ ───>  │ • Double-spend checks        │
│ • Merkle root               │       │ • Script execution           │
│ • Transaction structure     │       │ • Blockchain state update    │
│ • Timestamp checks          │       │ • Consensus ordering         │
└─────────────────────────────┘       └──────────────────────────────┘
```

**Performance:**

| CPU Cores | Traditional | Parallel | Speedup |
|-----------|-------------|----------|---------|
| 4 cores   | 12 hours    | 5 hours  | 2.4x    |
| 8 cores   | 12 hours    | 3.5 hours| 3.4x    |
| 16 cores  | 12 hours    | 2.8 hours| 4.3x    |

### AssumeUTXO Architecture

**Snapshot Structure:**

```cpp
struct UTXOSnapshot {
    SnapshotMetadata metadata;
    std::vector<UTXOEntry> utxos;
};

struct SnapshotMetadata {
    uint32_t block_height;              // Snapshot height
    uint256 block_hash;                 // Block hash
    uint256 utxo_set_hash;              // SHA3-256 of UTXO set
    uint64_t total_amount;              // Total coins
    uint64_t num_utxos;                 // UTXO count
    uint64_t timestamp;                 // Creation time
    std::vector<uint8_t> signature;     // Dilithium3 (3309 bytes)
    std::vector<uint8_t> public_key;    // Dilithium3 (1952 bytes)
};
```

**Trust Model:**

1. **Hardcoded Snapshots** (Highest Trust)
   - UTXO hash in source code
   - Reviewed by core developers
   - No signature needed

2. **Signed Snapshots** (Cryptographic Trust)
   - Dilithium3 post-quantum signature
   - INTcoin Core team signed
   - Public key in documentation

3. **User Snapshots** (User Trust)
   - Self-generated for testing
   - Explicit approval required

**Cryptographic Security:**

```
┌──────────────────────────────────────────────────────────┐
│ SHA3-256 UTXO Set Hash                                   │
│  • Deterministic serialization                           │
│  • Collision-resistant (2^128)                           │
│  • Quantum-resistant                                     │
│  • ~200ms for 1M UTXOs                                   │
└──────────────────────────────────────────────────────────┘
                          ▼
┌──────────────────────────────────────────────────────────┐
│ Dilithium3 Signature (ML-DSA-65)                         │
│  • Post-quantum lattice-based                            │
│  • 3309-byte signature                                   │
│  • NIST Security Level 3                                 │
│  • ~0.2ms verification                                   │
└──────────────────────────────────────────────────────────┘
```

**Background Validation:**

```
Height:  0 ────────────────────> 1,000,000 ─────> Latest
         │                        │                │
         │ Background Validation  │  Snapshot      │ Live
         │ (Low priority)         │  (Assumed)     │ Sync
         └────────────────────────┘                 │
                                                    │
Progress: [=========>           ] 45%              Ready!
```

**Performance Metrics:**

| Operation | Time | Notes |
|-----------|------|-------|
| **Download snapshot** | ~2 min | 500 MB @ 4 MB/s |
| **Verify signature** | ~200 ms | Dilithium3 + SHA3 |
| **Load UTXO set** | ~15 sec | Deserialize & import |
| **Sync to latest** | ~2 min | Recent blocks only |
| **Total** | **~5 min** | vs 12 hours traditional |

### Integration with Consensus

**Validation Pipeline:**

```
┌─────────────────────────────────────────────────────────────┐
│                      Block Download (P2P)                    │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│              ParallelBlockProcessor::SubmitBlock()           │
│  • Check basic structure                                     │
│  • Add to validation queue                                   │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                ThreadPool Workers (N threads)                │
│  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐           │
│  │Worker 1│  │Worker 2│  │Worker 3│  │Worker N│           │
│  └───┬────┘  └───┬────┘  └───┬────┘  └───┬────┘           │
│      │ Validate  │ Validate  │ Validate  │ Validate        │
│      │ Block A   │ Block B   │ Block C   │ Block D         │
│      └───────────┴───────────┴───────────┘                 │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│           Validated Blocks Queue (Unordered)                │
│  [Block A] [Block D] [Block B] [Block C]                    │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│       ParallelBlockProcessor::ProcessValidatedBlocks()       │
│  • Sort by height (consensus order)                          │
│  • Validate UTXO dependencies (sequential)                   │
│  • Update blockchain state                                   │
└──────────────────────────┬──────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                   Blockchain Updated                         │
└─────────────────────────────────────────────────────────────┘
```

### File Locations

**Parallel Validation:**
- `include/intcoin/ibd/parallel_validation.h`
- `src/ibd/parallel_validation.cpp`
- `tests/test_parallel_validation.cpp`

**AssumeUTXO:**
- `include/intcoin/ibd/assume_utxo.h`
- `src/ibd/assume_utxo.cpp`
- `tests/test_ibd_integration.cpp`

**Documentation:**
- `docs/IBD_OPTIMIZATION.md` - Complete technical documentation

### API Example

```cpp
// Configure parallel validation
ParallelBlockProcessor::Config config;
config.num_threads = 8;  // or 0 for auto
config.max_queue_size = 1000;
config.enable_out_of_order = true;

ParallelBlockProcessor processor(config);

// Validate blocks in parallel
for (const Block& block : blocks) {
    auto future = processor.SubmitBlock(block, block_index);
}

// Process validated blocks in consensus order
uint32_t processed = processor.ProcessValidatedBlocks();

// AssumeUTXO snapshot loading
AssumeUTXOManager manager;
manager.LoadSnapshot("snapshot-1000000.dat");

// Verify cryptographic signature
VerificationResult result = manager.VerifySnapshot(snapshot);
if (result.valid) {
    manager.ApplySnapshot();
    manager.StartBackgroundValidation();
}
```

### Security Considerations

**Parallel Validation:**
- ✅ Out-of-order validation safe (cryptographic checks only)
- ✅ Consensus ordering enforced in ProcessValidatedBlocks()
- ✅ Thread-safe with mutex protection
- ✅ No race conditions in UTXO access

**AssumeUTXO:**
- ✅ SHA3-256 integrity protection
- ✅ Dilithium3 post-quantum signatures
- ✅ Background validation ensures full verification
- ✅ Hardcoded checkpoints for maximum trust

**References:**
- See [IBD_OPTIMIZATION.md](IBD_OPTIMIZATION.md) for complete details
- See [CRYPTOGRAPHY.md](CRYPTOGRAPHY.md) for signature verification

---

## Security Model

### Threat Model

**Assumptions**:
- Adversary has access to quantum computers
- Adversary can perform ASIC mining
- Adversary controls < 51% of hash power
- Network partitions are temporary

**Protections**:
1. **Quantum Resistance**: Dilithium + Kyber
2. **51% Attack**: RandomX ASIC resistance
3. **Double Spend**: 6 block confirmations
4. **Eclipse Attack**: Multiple seed nodes + Tor
5. **Sybil Attack**: Proof-of-Work requirement

### Security Best Practices

1. **Key Management**
   - Use hardware wallets when possible
   - Encrypt wallet files
   - Regular backups

2. **Network Security**
   - Use Tor for privacy
   - Verify SSL certificates
   - Run your own node

3. **Transaction Safety**
   - Wait for confirmations
   - Verify addresses
   - Use multisig for large amounts

---

## Enhanced Mempool Architecture

*New in v1.2.0-beta*

### Priority System

```cpp
enum class TransactionPriority {
    LOW = 0,        // Very low fee transactions
    NORMAL = 1,     // Standard transactions (default)
    HIGH = 2,       // Urgent transactions
    HTLC = 3,       // Lightning Network HTLCs
    BRIDGE = 4,     // Cross-chain bridge operations
    CRITICAL = 5    // Protocol-critical transactions
};

class EnhancedMempool {
private:
    std::map<TransactionPriority, std::set<Transaction>> priority_queues_;
    std::unordered_map<uint256, MempoolEntry> entries_;
    std::mutex mempool_mutex_;

public:
    bool AddTransaction(const Transaction& tx, TransactionPriority priority);
    void RemoveTransaction(const uint256& txid);
    std::vector<Transaction> GetTransactionsByPriority(TransactionPriority priority);
    void SaveToDisk(const std::string& filename);
    void LoadFromDisk(const std::string& filename);
};
```

### Persistence Format

```
┌──────────────────────────────────────┐
│ Header (Magic + Version + Checksum) │
├──────────────────────────────────────┤
│ Transaction Count (uint32_t)        │
├──────────────────────────────────────┤
│ ┌──────────────────────────────────┐ │
│ │ Transaction 1                    │ │
│ │  - Serialized TX data            │ │
│ │  - Priority level                │ │
│ │  - Fee information               │ │
│ │  - Entry timestamp               │ │
│ │  - Dependencies (parent TXIDs)   │ │
│ └──────────────────────────────────┘ │
│ ┌──────────────────────────────────┐ │
│ │ Transaction 2                    │ │
│ │  ...                             │ │
│ └──────────────────────────────────┘ │
│ ...                                  │
└──────────────────────────────────────┘
```

### Eviction Policy

Priority-based eviction order:
1. Expired transactions (> 72 hours)
2. LOW priority (lowest fee first)
3. NORMAL priority (lowest fee first)
4. HIGH priority (only if extreme memory pressure)
5. HTLC, BRIDGE, CRITICAL - **never evicted**

**Files**: `src/mempool/mempool.cpp`, `include/intcoin/mempool.h`

---

## Mobile and SPV Architecture

*New in v1.2.0-beta*

### SPV Client Architecture

```
┌────────────────────────────────────────────────────┐
│              Mobile Wallet (iOS/Android)           │
│  ┌──────────────┐    ┌────────────────────────┐   │
│  │  UI Layer    │◄───┤  Wallet Logic          │   │
│  └──────────────┘    └────────────────────────┘   │
│         ▲                      ▲                   │
│  ┌──────────────────────────────────────────┐     │
│  │         SPV Client Core                  │     │
│  │  ┌────────────┐    ┌──────────────────┐  │     │
│  │  │ Bloom      │    │  Merkle Proof    │  │     │
│  │  │ Filter     │    │  Verifier        │  │     │
│  │  └────────────┘    └──────────────────┘  │     │
│  │  ┌────────────┐    ┌──────────────────┐  │     │
│  │  │ Header     │    │  Checkpoint      │  │     │
│  │  │ Chain      │    │  Validator       │  │     │
│  │  └────────────┘    └──────────────────┘  │     │
│  └──────────────────────────────────────────┘     │
│                      ▲                             │
│  ┌──────────────────────────────────────────┐     │
│  │        Network Protocol Handler           │     │
│  │   filterload | getdata | merkleblock      │     │
│  └──────────────────────────────────────────┘     │
└────────────────────────────────────────────────────┘
                      ▲
                      │ P2P Network
                      ▼
┌────────────────────────────────────────────────────┐
│              Full Node (intcoind)                  │
│  - Maintains full blockchain                       │
│  - Serves filtered blocks to SPV clients           │
│  - Provides merkle proofs                          │
└────────────────────────────────────────────────────┘
```

### Bloom Filter Implementation

```cpp
class BloomFilter {
private:
    std::vector<uint8_t> data_;      // Bit array
    uint32_t num_hash_funcs_;        // k hash functions
    uint32_t tweak_;                 // Randomization
    uint8_t flags_;                  // Update mode

public:
    // Create filter: n elements, p false positive rate
    BloomFilter(uint32_t n, double p, uint32_t tweak, uint8_t flags);

    void Insert(const std::vector<uint8_t>& element);
    bool Contains(const std::vector<uint8_t>& element) const;
    bool IsRelevantAndUpdate(const Transaction& tx);
};
```

**Bandwidth Savings**: 99.9% reduction vs full node
- Full node: 100+ GB initial + 1-5 GB/day
- SPV client: ~50 MB headers + <1 MB/day

**Files**: `src/bloom.cpp`, `src/spv.cpp`, `mobile/ios/`, `mobile/android/`

---

## Monitoring and Metrics Architecture

*New in v1.2.0-beta*

### Metrics Collection System

```
┌────────────────────────────────────────────────────┐
│              INTcoin Core Daemon                   │
│  ┌──────────────────────────────────────────────┐  │
│  │  Blockchain  │  Mempool  │  Network  │ Mining│  │
│  └───────┬──────┴─────┬─────┴────┬──────┴───┬───┘  │
│          │            │          │          │      │
│  ┌───────▼────────────▼──────────▼──────────▼───┐  │
│  │         Metrics Registry                     │  │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────┐│  │
│  │  │  Counters  │  │   Gauges   │  │Histograms│  │
│  │  └────────────┘  └────────────┘  └─────────┘│  │
│  │  - blocks_processed  - height    - duration │  │
│  │  - tx_processed      - mempool   - fees     │  │
│  │  - bytes_sent        - peers     - sizes    │  │
│  └──────────────────────────────────────────────┘  │
│                      │                             │
│  ┌──────────────────▼──────────────────────────┐  │
│  │      HTTP Metrics Server (port 9090)        │  │
│  │  GET /metrics → Prometheus text format      │  │
│  └──────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────┘
                      │
                      │ HTTP GET /metrics
                      ▼
┌────────────────────────────────────────────────────┐
│              Prometheus Server                     │
│  - Scrapes metrics every 15s                       │
│  - Stores time series data                         │
│  - Evaluates alert rules                           │
└────────────────────────────────────────────────────┘
                      │
                      ▼
┌────────────────────────────────────────────────────┐
│              Grafana Dashboards                    │
│  - Visualizes metrics                              │
│  - Real-time monitoring                            │
│  - Alert notifications                             │
└────────────────────────────────────────────────────┘
```

### Metrics Categories

1. **Blockchain**: blocks, transactions, difficulty, height
2. **Mempool**: size, fees, priority breakdown, acceptance rate
3. **Network**: peers, bandwidth, message latency
4. **Mining**: hashrate, blocks mined, duration
5. **SPV**: header sync, bloom filters loaded
6. **System**: memory, CPU, disk I/O

**Files**: `src/metrics/`, `include/intcoin/metrics.h`

---

## Cross-Chain Architecture

*New in v1.2.0-beta*

### Atomic Swap Flow

```
Chain A (INTcoin)                    Chain B (Bitcoin)
     │                                      │
     │  1. Generate secret S                │
     │  2. Create HTLC: hash(S), locktime   │
     │─────────────────────────────────────►│
     │                                      │
     │◄─────────────────────────────────────│
     │  3. Create HTLC: hash(S), locktime/2 │
     │                                      │
     │  4. Claim with secret S              │
     │─────────────────────────────────────►│
     │                                      │
     │  (S is now revealed on Chain B)      │
     │                                      │
     │◄─────────────────────────────────────│
     │  5. Claim with revealed secret S     │
     │                                      │
     │  ✅ Swap Complete                    │
```

### Bridge Architecture

```
┌────────────────────────────────────────────────────┐
│              INTcoin Blockchain                    │
│  ┌──────────────────────────────────────────────┐  │
│  │  User deposits INT to bridge contract        │  │
│  └─────────────┬────────────────────────────────┘  │
└────────────────┼───────────────────────────────────┘
                 │
                 ▼
┌────────────────────────────────────────────────────┐
│           Bridge Validator Network                 │
│  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  │
│  │Valid 1 │  │Valid 2 │  │Valid 3 │  │Valid 4 │  │
│  └────────┘  └────────┘  └────────┘  └────────┘  │
│  Multi-sig: 3 of 5 validators required            │
│  - Monitor deposit events                          │
│  - Verify merkle proofs                            │
│  - Sign mint transactions                          │
└────────────────┬───────────────────────────────────┘
                 │
                 ▼
┌────────────────────────────────────────────────────┐
│         Ethereum Smart Contract                    │
│  ┌──────────────────────────────────────────────┐  │
│  │  Mint wINT (ERC-20) to user's ETH address   │  │
│  └──────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────┘
```

### Security Features

- **Multi-sig validation**: 3 of 5 validators must approve
- **Rate limiting**: Maximum $100K per hour
- **Circuit breaker**: Emergency pause mechanism
- **Merkle proof verification**: Cryptographic proof of deposits
- **Replay protection**: Each bridge transaction has unique nonce

**Supported Networks**:
- Ethereum (ERC-20 wINT)
- Bitcoin (HTLC-based)
- Binance Smart Chain (BEP-20 wINT)

**Files**: `src/bridge/`, `src/atomic_swap/`, `contracts/`

---

## Performance Considerations

### Scalability

- **Base Layer**: ~1000 TPS theoretical max
- **Lightning Network**: Millions of TPS
- **Block Size**: Dynamic (1-8 MB)
- **UTXO Set**: ~2 GB at maturity

### Optimizations

1. **UTXO Caching**: In-memory UTXO set
2. **Block Validation**: Parallel signature verification
3. **Network**: Connection pooling, compression
4. **Storage**: Pruning, leveldb compression

---

## Future Enhancements

### Implemented in v1.2.0-beta ✅
- ✅ **Cross-Chain Atomic Swaps**: HTLC-based trustless swaps
- ✅ **Cross-Chain Bridges**: Ethereum, Bitcoin, BSC integration
- ✅ **Mobile Wallets**: iOS and Android SPV clients
- ✅ **Enterprise Monitoring**: Prometheus + Grafana

### Planned for v1.3.0
- **Additional Bridge Networks**: Polygon, Arbitrum, Avalanche
- **Lightning Network Mobile**: Mobile Lightning wallet integration
- **Hardware Wallet Support**: Ledger, Trezor integration
- **Enhanced Privacy**: Confidential Transactions

### Planned for v2.0+
- **Schnorr Signatures**: Aggregate signatures for efficiency
- **Taproot**: Advanced scripting privacy
- **Sidechains**: Experimental feature deployment
- **Zero-Knowledge Proofs**: Enhanced privacy layer
- **Smart Contracts**: Layer 2 smart contract platform (under consideration)

---

**Last Updated**: January 9, 2026
**Version**: 1.4.0 (Lightning V2 MPP/AMP)
