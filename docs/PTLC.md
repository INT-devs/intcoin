# INTcoin Point Time Locked Contracts (PTLCs)

## Overview

Point Time Locked Contracts (PTLCs) are a privacy-enhanced alternative to Hash Time Locked Contracts (HTLCs) used in Lightning Network payments. PTLCs use **adaptor signatures** and **elliptic curve points** instead of hash preimages, providing significantly better privacy and enabling advanced features like stuckless payments and payment decorrelation.

## Table of Contents

- [Overview](#overview)
- [HTLCs vs PTLCs](#htlcs-vs-ptlcs)
- [How PTLCs Work](#how-ptlcs-work)
- [Privacy Benefits](#privacy-benefits)
- [Architecture](#architecture)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Advanced Features](#advanced-features)

## HTLCs vs PTLCs

### Hash Time Locked Contracts (HTLCs)

**Traditional Lightning Payment:**
```
Sender → [HTLC: H(r)] → Hop1 → [HTLC: H(r)] → Hop2 → [HTLC: H(r)] → Receiver
         Same hash across all hops (linkable!)
```

**Problems:**
- ❌ **Same hash everywhere**: All hops see identical payment hash
- ❌ **Linkable**: Easy to correlate sender to receiver
- ❌ **No privacy**: Routing nodes can track payments
- ❌ **Stuck payments**: Capital locked if payment hangs
- ❌ **Larger scripts**: Requires hash locks in scripts

### Point Time Locked Contracts (PTLCs)

**PTLC Payment:**
```
Sender → [PTLC: P₁] → Hop1 → [PTLC: P₂] → Hop2 → [PTLC: P₃] → Receiver
         Different points (unlinkable!)
```

**Advantages:**
- ✅ **Different points**: Each hop sees unique payment point
- ✅ **Unlinkable**: Cannot correlate sender to receiver
- ✅ **Private**: Looks like regular payment
- ✅ **Stuckless**: Can cancel in-flight payments
- ✅ **Smaller transactions**: Scriptless scripts

## How PTLCs Work

### Step 1: Payment Point Generation

Sender generates:
- **Payment secret**: `p` (random scalar)
- **Payment point**: `P = p·G` (elliptic curve point)

### Step 2: Path Decorrelation

For each hop, create unique point:
- Hop 1: `P₁ = P + r₁·G`
- Hop 2: `P₂ = P + r₂·G`
- Hop 3: `P₃ = P + r₃·G`

Where `r₁, r₂, r₃` are random scalars known only to sender.

### Step 3: Adaptor Signatures

Each hop creates **adaptor signature**:
- Signature valid only when combined with payment secret `p`
- Revealing signature reveals the secret
- No on-chain hash required!

### Step 4: Payment Settlement

1. Receiver claims by revealing `p`
2. Previous hop extracts `p` from revealed signature
3. Each hop claims backward to sender
4. All done with **scriptless scripts** (no visible locks!)

## Privacy Benefits

### Payment Decorrelation

```
HTLC Payment (Linkable):
┌──────┐ Hash(r) ┌──────┐ Hash(r) ┌──────┐ Hash(r) ┌──────────┐
│Sender├────────>│ Hop1 ├────────>│ Hop2 ├────────>│ Receiver │
└──────┘         └──────┘         └──────┘         └──────────┘
         ↑________________Same hash everywhere!_________________↑


PTLC Payment (Unlinkable):
┌──────┐  P₁     ┌──────┐  P₂     ┌──────┐  P₃     ┌──────────┐
│Sender├────────>│ Hop1 ├────────>│ Hop2 ├────────>│ Receiver │
└──────┘         └──────┘         └──────┘         └──────────┘
         ↑________________Different points everywhere!__________↑
```

### On-Chain Privacy

**HTLC (Visible):**
```
OP_IF
  OP_SHA256 <payment_hash> OP_EQUALVERIFY <receiver_key> OP_CHECKSIG
OP_ELSE
  <timeout> OP_CHECKLOCKTIMEVERIFY OP_DROP <sender_key> OP_CHECKSIG
OP_ENDIF
```
*Everyone can see this is a Lightning payment!*

**PTLC (Invisible):**
```
<receiver_key> OP_CHECKSIG
```
*Looks like a regular payment!*

## Architecture

### Adaptor Signature Scheme

```
┌─────────────────────────────────────────┐
│         Adaptor Signature Flow          │
└─────────────────────────────────────────┘

1. Create Adaptor Signature:
   sig' = Sign(privkey, msg) - adaptor_secret
   ↓
   Partial signature (invalid without adaptor)

2. Complete Adaptor Signature:
   sig = sig' + adaptor_secret
   ↓
   Full signature (valid)

3. Extract Secret:
   adaptor_secret = sig - sig'
   ↓
   Recover payment secret from signatures!
```

### PTLC Structure

```cpp
struct PTLC {
    Hash256 ptlc_id;              // Unique identifier
    uint64_t amount_sat;          // Payment amount
    uint32_t timeout_height;      // Absolute timeout

    Hash256 payment_point;        // P = p·G (commitment)
    AdaptorSignature adaptor;     // Partial signature

    bool claimed;                 // Payment claimed?
    optional<Hash256> secret;     // Revealed secret (p)
};
```

## API Reference

### PTLCManager

```cpp
#include "intcoin/ptlc.h"

using namespace intcoin::ptlc;

// Create manager
PTLCManager manager;

// Create PTLC payment
auto payment_id = manager.create_ptlc_payment(
    destination_pubkey,
    100000,         // 100K sats
    route,          // Payment route
    144             // Timeout: 144 blocks
);

// Send payment
manager.send_ptlc_payment(payment_id);

// Claim payment (receiver)
manager.claim_ptlc(ptlc_id, payment_secret);
```

### Adaptor Signatures

```cpp
// Create adaptor signature
AdaptorSignature adaptor = manager.create_adaptor_signature(
    my_privkey,
    message,
    adaptor_point
);

// Complete signature (add secret)
CompletedSignature completed = manager.complete_adaptor_signature(
    adaptor,
    secret_scalar
);

// Extract secret from completed signature
Hash256 extracted_secret = manager.extract_secret(
    adaptor,
    completed
);
```

## Usage Examples

### Example 1: Sending a PTLC Payment

```cpp
#include "intcoin/ptlc.h"

PTLCManager manager;

// Find route to destination
std::vector<RouteHop> route = find_route(destination, 500000);

// Create PTLC payment (automatically decorrelates)
auto payment_id = manager.create_ptlc_payment(
    destination_pubkey,
    500000,         // 0.5M sats
    route,
    288             // 2 day timeout
);

if (payment_id.has_value()) {
    // Send payment
    if (manager.send_ptlc_payment(*payment_id)) {
        std::cout << "PTLC payment sent!" << std::endl;

        // Wait for completion (or failure)
        auto payment = manager.get_payment(*payment_id);
        if (payment->state == PTLCPayment::State::SUCCEEDED) {
            std::cout << "Payment succeeded!" << std::endl;
        }
    }
}
```

### Example 2: Receiving and Claiming a PTLC

```cpp
// Receiver side
void on_ptlc_received(const ChannelPTLC& ptlc) {
    std::cout << "Received PTLC for " << ptlc.amount_sat << " sats" << std::endl;

    // Verify payment point matches invoice
    if (verify_payment_point(ptlc.payment_point, my_invoice)) {
        // Claim payment by revealing secret
        manager.claim_ptlc(ptlc.ptlc_id, my_payment_secret);

        std::cout << "PTLC claimed!" << std::endl;
    }
}
```

### Example 3: Forwarding PTLC (Routing Node)

```cpp
// Routing node forwards PTLC
void forward_ptlc(const ChannelPTLC& incoming_ptlc) {
    // Create outgoing PTLC with decorrelated point
    ChannelPTLC outgoing;
    outgoing.payment_point = derive_outgoing_point(incoming_ptlc.payment_point);
    outgoing.amount_sat = incoming_ptlc.amount_sat - routing_fee;

    // Add to outgoing channel
    manager.add_channel_ptlc(
        outgoing_channel_id,
        outgoing,
        true  // outgoing
    );

    // When outgoing is claimed, claim incoming using extracted secret
}
```

### Example 4: Stuckless Payment (Cancel In-Flight)

```cpp
// Send payment
auto payment_id = manager.create_ptlc_payment(
    destination,
    1000000,
    route,
    144
);

manager.send_ptlc_payment(*payment_id);

// ... wait for response ...

// Timeout or no response - cancel payment!
// With PTLCs, we can cancel without waiting for full timeout
manager.fail_ptlc(ptlc_id, "Timeout - cancelling");

// Funds immediately available for retry!
```

## Advanced Features

### 1. Payment Decorrelation

Each hop sees a different payment point:

```cpp
// Sender derives unique points for each hop
Hash256 base_point = payment_secret * G;

for (size_t i = 0; i < route.size(); i++) {
    Hash256 hop_scalar = random_scalar();
    Hash256 hop_point = base_point + (hop_scalar * G);

    // Each hop gets unique point
    ptlcs[i].payment_point = hop_point;
}
```

### 2. Stuckless Payments

Cancel in-flight payments without waiting for timeout:

```cpp
// Traditional HTLC: Must wait for full timeout
wait_for_timeout(144_blocks);  // Funds locked!

// PTLC: Can cancel immediately
manager.fail_ptlc(ptlc_id, "Cancelling");  // Funds freed!
```

### 3. Atomic Multi-Path with PTLCs

Combine PTLCs with AMP for ultimate privacy:

```cpp
// Split payment across paths with different points
auto payment_id = amp_manager.create_amp_payment_with_ptlcs(
    destination,
    5000000,        // 5M sats
    4               // 4 paths
);

// Each path gets unique PTLC point
// Cannot correlate paths!
```

### 4. Scriptless Scripts

PTLCs enable "scriptless scripts" - complex conditions without visible scripts:

```cpp
// HTLC (Script Visible):
OP_IF
  OP_SHA256 <hash> OP_EQUALVERIFY ...
OP_ENDIF

// PTLC (Scriptless - Just a Signature):
<signature> <pubkey>

// But still enforces payment condition via adaptor signatures!
```

## Technical Details

### Elliptic Curve Operations

PTLCs use elliptic curve point arithmetic:

**Point Addition:**
```
P₁ + P₂ = P₃
(x₁,y₁) + (x₂,y₂) = (x₃,y₃)
```

**Scalar Multiplication:**
```
p·G = P (payment point)
Where p is secret scalar, G is generator point
```

**Point Derivation:**
```
P_hop = P_base + r·G
Where r is random blinding factor
```

### Adaptor Signature Construction

```cpp
// Create adaptor signature
1. Generate nonce: k = random()
2. Compute R = k·G (nonce point)
3. Compute T = adaptor_point (adaptor)
4. Create partial signature:
   s' = k - hash(R||T||msg) · privkey

AdaptorSignature { R, s', T }
```

**Complete signature:**
```cpp
// Add adaptor secret
s = s' + t (where T = t·G)

// Now (R, s) is valid Schnorr signature!
```

### Quantum Resistance

INTcoin's PTLC implementation uses:
- **CRYSTALS-Dilithium5** for adaptor signatures
- **SHA3-256** for point hashing
- Post-quantum secure point derivation

**Note:** Traditional PTLCs rely on discrete log hardness (broken by quantum computers). INTcoin's implementation uses post-quantum adaptor signatures.

## Security Considerations

### Payment Secret Protection

- Never reuse payment secrets
- Generate fresh secret for each payment
- Secure random number generation critical

### Timeout Management

```cpp
// Set appropriate timeouts
uint32_t timeout = estimate_route_delay(route) + safety_margin;

// Typical values:
// - Direct payment: 144 blocks (~12 hours)
// - Multi-hop: 288 blocks (~24 hours)
// - Long route: 576 blocks (~48 hours)
```

### Point Validation

Always validate received points:

```cpp
bool validate_payment_point(const Hash256& point) {
    // Check point is on curve
    if (!is_on_curve(point)) return false;

    // Check point is not identity
    if (is_identity(point)) return false;

    // Check point has correct order
    if (!has_correct_order(point)) return false;

    return true;
}
```

## Performance Comparison

| Metric | HTLC | PTLC | Improvement |
|--------|------|------|-------------|
| Script Size | ~100 bytes | ~35 bytes | **65% smaller** |
| On-Chain Footprint | Visible hash lock | Regular payment | **Completely hidden** |
| Privacy | Low (linkable) | High (unlinkable) | **Much better** |
| Cancellation | Wait for timeout | Immediate | **Instant** |
| Transaction Fees | Higher | Lower | **Cheaper** |

## Limitations

1. **More Complex**: Requires adaptor signatures
2. **Newer Technology**: Less battle-tested than HTLCs
3. **Elliptic Curve Dependency**: Relies on EC hardness (addressed with PQ signatures)

## Future Enhancements

- [ ] Optimized point derivation
- [ ] Multi-signature PTLCs
- [ ] Cross-chain PTLCs
- [ ] Hardware wallet support

## Compatibility

**Works Great With:**
- ✅ Eltoo (simplified updates)
- ✅ AMP (multi-path payments)
- ✅ Channel Factories
- ✅ Submarine Swaps

**Requires:**
- Adaptor signature support
- Elliptic curve operations
- SIGHASH_NOINPUT (for best results with Eltoo)

## References

- [Scriptless Scripts Paper](https://github.com/ElementsProject/scriptless-scripts)
- [Adaptor Signatures](https://github.com/BlockstreamResearch/scriptless-scripts/blob/master/md/atomic-swap.md)
- [PTLC Proposal](https://lists.linuxfoundation.org/pipermail/lightning-dev/2018-February/001038.html)

## License

This PTLC implementation is released under the MIT License.

Copyright (c) 2025 INTcoin Core (Maddison Lane)
