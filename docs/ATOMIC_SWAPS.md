# INTcoin Atomic Swaps Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta - Testnet Recommended

---

## Table of Contents

1. [Introduction](#introduction)
2. [What are Atomic Swaps](#what-are-atomic-swaps)
3. [Supported Chains](#supported-chains)
4. [User Guide](#user-guide)
5. [RPC API Reference](#rpc-api-reference)
6. [Technical Details](#technical-details)
7. [Security](#security)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

---

## Introduction

Atomic swaps enable **trustless, peer-to-peer cryptocurrency exchanges** between different blockchains without requiring a centralized exchange or third-party intermediary. Using Hash Time-Locked Contracts (HTLCs), two parties can safely exchange assets across chains with cryptographic guarantees.

### Key Features

- 🔒 **Trustless** - No third-party custody of funds
- ⚡ **Direct** - Peer-to-peer exchange between users
- 🌐 **Cross-chain** - Trade BTC ↔ INT, LTC ↔ INT, BTC ↔ LTC
- 🔐 **Cryptographically secure** - HTLC protection via hash preimage
- ⏱️ **Time-bounded** - Automatic refunds if swap fails
- 💰 **No exchange fees** - Only network transaction fees

### Use Cases

- **Portfolio rebalancing** without using exchanges
- **Private trading** without KYC/AML
- **Cross-chain arbitrage** opportunities
- **Emergency liquidation** when exchanges are down
- **Testing cross-chain protocols** (developers)

---

## What are Atomic Swaps?

### The Problem

Traditional cryptocurrency trading requires:
1. **Centralized exchanges** - Custody of your funds, KYC/AML, fees, hacking risk
2. **Trust** - Counterparty could fail to deliver after receiving your coins
3. **Escrow services** - Still requires trusting a third party

### The Solution: HTLCs

**Hash Time-Locked Contracts (HTLCs)** solve this using:

1. **Hash Locks** - Require knowledge of a secret (preimage) to claim funds
2. **Time Locks** - Automatic refund if claim doesn't happen within timeframe
3. **Atomic execution** - Either both parties get their funds, or both get refunds

### How It Works (Simplified)

```
Alice (has 1 BTC) wants to swap with Bob (has 10,000 INT)

1. Alice generates a secret and creates HTLC on Bitcoin:
   "Bob can claim 1 BTC if he reveals the secret before 48 hours"

2. Bob sees Alice's HTLC and creates matching HTLC on INTcoin:
   "Alice can claim 10,000 INT if she reveals the secret before 24 hours"

3. Alice claims Bob's INT by revealing the secret

4. Bob sees the revealed secret and uses it to claim Alice's BTC

Result: Alice has 10,000 INT, Bob has 1 BTC, no trust required!
```

### Why It's "Atomic"

- Either **both** swaps complete successfully, or **neither** does
- Cannot result in one party receiving funds while the other doesn't
- If anything goes wrong, both parties get automatic refunds

---

## Supported Chains

### Bitcoin (BTC)

- **Status**: ✅ Fully supported
- **Minimum amount**: 0.001 BTC (~$40 at current prices)
- **Confirmations required**: 6 blocks (~60 minutes)
- **Typical fees**: 0.0001-0.0005 BTC depending on network congestion
- **Timeouts**: Initiator 48h, Participant 24h

### Litecoin (LTC)

- **Status**: ✅ Fully supported
- **Minimum amount**: 0.01 LTC
- **Confirmations required**: 24 blocks (~60 minutes)
- **Typical fees**: 0.001-0.01 LTC
- **Timeouts**: Initiator 48h, Participant 24h

### INTcoin (INT)

- **Status**: ✅ Native support
- **Minimum amount**: 1 INT
- **Confirmations required**: 6 blocks (~30 minutes)
- **Typical fees**: 0.001-0.01 INT
- **Timeouts**: Configured by counterparty

### Upcoming Support

- Ethereum (ETH) - Q2 2026
- Monero (XMR) - Q3 2026
- More chains as requested by community

---

## User Guide

### Prerequisites

1. **Run full nodes** for both chains involved in swap
   - Example: For BTC ↔ INT swap, run Bitcoin Core + intcoind
   - Or connect to trusted full node RPC

2. **Have sufficient funds** + fees on initiator chain
   - Include extra for transaction fees (0.5-1% recommended)

3. **Find a counterparty**
   - P2P marketplaces (coming soon)
   - Community channels
   - Over-the-counter (OTC) trading groups

4. **Agree on exchange rate and amounts**
   - Use current market rates as reference
   - Factor in fees and time value

### Creating a Swap (Initiator)

You want to swap BTC → INT

**Step 1: Create swap offer**

```bash
intcoin-cli createswap \
  '{
    "initiator_chain": "BTC",
    "initiator_amount": "0.1",
    "participant_chain": "INT",
    "participant_amount": "10000",
    "initiator_address": "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    "participant_address": "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    "timeout_hours": 48
  }'
```

**Response:**
```json
{
  "swap_id": "a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7",
  "secret_hash": "3f5a8b9c...",
  "status": "OFFERED",
  "htlc_address_btc": "bc1q...",
  "expires_at": "2026-01-04T12:00:00Z"
}
```

**Step 2: Share swap offer with counterparty**

```bash
intcoin-cli getswapstatus a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7
```

Share the `swap_id` with your counterparty.

**Step 3: Fund the HTLC**

```bash
bitcoin-cli sendtoaddress bc1q... 0.1
```

**Step 4: Monitor swap progress**

```bash
watch intcoin-cli monitorswap a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7
```

States will progress:
- OFFERED → INITIATED → PARTICIPANT_FUNDED → INITIATOR_CLAIMED → COMPLETED

**Step 5: Claim participant's funds**

Once participant has funded their HTLC:

```bash
intcoin-cli claimswap a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7
```

This reveals the secret and transfers participant's INT to you.

### Accepting a Swap (Participant)

You received a swap offer for INT → BTC

**Step 1: Review swap offer**

```bash
intcoin-cli getswapstatus a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7
```

Verify:
- Amounts are correct
- Exchange rate is acceptable
- Timeout is reasonable (at least 24 hours for you)
- Initiator has funded their HTLC

**Step 2: Accept swap**

```bash
intcoin-cli acceptswap a7b2c3d4-e5f6-4789-a0b1-c2d3e4f5a6b7
```

This creates your HTLC on INTcoin chain using the same secret hash.

**Step 3: Fund your HTLC**

```bash
intcoin-cli sendtoaddress int1q... 10000
```

**Step 4: Wait for initiator to claim**

Monitor swap status. When initiator claims your INT, the secret is revealed.

**Step 5: Claim initiator's funds**

```bash
bitcoin-cli importprivkey <revealed_secret>
intcoin-cli claimhtlc \
  --chain BTC \
  --htlc <htlc_address> \
  --secret <revealed_secret>
```

Initiator's BTC is now yours!

### Monitoring a Swap

**Check swap status:**

```bash
intcoin-cli getswapstatus <swap_id>
```

**Automated monitoring:**

```bash
intcoin-cli monitorswap <swap_id> --interval 60 --notify
```

This will:
- Check swap status every 60 seconds
- Send notifications on state changes
- Auto-claim when secret is revealed
- Alert if timeout approaching

### Canceling a Swap

**Before counterparty accepts:**

```bash
intcoin-cli cancelswap <swap_id>
```

**After timeout (automatic refund):**

Funds automatically refund to original address after timeout expires.

Manual refund:

```bash
intcoin-cli refundhtlc \
  --chain BTC \
  --htlc <htlc_address>
```

---

## RPC API Reference

### createswap

Create a new atomic swap offer.

**Parameters:**
```json
{
  "initiator_chain": "BTC|LTC|INT",
  "initiator_amount": "string (decimal)",
  "participant_chain": "BTC|LTC|INT",
  "participant_amount": "string (decimal)",
  "initiator_address": "string (receiving address)",
  "participant_address": "string (optional, counterparty receiving address)",
  "timeout_hours": "number (default: 48)"
}
```

**Returns:**
```json
{
  "swap_id": "string (UUID)",
  "secret_hash": "string (hex)",
  "status": "OFFERED",
  "htlc_address": "string (chain-specific)",
  "expires_at": "string (ISO 8601)",
  "initiator_chain": "string",
  "participant_chain": "string"
}
```

### acceptswap

Accept an atomic swap offer.

**Parameters:**
```json
{
  "swap_id": "string (UUID)"
}
```

**Returns:**
```json
{
  "swap_id": "string",
  "status": "ACCEPTED",
  "htlc_address": "string (your HTLC address)",
  "timeout": "number (hours until refund)"
}
```

### getswapstatus

Get current status of a swap.

**Parameters:**
- `swap_id` (string): Swap identifier

**Returns:**
```json
{
  "swap_id": "string",
  "status": "OFFERED|INITIATED|ACCEPTED|PARTICIPANT_FUNDED|INITIATOR_CLAIMED|PARTICIPANT_CLAIMED|COMPLETED|CANCELLED|REFUNDED",
  "initiator": {
    "chain": "string",
    "amount": "string",
    "htlc_address": "string",
    "funded": "boolean",
    "claimed": "boolean"
  },
  "participant": {
    "chain": "string",
    "amount": "string",
    "htlc_address": "string",
    "funded": "boolean",
    "claimed": "boolean"
  },
  "secret_hash": "string",
  "secret": "string (null until revealed)",
  "timeout": {
    "initiator": "string (ISO 8601)",
    "participant": "string (ISO 8601)"
  },
  "created_at": "string (ISO 8601)",
  "updated_at": "string (ISO 8601)"
}
```

### cancelswap

Cancel a swap before counterparty accepts.

**Parameters:**
- `swap_id` (string): Swap identifier

**Returns:**
```json
{
  "swap_id": "string",
  "status": "CANCELLED",
  "refunded": "boolean"
}
```

### monitorswap

Monitor swap progress with automatic claiming.

**Parameters:**
```json
{
  "swap_id": "string",
  "interval": "number (seconds, default: 60)",
  "auto_claim": "boolean (default: true)",
  "notify": "boolean (default: false)"
}
```

**Returns:** Stream of status updates

---

## Technical Details

### HTLC Script Structure

#### Bitcoin HTLC Script

```
OP_IF
    OP_SHA256 <secret_hash> OP_EQUALVERIFY
    OP_DUP OP_HASH160 <participant_pubkey_hash> OP_EQUALVERIFY OP_CHECKSIG
OP_ELSE
    <timeout> OP_CHECKLOCKTIMEVERIFY OP_DROP
    OP_DUP OP_HASH160 <initiator_pubkey_hash> OP_EQUALVERIFY OP_CHECKSIG
OP_ENDIF
```

**Claim path** (if secret known):
- Provide secret + signature
- Secret must hash to `secret_hash`
- Signature must be from participant

**Refund path** (after timeout):
- Wait until `timeout` blocks/time
- Provide signature from initiator

#### INTcoin HTLC Script

Similar structure but uses Dilithium3 signatures instead of ECDSA:

```
IF
    SHA3-256 <secret_hash> EQUALVERIFY
    DILITHIUM3_VERIFY <participant_pubkey>
ELSE
    <timeout> CHECKSEQUENCEVERIFY DROP
    DILITHIUM3_VERIFY <initiator_pubkey>
ENDIF
```

### Hash Algorithm Support

- **SHA-256** - Bitcoin, Litecoin compatibility
- **SHA3-256** - INTcoin native, more quantum-resistant
- **RIPEMD160** - For certain chain compatibility

### State Machine

Atomic swap progresses through these states:

```
OFFERED
  ↓ (initiator funds HTLC)
INITIATED
  ↓ (participant accepts)
ACCEPTED
  ↓ (participant funds HTLC)
PARTICIPANT_FUNDED
  ↓ (initiator claims with secret)
INITIATOR_CLAIMED (secret now revealed)
  ↓ (participant claims with revealed secret)
PARTICIPANT_CLAIMED
  ↓
COMPLETED ✅

Or at any point:
  ↓ (timeout expires)
REFUNDED ❌
```

### Timeout Configuration

**Initiator timeout** must be **longer** than participant timeout to prevent griefing.

Recommended:
- Initiator: 48 hours (2880 blocks for Bitcoin)
- Participant: 24 hours (1440 blocks for Bitcoin)

This gives participant time to claim after seeing the secret, even if initiator delays.

### Fee Handling

Each party pays fees on their own chain:
- Initiator pays BTC network fees
- Participant pays INT network fees
- No additional swap fees

Recommended fee buffer: 0.5-1% of swap amount for unexpected fee spikes.

---

## Security

### HTLC Security Guarantees

✅ **Cannot be double-spent**
- HTLCs are on-chain, protected by blockchain consensus

✅ **Secret remains private until claim**
- Preimage only revealed when claiming

✅ **Automatic refunds**
- If swap fails, both parties get refunds after timeout

✅ **No third-party custody**
- Funds never held by intermediary

### Risks and Mitigations

**Risk: Exchange Rate Volatility**
- Prices may change during swap (typically 1-2 hours)
- **Mitigation**: Set acceptable price range, use short timeouts

**Risk: Chain Reorganization**
- Deep reorg could invalidate HTLCs
- **Mitigation**: Wait for sufficient confirmations (BTC: 6, LTC: 24, INT: 6)

**Risk: Timeout Griefing**
- Counterparty could delay until your timeout, then refund
- **Mitigation**: Initiator timeout > participant timeout, monitor actively

**Risk: Secret Preimage Leakage**
- If secret leaked before HTLCs are set up, participant could claim without creating their HTLC
- **Mitigation**: Never share secret, let protocol reveal it automatically

**Risk: Insufficient Fees**
- Transaction might not confirm before timeout
- **Mitigation**: Use elevated fees, monitor mempool, RBF if supported

### Best Practices

1. **Start with small amounts** on testnet
2. **Use reputable counterparties** (check reputation/history)
3. **Verify all parameters** before funding HTLC
4. **Monitor swap actively** - don't rely solely on automation
5. **Keep nodes online and synced** throughout swap
6. **Use trusted RPC endpoints** if not running full node
7. **Set conservative timeouts** - allow margin for delays
8. **Test with testnet first** - always test flow before mainnet

---

## Troubleshooting

### Swap Stuck in INITIATED

**Cause**: Participant hasn't accepted yet

**Solutions**:
1. Wait for participant response
2. Check that counterparty has swap_id
3. Verify swap parameters are acceptable
4. If no response after reasonable time, cancel swap

### Swap Stuck in PARTICIPANT_FUNDED

**Cause**: Initiator hasn't claimed yet

**Solutions**:
1. Check that initiator's node is online and synced
2. Verify sufficient confirmations on participant's HTLC
3. Wait or contact initiator
4. As participant: Monitor for secret revelation, auto-claim when possible

### "Insufficient Confirmations" Error

**Cause**: HTLC transaction hasn't confirmed yet

**Solutions**:
1. Wait for required confirmations (BTC: 6, LTC: 24, INT: 6)
2. Check transaction on block explorer
3. If stuck: Increase fee (if RBF enabled)

### Timeout Approaching

**Warning**: If timeout approaches before claim:

**As Initiator**:
- If participant hasn't funded, prepare for refund
- If participant funded but you can't claim, investigate immediately

**As Participant**:
- If initiator claimed (secret revealed), claim immediately
- If initiator hasn't claimed, prepare for automatic refund

**Recovery**:
```bash
intcoin-cli refundhtlc --chain <CHAIN> --htlc <ADDRESS>
```

### Claim Transaction Failed

**Causes**:
- Incorrect secret
- Insufficient fees
- Timeout already passed
- HTLC already claimed

**Solutions**:
1. Verify secret is correct
2. Increase transaction fee
3. Check HTLC status on block explorer
4. If timeout passed, initiate refund instead

### Node Offline During Swap

**Impact**: Cannot monitor or auto-claim

**Solutions**:
1. Restart node immediately
2. Sync blockchain
3. Check swap status
4. Manually claim if secret revealed
5. For future: Use automated monitoring service

---

## FAQ

### How long does an atomic swap take?

Typical timeline:
- **Setup**: 10-30 minutes (creating HTLCs)
- **Confirmations**: 30-120 minutes (depending on chains)
- **Claims**: 10-30 minutes
- **Total**: 1-3 hours for full swap

### Can I cancel a swap after it starts?

- **Before counterparty accepts**: Yes, via `cancelswap`
- **After both HTLCs funded**: No, must wait for timeout
- **After timeout**: Automatic refund

### What if the other person disappears?

If counterparty disappears:
- **Before funding**: Cancel swap
- **After you funded**: Wait for timeout, get automatic refund
- **After they funded**: Wait for timeout or claim if secret revealed

### Do both parties need to be online?

Yes, for:
- Creating and accepting swap
- Claiming funds

No, for:
- Refunds (automatic after timeout)
- Monitoring (can use automated service)

### Are atomic swaps anonymous?

**Semi-anonymous**:
- On-chain HTLCs are public (visible on block explorers)
- Linkage between chains possible (same secret hash)
- No KYC/AML required
- Better privacy than centralized exchanges

For maximum privacy, use privacy-focused coins (Monero support coming).

### What happens if Bitcoin forks during a swap?

- HTLCs remain valid on longest chain
- Monitor for deep reorganizations
- Wait for extra confirmations if fork risk
- Worst case: Refund after timeout

### Can I swap more than 2 chains at once?

Not currently supported. For multi-chain swaps:
1. Perform BTC → INT swap
2. Then perform INT → LTC swap
3. Two separate atomic swaps

### What are the fees?

Only network transaction fees:
- **Bitcoin**: ~$1-10 depending on network congestion
- **Litecoin**: ~$0.01-0.50
- **INTcoin**: ~$0.01-0.10

No atomic swap protocol fees.

### Is this safe for large amounts?

Start with small test swaps. For large amounts:
- Use trusted, reputable counterparties
- Test swap process first with small amounts
- Consider doing swap in smaller chunks
- Use multi-signature escrow for very large amounts

---

## Resources

- **HTLC Technical Specification**: [docs/HTLC_SPECIFICATION.md](HT LC_SPECIFICATION.md)
- **RPC API Documentation**: [docs/RPC.md](RPC.md)
- **Source Code**: [src/swap/atomic_swap.cpp](../src/swap/atomic_swap.cpp)
- **Bitcoin Integration**: [src/blockchain/bitcoin_monitor.cpp](../src/blockchain/bitcoin_monitor.cpp)
- **Litecoin Integration**: [src/blockchain/litecoin_monitor.cpp](../src/blockchain/litecoin_monitor.cpp)
- **Testnet Coins**: https://faucet.intcoin.org

---

## Support

For atomic swap assistance:
- **GitHub Issues**: https://github.com/InternationalCoin/intcoin/issues
- **Discussions**: https://github.com/InternationalCoin/intcoin/discussions
- **Email**: swaps@international-coin.org

**Security Issues**: security@international-coin.org (PGP encrypted)

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
**Status**: Production Beta - Testnet Recommended
