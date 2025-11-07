# INTcoin Lightning Network Implementation

## Overview

The Lightning Network is a Layer 2 payment protocol that enables instant, low-cost transactions on top of INTcoin's blockchain. This implementation provides full support for payment channels, routing, and invoice management.

## Features

### Core Functionality

✅ **Payment Channels**
- Bidirectional payment channels
- Multi-signature funding transactions
- Commitment transactions with revocation
- Cooperative and unilateral channel closing
- Channel reserve and fee management

✅ **HTLCs (Hash Time Locked Contracts)**
- Conditional payments with preimage
- Time-lock refunds
- Multi-hop payment forwarding
- Atomic payment guarantees

✅ **Routing & Pathfinding**
- Network graph maintenance
- Dijkstra's shortest path algorithm
- Multi-path payments (MPP)
- Route optimization
- Fee calculation
- Success probability estimation

✅ **Invoice System (BOLT #11)**
- Payment request generation
- Bech32 encoding
- Route hints for private channels
- Expiry and minimum CLTV
- Payment secret for MPP

✅ **Onion Routing**
- Sphinx packet construction
- Privacy-preserving payments
- Source routing
- Up to 20 hops per payment

## Architecture

```
┌──────────────────────────────────────────┐
│         Lightning Node                    │
│                                           │
│  ┌─────────────────────────────────────┐ │
│  │     Channel Manager                  │ │
│  │  - Open/Close channels               │ │
│  │  - Commitment management             │ │
│  │  - HTLC tracking                     │ │
│  └─────────────────────────────────────┘ │
│                                           │
│  ┌─────────────────────────────────────┐ │
│  │     Network Graph                    │ │
│  │  - Topology tracking                 │ │
│  │  - Channel announcements             │ │
│  │  - Node announcements                │ │
│  └─────────────────────────────────────┘ │
│                                           │
│  ┌─────────────────────────────────────┐ │
│  │     Route Finder                     │ │
│  │  - Pathfinding algorithms            │ │
│  │  - Fee optimization                  │ │
│  │  - Multi-path splitting              │ │
│  └─────────────────────────────────────┘ │
│                                           │
│  ┌─────────────────────────────────────┐ │
│  │     Invoice Manager                  │ │
│  │  - Invoice generation                │ │
│  │  - Payment tracking                  │ │
│  │  - Preimage management               │ │
│  └─────────────────────────────────────┘ │
└──────────────────────────────────────────┘
           │                    │
           │                    │
    ┌──────▼────────┐   ┌──────▼────────┐
    │  Peer Network │   │   Blockchain  │
    │  (P2P Gossip) │   │  (On-chain)   │
    └───────────────┘   └───────────────┘
```

## Usage

### Creating a Lightning Node

```cpp
#include <intcoin/lightning/lightning_node.h>

using namespace intcoin::lightning;

// Initialize blockchain
Blockchain blockchain;

// Create node with identity
PrivateKey node_key = PrivateKey::generate();
LightningNode ln_node("MyLightningNode", node_key, &blockchain);

// Configure node
LightningNode::Config config;
config.cltv_expiry_delta = 40;
config.fee_base_msat = 1000;      // 1 satoshi base fee
config.fee_proportional_millionths = 100;  // 0.01% fee
ln_node.set_config(config);

// Start Lightning node
if (ln_node.start(9735)) {
    std::cout << "Lightning node running on port 9735" << std::endl;
    std::cout << "Node ID: " << ln_node.get_node_id().to_string() << std::endl;
}
```

### Opening a Payment Channel

```cpp
// Connect to peer
PublicKey peer_id = PublicKey::from_string("02abc123...");
ln_node.connect_peer("lightning.example.com", 9735);

// Open channel with 0.1 INT capacity
uint64_t capacity = 10000000;  // 0.1 INT in satoshis
Hash256 channel_id = ln_node.open_channel(peer_id, capacity);

std::cout << "Channel opened: " << channel_id.to_string() << std::endl;
```

### Creating and Paying an Invoice

```cpp
// Receiver creates invoice
Invoice invoice = ln_node.create_invoice(
    100000000,                    // 1 INT (in millisatoshis)
    "Payment for coffee",
    3600                          // 1 hour expiry
);

std::string payment_request = invoice.to_string();
std::cout << "Invoice: " << payment_request << std::endl;

// Sender pays invoice
auto result = ln_node.send_payment(invoice);

if (result.success) {
    std::cout << "Payment sent successfully!" << std::endl;
    std::cout << "Preimage: " << result.payment_preimage.to_string() << std::endl;
    std::cout << "Fees paid: " << result.fees_paid << " msat" << std::endl;
} else {
    std::cerr << "Payment failed: " << result.error_message << std::endl;
}
```

### Multi-Path Payments

```cpp
// Send large payment split across multiple routes
auto result = ln_node.send_multi_path_payment(invoice, 5);

if (result.success) {
    std::cout << "Multi-path payment successful!" << std::endl;
    std::cout << "Split across " << result.route.hop_count() << " paths" << std::endl;
}
```

### Finding Routes

```cpp
// Find routes to destination
PublicKey destination = PublicKey::from_string("03def456...");
uint64_t amount_msat = 50000000;  // 0.5 INT

auto routes = ln_node.find_route(destination, amount_msat);

for (const auto& route : routes) {
    std::cout << "Route found:" << std::endl;
    std::cout << "  Hops: " << route.hop_count() << std::endl;
    std::cout << "  Total fees: " << route.total_fees << " msat" << std::endl;
    std::cout << "  Success probability: "
              << (route.success_probability * 100) << "%" << std::endl;
}
```

### Channel Management

```cpp
// List all channels
auto channels = ln_node.list_channels();

for (const auto& channel : channels) {
    std::cout << "Channel ID: " << channel->get_channel_id().to_string() << std::endl;
    std::cout << "  State: " << (channel->is_open() ? "OPEN" : "CLOSED") << std::endl;
    std::cout << "  Capacity: " << channel->get_capacity() << " sat" << std::endl;
    std::cout << "  Local balance: " << channel->get_local_balance() << " sat" << std::endl;
    std::cout << "  Remote balance: " << channel->get_remote_balance() << " sat" << std::endl;
}

// Close channel
ln_node.close_channel(channel_id, false);  // Cooperative close
```

### Statistics

```cpp
auto stats = ln_node.get_stats();

std::cout << "Lightning Node Statistics:" << std::endl;
std::cout << "  Channels: " << stats.num_channels << std::endl;
std::cout << "  Active channels: " << stats.num_active_channels << std::endl;
std::cout << "  Total capacity: " << stats.total_capacity << " sat" << std::endl;
std::cout << "  Payments sent: " << stats.num_payments_sent << std::endl;
std::cout << "  Payments received: " << stats.num_payments_received << std::endl;
std::cout << "  Total sent: " << stats.total_sent << " msat" << std::endl;
std::cout << "  Total received: " << stats.total_received << " msat" << std::endl;
std::cout << "  Fees earned: " << stats.total_fees_earned << " msat" << std::endl;
```

## Payment Flow

### Sending a Payment

1. **Invoice Decode**: Parse payment request
2. **Route Finding**: Discover path to destination
3. **HTLC Creation**: Build HTLCs for each hop
4. **Onion Construction**: Create encrypted payment packet
5. **Payment Sending**: Forward payment through route
6. **Preimage Receipt**: Claim payment with preimage
7. **Channel Update**: Update commitment transactions

### Receiving a Payment

1. **Invoice Generation**: Create payment request
2. **HTLC Receipt**: Receive incoming HTLC
3. **Preimage Release**: Reveal payment preimage
4. **Settlement**: Claim funds in channel
5. **Invoice Marking**: Mark invoice as paid

### Routing a Payment

1. **HTLC Receipt**: Receive HTLC from previous hop
2. **Onion Peeling**: Decrypt routing information
3. **Fee Deduction**: Deduct routing fee
4. **HTLC Forward**: Create HTLC to next hop
5. **Preimage Relay**: Forward preimage back
6. **Fee Collection**: Earn routing fees

## Channel Lifecycle

### Opening a Channel

```
1. Funding Transaction
   └─> Lock funds on-chain with 2-of-2 multisig

2. Exchange Channel Parameters
   └─> CLTV delta, fees, reserves

3. Create Initial Commitment
   └─> Both parties sign initial state

4. Wait for Confirmations
   └─> Funding tx confirmed on blockchain

5. Channel Ready
   └─> Ready to send/receive payments
```

### Updating a Channel

```
1. Propose Update (add HTLC)
   └─> Sender proposes new HTLC

2. Accept Update
   └─> Receiver validates and accepts

3. Create New Commitment
   └─> Both parties create new commitment tx

4. Revoke Old Commitment
   └─> Exchange revocation secrets

5. Channel Updated
   └─> New balance state active
```

### Closing a Channel

**Mutual Close (Cooperative)**:
1. Negotiate final state
2. Create closing transaction
3. Broadcast to blockchain
4. Funds distributed immediately

**Unilateral Close (Force Close)**:
1. Broadcast latest commitment
2. Wait for timelock
3. Claim funds after delay
4. May trigger penalty if cheating

## Security Features

### Channel Security
- 2-of-2 multisig funding
- Revocable commitment transactions
- Penalty mechanism for cheating
- Watchtowers for monitoring (planned)

### Payment Security
- HTLCs ensure atomicity
- Onion routing for privacy
- Payment secrets prevent probing
- No routing node learns full path

### Network Security
- Signature validation on all messages
- DoS protection via rate limiting
- Channel spam prevention
- Gossip message verification

## Configuration

### Channel Parameters

```cpp
LightningNode::Config config;

// HTLC parameters
config.htlc_minimum_msat = 1000;           // Minimum payment
config.htlc_maximum_msat = 10000000000;    // Maximum payment
config.max_htlc_in_flight = 483;           // Max pending HTLCs
config.max_accepted_htlcs = 483;           // Max HTLCs to accept

// Fee parameters
config.fee_base_msat = 1000;               // Base fee (1 sat)
config.fee_proportional_millionths = 100;  // 0.01% proportional

// Timelock parameters
config.cltv_expiry_delta = 40;             // CLTV delta (blocks)

// Channel parameters
config.channel_reserve_satoshis = 10000;   // Reserve amount
config.accept_inbound_channels = true;     // Accept incoming channels

ln_node.set_config(config);
```

## BOLT Specifications

This implementation follows the Lightning Network BOLT specifications:

- **BOLT #2**: Peer Protocol for Channel Management
- **BOLT #3**: Bitcoin Transaction and Script Formats
- **BOLT #4**: Onion Routing Protocol
- **BOLT #5**: Recommendations for On-chain Transaction Handling
- **BOLT #7**: P2P Node and Channel Discovery
- **BOLT #11**: Invoice Protocol for Lightning Payments

## Future Enhancements

### Planned Features

1. **Watchtowers**
   - Monitor channels for cheating
   - Automated breach remediation
   - Privacy-preserving monitoring

2. **Submarine Swaps**
   - On-chain to off-chain swaps
   - Off-chain to on-chain swaps
   - Trustless exchange

3. **Atomic Multi-Path Payments (AMP)**
   - Split payments across paths
   - Increased reliability
   - Better privacy

4. **Trampoline Routing**
   - Delegate pathfinding to routing nodes
   - Reduced mobile client burden
   - Better for resource-constrained devices

5. **Channel Factories**
   - Multiple channels from single transaction
   - Reduced on-chain footprint
   - Better capital efficiency

6. **Splicing**
   - Add/remove funds without closing
   - Seamless capacity management
   - No downtime

7. **Dual-Funded Channels**
   - Both parties contribute funds
   - Better liquidity distribution
   - More balanced channels

## Building

Add to CMakeLists.txt:

```cmake
add_library(lightning
    src/lightning/channel.cpp
    src/lightning/routing.cpp
    src/lightning/invoice.cpp
    src/lightning/lightning_node.cpp
)

target_link_libraries(lightning
    PRIVATE core crypto
)
```

## Testing

```bash
# Run Lightning tests
./test_lightning

# Start Lightning node
./intcoin-lightning --alias "MyNode" --port 9735

# CLI operations
./intcoin-lightning-cli openchannel <peer_id> <amount>
./intcoin-lightning-cli createinvoice <amount> <description>
./intcoin-lightning-cli payinvoice <invoice_string>
./intcoin-lightning-cli listchannels
```

## Performance

- **Payment Latency**: < 1 second
- **Throughput**: Thousands of payments per second per channel
- **Fees**: Typically < 1% of payment amount
- **Scalability**: Unlimited off-chain capacity

## References

- [Lightning Network Whitepaper](https://lightning.network/lightning-network-paper.pdf)
- [BOLT Specifications](https://github.com/lightning/bolts)
- [LND Documentation](https://docs.lightning.engineering/)
- [c-lightning Documentation](https://lightning.readthedocs.io/)
- [Eclair Documentation](https://github.com/ACINQ/eclair)

## License

MIT License - See LICENSE file for details

## Authors

INTcoin Core Development Team
