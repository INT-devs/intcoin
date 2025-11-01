# INTcoin Implementation Guide

**Status**: Development Build
**Version**: 0.1.0-alpha
**Last Updated**: January 2025

---

## Overview

This document provides a comprehensive guide for implementing the remaining core features of INTcoin. The project currently has:

✅ Complete project structure
✅ Build system (CMake, CI/CD)
✅ Documentation and wiki
✅ Cryptographic interfaces defined
✅ SHA-256 and SHA3-256 implementations
✅ Post-quantum crypto stubs (Dilithium, Kyber)

**Remaining work**: Full implementation of blockchain core, networking, wallet, and mining features.

---

## Implementation Status

### Cryptography Module

| Component | Status | Notes |
|-----------|--------|-------|
| SHA-256 (PoW) | ✅ Complete | OpenSSL implementation |
| SHA3-256 | ✅ Complete | OpenSSL EVP implementation |
| Dilithium5 | ⚠️ Stub | Requires liboqs integration |
| Kyber1024 | ⚠️ Stub | Requires liboqs integration |
| HKDF | 📝 To implement | Use OpenSSL EVP_KDF |
| Random | 📝 To implement | Use OpenSSL RAND_bytes |
| Mnemonic (BIP39) | 📝 To implement | Word list + PBKDF2 |
| Address encoding | 📝 To implement | Bech32 + checksum |

### Blockchain Core

| Component | Status | Notes |
|-----------|--------|-------|
| Block structure | ✅ Defined | Header in primitives.h |
| Transaction structure | ✅ Defined | In/out format defined |
| UTXO database | 📝 To implement | LevelDB or RocksDB |
| Block validation | 📝 To implement | PoW, signatures, rules |
| Chain state | 📝 To implement | Best chain tracking |
| Merkle tree | 📝 To implement | Binary tree, SHA3-256 |

### Networking (P2P)

| Component | Status | Notes |
|-----------|--------|-------|
| Protocol messages | ✅ Defined | In p2p.h |
| Message serialization | 📝 To implement | Binary protocol |
| Peer management | 📝 To implement | Connection pool |
| Block relay | 📝 To implement | Inv/GetData pattern |
| Transaction relay | 📝 To implement | Mempool propagation |

### Mempool

| Component | Status | Notes |
|-----------|--------|-------|
| Transaction pool | ✅ Defined | Priority queue in mempool.h |
| Fee estimation | 📝 To implement | Historical data |
| Conflict detection | 📝 To implement | Double-spend check |
| Eviction policy | 📝 To implement | Low-fee removal |

### Wallet

| Component | Status | Notes |
|-----------|--------|-------|
| HD wallet structure | ✅ Defined | BIP32-style derivation |
| Key generation | 📝 To implement | Dilithium keypairs |
| Address generation | 📝 To implement | From public keys |
| Transaction building | 📝 To implement | Coin selection, signing |
| Wallet database | 📝 To implement | SQLite or LevelDB |

### Mining

| Component | Status | Notes |
|-----------|--------|-------|
| Block template | 📝 To implement | From mempool |
| SHA-256 PoW | 📝 To implement | Double SHA-256 |
| Difficulty adjustment | 📝 To implement | Every 2016 blocks |
| Multi-threading | 📝 To implement | Thread pool |

### RPC Server

| Component | Status | Notes |
|-----------|--------|-------|
| JSON-RPC 2.0 | ✅ Defined | Protocol in rpc.h |
| HTTP server | 📝 To implement | Boost.Beast or similar |
| Authentication | 📝 To implement | Username/password |
| Command handlers | 📝 To implement | 25+ methods |

---

## Implementation Priority

### Phase 1: Core Crypto (Week 1-2)
**Goal**: Working cryptographic primitives

1. **Implement random.cpp**
   ```cpp
   // Use OpenSSL RAND_bytes
   void random_bytes(uint8_t* output, size_t len);
   ```

2. **Implement hkdf.cpp**
   ```cpp
   // Use OpenSSL EVP_KDF
   std::vector<uint8_t> hkdf_expand(
       const std::vector<uint8_t>& key,
       const std::string& info,
       size_t output_len
   );
   ```

3. **Implement mnemonic.cpp**
   ```cpp
   // BIP39 word list (2048 words)
   // PBKDF2 for seed derivation
   std::string generate_mnemonic(size_t bits = 256);
   std::vector<uint8_t> mnemonic_to_seed(const std::string& mnemonic);
   ```

4. **Implement address.cpp**
   ```cpp
   // Bech32 encoding with "int1" HRP
   std::string encode_address(const DilithiumPubKey& pubkey);
   std::optional<DilithiumPubKey> decode_address(const std::string& addr);
   ```

### Phase 2: Blockchain Core (Week 3-5)
**Goal**: Block validation and chain state

1. **Implement block.cpp**
   - Block header serialization
   - Block hash calculation (SHA-256 double hash)
   - PoW validation (`hash < target`)

2. **Implement transaction.cpp**
   - Transaction serialization
   - Transaction ID calculation (SHA3-256)
   - Input/output handling

3. **Implement merkle.cpp**
   - Binary merkle tree
   - Root calculation
   - Merkle proof generation/verification

4. **Implement blockchain.cpp**
   - UTXO database (LevelDB)
   - Block validation logic
   - Chain reorganization
   - Best chain tracking
   - Difficulty adjustment

### Phase 3: Networking (Week 6-7)
**Goal**: P2P communication

1. **Implement p2p.cpp**
   - TCP socket management (Boost.Asio)
   - Message framing
   - Protocol message handlers
   - Peer discovery
   - Block/transaction relay

2. **Peer management**
   - Connection limits (125 peers max)
   - Misbehavior scoring
   - Peer banning
   - Address database

### Phase 4: Mempool (Week 8)
**Goal**: Transaction pool

1. **Implement mempool.cpp**
   - Priority queue (fee-based)
   - Conflict detection
   - Mempool limits (300 MB)
   - Orphan transaction handling

### Phase 5: Wallet (Week 9-10)
**Goal**: HD wallet functionality

1. **Implement wallet.cpp**
   - Key derivation (HKDF-based)
   - Address generation
   - UTXO tracking
   - Transaction building
   - Coin selection
   - Wallet encryption (AES-256-CBC)

### Phase 6: Mining (Week 11)
**Goal**: CPU mining

1. **Implement miner.cpp**
   - Block template creation
   - SHA-256 mining loop
   - Multi-threaded mining
   - Solution submission

### Phase 7: RPC Server (Week 12)
**Goal**: JSON-RPC interface

1. **Implement rpc_server.cpp**
   - HTTP server (Boost.Beast)
   - JSON parsing
   - Authentication
   - Command handlers (25+ methods)

### Phase 8: Integration & Testing (Week 13-14)
**Goal**: Functional node

1. **Implement daemon (intcoind)**
   - Initialization
   - Component lifecycle
   - Signal handling
   - Configuration loading

2. **Implement CLI (intcoin-cli)**
   - RPC client
   - Command-line parsing
   - Output formatting

3. **Testing**
   - Unit tests
   - Integration tests
   - Network testing

---

## Detailed Implementation Notes

### UTXO Database Schema

```cpp
// LevelDB key-value pairs
// Outpoint -> UTXO
Key: "u" + txid (32 bytes) + vout (4 bytes)
Value: amount (8 bytes) + script_pubkey (variable)

// Block index
Key: "b" + block_hash (32 bytes)
Value: height (4 bytes) + header (variable)

// Best chain
Key: "best"
Value: block_hash (32 bytes)
```

### Transaction Validation Rules

1. **Format validation**
   - Non-empty inputs and outputs
   - Valid script formats
   - Dilithium signatures present

2. **Signature validation**
   - Each input must have valid Dilithium5 signature
   - Signature covers transaction hash
   - Public key matches previous output

3. **Value validation**
   - Sum(inputs) >= Sum(outputs) + fee
   - No negative values
   - No integer overflow

4. **UTXO validation**
   - All inputs exist in UTXO set
   - Not already spent (double-spend check)

### Block Validation Rules

1. **Header validation**
   - Valid PoW (hash < target)
   - Correct difficulty target
   - Valid timestamp (within 2 hours)
   - Correct height

2. **Transaction validation**
   - First transaction is coinbase
   - All other transactions validated
   - Merkle root matches

3. **Coinbase validation**
   - Correct reward amount (50 INT initially)
   - Halving every 210,000 blocks
   - Height in coinbase

### Difficulty Adjustment

```cpp
// Every 2016 blocks (~2 weeks)
if (height % 2016 == 0) {
    uint64_t actual_time = current_time - prev_adjustment_time;
    uint64_t expected_time = 2016 * 600; // 10 minutes per block

    // Limit adjustment to 4x
    if (actual_time < expected_time / 4) actual_time = expected_time / 4;
    if (actual_time > expected_time * 4) actual_time = expected_time * 4;

    new_target = old_target * actual_time / expected_time;
}
```

### P2P Protocol Flow

1. **Connection establishment**
   ```
   Node A -> Node B: VERSION
   Node B -> Node A: VERACK
   Node B -> Node A: VERSION
   Node A -> Node B: VERACK
   ```

2. **Block synchronization**
   ```
   Node A -> Node B: GETHEADERS (from known block)
   Node B -> Node A: HEADERS (up to 2000)
   Node A -> Node B: GETDATA (specific blocks)
   Node B -> Node A: BLOCK (data)
   ```

3. **Transaction relay**
   ```
   Node A -> Node B: INV (tx hash)
   Node B -> Node A: GETDATA (if interested)
   Node A -> Node B: TX (data)
   ```

---

## Testing Strategy

### Unit Tests

```cpp
// tests/test_crypto.cpp
TEST_CASE("SHA3-256 produces correct hash") {
    auto hash = SHA3_256::hash({0x01, 0x02, 0x03});
    REQUIRE(hash.size() == 32);
    // Check against known test vector
}

TEST_CASE("Dilithium signature verifies correctly") {
    auto keypair = Dilithium::generate_keypair();
    Hash256 message = /* ... */;
    auto signature = Dilithium::sign(keypair.private_key, message);
    REQUIRE(Dilithium::verify(keypair.public_key, message, signature));
}
```

### Integration Tests

```python
# tests/test_rpc.py
def test_blockchain_sync():
    node1 = start_node(0)
    node2 = start_node(1)
    connect_nodes(node1, node2)

    # Mine blocks on node1
    node1.generate(10)
    sync_blocks([node1, node2])

    assert node1.getblockcount() == node2.getblockcount()
```

---

## Dependencies

### Required
- **CMake** 3.20+
- **C++23** compiler (GCC 11+, Clang 14+)
- **OpenSSL** 3.0+ (or 1.1.1+)
- **Boost** 1.70+
- **Qt5** (for GUI)

### Optional
- **liboqs** 0.10+ (for real post-quantum crypto)
- **LevelDB** or **RocksDB** (for database)
- **Google Test** (for testing)

---

## Build Instructions

```bash
# Install dependencies (Ubuntu example)
sudo apt install build-essential cmake \
    libboost-all-dev libssl-dev \
    libleveldb-dev libgtest-dev \
    qtbase5-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
ctest

# Install
sudo make install
```

---

## Integration with liboqs

When ready for production:

1. **Install liboqs**
   ```bash
   git clone https://github.com/open-quantum-safe/liboqs.git
   cd liboqs && mkdir build && cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   make -j$(nproc) && sudo make install
   ```

2. **Replace stub implementations**
   - Remove `dilithium_stub.cpp` and `kyber_stub.cpp`
   - Restore original `dilithium.cpp` and `kyber.cpp`
   - Update `CMakeLists.txt` to link liboqs

3. **Update CMakeLists.txt**
   ```cmake
   find_package(liboqs REQUIRED)
   target_link_libraries(crypto PUBLIC liboqs::oqs)
   ```

---

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| Block validation | < 100ms | Including signature verification |
| Transaction broadcast | < 50ms | To all peers |
| Mempool addition | < 10ms | With conflict check |
| Mining hashrate | 100+ KH/s | Per CPU core |
| Sync speed | 1000+ blocks/min | Initial sync |
| RPC latency | < 5ms | Local calls |

---

## Security Considerations

1. **Input validation**
   - Validate all network messages
   - Check buffer sizes
   - Prevent integer overflow

2. **Resource limits**
   - Max 125 peer connections
   - Max 300 MB mempool
   - Max 2000 headers per message

3. **DoS protection**
   - Rate limiting on messages
   - Peer banning for misbehavior
   - Proof of work on connections

4. **Wallet security**
   - Encrypt private keys (AES-256)
   - Secure key derivation (HKDF)
   - Clear sensitive memory

---

## Next Steps

1. Review this guide
2. Set up development environment
3. Start with Phase 1 (Core Crypto)
4. Implement incrementally
5. Test each component
6. Integrate and test together

---

## Resources

- **liboqs**: https://github.com/open-quantum-safe/liboqs
- **Bitcoin**: https://github.com/bitcoin/bitcoin (reference implementation)
- **Boost.Asio**: https://www.boost.org/doc/libs/release/doc/html/boost_asio.html
- **LevelDB**: https://github.com/google/leveldb
- **OpenSSL**: https://www.openssl.org/docs/

---

**For questions or discussion**: team@international-coin.org

**Last Updated**: January 2025
