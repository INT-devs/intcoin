# INTcoin API Reference

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

This document provides comprehensive API reference documentation for INTcoin's core components, RPC interface, and network protocol.

## What's New in v1.2.0-beta

- 📊 **Enhanced Mempool API**: Priority-based mempool management with 6 priority levels
- 📈 **Prometheus Metrics API**: 40+ metrics endpoints for monitoring and observability
- 🔄 **Atomic Swaps API**: HTLC-based cross-chain trading (BTC, LTC, XMR)
- 🌉 **Bridge API**: Cross-chain bridges for ETH, BTC, BSC
- 📱 **SPV API**: Bloom filter and header synchronization for mobile wallets

See complete RPC documentation: [RPC.md](RPC.md)

---

## Table of Contents

1. [RPC API](#rpc-api)
2. [Blockchain API](#blockchain-api)
3. [Wallet API](#wallet-api)
4. [Mining API](#mining-api)
5. [Network API](#network-api)
6. [Lightning Network API](#lightning-network-api)
7. [Mining Pool API](#mining-pool-api)
8. [Enhanced Mempool API (v1.2.0)](#enhanced-mempool-api-v120)
9. [Prometheus Metrics API (v1.2.0)](#prometheus-metrics-api-v120)
10. [Atomic Swaps API (v1.2.0)](#atomic-swaps-api-v120)
11. [Bridge API (v1.2.0)](#bridge-api-v120)
12. [SPV API (v1.2.0)](#spv-api-v120)
13. [Data Types](#data-types)
14. [Error Codes](#error-codes)

---

## RPC API

INTcoin provides a JSON-RPC 2.0 interface for interacting with the node. The RPC server listens on port `9332` by default.

### Connection

```bash
# Using curl
curl --user username:password --data-binary '{"jsonrpc": "2.0", "id": "1", "method": "getblockchaininfo", "params": []}' \
  -H 'content-type: application/json' http://127.0.0.1:9332/

# Using intcoin-cli
intcoin-cli getblockchaininfo
```

### Authentication

RPC requests require HTTP Basic Authentication. Configure credentials in `intcoin.conf`:

```ini
rpcuser=yourusername
rpcpassword=yourpassword
rpcport=9332
```

---

## Blockchain API

### `getblockchaininfo`

Returns information about the current state of the blockchain.

**Parameters**: None

**Result**:
```json
{
  "chain": "main",
  "blocks": 12345,
  "headers": 12345,
  "bestblockhash": "0000000000000000000abc123...",
  "difficulty": 1234567.89,
  "chainwork": "00000000000000000000000000000abc",
  "pruned": false
}
```

**Example**:
```bash
intcoin-cli getblockchaininfo
```

---

### `getblock`

Returns information about a block by hash or height.

**Parameters**:
1. `blockhash` (string, required) - The block hash
2. `verbosity` (integer, optional, default=1) - 0=hex, 1=json, 2=json with tx details

**Result** (verbosity=1):
```json
{
  "hash": "0000000000000000000abc123...",
  "confirmations": 10,
  "height": 12345,
  "version": 1,
  "merkleroot": "abc123...",
  "time": 1735171200,
  "nonce": 987654321,
  "bits": "1a00ffff",
  "difficulty": 1234567.89,
  "chainwork": "00000000000000000000000000000abc",
  "previousblockhash": "0000000000000000000xyz789...",
  "nextblockhash": "0000000000000000000def456...",
  "tx": ["txid1", "txid2", ...]
}
```

**Example**:
```bash
intcoin-cli getblock "0000000000000000000abc123..." 1
```

---

### `getblockheader`

Returns the block header for a given block hash.

**Parameters**:
1. `blockhash` (string, required) - The block hash
2. `verbose` (boolean, optional, default=true) - true for JSON, false for hex

**Result**:
```json
{
  "hash": "0000000000000000000abc123...",
  "height": 12345,
  "version": 1,
  "merkleroot": "abc123...",
  "time": 1735171200,
  "nonce": 987654321,
  "bits": "1a00ffff",
  "difficulty": 1234567.89,
  "previousblockhash": "0000000000000000000xyz789...",
  "nextblockhash": "0000000000000000000def456..."
}
```

---

### `getblockhash`

Returns the block hash at a given height.

**Parameters**:
1. `height` (integer, required) - The block height

**Result**: Block hash (string)

**Example**:
```bash
intcoin-cli getblockhash 12345
```

---

### `getblockcount`

Returns the number of blocks in the longest blockchain.

**Parameters**: None

**Result**: Block count (integer)

**Example**:
```bash
intcoin-cli getblockcount
```

---

### `getbestblockhash`

Returns the hash of the best (tip) block in the longest blockchain.

**Parameters**: None

**Result**: Block hash (string)

---

### `getdifficulty`

Returns the current mining difficulty.

**Parameters**: None

**Result**: Difficulty (double)

---

### `getrawmempool`

Returns all transaction IDs in the memory pool.

**Parameters**:
1. `verbose` (boolean, optional, default=false) - true for detailed info

**Result** (verbose=false): Array of transaction IDs

**Result** (verbose=true):
```json
{
  "txid1": {
    "size": 250,
    "fee": 0.0001,
    "time": 1735171200,
    "height": 12345,
    "depends": []
  },
  ...
}
```

---

### `gettxout`

Returns details about an unspent transaction output (UTXO).

**Parameters**:
1. `txid` (string, required) - The transaction ID
2. `vout` (integer, required) - The output index
3. `include_mempool` (boolean, optional, default=true) - Whether to include mempool

**Result**:
```json
{
  "bestblock": "0000000000000000000abc123...",
  "confirmations": 10,
  "value": 50.0,
  "scriptPubKey": {
    "asm": "OP_DUP OP_HASH160 ...",
    "hex": "76a914...",
    "type": "pubkeyhash",
    "address": "INT1abc123..."
  },
  "coinbase": false
}
```

---

### `verifytxoutproof`

Verifies that a proof points to a transaction in a block.

**Parameters**:
1. `proof` (string, required) - The hex-encoded proof

**Result**: Array of transaction IDs

---

## Wallet API

### `getnewaddress`

Returns a new INTcoin address for receiving payments.

**Parameters**:
1. `label` (string, optional) - Label for the address

**Result**: INTcoin address (string)

**Example**:
```bash
intcoin-cli getnewaddress "My Address"
```

---

### `getbalance`

Returns the total available balance in the wallet.

**Parameters**:
1. `minconf` (integer, optional, default=1) - Minimum confirmations

**Result**: Balance in INT (double)

**Example**:
```bash
intcoin-cli getbalance
```

---

### `sendtoaddress`

Sends INT to a given address.

**Parameters**:
1. `address` (string, required) - The recipient address
2. `amount` (double, required) - The amount in INT
3. `comment` (string, optional) - Transaction comment
4. `subtractfeefromamount` (boolean, optional, default=false) - Deduct fee from amount

**Result**: Transaction ID (string)

**Example**:
```bash
intcoin-cli sendtoaddress "INT1abc123..." 10.5
```

---

### `listunspent`

Returns array of unspent transaction outputs.

**Parameters**:
1. `minconf` (integer, optional, default=1) - Minimum confirmations
2. `maxconf` (integer, optional, default=9999999) - Maximum confirmations
3. `addresses` (array, optional) - Filter by addresses

**Result**:
```json
[
  {
    "txid": "abc123...",
    "vout": 0,
    "address": "INT1abc123...",
    "scriptPubKey": "76a914...",
    "amount": 50.0,
    "confirmations": 10,
    "spendable": true,
    "solvable": true
  },
  ...
]
```

---

### `listtransactions`

Returns list of transactions for the wallet.

**Parameters**:
1. `count` (integer, optional, default=10) - Number of transactions
2. `skip` (integer, optional, default=0) - Skip first N transactions
3. `include_watchonly` (boolean, optional, default=false) - Include watch-only addresses

**Result**:
```json
[
  {
    "txid": "abc123...",
    "amount": 50.0,
    "confirmations": 10,
    "blockhash": "0000000000000000000abc123...",
    "blocktime": 1735171200,
    "time": 1735171200,
    "timereceived": 1735171200,
    "category": "receive",
    "address": "INT1abc123...",
    "vout": 0
  },
  ...
]
```

---

### `backupwallet`

Safely copies wallet.dat to destination.

**Parameters**:
1. `destination` (string, required) - Backup file path

**Result**: null on success

**Example**:
```bash
intcoin-cli backupwallet "/backup/wallet-backup.dat"
```

---

### `encryptwallet`

Encrypts the wallet with a passphrase.

**Parameters**:
1. `passphrase` (string, required) - Encryption passphrase

**Result**: Warning message about wallet restart

**Example**:
```bash
intcoin-cli encryptwallet "mysecurepassphrase"
```

---

### `walletpassphrase`

Unlocks the wallet for a specified time.

**Parameters**:
1. `passphrase` (string, required) - Wallet passphrase
2. `timeout` (integer, required) - Unlock duration in seconds

**Result**: null on success

**Example**:
```bash
intcoin-cli walletpassphrase "mysecurepassphrase" 300
```

---

### `walletlock`

Locks the wallet.

**Parameters**: None

**Result**: null on success

---

## Mining API

### `getmininginfo`

Returns mining-related information.

**Parameters**: None

**Result**:
```json
{
  "blocks": 12345,
  "currentblocksize": 1000000,
  "currentblocktx": 500,
  "difficulty": 1234567.89,
  "networkhashps": 123456789012345,
  "pooledtx": 10,
  "chain": "main"
}
```

---

### `getblocktemplate`

Returns data needed to construct a block to work on.

**Parameters**:
1. `template_request` (object, optional) - Template request object

**Result**:
```json
{
  "version": 1,
  "previousblockhash": "0000000000000000000abc123...",
  "transactions": [...],
  "coinbasevalue": 5000000000,
  "target": "00000000ffff0000000000000000000000000000000000000000000000000000",
  "mintime": 1735171200,
  "mutable": ["time", "transactions", "prevblock"],
  "noncerange": "00000000ffffffff",
  "sigoplimit": 20000,
  "sizelimit": 8000000,
  "curtime": 1735171200,
  "bits": "1a00ffff",
  "height": 12345
}
```

---

### `submitblock`

Attempts to submit a new block to the network.

**Parameters**:
1. `hexdata` (string, required) - Block data in hex
2. `parameters` (object, optional) - Additional parameters

**Result**: null on success, error string on rejection

**Example**:
```bash
intcoin-cli submitblock "0100000000000000..."
```

---

### `getnetworkhashps`

Returns estimated network hash rate.

**Parameters**:
1. `nblocks` (integer, optional, default=120) - Number of blocks to estimate
2. `height` (integer, optional, default=-1) - Height to estimate at

**Result**: Network hash rate in hashes per second (double)

---

## Network API

### `getpeerinfo`

Returns information about each connected network node.

**Parameters**: None

**Result**:
```json
[
  {
    "id": 1,
    "addr": "192.168.1.100:9333",
    "addrlocal": "192.168.1.50:12345",
    "services": "0000000000000001",
    "lastsend": 1735171200,
    "lastrecv": 1735171200,
    "bytessent": 123456,
    "bytesrecv": 654321,
    "conntime": 1735170000,
    "version": 70015,
    "subver": "/INTcoin:1.0.0/",
    "inbound": false,
    "startingheight": 12340
  },
  ...
]
```

---

### `addnode`

Attempts to add or remove a node from the addnode list.

**Parameters**:
1. `node` (string, required) - Node address (IP:port)
2. `command` (string, required) - "add", "remove", or "onetry"

**Result**: null on success

**Example**:
```bash
intcoin-cli addnode "192.168.1.100:9333" "add"
```

---

### `getconnectioncount`

Returns the number of connections to other nodes.

**Parameters**: None

**Result**: Connection count (integer)

---

### `getnettotals`

Returns information about network traffic.

**Parameters**: None

**Result**:
```json
{
  "totalbytesrecv": 123456789,
  "totalbytessent": 987654321,
  "timemillis": 1735171200000
}
```

---

## Lightning Network API

### `lightning_openchannel`

Opens a Lightning Network payment channel.

**Parameters**:
1. `node_id` (string, required) - Remote node public key
2. `amount` (double, required) - Channel capacity in INT
3. `push_amount` (double, optional, default=0) - Amount to push to remote

**Result**:
```json
{
  "channel_id": "abc123...",
  "funding_txid": "def456...",
  "status": "pending"
}
```

---

### `lightning_closechannel`

Closes a Lightning Network channel.

**Parameters**:
1. `channel_id` (string, required) - Channel ID
2. `force` (boolean, optional, default=false) - Force close

**Result**:
```json
{
  "channel_id": "abc123...",
  "closing_txid": "ghi789...",
  "status": "closing"
}
```

---

### `lightning_sendpayment`

Sends a Lightning Network payment.

**Parameters**:
1. `invoice` (string, required) - BOLT #11 payment invoice
2. `amount` (double, optional) - Amount in INT (if not in invoice)

**Result**:
```json
{
  "payment_hash": "abc123...",
  "payment_preimage": "def456...",
  "amount": 0.01,
  "fee": 0.000001,
  "status": "success"
}
```

---

### `lightning_createinvoice`

Creates a Lightning Network payment invoice.

**Parameters**:
1. `amount` (double, required) - Amount in INT
2. `description` (string, required) - Invoice description
3. `expiry` (integer, optional, default=3600) - Expiry time in seconds

**Result**:
```json
{
  "payment_hash": "abc123...",
  "invoice": "lnint1...",
  "expires_at": 1735174800
}
```

---

### `lightning_listchannels`

Lists all Lightning Network channels.

**Parameters**: None

**Result**:
```json
[
  {
    "channel_id": "abc123...",
    "node_id": "def456...",
    "capacity": 10.0,
    "local_balance": 5.0,
    "remote_balance": 5.0,
    "status": "active"
  },
  ...
]
```

---

## Mining Pool API

### `pool_getstats`

Returns mining pool statistics.

**Parameters**: None

**Result**:
```json
{
  "hashrate": 123456789012345,
  "miners": 100,
  "blocks_found": 50,
  "total_paid": 2500.0,
  "pool_fee": 1.0,
  "min_payout": 1.0
}
```

---

### `pool_getworker`

Returns statistics for a specific worker.

**Parameters**:
1. `address` (string, required) - Worker's payout address

**Result**:
```json
{
  "address": "INT1abc123...",
  "hashrate": 1234567890,
  "shares_valid": 1000,
  "shares_invalid": 10,
  "balance": 5.5,
  "total_paid": 45.0,
  "last_share": 1735171200
}
```

---

### `pool_getblocks`

Returns recent blocks found by the pool.

**Parameters**:
1. `limit` (integer, optional, default=10) - Number of blocks to return

**Result**:
```json
[
  {
    "height": 12345,
    "hash": "0000000000000000000abc123...",
    "reward": 50.0,
    "finder": "INT1abc123...",
    "time": 1735171200,
    "status": "confirmed"
  },
  ...
]
```

---

### `pool_getpayments`

Returns recent payments made by the pool.

**Parameters**:
1. `limit` (integer, optional, default=20) - Number of payments to return
2. `address` (string, optional) - Filter by address

**Result**:
```json
[
  {
    "txid": "abc123...",
    "address": "INT1abc123...",
    "amount": 10.5,
    "time": 1735171200,
    "confirmations": 10
  },
  ...
]
```

---

## Enhanced Mempool API (v1.2.0)

INTcoin v1.2.0-beta introduces enhanced mempool management with priority-based transaction handling.

### `getmempoolinfo`

Returns detailed mempool statistics including priority breakdown.

**Parameters**: None

**Result**:
```json
{
  "size": 1523,
  "bytes": 456789,
  "usage": 567890,
  "maxmempool": 300000000,
  "mempoolminfee": 0.00001000,
  "priority_breakdown": {
    "LOW": 100,
    "NORMAL": 1200,
    "HIGH": 200,
    "HTLC": 15,
    "BRIDGE": 5,
    "CRITICAL": 3
  }
}
```

### `setmempoolpriority`

Changes the priority of a transaction in the mempool.

**Parameters**:
1. `txid` (string, required) - Transaction ID
2. `priority` (string, required) - Priority level: LOW, NORMAL, HIGH, HTLC, BRIDGE, CRITICAL

**Result**: `true` on success

**Example**:
```bash
intcoin-cli setmempoolpriority "abc123..." "HIGH"
```

### `savemempool`

Persists the current mempool to disk.

**Parameters**: None

**Result**: `true` on success

See also: [Enhanced Mempool Documentation](ENHANCED_MEMPOOL.md)

---

## Prometheus Metrics API (v1.2.0)

### `getmetricsinfo`

Returns Prometheus metrics server configuration and status.

**Parameters**: None

**Result**:
```json
{
  "enabled": true,
  "bind": "127.0.0.1",
  "port": 9090,
  "threads": 2,
  "requests_total": 12345,
  "uptime_seconds": 86400
}
```

### `getprometheusmetrics`

Returns all metrics in Prometheus text exposition format.

**Parameters**: None

**Result**: Prometheus metrics text (string)

**HTTP Endpoint**: `GET http://localhost:9090/metrics`

See also: [Prometheus Metrics Documentation](PROMETHEUS_METRICS.md)

---

## Atomic Swaps API (v1.2.0)

### `initiate_atomic_swap`

Initiates an atomic swap with a counterparty.

**Parameters**:
1. `chain` (string, required) - Target blockchain (BTC, LTC, XMR)
2. `amount` (double, required) - Amount to swap (in INT)
3. `counterparty_amount` (double, required) - Amount to receive
4. `counterparty_address` (string, required) - Counterparty's address
5. `locktime` (integer, required) - HTLC locktime (blocks)

**Result**:
```json
{
  "swap_id": "abc123...",
  "secret_hash": "def456...",
  "htlc_address": "INT1qxy...",
  "funding_txid": "ghi789...",
  "status": "initiated"
}
```

### `redeem_atomic_swap`

Redeems funds from a completed atomic swap.

**Parameters**:
1. `swap_id` (string, required) - Swap ID
2. `secret` (string, required) - Preimage secret

**Result**: Transaction ID of redemption

### `refund_atomic_swap`

Refunds an expired atomic swap.

**Parameters**:
1. `swap_id` (string, required) - Swap ID

**Result**: Transaction ID of refund

See also: [Atomic Swaps Documentation](ATOMIC_SWAPS.md)

---

## Bridge API (v1.2.0)

### `bridge_deposit`

Deposits INT to a cross-chain bridge.

**Parameters**:
1. `bridge` (string, required) - Bridge name (ETH, BTC, BSC)
2. `amount` (double, required) - Amount to bridge
3. `destination_address` (string, required) - Address on destination chain

**Result**:
```json
{
  "deposit_id": "abc123...",
  "lock_txid": "def456...",
  "mint_address": "0x789...",
  "status": "pending"
}
```

### `bridge_withdraw`

Withdraws from a cross-chain bridge back to INT.

**Parameters**:
1. `bridge` (string, required) - Bridge name
2. `amount` (double, required) - Amount to withdraw
3. `burn_txid` (string, required) - Burn transaction on other chain

**Result**:
```json
{
  "withdrawal_id": "abc123...",
  "unlock_txid": "def456...",
  "status": "pending"
}
```

### `bridge_status`

Gets the status of a bridge operation.

**Parameters**:
1. `operation_id` (string, required) - Deposit or withdrawal ID

**Result**:
```json
{
  "operation_id": "abc123...",
  "type": "deposit",
  "bridge": "ETH",
  "amount": 10.5,
  "status": "confirmed",
  "confirmations": 12,
  "created_at": 1735171200
}
```

See also: [Cross-Chain Bridges Documentation](CROSS_CHAIN_BRIDGES.md)

---

## SPV API (v1.2.0)

### `loadbloomfilter`

Loads a Bloom filter for SPV transaction filtering.

**Parameters**:
1. `filter` (string, required) - Hex-encoded Bloom filter
2. `num_hash_funcs` (integer, required) - Number of hash functions
3. `tweak` (integer, required) - Randomization tweak
4. `flags` (integer, required) - Update flags

**Result**: `true` on success

### `getheaders`

Gets block headers for SPV synchronization.

**Parameters**:
1. `block_locator` (array, required) - Array of block hashes
2. `hash_stop` (string, optional) - Hash to stop at

**Result**: Array of block headers

### `getmerkleproof`

Gets Merkle proof for a transaction.

**Parameters**:
1. `txid` (string, required) - Transaction ID
2. `blockhash` (string, optional) - Block hash containing transaction

**Result**:
```json
{
  "txid": "abc123...",
  "blockhash": "0000000000000000000abc123...",
  "merkleproof": ["hash1", "hash2", ...],
  "index": 5
}
```

See also: [SPV & Bloom Filters Documentation](SPV_AND_BLOOM_FILTERS.md), [Mobile SDK](MOBILE_SDK.md)

---

## Data Types

### Address

INTcoin addresses are Bech32-encoded strings starting with "INT1" for mainnet and "TINT1" for testnet.

**Format**: `INT1[a-z0-9]{39,59}`

**Example**: `INT1qp3wjpa3tjlj042z2wv7hahsldgwhwy0l9ctz`

---

### Transaction ID (txid)

64-character hexadecimal string representing the SHA3-256 hash of a transaction.

**Format**: `[0-9a-f]{64}`

**Example**: `abc123def456...` (64 characters)

---

### Block Hash

64-character hexadecimal string representing the SHA3-256 hash of a block header.

**Format**: `[0-9a-f]{64}`

**Example**: `0000000000000000000abc123...` (64 characters)

---

### Amount

Amounts are represented as decimal numbers in INT.

- **1 INT** = 100,000,000 satoshis
- **Precision**: 8 decimal places
- **Range**: 0.00000001 to 21,000,000.00000000

---

## Error Codes

### RPC Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -1 | `RPC_MISC_ERROR` | Standard RPC error |
| -3 | `RPC_TYPE_ERROR` | Invalid type for parameter |
| -5 | `RPC_INVALID_ADDRESS_OR_KEY` | Invalid address or key |
| -8 | `RPC_OUT_OF_MEMORY` | Out of memory |
| -20 | `RPC_DATABASE_ERROR` | Database error |
| -22 | `RPC_DESERIALIZATION_ERROR` | Error parsing or validating structure |
| -25 | `RPC_VERIFY_ERROR` | Verification error |
| -26 | `RPC_VERIFY_REJECTED` | Transaction or block rejected |
| -27 | `RPC_VERIFY_ALREADY_IN_CHAIN` | Transaction already in chain |
| -28 | `RPC_IN_WARMUP` | Client still warming up |

### Wallet Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -4 | `RPC_WALLET_ERROR` | Unspecified wallet error |
| -6 | `RPC_WALLET_INSUFFICIENT_FUNDS` | Not enough funds |
| -13 | `RPC_WALLET_PASSPHRASE_INCORRECT` | Incorrect passphrase |
| -14 | `RPC_WALLET_WRONG_ENC_STATE` | Command given in wrong wallet encryption state |
| -15 | `RPC_WALLET_ENCRYPTION_FAILED` | Failed to encrypt wallet |
| -16 | `RPC_WALLET_ALREADY_UNLOCKED` | Wallet is already unlocked |

---

## HTTP REST API

INTcoin also provides a REST API accessible via HTTP GET requests.

### Endpoints

- `GET /rest/tx/<txid>.<format>` - Get transaction
- `GET /rest/block/<hash>.<format>` - Get block
- `GET /rest/chaininfo.<format>` - Get blockchain info
- `GET /rest/mempool/contents.<format>` - Get mempool contents

**Supported Formats**: `json`, `bin`, `hex`

**Example**:
```bash
curl http://127.0.0.1:9332/rest/block/0000000000000000000abc123.json
```

---

## Code Examples

### Python Example

```python
import requests
import json

# RPC configuration
rpc_user = "yourusername"
rpc_password = "yourpassword"
rpc_url = "http://127.0.0.1:9332/"

def rpc_call(method, params=[]):
    payload = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": method,
        "params": params
    }
    response = requests.post(
        rpc_url,
        auth=(rpc_user, rpc_password),
        data=json.dumps(payload),
        headers={"content-type": "application/json"}
    )
    return response.json()["result"]

# Get blockchain info
info = rpc_call("getblockchaininfo")
print(f"Current height: {info['blocks']}")

# Get new address
address = rpc_call("getnewaddress", ["My Address"])
print(f"New address: {address}")

# Send payment
txid = rpc_call("sendtoaddress", [address, 10.5])
print(f"Transaction ID: {txid}")
```

---

## Additional Resources

- **Building Guide**: [docs/BUILDING.md](BUILDING.md)
- **Mining Guide**: [docs/MINING.md](MINING.md)
- **Testing Guide**: [docs/TESTING.md](TESTING.md)
- **Lightning Network**: [docs/LIGHTNING.md](LIGHTNING.md)
- **Pool Setup**: [docs/POOL_SETUP.md](POOL_SETUP.md)

---

**For Questions**: https://github.com/INT-devs/intcoin/issues
