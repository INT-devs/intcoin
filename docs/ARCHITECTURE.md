# INTcoin Architecture

**Version**: 1.0.0
**Last Updated**: January 2025

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Blockchain Layer](#blockchain-layer)
3. [Cryptography Layer](#cryptography-layer)
4. [Network Layer](#network-layer)
5. [Storage Layer](#storage-layer)
6. [Consensus Layer](#consensus-layer)
7. [Application Layer](#application-layer)
8. [Security Model](#security-model)

---

## System Overview

INTcoin is designed as a multi-layered system with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────┐
│              Application Layer                          │
│  (Wallets, Explorer, RPC, CLI, Lightning Network)      │
└─────────────────────────────────────────────────────────┘
                          ▲
┌─────────────────────────────────────────────────────────┐
│              Consensus Layer                            │
│     (RandomX PoW, Difficulty Adjustment, Validation)    │
└─────────────────────────────────────────────────────────┘
                          ▲
┌─────────────────────────────────────────────────────────┐
│              Blockchain Layer                           │
│      (Blocks, Transactions, UTXO, Scripts)             │
└─────────────────────────────────────────────────────────┘
                          ▲
┌─────────────────────────────────────────────────────────┐
│              Network Layer                              │
│        (P2P, Message Protocol, Peer Discovery)          │
└─────────────────────────────────────────────────────────┘
                          ▲
┌─────────────────────────────────────────────────────────┐
│              Storage Layer                              │
│        (RocksDB, Indexing, Pruning, UTXO Set)          │
└─────────────────────────────────────────────────────────┘
                          ▲
┌─────────────────────────────────────────────────────────┐
│              Cryptography Layer                         │
│   (Dilithium Signatures, Kyber Key Exchange, Hashing)  │
└─────────────────────────────────────────────────────────┘
```

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

1. **DNS Seeding**: Query `seed.international-coin.org`
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

- **Confidential Transactions**: Hide transaction amounts
- **Schnorr Signatures**: Aggregate signatures
- **Taproot**: Advanced scripting privacy
- **Cross-Chain Atomic Swaps**: Trade with other chains
- **Sidechains**: Experimental features

---

**Last Updated**: January 2025
**Version**: 1.0.0
