# CRYSTALS Cryptography Integration Guide

This document explains how to integrate production-ready CRYSTALS-Dilithium and CRYSTALS-Kyber implementations into INTcoin.

## Overview

INTcoin uses post-quantum cryptography algorithms from the NIST PQC standardization process:
- **CRYSTALS-Dilithium5** for digital signatures (NIST Level 5 security)
- **CRYSTALS-Kyber1024** for key encapsulation (NIST Level 5 security)

The build system supports three implementation modes:

1. **liboqs (Recommended)** - Open Quantum Safe library with optimized implementations
2. **Reference Implementation** - NIST reference code via git submodules
3. **Stub Implementation** - Placeholder for development (NOT SECURE)

## Option 1: Using liboqs (Recommended)

liboqs provides production-ready, optimized implementations of post-quantum cryptography algorithms.

### Installation

#### Ubuntu/Debian
```bash
# Install dependencies
sudo apt update
sudo apt install cmake ninja-build gcc g++ libssl-dev

# Clone and build liboqs
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -GNinja .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON
ninja
sudo ninja install
sudo ldconfig
```

#### macOS
```bash
# Using Homebrew
brew install liboqs

# Or build from source
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -GNinja .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON
ninja
sudo ninja install
```

#### FreeBSD
```bash
# Install from ports
cd /usr/ports/security/liboqs
make install clean

# Or build from source
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -GNinja .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON
ninja
sudo ninja install
```

#### Windows
```powershell
# Install dependencies via vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install liboqs:x64-windows

# Or build from source
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="C:\Program Files\liboqs"
cmake --build . --config Release
cmake --install .
```

### Building INTcoin with liboqs

Once liboqs is installed, INTcoin will automatically detect and use it:

```bash
cd /path/to/intcoin
rm -rf build  # Clean previous build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

You should see:
```
-- Found liboqs - using Open Quantum Safe implementation
```

### Verification

```bash
# Check that liboqs is linked
ldd build/intcoind | grep oqs
# Should show: liboqs.so.X => /usr/local/lib/liboqs.so.X

# Check compile definitions
grep -r "INTCOIN_USE_LIBOQS" build/
```

## Option 2: Using Reference Implementations

If liboqs is not available, you can use the NIST reference implementations via git submodules.

### Adding Submodules

```bash
cd /path/to/intcoin

# Add CRYSTALS-Dilithium reference
git submodule add https://github.com/pq-crystals/dilithium.git external/dilithium/ref
cd external/dilithium/ref
git checkout round3  # Use round 3 reference implementation
cd ../../..

# Add CRYSTALS-Kyber reference
git submodule add https://github.com/pq-crystals/kyber.git external/kyber/ref
cd external/kyber/ref
git checkout round3  # Use round 3 reference implementation
cd ../../..

# Initialize submodules
git submodule update --init --recursive
```

### Building with Reference Implementations

```bash
cd /path/to/intcoin
mkdir build && cd build
cmake ..
make -j$(nproc)
```

You should see:
```
-- Found CRYSTALS-Dilithium reference implementation
-- Found CRYSTALS-Kyber reference implementation
```

### Verification

```bash
# Check compile definitions
grep -r "INTCOIN_USE_DILITHIUM_REF" build/
grep -r "INTCOIN_USE_KYBER_REF" build/

# Run tests
cd build
ctest -R crypto
```

## Option 3: Stub Implementation (Development Only)

⚠️ **WARNING**: The stub implementation is NOT cryptographically secure and should NEVER be used in production!

If neither liboqs nor reference implementations are available, the build will use stubs with clear warnings:

```
-- WARNING: CRYSTALS-Dilithium implementation not found!
-- WARNING: Using STUB implementation - NOT SECURE FOR PRODUCTION!
-- WARNING: CRYSTALS-Kyber implementation not found!
-- WARNING: Using STUB implementation - NOT SECURE FOR PRODUCTION!
```

The stub implementations:
- Generate deterministic, insecure keys
- Use placeholder signature/encryption operations
- Are suitable ONLY for:
  - Build testing
  - Development without crypto requirements
  - CI/CD pipeline validation

**Never use stub implementations for**:
- Production deployments
- Testnet deployments
- Any environment handling real value
- Security testing

## Implementation Details

### CRYSTALS-Dilithium Configuration

INTcoin uses Dilithium5 (NIST Level 5):
- **Public key size**: 2,592 bytes
- **Secret key size**: 4,864 bytes
- **Signature size**: 4,595 bytes
- **Security level**: 256-bit classical, quantum-resistant

### CRYSTALS-Kyber Configuration

INTcoin uses Kyber1024 (NIST Level 5):
- **Public key size**: 1,568 bytes
- **Secret key size**: 3,168 bytes
- **Ciphertext size**: 1,568 bytes
- **Shared secret**: 32 bytes
- **Security level**: 256-bit classical, quantum-resistant

### Compile Definitions

The build system sets these definitions based on the chosen implementation:

| Definition | Meaning |
|------------|---------|
| `INTCOIN_USE_LIBOQS` | Using liboqs library |
| `INTCOIN_USE_DILITHIUM_REF` | Using Dilithium reference implementation |
| `INTCOIN_USE_KYBER_REF` | Using Kyber reference implementation |
| `INTCOIN_USE_DILITHIUM_STUB` | Using Dilithium stub (NOT SECURE) |
| `INTCOIN_USE_KYBER_STUB` | Using Kyber stub (NOT SECURE) |
| `DILITHIUM_MODE=5` | Dilithium security level 5 |
| `KYBER_K=4` | Kyber1024 parameter set |

### Code Usage Example

```cpp
#include "intcoin/crypto/dilithium.h"
#include "intcoin/crypto/kyber.h"

// Generate Dilithium keypair
DilithiumKeypair dilithium_keys;
if (!dilithium_keygen(&dilithium_keys)) {
    // Handle error
}

// Sign message
std::vector<uint8_t> message = {/* ... */};
DilithiumSignature signature;
if (!dilithium_sign(message.data(), message.size(),
                    &dilithium_keys.secret, &signature)) {
    // Handle error
}

// Verify signature
bool valid = dilithium_verify(message.data(), message.size(),
                               &signature, &dilithium_keys.public_key);

// Generate Kyber keypair
KyberKeypair kyber_keys;
if (!kyber_keygen(&kyber_keys)) {
    // Handle error
}

// Encapsulate (encrypt)
KyberCiphertext ciphertext;
KyberSharedSecret shared_secret_sender;
if (!kyber_encaps(&ciphertext, &shared_secret_sender,
                  &kyber_keys.public_key)) {
    // Handle error
}

// Decapsulate (decrypt)
KyberSharedSecret shared_secret_receiver;
if (!kyber_decaps(&shared_secret_receiver, &ciphertext,
                  &kyber_keys.secret)) {
    // Handle error
}

// shared_secret_sender == shared_secret_receiver (32 bytes)
```

## Performance Considerations

### liboqs Performance (Recommended)

liboqs includes AVX2 optimizations on x86_64:
- **Dilithium5 sign**: ~1.5 ms
- **Dilithium5 verify**: ~1.5 ms
- **Kyber1024 keygen**: ~0.5 ms
- **Kyber1024 encaps**: ~0.7 ms
- **Kyber1024 decaps**: ~0.9 ms

### Reference Implementation Performance

The NIST reference implementations are not optimized:
- **Dilithium5 sign**: ~5-8 ms
- **Dilithium5 verify**: ~5-8 ms
- **Kyber1024 keygen**: ~2-3 ms
- **Kyber1024 encaps**: ~2-3 ms
- **Kyber1024 decaps**: ~3-4 ms

**Recommendation**: Use liboqs for production deployments to achieve optimal performance.

## Testing

### Unit Tests

```bash
cd build
ctest -R crypto -V
```

Tests verify:
- Keypair generation
- Signing and verification
- Encapsulation and decapsulation
- Key serialization/deserialization
- Cross-implementation compatibility

### NIST Test Vectors

INTcoin includes NIST Known Answer Tests (KATs):

```bash
cd build
./test_dilithium_nist
./test_kyber_nist
```

These tests verify:
- Deterministic key generation
- Signature determinism
- Ciphertext determinism
- NIST test vector compliance

### Integration Tests

```bash
cd build
./test_blockchain  # Tests blockchain with real crypto
./test_wallet      # Tests wallet operations
./test_network     # Tests network message signing
```

## Troubleshooting

### liboqs Not Found

**Error**: `CMake Warning: liboqs not found`

**Solution**:
1. Verify liboqs is installed: `pkg-config --modversion liboqs`
2. Set `PKG_CONFIG_PATH`: `export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH`
3. Or specify path: `cmake -Dliboqs_DIR=/usr/local/lib/cmake/liboqs ..`

### Reference Implementation Build Errors

**Error**: `ref/sign.c: No such file or directory`

**Solution**:
1. Initialize submodules: `git submodule update --init --recursive`
2. Check submodule status: `git submodule status`
3. Verify files exist: `ls external/dilithium/ref/sign.c`

### Stub Implementation Warnings

**Warning**: `Using STUB implementation - NOT SECURE FOR PRODUCTION!`

**Solution**:
- Install liboqs (recommended)
- Or add reference implementations as submodules
- Never deploy with stub implementations

### Performance Issues

**Symptom**: Slow signing/verification operations

**Solutions**:
1. Use liboqs with AVX2 optimizations
2. Enable compiler optimizations: `cmake -DCMAKE_BUILD_TYPE=Release ..`
3. Use native architecture: `cmake -DCMAKE_CXX_FLAGS="-march=native" ..`
4. Check CPU features: `lscpu | grep avx2`

### ABI Compatibility Issues

**Error**: `undefined reference to 'pqcrystals_dilithium5_ref_...'`

**Solution**:
1. Clean build directory: `rm -rf build && mkdir build`
2. Rebuild from scratch: `cd build && cmake .. && make clean && make`
3. Verify symbol names: `nm -D /usr/local/lib/liboqs.so | grep dilithium`

## Security Recommendations

1. **Always use liboqs or reference implementations in production**
2. **Never use stub implementations for real deployments**
3. **Keep cryptography libraries up to date**
4. **Monitor NIST PQC standardization updates**
5. **Use hardware RNG for key generation** (`/dev/urandom` on Unix)
6. **Implement proper key management and storage**
7. **Regularly test with NIST test vectors**
8. **Consider hardware security modules (HSMs) for key storage**

## Migration Path

### From Stub to Reference Implementation

```bash
# Add submodules
git submodule add https://github.com/pq-crystals/dilithium.git external/dilithium/ref
git submodule add https://github.com/pq-crystals/kyber.git external/kyber/ref
git submodule update --init --recursive

# Rebuild
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Verify
grep "INTCOIN_USE.*REF" CMakeCache.txt
```

### From Reference to liboqs

```bash
# Install liboqs (see Option 1 above)

# Rebuild
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Verify
ldd intcoind | grep oqs
```

## Related Documentation

- [NIST PQC Standardization](https://csrc.nist.gov/projects/post-quantum-cryptography)
- [CRYSTALS-Dilithium](https://pq-crystals.org/dilithium/)
- [CRYSTALS-Kyber](https://pq-crystals.org/kyber/)
- [Open Quantum Safe](https://openquantumsafe.org/)
- [liboqs Documentation](https://github.com/open-quantum-safe/liboqs/wiki)
- [INTcoin Cryptography Design](../wiki/Cryptography.md)

## Support

For issues with cryptography integration:
- **GitHub Issues**: https://github.com/intcoin/core/issues
- **Email**: security@intcoin.org
- **Discord**: #crypto-dev channel

---

**Last Updated**: November 2025
**Version**: 1.2.0
