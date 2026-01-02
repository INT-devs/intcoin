# INTcoin RPC API Documentation

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026

This document describes the JSON-RPC API for interacting with the INTcoin daemon (`intcoind`).

## What's New in v1.2.0-beta

- **Enhanced Mempool RPCs**: Priority-aware transaction queries
- **Metrics RPCs**: Prometheus metrics endpoint information
- **Bridge RPCs**: Cross-chain bridge operations
- **Atomic Swap RPCs**: HTLC and atomic swap management
- **SPV RPCs**: Bloom filter and headers-only sync

---

## Table of Contents

- [Overview](#overview)
- [Connection](#connection)
- [Authentication](#authentication)
- [Request Format](#request-format)
- [Response Format](#response-format)
- [Error Codes](#error-codes)
- [RPC Methods](#rpc-methods)
  - [Blockchain](#blockchain-methods)
  - [Network](#network-methods)
  - [Mining](#mining-methods)
  - [Raw Transactions](#raw-transaction-methods)
  - [Utility](#utility-methods)
  - [Fee Estimation](#fee-estimation-methods)
  - [Lightning Network](#lightning-network-methods)
  - [Mining Pool](#mining-pool-methods)
  - [Enhanced Blockchain](#enhanced-blockchain-methods)
  - [Enhanced Mempool](#enhanced-mempool-methods) **(New in v1.2.0)**
  - [Metrics & Monitoring](#metrics-and-monitoring-methods) **(New in v1.2.0)**
  - [Atomic Swaps](#atomic-swap-methods) **(New in v1.2.0)**
  - [Cross-Chain Bridges](#cross-chain-bridge-methods) **(New in v1.2.0)**
  - [SPV & Bloom Filters](#spv-and-bloom-filter-methods) **(New in v1.2.0)**
- [Examples](#examples)

---

## Overview

INTcoin implements a JSON-RPC 2.0 server that is compatible with Bitcoin Core RPC for ease of integration with existing tools and libraries.

**Features**:
- JSON-RPC 2.0 protocol
- HTTP Basic Authentication
- Bitcoin-compatible error codes
- Zero external JSON library dependencies
- Thread-safe multi-client support
- Non-blocking accept loop

---

## Connection

### Default Ports
- **Mainnet RPC**: 2211
- **Testnet RPC**: 12211

### Connection URL
```
http://127.0.0.1:2211
```

### Using intcoin-cli

The simplest way to interact with the RPC server is using `intcoin-cli`:

```bash
# Basic command
./intcoin-cli getblockcount

# With authentication
./intcoin-cli -rpcuser=myuser -rpcpassword=mypass getblockcount

# Testnet
./intcoin-cli -testnet getblockcount

# Remote server
./intcoin-cli -rpcconnect=192.168.1.100 -rpcport=2211 getblockcount
```

---

## Authentication

The RPC server supports HTTP Basic Authentication.

### Configuration

Set authentication in `intcoind` startup:

```bash
./intcoind -rpcuser=myuser -rpcpassword=mypassword
```

Or in configuration file (`~/.intcoin/intcoin.conf`):

```conf
rpcuser=myuser
rpcpassword=mypassword
rpcport=2211
rpcbind=127.0.0.1
```

### Security Best Practices

✅ **DO**:
- Use strong, randomly generated passwords (32+ characters)
- Bind to localhost (127.0.0.1) only unless necessary
- Use firewall rules to restrict access
- Use TLS/SSL proxy (nginx, stunnel) for remote access

❌ **DON'T**:
- Use weak or default passwords
- Expose RPC to public internet without TLS
- Reuse passwords across systems

---

## Request Format

JSON-RPC 2.0 request structure:

```json
{
  "jsonrpc": "2.0",
  "id": "client-id",
  "method": "method_name",
  "params": ["param1", "param2"]
}
```

### Fields

- `jsonrpc` (string): Must be `"2.0"`
- `id` (string|number): Client-provided request identifier
- `method` (string): RPC method name
- `params` (array): Method parameters (optional, defaults to `[]`)

---

## Response Format

### Success Response

```json
{
  "jsonrpc": "2.0",
  "id": "client-id",
  "result": { ... }
}
```

### Error Response

```json
{
  "jsonrpc": "2.0",
  "id": "client-id",
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

---

## Error Codes

INTcoin uses Bitcoin-compatible error codes:

### Standard JSON-RPC Errors

| Code | Message | Description |
|------|---------|-------------|
| `-32700` | Parse error | Invalid JSON |
| `-32600` | Invalid request | Invalid JSON-RPC |
| `-32601` | Method not found | Unknown method |
| `-32602` | Invalid params | Invalid method parameters |
| `-32603` | Internal error | Internal JSON-RPC error |

### Application Errors

| Code | Message | Description |
|------|---------|-------------|
| `-1` | Miscellaneous error | Generic error |
| `-3` | Invalid amount | Invalid amount specified |
| `-5` | Invalid address | Invalid INTcoin address |
| `-8` | Block not found | Block hash not found |
| `-17` | Database error | Database read/write error |
| `-25` | Transaction rejected | Transaction not accepted |

---

## RPC Methods

### Blockchain Methods

#### `getblockcount`

Returns the current blockchain height.

**Parameters**: None

**Returns**: `number` - Block height

**Example**:
```bash
./intcoin-cli getblockcount
```

```json
{
  "result": 12345
}
```

---

#### `getbestblockhash`

Returns the hash of the best (tip) block.

**Parameters**: None

**Returns**: `string` - Block hash (hex)

**Example**:
```bash
./intcoin-cli getbestblockhash
```

```json
{
  "result": "00000000000000000007878ec04bb2b2e12317804810f4c26033585b3f81ffaa"
}
```

---

#### `getblockhash`

Returns the hash of the block at the specified height.

**Parameters**:
- `height` (number): Block height

**Returns**: `string` - Block hash (hex)

**Example**:
```bash
./intcoin-cli getblockhash 100
```

```json
{
  "result": "00000000009e2958c15ff9290d571bf9459e93b19765c6801ddeccadbb160a1e"
}
```

---

#### `getblock`

Returns information about a block.

**Parameters**:
- `hash` (string): Block hash (hex)
- `verbosity` (number, optional): 0 = hex, 1 = JSON, 2 = JSON with tx details (default: 1)

**Returns**: `object|string` - Block information or hex

**Example**:
```bash
./intcoin-cli getblock "00000000000000000007878ec04bb2b2e12317804810f4c26033585b3f81ffaa"
```

```json
{
  "result": {
    "hash": "00000000000000000007878ec04bb2b2e12317804810f4c26033585b3f81ffaa",
    "height": 12345,
    "version": 1,
    "merkleroot": "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b",
    "time": 1701648000,
    "nonce": 123456789,
    "bits": "1d00ffff",
    "difficulty": 1.0,
    "previousblockhash": "0000000000000000000123...",
    "tx": ["txid1", "txid2", ...],
    "size": 1234,
    "weight": 4936
  }
}
```

---

#### `getblockheader`

Returns block header information.

**Parameters**:
- `hash` (string): Block hash (hex)
- `verbose` (boolean, optional): true = JSON, false = hex (default: true)

**Returns**: `object|string` - Block header

---

#### `getblockchaininfo`

Returns information about the blockchain.

**Parameters**: None

**Returns**: `object` - Blockchain information

**Example**:
```bash
./intcoin-cli getblockchaininfo
```

```json
{
  "result": {
    "chain": "main",
    "blocks": 12345,
    "headers": 12345,
    "bestblockhash": "00000000000...",
    "difficulty": 1234567.89,
    "mediantime": 1701648000,
    "verificationprogress": 0.9999,
    "initialblockdownload": false,
    "chainwork": "0000000000000000000000000000000000000000000000000000000000100010",
    "size_on_disk": 12345678901,
    "pruned": false
  }
}
```

---

#### `gettxout`

Returns details about an unspent transaction output (UTXO).

**Parameters**:
- `txid` (string): Transaction ID
- `n` (number): Output index
- `include_mempool` (boolean, optional): Include mempool (default: true)

**Returns**: `object|null` - UTXO details or null if spent

---

#### `getchaintxstats`

Returns statistics about the blockchain.

**Parameters**:
- `nblocks` (number, optional): Number of blocks to analyze (default: 1 month)
- `blockhash` (string, optional): Block hash to start from

**Returns**: `object` - Transaction statistics

---

#### `getdifficulty`

Returns the current proof-of-work difficulty.

**Parameters**: None

**Returns**: `number` - Difficulty

---

#### `getmempoolinfo`

Returns information about the memory pool.

**Parameters**: None

**Returns**: `object` - Mempool information

**Example**:
```bash
./intcoin-cli getmempoolinfo
```

```json
{
  "result": {
    "size": 123,
    "bytes": 45678,
    "usage": 234567,
    "maxmempool": 104857600,
    "mempoolminfee": 0.00001
  }
}
```

---

#### `getrawmempool`

Returns all transaction IDs in the memory pool.

**Parameters**:
- `verbose` (boolean, optional): true = JSON with details, false = array of txids (default: false)

**Returns**: `array|object` - Transaction IDs or details

---

### Network Methods

#### `getnetworkinfo`

Returns information about the P2P network.

**Parameters**: None

**Returns**: `object` - Network information

**Example**:
```bash
./intcoin-cli getnetworkinfo
```

```json
{
  "result": {
    "version": 100000,
    "subversion": "/INTcoin:1.0.0/",
    "protocolversion": 70015,
    "localservices": "0000000000000001",
    "localrelay": true,
    "timeoffset": 0,
    "networkactive": true,
    "connections": 8,
    "networks": [
      {
        "name": "ipv4",
        "limited": false,
        "reachable": true,
        "proxy": "",
        "proxy_randomize_credentials": false
      }
    ],
    "relayfee": 0.00001000,
    "incrementalfee": 0.00001000,
    "warnings": ""
  }
}
```

---

#### `getpeerinfo`

Returns information about connected peers.

**Parameters**: None

**Returns**: `array` - Array of peer objects

**Example**:
```bash
./intcoin-cli getpeerinfo
```

```json
{
  "result": [
    {
      "id": 1,
      "addr": "192.168.1.100:2210",
      "addrlocal": "192.168.1.1:50123",
      "services": "0000000000000001",
      "lastsend": 1701648000,
      "lastrecv": 1701648001,
      "bytessent": 12345,
      "bytesrecv": 54321,
      "conntime": 1701640000,
      "timeoffset": 0,
      "pingtime": 0.012,
      "version": 70015,
      "subver": "/INTcoin:1.0.0/",
      "inbound": false,
      "banscore": 0,
      "synced_headers": 12345,
      "synced_blocks": 12345
    }
  ]
}
```

---

#### `getconnectioncount`

Returns the number of connections to other nodes.

**Parameters**: None

**Returns**: `number` - Connection count

---

#### `addnode`

Manually add or remove a node.

**Parameters**:
- `node` (string): Node address (IP:port)
- `command` (string): "add", "remove", or "onetry"

**Returns**: `null`

**Example**:
```bash
./intcoin-cli addnode "192.168.1.100:2210" "add"
```

---

#### `disconnectnode`

Disconnect from a node.

**Parameters**:
- `address` (string, optional): Node IP address
- `nodeid` (number, optional): Node ID

**Returns**: `null`

---

#### `getaddednodeinfo`

Returns information about manually added nodes.

**Parameters**:
- `node` (string, optional): Specific node address

**Returns**: `array` - Added node information

---

#### `setban`

Ban or unban an IP address.

**Parameters**:
- `subnet` (string): IP/netmask (e.g., "192.168.1.0/24")
- `command` (string): "add" or "remove"
- `bantime` (number, optional): Ban duration in seconds (default: 24h)
- `absolute` (boolean, optional): If false, ban duration is relative to now

**Returns**: `null`

**Example**:
```bash
./intcoin-cli setban "192.168.1.100" "add" 86400
```

---

#### `listbanned`

Returns all banned IPs/subnets.

**Parameters**: None

**Returns**: `array` - Banned addresses

---

#### `clearbanned`

Clear all banned IPs.

**Parameters**: None

**Returns**: `null`

---

### Mining Methods

#### `getmininginfo`

Returns mining-related information.

**Parameters**: None

**Returns**: `object` - Mining information

**Example**:
```bash
./intcoin-cli getmininginfo
```

```json
{
  "result": {
    "blocks": 12345,
    "currentblocksize": 1234,
    "currentblockweight": 4936,
    "difficulty": 1234567.89,
    "networkhashps": 123456789012345,
    "pooledtx": 123,
    "chain": "main",
    "warnings": ""
  }
}
```

---

#### `getblocktemplate`

Returns a block template for mining.

**Parameters**:
- `template_request` (object, optional): Template request options

**Returns**: `object` - Block template

---

#### `submitblock`

Submit a mined block.

**Parameters**:
- `hexdata` (string): Block data (hex)

**Returns**: `null|string` - null on success, error string on failure

---

#### `generatetoaddress`

Mine blocks immediately to specified address (testnet/regtest only).

**Parameters**:
- `nblocks` (number): Number of blocks to mine
- `address` (string): Address to receive block rewards

**Returns**: `array` - Array of block hashes

**Example**:
```bash
./intcoin-cli -testnet generatetoaddress 10 "int1qxyz..."
```

---

### Raw Transaction Methods

#### `getrawtransaction`

Returns raw transaction data.

**Parameters**:
- `txid` (string): Transaction ID
- `verbose` (boolean, optional): true = JSON, false = hex (default: false)

**Returns**: `string|object` - Transaction hex or details

---

#### `decoderawtransaction`

Decode a raw transaction hex.

**Parameters**:
- `hexstring` (string): Transaction hex

**Returns**: `object` - Decoded transaction

---

#### `createrawtransaction`

Create a raw transaction.

**Parameters**:
- `inputs` (array): Array of input objects
- `outputs` (object): Output addresses and amounts

**Returns**: `string` - Transaction hex

**Example**:
```bash
./intcoin-cli createrawtransaction '[{"txid":"abc...","vout":0}]' '{"int1qxyz...":10.5}'
```

---

#### `signrawtransaction`

Sign a raw transaction.

**Parameters**:
- `hexstring` (string): Transaction hex
- `prevtxs` (array, optional): Previous transaction outputs
- `privkeys` (array, optional): Private keys

**Returns**: `object` - Signed transaction

---

#### `sendrawtransaction`

Broadcast a raw transaction.

**Parameters**:
- `hexstring` (string): Signed transaction hex

**Returns**: `string` - Transaction ID

---

### Utility Methods

#### `help`

List all commands or get help for a specific command.

**Parameters**:
- `command` (string, optional): Command name

**Returns**: `string` - Help text

**Example**:
```bash
./intcoin-cli help
./intcoin-cli help getblock
```

---

#### `uptime`

Returns the daemon uptime in seconds.

**Parameters**: None

**Returns**: `number` - Uptime in seconds

---

#### `getinfo`

Returns general information about the node.

**Parameters**: None

**Returns**: `object` - Node information (deprecated, use specialized methods)

---

#### `validateaddress`

Validate an INTcoin address.

**Parameters**:
- `address` (string): INTcoin address

**Returns**: `object` - Validation result

**Example**:
```bash
./intcoin-cli validateaddress "int1qxyz..."
```

```json
{
  "result": {
    "isvalid": true,
    "address": "int1qxyz...",
    "scriptPubKey": "0014...",
    "iswitness": true,
    "witness_version": 0,
    "witness_program": "..."
  }
}
```

---

#### `verifymessage`

Verify a signed message.

**Parameters**:
- `address` (string): INTcoin address
- `signature` (string): Message signature
- `message` (string): Message text

**Returns**: `boolean` - Verification result

---

### Fee Estimation Methods

#### `estimatesmartfee`

Estimate the fee rate needed for a transaction to be confirmed within a target number of blocks.

**Parameters**:
- `conf_target` (number): Number of blocks for confirmation (1-1000)
- `estimate_mode` (string, optional): "ECONOMICAL" or "CONSERVATIVE" (default: "CONSERVATIVE")

**Returns**: `object` - Fee estimation

**Example**:
```bash
./intcoin-cli estimatesmartfee 6
```

```json
{
  "result": {
    "feerate": 0.00001000,
    "blocks": 6
  }
}
```

---

#### `estimaterawfee`

Estimate the fee rate for raw transactions with detailed statistics.

**Parameters**:
- `conf_target` (number): Number of blocks for confirmation
- `threshold` (number, optional): Success probability threshold (0-1, default: 0.95)

**Returns**: `object` - Detailed fee estimation with statistics

---

#### `estimatefee`

**DEPRECATED** - Use `estimatesmartfee` instead.

Estimate fee rate for transactions (legacy method for Bitcoin compatibility).

**Parameters**:
- `nblocks` (number): Number of blocks

**Returns**: `number` - Fee rate in INTS/kB

---

### Lightning Network Methods

#### `lightning_openchannel`

Open a new Lightning Network payment channel with a remote node.

**Parameters**:
- `node_id` (string): Remote node public key (hex, 1952 bytes for Dilithium3)
- `capacity` (number): Channel capacity in INTS
- `push_amount` (number, optional): Amount to push to remote (default: 0)

**Returns**: `object` - Channel information

**Example**:
```bash
./intcoin-cli lightning_openchannel "03abc..." 10000000
```

```json
{
  "result": {
    "channel_id": "a1b2c3d4...",
    "capacity": 10000000,
    "state": "opening"
  }
}
```

---

#### `lightning_closechannel`

Close an existing Lightning Network channel.

**Parameters**:
- `channel_id` (string): Channel ID (hex)
- `force` (boolean, optional): Force close without cooperation (default: false)

**Returns**: `object` - Closing transaction information

**Example**:
```bash
./intcoin-cli lightning_closechannel "a1b2c3d4..." false
```

```json
{
  "result": {
    "txid": "e5f6g7h8...",
    "type": "mutual_close"
  }
}
```

---

#### `lightning_sendpayment`

Send a Lightning Network payment via invoice or directly to a node.

**Parameters**:
- `invoice_or_node_id` (string): BOLT #11 invoice or destination node public key
- `amount` (number, optional): Amount in INTS (required if using node_id)

**Returns**: `object` - Payment result

**Example**:
```bash
# Via invoice
./intcoin-cli lightning_sendpayment "lnint1..."

# Direct payment
./intcoin-cli lightning_sendpayment "03xyz..." 1000000
```

```json
{
  "result": {
    "payment_hash": "hash123...",
    "amount": 1000000,
    "fee": 1000,
    "status": "succeeded"
  }
}
```

---

#### `lightning_createinvoice`

Create a BOLT #11 Lightning invoice for receiving payments.

**Parameters**:
- `amount` (number): Amount in INTS
- `description` (string): Payment description
- `expiry` (number, optional): Expiry time in seconds (default: 3600)

**Returns**: `object` - Invoice information

**Example**:
```bash
./intcoin-cli lightning_createinvoice 500000 "Payment for services"
```

```json
{
  "result": {
    "bolt11": "lnint1...",
    "payment_hash": "hash456...",
    "amount": 500000,
    "description": "Payment for services",
    "expiry": 3600
  }
}
```

---

#### `lightning_listchannels`

List all Lightning Network channels.

**Parameters**: None

**Returns**: `array` - List of channels

**Example**:
```bash
./intcoin-cli lightning_listchannels
```

```json
{
  "result": [
    {
      "channel_id": "a1b2c3...",
      "remote_node": "03def...",
      "capacity": 10000000,
      "local_balance": 6000000,
      "remote_balance": 4000000,
      "state": 2,
      "active": true
    }
  ]
}
```

---

#### `lightning_getnodeinfo`

Get information about the local Lightning node.

**Parameters**: None

**Returns**: `object` - Node information and statistics

**Example**:
```bash
./intcoin-cli lightning_getnodeinfo
```

```json
{
  "result": {
    "node_id": "03abc...",
    "alias": "MyLightningNode",
    "running": true,
    "num_channels": 5,
    "num_active_channels": 4,
    "total_capacity": 50000000,
    "local_balance": 30000000,
    "remote_balance": 20000000,
    "num_pending_htlcs": 2,
    "num_payments_sent": 150,
    "num_payments_received": 200,
    "total_fees_earned": 50000,
    "total_fees_paid": 25000
  }
}
```

---

#### `lightning_getnetworkgraph`

Get the Lightning Network graph (nodes and channels).

**Parameters**: None

**Returns**: `object` - Network graph information

**Example**:
```bash
./intcoin-cli lightning_getnetworkgraph
```

```json
{
  "result": {
    "num_nodes": 0,
    "num_channels": 0,
    "nodes": [],
    "channels": []
  }
}
```

---

### Mining Pool Methods

#### `pool_getstats`

Get comprehensive mining pool statistics.

**Parameters**: None

**Returns**: `object` - Pool statistics

**Example**:
```bash
./intcoin-cli pool_getstats
```

```json
{
  "result": {
    "network_height": 12345,
    "network_difficulty": 1500000,
    "network_hashrate": 50000000,
    "active_miners": 25,
    "active_workers": 100,
    "total_connections": 100,
    "pool_hashrate": 5000000,
    "pool_hashrate_percentage": 10.0,
    "shares_this_round": 1000,
    "shares_last_hour": 5000,
    "shares_last_day": 120000,
    "total_shares": 10000000,
    "blocks_found": 50,
    "blocks_pending": 2,
    "blocks_confirmed": 48,
    "blocks_orphaned": 2,
    "average_block_time": 600.0,
    "total_paid": 50000000,
    "total_unpaid": 500000,
    "pool_revenue": 250000,
    "uptime_hours": 720.5,
    "efficiency": 98.5,
    "luck": 105.2
  }
}
```

---

#### `pool_getworkers`

List active workers with detailed statistics.

**Parameters**:
- `miner_id` (number, optional): Filter by specific miner ID

**Returns**: `array` - List of workers

**Example**:
```bash
./intcoin-cli pool_getworkers
./intcoin-cli pool_getworkers 42
```

```json
{
  "result": [
    {
      "worker_id": 1,
      "miner_id": 42,
      "worker_name": "rig1",
      "user_agent": "randomx-miner/1.0",
      "shares_submitted": 1000,
      "shares_accepted": 980,
      "shares_rejected": 15,
      "shares_stale": 5,
      "blocks_found": 2,
      "current_hashrate": 50000,
      "average_hashrate": 48000,
      "current_difficulty": 1000,
      "ip_address": "192.168.1.100",
      "is_active": true
    }
  ]
}
```

---

#### `pool_getpayments`

Get payment history with optional filtering and pagination.

**Parameters**:
- `miner_id` (number, optional): Filter by specific miner ID
- `limit` (number, optional): Maximum number of results (default: 100)

**Returns**: `array` - List of payments

**Example**:
```bash
./intcoin-cli pool_getpayments
./intcoin-cli pool_getpayments 42 50
```

```json
{
  "result": [
    {
      "payment_id": 1,
      "miner_id": 42,
      "payout_address": "int1q...",
      "amount": 1000000,
      "tx_hash": "abc123...",
      "status": "confirmed",
      "is_confirmed": true
    }
  ]
}
```

---

#### `pool_gettopminers`

Get top miners ranked by hashrate.

**Parameters**:
- `limit` (number, optional): Maximum number of results (default: 100)

**Returns**: `array` - List of top miners

**Example**:
```bash
./intcoin-cli pool_gettopminers 10
```

```json
{
  "result": [
    {
      "miner_id": 42,
      "username": "alice",
      "payout_address": "int1q...",
      "total_hashrate": 250000,
      "total_shares_submitted": 10000,
      "total_shares_accepted": 9800,
      "total_blocks_found": 5,
      "unpaid_balance": 50000,
      "paid_balance": 5000000,
      "active_workers": 5
    }
  ]
}
```

---

### Enhanced Blockchain Methods

#### `getblockstats`

Get comprehensive statistics for a specific block.

**Parameters**:
- `hash_or_height` (string|number): Block hash or height

**Returns**: `object` - Block statistics

**Example**:
```bash
./intcoin-cli getblockstats 100
./intcoin-cli getblockstats "00000000009e2958..."
```

```json
{
  "result": {
    "blockhash": "00000000009e2958...",
    "time": 1609459200,
    "txs": 150,
    "total_out": 5000000000,
    "total_size": 250000,
    "avgtxsize": 1666,
    "feerate_percentiles": [10, 25, 50, 75, 100],
    "totalfee": 15000,
    "avgfee": 100,
    "avgfeerate": 60
  }
}
```

---

#### `getrawmempool` (Enhanced)

Get all transactions in the mempool with optional verbose output.

**Parameters**:
- `verbose` (boolean, optional): Return detailed transaction info (default: false)

**Returns**: `array|object` - Transaction IDs or detailed transaction information

**Example**:
```bash
# Simple (transaction IDs only)
./intcoin-cli getrawmempool

# Verbose (with details)
./intcoin-cli getrawmempool true
```

```json
{
  "result": {
    "txid1": {
      "size": 250,
      "fee": 1000,
      "time": 1609459200,
      "height": 12345
    }
  }
}
```

---

#### `gettxoutsetinfo`

Get statistics about the UTXO set.

**Parameters**: None

**Returns**: `object` - UTXO set statistics

**Example**:
```bash
./intcoin-cli gettxoutsetinfo
```

```json
{
  "result": {
    "height": 12345,
    "bestblock": "00000000009e2958...",
    "txouts": 1500000,
    "bogosize": 150000000,
    "hash_serialized_2": "abc123...",
    "total_amount": 21000000000000
  }
}
```

---

## Examples

### Python Example

```python
import requests
import json

class INTcoinRPC:
    def __init__(self, url='http://127.0.0.1:2211', user='', password=''):
        self.url = url
        self.auth = (user, password) if user else None

    def call(self, method, params=[]):
        payload = {
            'jsonrpc': '2.0',
            'id': 'python',
            'method': method,
            'params': params
        }
        response = requests.post(self.url, json=payload, auth=self.auth)
        return response.json()

# Usage
rpc = INTcoinRPC(user='myuser', password='mypass')

# Get block count
result = rpc.call('getblockcount')
print(f"Block height: {result['result']}")

# Get best block hash
result = rpc.call('getbestblockhash')
print(f"Best block: {result['result']}")

# Get block info
result = rpc.call('getblock', [result['result']])
print(f"Block info: {json.dumps(result['result'], indent=2)}")
```

### JavaScript/Node.js Example

```javascript
const axios = require('axios');

class INTcoinRPC {
    constructor(url = 'http://127.0.0.1:2211', user = '', password = '') {
        this.url = url;
        this.auth = user ? {
            username: user,
            password: password
        } : null;
    }

    async call(method, params = []) {
        const response = await axios.post(this.url, {
            jsonrpc: '2.0',
            id: 'nodejs',
            method: method,
            params: params
        }, {
            auth: this.auth
        });
        return response.data;
    }
}

// Usage
(async () => {
    const rpc = new INTcoinRPC('http://127.0.0.1:2211', 'myuser', 'mypass');

    // Get block count
    const count = await rpc.call('getblockcount');
    console.log(`Block height: ${count.result}`);

    // Get peer info
    const peers = await rpc.call('getpeerinfo');
    console.log(`Connected peers: ${peers.result.length}`);
})();
```

### cURL Example

```bash
# Get block count
curl --user myuser:mypass --data-binary '{"jsonrpc":"2.0","id":"curl","method":"getblockcount","params":[]}' -H 'content-type: application/json;' http://127.0.0.1:2211/

# Get best block hash
curl --user myuser:mypass --data-binary '{"jsonrpc":"2.0","id":"curl","method":"getbestblockhash","params":[]}' -H 'content-type: application/json;' http://127.0.0.1:2211/

# Get block
curl --user myuser:mypass --data-binary '{"jsonrpc":"2.0","id":"curl","method":"getblock","params":["blockhash"]}' -H 'content-type: application/json;' http://127.0.0.1:2211/
```

---

## Enhanced Mempool Methods

*New in v1.2.0-beta*

### getmempoolinfo

Get detailed mempool statistics including priority breakdown.

**Syntax**:
```bash
intcoin-cli getmempoolinfo
```

**Response**:
```json
{
  "size": 1523,
  "bytes": 456789,
  "usage": 1234567,
  "maxmempool": 314572800,
  "mempoolminfee": 0.00001000,
  "minrelaytxfee": 0.00001000,
  "priority_stats": {
    "LOW": {"count": 100, "bytes": 25000},
    "NORMAL": {"count": 1200, "bytes": 400000},
    "HIGH": {"count": 200, "bytes": 30000},
    "HTLC": {"count": 15, "bytes": 1500},
    "BRIDGE": {"count": 7, "bytes": 289},
    "CRITICAL": {"count": 1, "bytes": 250}
  },
  "total_fees": "15.5"
}
```

### getmempoolentry

Get mempool entry for a specific transaction.

**Syntax**:
```bash
intcoin-cli getmempoolentry <txid>
```

**Parameters**:
- `txid` (string, required): Transaction ID

**Response**:
```json
{
  "fees": {
    "base": 0.00050000,
    "modified": 0.00050000,
    "ancestor": 0.00050000,
    "descendant": 0.00050000
  },
  "size": 250,
  "fee": 0.00050000,
  "modifiedfee": 0.00050000,
  "time": 1704153600,
  "height": 150000,
  "priority": "NORMAL",
  "depends": [],
  "spentby": [],
  "bip125-replaceable": false
}
```

### savemempool

Manually save mempool to disk.

**Syntax**:
```bash
intcoin-cli savemempool
```

**Response**:
```json
{
  "result": "Mempool saved to disk",
  "file": "/home/user/.intcoin/mempool.dat",
  "transactions": 1523
}
```

### setmempoolpriority

Set priority for a transaction (requires authorization).

**Syntax**:
```bash
intcoin-cli setmempoolpriority <txid> <priority>
```

**Parameters**:
- `txid` (string, required): Transaction ID
- `priority` (string, required): LOW, NORMAL, HIGH, HTLC, BRIDGE, CRITICAL

**Response**:
```json
{
  "result": "Priority updated",
  "txid": "abc123...",
  "old_priority": "NORMAL",
  "new_priority": "HIGH"
}
```

---

## Metrics and Monitoring Methods

*New in v1.2.0-beta*

### getmetricsinfo

Get Prometheus metrics endpoint information.

**Syntax**:
```bash
intcoin-cli getmetricsinfo
```

**Response**:
```json
{
  "enabled": true,
  "bind_address": "127.0.0.1",
  "port": 9090,
  "threads": 2,
  "endpoint": "http://127.0.0.1:9090/metrics",
  "uptime": 3600,
  "requests_total": 245
}
```

### getprometheusmetrics

Get Prometheus metrics in text exposition format.

**Syntax**:
```bash
intcoin-cli getprometheusmetrics
```

**Response**: Prometheus text format (truncated example)
```
# HELP intcoin_blocks_processed_total Total number of blocks processed
# TYPE intcoin_blocks_processed_total counter
intcoin_blocks_processed_total 150234.00

# HELP intcoin_blockchain_height Current blockchain height
# TYPE intcoin_blockchain_height gauge
intcoin_blockchain_height 150234.00

# HELP intcoin_mempool_size Current mempool transaction count
# TYPE intcoin_mempool_size gauge
intcoin_mempool_size 1523.00
...
```

---

## Atomic Swap Methods

*New in v1.2.0-beta*

### initiate_atomic_swap

Initiate an atomic swap (creates HTLC on chain A).

**Syntax**:
```bash
intcoin-cli initiate_atomic_swap <amount> <recipient_address> <refund_address> <secret_hash> <locktime>
```

**Parameters**:
- `amount` (numeric, required): Amount to swap
- `recipient_address` (string, required): Counterparty's address
- `refund_address` (string, required): Your refund address
- `secret_hash` (string, required): SHA256 hash of secret
- `locktime` (numeric, required): Refund locktime (Unix timestamp)

**Response**:
```json
{
  "txid": "abc123...",
  "htlc_address": "int1qhtlc...",
  "secret_hash": "def456...",
  "locktime": 1704240000,
  "amount": 10.5,
  "status": "initiated"
}
```

### participate_atomic_swap

Participate in atomic swap (creates HTLC on chain B).

**Syntax**:
```bash
intcoin-cli participate_atomic_swap <amount> <recipient_address> <refund_address> <secret_hash> <locktime>
```

**Response**: Same format as `initiate_atomic_swap`

### redeem_atomic_swap

Redeem atomic swap by revealing secret.

**Syntax**:
```bash
intcoin-cli redeem_atomic_swap <htlc_address> <secret>
```

**Parameters**:
- `htlc_address` (string, required): HTLC contract address
- `secret` (string, required): Preimage of secret_hash

**Response**:
```json
{
  "txid": "ghi789...",
  "secret": "jkl012...",
  "amount": 10.5,
  "status": "redeemed"
}
```

### refund_atomic_swap

Refund atomic swap after locktime expires.

**Syntax**:
```bash
intcoin-cli refund_atomic_swap <htlc_address>
```

**Response**:
```json
{
  "txid": "mno345...",
  "amount": 10.5,
  "status": "refunded"
}
```

### get_atomic_swap_status

Get status of an atomic swap.

**Syntax**:
```bash
intcoin-cli get_atomic_swap_status <htlc_address>
```

**Response**:
```json
{
  "htlc_address": "int1qhtlc...",
  "status": "initiated",
  "amount": 10.5,
  "locktime": 1704240000,
  "secret_hash": "def456...",
  "confirmations": 3
}
```

---

## Cross-Chain Bridge Methods

*New in v1.2.0-beta*

### bridge_deposit

Deposit INT tokens to cross-chain bridge.

**Syntax**:
```bash
intcoin-cli bridge_deposit <amount> <destination_chain> <destination_address>
```

**Parameters**:
- `amount` (numeric, required): Amount to bridge
- `destination_chain` (string, required): ethereum, bitcoin, bsc
- `destination_address` (string, required): Address on destination chain

**Response**:
```json
{
  "deposit_txid": "pqr678...",
  "bridge_id": "BRG-123456",
  "amount": 100.0,
  "destination_chain": "ethereum",
  "destination_address": "0xabc...",
  "estimated_time": "10-30 minutes",
  "status": "pending"
}
```

### bridge_withdraw

Withdraw tokens from bridge back to INT.

**Syntax**:
```bash
intcoin-cli bridge_withdraw <bridge_id> <int_address>
```

**Parameters**:
- `bridge_id` (string, required): Bridge transaction ID
- `int_address` (string, required): INT address to receive tokens

**Response**:
```json
{
  "withdrawal_txid": "stu901...",
  "bridge_id": "BRG-123456",
  "amount": 100.0,
  "int_address": "int1q...",
  "status": "processing"
}
```

### bridge_status

Get status of bridge transaction.

**Syntax**:
```bash
intcoin-cli bridge_status <bridge_id>
```

**Response**:
```json
{
  "bridge_id": "BRG-123456",
  "type": "deposit",
  "source_chain": "intcoin",
  "destination_chain": "ethereum",
  "amount": 100.0,
  "status": "completed",
  "confirmations": {
    "source": 50,
    "destination": 12
  },
  "txids": {
    "source": "pqr678...",
    "destination": "0xdef..."
  }
}
```

### list_bridges

List all bridge transactions for current wallet.

**Syntax**:
```bash
intcoin-cli list_bridges [count] [skip]
```

**Response**:
```json
[
  {
    "bridge_id": "BRG-123456",
    "type": "deposit",
    "amount": 100.0,
    "destination_chain": "ethereum",
    "status": "completed",
    "timestamp": 1704153600
  },
  ...
]
```

---

## SPV and Bloom Filter Methods

*New in v1.2.0-beta*

### loadbloomfilter

Load Bloom filter for SPV client.

**Syntax**:
```bash
intcoin-cli loadbloomfilter <filter_data> <num_hash_funcs> <tweak> <flags>
```

**Parameters**:
- `filter_data` (string, required): Hex-encoded filter data
- `num_hash_funcs` (numeric, required): Number of hash functions
- `tweak` (numeric, required): Randomization parameter
- `flags` (numeric, required): Update flags (0=NONE, 1=ALL, 2=P2PUBKEY_ONLY)

**Response**:
```json
{
  "result": "Bloom filter loaded",
  "size": 180,
  "hash_funcs": 10,
  "false_positive_rate": 0.001
}
```

### addtobloomfilter

Add element to existing Bloom filter.

**Syntax**:
```bash
intcoin-cli addtobloomfilter <element>
```

**Parameters**:
- `element` (string, required): Hex-encoded element to add

**Response**:
```json
{
  "result": "Element added to Bloom filter"
}
```

### clearbloomfilter

Clear loaded Bloom filter.

**Syntax**:
```bash
intcoin-cli clearbloomfilter
```

**Response**:
```json
{
  "result": "Bloom filter cleared"
}
```

### getheaders

Get block headers for SPV synchronization.

**Syntax**:
```bash
intcoin-cli getheaders <start_height> [count]
```

**Parameters**:
- `start_height` (numeric, required): Starting block height
- `count` (numeric, optional, default=2000): Number of headers

**Response**:
```json
{
  "headers": [
    {
      "height": 150000,
      "hash": "abc123...",
      "version": 1,
      "prev_hash": "def456...",
      "merkle_root": "ghi789...",
      "timestamp": 1704153600,
      "bits": "1f0ff0f0",
      "nonce": 12345678
    },
    ...
  ],
  "count": 2000
}
```

### getmerkleproof

Get merkle proof for a transaction.

**Syntax**:
```bash
intcoin-cli getmerkleproof <txid>
```

**Parameters**:
- `txid` (string, required): Transaction ID

**Response**:
```json
{
  "txid": "jkl012...",
  "block_hash": "mno345...",
  "merkle_root": "pqr678...",
  "proof": ["stu901...", "vwx234...", "yza567..."],
  "position": 42,
  "total_transactions": 1000
}
```

---

## See Also

- [CLI Guide](../wiki/CLI-Guide.md) - intcoin-cli usage guide
- [Running intcoind](../wiki/Running-intcoind.md) - Daemon setup guide
- [Developer Hub](../wiki/Developers.md) - Complete developer documentation

---

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Build**: Complete - 70+ RPC methods implemented (includes Lightning, Pool, Fee Estimation, Mempool Priority, Metrics, Atomic Swaps, Bridges, and SPV)
