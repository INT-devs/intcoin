# INTcoin Atomic Swaps Tutorial

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Difficulty**: Advanced
**Time Required**: 60 minutes

Learn how to perform trustless cross-chain atomic swaps between INTcoin and other cryptocurrencies.

---

## What is an Atomic Swap?

An **atomic swap** is a smart contract technology that enables the exchange of one cryptocurrency for another **without using centralized intermediaries** (exchanges). The swap either happens completely or not at all - hence "atomic".

### How It Works

1. **Hash Time Locked Contracts (HTLC)**: Both parties lock funds in contracts
2. **Secret Exchange**: One party reveals a secret to claim funds
3. **Timelock**: If swap fails, funds are refunded after timeout
4. **Trustless**: No third party needed - cryptography ensures fairness

### Supported Blockchains

| Blockchain | Status | Notes |
|------------|--------|-------|
| **Bitcoin (BTC)** | ✅ Production | Fully tested |
| **Litecoin (LTC)** | ✅ Production | Fully tested |
| **Monero (XMR)** | ⚠️ Experimental | Use with caution |

---

## Prerequisites

**Requirements**:
- INTcoin node v1.2.0-beta (with atomic swaps enabled)
- Wallet on target blockchain (BTC, LTC, or XMR)
- Basic understanding of blockchain transactions
- Patience (swaps can take 30-60 minutes)

**Skills Needed**:
- Command line proficiency
- Understanding of transaction fees
- Risk management (start with small amounts!)

---

## Part 1: Setup

### Step 1: Enable Atomic Swaps

Edit INTcoin configuration:

```bash
nano ~/.intcoin/intcoin.conf
```

Add these lines:

```ini
# Enable atomic swaps
atomic_swaps.enabled=1
atomic_swaps.chains=BTC,LTC,XMR

# HTLC settings
htlc.min_locktime=24          # Minimum locktime (blocks)
htlc.max_locktime=288         # Maximum locktime (blocks)
```

Restart node:

```bash
intcoin-cli stop
intcoind -daemon
```

### Step 2: Verify Setup

```bash
# Check atomic swap support
intcoin-cli getnetworkinfo | grep atomic

# Should see: "atomic_swaps": true
```

### Step 3: Fund Your Wallet

Ensure you have enough INT for the swap plus fees:

```bash
# Check balance
intcoin-cli getbalance

# If needed, get a receiving address
intcoin-cli getnewaddress "Swap Funds"
```

**Recommended starting amount**: 1-5 INT for first swap

---

## Part 2: Swap INT → BTC

This example swaps **10 INT for 0.001 BTC**.

### Step 1: Find a Counterparty

**Option 1: Use Swap Exchange**
- Visit: https://atomic.intcoin.org
- Create swap offer
- Wait for match

**Option 2: Manual (P2P)**
- Find trading partner on Discord/Telegram
- Agree on terms: amounts, timelocks, fees
- Exchange contact info

**For this tutorial**, we'll assume you found a counterparty who agrees to:
- Give you: 0.001 BTC
- Receive from you: 10 INT
- Locktime: 48 blocks (~48 minutes)

### Step 2: Initiate Swap

As the **initiator**, you start the swap:

```bash
intcoin-cli initiate_atomic_swap \
  "BTC" \
  10.0 \
  0.001 \
  "bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh" \
  48
```

Parameters:
- `chain`: "BTC" (target blockchain)
- `amount`: 10.0 (INT to send)
- `counterparty_amount`: 0.001 (BTC to receive)
- `counterparty_address`: Bitcoin address to receive BTC
- `locktime`: 48 blocks

**Output**:
```json
{
  "swap_id": "abc123def456...",
  "secret_hash": "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
  "htlc_address": "INT1qhtlc2kgdygjrsqtzq2n0yrf2493p83kkfjhxabc",
  "funding_txid": "tx123abc...",
  "status": "initiated"
}
```

**🔐 CRITICAL**: Save the `swap_id` and full output!

### Step 3: Share Details with Counterparty

Send your counterparty:
- ✅ `secret_hash`
- ✅ `htlc_address`
- ✅ `funding_txid`
- ✅ Your Bitcoin address (to receive BTC)
- ❌ **NEVER share the secret (preimage)**

### Step 4: Wait for Counterparty's HTLC

The counterparty will create their own HTLC on Bitcoin blockchain using your `secret_hash`.

They should send you:
- Their BTC HTLC address
- Their BTC funding transaction ID

**Verify on blockchain explorer**:
```bash
# Check Bitcoin blockchain explorer
# Example: https://blockstream.info/tx/THEIR_BTC_TXID
```

Confirm:
- ✅ Amount matches (0.001 BTC)
- ✅ Hash matches your `secret_hash`
- ✅ Locktime is reasonable (e.g., 24 blocks on BTC)

### Step 5: Redeem Bitcoin (Reveal Secret)

Once counterparty's HTLC is confirmed (1+ confirmations):

```bash
# Redeem BTC (this reveals your secret)
intcoin-cli redeem_atomic_swap \
  "abc123def456..." \
  "your-secret-preimage-here"
```

**🔐 Important**: Redeeming reveals the secret to the counterparty!

**Output**:
```json
{
  "swap_id": "abc123def456...",
  "redeem_txid": "btc_tx789...",
  "status": "completed_by_you"
}
```

### Step 6: Counterparty Redeems INT

The counterparty now sees your secret (from the BTC blockchain) and uses it to redeem the INT from your HTLC:

```bash
# They run (you don't):
intcoin-cli redeem_atomic_swap \
  "abc123def456..." \
  "your-revealed-secret"
```

### Step 7: Verify Completion

Check swap status:

```bash
intcoin-cli get_atomic_swap_status "abc123def456..."
```

**Output**:
```json
{
  "swap_id": "abc123def456...",
  "status": "completed",
  "your_redeemed": true,
  "counterparty_redeemed": true,
  "completion_time": 1735171200
}
```

✅ **Success!** You swapped 10 INT for 0.001 BTC trustlessly!

---

## Part 3: Swap BTC → INT

This is the reverse: you're **receiving INT** and **sending BTC**.

### Step 1: Counterparty Initiates

The counterparty (who has INT) initiates the swap:

They run (you wait):
```bash
intcoin-cli initiate_atomic_swap "BTC" 5.0 0.0005 "YOUR_BTC_ADDRESS" 48
```

They send you:
- `secret_hash`
- `htlc_address` (INT address)
- `funding_txid` (INT transaction)

### Step 2: Verify INT HTLC

```bash
# Check their HTLC transaction
intcoin-cli gettransaction "their_funding_txid"

# Verify:
# - Amount: 5.0 INT
# - Hash: matches their secret_hash
# - Locktime: reasonable (48 blocks)
```

### Step 3: Create Your BTC HTLC

**On Bitcoin Core** (or compatible wallet):

```bash
# Create HTLC (example using Bitcoin Core)
bitcoin-cli createhtlc \
  --secret-hash "their_secret_hash" \
  --amount 0.0005 \
  --locktime 24 \
  --recipient "their_btc_address"
```

**Output**: BTC HTLC address and funding transaction

### Step 4: Share Your BTC HTLC

Send counterparty:
- Your BTC HTLC address
- Your BTC funding txid
- Confirmations (wait for 1+ confirmations)

### Step 5: Counterparty Redeems BTC (Reveals Secret)

They redeem your BTC HTLC, revealing the secret.

**Monitor Bitcoin blockchain** for their redemption transaction.

### Step 6: Extract Secret from BTC Blockchain

```bash
# Use blockchain explorer or Bitcoin Core
bitcoin-cli getrawtransaction "their_btc_redeem_txid" 1

# Look for the secret in scriptSig or witness data
```

The secret will be visible in the transaction data.

### Step 7: Redeem INT

Use the secret to redeem INT:

```bash
intcoin-cli redeem_atomic_swap \
  "swap_id_from_step1" \
  "extracted_secret"
```

✅ **Success!** You received 5 INT for 0.0005 BTC!

---

## Part 4: Handling Failures

### Scenario: Counterparty Doesn't Follow Through

If the counterparty never creates their HTLC or disappears:

**Wait for timelock expiration** (e.g., 48 blocks), then refund:

```bash
# Check if locktime passed
intcoin-cli get_atomic_swap_status "swap_id"

# If expired, refund
intcoin-cli refund_atomic_swap "swap_id"
```

**Output**:
```json
{
  "swap_id": "abc123...",
  "refund_txid": "refund_tx456...",
  "status": "refunded"
}
```

Your funds are safely returned.

### Scenario: You Made a Mistake

**Before revealing secret**:
- ✅ Can refund after locktime expires

**After revealing secret**:
- ❌ Cannot refund (secret is public)
- ⚠️ Counterparty will claim your INT

**Prevention**: Always verify counterparty's HTLC before redeeming!

---

## Part 5: Best Practices

### Security

1. ✅ **Start small**: First swap should be <$100 value
2. ✅ **Verify everything**: Check addresses, amounts, hashes
3. ✅ **Use reputable counterparties**: Check reputation/reviews
4. ✅ **Never share secret** until counterparty's HTLC is confirmed
5. ✅ **Save all details**: Keep swap_id, secret, addresses

### Timelocks

**Recommended locktime settings**:

| Blockchain | Blocks | Approximate Time |
|------------|--------|------------------|
| **INTcoin (initiator)** | 48 | ~48 minutes |
| **Bitcoin (participant)** | 24 | ~4 hours |
| **Litecoin (participant)** | 96 | ~4 hours |

**Rule**: Participant's locktime should be **shorter** than initiator's.

**Why**: Prevents participant from holding funds hostage.

### Fees

**Budget for fees on both chains**:

| Blockchain | Typical Fee | Notes |
|------------|-------------|-------|
| **INTcoin** | 0.0001 INT | Low priority sufficient |
| **Bitcoin** | 5-50 sat/vB | Check mempool before swapping |
| **Litecoin** | 1-10 sat/vB | Usually very low |

**Total cost**: ~$2-10 per swap (depends on BTC fees)

### Common Mistakes

❌ **DON'T**:
- Redeem before verifying counterparty's HTLC
- Use addresses you don't control
- Swap with untrusted parties for large amounts
- Ignore locktime settings

✅ **DO**:
- Test with small amounts first
- Double-check all addresses
- Save all transaction IDs and secrets
- Wait for confirmations

---

## Part 6: Advanced Features

### Multi-Chain Swaps

Swap INT → LTC → BTC (triangular arbitrage):

```bash
# 1. INT → LTC
intcoin-cli initiate_atomic_swap "LTC" 10 0.1 "ltc_address" 48

# 2. LTC → BTC (on Litecoin wallet)
litecoin-cli initiate_atomic_swap "BTC" 0.1 0.001 "btc_address" 48

# 3. Complete both swaps
```

### Automated Market Making

Create a script to automatically accept swaps at specific rates:

```bash
#!/bin/bash
# Auto-accept swaps for 1 INT = 0.0001 BTC

while true; do
  # Check for new swap offers
  offers=$(intcoin-cli list_atomic_swap_offers)

  # Accept if rate is favorable
  # ... (implementation left as exercise)

  sleep 60
done
```

See: [ATOMIC_SWAPS.md](ATOMIC_SWAPS.md) for API details

---

## Troubleshooting

### "Atomic swaps not enabled"

**Solution**:
```bash
# Check config
grep atomic_swaps ~/.intcoin/intcoin.conf

# Enable if missing
echo "atomic_swaps.enabled=1" >> ~/.intcoin/intcoin.conf
intcoin-cli stop && intcoind -daemon
```

### "Secret hash mismatch"

**Cause**: You or counterparty used wrong secret/hash

**Solution**:
- Verify both parties are using the same `secret_hash`
- Regenerate swap if needed
- Wait for locktime and refund

### "Timelock not yet expired"

**Cause**: Trying to refund before locktime passes

**Solution**:
```bash
# Check current block height
intcoin-cli getblockcount

# Check HTLC locktime
intcoin-cli get_atomic_swap_status "swap_id"

# Wait until: current_height >= locktime_height
```

### "Transaction broadcast failed"

**Cause**: Insufficient fees or network congestion

**Solution**:
```bash
# Increase fee
intcoin-cli settxfee 0.0002  # Higher fee

# Retry
intcoin-cli redeem_atomic_swap "swap_id" "secret"
```

---

## RPC Reference

### List Active Swaps

```bash
intcoin-cli list_atomic_swaps
```

### Get Swap Status

```bash
intcoin-cli get_atomic_swap_status "swap_id"
```

### Cancel Pending Swap

```bash
# Only works if not yet broadcasted
intcoin-cli cancel_atomic_swap "swap_id"
```

---

## Resources

**Documentation**:
- [Atomic Swaps Deep Dive](ATOMIC_SWAPS.md)
- [HTLC Technical Specification](https://en.bitcoin.it/wiki/Hash_Time_Locked_Contracts)
- [Cross-Chain Swaps Research](https://arxiv.org/abs/1801.09515)

**Tools**:
- Atomic Swap Explorer: https://atomic-swaps.intcoin.org
- Testnet Faucets: Get free testnet coins for practice

**Community**:
- Discord: #atomic-swaps channel
- Forum: https://forum.intcoin.org/atomic-swaps

---

**Congratulations!** 🎉 You can now perform trustless cross-chain atomic swaps!

**⚠️ Warning**: Atomic swaps are advanced. Always test on testnet first!

**Last Updated**: January 2, 2026
