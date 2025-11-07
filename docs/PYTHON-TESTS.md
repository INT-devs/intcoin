# Python Functional Tests - Implementation Guide

## Overview

This document outlines the Python functional test suite for INTcoin. The functional tests complement the C++ unit tests by providing end-to-end system testing.

## Test Framework Architecture

### Directory Structure
```
tests/functional/
├── test_framework.py         # Base test framework
├── test_blockchain.py         # Blockchain functional tests
├── test_wallet.py             # Wallet functional tests
├── test_rpc.py                # RPC interface tests
├── test_p2p.py                # P2P network tests
├── test_mining.py             # Mining tests
├── test_mempool.py            # Transaction pool tests
├── test_reorg.py              # Chain reorganization tests
├── test_lightning.py          # Lightning Network tests
├── utils/                     # Test utilities
│   ├── rpc_client.py         # RPC client library
│   ├── test_node.py          # Test node wrapper
│   └── helpers.py            # Helper functions
└── run_tests.py              # Test runner
```

## Test Categories

### 1. Blockchain Tests (`test_blockchain.py`)
- Block generation and validation
- Chain synchronization
- Block reorganization
- Difficulty adjustment
- Subsidy halving

### 2. Wallet Tests (`test_wallet.py`)
- Address generation
- Transaction creation
- Balance tracking
- HD wallet functionality
- Encryption/decryption

### 3. RPC Tests (`test_rpc.py`)
- All RPC commands
- Error handling
- Authentication
- Response formats

### 4. P2P Tests (`test_p2p.py`)
- Peer connection
- Block propagation
- Transaction relay
- Peer discovery

### 5. Mining Tests (`test_mining.py`)
- Block generation
- Difficulty calculation
- Coinbase transactions
- Mining rewards

## Example Test Implementation

### Basic Test Structure

```python
#!/usr/bin/env python3
from test_framework import TestFramework

class BlockchainTest(TestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]

        # Test block generation
        self.log_info("Testing block generation")
        blockhash = node.generate(1)[0]
        assert node.getblockcount() == 1

        # Test block retrieval
        block = node.getblock(blockhash)
        assert block['height'] == 1

        self.log_info("All tests passed")

if __name__ == '__main__':
    BlockchainTest().main()
```

### Multi-Node Test

```python
class P2PTest(TestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        # Connect nodes
        self.connect_nodes(0, 1)

        # Generate on node 0
        self.nodes[0].generate(10)

        # Sync and verify
        self.sync_all()
        assert_equal(
            self.nodes[0].getblockcount(),
            self.nodes[1].getblockcount()
        )
```

## RPC Client Library

```python
import http.client
import json
import base64

class RPCClient:
    def __init__(self, host, port, username, password):
        self.host = host
        self.port = port
        self.auth = base64.b64encode(
            f"{username}:{password}".encode()
        ).decode()

    def call(self, method, params=None):
        conn = http.client.HTTPConnection(self.host, self.port)
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Basic {self.auth}"
        }

        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": method,
            "params": params or []
        }

        conn.request("POST", "/", json.dumps(payload), headers)
        response = conn.getresponse()
        result = json.loads(response.read())

        if "error" in result and result["error"]:
            raise Exception(result["error"])

        return result.get("result")
```

## Running Tests

### Single Test
```bash
python3 tests/functional/test_blockchain.py
```

### All Tests
```bash
python3 tests/functional/run_tests.py
```

### With Options
```bash
# Verbose output
python3 tests/functional/run_tests.py --verbose

# Specific test
python3 tests/functional/run_tests.py test_wallet

# Keep data
python3 tests/functional/run_tests.py --nocleanup
```

## Test Utilities

### Helper Functions

```python
def assert_equal(thing1, thing2, message=""):
    if thing1 != thing2:
        raise AssertionError(
            f"{message} Expected {thing2}, got {thing1}"
        )

def wait_until(predicate, timeout=60):
    start = time.time()
    while time.time() - start < timeout:
        if predicate():
            return True
        time.sleep(0.5)
    raise TimeoutError()

def sync_blocks(nodes, timeout=60):
    def synced():
        heights = [n.getblockcount() for n in nodes]
        return len(set(heights)) == 1
    wait_until(synced, timeout)
```

### Test Node Wrapper

```python
class TestNode:
    def __init__(self, index, datadir):
        self.index = index
        self.datadir = datadir
        self.process = None
        self.rpc = None

    def start(self):
        # Start intcoind process
        cmd = [
            "./intcoind",
            f"-datadir={self.datadir}",
            "-regtest"
        ]
        self.process = subprocess.Popen(cmd)
        self.rpc = RPCClient(...)

    def stop(self):
        self.rpc.stop()
        self.process.wait()
```

## Test Scenarios

### 1. Basic Blockchain Operation
```python
def test_basic_blockchain(self):
    # Start node
    node = self.nodes[0]

    # Mine blocks
    blocks = node.generate(100)
    assert len(blocks) == 100

    # Verify chain
    assert node.getblockcount() == 100

    # Get best block
    best_hash = node.getbestblockhash()
    block = node.getblock(best_hash)
    assert block['height'] == 100
```

### 2. Transaction Test
```python
def test_transaction(self):
    node = self.nodes[0]

    # Generate coins
    node.generate(101)

    # Create transaction
    addr = node.getnewaddress()
    txid = node.sendtoaddress(addr, 10.0)

    # Mine transaction
    node.generate(1)

    # Verify
    tx = node.gettransaction(txid)
    assert tx['confirmations'] >= 1
```

### 3. Multi-Node Sync
```python
def test_multinode_sync(self):
    # Connect nodes
    self.connect_nodes(0, 1)

    # Mine on node 0
    self.nodes[0].generate(50)

    # Wait for sync
    self.sync_all()

    # Verify both nodes have same chain
    assert_equal(
        self.nodes[0].getbestblockhash(),
        self.nodes[1].getbestblockhash()
    )
```

## Integration with CMake

```cmake
# tests/CMakeLists.txt
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME functional_tests
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/functional/run_tests.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
```

## Dependencies

### Required Python Packages
```
requirements.txt:
pytest>=7.0.0
requests>=2.28.0
```

### Installation
```bash
pip3 install -r requirements.txt
```

## Best Practices

1. **Cleanup**: Always cleanup test data
2. **Isolation**: Tests should be independent
3. **Deterministic**: Use regtest mode
4. **Fast**: Keep tests quick
5. **Clear**: Use descriptive test names
6. **Error Messages**: Provide helpful assertions

## CI Integration

### GitHub Actions
```yaml
- name: Run functional tests
  run: |
    cd build
    python3 ../tests/functional/run_tests.py --ci
```

### Test Matrix
- Test on multiple Python versions (3.8, 3.9, 3.10, 3.11)
- Test on multiple platforms (Linux, macOS, FreeBSD)
- Test with different configurations

## Debugging

### Verbose Mode
```bash
python3 test_blockchain.py --verbose
```

### Keep Test Data
```bash
python3 test_blockchain.py --nocleanup
# Data in: test_datadir_*
```

### Debug Output
```bash
DEBUG=1 python3 test_blockchain.py
```

## Future Enhancements

- [ ] Stress tests
- [ ] Performance benchmarks
- [ ] Fuzz testing
- [ ] Network partition tests
- [ ] Long-running tests
- [ ] Memory leak detection
- [ ] Coverage reports

## Status

**Current Implementation**: Design phase
**Priority**: High
**Estimated Effort**: 2-3 weeks
**Dependencies**: Complete C++ RPC implementation

## References

- Bitcoin's functional test suite
- Python testing best practices
- INTcoin RPC documentation

## License

Copyright (c) 2025 INTcoin Core (Maddison Lane)
Distributed under the MIT software license
