# INTcoin Eltoo Implementation

## Overview

Eltoo is a channel update mechanism that dramatically simplifies Lightning Network channels by eliminating the need for penalty transactions and revocation keys. It uses the SIGHASH_NOINPUT signature flag to allow update transactions to spend any previous update, making channel management much simpler and safer.

## Table of Contents

- [Overview](#overview)
- [How Eltoo Works](#how-eltoo-works)
- [Advantages Over Traditional Lightning](#advantages-over-traditional-lightning)
- [Architecture](#architecture)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Technical Details](#technical-details)

## How Eltoo Works

### Traditional Lightning Channels

Traditional Lightning channels use **penalty-based mechanisms**:
- Each channel update creates a new commitment transaction
- Old commitments must be **revoked** with revocation keys
- If an old state is broadcast, the counterparty can **claim all funds** as penalty
- Requires storing all old states to detect fraud
- Complex watchtower protocol

### Eltoo Channels

Eltoo uses **update-based mechanisms**:
- Each update has a **monotonically increasing number**
- Any update can spend any previous update (using SIGHASH_NOINPUT)
- No penalty transactions - just broadcast latest state
- Only need to store **latest state**
- Simpler watchtower protocol

## Advantages Over Traditional Lightning

| Feature | Traditional Lightning | Eltoo |
|---------|---------------------|--------|
| Penalty Transactions | ✅ Required | ❌ Not needed |
| Revocation Keys | ✅ Required (one per update) | ❌ Not needed |
| State Storage | All states (for breach detection) | Latest state only |
| Watchtower Complexity | Complex (store all breach remedies) | Simple (store latest update) |
| Backup/Restore | Complex (need all revocation keys) | Simple (just latest state) |
| Channel Factories | Difficult | Easy |
| Update Flexibility | Limited | High |
| Security Model | Punitive | Non-punitive |

## Architecture

### Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Eltoo Channel Lifecycle                   │
└─────────────────────────────────────────────────────────────┘

1. Funding Transaction (On-Chain)
   ├─ Creates 2-of-2 multisig output
   └─ Locked by both parties' signatures

2. Update Transactions (Off-Chain, SIGHASH_NOINPUT)
   ├─ Update #1: Balances (A: 0.6, B: 0.4)
   ├─ Update #2: Balances (A: 0.5, B: 0.5) ← Can spend Update #1
   ├─ Update #3: Balances (A: 0.7, B: 0.3) ← Can spend Update #1 or #2
   └─ Update #N: Latest state

3. Settlement Transaction (Delayed by CSV)
   ├─ Spends latest update output
   ├─ Delayed by CheckSequenceVerify (e.g., 144 blocks)
   └─ Pays final balances to both parties
```

### Update Mechanism

```
Old Update (e.g., #5):
┌────────────────────────┐
│   Update Tx #5         │
│   Balances: A:60, B:40 │
└────────────────────────┘

New Update (#6) with SIGHASH_NOINPUT:
┌────────────────────────┐
│   Update Tx #6         │
│   Balances: A:55, B:45 │
│   ↓                    │
│   Can spend ANY        │
│   previous update      │
│   (including #1-5)     │
└────────────────────────┘
```

## API Reference

### EltooChannelManager

```cpp
#include "intcoin/eltoo.h"

using namespace intcoin::eltoo;

// Create manager
EltooChannelManager manager;

// Open channel
auto channel_id = manager.open_channel(
    peer_pubkey,
    1000000,    // Local funding: 1M sats
    500000,     // Remote funding: 500K sats
    144         // Settlement delay: 144 blocks (~1 day)
);

// Create update (no revocation needed!)
auto update = manager.create_update(
    channel_id,
    900000,     // New local balance
    600000      // New remote balance
);

// Send payment (automatically creates update)
manager.send_payment(channel_id, 100000);  // Send 100K sats

// Close channel
manager.close_channel_cooperative(channel_id);
```

### EltooUpdate Structure

```cpp
struct EltooUpdate {
    uint32_t update_number;           // Monotonically increasing
    uint64_t party_a_balance_sat;     // Party A balance
    uint64_t party_b_balance_sat;     // Party B balance

    Transaction update_tx;            // Update transaction
    Transaction settlement_tx;        // Settlement transaction
    uint32_t settlement_delay;        // CSV delay (blocks)

    DilithiumSignature party_a_sig;   // Signature (SIGHASH_NOINPUT)
    DilithiumSignature party_b_sig;   // Signature (SIGHASH_NOINPUT)
};
```

## Usage Examples

### Example 1: Opening a Channel

```cpp
#include "intcoin/eltoo.h"

EltooChannelManager manager;

// Alice opens channel with Bob
auto channel_id = manager.open_channel(
    bob_pubkey,
    2000000,    // Alice contributes 2M sats
    1000000,    // Bob contributes 1M sats
    144         // 1 day settlement delay
);

if (channel_id.has_value()) {
    std::cout << "Channel opened: " << channel_id->to_string() << std::endl;

    // Confirm funding transaction
    manager.confirm_funding(*channel_id, current_block_height);
}
```

### Example 2: Updating Channel State

```cpp
// Create new update (no revocation keys needed!)
auto update = manager.create_update(
    channel_id,
    1800000,    // Alice: 1.8M sats
    1200000     // Bob: 1.2M sats
);

if (update.has_value()) {
    std::cout << "Update #" << update->update_number << " created" << std::endl;

    // Sign with SIGHASH_NOINPUT
    auto sighash = manager.get_update_sighash(*update);
    auto signature = sign_sighash_noinput(alice_privkey, sighash);

    // Apply update
    manager.sign_update(channel_id, update->update_number, signature);
    manager.apply_update(channel_id, *update);
}
```

### Example 3: Sending Payments

```cpp
// Send payment (internally creates and applies update)
bool success = manager.send_payment(channel_id, 500000);  // 0.5M sats

if (success) {
    auto [local, remote] = manager.get_channel_balance(channel_id);
    std::cout << "New balances - Local: " << local
              << ", Remote: " << remote << std::endl;
}
```

### Example 4: Closing Channel

```cpp
// Cooperative close
if (manager.close_channel_cooperative(channel_id)) {
    std::cout << "Channel closed cooperatively" << std::endl;
}

// Or force close (broadcast latest update)
if (manager.close_channel_force(channel_id)) {
    std::cout << "Force close initiated" << std::endl;

    // After settlement delay, broadcast settlement
    // (wait for CSV timelock to expire)
    manager.broadcast_settlement(channel_id);
}
```

## Technical Details

### SIGHASH_NOINPUT

SIGHASH_NOINPUT is a signature flag that tells the signature to **not commit to the input** being spent:

**Normal Signature** commits to:
- Transaction ID (txid)
- Output index (vout)
- Sequence number
- All outputs

**SIGHASH_NOINPUT** commits to:
- ❌ NOT txid (can spend any previous update)
- ❌ NOT vout
- ❌ NOT sequence
- ✅ All outputs (settlement amounts)
- ✅ Locktime

This allows one update transaction to spend **any** previous update transaction!

### Update Transaction Structure

```
Update Transaction #N:
━━━━━━━━━━━━━━━━━━━━━━
Inputs:
  [0] Previous Update or Funding Output
      ↳ Can be ANY previous update due to SIGHASH_NOINPUT
      ↳ Signed by both parties

Outputs:
  [0] 2-of-2 Multisig
      ↳ Amount: Total channel capacity
      ↳ Spendable by:
         • Settlement Tx (after CSV delay)
         • Next Update Tx (immediately)

Locktime: 0
Signatures: SIGHASH_NOINPUT
```

### Settlement Transaction Structure

```
Settlement Transaction:
━━━━━━━━━━━━━━━━━━━━━━
Inputs:
  [0] Latest Update Output
      ↳ Requires CSV delay (e.g., 144 blocks)
      ↳ Signed by both parties

Outputs:
  [0] Party A's address
      ↳ Amount: Party A's final balance
  [1] Party B's address
      ↳ Amount: Party B's final balance

Locktime: 0
Sequence: CSV delay (relative timelock)
```

### Quantum Resistance

INTcoin's Eltoo implementation uses:
- **CRYSTALS-Dilithium5** for all signatures (including SIGHASH_NOINPUT)
- **SHA3-256** for all hashing operations
- Post-quantum secure update mechanisms

## Security Considerations

### Update Number Monotonicity

- Update numbers **must** increase monotonically
- Prevents replay of old states
- Enforced at protocol level

### Settlement Delay

- CSV delay prevents instant settlement
- Gives time to broadcast newer updates
- Typical delay: 144 blocks (~1 day)

### Backup and Restore

**Simple!** Just backup:
1. Latest update number
2. Current balances
3. Channel ID

**No need to store:**
- Old states
- Revocation keys
- Penalty transactions

### Watchtower Protocol

Much simpler than traditional Lightning:

```cpp
// Traditional watchtower (complex):
for each_state_update:
    encrypt_breach_remedy(old_state, revocation_key)
    upload_to_watchtower()

// Eltoo watchtower (simple):
watchtower.store_latest_update(channel_id, latest_update);
```

## Performance Benefits

| Metric | Traditional | Eltoo | Improvement |
|--------|------------|-------|-------------|
| Storage per Update | ~500 bytes | ~100 bytes | **80% reduction** |
| Backup Size | Grows with updates | Fixed | **Constant** |
| Watchtower Storage | O(n) updates | O(1) latest | **Linear to constant** |
| Update Complexity | O(n) | O(1) | **Faster updates** |

## Compatibility

### Works Great With:
- ✅ Channel Factories (simpler multi-party management)
- ✅ Splicing (easier capacity adjustments)
- ✅ PTLCs (enhanced privacy)
- ✅ Watchtowers (simpler protocol)

### Requirements:
- ⚠️ SIGHASH_NOINPUT soft fork (consensus change)
- ⚠️ CheckSequenceVerify (CSV) - already in Bitcoin/INTcoin

## Migration from Traditional Channels

To migrate existing channels to Eltoo:

1. **Close old channel** cooperatively
2. **Open new Eltoo channel** with same peer
3. **No data migration needed** (fresh start)

## Limitations

1. **Requires SIGHASH_NOINPUT**: Needs consensus upgrade
2. **Settlement Delay**: Always requires CSV delay (but same as traditional)
3. **Not Backward Compatible**: Can't mix Eltoo and traditional in same channel

## Future Enhancements

- [ ] Dynamic settlement delays based on channel value
- [ ] Optimized watchtower protocol
- [ ] Multi-party Eltoo channels (channel factories)
- [ ] SIGHASH_ANYPREVOUT support (alternative to NOINPUT)

## References

- [Eltoo Paper](https://blockstream.com/eltoo.pdf)
- [SIGHASH_NOINPUT BIP](https://github.com/bitcoin/bips/blob/master/bip-0118.mediawiki)
- [Lightning Network BOLTs](https://github.com/lightning/bolts)

## License

This Eltoo implementation is released under the MIT License.

Copyright (c) 2025 INTcoin Core (Maddison Lane)
