# Lightning Network User Guide

Complete guide to using Lightning Network on INTcoin for instant, low-fee payments.

---

## Quick Start

### 1. Start Lightning Node

```bash
# Start intcoind with Lightning enabled
./intcoind --lightning

# Or start Qt wallet (Lightning integrated)
./intcoin-qt
```

### 2. Open a Channel

```bash
# Via RPC
intcoin-cli lightning_openchannel \
  <node_pubkey> \
  <capacity_in_ints> \
  [push_amount]

# Example: Open 1 INT channel, push 0.5 INT to remote
intcoin-cli lightning_openchannel \
  02a1b2c3d4... \
  1000000 \
  500000
```

### 3. Send Payment

```bash
# Pay a BOLT11 invoice
intcoin-cli lightning_payinvoice lint1...

# Send to node directly
intcoin-cli lightning_sendpayment \
  <dest_pubkey> \
  <amount_msat> \
  "Description"
```

### 4. Receive Payment

```bash
# Create invoice
intcoin-cli lightning_createinvoice \
  <amount_msat> \
  "Description"

# Returns: lint1... (share with payer)
```

---

## Concepts

### Payment Channels

Channels are bidirectional payment conduits:
- **Capacity**: Total funds locked in channel
- **Local Balance**: Your spendable funds
- **Remote Balance**: Partner's spendable funds
- **Reserve**: Minimum balance (security deposit)

### HTLCs (Hash Time-Locked Contracts)

Conditional payments secured by:
- **Hash Lock**: Payment preimage required
- **Time Lock**: Refund after timeout

### Routing

Multi-hop payments via onion routing:
```
Alice --[HTLC]--> Bob --[HTLC]--> Carol
```
- Privacy: Bob doesn't know Alice pays Carol
- Atomicity: All-or-nothing payment
- Fees: Each hop charges routing fee

---

## Channel Management

### Opening Channels

**Choose Partners**:
- Well-connected nodes (good routing)
- Reliable uptime (availability)
- Reasonable fees (cost-effective)

**Recommended Capacity**:
- Small: 0.1 - 1 INT (testing, small payments)
- Medium: 1 - 10 INT (regular use)
- Large: 10+ INT (routing node, merchants)

### Closing Channels

**Cooperative Close** (recommended):
```bash
intcoin-cli lightning_closechannel <channel_id>
```
- Fastest settlement
- Lowest fees
- Both parties agree

**Force Close** (uncooperative):
```bash
intcoin-cli lightning_closechannel <channel_id> --force
```
- Slower (CSV delay)
- Higher fees
- Use when partner offline

---

## Payments

### Creating Invoices

```bash
# Basic invoice
intcoin-cli lightning_createinvoice 100000 "Coffee"

# Invoice with expiry
intcoin-cli lightning_createinvoice \
  100000 \
  "Coffee" \
  --expiry=3600

# Invoice with route hints (private channels)
intcoin-cli lightning_createinvoice \
  100000 \
  "Coffee" \
  --routehints=true
```

### Paying Invoices

```bash
# Decode invoice first (optional)
intcoin-cli lightning_decodeinvoice lint1...

# Pay invoice
intcoin-cli lightning_payinvoice lint1...

# Pay with fee limit
intcoin-cli lightning_payinvoice lint1... --feeppm=1000
```

### Multi-Path Payments

Split large payments across multiple routes:
```bash
# Automatic MPP (if supported)
intcoin-cli lightning_payinvoice lint1... --mpp=auto

# Manual split
intcoin-cli lightning_sendmpp \
  <invoice> \
  <num_paths>
```

---

## Security

### Channel Backups

**Static Channel Backup (SCB)**:
```bash
# Export backup
intcoin-cli lightning_exportbackup > channels.backup

# Restore from backup
intcoin-cli lightning_restorebackup channels.backup
```

**Important**: Backup after every channel update!

### Watchtowers

Protect against cheating while offline:
```bash
# Register with watchtower
intcoin-cli lightning_registerwatchtower \
  <tower_pubkey> \
  <tower_address>

# List watchtowers
intcoin-cli lightning_listwatchtowers
```

### Best Practices

1. **Never lose channel state** (backup regularly)
2. **Monitor channels** (check balance, status)
3. **Use watchtowers** (if running merchant node)
4. **Start small** (test with small amounts)
5. **Verify invoices** (decode before paying)

---

## Routing Node

### Requirements

- **Uptime**: 99%+ availability
- **Bandwidth**: High-speed internet
- **Capital**: 10+ INT for multiple channels
- **Monitoring**: 24/7 alerting

### Setup

1. **Open balanced channels**:
   ```bash
   # Open channel with push amount
   intcoin-cli lightning_openchannel \
     <node> 5000000 2500000
   ```

2. **Set routing fees**:
   ```bash
   intcoin-cli lightning_setfees \
     --base=1000 \
     --rate=1
   ```

3. **Optimize routing**:
   ```bash
   # Enable autopilot
   intcoin-cli lightning_autopilot true
   
   # Set channel targets
   intcoin-cli lightning_autopilot \
     --channels=10 \
     --capacity=50000000
   ```

### Fee Strategy

**Base Fee**: Fixed per-payment (default: 1000 msat = 1 sat)
**Fee Rate**: Proportional (default: 1 ppm = 0.0001%)

Example: 100,000 sat payment
- Base: 1 sat
- Rate: 100,000 * 0.000001 = 0.1 sat
- Total: 1.1 sat

---

## Troubleshooting

### Channel Stuck Opening

**Problem**: Channel funding transaction unconfirmed

**Solutions**:
1. Wait for confirmation (2+ blocks)
2. Check mempool: `intcoin-cli getmempoolinfo`
3. Bump fee (if RBF enabled)
4. Cancel if too long: `intcoin-cli lightning_abandonchannel`

### Payment Failed

**Reasons**:
- Insufficient capacity
- No route found
- Recipient offline
- Fee limit too low

**Solutions**:
```bash
# Check route
intcoin-cli lightning_findroute <dest> <amount>

# Increase fee limit
intcoin-cli lightning_payinvoice <invoice> --feeppm=5000

# Use MPP
intcoin-cli lightning_payinvoice <invoice> --mpp=auto
```

### Channel Force Closed

**Check reason**:
```bash
intcoin-cli lightning_listclosedchannels
```

**Common causes**:
- Partner offline (timeout)
- Protocol violation (penalty)
- Network error (connection lost)

---

## Advanced Features

### Hold Invoices

Create invoice, hold payment until manual settlement:
```bash
# Create hold invoice
intcoin-cli lightning_createholdinvoice 100000 "Escrow"

# Settle when ready
intcoin-cli lightning_settleinvoice <payment_hash>

# Or cancel
intcoin-cli lightning_cancelinvoice <payment_hash>
```

### Keysend (Spontaneous Payments)

Send without invoice:
```bash
intcoin-cli lightning_keysend \
  <dest_pubkey> \
  <amount_msat> \
  [message]
```

### Channel Rebalancing

Move funds within channel network:
```bash
# Circular rebalancing
intcoin-cli lightning_rebalance \
  <channel_id> \
  <amount> \
  --feeppm=100
```

---

## Monitoring

### Channel Status

```bash
# List all channels
intcoin-cli lightning_listchannels

# Channel details
intcoin-cli lightning_getchannel <channel_id>

# Channel balance
intcoin-cli lightning_getbalance
```

### Payment History

```bash
# List payments
intcoin-cli lightning_listpayments

# Payment details
intcoin-cli lightning_getpayment <payment_hash>

# Invoices
intcoin-cli lightning_listinvoices
```

### Network Stats

```bash
# Network graph
intcoin-cli lightning_describegraph

# Node info
intcoin-cli lightning_getnodeinfo <pubkey>

# Routing stats
intcoin-cli lightning_routingstats
```

---

## RPC Reference

### Channel Management

| Command | Description |
|---------|-------------|
| `lightning_openchannel` | Open new channel |
| `lightning_closechannel` | Close channel |
| `lightning_listchannels` | List channels |
| `lightning_getchannel` | Get channel details |

### Payments

| Command | Description |
|---------|-------------|
| `lightning_createinvoice` | Generate invoice |
| `lightning_decodeinvoice` | Decode invoice |
| `lightning_payinvoice` | Pay invoice |
| `lightning_sendpayment` | Send payment |
| `lightning_listpayments` | List payments |

### Network

| Command | Description |
|---------|-------------|
| `lightning_getnodeinfo` | Get node info |
| `lightning_listpeers` | List Lightning peers |
| `lightning_connect` | Connect to Lightning peer |
| `lightning_disconnect` | Disconnect from peer |

---

## FAQ

**Q: What's the difference between on-chain and Lightning?**
A: On-chain is slow but final; Lightning is instant but requires channels.

**Q: Are Lightning payments final?**
A: Yes, once settled (preimage revealed), payment is final.

**Q: Can I lose funds in Lightning?**
A: Only if you lose channel backup AND partner broadcasts revoked state.

**Q: How much can I send?**
A: Limited by smallest channel balance in route.

**Q: Do I need to be online?**
A: To send/receive, yes. Use watchtowers for offline protection.

**Q: What happens if my node crashes?**
A: Channels remain safe if you have backup. Restore from backup.

---

## Resources

- BOLT Specification: docs/BOLT_SPECIFICATION.md
- Architecture: docs/ARCHITECTURE.md
- RPC API: docs/RPC.md
- Community: Discord, Telegram

---

*Last Updated: December 23, 2025*
