# INTcoin Qt Wallet User Guide

**Version**: 1.0.0-alpha
**Last Updated**: December 24, 2025

Complete guide to using the INTcoin Qt desktop wallet for managing your INT coins and Lightning Network channels.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Overview Page](#overview-page)
3. [Sending Coins](#sending-coins)
4. [Receiving Coins](#receiving-coins)
5. [Transaction History](#transaction-history)
6. [Address Book](#address-book)
7. [Lightning Network](#lightning-network)
8. [Settings](#settings)
9. [Wallet Management](#wallet-management)
10. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Installing the Qt Wallet

**Linux/FreeBSD**:
```bash
# After building from source
sudo make install

# Launch wallet
intcoin-qt
```

**macOS**:
```bash
# Launch from Applications or terminal
/Applications/INTcoin-Qt.app/Contents/MacOS/INTcoin-Qt
```

**Windows**:
```
Double-click intcoin-qt.exe from installation directory
```

### First Launch

1. **Initial Synchronization**:
   - The wallet will begin downloading the blockchain
   - This may take several hours depending on network speed
   - You can use the wallet while syncing, but balances won't be accurate until fully synced

2. **Creating Your First Wallet**:
   - If no wallet exists, you'll be prompted to create one
   - Choose a strong passphrase (optional but recommended)
   - Save your recovery mnemonic (24 words) in a secure location
   - **Never share your mnemonic with anyone!**

3. **Wallet Encryption** (Recommended):
   - Go to **Wallet → Encrypt Wallet**
   - Choose a strong passphrase
   - **Write down your passphrase and store it securely**
   - You'll need this to send coins and unlock your wallet

### Main Window Layout

```
┌─────────────────────────────────────────────────────────┐
│ File  Wallet  View  Help                                │
├─────────────────────────────────────────────────────────┤
│ [Overview] [Send] [Receive] [Transactions] [Addresses]  │
│ [Lightning Network]                                      │
├─────────────────────────────────────────────────────────┤
│                                                          │
│             [Main Content Area]                          │
│                                                          │
│                                                          │
├─────────────────────────────────────────────────────────┤
│ Connections: 8  │  Blocks: 12,345  │  Up to date       │
└─────────────────────────────────────────────────────────┘
```

### Keyboard Shortcuts

- **Alt+1**: Overview page
- **Alt+2**: Send coins page
- **Alt+3**: Receive coins page
- **Alt+4**: Transactions page
- **Alt+5**: Address book page
- **Alt+6**: Lightning Network page
- **Alt+7**: Settings page
- **Ctrl+Q**: Quit application

---

## Overview Page

The Overview page provides a quick summary of your wallet status.

### Balance Display

**Available Balance**:
- Funds you can spend immediately
- Excludes immature coins (mining rewards < 100 confirmations)
- Excludes pending transactions

**Pending Balance**:
- Coins in unconfirmed transactions
- Becomes available after confirmations

**Immature Balance**:
- Mining rewards awaiting maturity (100 confirmations)
- Only displayed if you've mined blocks

**Total Balance**:
- Sum of all balances
- Your complete wallet holdings

### Recent Transactions

Shows your last 10 transactions:
- **Type**: Received, Sent, or Mined
- **Date**: Transaction timestamp
- **Amount**: Amount in INT
- **Status**: Confirmations (e.g., "6 confirmations")

Click any transaction to view details.

### Lightning Network Summary

If Lightning Network is active:
- **Total Capacity**: Combined capacity of all channels
- **Local Balance**: Funds you can send via Lightning
- **Remote Balance**: Funds you can receive via Lightning
- **Active Channels**: Number of operational channels

---

## Sending Coins

### Basic Send

1. Navigate to **Send** page (Alt+2)
2. Enter recipient's address:
   - Mainnet: starts with `int1`
   - Testnet: starts with `intc1`
3. Enter amount in INT
4. (Optional) Add a label for your records
5. (Optional) Add a message to recipient
6. Click **Send**
7. Confirm transaction details
8. Enter wallet passphrase if encrypted
9. Transaction sent!

### Advanced Options

**Fee Selection**:
- Drag the fee slider to adjust transaction priority
- **Low**: Slower confirmation (< 10 blocks)
- **Medium**: Normal confirmation (~2-4 blocks)
- **High**: Fast confirmation (~1-2 blocks)
- Custom fee: Enter manual fee rate

**Send to Multiple Recipients**:
1. Click **Add Recipient** button
2. Enter additional addresses and amounts
3. Total amount updates automatically
4. All recipients paid in single transaction (saves fees)

**Coin Control** (Advanced):
1. Click **Inputs** button
2. Select specific UTXOs to spend
3. Useful for:
   - Privacy (avoid linking addresses)
   - Fee optimization
   - Managing specific coins

### Transaction Status

After sending:
- **0 confirmations**: Broadcasted to network
- **1-5 confirmations**: Partially confirmed
- **6+ confirmations**: Fully confirmed (recommended)
- **100+ confirmations**: Deeply confirmed (irreversible)

---

## Receiving Coins

### Generating Receive Addresses

1. Navigate to **Receive** page (Alt+3)
2. (Optional) Enter a label to identify this address
3. (Optional) Enter expected amount
4. (Optional) Add a message
5. Click **Request Payment**

### Address Display

- **Address**: Full Bech32 address (int1...)
- **QR Code**: Scannable QR code for mobile wallets
- **URI**: Bitcoin-compatible payment URI
- **Copy**: Click to copy address to clipboard

### Address Reuse

**Best Practice**: Use a new address for each payment
- Improves privacy
- Easier to track payments
- HD wallet generates unlimited addresses

**Address History**:
- View all previously generated addresses
- See which addresses have received payments
- Track balances per address

### Sharing Your Address

**Methods**:
- Copy and paste the address
- Share QR code (scan with mobile wallet)
- Share payment URI (click-to-pay links)
- Email/messenger (ensure secure channel)

---

## Transaction History

### Viewing Transactions

The Transactions page (Alt+4) shows all wallet activity.

**Transaction Table Columns**:
- **Status**: Confirmation icons
- **Date**: Transaction timestamp
- **Type**: Received / Sent / Mined
- **Label**: User-defined label
- **Amount**: Amount in INT (+ received, - sent)

### Filtering Transactions

**Date Range**:
- Select start and end dates
- View transactions within specific period

**Type Filter**:
- All transactions
- Received only
- Sent only
- Mined only

**Search**:
- Search by label, address, or transaction ID
- Real-time filtering as you type

### Transaction Details

Double-click any transaction to view:
- **Transaction ID**: Unique identifier (txid)
- **Confirmations**: Current confirmation count
- **Date & Time**: Precise timestamp
- **Amount**: Transaction amount
- **Fee**: Transaction fee paid
- **Inputs**: Source addresses and amounts
- **Outputs**: Destination addresses and amounts
- **Block**: Block height (if confirmed)

### Exporting Transactions

1. Click **Export** button
2. Choose file format:
   - CSV (Excel compatible)
   - JSON (programmatic use)
3. Select save location
4. Opens in your default spreadsheet application

---

## Address Book

### Managing Contacts

The Address Book (Alt+5) stores frequently used addresses.

### Adding Contacts

1. Click **New Address** button
2. Enter contact information:
   - **Label**: Contact name or description
   - **Address**: INTcoin address (int1...)
3. Click **OK**

### Editing Contacts

1. Select contact from list
2. Click **Edit** button
3. Modify label or address
4. Click **OK** to save

### Using Address Book

**When Sending**:
1. On Send page, click **Address Book** icon
2. Select contact
3. Address automatically filled

**Organizing Contacts**:
- Sort by label or address
- Search/filter contacts
- Delete unused contacts

---

## Lightning Network

The Lightning Network page (Alt+6) provides full control over payment channels and instant payments.

### Page Overview

Four tabs provide complete Lightning functionality:
1. **Channels**: Manage payment channels
2. **Send Payment**: Pay Lightning invoices
3. **Receive Payment**: Create Lightning invoices
4. **Node Info**: View node status and statistics

---

### Channels Tab

Manage your Lightning Network payment channels.

#### Channel List

Displays all channels with detailed information:

| Column | Description |
|--------|-------------|
| **Channel ID** | Unique channel identifier (first 8 bytes) |
| **Peer** | Remote node public key (truncated) |
| **Status** | Active, Opening, Closing, or Closed |
| **Capacity** | Total channel capacity in INT |
| **Local Balance** | Your spendable funds (can send) |
| **Remote Balance** | Partner's funds (can receive) |
| **State** | Current channel state |

**Auto-Refresh**: Channel list updates every 5 seconds automatically.

#### Opening a Channel

1. Click **Open Channel** button
2. Enter peer information:
   - **Peer Public Key**: Dilithium3 public key (1952 bytes, hex-encoded)
   - Get peer's public key from their node
3. Set channel capacity:
   - **Recommended**: 0.001 - 10 INT
   - **Minimum**: 100,000 INTS (0.0001 INT)
   - **Maximum**: 1,000,000,000 INTS (1 INT)
4. Click **OK**
5. On-chain transaction creates channel funding
6. Wait for confirmation (2+ blocks recommended)
7. Channel becomes **Active** when confirmed

**Tips**:
- Start with small capacity for testing
- Choose well-connected peers for better routing
- Ensure you have on-chain balance for funding + fees

#### Closing a Channel

1. Select channel from table
2. Click **Close Channel** button
3. Confirm closure:
   - **Cooperative Close**: Fast, low fees (recommended)
   - **Force Close**: Slower, higher fees (if partner offline)
4. Funds return to on-chain wallet
5. Wait for settlement transaction to confirm

**Important**:
- Cooperative close preferred when possible
- Force close may have time delays (CSV locks)
- Channels cannot be reopened (must create new)

#### Balance Display

Above the channel table:
- **Total Capacity**: Sum of all channel capacities
- **Local Balance**: Total you can send via Lightning
- **Remote Balance**: Total you can receive via Lightning

---

### Send Payment Tab

Pay Lightning Network invoices for instant, low-fee payments.

#### Payment Process

1. **Obtain Invoice**:
   - Recipient provides BOLT #11 invoice (lint1...)
   - Copy invoice to clipboard

2. **Decode Invoice** (Optional but Recommended):
   - Paste invoice into **Payment Request** field
   - Click **Decode Invoice** button
   - Verify displayed information:
     - Amount in INTS
     - Description/memo
     - Expiry time
     - Payee public key
     - Payment hash

3. **Send Payment**:
   - Review decoded details
   - (Optional) Override amount if invoice allows
   - (Optional) Add internal memo for your records
   - Click **Send Payment** button
   - Payment routes through Lightning Network
   - Confirmation dialog shows payment hash

#### Payment Status

**Success**:
- Green confirmation message
- Payment hash displayed
- Deducted from local channel balance

**Failure**:
- Red error message with reason
- Common failures:
  - Insufficient local balance
  - No route to recipient
  - Invoice expired
  - Payment timeout

#### Payment History

Table below shows recent Lightning payments:
- **Date**: Payment timestamp
- **Direction**: Sent / Received
- **Amount**: Payment amount in INTS
- **Status**: Success / Failed / Pending
- **Description**: Payment memo

---

### Receive Payment Tab

Create Lightning Network invoices to receive instant payments.

#### Creating an Invoice

1. **Enter Amount**:
   - Spin box shows amount in INTS
   - 1 INT = 1,000,000,000 INTS
   - Can create zero-amount invoices (payer sets amount)

2. **Add Description** (Optional):
   - Short memo describing payment
   - Visible to payer
   - Examples: "Coffee", "Website hosting", "Consulting fee"

3. **Set Expiry** (Optional):
   - Default: 3600 seconds (1 hour)
   - Adjust for your needs
   - Invoice becomes invalid after expiry

4. **Click Create Invoice**:
   - BOLT #11 invoice generated (lint1...)
   - Full invoice string displayed
   - QR code shown (for mobile wallets)

#### Sharing Invoice

**Copy to Clipboard**:
- Click **Copy Invoice** button
- Share via:
  - Email
  - Messenger
  - SMS
  - Payment request

**QR Code**:
- Mobile wallets can scan directly
- Good for in-person payments
- (Note: Full QR implementation pending)

#### Invoice History

Table shows all generated invoices:
- **Date**: Invoice creation time
- **Amount**: Invoice amount
- **Status**: Pending / Paid / Expired
- **Description**: Invoice memo

**Tracking Payments**:
- **Pending**: Invoice valid, awaiting payment
- **Paid**: Payment received successfully
- **Expired**: Invoice expired, cannot be paid

---

### Node Info Tab

View your Lightning Network node information and statistics.

#### Node Information

**Status**:
- **Running**: Lightning Network operational
- **Not Running**: Lightning Network not started

**Public Key**:
- Full Dilithium3 public key (1952 bytes, hex)
- Share with peers for channel opening
- Click **Copy Node Info** to copy all details

**Network Address**:
- Your node's IP address
- Default: 127.0.0.1 (localhost)

**Port**:
- **Mainnet**: 2213
- **Testnet**: 12213

#### Channel Statistics

**Total Channels**:
- All channels (any state)
- Includes opening, active, closing

**Active Channels**:
- Currently operational channels
- Can send/receive payments

**Pending Channels**:
- Opening or closing
- Not yet operational

**Total Capacity**:
- Sum of all channel capacities
- Maximum theoretical throughput

**Local Balance**:
- Total you can send via all channels
- Available outbound liquidity

**Remote Balance**:
- Total you can receive via all channels
- Available inbound liquidity

#### Sharing Node Info

Click **Copy Node Info** to copy formatted details:
```
Node Public Key: 0a1b2c3d...
Network Address: 203.0.113.1
Port: 2213
```

Share with peers who want to open channels to your node.

---

### Lightning Best Practices

1. **Start Small**:
   - Test with small channel capacities first
   - Verify everything works before larger channels
   - Use testnet for experimentation

2. **Monitor Regularly**:
   - Check channel balances frequently
   - Ensure channels remain balanced
   - Rebalance if needed (advanced)

3. **Backup Channels**:
   - Use RPC commands for channel backups
   - Critical for fund recovery
   - Backup after each channel change

4. **Verify Invoices**:
   - Always decode invoices before paying
   - Check amount, description, expiry
   - Ensure invoice is from trusted source

5. **Security**:
   - Keep wallet encrypted
   - Never share private keys or mnemonic
   - Use hardware wallet for large amounts (future)

---

## Settings

### Network Settings

**Proxy Configuration**:
- **Tor**: Enable SOCKS5 proxy (9050)
- **I2P**: Enable SAM proxy (7656)
- **Custom Proxy**: Enter proxy address and port

**Network Options**:
- **Listen**: Allow incoming connections
- **UPnP**: Automatic port forwarding
- **Connections**: Maximum peer connections (default: 125)

**Port Configuration**:
- **P2P Port**: Default 2210 (mainnet), 12210 (testnet)
- **RPC Port**: Default 2211 (mainnet), 12211 (testnet)

### Display Settings

**Units**:
- **INT**: Display in INT (whole coins)
- **mINT**: Display in milli-INT (0.001 INT)
- **INTS**: Display in INTS (satoshis, 0.000000001 INT)

**Theme**:
- **Light**: Light theme (default)
- **Dark**: Dark theme (easier on eyes)
- **Auto**: Follow system theme

**Language**:
- Select interface language
- Restart required for changes

### Main Options

**Start on Boot**:
- Launch wallet automatically when computer starts
- Useful for running a full node 24/7

**Minimize to Tray**:
- Keep wallet running in system tray
- Quick access without closing

**Database Cache**:
- Memory allocated for blockchain database
- Higher = faster, but uses more RAM
- Default: 450 MB

**Script Verification Threads**:
- CPU threads for transaction validation
- More threads = faster sync
- Default: Auto (number of CPU cores)

---

## Wallet Management

### Backup Wallet

**Critical**: Always backup your wallet!

1. Go to **File → Backup Wallet**
2. Choose secure location:
   - External drive
   - USB stick
   - Cloud storage (encrypted!)
3. Name file: `intcoin-wallet-backup-YYYY-MM-DD.dat`
4. Store multiple backups in different locations

**What's Backed Up**:
- All private keys
- Address labels
- Transaction history
- Settings

**Not Backed Up**:
- Blockchain data (can re-download)
- Peer connections

### Restore Wallet

1. Close wallet completely
2. Locate wallet file:
   - **Linux**: `~/.intcoin/wallet.dat`
   - **macOS**: `~/Library/Application Support/INTcoin/wallet.dat`
   - **Windows**: `%APPDATA%\INTcoin\wallet.dat`
3. Replace with backup file
4. Restart wallet
5. Rescan blockchain: `intcoin-qt -rescan`

### Encrypt Wallet

**Highly Recommended** for security:

1. **Wallet → Encrypt Wallet**
2. Enter strong passphrase:
   - At least 12 characters
   - Mix of letters, numbers, symbols
   - Don't use dictionary words
3. Confirm passphrase
4. Wallet restarts (automatic)
5. **Write down passphrase immediately!**

**Important**:
- **Lost passphrase = lost funds forever**
- No recovery mechanism
- Store passphrase securely (safe, safety deposit box)

### Change Passphrase

If you need to update your passphrase:

1. **Wallet → Change Passphrase**
2. Enter current passphrase
3. Enter new passphrase
4. Confirm new passphrase
5. Passphrase updated

### Lock/Unlock Wallet

**Lock Wallet** (Wallet → Lock Wallet):
- Prevents sending coins
- Keeps wallet synchronized
- Good for security when not actively using

**Unlock Wallet** (Wallet → Unlock Wallet):
- Required for:
  - Sending coins
  - Creating Lightning invoices
  - Opening/closing Lightning channels
- Enter passphrase to unlock
- Can set unlock duration (minutes)

---

## Troubleshooting

### Wallet Won't Start

**Check**:
1. Ensure previous instance closed completely
2. Check disk space (need 50+ GB for full node)
3. Verify wallet.dat file exists and isn't corrupted
4. Check debug.log for errors:
   - **Linux**: `~/.intcoin/debug.log`
   - **macOS**: `~/Library/Application Support/INTcoin/debug.log`
   - **Windows**: `%APPDATA%\INTcoin\debug.log`

**Solutions**:
- Delete peers.dat and restart
- Start with `-rescan` flag
- Start with `-reindex` flag (slow but thorough)

### Synchronization Stuck

**Symptoms**:
- Block count not increasing
- "Synchronizing... X%" stuck

**Solutions**:
1. Check internet connection
2. Check firewall allows port 2210
3. Add nodes manually:
   ```
   intcoin-cli addnode "203.0.113.1:2210" "add"
   ```
4. Restart wallet
5. Try `-reindex` if persistent

### Balance Incorrect

**Common Causes**:
- Not fully synchronized (wait for sync)
- Pending transactions (check confirmations)
- Change addresses (normal, check all addresses)

**Solutions**:
1. Wait for full synchronization
2. Check **Transactions** page for all activity
3. Verify on block explorer
4. If still wrong, rescan: `intcoin-qt -rescan`

### Transaction Not Confirming

**Reasons**:
- Fee too low (underpaid)
- Network congestion
- Double-spend attempt detected

**Solutions**:
- Wait (may take hours with low fee)
- Abandon transaction if possible
- Use higher fee next time
- Enable Replace-By-Fee (RBF) in settings

### Lightning Network Issues

**Lightning Not Available**:
- Check daemon running: **Node Info** tab shows "Running"
- Restart Lightning Network
- Check logs for errors

**Channel Won't Open**:
- Verify peer public key correct (1952 bytes)
- Check sufficient on-chain balance
- Wait for confirmations (need 2+ blocks)
- Check peer is online and accepting channels

**Payment Failing**:
- Decode invoice to verify valid
- Check local balance sufficient
- Ensure recipient reachable
- Try different route (automatic)

### Forgotten Passphrase

**Unfortunately**:
- No recovery possible
- Passphrase is encryption key
- Lost passphrase = lost access to funds

**Prevention**:
- Write down passphrase immediately
- Store in multiple secure locations
- Consider using a password manager
- Share location (not content!) with trusted person

### Getting Help

**Resources**:
- **Documentation**: [https://international-coin.org/docs](https://international-coin.org/docs)
- **GitLab Issues**: [https://gitlab.com/intcoin/crypto/-/issues](https://gitlab.com/intcoin/crypto/-/issues)
- **Discord**: [https://discord.gg/intcoin](https://discord.gg/intcoin)
- **Telegram**: [https://t.me/INTcoin_official](https://t.me/INTcoin_official)

**When Reporting Issues**:
1. Include wallet version: **Help → About**
2. Include OS and version
3. Describe steps to reproduce
4. Include relevant log excerpts (from debug.log)
5. **Never share private keys or mnemonic!**

---

## Advanced Topics

### Running a Full Node

**Benefits**:
- Support network decentralization
- Validate all transactions yourself
- Enhanced privacy
- Help other nodes synchronize

**Requirements**:
- 50+ GB disk space
- Reliable internet connection
- Adequate bandwidth (~10 GB/month)
- Can run on Raspberry Pi 4+

**Setup**:
1. Ensure wallet always running
2. Enable **Settings → Listen for connections**
3. Forward port 2210 in router
4. Consider static IP or dynamic DNS

### Command-Line Options

Launch wallet with special options:

```bash
# Rescan blockchain (fix missing transactions)
intcoin-qt -rescan

# Reindex blockchain (rebuild database)
intcoin-qt -reindex

# Start on testnet
intcoin-qt -testnet

# Custom data directory
intcoin-qt -datadir=/path/to/data

# Connect to specific node
intcoin-qt -connect=203.0.113.1:2210

# Enable debug logging
intcoin-qt -debug=net,rpc
```

### Using RPC Commands

Access full node functionality:

```bash
# From terminal (wallet must be running)
intcoin-cli getbalance
intcoin-cli getnewaddress
intcoin-cli sendtoaddress "int1..." 1.5

# Lightning commands
intcoin-cli lightning_listchannels
intcoin-cli lightning_createinvoice 100000 "Payment"
```

---

## Security Best Practices

1. **Encrypt Wallet**: Always encrypt with strong passphrase
2. **Backup Regularly**: Multiple backups, different locations
3. **Update Software**: Keep wallet updated for security patches
4. **Verify Downloads**: Check GPG signatures
5. **Use Hardware Wallet**: For large amounts (future support)
6. **Beware Phishing**: Only download from official sources
7. **Test Before Large Transactions**: Send small amount first
8. **Enable 2FA**: On exchange accounts, not directly in wallet
9. **Privacy**: Use Tor for enhanced anonymity
10. **Cold Storage**: Keep bulk of funds offline

---

## Glossary

**Address**: Destination for receiving coins (int1...)
**Balance**: Amount of INT you control
**Block**: Group of transactions added to blockchain
**Blockchain**: Distributed ledger of all transactions
**Bech32**: Address encoding format (human-readable)
**Confirmation**: Number of blocks after transaction
**Fee**: Cost to send transaction (paid to miners)
**HD Wallet**: Hierarchical Deterministic (BIP32/BIP39)
**HTLC**: Hash Time-Locked Contract (Lightning)
**Lightning Network**: Layer 2 for instant payments
**Mainnet**: Production network (real money)
**Mempool**: Pool of unconfirmed transactions
**Mnemonic**: 24-word recovery phrase
**Passphrase**: Encryption password for wallet
**Peer**: Other node on network
**Private Key**: Secret key controlling funds
**Public Key**: Public key for receiving funds (derived from private key)
**QR Code**: Scannable code for easy address sharing
**Satoshi**: Smallest unit (0.000000001 INT = 1 INTS)
**Testnet**: Test network (fake money for testing)
**Transaction**: Transfer of coins between addresses
**UTXO**: Unspent Transaction Output

---

**For More Information**:
- [Installation Guide](getting-started/Installation.md)
- [Lightning Network Guide](LIGHTNING.md)
- [RPC API Reference](RPC.md)
- [Security & Privacy](SECURITY_SANITIZATION.md)

---

*INTcoin Core Team*
*December 24, 2025*
