# INTcoin Integration Testing Guide

**Version**: 1.0.0-alpha
**Last Updated**: December 25, 2025
**Purpose**: Comprehensive integration testing for v1.0.0-alpha release

This document provides step-by-step integration testing procedures for all critical INTcoin components.

---

## Table of Contents

1. [Test Environment Setup](#test-environment-setup)
2. [Test Scenario 1: Full Mining Flow](#test-scenario-1-full-mining-flow)
3. [Test Scenario 2: Pool Mining Flow](#test-scenario-2-pool-mining-flow)
4. [Test Scenario 3: Wallet Flow](#test-scenario-3-wallet-flow)
5. [Test Scenario 4: Blockchain Sync Flow](#test-scenario-4-blockchain-sync-flow)
6. [Test Results Documentation](#test-results-documentation)

---

## Test Environment Setup

### Prerequisites

1. **Build All Components**:
```bash
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

2. **Verify Binaries**:
```bash
ls -lh bin/
# Expected:
# intcoind
# intcoin-cli
# intcoin-qt
# intcoin-miner
# intcoin-pool-server
```

3. **Create Test Directories**:
```bash
mkdir -p ~/intcoin-test/{node1,node2,pool,wallets}
```

4. **Configure Testnet**:

Create `~/intcoin-test/node1/intcoin.conf`:
```conf
# Testnet Node 1 Configuration
testnet=1
server=1
rpcuser=test_user_1
rpcpassword=test_pass_1
rpcport=12211
port=12210
datadir=~/intcoin-test/node1
listen=1
discover=1

# RPC access
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Mining
gen=0
```

Create `~/intcoin-test/node2/intcoin.conf`:
```conf
# Testnet Node 2 Configuration
testnet=1
server=1
rpcuser=test_user_2
rpcpassword=test_pass_2
rpcport=12212
port=12213
datadir=~/intcoin-test/node2
listen=1
discover=1

# Connect to node1
connect=127.0.0.1:12210

# RPC access
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Mining
gen=0
```

---

## Test Scenario 1: Full Mining Flow

**Objective**: Verify solo mining works end-to-end

### Step 1: Start Node

```bash
cd build
./bin/intcoind -conf=~/intcoin-test/node1/intcoin.conf -daemon

# Wait for startup
sleep 5

# Verify node is running
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockchaininfo
```

**Expected Output**:
```json
{
  "chain": "test",
  "blocks": 0,
  "headers": 0,
  "bestblockhash": "genesis_hash...",
  "difficulty": 1.0,
  "chainwork": "0000...0001"
}
```

### Step 2: Generate Mining Address

```bash
# Get new address for mining rewards
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getnewaddress

# Save the address
MINING_ADDRESS="intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh..."
```

**Expected**: Valid testnet address starting with `intc1`

### Step 3: Get Block Template

```bash
# Request block template from node
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf \
    getblocktemplate '{"rules":["segwit"]}'
```

**Expected Output**:
```json
{
  "version": 1,
  "previousblockhash": "genesis_hash...",
  "transactions": [],
  "coinbasevalue": 105113636,
  "target": "00000000ffff0000000000000000000000000000000000000000000000000000",
  "mintime": 1703462400,
  "height": 1,
  "bits": "1d00ffff"
}
```

**Verification**:
- ✅ Template contains coinbase value (105,113,636 INT)
- ✅ Height is 1 (first block after genesis)
- ✅ Previous block hash is genesis hash
- ✅ Target difficulty is set

### Step 4: Start Solo Miner

```bash
# Start miner with low thread count for testing
./bin/intcoin-miner \
    --solo \
    --address=$MINING_ADDRESS \
    --rpc-user=test_user_1 \
    --rpc-password=test_pass_1 \
    --rpc-port=12211 \
    --threads=2 \
    --testnet
```

**Expected Output**:
```
[INFO] INTcoin Miner v1.0.0-alpha
[INFO] Solo mining to address: intc1qxy2...
[INFO] Using 2 threads
[INFO] Connected to intcoind at 127.0.0.1:12211
[INFO] Mining...
[INFO] Hashrate: 1.23 KH/s
```

### Step 5: Monitor Mining

```bash
# In another terminal, watch for blocks
watch -n 5 './bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockcount'
```

**Expected**: Block count incrementing as blocks are found

### Step 6: Verify Block Found

```bash
# Wait for at least 1 block to be mined
# Then check block details
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblock \
    $(./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockhash 1)
```

**Expected Output**:
```json
{
  "hash": "00000abc123...",
  "confirmations": 1,
  "height": 1,
  "version": 1,
  "merkleroot": "def456...",
  "tx": ["coinbase_tx_hash..."],
  "time": 1703462410,
  "nonce": 123456,
  "difficulty": 1.0,
  "chainwork": "0000...0002"
}
```

**Verification**:
- ✅ Block hash meets difficulty target
- ✅ Height is correct
- ✅ Contains coinbase transaction
- ✅ Merkle root is calculated correctly

### Step 7: Verify Coinbase Transaction

```bash
# Get coinbase transaction
COINBASE_TX=$(./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf \
    getblock $(./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockhash 1) | \
    jq -r '.tx[0]')

./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf \
    getrawtransaction $COINBASE_TX true
```

**Verification**:
- ✅ Coinbase input has previous tx hash = all zeros
- ✅ Coinbase output pays to mining address
- ✅ Coinbase value = 105,113,636 INT (block reward)
- ✅ Transaction is properly signed

### Step 8: Stop Miner

```bash
# Stop miner (Ctrl+C)
# Verify final statistics
```

**Expected Output**:
```
========================================
Mining Statistics:
========================================
Blocks Found: 5
Total Hashes: 1,234,567,890
Average Hashrate: 1.23 KH/s
Uptime: 3600 seconds
========================================
```

### Test Result: ✅ PASS / ❌ FAIL

---

## Test Scenario 2: Pool Mining Flow

**Objective**: Verify pool server and pool mining works end-to-end

### Step 1: Configure Pool Server

Create `~/intcoin-test/pool/pool.conf`:
```conf
# Pool Configuration
testnet=1

# Stratum
stratum-port=13333
bind-address=0.0.0.0

# HTTP API
http-port=18080

# Blockchain connection
rpc-host=127.0.0.1
rpc-port=12211
rpc-user=test_user_1
rpc-password=test_pass_1

# Pool settings
pool-name=TestPool
pool-fee=1.0
payout-threshold=10000000
payout-method=pplns
pplns-window=1000000

# VarDiff
vardiff-enabled=true
vardiff-target-time=10
vardiff-min-difficulty=1000
vardiff-max-difficulty=1000000

# Database
db-path=~/intcoin-test/pool/pooldb

# Logging
log-level=info
log-file=~/intcoin-test/pool/pool.log
```

### Step 2: Start Pool Server

```bash
# Ensure node is running
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockcount

# Start pool server
./bin/intcoin-pool-server --config=~/intcoin-test/pool/pool.conf
```

**Expected Output**:
```
[INFO] INTcoin Pool Server v1.0.0-alpha
[INFO] Loading configuration from ~/intcoin-test/pool/pool.conf
[INFO] Connecting to intcoind at 127.0.0.1:12211
[INFO] Connected to intcoind (testnet)
[INFO] Database opened: ~/intcoin-test/pool/pooldb
[INFO] Stratum server listening on 0.0.0.0:13333
[INFO] HTTP API server listening on 0.0.0.0:18080
[INFO] Pool ready - accepting connections
```

**Verification**:
- ✅ Pool connects to blockchain node
- ✅ Stratum port is open
- ✅ HTTP API is accessible

### Step 3: Test HTTP API

```bash
# Test pool stats endpoint
curl http://localhost:18080/api/pool/stats | jq
```

**Expected Output**:
```json
{
  "hashrate": 0,
  "difficulty": 1.0,
  "miners": 0,
  "blocks_found": 0,
  "total_shares": 0,
  "valid_shares_24h": 0
}
```

### Step 4: Connect Pool Miner

```bash
# In new terminal, start pool miner
./bin/intcoin-miner \
    --pool \
    --pool-host=127.0.0.1 \
    --pool-port=13333 \
    --pool-user=worker1.miner01 \
    --pool-password=x \
    --threads=2
```

**Expected Output**:
```
[INFO] INTcoin Miner v1.0.0-alpha
[INFO] Pool mining to: 127.0.0.1:13333
[INFO] Worker: worker1.miner01
[INFO] Using 2 threads
[INFO] Connected to pool
[INFO] Subscribed to pool (session_id: 1)
[INFO] Authorized as worker1.miner01
[INFO] Received work (job_id: 1)
[INFO] Mining...
[INFO] Hashrate: 1.23 KH/s
```

### Step 5: Monitor Pool Activity

```bash
# Check pool stats
curl http://localhost:18080/api/pool/stats | jq

# Check top miners
curl http://localhost:18080/api/pool/topminers?limit=10 | jq
```

**Expected**:
- Miners count increased
- Hashrate showing worker contribution
- Worker appears in top miners list

### Step 6: Verify Share Submission

```bash
# Watch pool logs
tail -f ~/intcoin-test/pool/pool.log

# Look for:
# [INFO] Share accepted from worker 1 (difficulty: 10000)
# [INFO] VarDiff adjusted for worker 1: 10000 -> 15000
```

**Verification**:
- ✅ Shares are being accepted
- ✅ VarDiff is adjusting based on hashrate
- ✅ No rejected shares (or very few)

### Step 7: Wait for Block Found

```bash
# Monitor for block found message
tail -f ~/intcoin-test/pool/pool.log

# Expected:
# [INFO] *** BLOCK FOUND *** Height: 10, Hash: 00000abc..., Finder: worker1
# [INFO] Block submitted to blockchain
# [INFO] Block accepted by network
# [INFO] Starting new round (round_id: 2)
```

### Step 8: Verify Payout Calculation

```bash
# Check recent blocks
curl http://localhost:18080/api/pool/blocks?limit=5 | jq

# Check worker stats
curl "http://localhost:18080/api/pool/worker?address=worker1" | jq
```

**Expected Output** (worker stats):
```json
{
  "address": "worker1",
  "hashrate": 1230000,
  "shares": 1234,
  "balance": 104062500,
  "total_paid": 0
}
```

**Verification**:
- ✅ Block appears in pool's block list
- ✅ Worker balance updated with reward share
- ✅ Payout calculation is correct (PPLNS formula)

### Test Result: ✅ PASS / ❌ FAIL

---

## Test Scenario 3: Wallet Flow

**Objective**: Verify Qt wallet functionality end-to-end

### Step 1: Launch Qt Wallet

```bash
./bin/intcoin-qt -testnet -datadir=~/intcoin-test/wallets/wallet1
```

**Expected**: Qt wallet GUI opens

### Step 2: Create New Wallet

**Steps**:
1. Click "File" → "Create New Wallet"
2. Enter password: `TestWallet123!`
3. Confirm password: `TestWallet123!`
4. Click "OK"

**Expected**:
- Mnemonic phrase is displayed (24 words)
- User is prompted to write it down
- Wallet is created successfully

**Record Mnemonic**: `abandon abandon abandon ...` (write down for testing)

### Step 3: Verify Wallet Created

**Steps**:
1. Check wallet status bar
2. Verify: "Wallet: Encrypted, Locked"

**Expected**:
- ✅ Wallet shows as encrypted
- ✅ Wallet shows as locked
- ✅ Balance shows 0.00 INT

### Step 4: Generate Receiving Address

**Steps**:
1. Click "Receive" tab
2. Click "New Address"
3. Enter label: "Test Receipt"
4. Click "OK"

**Expected**:
- New address displayed (starts with `intc1`)
- QR code generated
- Address added to address list

**Record Address**: `intc1qxyz...` (copy for later)

### Step 5: Mine Coins to Wallet

```bash
# In terminal, mine some blocks to wallet address
./bin/intcoin-miner \
    --solo \
    --address=intc1qxyz... \
    --rpc-user=test_user_1 \
    --rpc-password=test_pass_1 \
    --rpc-port=12211 \
    --threads=4 \
    --testnet

# Wait for 101 blocks (coinbase maturity)
```

### Step 6: Verify Balance Update

**In Qt Wallet**:
1. Wait for balance to update
2. Check "Transactions" tab

**Expected**:
- ✅ Balance shows mined coins (after 100 confirmations)
- ✅ Transactions list shows coinbase transactions
- ✅ Each transaction shows proper confirmations

### Step 7: Send Transaction

**Steps**:
1. Click "Send" tab
2. Unlock wallet (enter password: `TestWallet123!`)
3. Enter recipient address: (generate new address or use existing)
4. Enter amount: 1.0 INT (1,000,000,000 base units)
5. Enter fee: 0.001 INT (1,000,000 base units)
6. Click "Send"

**Expected**:
- Confirmation dialog appears
- Transaction details are correct
- Click "Confirm" → Transaction sent
- Transaction appears in "Transactions" tab

### Step 8: Verify Transaction Broadcast

```bash
# Check mempool
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getmempoolinfo
```

**Expected**:
```json
{
  "size": 1,
  "bytes": 3500,
  "usage": 4096
}
```

### Step 9: Backup Wallet

**Steps**:
1. Click "File" → "Backup Wallet"
2. Choose location: `~/intcoin-test/wallets/backup.dat`
3. Click "Save"

**Expected**:
- Success message: "Wallet backed up successfully"
- Backup file exists

**Verification**:
```bash
ls -lh ~/intcoin-test/wallets/backup.dat
# Expected: file exists, ~50-200 KB
```

### Step 10: Restore from Backup

**Steps**:
1. Close Qt wallet
2. Launch new wallet instance:
```bash
./bin/intcoin-qt -testnet -datadir=~/intcoin-test/wallets/wallet2
```
3. Click "File" → "Open Wallet"
4. Select `~/intcoin-test/wallets/backup.dat`
5. Enter password: `TestWallet123!`

**Expected**:
- ✅ Wallet opens successfully
- ✅ Same balance as original wallet
- ✅ Same transaction history
- ✅ Same addresses

### Test Result: ✅ PASS / ❌ FAIL

---

## Test Scenario 4: Blockchain Sync Flow

**Objective**: Verify blockchain synchronization between nodes

### Step 1: Generate Blockchain on Node 1

```bash
# Mine 50 blocks on node 1
for i in {1..50}; do
    ./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf \
        generatetoaddress 1 intc1qxyz...
    sleep 1
done

# Verify block count
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockcount
# Expected: 50
```

### Step 2: Start Node 2 (Fresh Sync)

```bash
# Ensure node2 data directory is empty
rm -rf ~/intcoin-test/node2/testnet3

# Start node 2
./bin/intcoind -conf=~/intcoin-test/node2/intcoin.conf -daemon

# Wait for startup
sleep 5
```

### Step 3: Monitor Sync Progress

```bash
# Watch node 2 sync
watch -n 2 './bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf getblockchaininfo | jq ".blocks, .headers"'
```

**Expected**:
- Headers download first (fast)
- Blocks download and validate (slower)
- Progress: 0 → 50 blocks

### Step 4: Verify Sync Completion

```bash
# Check node 2 status
./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf getblockchaininfo
```

**Expected Output**:
```json
{
  "chain": "test",
  "blocks": 50,
  "headers": 50,
  "bestblockhash": "same_as_node1...",
  "difficulty": 1.0,
  "chainwork": "0000...0033"
}
```

**Verification**:
- ✅ Block count matches node 1
- ✅ Best block hash matches node 1
- ✅ Chain work matches node 1

### Step 5: Verify Block Consistency

```bash
# Compare block hashes for heights 1, 25, 50
for HEIGHT in 1 25 50; do
    HASH1=$(./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockhash $HEIGHT)
    HASH2=$(./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf getblockhash $HEIGHT)

    if [ "$HASH1" == "$HASH2" ]; then
        echo "✅ Block $HEIGHT matches"
    else
        echo "❌ Block $HEIGHT MISMATCH"
    fi
done
```

**Expected**: All blocks match

### Step 6: Verify UTXO Set

```bash
# Get UTXO set info from both nodes
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf gettxoutsetinfo
./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf gettxoutsetinfo
```

**Expected**: UTXO set hash matches between nodes

### Step 7: Test Reorg Handling

```bash
# Disconnect nodes
./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf \
    disconnectnode "127.0.0.1:12210"

# Mine 5 blocks on node 1
for i in {1..5}; do
    ./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf \
        generatetoaddress 1 intc1qabc...
done

# Mine 7 blocks on node 2 (longer chain)
for i in {1..7}; do
    ./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf \
        generatetoaddress 1 intc1qdef...
done

# Reconnect nodes
./bin/intcoin-cli -conf=~/intcoin-test/node2/intcoin.conf \
    addnode "127.0.0.1:12210" "add"

# Wait for sync
sleep 10

# Verify node 1 reorganized to node 2's chain
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getblockcount
# Expected: 57 (50 + 7 from node2)
```

**Verification**:
- ✅ Node 1 accepted longer chain from node 2
- ✅ Node 1 reverted its 5 blocks
- ✅ Both nodes now on same chain (57 blocks)

### Step 8: Verify Transaction Reorg

```bash
# Check if any transactions were returned to mempool after reorg
./bin/intcoin-cli -conf=~/intcoin-test/node1/intcoin.conf getmempoolinfo
```

**Expected**: Transactions from reverted blocks are back in mempool

### Test Result: ✅ PASS / ❌ FAIL

---

## Test Results Documentation

### Summary Template

```
INTcoin v1.0.0-alpha Integration Test Results
Date: YYYY-MM-DD
Tester: [Name]
Environment: [OS, CPU, RAM]

Scenario 1: Full Mining Flow ................ [✅ PASS / ❌ FAIL]
Scenario 2: Pool Mining Flow ................ [✅ PASS / ❌ FAIL]
Scenario 3: Wallet Flow ..................... [✅ PASS / ❌ FAIL]
Scenario 4: Blockchain Sync Flow ............ [✅ PASS / ❌ FAIL]

Overall Result: [✅ ALL PASS / ❌ FAILURES DETECTED]

Issues Found:
[List any bugs, errors, or unexpected behavior]

Performance Notes:
- Mining hashrate: X KH/s
- Block time: X seconds
- Sync speed: X blocks/second
- Pool share rate: X shares/minute

Notes:
[Any additional observations]
```

### Automated Test Script

Create `run_integration_tests.sh`:

```bash
#!/bin/bash
# INTcoin Integration Test Runner

set -e

echo "========================================="
echo "INTcoin v1.0.0-alpha Integration Tests"
echo "========================================="

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Cleanup function
cleanup() {
    echo "Cleaning up test environment..."
    killall intcoind 2>/dev/null || true
    killall intcoin-miner 2>/dev/null || true
    killall intcoin-pool-server 2>/dev/null || true
}

trap cleanup EXIT

# Run test scenario
run_test() {
    local test_name=$1
    local test_command=$2

    echo -n "Running: $test_name... "

    if eval "$test_command" > /tmp/test_output.log 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        cat /tmp/test_output.log
        ((TESTS_FAILED++))
    fi
}

# Test 1: Node startup
run_test "Node Startup" "./bin/intcoind -testnet -daemon"

# Test 2: GetBlockTemplate
run_test "GetBlockTemplate" "./bin/intcoin-cli -testnet getblocktemplate '{\"rules\":[\"segwit\"]}'"

# ... Add more tests ...

echo "========================================="
echo "Test Results:"
echo "  Passed: $TESTS_PASSED"
echo "  Failed: $TESTS_FAILED"
echo "========================================="

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}SOME TESTS FAILED${NC}"
    exit 1
fi
```

---

## Conclusion

These integration tests ensure that all major components of INTcoin v1.0.0-alpha work correctly:

- ✅ **Blockchain**: Core consensus, validation, and UTXO management
- ✅ **Mining**: Solo mining with GetBlockTemplate
- ✅ **Pool**: Pool server, Stratum protocol, share validation, payouts
- ✅ **Wallet**: HD wallet, encryption, transactions, backup/restore
- ✅ **Network**: P2P sync, reorganization handling

**Next Steps**:
1. Run all test scenarios
2. Document results
3. Fix any issues found
4. Re-test after fixes
5. Prepare release notes

---

**For Questions or Issues**: https://github.com/intcoin/crypto/issues
