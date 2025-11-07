# INTcoin Fuzz Testing Guide

## Overview

Fuzz testing (fuzzing) is an automated software testing technique that involves providing invalid, unexpected, or random data as inputs to find bugs, crashes, and security vulnerabilities. INTcoin implements comprehensive fuzz testing for all critical components.

## Fuzz Testing Strategy

### Targeted Components

1. **Transaction Deserialization** (`fuzz_transaction`)
   - Tests transaction parsing with malformed input
   - Validates serialization round-trips
   - Checks for buffer overflows and crashes

2. **Block Deserialization** (`fuzz_block`)
   - Tests block parsing with invalid data
   - Validates merkle tree calculations
   - Tests block header validation

3. **P2P Messages** (`fuzz_p2p_message`)
   - Tests network message parsing
   - Validates message headers
   - Tests inventory vectors

4. **Cryptographic Operations** (`fuzz_script`)
   - Tests signature verification with invalid data
   - Tests hash functions with edge cases
   - Validates quantum-resistant crypto

5. **RPC Interface** (`fuzz_rpc`)
   - Tests JSON-RPC parsing
   - Validates parameter handling
   - Tests error conditions

## Fuzzing Engines

### libFuzzer (Recommended)

**Advantages:**
- In-process fuzzing (faster)
- Coverage-guided
- Built into LLVM/Clang
- Continuous mode

**Installation:**
```bash
# Usually included with Clang
clang++ --version  # Check if available
```

### AFL (American Fuzzy Lop)

**Advantages:**
- Battle-tested
- Excellent at finding edge cases
- Good documentation
- Parallel fuzzing support

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install afl++

# macOS
brew install afl++

# FreeBSD
sudo pkg install afl
```

## Building for Fuzzing

### Using libFuzzer

```bash
# Configure with fuzzing enabled
mkdir build-fuzz && cd build-fuzz

cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DFUZZ_ENGINE=libfuzzer \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Debug

make
```

### Using AFL

```bash
# Configure with AFL
mkdir build-afl && cd build-afl

cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DFUZZ_ENGINE=afl \
    -DCMAKE_CXX_COMPILER=afl-clang++ \
    -DCMAKE_BUILD_TYPE=Debug

make
```

## Running Fuzz Tests

### Quick Start

```bash
# Run all fuzzers for 1 hour each
./tests/fuzz/run-fuzzing.sh

# Run for 5 minutes each
./tests/fuzz/run-fuzzing.sh --time 300

# Run in parallel (4 jobs)
./tests/fuzz/run-fuzzing.sh --jobs 4
```

### Individual Fuzzers

#### libFuzzer

```bash
cd build-fuzz

# Run transaction fuzzer for 1 hour
./fuzz_transaction \
    -max_total_time=3600 \
    -print_final_stats=1 \
    corpus_transaction/

# With specific options
./fuzz_transaction \
    -max_total_time=3600 \
    -max_len=1048576 \
    -timeout=10 \
    -rss_limit_mb=2048 \
    corpus_transaction/
```

#### AFL

```bash
cd build-afl

# Create input corpus
mkdir -p afl_in/transaction
echo "test" > afl_in/transaction/seed

# Run fuzzer
afl-fuzz \
    -i afl_in/transaction \
    -o afl_out/transaction \
    -- ./fuzz_transaction @@
```

## Corpus Management

### Creating Seed Corpus

```bash
# Create directory structure
mkdir -p corpus_transaction
mkdir -p corpus_block
mkdir -p corpus_p2p_message

# Add valid transaction example
echo -n "01000000..." > corpus_transaction/valid_tx.bin

# Add edge cases
echo "" > corpus_transaction/empty.bin
dd if=/dev/urandom of=corpus_transaction/random.bin bs=1024 count=1
```

### Minimizing Corpus

```bash
# Merge and minimize corpus
./fuzz_transaction \
    -merge=1 \
    corpus_transaction_min/ \
    corpus_transaction/
```

## Crash Analysis

### Reproducing Crashes

```bash
# Run fuzzer on specific crash input
./fuzz_transaction crash-abc123

# With debugger
gdb --args ./fuzz_transaction crash-abc123
```

### Crash Triage

```bash
# Check crash location
addr2line -e ./fuzz_transaction 0x12345678

# Get stack trace
./fuzz_transaction crash-abc123 2>&1 | c++filt
```

## Continuous Fuzzing

### Local Continuous Fuzzing

```bash
#!/bin/bash
# run-continuous-fuzz.sh

while true; do
    ./tests/fuzz/run-fuzzing.sh --time 3600

    # Check for crashes
    if [ -d fuzz_crashes ]; then
        echo "Crashes found, stopping"
        break
    fi

    sleep 60
done
```

### Integration with CI

```yaml
# .github/workflows/fuzz.yml
name: Fuzz Testing

on:
  schedule:
    - cron: '0 0 * * *'  # Daily

jobs:
  fuzz:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y clang llvm

    - name: Build fuzzers
      run: |
        mkdir build && cd build
        cmake .. -DENABLE_FUZZ_TESTING=ON
        make

    - name: Run fuzz tests
      run: |
        cd build
        timeout 3600 ./tests/fuzz/run-fuzzing.sh --time 300

    - name: Upload crashes
      if: failure()
      uses: actions/upload-artifact@v2
      with:
        name: fuzz-crashes
        path: fuzz_crashes/
```

## Advanced Fuzzing

### Sanitizers

#### AddressSanitizer (ASan)

Detects:
- Buffer overflows
- Use-after-free
- Memory leaks

```bash
cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

#### UndefinedBehaviorSanitizer (UBSan)

Detects:
- Integer overflows
- Null pointer dereferences
- Division by zero

```bash
cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
```

#### MemorySanitizer (MSan)

Detects:
- Uninitialized memory reads

```bash
cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DCMAKE_CXX_FLAGS="-fsanitize=memory"
```

### Coverage-Guided Fuzzing

```bash
# Build with coverage
cmake .. \
    -DENABLE_FUZZ_TESTING=ON \
    -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

# Run fuzzer
./fuzz_transaction corpus/ -runs=1000000

# Generate coverage report
llvm-profdata merge default.profraw -o default.profdata
llvm-cov show ./fuzz_transaction -instr-profile=default.profdata
```

### Dictionary-Based Fuzzing

```
# transaction.dict
keyword_version="version"
keyword_locktime="locktime"
magic_bytes="\xD9\xB4\xBE\xF9"
```

```bash
./fuzz_transaction \
    -dict=transaction.dict \
    corpus_transaction/
```

## Best Practices

1. **Run Regularly**: Integrate into CI/CD
2. **Diverse Corpus**: Include valid and invalid inputs
3. **Long Runs**: Run overnight or longer for best results
4. **Multiple Engines**: Use both libFuzzer and AFL
5. **Monitor Coverage**: Track code coverage improvements
6. **Triage Quickly**: Fix crashes as soon as found
7. **Regression Tests**: Add crashes to test suite

## Performance Tips

### Parallel Fuzzing

```bash
# Run 8 fuzzers in parallel
for i in {1..8}; do
    ./fuzz_transaction \
        -jobs=$i \
        -workers=$i \
        corpus_transaction/ &
done
wait
```

### Distributed Fuzzing

```bash
# On multiple machines, share corpus
# Machine 1
./fuzz_transaction corpus_shared/

# Machine 2
./fuzz_transaction corpus_shared/
```

## Metrics and Reporting

### Coverage Metrics

```bash
# Show coverage percentage
llvm-cov report ./fuzz_transaction -instr-profile=default.profdata

# HTML report
llvm-cov show ./fuzz_transaction \
    -instr-profile=default.profdata \
    -format=html \
    -output-dir=coverage_html
```

### Fuzzing Statistics

```bash
# libFuzzer stats
./fuzz_transaction \
    -print_final_stats=1 \
    corpus/ 2>&1 | grep "stat::"
```

## Troubleshooting

### OOM (Out of Memory)

```bash
# Limit memory usage
./fuzz_transaction \
    -rss_limit_mb=2048 \
    corpus/
```

### Slow Fuzzing

```bash
# Reduce input size
./fuzz_transaction \
    -max_len=1024 \
    corpus/

# Increase timeout
./fuzz_transaction \
    -timeout=30 \
    corpus/
```

### No New Coverage

```bash
# Try different strategies
./fuzz_transaction \
    -use_value_profile=1 \
    -use_cmp=1 \
    corpus/
```

## Resources

- [libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [AFL Documentation](https://github.com/google/AFL)
- [OSS-Fuzz](https://github.com/google/oss-fuzz)
- [Fuzzing Book](https://www.fuzzingbook.org/)

## Contributing Fuzz Targets

When adding new fuzz targets:

1. Create `fuzz_<component>.cpp`
2. Implement `LLVMFuzzerTestOneInput`
3. Add to `CMakeLists.txt`
4. Create seed corpus
5. Document in this file
6. Run for at least 1 hour
7. Add any crashes to regression tests

## License

Copyright (c) 2025 INTcoin Core (Maddison Lane)
Distributed under the MIT software license
