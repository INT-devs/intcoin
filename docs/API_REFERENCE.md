# INTcoin API Reference

**Version**: 1.0.0-beta
**Last Updated**: December 26, 2025

This document provides comprehensive API reference documentation for INTcoin's core components, RPC interface, and network protocol.

---

## Table of Contents

1. [RPC API](#rpc-api)
2. [Blockchain API](#blockchain-api)
3. [Wallet API](#wallet-api)
4. [Mining API](#mining-api)
5. [Network API](#network-api)
6. [Lightning Network API](#lightning-network-api)
7. [Mining Pool API](#mining-pool-api)
8. [Data Types](#data-types)
9. [Error Codes](#error-codes)

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
