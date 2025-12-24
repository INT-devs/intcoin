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

## Using Lightning Network in Qt Wallet

INTcoin's desktop wallet provides a full-featured Lightning Network interface accessible via the graphical user interface.

### Accessing Lightning Features

1. Launch the Qt wallet:
   ```bash
   ./intcoin-qt
   ```

2. Navigate to the Lightning Network page:
   - Click **"Lightning Network"** in the toolbar (Alt+6)
   - Or use menu: **View → Lightning Network**

### Lightning Page Overview

The Lightning page features four tabs for managing all Lightning Network operations:

#### 1. Channels Tab

Manage payment channels with visualized channel data:

**Channel Table Columns**:
- **Channel ID**: Unique identifier (first 8 bytes shown)
- **Peer**: Remote node public key (truncated)
- **Status**: Active, Opening, Closing, or Closed
- **Capacity**: Total channel capacity in INT
- **Local Balance**: Your spendable funds
- **Remote Balance**: Partner's spendable funds
- **State**: Current channel state

**Opening a Channel**:
1. Click **"Open Channel"** button
2. Enter peer's Dilithium3 public key (1952 bytes, hex-encoded)
3. Set channel capacity (0.001 - 10 INT recommended)
4. Click OK - channel opens with on-chain confirmation

**Closing a Channel**:
1. Select channel from table
2. Click **"Close Channel"** button
3. Confirm closure (cooperative close preferred)
4. Funds return to on-chain wallet after confirmation

**Real-time Updates**:
- Channel list refreshes every 5 seconds
- Balance displays update automatically
- Status changes reflected immediately

#### 2. Send Payment Tab

Send Lightning Network payments using BOLT #11 invoices:

**Payment Workflow**:
1. Enter BOLT #11 payment request (lint1...)
2. Click **"Decode Invoice"** to verify details:
   - Amount in INTS
   - Payment description
   - Expiry time
   - Payee public key
   - Payment hash
3. Review decoded information
4. Click **"Send Payment"** to complete

**Features**:
- Invoice validation before payment
- Real-time payment status
- Payment hash displayed on success
- Payment history table shows recent transactions

**Amount Override**:
- Some invoices allow custom amounts
- Enter amount manually if invoice permits
- Add optional memo for record-keeping

#### 3. Receive Payment Tab

Create Lightning Network invoices for receiving payments:

**Creating an Invoice**:
1. Enter amount in INTS (1 INT = 1,000,000,000 INTS)
2. Add description/memo (optional, shown to payer)
3. Set expiry time (default: 3600 seconds / 1 hour)
4. Click **"Create Invoice"**

**Invoice Display**:
- Full BOLT #11 invoice string generated
- Click **"Copy Invoice"** to copy to clipboard
- QR code displayed (for mobile wallets - TODO: full implementation)
- Share invoice with payer via any channel

**Invoice History**:
- Table shows all generated invoices
- Track payment status (Pending, Paid, Expired)
- View invoice details and amounts

#### 4. Node Info Tab

View Lightning Network node information and statistics:

**Node Information**:
- **Status**: Running or Not Running
- **Public Key**: Your node's Dilithium3 public key (full 1952 bytes)
- **Network Address**: Node IP address
- **Port**: Lightning Network port (2213 mainnet, 12213 testnet)

**Channel Statistics**:
- **Total Channels**: All channels (all states)
- **Active Channels**: Currently open and operational
- **Pending Channels**: Opening or closing
- **Total Capacity**: Sum of all channel capacities
- **Local Balance**: Total funds you can send
- **Remote Balance**: Total funds you can receive

**Copy Node Info**:
- Click **"Copy Node Info"** to copy formatted details
- Share with peers for channel opening
- Includes public key, address, and port

### Keyboard Shortcuts

- **Alt+6**: Switch to Lightning Network page
- **F5**: Refresh channel data manually

### Best Practices for Qt Wallet

1. **Monitor Regularly**: Check channel balances and status frequently
2. **Backup Channels**: Use RPC commands for channel backups
3. **Start Small**: Test with small amounts before larger channels
4. **Keep Updated**: Balance information auto-refreshes every 5 seconds
5. **Verify Before Sending**: Always decode invoices before payment

### Troubleshooting Qt Wallet

**Lightning Not Available**:
- Check that Lightning Network is running
- Status shown in Node Info tab
- Restart daemon if needed

**Channel Not Opening**:
- Verify peer public key is correct (1952 bytes Dilithium3)
- Check on-chain balance for funding
- Wait for blockchain confirmation (2+ blocks)

**Payment Failing**:
- Decode invoice to verify it's valid
- Check channel has sufficient local balance
- Ensure recipient is reachable

**Balance Not Updating**:
- Auto-refresh runs every 5 seconds
- Click **"Refresh Channels"** for manual update
- Check Lightning Network daemon status

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
