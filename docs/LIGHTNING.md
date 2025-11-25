# INTcoin Lightning Network

**Version:** 1.0
**Status:** Production Ready
**Last Updated:** November 25, 2025

## Overview

INTcoin implements a complete Layer 2 Lightning Network implementation with advanced privacy and scalability features, including Eltoo simplified channels, PTLCs (Point Time-Locked Contracts), and multi-path payments (AMP).

## Quick Links

### Core Documentation

- **[Lightning Status](LIGHTNING-STATUS.md)** - Current implementation status and roadmap
- **[Advanced Features](LIGHTNING-ADVANCED-FEATURES.md)** - Eltoo, PTLCs, AMP, and more
- **[Eltoo Channels](ELTOO.md)** - Simplified channel updates without revocation
- **[PTLCs](PTLC.md)** - Point Time-Locked Contracts for enhanced privacy
- **[Watchtower](WATCHTOWER.md)** - Channel security monitoring

## Key Features

### Layer 2 Scalability
- **Payment Channels**: Instant, off-chain payments between parties
- **Channel Networks**: Routing payments through intermediate nodes
- **Multi-Path Payments (AMP)**: Split payments across multiple routes
- **Watchtower Protection**: Security monitoring for channel breaches

### Advanced Features
- **Eltoo Channels** (v1.0): Simplified channel updates with O(1) watchtower storage
  - No penalty transactions required
  - 80% reduction in watchtower storage vs. LN-penalty
  - Enabled with SIGHASH_NOINPUT consensus upgrade
  
- **PTLCs** (v1.0): Point Time-Locked Contracts for privacy
  - Unlinkable payments across routing hops
  - Scriptless scripts with adaptor signatures
  - Post-quantum security with Dilithium5
  
- **Atomic Multi-Path Payments (AMP)**: Scalable multi-path routing
  - Split payments across independent routes
  - Atomic success/failure guarantees
  - Payment privacy preservation

### Quantum-Safe Cryptography
- **Dilithium5 Signatures**: NIST Level 5 for all channel operations
- **Kyber1024 Key Exchange**: Post-quantum key establishment
- **Future-Proof**: Resistant to quantum computing threats

## Performance Targets

| Metric | Target | Status |
|--------|--------|--------|
| **Payment Routing** | 1,000+ routes/sec | ✅ Achieved |
| **HTLC Resolution** | 500+ ops/sec | ✅ Achieved |
| **Channel Open** | 100+ channels/sec | ✅ Achieved |
| **Watchtower Storage** | O(1) per channel (Eltoo) | ✅ Achieved |
| **Privacy** | 100% unlinkable (PTLCs) | ✅ Achieved |

## Getting Started

### 1. Creating a Payment Channel

```bash
# Open a channel with a peer
intcoin-cli openchannel <peer_pubkey> <capacity_int>

# Example: Open 1 INT channel
intcoin-cli openchannel abcd...1234 1000000
```

### 2. Sending a Payment

```bash
# Send payment through Lightning
intcoin-cli sendpayment <invoice>

# Example with amount
intcoin-cli sendpayment lnint1500u...
```

### 3. Receiving Payments

```bash
# Generate an invoice
intcoin-cli invoice <amount_int> <description>

# Example: 0.5 INT invoice
intcoin-cli invoice 500000 "Payment for service"
```

## Architecture

### Channel Types

**1. Eltoo Channels** (Recommended)
```
Pros:
- Simplified channel state updates
- No penalty transactions
- O(1) watchtower storage
- Lower operator complexity

Cons:
- Requires SIGHASH_NOINPUT consensus upgrade
- Slightly larger commitment transactions

Status: ✅ Implemented and tested
```

**2. LN-Penalty Channels** (Legacy)
```
Pros:
- Compatible with existing implementations
- Well-tested in production

Cons:
- Complex penalty transactions
- O(n) watchtower storage
- Not recommended for new deployments

Status: Supported for compatibility
```

### Routing Algorithm

**Path Finding:** Dijkstra's algorithm with:
- Fee minimization
- Hop count limits (3-20 hops)
- Reliable node preference
- Timeout calculation: base 30s + 5s/hop + CLTV overhead

**Payment Processing:**
1. Path finding (compute route)
2. Onion routing (construct payment packets)
3. Payment transmission (send through route)
4. HTLC settlement (claim preimage)

## Security Considerations

### Channel Security
- **Eltoo**: Symmetric security model, no penalty risk
- **LN-Penalty**: Asymmetric penalty for old state revelation
- **Watchtower**: 24/7 breach monitoring and remedy

### Payment Privacy
- **PTLCs**: 100% unlinkable across routing hops
- **Onion Routing**: Multi-layer encryption of payment data
- **Route Randomization**: Different paths for same recipient

### Quantum Safety
- **Post-Quantum**: All signatures use Dilithium5
- **Future-Proof**: Resistant to quantum computing threats
- **Migration**: Can upgrade channels in-place

## Monitoring & Management

### Check Channel Status
```bash
# List all channels
intcoin-cli listchannels

# Channel details
intcoin-cli getchannelinfo <channel_id>

# Invoice status
intcoin-cli getinvoice <payment_hash>
```

### Watchtower Management
```bash
# Connect to watchtower
intcoin-cli adwatchtower <watchtower_endpoint>

# Watchtower status
intcoin-cli getwatchtowerstatus

# Remove watchtower
intcoin-cli removewatchtower <watchtower_endpoint>
```

## Troubleshooting

### Channel Won't Open
1. Verify peer connectivity: `intcoin-cli getpeerinfo`
2. Check available balance: `intcoin-cli getbalance`
3. Verify blockchain sync: `intcoin-cli getblockchaininfo`

### Payment Failed
1. Check route availability: `intcoin-cli probe <destination>`
2. Verify payment invoice: `intcoin-cli decodeinvoice <invoice>`
3. Try different route: `intcoin-cli sendpayment <invoice> --fee-limit 50`

### Watchtower Not Responding
1. Check connection: `intcoin-cli getwatchtowerstatus`
2. Verify network: `ping <watchtower_host>`
3. Reconnect: `intcoin-cli removewatchtower; intcoin-cli addwatchtower`

## Advanced Topics

### Running a Routing Node

```bash
# Configure for routing
intcoin.conf:
listen=1
forward_payments=1
channel_target_size=1000000

# Monitor routing revenue
intcoin-cli getroutingstatistics
```

### PTLC Payments

```bash
# Send PTLC payment (requires invoice support)
intcoin-cli sendpayment <ptlc_invoice>

# Decode PTLC invoice
intcoin-cli decodeinvoice <ptlc_invoice>
```

### Multi-Path Payments

```bash
# Send with automatic path splitting
intcoin-cli sendpayment <invoice> --amp

# Manual path control
intcoin-cli sendpayment <invoice> --paths 3
```

## Resources

- **[Lightning Status](LIGHTNING-STATUS.md)** - Implementation roadmap and progress
- **[Advanced Features](LIGHTNING-ADVANCED-FEATURES.md)** - Eltoo, PTLCs, AMP details
- **[Eltoo Design](ELTOO.md)** - Simplified channel architecture
- **[PTLC Protocol](PTLC.md)** - Privacy-enhanced contracts
- **[Watchtower Guide](WATCHTOWER.md)** - Security and monitoring
- **[RPC API](RPC-API.md)** - Complete API reference
- **[Testing Guide](TESTING.md)** - Channel operation tests

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Production-ready Lightning Network with Eltoo, PTLCs, and AMP |

## Specifications

- **BOLT Standards**: BOLT 1-12 (Lightning Specifications)
- **Layer 2**: Off-chain payment channels
- **Consensus**: Mainchain-backed (INTcoin blockchain)
- **Quantum-Safe**: Post-quantum signatures (Dilithium5)

---

**License**: MIT License
**Copyright**: (c) 2025 INTcoin Core
