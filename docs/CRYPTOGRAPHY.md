# INTcoin Cryptography

**Status**: ✅ Complete (100%)
**Tests**: 5/5 passing
**Last Updated**: November 26, 2025

INTcoin uses post-quantum cryptographic algorithms approved by NIST to ensure long-term security in the post-quantum era.

---

## Table of Contents

1. [Overview](#overview)
2. [Digital Signatures (Dilithium3)](#digital-signatures-dilithium3)
3. [Key Encapsulation (Kyber768)](#key-encapsulation-kyber768)
4. [Hashing (SHA3-256)](#hashing-sha3-256)
5. [Implementation](#implementation)
6. [Security Considerations](#security-considerations)
7. [Test Coverage](#test-coverage)

---

## Overview

INTcoin implements a hybrid post-quantum cryptographic system combining:

| Component | Algorithm | NIST Standard | Security Level |
|-----------|-----------|---------------|----------------|
| **Digital Signatures** | Dilithium3 (ML-DSA-65) | FIPS 204 | Level 3 (~AES-192) |
| **Key Encapsulation** | Kyber768 (ML-KEM-768) | FIPS 203 | Level 3 (~AES-192) |
| **Hashing** | SHA3-256 | FIPS 202 | 256-bit |

### Why Post-Quantum Cryptography?

Traditional cryptographic algorithms (RSA, ECDSA) will be vulnerable to quantum computers using Shor's algorithm. INTcoin uses lattice-based cryptography that is believed to be secure against both classical and quantum attacks.

---

## Digital Signatures (Dilithium3)

### Overview

Dilithium is a lattice-based digital signature scheme based on the hardness of Module-LWE (Learning With Errors) and Module-SIS (Short Integer Solution) problems.

### Specification

```cpp
// Key sizes
Public Key:  1952 bytes
Secret Key:  4000 bytes
Signature:   3293 bytes (variable, but ≤ 3293)

// Security Parameters
Security Level: NIST Level 3 (equivalent to AES-192)
Quantum Security: ~128 bits
Classical Security: ~192 bits
```

### API Usage

```cpp
#include "intcoin/crypto.h"

// Generate keypair
auto keypair_result = DilithiumCrypto::GenerateKeyPair();
if (keypair_result.IsOk()) {
    PublicKey public_key = keypair_result.value->public_key;
    SecretKey secret_key = keypair_result.value->secret_key;
}

// Sign message
std::vector<uint8_t> message = {/* your message */};
auto sig_result = DilithiumCrypto::Sign(message, secret_key);
if (sig_result.IsOk()) {
    Signature signature = *sig_result.value;
}

// Verify signature
auto verify_result = DilithiumCrypto::Verify(message, signature, public_key);
if (verify_result.IsOk()) {
    // Signature is valid
}

// Sign hash (common in blockchain)
uint256 hash = SHA3::Hash(message);
auto sig_result = DilithiumCrypto::SignHash(hash, secret_key);
```

### Performance

| Operation | Time (Apple M1) | Cycles |
|-----------|----------------|--------|
| KeyGen | ~0.15 ms | ~75,000 |
| Sign | ~0.30 ms | ~150,000 |
| Verify | ~0.18 ms | ~90,000 |

---

## Key Encapsulation (Kyber768)

### Overview

Kyber is a lattice-based Key Encapsulation Mechanism (KEM) used for establishing shared secrets. In INTcoin, it's used for encrypted peer-to-peer communication and secure wallet backup.

### Specification

```cpp
// Key sizes
Public Key:    1184 bytes
Secret Key:    2400 bytes
Ciphertext:    1088 bytes
Shared Secret: 32 bytes

// Security Parameters
Security Level: NIST Level 3 (equivalent to AES-192)
Quantum Security: ~128 bits
Classical Security: ~192 bits
```

### API Usage

```cpp
#include "intcoin/crypto.h"

// Generate keypair
auto keypair_result = KyberCrypto::GenerateKeyPair();
if (keypair_result.IsOk()) {
    auto public_key = keypair_result.value->public_key;
    auto secret_key = keypair_result.value->secret_key;
}

// Encapsulate (sender)
auto encap_result = KyberCrypto::Encapsulate(public_key);
if (encap_result.IsOk()) {
    auto [shared_secret, ciphertext] = *encap_result.value;
    // Send ciphertext to recipient
    // Use shared_secret to encrypt data
}

// Decapsulate (recipient)
auto decap_result = KyberCrypto::Decapsulate(ciphertext, secret_key);
if (decap_result.IsOk()) {
    auto shared_secret = *decap_result.value;
    // Use shared_secret to decrypt data
}
```

### Performance

| Operation | Time (Apple M1) | Cycles |
|-----------|----------------|--------|
| KeyGen | ~0.08 ms | ~40,000 |
| Encaps | ~0.10 ms | ~50,000 |
| Decaps | ~0.12 ms | ~60,000 |

---

## Hashing (SHA3-256)

### Overview

SHA3-256 is used throughout INTcoin for:
- Transaction IDs
- Block hashes (in combination with RandomX)
- Public key hashing (for addresses)
- Merkle tree construction
- RandomX epoch key generation

### Specification

```cpp
Input: Variable length
Output: 32 bytes (256 bits)
Algorithm: Keccak-256 (NIST FIPS 202)
```

### API Usage

```cpp
#include "intcoin/crypto.h"

// Hash data
std::vector<uint8_t> data = {/* your data */};
uint256 hash = SHA3::Hash(data);

// Double hash (common in blockchain)
uint256 double_hash = SHA3::DoubleHash(data);

// Hash with pointer and length
const uint8_t* ptr = data.data();
size_t len = data.size();
uint256 hash2 = SHA3::Hash(ptr, len);

// Convert to hex string
std::string hash_hex = ToHex(hash);
```

### Performance

| Operation | Throughput | Time (1 KB) |
|-----------|-----------|-------------|
| Hash | ~500 MB/s | ~2 μs |
| DoubleHash | ~250 MB/s | ~4 μs |

---

## Implementation

### Dependencies

```bash
# liboqs (Open Quantum Safe)
Version: 0.15.0
Algorithms: ML-DSA-65 (Dilithium3), ML-KEM-768 (Kyber768)
Installation: See BUILD_STATUS.md

# OpenSSL
Version: 3.5.4+
Algorithms: SHA3-256
```

### Source Files

```
src/crypto/crypto.cpp       # Implementation
include/intcoin/crypto.h    # API definitions
tests/test_crypto.cpp       # Test suite
```

### Key Functions

```cpp
// Dilithium3 (ML-DSA-65)
Result<DilithiumCrypto::KeyPair> DilithiumCrypto::GenerateKeyPair();
Result<Signature> DilithiumCrypto::Sign(const vector<uint8_t>& message, const SecretKey& sk);
Result<Signature> DilithiumCrypto::SignHash(const uint256& hash, const SecretKey& sk);
Result<void> DilithiumCrypto::Verify(const vector<uint8_t>& message, const Signature& sig, const PublicKey& pk);
Result<void> DilithiumCrypto::VerifyHash(const uint256& hash, const Signature& sig, const PublicKey& pk);

// Kyber768 (ML-KEM-768)
Result<KyberCrypto::KeyPair> KyberCrypto::GenerateKeyPair();
Result<pair<SharedSecret, Ciphertext>> KyberCrypto::Encapsulate(const PublicKey& pk);
Result<SharedSecret> KyberCrypto::Decapsulate(const Ciphertext& ct, const SecretKey& sk);

// SHA3-256
uint256 SHA3::Hash(const vector<uint8_t>& data);
uint256 SHA3::DoubleHash(const vector<uint8_t>& data);
uint256 SHA3::Hash(const uint8_t* data, size_t len);
```

---

## Security Considerations

### Randomness

All cryptographic operations use cryptographically secure random number generators (CSPRNG):
- `/dev/urandom` on Unix-like systems
- `CryptGenRandom` on Windows
- System entropy pools

### Side-Channel Protection

- **Constant-time operations**: All signature verification uses constant-time comparison
- **Memory wiping**: Secret keys are securely wiped with `SecureWipe()`
- **No branching on secrets**: Critical operations avoid data-dependent branches

### Known Limitations

1. **Key Size**: Post-quantum keys and signatures are larger than classical cryptography
   - Dilithium public key: 1952 bytes vs. ECDSA 33 bytes
   - Signature: 3293 bytes vs. ECDSA 64-72 bytes

2. **Performance**: Slower than classical cryptography but acceptable for blockchain use
   - Sign: ~0.3 ms (vs. ECDSA ~0.05 ms)
   - Verify: ~0.18 ms (vs. ECDSA ~0.15 ms)

### Future Enhancements

- [ ] Hardware acceleration (AVX2, NEON)
- [ ] Batch signature verification
- [ ] Signature aggregation research
- [ ] Integration with hardware security modules (HSM)

---

## Test Coverage

### Test Suite (tests/test_crypto.cpp)

**Status**: ✅ 5/5 tests passing

1. **Dilithium Keypair Generation**
   - Generates valid keypairs
   - Keys have correct sizes
   - Multiple generations produce different keys

2. **Dilithium Sign/Verify**
   - Signs messages correctly
   - Verifies valid signatures
   - Rejects invalid signatures
   - Rejects tampered messages

3. **Kyber Key Encapsulation**
   - Generates valid keypairs
   - Encapsulation succeeds
   - Decapsulation recovers shared secret
   - Shared secrets match

4. **SHA3-256 Hashing**
   - Produces 32-byte hashes
   - Deterministic (same input → same output)
   - Different inputs → different outputs
   - Double hashing works correctly

5. **Public Key to Address**
   - Converts public keys to hashes
   - Deterministic conversion
   - Different keys → different hashes

### Running Tests

```bash
cd build
./tests/test_crypto

# Expected output:
# ========================================
# Cryptography Tests
# ========================================
# ✓ Test 1: Dilithium Keypair Generation
# ✓ Test 2: Dilithium Sign/Verify
# ✓ Test 3: Kyber Key Encapsulation
# ✓ Test 4: SHA3-256 Hashing
# ✓ Test 5: Public Key to Address
# ========================================
# FINAL RESULTS: 5/5 tests passed
# ========================================
```

---

## References

### NIST Standards

- [FIPS 202: SHA-3 Standard](https://csrc.nist.gov/publications/detail/fips/202/final)
- [FIPS 203: Module-Lattice-Based Key-Encapsulation Mechanism Standard (ML-KEM)](https://csrc.nist.gov/publications/detail/fips/203/final)
- [FIPS 204: Module-Lattice-Based Digital Signature Standard (ML-DSA)](https://csrc.nist.gov/publications/detail/fips/204/final)

### Papers

- [Dilithium: Crystals-Dilithium Algorithm Specifications](https://pq-crystals.org/dilithium/data/dilithium-specification-round3-20210208.pdf)
- [Kyber: Crystals-Kyber Algorithm Specifications](https://pq-crystals.org/kyber/data/kyber-specification-round3-20210804.pdf)
- [The Keccak SHA-3 Submission](https://keccak.team/keccak_specs_summary.html)

### Implementation

- [liboqs: Open Quantum Safe](https://github.com/open-quantum-safe/liboqs)
- [OpenSSL 3.x Documentation](https://www.openssl.org/docs/man3.0/)

---

**Next**: [Consensus (RandomX + Digishield)](CONSENSUS.md)
