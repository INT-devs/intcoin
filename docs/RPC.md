# INTcoin RPC API Documentation

**Version**: 1.0.0-alpha
**Last Updated**: December 3, 2025

This document describes the JSON-RPC API for interacting with the INTcoin daemon (`intcoind`).

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

## See Also

- [CLI Guide](../wiki/CLI-Guide.md) - intcoin-cli usage guide
- [Running intcoind](../wiki/Running-intcoind.md) - Daemon setup guide
- [Developer Hub](../wiki/Developers.md) - Complete developer documentation

---

**Version**: 1.0.0-alpha
**Last Updated**: December 3, 2025
**Build**: Complete - 32+ RPC methods implemented
