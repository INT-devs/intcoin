# SPV and Bloom Filters Technical Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production

---

## Table of Contents

1. [Introduction](#introduction)
2. [SPV Architecture](#spv-architecture)
3. [Bloom Filters](#bloom-filters)
4. [Implementation Details](#implementation-details)
5. [Security Considerations](#security-considerations)
6. [Performance Optimization](#performance-optimization)
7. [API Reference](#api-reference)
8. [Troubleshooting](#troubleshooting)

---

## Introduction

Simplified Payment Verification (SPV) is a method for verifying cryptocurrency transactions without downloading the entire blockchain. Combined with Bloom filters, SPV enables lightweight mobile and desktop wallets to securely verify payments while maintaining user privacy.

### Why SPV?

**Traditional Full Node**:
- Downloads entire blockchain (100+ GB)
- Validates all transactions
- Requires significant bandwidth, storage, and CPU
- Takes hours or days for initial sync

**SPV Client**:
- Downloads only block headers (~50 MB)
- Validates transactions relevant to user
- Minimal bandwidth and storage
- Syncs in seconds

### INTcoin SPV Features

- 🔗 **BIP37 Compatible** - Standard Bloom filter protocol
- 🔒 **Privacy Preserving** - Configurable false positive rates
- ⚡ **Fast Synchronization** - Headers-first approach
- 📱 **Mobile Optimized** - Low bandwidth and battery usage
- 🛡️ **Checkpoint Verification** - Protection against long-range attacks
- 🔄 **Dynamic Filter Updates** - Efficient address tracking

---

## SPV Architecture

### Overview

SPV clients verify payments by:
1. Downloading block headers only
2. Requesting merkle proofs for relevant transactions
3. Using Bloom filters to request filtered blocks

### Block Header Structure

Each block header is 80 bytes:

```cpp
struct BlockHeader {
    uint32_t version;           // 4 bytes - Block version
    uint8_t prevBlock[32];      // 32 bytes - Previous block hash
    uint8_t merkleRoot[32];     // 32 bytes - Merkle root
    uint32_t timestamp;         // 4 bytes - Unix timestamp
    uint32_t bits;              // 4 bytes - Difficulty target
    uint32_t nonce;             // 4 bytes - Proof-of-work nonce
};
```

**Storage calculation**:
- 1,000,000 blocks × 80 bytes = 80 MB
- Current INTcoin chain: ~150,000 blocks = 12 MB

### Headers-First Synchronization

**Process**:

1. **Connect to peers**
   ```
   SPV Client → Full Node: getheaders
   Full Node → SPV Client: headers (up to 2000)
   ```

2. **Validate headers**
   - Verify proof-of-work (difficulty target)
   - Check timestamp progression
   - Verify header chain continuity

3. **Request filtered blocks**
   ```
   SPV Client → Full Node: filterload (Bloom filter)
   SPV Client → Full Node: getdata (filtered block)
   Full Node → SPV Client: merkleblock + transactions
   ```

4. **Verify merkle proofs**
   - Confirm transaction is in merkle tree
   - Validate merkle path to root

### Merkle Proof Verification

**Merkle Tree Structure**:
```
                    Root Hash
                   /          \
               H(AB)            H(CD)
              /    \           /    \
           H(A)   H(B)      H(C)   H(D)
            |      |         |      |
           Tx A   Tx B      Tx C   Tx D
```

**Proof for Transaction B**:
- Provide: Hash(A), Hash(CD)
- Compute: Hash(AB) = Hash(Hash(A) + Hash(B))
- Compute: Root = Hash(Hash(AB) + Hash(CD))
- Verify: Root matches header's merkle root

**Proof size**: O(log n) where n = transactions per block
- 1,000 tx block: ~10 hashes × 32 bytes = 320 bytes
- 10,000 tx block: ~14 hashes × 32 bytes = 448 bytes

---

## Bloom Filters

### What is a Bloom Filter?

A Bloom filter is a probabilistic data structure that tests whether an element is a member of a set. It allows **false positives** but **never false negatives**.

**Properties**:
- ✅ If filter says "not in set" → definitely not in set
- ⚠️ If filter says "in set" → probably in set (might be false positive)

### Why Use Bloom Filters for SPV?

**Privacy**: Without Bloom filters, SPV clients would need to:
- Request transactions for specific addresses (reveals addresses to peers)
- Download all transactions (defeats purpose of SPV)

**Solution**: Bloom filters allow clients to request transactions that:
- Match their addresses (true positives)
- Also match unrelated addresses (false positives for privacy)

### Bloom Filter Structure

```cpp
struct BloomFilter {
    std::vector<uint8_t> data;  // Bit array
    uint32_t nHashFuncs;        // Number of hash functions
    uint32_t nTweak;            // Randomization parameter
    uint8_t nFlags;             // Update flags
};
```

**Parameters**:
- **Size (bits)**: Larger = fewer false positives, less privacy
- **Hash functions**: More = better accuracy, slower
- **False positive rate**: Configurable (0.001% - 1%)

### Creating a Bloom Filter

**Algorithm**:

1. **Initialize bit array** of size `m` (all bits = 0)

2. **Choose `k` hash functions** (typically 3-10)

3. **Insert elements** (addresses, UTXOs, transaction IDs):
   ```
   For each element e:
       For i = 1 to k:
           hash = Hash(e, i, tweak) mod m
           bitArray[hash] = 1
   ```

4. **Test membership**:
   ```
   For each element e:
       For i = 1 to k:
           hash = Hash(e, i, tweak) mod m
           If bitArray[hash] == 0:
               return False (definitely not in set)
       return True (probably in set)
   ```

### Optimal Parameters

**Given**:
- `n` = number of elements (addresses)
- `p` = desired false positive rate

**Calculate**:
- **Filter size**: `m = -n * ln(p) / (ln(2))^2`
- **Hash functions**: `k = (m/n) * ln(2)`

**Example** (100 addresses, 0.1% false positive rate):
- `m = -100 * ln(0.001) / (ln(2))^2 ≈ 1,437 bits ≈ 180 bytes`
- `k = (1437/100) * ln(2) ≈ 10 hash functions`

### False Positive Rate

**Trade-offs**:

| FP Rate | Privacy | Bandwidth | Filter Size (100 addresses) |
|---------|---------|-----------|----------------------------|
| 0.01% | Low | High | 240 bytes |
| 0.1% | Medium | Medium | 180 bytes |
| 1% | High | Low | 120 bytes |

**Recommendation**: 0.1% for most use cases

### Bloom Filter Updates

**Update modes** (BIP37):

1. **BLOOM_UPDATE_NONE** (0)
   - Never update filter
   - Best for watch-only wallets

2. **BLOOM_UPDATE_ALL** (1)
   - Update filter when matched transaction creates new outputs
   - Tracks UTXO chain

3. **BLOOM_UPDATE_P2PUBKEY_ONLY** (2)
   - Update only for P2PK/P2MULTISIG outputs
   - Balance between privacy and functionality

---

## Implementation Details

### C++ API

#### Creating a Bloom Filter

```cpp
#include "intcoin/bloom.h"

// Create filter for 100 addresses, 0.1% false positive rate
BloomFilter filter(100, 0.001, 0, BLOOM_UPDATE_ALL);

// Add address
std::vector<uint8_t> address = DecodeBase58("int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh");
filter.insert(address);

// Add UTXO (txid + output index)
COutPoint outpoint(txid, 0);
filter.insert(outpoint);

// Add public key
CPubKey pubkey = wallet.GetPublicKey();
filter.insert(pubkey);
```

#### Testing Membership

```cpp
// Check if transaction matches filter
bool matches = filter.IsRelevantAndUpdate(transaction);

if (matches) {
    // Transaction might be relevant (true or false positive)
    // Verify and process
}
```

#### Sending Filter to Peers

```cpp
// Create filter
BloomFilter filter = wallet.CreateBloomFilter();

// Send to peer
CNode* peer = GetPeer();
peer->PushMessage("filterload", filter);

// Request filtered blocks
peer->PushMessage("getdata", vInv);
```

### Mobile SDK API

#### iOS (Swift)

```swift
import INTcoinKit

// Create SPV wallet
let wallet = SPVWallet()

// Configure Bloom filter
wallet.bloomFilterConfig = BloomFilterConfig(
    falsePositiveRate: 0.001,
    maxElements: 100,
    updateMode: .updateAll
)

// Start synchronization
wallet.startSync { progress in
    print("Sync progress: \(progress)%")
}

// Monitor transactions
wallet.onTransactionReceived = { tx in
    print("Received: \(tx.amount) INT")
}
```

#### Android (Kotlin)

```kotlin
import org.intcoin.spv.*

// Create SPV wallet
val wallet = SPVWallet(context)

// Configure Bloom filter
wallet.bloomFilterConfig = BloomFilterConfig(
    falsePositiveRate = 0.001,
    maxElements = 100,
    updateMode = BloomFilterUpdateMode.UPDATE_ALL
)

// Start sync
wallet.startSync { progress ->
    println("Sync progress: $progress%")
}

// Receive transactions
wallet.transactionReceived.observe(this) { tx ->
    println("Received: ${tx.amount} INT")
}
```

### Network Protocol (BIP37)

#### filterload

Load Bloom filter into peer's memory.

**Message**:
```
filterload:
  - data: <filter_bytes>
  - nHashFuncs: <num_hash_functions>
  - nTweak: <randomization>
  - nFlags: <update_mode>
```

#### filteradd

Add element to existing filter.

**Message**:
```
filteradd:
  - data: <element_bytes>
```

**Use case**: Add new address without resending entire filter

#### filterclear

Remove Bloom filter from peer.

**Message**:
```
filterclear
```

**Use case**: Stop filtering, receive all transactions

#### merkleblock

Filtered block with merkle proofs.

**Message**:
```
merkleblock:
  - header: <block_header>
  - numTransactions: <total_tx_count>
  - hashes: <merkle_proof_hashes>
  - flags: <merkle_tree_flags>
```

Followed by separate `tx` messages for matched transactions.

---

## Security Considerations

### Privacy Concerns

**Bloom Filter Fingerprinting**:
- Unique filter patterns can identify users
- Filter size, hash functions, and update mode leak info

**Mitigation**:
- Randomize `nTweak` parameter
- Use common filter parameters
- Rotate filters periodically
- Connect to multiple diverse peers

**Address Clustering**:
- Filters reveal which addresses belong to same wallet
- UTXO linkage exposes transaction history

**Mitigation**:
- Higher false positive rate
- Separate filters for different address groups
- Avoid filter updates when possible

### SPV Security Model

**Assumptions**:
1. **Longest chain is honest** - Majority hashpower is honest
2. **Peers provide valid merkle proofs** - Can be verified cryptographically
3. **Miners validate transactions** - SPV doesn't check transaction validity

**Risks**:

1. **Invalid transactions in blocks**
   - If all miners collude to include invalid tx
   - SPV client accepts it (doesn't validate scripts/amounts)
   - **Likelihood**: Extremely low (economic incentive against)

2. **Long-range attacks**
   - Attacker creates fake chain from old blocks
   - SPV client may follow fake chain
   - **Mitigation**: Checkpoint verification

3. **Eclipse attacks**
   - Attacker controls all peer connections
   - Feeds SPV client fake blocks
   - **Mitigation**: Connect to diverse peers, use trusted nodes

### Checkpoint Verification

**Checkpoints** are hardcoded block hashes at specific heights.

```cpp
static const Checkpoints checkpoints[] = {
    {     0, uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")},
    { 11111, uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d")},
    { 50000, uint256S("0x000000000592a974b1074da1e6adfbdc5e21b5e7dfe63e9a8c9f3d8de8e96f8a")},
    {100000, uint256S("0x00000000009e2958c15ff9290d571bf9459e93b19765c6801ddeccadbb160a1e")},
};
```

**Verification**:
- SPV client checks header chain against checkpoints
- Rejects chains that don't match checkpoints
- Prevents long-range attacks before checkpoint

**Update strategy**:
- Release new checkpoints with software updates
- Typically every 10,000-50,000 blocks

### DOS Protection

**Filter limits**:
```cpp
static const unsigned int MAX_BLOOM_FILTER_SIZE = 36000;  // bytes
static const unsigned int MAX_HASH_FUNCS = 50;
```

**Rationale**:
- Prevent memory exhaustion attacks
- Limit CPU usage for filter testing
- Balance functionality and security

---

## Performance Optimization

### Bandwidth Optimization

**Reduce filter size**:
```cpp
// Smaller filter = less bandwidth, higher false positive rate
BloomFilter filter(nElements, 0.01, 0, BLOOM_UPDATE_NONE);  // 1% FP rate
```

**Disable filter updates**:
```cpp
// Update mode NONE = no filteradd messages
BloomFilter filter(nElements, fpRate, 0, BLOOM_UPDATE_NONE);
```

**Batch header requests**:
```cpp
// Request maximum headers per message (2000)
peer->PushMessage("getheaders", locator);
```

### Storage Optimization

**Header-only storage**:
```cpp
// Store headers in compact format
struct CompactHeader {
    uint8_t prevBlock[32];
    uint8_t merkleRoot[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} __attribute__((packed));  // 76 bytes vs 80 bytes
```

**Pruned header chain**:
- Keep only last N headers in memory
- Store older headers on disk
- Load on-demand for reorganization handling

### CPU Optimization

**Optimized hash functions**:
```cpp
// Use MurmurHash3 for Bloom filter (fast, good distribution)
inline uint32_t MurmurHash3(uint32_t nHashNum, const std::vector<uint8_t>& vDataToHash) {
    // Fast non-cryptographic hash
}
```

**Merkle proof caching**:
```cpp
// Cache verified merkle proofs
std::unordered_map<uint256, MerkleProof> proofCache;
```

---

## API Reference

### BloomFilter Class

#### Constructor

```cpp
BloomFilter(uint32_t nElements, double fpRate, uint32_t nTweak, uint8_t nFlags);
```

**Parameters**:
- `nElements`: Expected number of elements
- `fpRate`: False positive rate (0.0 - 1.0)
- `nTweak`: Random tweak for hash functions
- `nFlags`: Update mode (BLOOM_UPDATE_NONE, BLOOM_UPDATE_ALL, BLOOM_UPDATE_P2PUBKEY_ONLY)

#### Methods

```cpp
// Insert element into filter
void insert(const std::vector<uint8_t>& vKey);
void insert(const COutPoint& outpoint);
void insert(const uint256& hash);

// Test if element matches filter
bool contains(const std::vector<uint8_t>& vKey) const;

// Check if transaction is relevant and update filter if needed
bool IsRelevantAndUpdate(const CTransaction& tx);

// Clear filter
void clear();

// Check if filter is within size limits
bool IsWithinSizeConstraints() const;
```

### SPVWallet Class

#### Constructor

```cpp
SPVWallet(const WalletConfig& config);
```

#### Methods

```cpp
// Start header synchronization
void startSync(ProgressCallback callback);

// Stop synchronization
void stopSync();

// Get sync progress (0-100)
int getSyncProgress() const;

// Get current block height
uint32_t getBlockHeight() const;

// Create Bloom filter for wallet
BloomFilter createBloomFilter(double fpRate = 0.001) const;

// Send Bloom filter to peers
void updateBloomFilter();

// Transaction callbacks
void setTransactionCallback(TransactionCallback callback);
```

---

## Troubleshooting

### Transactions Not Appearing

**Symptom**: Sent transaction not showing in SPV wallet

**Causes**:
1. Bloom filter doesn't include address
2. Filter not sent to peers
3. Wallet not synced to latest block

**Solutions**:
```cpp
// Recreate and send filter
wallet.updateBloomFilter();

// Resync from last checkpoint
wallet.resyncFromHeight(checkpointHeight);

// Check filter contains address
bool contains = filter.contains(addressData);
```

### Slow Synchronization

**Symptom**: Header sync takes longer than expected

**Causes**:
1. Slow or unresponsive peers
2. Network congestion
3. Too many peer connections

**Solutions**:
```cpp
// Connect to specific fast peers
wallet.addPeer("fast-node.intcoin.org:8333");

// Increase timeout
wallet.setPeerTimeout(30);  // seconds

// Reduce peer count
wallet.setMaxPeers(4);
```

### High Bandwidth Usage

**Symptom**: SPV client using too much data

**Causes**:
1. Low false positive rate (too precise filter)
2. Filter update mode ALL (too many filteradd messages)
3. Requesting too many blocks

**Solutions**:
```cpp
// Increase false positive rate
BloomFilter filter(nElements, 0.01, 0, BLOOM_UPDATE_NONE);  // 1% instead of 0.1%

// Disable filter updates
config.bloomUpdateMode = BLOOM_UPDATE_NONE;

// Only sync recent blocks
wallet.syncFromHeight(currentHeight - 1000);
```

### Privacy Concerns

**Symptom**: Worried about Bloom filter leaking address info

**Solutions**:
```cpp
// Higher false positive rate
BloomFilter filter(nElements, 0.05, 0, flags);  // 5% FP rate

// Randomize tweak regularly
uint32_t tweak = GetRand(0xFFFFFFFF);
BloomFilter filter(nElements, fpRate, tweak, flags);

// Use Tor/VPN
wallet.setSocks5Proxy("127.0.0.1:9050");

// Connect to trusted node only
wallet.addTrustedPeer("my-node.example.com:8333");
wallet.setTrustedOnly(true);
```

---

## Example: Complete SPV Implementation

### Minimal SPV Client

```cpp
#include "intcoin/spv.h"
#include "intcoin/bloom.h"
#include <iostream>

int main() {
    // Create wallet
    SPVWallet wallet;

    // Add addresses to watch
    std::vector<std::string> addresses = {
        "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
        "int1qrz8j9a4xlwfp7xxz3xhxq2jm4u6xv5k7lpwq3m"
    };

    for (const auto& addr : addresses) {
        wallet.addAddress(addr);
    }

    // Create Bloom filter (100 addresses, 0.1% FP rate)
    BloomFilter filter = wallet.createBloomFilter(0.001);

    // Connect to peers
    wallet.addPeer("node1.intcoin.org:8333");
    wallet.addPeer("node2.intcoin.org:8333");

    // Set callbacks
    wallet.setTransactionCallback([](const CTransaction& tx) {
        std::cout << "Received transaction: " << tx.GetHash().ToString() << std::endl;
    });

    wallet.setSyncProgressCallback([](int progress) {
        std::cout << "Sync: " << progress << "%" << std::endl;
    });

    // Start sync
    wallet.startSync();

    // Wait for sync
    while (wallet.getSyncProgress() < 100) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Synced to height: " << wallet.getBlockHeight() << std::endl;

    // Monitor for new transactions
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}
```

---

## Resources

- **BIP37**: Bloom Filtering - https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki
- **BIP157/158**: Compact Block Filters - https://github.com/bitcoin/bips/blob/master/bip-0157.mediawiki
- **Source Code**: [src/bloom.cpp](../src/bloom.cpp), [src/spv.cpp](../src/spv.cpp)
- **Tests**: [tests/test_bloom.cpp](../tests/test_bloom.cpp)
- **Mobile Wallet Guide**: [MOBILE_WALLET.md](MOBILE_WALLET.md)
- **Mobile SDK**: [MOBILE_SDK.md](MOBILE_SDK.md)

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
