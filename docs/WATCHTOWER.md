# INTcoin Watchtower Implementation

## Overview

Watchtowers are third-party monitoring services that provide security for Lightning Network payment channels when users are offline. They monitor the blockchain for fraudulent channel closes (breaches) and automatically broadcast penalty transactions to protect user funds.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [How It Works](#how-it-works)
- [Security Model](#security-model)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Configuration](#configuration)
- [Running a Watchtower Server](#running-a-watchtower-server)
- [Privacy Considerations](#privacy-considerations)

## Architecture

The watchtower system consists of three main components:

1. **Watchtower Client** - User-side component that encrypts and uploads breach remedies
2. **Watchtower Server** - Monitoring service that watches the blockchain and broadcasts penalties
3. **Watchtower Manager** - Coordinator for managing multiple watchtowers for redundancy

```
┌─────────────────┐         Upload Breach Remedy        ┌──────────────────┐
│                 │────────────────────────────────────>│                  │
│  Watchtower     │                                      │   Watchtower     │
│  Client         │<────────────────────────────────────│   Server         │
│  (User Side)    │         Registration Ack            │  (Monitor)       │
└─────────────────┘                                      └──────────────────┘
        │                                                         │
        │ Channel State Updates                                  │
        ▼                                                         ▼
┌─────────────────┐                                      ┌──────────────────┐
│  Lightning      │                                      │   Blockchain     │
│  Channel        │                                      │   Monitor        │
└─────────────────┘                                      └──────────────────┘
                                                                  │
                                                                  ▼ Breach Detected
                                                          ┌──────────────────┐
                                                          │  Broadcast       │
                                                          │  Penalty TX      │
                                                          └──────────────────┘
```

## How It Works

### 1. Client Registration

When a user first connects to a watchtower:

```cpp
WatchtowerClient client(user_private_key);
client.register_with_watchtower("watchtower.example.com", 9735);
```

The client sends:
- Client public key
- Timestamp
- Signature proving ownership of the key

### 2. Breach Remedy Upload

Every time a channel state updates (new commitment transaction created), the client:

1. **Creates a penalty transaction** that spends the old commitment transaction
2. **Encrypts the penalty data** using a key derived from the commitment TXID
3. **Creates a blinded hint** so the watchtower can match without seeing the TXID
4. **Uploads to watchtower** with signature

```cpp
client.upload_breach_remedy(
    "watchtower.example.com",
    9735,
    channel_id,
    old_commitment,      // The commitment being replaced
    revocation_privkey,  // Key to spend revoked commitment
    penalty_tx           // Pre-signed penalty transaction
);
```

**Key Insight**: The encryption key is the commitment TXID itself. Only when that specific transaction appears on-chain can the watchtower decrypt the remedy.

### 3. Breach Detection

The watchtower continuously monitors new blocks:

```cpp
WatchtowerServer server(9735);
server.start();

// For each new block:
server.process_block(block.transactions, block.height);
```

For each transaction:
1. Compute its TXID
2. Try to decrypt stored breach remedies using that TXID
3. If decryption succeeds → breach detected!
4. Broadcast the penalty transaction

### 4. Penalty Enforcement

When a breach is detected:

```cpp
// Watchtower decrypts the remedy using the breach TXID
auto remedy_payload = decrypt_remedy_payload(remedy, breach_txid);

// Broadcasts the penalty transaction
broadcast_penalty(remedy_payload.penalty_tx);
```

The penalty transaction:
- Spends all funds from the old commitment transaction
- Sends funds to the honest party
- Uses the revocation key that was shared via the encrypted remedy

## Security Model

### Threat Model

**What watchtowers protect against:**
- ✅ Users being offline when counterparty broadcasts old state
- ✅ Users going offline permanently after channel closure attempt
- ✅ Counterparty attempting to steal funds via old commitment

**What watchtowers do NOT protect against:**
- ❌ Counterparty stealing via current/latest commitment (that's legal!)
- ❌ Bugs in the penalty transaction construction
- ❌ Watchtower itself going offline (use multiple watchtowers!)

### Privacy Model

**What watchtowers learn:**
- Number of channels you have (channel IDs)
- When channels update (breach remedy timestamps)
- Approximate channel capacity (from remedy size)

**What watchtowers do NOT learn:**
- Actual commitment transaction details (encrypted)
- Payment amounts or destinations (encrypted)
- Who you're transacting with (blinded)
- When breaches occur (unless they actually happen)

### Encryption Scheme

The breach remedy encryption is designed such that:

1. **Encryption Key** = `SHA3-256(commitment_txid || salt)`
2. **TXID Hint** = `SHA3-256(commitment_txid || salt || "hint")`

Properties:
- Watchtower cannot decrypt without seeing the commitment TXID on-chain
- Watchtower cannot correlate remedies across channels
- Client can prove ownership with Dilithium5 signature

## API Reference

### WatchtowerClient

```cpp
class WatchtowerClient {
public:
    // Constructor
    WatchtowerClient(const DilithiumPrivKey& client_privkey);

    // Register with a watchtower
    bool register_with_watchtower(const std::string& address, uint16_t port);

    // Upload breach remedy
    bool upload_breach_remedy(
        const std::string& address,
        uint16_t port,
        const Hash256& channel_id,
        const CommitmentTransaction& commitment,
        const DilithiumPrivKey& revocation_privkey,
        const Transaction& penalty_tx
    );

    // Management
    std::vector<std::pair<std::string, uint16_t>> get_watchtowers() const;
    bool remove_watchtower(const std::string& address, uint16_t port);

    // Statistics
    size_t get_remedy_count() const;
    size_t get_watchtower_count() const;
};
```

### WatchtowerServer

```cpp
class WatchtowerServer {
public:
    // Constructor
    WatchtowerServer(uint16_t listen_port);

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Blockchain monitoring
    void process_block(const std::vector<Transaction>& transactions,
                      uint32_t block_height);

    // Statistics
    struct Stats {
        size_t registered_clients;
        size_t stored_remedies;
        size_t breaches_detected;
        size_t penalties_broadcast;
        uint64_t uptime_seconds;
    };
    Stats get_stats() const;

    // Configuration
    void set_max_clients(size_t max_clients);
    void set_max_remedies_per_client(size_t max_remedies);
};
```

### WatchtowerManager

```cpp
class WatchtowerManager {
public:
    // Constructor
    WatchtowerManager(const DilithiumPrivKey& client_privkey);

    // Watchtower management
    bool add_watchtower(const std::string& address, uint16_t port);
    bool remove_watchtower(const std::string& address, uint16_t port);

    // Upload to all watchtowers (returns success count)
    size_t upload_to_all_watchtowers(
        const Hash256& channel_id,
        const CommitmentTransaction& commitment,
        const DilithiumPrivKey& revocation_privkey,
        const Transaction& penalty_tx
    );

    // Status monitoring
    struct WatchtowerStatus {
        std::string address;
        uint16_t port;
        bool online;
        uint64_t last_contact;
        size_t remedies_uploaded;
    };
    std::vector<WatchtowerStatus> get_watchtower_status() const;

    // Statistics
    size_t get_total_watchtowers() const;
    size_t get_online_watchtowers() const;
    size_t get_total_remedies_uploaded() const;
};
```

## Usage Examples

### Client Setup

```cpp
#include "intcoin/lightning_watchtower.h"

using namespace intcoin::lightning;

// Generate client key pair
DilithiumPrivKey client_privkey = generate_dilithium_keypair();

// Create watchtower manager
WatchtowerManager manager(client_privkey);

// Add watchtowers for redundancy
manager.add_watchtower("watchtower1.intcoin.org", 9735);
manager.add_watchtower("watchtower2.intcoin.org", 9735);
manager.add_watchtower("watchtower3.intcoin.org", 9735);

std::cout << "Connected to " << manager.get_total_watchtowers()
          << " watchtowers" << std::endl;
```

### Uploading Breach Remedies

```cpp
// When creating a new commitment transaction
CommitmentTransaction new_commitment = channel.create_new_commitment(...);

// The old commitment is now revoked
CommitmentTransaction old_commitment = channel.get_previous_commitment();
DilithiumPrivKey revocation_key = channel.get_revocation_key(old_commitment);

// Create penalty transaction
Transaction penalty_tx = channel.create_penalty_transaction(
    old_commitment,
    revocation_key
);

// Upload to all watchtowers
size_t success_count = manager.upload_to_all_watchtowers(
    channel.get_id(),
    old_commitment,
    revocation_key,
    penalty_tx
);

std::cout << "Uploaded to " << success_count << " watchtowers" << std::endl;
```

### Running a Watchtower Server

```cpp
// Create watchtower server
WatchtowerServer server(9735);

// Configure
server.set_max_clients(10000);
server.set_max_remedies_per_client(50000);

// Start server
if (server.start()) {
    std::cout << "Watchtower server started on port 9735" << std::endl;
}

// Monitor blockchain (called for each new block)
void on_new_block(const std::vector<Transaction>& txs, uint32_t height) {
    server.process_block(txs, height);

    // Print stats periodically
    if (height % 100 == 0) {
        auto stats = server.get_stats();
        std::cout << "Watchtower Stats:" << std::endl;
        std::cout << "  Clients: " << stats.registered_clients << std::endl;
        std::cout << "  Remedies: " << stats.stored_remedies << std::endl;
        std::cout << "  Breaches: " << stats.breaches_detected << std::endl;
        std::cout << "  Penalties: " << stats.penalties_broadcast << std::endl;
    }
}
```

### Monitoring Watchtower Status

```cpp
// Get status of all watchtowers
auto statuses = manager.get_watchtower_status();

for (const auto& status : statuses) {
    std::cout << status.address << ":" << status.port << std::endl;
    std::cout << "  Online: " << (status.online ? "Yes" : "No") << std::endl;
    std::cout << "  Remedies uploaded: " << status.remedies_uploaded << std::endl;
    std::cout << "  Last contact: " << status.last_contact << std::endl;
}
```

## Configuration

### Client Configuration

Recommended client setup:

```cpp
// Use at least 3 watchtowers for redundancy
const size_t MIN_WATCHTOWERS = 3;

// Primary watchtowers (high reliability)
manager.add_watchtower("watchtower1.intcoin.org", 9735);
manager.add_watchtower("watchtower2.intcoin.org", 9735);
manager.add_watchtower("watchtower3.intcoin.org", 9735);

// Backup watchtowers (optional)
manager.add_watchtower("backup.watchtower.com", 9735);

// Verify all are online
if (manager.get_online_watchtowers() < MIN_WATCHTOWERS) {
    std::cerr << "WARNING: Insufficient online watchtowers!" << std::endl;
}
```

### Server Configuration

```cpp
WatchtowerServer server(9735);

// Storage limits
server.set_max_clients(10000);              // Maximum registered clients
server.set_max_remedies_per_client(50000);  // Max remedies per client

// Start with blockchain at specific height
server.start();
```

## Running a Watchtower Server

### Prerequisites

- Full INTcoin node (for blockchain monitoring)
- Stable internet connection and uptime
- Sufficient storage (estimate: ~100MB per 1000 channels)

### Setup

```bash
# Build watchtower server
cmake --build build --target watchtower_server

# Run server
./build/watchtower_server --port=9735 --max-clients=10000
```

### Server CLI Arguments

```
--port=PORT              Listen port (default: 9735)
--max-clients=N          Maximum registered clients (default: 10000)
--max-remedies=N         Maximum remedies per client (default: 10000)
--data-dir=PATH          Data directory for storage
--log-level=LEVEL        Logging level (debug, info, warn, error)
```

### Monitoring

```bash
# Check server status
./build/watchtower_cli status

# View statistics
./build/watchtower_cli stats

# List registered clients
./build/watchtower_cli clients

# View stored remedies
./build/watchtower_cli remedies --channel=<CHANNEL_ID>
```

## Privacy Considerations

### What Watchtowers Can Learn

1. **Channel Count**: Number of channels you have (via channel IDs)
2. **Update Frequency**: How often channel states change
3. **Remedy Size**: Approximate size of penalty transactions

### What Watchtowers Cannot Learn

1. **Transaction Details**: All payment info is encrypted
2. **Counterparty Identity**: Channels are blinded
3. **Payment Amounts**: Encrypted in breach remedy
4. **Channel Balances**: Only revealed if breach occurs

### Best Practices

1. **Use Multiple Watchtowers**: Don't rely on a single provider
2. **Tor Support**: Connect to watchtowers via Tor for IP privacy
3. **Rotate Watchtowers**: Periodically change watchtower set
4. **Self-Hosted**: Run your own watchtower for maximum privacy

## Quantum Resistance

INTcoin's watchtower implementation uses:

- **CRYSTALS-Dilithium5** for all signatures (client registration, remedy signatures)
- **SHA3-256** for all hashing (TXID derivation, encryption keys, hints)

This ensures that even quantum computers cannot:
- Forge client registrations
- Forge breach remedy signatures
- Reverse engineer encryption keys from hints

## Limitations

### Current Limitations

1. **Network Layer**: Simplified network communication (production needs p2p)
2. **Persistence**: In-memory storage only (production needs database)
3. **Breach Detection**: Simplified matching (production needs TXID extraction)

### Future Improvements

- [ ] Add database persistence (RocksDB/SQLite)
- [ ] Implement full P2P networking
- [ ] Add payment/fee mechanism for watchtower services
- [ ] Support for watchtower federation/redundancy
- [ ] Advanced breach detection heuristics
- [ ] Support for batch remedy uploads

## References

- [BOLT #13 - Watchtowers](https://github.com/lightning/bolts/blob/master/13-watchtowers.md)
- [The Eye of Satoshi - Watchtower Design](https://github.com/talaia-labs/python-teos)
- [Lightning Network Watchtowers](https://docs.lightning.engineering/lightning-network-tools/lnd/watchtower)

## License

This watchtower implementation is released under the MIT License, same as INTcoin Core.

Copyright (c) 2025 INTcoin Core (Maddison Lane)
