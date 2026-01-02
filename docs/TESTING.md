# INTcoin Testing Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Test Status**: ✅ 17/17 test suites passing (100%)

This document describes the testing framework, test suites, and how to run and write tests for INTcoin.

## What's New in v1.2.0-beta

- ✅ **5 New Test Suites**: Enhanced Mempool, Bloom Filters, Prometheus Metrics, Atomic Swaps, Bridges
- ✅ **30+ New Tests**: Comprehensive coverage of all v1.2.0 features
- ✅ **Mobile SDK Tests**: iOS and Android SDK unit tests
- ✅ **100% Pass Rate**: All 17 test suites passing

---

## Table of Contents

- [Overview](#overview)
- [Test Infrastructure](#test-infrastructure)
- [Running Tests](#running-tests)
- [Test Suites](#test-suites)
- [Writing Tests](#writing-tests)
- [Test Coverage](#test-coverage)
- [Continuous Integration](#continuous-integration)
- [Best Practices](#best-practices)

---

## Overview

INTcoin uses a comprehensive testing strategy to ensure reliability and correctness:

- **Unit Tests**: Test individual components in isolation
- **Integration Tests**: Test component interactions
- **Functional Tests**: Test end-to-end workflows
- **Performance Tests**: Benchmark critical operations

**Current Status**:
- ✅ 17 test suites (12 core + 5 new in v1.2.0)
- ✅ 100% pass rate across all suites
- ✅ ~15s total runtime
- ✅ All critical components covered
- ✅ Mobile SDK tests included

---

## Test Infrastructure

### Build System

Tests are built using CMake and CTest:

```cmake
# tests/CMakeLists.txt
add_executable(test_crypto test_crypto.cpp)
target_link_libraries(test_crypto intcoin_core)

add_test(NAME CryptoTest COMMAND test_crypto)
```

### Test Framework

INTcoin uses a simple assertion-based testing framework without external dependencies:

```cpp
// Simple assertion macros
#include <cassert>
#include <iostream>

void test_feature() {
    std::cout << "Testing feature...\n";

    // Test assertions
    assert(condition == true);
    assert(result.IsOk());
    assert(value == expected);

    std::cout << "✓ Feature test passed\n";
}
```

### Test Structure

Each test file follows this structure:

```cpp
// tests/test_example.cpp
#include "intcoin/intcoin.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

void test_feature_1() {
    std::cout << "Testing feature 1...\n";
    // Test code
    assert(result);
    std::cout << "✓ Feature 1 passed\n";
}

void test_feature_2() {
    std::cout << "Testing feature 2...\n";
    // Test code
    assert(result);
    std::cout << "✓ Feature 2 passed\n";
}

int main() {
    std::cout << "=== Example Test Suite ===\n";

    test_feature_1();
    test_feature_2();

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
```

---

## Running Tests

### Build Tests

```bash
# Build project with tests enabled
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

### Run All Tests

```bash
# Using CTest
cd build
ctest

# Verbose output
ctest -V

# Parallel execution
ctest -j$(nproc)
```

### Run Individual Tests

```bash
# Run specific test
./tests/test_crypto

# Run with verbose output
./tests/test_wallet -v

# Run specific test case
./tests/test_crypto --test-case="Dilithium3"
```

### Run Tests by Pattern

```bash
# Run all crypto-related tests
ctest -R crypto

# Run all blockchain tests
ctest -R blockchain

# Exclude specific tests
ctest -E wallet
```

---

## Test Suites

### 1. CryptoTest (0.53s) ✅

**File**: `tests/test_crypto.cpp`

**Coverage**:
- Dilithium3 signature generation and verification
- Kyber768 key encapsulation and decapsulation
- SHA3-256 hashing
- Bech32 address encoding/decoding
- Public key to hash conversion

**Test Cases**:
```cpp
test_dilithium3_signature()    // Post-quantum signatures
test_kyber768_kem()             // Key encapsulation
test_sha3_256()                 // Cryptographic hashing
test_bech32_encoding()          // Address encoding
test_public_key_hash()          // Address generation
```

**Run**:
```bash
./tests/test_crypto
```

---

### 2. RandomXTest (3.66s) ✅

**File**: `tests/test_randomx.cpp`

**Coverage**:
- RandomX initialization and hashing
- Proof-of-Work validation
- Digishield V3 difficulty adjustment
- Block mining simulation
- Difficulty target calculation

**Test Cases**:
```cpp
test_randomx_init()             // RandomX setup
test_randomx_hashing()          // Hash computation
test_pow_validation()           // PoW verification
test_digishield_v3()            // Difficulty adjustment
test_difficulty_targets()       // Target calculation
```

**Run**:
```bash
./tests/test_randomx
```

---

### 3. Bech32Test (0.38s) ✅

**File**: `tests/test_bech32.cpp`

**Coverage**:
- Bech32 encoding
- Bech32 decoding
- Error detection
- HRP (Human-Readable Part) validation
- Checksum verification

**Test Cases**:
```cpp
test_bech32_encode()            // Encoding validation
test_bech32_decode()            // Decoding validation
test_bech32_errors()            // Error detection
test_invalid_hrp()              // Invalid prefix handling
```

**Run**:
```bash
./tests/test_bech32
```

---

### 4. SerializationTest (0.36s) ✅

**File**: `tests/test_serialization.cpp`

**Coverage**:
- Block serialization/deserialization
- Transaction serialization/deserialization
- Merkle tree serialization
- Binary format consistency
- Endianness handling

**Test Cases**:
```cpp
test_block_serialization()      // Block encoding
test_transaction_serialization() // Transaction encoding
test_merkle_tree()               // Merkle root calculation
test_binary_format()             // Format consistency
```

**Run**:
```bash
./tests/test_serialization
```

---

### 5. StorageTest (0.81s) ✅

**File**: `tests/test_storage.cpp`

**Coverage**:
- RocksDB database operations
- Block storage and retrieval
- Transaction indexing
- UTXO set persistence
- Batch operations
- Database cleanup

**Test Cases**:
```cpp
test_database_open()            // DB initialization
test_block_storage()            // Block persistence
test_transaction_index()        // TX indexing
test_utxo_operations()          // UTXO management
test_batch_operations()         // Atomic writes
test_database_cleanup()         // Resource cleanup
```

**Run**:
```bash
./tests/test_storage
```

---

### 6. ValidationTest (0.79s) ✅

**File**: `tests/test_validation.cpp`

**Coverage**:
- Block header validation
- Transaction validation
- UTXO validation
- Double-spend detection
- Fee validation
- Signature verification
- Merkle root verification

**Test Cases**:
```cpp
test_block_validation()         // Block structure
test_transaction_validation()   // TX structure
test_utxo_validation()          // UTXO checks
test_double_spend()             // Double-spend prevention
test_fee_validation()           // Fee calculation
test_signature_validation()     // Signature checks
test_merkle_validation()        // Merkle tree
```

**Run**:
```bash
./tests/test_validation
```

---

### 7. GenesisTest (0.40s) ✅

**File**: `tests/test_genesis.cpp`

**Coverage**:
- Genesis block creation
- Genesis block validation
- Initial UTXO set
- Timestamp verification
- News headline embedding

**Test Cases**:
```cpp
test_genesis_creation()         // Genesis generation
test_genesis_validation()       // Genesis validation
test_genesis_timestamp()        // Timestamp check
test_genesis_coinbase()         // Initial UTXO
```

**Genesis Block Timestamp**:
```
"13:18, 26 November 2025 This Is Money, Financial markets in turmoil
as Budget leak fiasco sends pound and gilts on rollercoaster ride"
```

**Run**:
```bash
./tests/test_genesis
```

---

### 8. NetworkTest (0.43s) ✅

**File**: `tests/test_network.cpp`

**Coverage**:
- P2P message serialization
- Network handshake (VERSION/VERACK)
- Peer management
- Mempool operations
- Message handling
- Peer banning

**Test Cases**:
```cpp
test_message_serialization()    // Message encoding
test_network_handshake()        // VERSION/VERACK
test_peer_management()          // Peer tracking
test_mempool_operations()       // TX pool
test_message_handlers()         // Message processing
test_peer_banning()             // Ban system
```

**Run**:
```bash
./tests/test_network
```

---

### 9. MLTest (0.50s) ✅

**File**: `tests/test_ml.cpp`

**Coverage**:
- Transaction anomaly detection
- Network behavior analysis
- Fee estimation
- Difficulty prediction
- Neural network training
- Statistical analysis

**Test Cases**:
```cpp
test_anomaly_detection()        // TX anomaly scoring
test_behavior_analysis()        // Peer reputation
test_fee_estimation()           // Smart fee prediction
test_difficulty_prediction()    // Hashrate forecasting
test_neural_network()           // ML training
test_statistics()               // Statistical utils
```

**Run**:
```bash
./tests/test_ml
```

---

### 10. WalletTest (0.86s) ✅

**File**: `tests/test_wallet.cpp`

**Coverage**:
- BIP39 mnemonic generation
- HD key derivation (BIP32/44)
- Address generation
- Wallet database operations
- UTXO management
- Transaction creation
- Transaction signing
- Balance tracking

**Test Cases**:
```cpp
test_mnemonic_generation()      // BIP39 phrases
test_mnemonic_validation()      // Phrase validation
test_seed_generation()          // Seed derivation
test_key_derivation()           // BIP32/44 paths
test_wallet_creation()          // Wallet init
test_address_generation()       // Address creation
test_wallet_database()          // DB operations
test_utxo_management()          // UTXO tracking
test_transaction_creation()     // TX building
test_transaction_signing()      // Dilithium3 signing
test_balance_tracking()         // Balance calculation
test_wallet_rescan()            // Blockchain rescan
```

**Run**:
```bash
./tests/test_wallet
```

---

### 11. MempoolTest (0.45s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_mempool.cpp`

**Coverage**:
- 6-level priority system
- Mempool persistence
- Transaction dependency tracking
- Priority-based eviction
- Fee-based prioritization
- CPFP (Child Pays For Parent)

**Test Cases**:
```cpp
test_priority_assignment()      // Priority level assignment
test_mempool_persistence()      // Save/load to disk
test_dependency_tracking()      // Parent-child relationships
test_priority_eviction()        // Eviction order
test_fee_prioritization()       // Fee-based sorting
test_htlc_priority()            // HTLC transaction priority
test_bridge_priority()          // Bridge transaction priority
test_critical_priority()        // Critical transaction handling
test_mempool_limits()           // Size and count limits
test_cpfp_support()             // Child pulls parent
test_expired_transactions()     // Transaction expiry
test_duplicate_detection()      // Prevent duplicates
```

**Run**:
```bash
./tests/test_mempool
```

---

### 12. BloomFilterTest (0.32s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_bloom.cpp`

**Coverage**:
- Bloom filter creation
- Element insertion and testing
- False positive rate validation
- Hash function correctness
- Update modes (NONE, ALL, P2PUBKEY_ONLY)
- Network protocol integration

**Test Cases**:
```cpp
test_bloom_creation()           // Filter initialization
test_element_insertion()        // Add elements
test_contains_test()            // Membership testing
test_false_positive_rate()      // FP rate validation
test_multiple_hash_funcs()      // k hash functions
test_update_modes()             // Filter update behavior
test_transaction_matching()     // TX relevance checking
test_dos_protection()           // Size limits
```

**Run**:
```bash
./tests/test_bloom
```

---

### 13. MetricsTest (0.38s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_metrics.cpp`

**Coverage**:
- Counter metrics
- Gauge metrics
- Histogram metrics
- Thread safety
- Metric registration
- Prometheus text format

**Test Cases**:
```cpp
test_counter_metrics()          // Counter increment/add
test_gauge_metrics()            // Gauge set/inc/dec
test_histogram_metrics()        // Histogram observe
test_metric_registration()      // Registry operations
test_thread_safety()            // Concurrent access
test_prometheus_format()        // Text export format
test_metric_labels()            // Label handling
test_histogram_buckets()        // Bucket distribution
test_metric_reset()             // Reset functionality
test_registry_iteration()       // Iterate all metrics
```

**Run**:
```bash
./tests/test_metrics
```

---

### 14. MetricsServerTest (0.41s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_metrics_server.cpp`

**Coverage**:
- HTTP server startup/shutdown
- GET /metrics endpoint
- Prometheus text response
- Error handling (404, 405)
- Multi-threaded server
- Configuration options

**Test Cases**:
```cpp
test_server_startup()           // Server initialization
test_metrics_endpoint()         // GET /metrics
test_prometheus_response()      // Response format
test_404_not_found()            // Invalid paths
test_405_method_not_allowed()   // POST/PUT/DELETE
test_concurrent_requests()      // Thread safety
test_server_shutdown()          // Clean shutdown
test_configuration()            // Bind/port settings
```

**Run**:
```bash
./tests/test_metrics_server
```

---

### 15. AtomicSwapTest (0.52s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_atomic_swap.cpp`

**Coverage**:
- HTLC contract creation
- Secret hash generation
- Timelock validation
- Swap initiation
- Swap redemption
- Swap refund after timeout

**Test Cases**:
```cpp
test_htlc_creation()            // Create HTLC contract
test_secret_generation()        // Generate secret & hash
test_swap_initiation()          // Initiator creates HTLC
test_swap_participation()       // Participant creates HTLC
test_swap_redemption()          // Redeem with secret
test_swap_refund()              // Refund after timeout
test_timelock_validation()      // Locktime checks
test_cross_chain_flow()         // Full swap flow
```

**Run**:
```bash
./tests/test_atomic_swap
```

---

### 16. BridgeTest (0.48s) ✅

*New in v1.2.0-beta*

**File**: `tests/test_bridge.cpp`

**Coverage**:
- Bridge deposit operations
- Bridge withdrawal operations
- Multi-sig validation
- Merkle proof verification
- Event monitoring
- Rate limiting

**Test Cases**:
```cpp
test_bridge_deposit()           // Deposit INT to bridge
test_bridge_withdrawal()        // Withdraw from bridge
test_multisig_validation()      // 3-of-5 validation
test_merkle_proofs()            // Proof verification
test_event_monitoring()         // Event indexing
test_rate_limiting()            // Deposit limits
test_circuit_breaker()          // Emergency pause
test_replay_protection()        // Nonce validation
```

**Run**:
```bash
./tests/test_bridge
```

---

### 17. IntegrationTest (1.12s) ✅

**File**: `tests/test_integration.cpp`

**Coverage**:
- End-to-end blockchain workflows
- Multi-component integration
- Blockchain + storage integration
- Wallet + blockchain integration
- Network + mempool integration
- Full transaction lifecycle

**Test Cases**:
```cpp
test_blockchain_storage()       // Blockchain persistence
test_wallet_integration()       // Wallet operations
test_transaction_flow()         // TX creation to confirmation
test_network_mempool()          // P2P transaction propagation
test_mining_consensus()         // Block mining and validation
test_end_to_end_payment()       // Complete payment flow
```

**Run**:
```bash
./tests/test_integration
```

---

## Writing Tests

### Test Template

```cpp
// tests/test_new_feature.cpp
#include "intcoin/intcoin.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace intcoin;

// Helper function to clean up test data
void cleanup_test_dir(const std::string& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
}

void test_new_feature() {
    std::cout << "Testing new feature...\n";

    // Setup
    auto test_data = create_test_data();

    // Execute
    auto result = new_feature(test_data);

    // Verify
    assert(result.IsOk());
    assert(result.value.value() == expected_value);

    // Cleanup
    cleanup_test_dir("./test_data");

    std::cout << "✓ New feature test passed\n";
}

int main() {
    std::cout << "=== New Feature Test Suite ===\n\n";

    try {
        test_new_feature();

        std::cout << "\n=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
```

### Add Test to CMake

```cmake
# tests/CMakeLists.txt

# Add new test executable
add_executable(test_new_feature test_new_feature.cpp)
target_link_libraries(test_new_feature intcoin_core ${ROCKSDB_LIB})

# Register with CTest
add_test(NAME NewFeatureTest COMMAND test_new_feature)

# Install (optional)
install(TARGETS test_new_feature DESTINATION bin/tests)
```

### Best Practices

✅ **DO**:
- Test one feature per test function
- Use descriptive test names
- Clean up test resources
- Test both success and failure cases
- Use assertions to verify results
- Print progress messages
- Handle exceptions appropriately

❌ **DON'T**:
- Depend on external services
- Use hardcoded file paths
- Leave test data on disk
- Test multiple unrelated features together
- Ignore test failures
- Skip error checking

---

## Test Coverage

### Current Coverage

| Component | Coverage | Tests | Status |
|-----------|----------|-------|--------|
| **Cryptography** | 100% | 5 | ✅ |
| **Consensus (PoW)** | 100% | 5 | ✅ |
| **Serialization** | 100% | 4 | ✅ |
| **Storage** | 100% | 10 | ✅ |
| **Validation** | 100% | 7 | ✅ |
| **Genesis Block** | 100% | 4 | ✅ |
| **Network** | 100% | 8 | ✅ |
| **Machine Learning** | 100% | 8 | ✅ |
| **Wallet** | 100% | 12 | ✅ |
| **Bech32** | 100% | 4 | ✅ |

**Total**: 10 test suites, 67 test cases, 100% pass rate

### Coverage Goals

- ✅ Core blockchain: 100%
- ✅ Cryptography: 100%
- ✅ Validation: 100%
- ✅ Storage: 100%
- ✅ Networking: 100%
- ✅ Wallet: 100%

---

## Continuous Integration

### GitHub Actions / GitLab CI

```yaml
# .gitlab-ci.yml
test:
  stage: test
  script:
    - mkdir build && cd build
    - cmake .. -DBUILD_TESTS=ON
    - cmake --build .
    - ctest -V
  artifacts:
    reports:
      junit: build/test_results.xml
```

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

echo "Running tests before commit..."

cd build
ctest

if [ $? -ne 0 ]; then
    echo "Tests failed! Commit aborted."
    exit 1
fi

echo "All tests passed!"
```

---

## Performance Benchmarks

### Test Runtime

| Test Suite | Runtime | Relative |
|------------|---------|----------|
| SerializationTest | 0.36s | Fastest |
| Bech32Test | 0.38s | Very Fast |
| GenesisTest | 0.40s | Very Fast |
| NetworkTest | 0.43s | Fast |
| MLTest | 0.50s | Fast |
| CryptoTest | 0.53s | Normal |
| ValidationTest | 0.79s | Normal |
| StorageTest | 0.81s | Normal |
| WalletTest | 0.86s | Normal |
| RandomXTest | 3.66s | Slow (PoW intensive) |
| **Total** | **8.73s** | - |

### Optimization Opportunities

- RandomXTest is intentionally slow (tests actual PoW)
- Other tests run in < 1 second
- Parallel execution recommended: `ctest -j$(nproc)`

---

## Debugging Tests

### Run with Debugger

```bash
# GDB
gdb --args ./tests/test_wallet

# LLDB
lldb ./tests/test_wallet
```

### Valgrind Memory Check

```bash
valgrind --leak-check=full ./tests/test_storage
```

### AddressSanitizer

```bash
# Build with ASAN
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build .

# Run tests
./tests/test_validation
```

---

## Future Test Plans

### Planned Test Additions

- [ ] **Stress Tests**: High load scenarios
- [ ] **Fuzz Tests**: Random input testing
- [ ] **Performance Tests**: Benchmarking
- [ ] **Integration Tests**: Multi-component workflows
- [ ] **Regression Tests**: Bug prevention
- [ ] **Security Tests**: Attack scenarios

### Test Framework Enhancements

- [ ] Test result reporting (JUnit XML)
- [ ] Code coverage analysis (lcov)
- [ ] Performance profiling integration
- [ ] Automated test generation

---

## Contributing Tests

When contributing to INTcoin:

1. **Write tests first** (TDD approach)
2. **Ensure all tests pass** before submitting
3. **Add new tests** for new features
4. **Document test cases** clearly
5. **Clean up test resources** properly

See [CONTRIBUTING.md](../CONTRIBUTING.md) for details.

---

## See Also

- [Building from Source](BUILDING.md)
- [Contributing Guidelines](../CONTRIBUTING.md)
- [Developer Hub](../wiki/Developers.md)
- [CI/CD Pipeline](../wiki/CI-CD.md)

---

**Test Status**: ✅ 10/10 tests passing (100%)
**Last Updated**: December 3, 2025
**Total Runtime**: 8.73 seconds
