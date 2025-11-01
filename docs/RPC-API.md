# INTcoin RPC API Documentation

**Version**: 0.2.0-alpha
**Protocol**: JSON-RPC 2.0
**Default Port**: 8332 (mainnet), 18332 (testnet)

---

## Table of Contents

1. [Introduction](#introduction)
2. [Authentication](#authentication)
3. [Request Format](#request-format)
4. [Response Format](#response-format)
5. [Error Codes](#error-codes)
6. [Blockchain Methods](#blockchain-methods)
7. [Wallet Methods](#wallet-methods)
8. [Mining Methods](#mining-methods)
9. [Network Methods](#network-methods)
10. [Mempool Methods](#mempool-methods)
11. [Utility Methods](#utility-methods)

---

## Introduction

The INTcoin RPC API allows you to interact with the INTcoin daemon programmatically using JSON-RPC 2.0 protocol over HTTP.

### Connecting

```bash
# Using curl
curl --user username:password --data-binary '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":"1"}' -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### Using the CLI

```bash
intcoin-cli getblockcount
intcoin-cli getnewaddress "My Address"
intcoin-cli sendtoaddress "INT1..." 10.5
```

---

## Authentication

RPC calls require authentication using HTTP Basic Auth. Configure username and password in `intcoin.conf`:

```
rpcuser=yourusername
rpcpassword=yourpassword
rpcport=8332
```

---

## Request Format

```json
{
  "jsonrpc": "2.0",
  "method": "method_name",
  "params": [param1, param2, ...],
  "id": "request_id"
}
```

---

## Response Format

### Success Response

```json
{
  "jsonrpc": "2.0",
  "result": result_value,
  "id": "request_id"
}
```

### Error Response

```json
{
  "jsonrpc": "2.0",
  "error": "error_message",
  "id": "request_id"
}
```

---

## Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -1 | Method not found | The requested method does not exist |
| -2 | Invalid params | Invalid method parameters |
| -3 | Internal error | Internal JSON-RPC error |
| -4 | Parse error | JSON parse error |
| -5 | Wallet error | Wallet-related error |
| -6 | Network error | Network-related error |

---

## Blockchain Methods

### getblockcount

Returns the current block height.

**Parameters**: None

**Returns**: `number` - The current block count

**Example**:
```bash
intcoin-cli getblockcount
```

**Response**:
```json
{
  "result": 12345
}
```

---

### getblockhash

Returns the hash of a block at the given height.

**Parameters**:
- `height` (number, required) - The block height

**Returns**: `string` - Block hash in hex

**Example**:
```bash
intcoin-cli getblockhash 1000
```

**Response**:
```json
{
  "result": "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
}
```

---

### getblock

Returns information about a block.

**Parameters**:
- `blockhash` (string, required) - The block hash

**Returns**: `object` - Block information

**Example**:
```bash
intcoin-cli getblock "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
```

**Response**:
```json
{
  "result": {
    "hash": "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048",
    "version": 1,
    "previousblockhash": "000000000000000000000000000000000000000000000000000000000000000",
    "merkleroot": "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b",
    "time": 1735689600,
    "bits": 486604799,
    "nonce": 2083236893,
    "tx": 1
  }
}
```

---

### getblockchaininfo

Returns blockchain information.

**Parameters**: None

**Returns**: `object` - Blockchain state information

**Example**:
```bash
intcoin-cli getblockchaininfo
```

**Response**:
```json
{
  "result": {
    "chain": "main",
    "blocks": 12345,
    "bestblockhash": "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048",
    "difficulty": 1.0,
    "chainwork": "0000000000000000000000000000000000000000000000000000000100010001"
  }
}
```

---

## Wallet Methods

### getnewaddress

Generates a new receiving address.

**Parameters**:
- `label` (string, optional) - Address label

**Returns**: `string` - New INTcoin address

**Example**:
```bash
intcoin-cli getnewaddress "My Savings"
```

**Response**:
```json
{
  "result": "32BjG5JeyLckPYy4m8xkWY4ZCTaR7bZwEzyicuoDpCcekxGoFKa"
}
```

---

### getbalance

Returns the wallet balance.

**Parameters**: None

**Returns**: `number` - Total balance in INT

**Example**:
```bash
intcoin-cli getbalance
```

**Response**:
```json
{
  "result": 1234.56789012
}
```

---

### sendtoaddress

Sends INT to an address.

**Parameters**:
- `address` (string, required) - Recipient address
- `amount` (number, required) - Amount to send in INT

**Returns**: `string` - Transaction ID (hex)

**Example**:
```bash
intcoin-cli sendtoaddress "32BjG5JeyLckPYy4m8xkWY4ZCTaR7bZwEzyicuoDpCcekxGoFKa" 10.5
```

**Response**:
```json
{
  "result": "3e3d8b084fbce046a9fb59b0b1c6e9cb4e53ed46b6f0e9d9f7f8e5c4d3e2a1f0"
}
```

---

### listtransactions

Lists wallet transactions.

**Parameters**: None

**Returns**: `array` - Array of transaction objects

**Example**:
```bash
intcoin-cli listtransactions
```

**Response**:
```json
{
  "result": [
    {
      "txid": "3e3d8b084fbce046a9fb59b0b1c6e9cb4e53ed46b6f0e9d9f7f8e5c4d3e2a1f0",
      "amount": 10.5,
      "confirmations": 6
    }
  ]
}
```

---

### listaddresses

Lists all wallet addresses.

**Parameters**: None

**Returns**: `array` - Array of addresses

**Example**:
```bash
intcoin-cli listaddresses
```

**Response**:
```json
{
  "result": [
    "32BjG5JeyLckPYy4m8xkWY4ZCTaR7bZwEzyicuoDpCcekxGoFKa",
    "4jKhFuCHS37JGPc2zhci9te3uGz3GWYbNbbD4YswWcxRviRDfBM"
  ]
}
```

---

## Mining Methods

### getmininginfo

Returns mining information.

**Parameters**: None

**Returns**: `object` - Mining statistics

**Example**:
```bash
intcoin-cli getmininginfo
```

**Response**:
```json
{
  "result": {
    "mining": true,
    "hashrate": 125000,
    "blocks": 5,
    "difficulty": 1
  }
}
```

---

### startmining

Starts CPU mining.

**Parameters**:
- `threads` (number, optional) - Number of threads (0 = auto-detect)

**Returns**: `boolean` - Success status

**Example**:
```bash
intcoin-cli startmining 4
```

**Response**:
```json
{
  "result": true
}
```

---

### stopmining

Stops CPU mining.

**Parameters**: None

**Returns**: `boolean` - Success status

**Example**:
```bash
intcoin-cli stopmining
```

**Response**:
```json
{
  "result": true
}
```

---

## Network Methods

### getpeerinfo

Returns information about connected peers.

**Parameters**: None

**Returns**: `array` - Array of peer objects

**Example**:
```bash
intcoin-cli getpeerinfo
```

**Response**:
```json
{
  "result": [
    {
      "addr": "192.168.1.100:8333",
      "services": 1
    }
  ]
}
```

---

### getnetworkinfo

Returns network information.

**Parameters**: None

**Returns**: `object` - Network state

**Example**:
```bash
intcoin-cli getnetworkinfo
```

**Response**:
```json
{
  "result": {
    "version": 1,
    "connections": 8,
    "networkactive": true
  }
}
```

---

### addnode

Adds a node to connect to.

**Parameters**:
- `node` (string, required) - Node address (IP:port)

**Returns**: `boolean` - Success status

**Example**:
```bash
intcoin-cli addnode "192.168.1.100:8333"
```

**Response**:
```json
{
  "result": true
}
```

---

## Mempool Methods

### getmempoolinfo

Returns mempool information.

**Parameters**: None

**Returns**: `object` - Mempool statistics

**Example**:
```bash
intcoin-cli getmempoolinfo
```

**Response**:
```json
{
  "result": {
    "size": 150,
    "bytes": 45000,
    "usage": 45000,
    "total_fee": 0.0015
  }
}
```

---

### getrawmempool

Returns all transaction IDs in the mempool.

**Parameters**: None

**Returns**: `array` - Array of transaction IDs

**Example**:
```bash
intcoin-cli getrawmempool
```

**Response**:
```json
{
  "result": [
    "3e3d8b084fbce046a9fb59b0b1c6e9cb4e53ed46b6f0e9d9f7f8e5c4d3e2a1f0",
    "7f8e5c4d3e2a1f0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3a4b"
  ]
}
```

---

## Utility Methods

### help

Lists all available RPC commands.

**Parameters**: None

**Returns**: `array` - Array of command names

**Example**:
```bash
intcoin-cli help
```

**Response**:
```json
{
  "result": [
    "getblockcount",
    "getblockhash",
    "getblock",
    "getnewaddress",
    "getbalance",
    "sendtoaddress",
    "getmininginfo",
    "startmining",
    "stopmining",
    "getpeerinfo",
    "help",
    "stop"
  ]
}
```

---

### stop

Stops the INTcoin daemon.

**Parameters**: None

**Returns**: `string` - Shutdown message

**Example**:
```bash
intcoin-cli stop
```

**Response**:
```json
{
  "result": "Server stopping"
}
```

---

## Programming Examples

### Python

```python
import requests
import json

def rpc_call(method, params=[]):
    url = "http://127.0.0.1:8332/"
    headers = {'content-type': 'application/json'}
    payload = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": "1"
    }

    response = requests.post(
        url,
        data=json.dumps(payload),
        headers=headers,
        auth=('username', 'password')
    )

    return response.json()['result']

# Examples
height = rpc_call('getblockcount')
balance = rpc_call('getbalance')
address = rpc_call('getnewaddress', ['My Address'])
```

### JavaScript (Node.js)

```javascript
const axios = require('axios');

async function rpcCall(method, params = []) {
    const response = await axios.post('http://127.0.0.1:8332/', {
        jsonrpc: '2.0',
        method: method,
        params: params,
        id: '1'
    }, {
        auth: {
            username: 'username',
            password: 'password'
        }
    });

    return response.data.result;
}

// Examples
(async () => {
    const height = await rpcCall('getblockcount');
    const balance = await rpcCall('getbalance');
    const address = await rpcCall('getnewaddress', ['My Address']);
})();
```

### C++

```cpp
#include "intcoin/rpc.h"

using namespace intcoin::rpc;

int main() {
    Client client("127.0.0.1", 8332);
    client.connect();

    // Get block count
    Response resp = client.call("getblockcount");
    std::cout << "Block count: " << resp.result << std::endl;

    // Get new address
    resp = client.call("getnewaddress", {"My Address"});
    std::cout << "New address: " << resp.result << std::endl;

    client.disconnect();
    return 0;
}
```

---

**Copyright © 2025 INTcoin Core**
**License**: MIT
