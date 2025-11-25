# INTcoin Testing Guide

## Overview

INTcoin includes a comprehensive test suite covering all major components of the system. This guide explains how to run tests, write new tests, and interpret results.

## Test Organization

### Test Suites

1. **Cryptography Tests** (`test_crypto.cpp`)
   - SHA3-256 hashing
   - CRYSTALS-Dilithium signatures
   - CRYSTALS-Kyber key exchange
   - Quantum-resistant cryptography

2. **Blockchain Tests** (`test_blockchain.cpp`)
   - Block creation and validation
   - Transaction processing
   - Merkle tree construction
   - UTXO management
   - Difficulty adjustment
   - Block subsidy halving

3. **Wallet Tests** (`test_wallet.cpp`)
   - Key generation and management
   - HD wallet derivation
   - Transaction creation and signing
   - Balance calculation
   - Coin selection
   - Encryption and security

4. **Network Tests** (`test_network.cpp`)
   - P2P messaging
   - Peer discovery and management
   - Message serialization
   - Protocol compliance

5. **Lightning Network Tests** (`test_lightning.cpp`)
   - Channel opening and closing
   - HTLC creation and settlement
   - Payment routing
   - Invoice generation
   - Commitment transactions

6. **TOR Tests** (`test_tor.cpp`)
   - SOCKS5 proxy connections
   - Hidden service hosting
   - Onion address validation
   - Network anonymization

7. **Security Tests** (`test_security.cpp`)
   - Memory safety
   - Buffer overflow protection
   - Integer overflow detection
   - Safe arithmetic operations

8. **Security Audit Tests** (`test_security_performance.cpp`)
   - Smart contract security audit framework (10 checks)
   - Performance benchmarking (8 suites, 28+ tests)
   - Integrated security + performance validation
   - Reentrancy detection, overflow/underflow, delegatecall risks
   - Timing attacks, memory safety, crypto misuse, DOS prevention

9. **Fuzz Testing** (14 Targets - `tests/fuzz/`)
   - **Core targets**: transaction, block, P2P messages, script, RPC
   - **Extended targets**: wallet, contract, lightning, bridge, PQC
   - **Advanced targets**: mempool, consensus engine
   - **Integration targets**: exchange API validation

10. **Exchange Integration Tests** (`test_exchange_integration.cpp`)
    - 12 comprehensive API tests
    - Hot/cold wallet segregation
    - Multi-signature withdrawal validation
    - Deposit address generation and verification
    - Transaction signing with quantum signatures
    - Rate limiting and API compliance
    - KYC/AML compliance checking

## Running Tests

### Quick Start

```bash
# Build and run all tests
./run-tests.sh

# Verbose output
./run-tests.sh --verbose

# Include integration tests
./run-tests.sh --integration

# Generate coverage report
./run-tests.sh --coverage
```

### Using CMake/CTest

```bash
# Build tests
cd build
cmake .. -DBUILD_TESTS=ON
make

# Run all tests
ctest

# Run specific test suite
ctest -R crypto_tests

# Verbose output
ctest -V

# Run tests in parallel
ctest -j8
```

### Individual Test Executables

```bash
# Run individual test suites
cd build

./test_crypto
./test_blockchain
./test_wallet
./test_network
./test_lightning
./test_tor
./test_security
./test_security_performance          # NEW: Combined security audit + benchmarks
./test_exchange_integration          # NEW: Exchange API testing

# Run specific fuzz targets
cd build
./fuzz_transaction                   # Original fuzz targets (5)
./fuzz_block
./fuzz_p2p_messages
./fuzz_script
./fuzz_rpc_json

./fuzz_wallet                        # Extended fuzz targets (5)
./fuzz_contract
./fuzz_lightning
./fuzz_bridge
./fuzz_pqc

./fuzz_mempool                       # Advanced fuzz targets (2)
./fuzz_consensus

# Run with corpus (if available)
./fuzz_transaction tests/fuzz/corpus/transaction/
```

## Benchmark Testing

### Performance Benchmarking (8 Suites)

```bash
# Run performance benchmarks
./test_security_performance --benchmark

# Benchmark results output:
# - Cryptographic ops throughput (signs/sec, verifies/sec)
# - Transaction processing (parse/sec, validate/sec)
# - Mining performance (PoW checks/sec, difficulty calcs/sec)
# - Smart contract execution (gas efficiency, opcode overhead)
# - Lightning routing (payment routing speed, HTLC resolution)
# - Bridge operations (atomic swap latency, proof verification)
# - Network performance (P2P throughput, connection establishment)
# - Memory usage (blockchain sync, UTXO set, cache efficiency)
```

### Benchmark Output Format

```
=== Cryptographic Operations Benchmark ===
Dilithium5 Signature Generation: 500 ops/sec
Dilithium5 Signature Verification: 1000 ops/sec
Kyber1024 Encapsulation: 800 ops/sec
Kyber1024 Decapsulation: 800 ops/sec
SHA3-256 Hashing: 50000 ops/sec
Average throughput: 10,620 ops/sec

=== Transaction Processing Benchmark ===
Transaction Parsing: 100,000 ops/sec
Transaction Validation: 50,000 ops/sec
Fee Calculation: 200,000 ops/sec
Total throughput: 116,667 ops/sec
```

## Security Audit Testing

### Contract Security Analysis (10 Checks)

```bash
# Run automated security audit
./test_security_performance --audit-contracts

# Checks performed:
# 1. Reentrancy detection
# 2. Integer overflow/underflow
# 3. Delegatecall vulnerabilities
# 4. Access control validation
# 5. Timestamp dependency issues
# 6. Memory safety violations
# 7. Cryptographic misuse
# 8. Denial-of-service vectors
# 9. Quantum-safe opcodes verification
# 10. State consistency validation

# Output includes severity levels and remediation steps
```

## Exchange Integration Testing

### API Validation Tests (12 Tests)

```bash
# Run exchange integration tests
./test_exchange_integration

# Tests include:
# 1. Hot/Cold Wallet Segregation
# 2. Multi-Signature Withdrawal (2-of-3)
# 3. Deposit Address Generation
# 4. Transaction Signing (Dilithium5)
# 5. Rate Limiting Enforcement
# 6. Audit Logging Verification
# 7. Quantum-Safe Signature Generation
# 8. Batch Operations
# 9. Compliance Checks (KYC/AML)
# 10. Security Validators (3 functions)
# 11. Performance Monitoring
# 12. Reconnection Handling

# Performance targets:
# - Deposits: 1,000+ ops/sec
# - Withdrawals: 500+ ops/sec
# - Latency: <100ms average
```

## Test Types

### Unit Tests

Test individual components in isolation:
- Fast execution
- No external dependencies
- Deterministic results

### Integration Tests

Test component interactions:
- Require external services (TOR, etc.)
- May be slower
- Test real-world scenarios

Run integration tests:
```bash
./run-tests.sh --integration
```

## Writing Tests

### Test Structure

```cpp
#include "intcoin/component.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

// Test counters
int tests_passed = 0;
int tests_failed = 0;

// Test helper
void test_assert(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "[PASS] " << test_name << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] " << test_name << std::endl;
        tests_failed++;
    }
}

// Test function
void test_feature() {
    std::cout << "\n=== Testing Feature ===" << std::endl;

    // Test code here
    test_assert(true, "Feature works correctly");
}

// Main runner
int main() {
    std::cout << "Test Suite Name" << std::endl;
    std::cout << "===============" << std::endl;

    test_feature();

    // Summary
    std::cout << "\n===============" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "===============" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
```

### Best Practices

1. **Descriptive Names**: Use clear, descriptive test names
2. **One Assertion**: Each test should verify one thing
3. **Independence**: Tests should not depend on each other
4. **Cleanup**: Clean up resources after tests
5. **Edge Cases**: Test boundary conditions
6. **Error Cases**: Test error handling

### Adding New Tests

1. Create test file in `tests/` directory
2. Add to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(test_myfeature test_myfeature.cpp)
   target_link_libraries(test_myfeature PRIVATE intcoin_core)
   add_test(NAME myfeature_tests COMMAND test_myfeature)
   ```
3. Update `run-tests.sh` to include new test

## Coverage Reports

### Generating Coverage

```bash
# Configure with coverage
cd build
cmake .. -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug

# Run tests
make coverage

# View report
open coverage/index.html  # macOS
xdg-open coverage/index.html  # Linux
```

### Coverage Tools

- **gcov**: GNU coverage tool
- **lcov**: Front-end for gcov
- **llvm-cov**: LLVM coverage tool (for Clang)

Install coverage tools:
```bash
# Ubuntu/Debian
sudo apt-get install lcov

# macOS
brew install lcov

# FreeBSD
sudo pkg install lcov
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake build-essential libssl-dev

    - name: Build
      run: |
        mkdir build && cd build
        cmake .. -DBUILD_TESTS=ON
        make -j$(nproc)

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure

    - name: Generate coverage
      if: github.event_name == 'push'
      run: |
        ./run-tests.sh --coverage
```

## Benchmarking

### Performance Tests

```bash
# Run with timing
time ./test_crypto

# Benchmark specific operations
./test_crypto --benchmark
```

### Memory Profiling

```bash
# Valgrind memory check
valgrind --leak-check=full ./test_crypto

# Valgrind memory profiling
valgrind --tool=massif ./test_crypto
```

## Debugging Failed Tests

### Verbose Output

```bash
# Run with verbose output
./test_blockchain 2>&1 | tee test_output.log

# Use debugger
gdb ./test_blockchain
```

### Common Issues

1. **Timing Issues**: Use timeouts for time-sensitive tests
2. **Resource Cleanup**: Ensure proper cleanup in destructors
3. **Platform Differences**: Handle platform-specific behavior
4. **Race Conditions**: Use proper synchronization

### Debug Builds

```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# Run under debugger
gdb ./test_blockchain
```

## Test Environment

### In-Memory Databases

Tests use in-memory SQLite databases:
```cpp
Wallet wallet(":memory:");  // No file created
Blockchain blockchain(":memory:");
```

### Temporary Files

Clean up temporary files:
```cpp
#include <filesystem>

// After test
std::filesystem::remove("/tmp/test_wallet.db");
```

### Mocking

For testing without external dependencies:
```cpp
// Mock blockchain for wallet tests
class MockBlockchain : public Blockchain {
public:
    // Override methods for testing
};
```

## Integration Test Requirements

### TOR Integration Tests

Require TOR daemon running:
```bash
# Install TOR
sudo apt-get install tor

# Start TOR
sudo systemctl start tor

# Run TOR integration tests
./test_tor --integration
```

### Network Integration Tests

May require:
- Open network ports
- Multiple node instances
- Timing tolerance

## Continuous Monitoring

### Test Metrics

Track over time:
- Test count
- Coverage percentage
- Execution time
- Failure rate

### Automated Testing

Run tests:
- On every commit
- Before merges
- Nightly builds
- Release candidates

## Troubleshooting

### Build Failures

```bash
# Clean rebuild
rm -rf build
mkdir build && cd build
cmake ..
make clean
make
```

### Test Failures

1. Check test output for errors
2. Run individual failed test with verbose output
3. Check logs in `/tmp/test_*.log`
4. Verify dependencies are installed

### Platform-Specific Issues

- **macOS**: May need to install additional libraries via Homebrew
- **Linux**: Check library versions
- **FreeBSD**: Use `gmake` instead of `make`
- **Windows**: Ensure Visual Studio is properly configured

## Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [Catch2 Framework](https://github.com/catchorg/Catch2)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [Code Coverage Best Practices](https://testing.googleblog.com/2020/08/code-coverage-best-practices.html)

## Contributing Tests

When contributing:

1. Add tests for new features
2. Ensure all tests pass
3. Maintain or improve coverage
4. Follow existing test patterns
5. Document complex test scenarios

## License

Tests are released under the MIT License.
Copyright (c) 2025 INTcoin Core (Maddison Lane)
