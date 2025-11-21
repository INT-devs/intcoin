# INTcoin Functional Tests

This directory contains Python-based functional tests for INTcoin Core.

## Test Framework

The `test_framework.py` module provides base classes and utilities for writing functional tests:

- **TestFramework**: Base class for all functional tests
- **TestNode**: Represents a running INTcoin node with RPC interface
- **RPCError**: Exception for RPC errors

## Available Tests

### test_basic.py
Tests basic blockchain operations:
- Initial block count
- Block generation
- Balance after mining
- Address generation
- Simple transactions

### test_wallet.py
Tests wallet functionality:
- Multi-node wallet operations
- Sending and receiving coins
- Balance verification
- Transaction confirmation
- Invalid transaction handling

### test_blockchain.py
Tests blockchain consensus:
- Multi-node synchronization
- Block hash consistency
- Chain reorganization
- Blockchain info verification

### test_mining.py
Tests mining operations:
- Single and batch block generation
- Mining rewards
- Coinbase maturity
- Block timestamps
- Mining difficulty

### test_rpc.py
Tests RPC interface:
- Core RPC commands (getblockchaininfo, getnetworkinfo, etc.)
- Block and transaction RPCs
- Wallet RPCs
- Error handling
- Help system

## Running Tests

### Prerequisites

1. Build INTcoin daemon:
```bash
./build.sh
```

2. Ensure Python 3.7+ is installed:
```bash
python3 --version
```

### Run Individual Test

```bash
cd tests/functional
python3 test_basic.py
```

### Run All Tests

```bash
cd tests/functional
./run_tests.sh
```

Or manually:
```bash
for test in test_*.py; do
    echo "Running $test..."
    python3 "$test"
    if [ $? -ne 0 ]; then
        echo "FAILED: $test"
        exit 1
    fi
done
echo "All tests passed!"
```

## Writing New Tests

To create a new functional test:

1. Create a new file `test_yourfeature.py`
2. Import the test framework:
```python
#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework import TestFramework

class YourFeatureTest(TestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 1  # Number of nodes to start

    def run_test(self):
        """Test logic goes here."""
        node = self.nodes[0]

        # Your test code
        self.log_info("Running test")
        node.generate(10)
        height = node.getblockcount()
        self.assert_equal(height, 10, "Should have 10 blocks")
        self.log_success("Test passed")

if __name__ == '__main__':
    test = YourFeatureTest()
    exit(test.main())
```

3. Make executable:
```bash
chmod +x test_yourfeature.py
```

## Test Framework API

### TestFramework Methods

- `setup_nodes()`: Automatically called to start nodes
- `run_test()`: Override with your test logic
- `log_info(msg)`: Log informational message
- `log_success(msg)`: Log success message
- `log_error(msg)`: Log error message
- `assert_equal(actual, expected, msg)`: Assert equality
- `assert_not_equal(actual, expected, msg)`: Assert inequality
- `assert_greater_than(actual, expected, msg)`: Assert >
- `assert_greater_or_equal(actual, expected, msg)`: Assert >=
- `assert_raises(exception_type, callable, *args)`: Assert exception
- `assert_raises_rpc_error(code, pattern, callable, *args)`: Assert RPC error
- `sync_blocks(timeout)`: Wait for nodes to sync
- `wait_for_balance(node, expected, timeout)`: Wait for balance

### TestNode Methods

- `start(extra_args)`: Start the node
- `stop()`: Stop the node
- `is_running()`: Check if running
- `rpc(method, params)`: Make RPC call
- `generate(nblocks)`: Generate blocks
- `getblockcount()`: Get block height
- `getbalance()`: Get wallet balance
- `sendtoaddress(address, amount)`: Send transaction
- `getnewaddress()`: Generate new address

## Debugging Tests

Enable verbose output:
```python
def run_test(self):
    self.log_info("Starting test")
    # Add detailed logging
    result = self.nodes[0].rpc("somecommand")
    self.log_info(f"Result: {result}")
```

View node logs:
```bash
# Temporary datadir created in /tmp/intcoin_test_XXXXXX/
# Check stderr/stdout for node output
```

## CI/CD Integration

Tests can be integrated into continuous integration:

```yaml
# .gitlab-ci.yml or .github/workflows/test.yml
test:
  script:
    - ./build.sh
    - cd tests/functional
    - ./run_tests.sh
```

## Test Coverage

Current test coverage:
- ✅ Basic blockchain operations
- ✅ Wallet functionality
- ✅ Multi-node consensus
- ✅ Mining and block generation
- ✅ RPC interface
- 🔲 Lightning Network
- 🔲 Smart contracts
- 🔲 Atomic swaps
- 🔲 Tor integration

## Troubleshooting

### Binary not found
```
FileNotFoundError: intcoind binary not found
```
Solution: Build the project first with `./build.sh`

### Port already in use
```
Address already in use
```
Solution: Stop any running INTcoin nodes or change RPC ports in test

### Test timeout
```
TimeoutError: Nodes failed to sync
```
Solution: Increase timeout in `sync_blocks()` or check node logs

## Contributing

When adding new features to INTcoin, please:
1. Write functional tests covering the new functionality
2. Ensure all existing tests pass
3. Update this README with test descriptions
4. Follow the existing code style

## Resources

- [Bitcoin functional tests](https://github.com/bitcoin/bitcoin/tree/master/test/functional)
- [Python unittest documentation](https://docs.python.org/3/library/unittest.html)
- [INTcoin RPC documentation](../../wiki/RPC-API.md)
