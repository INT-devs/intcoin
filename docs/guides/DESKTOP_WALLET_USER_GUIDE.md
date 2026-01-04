# INTcoin Desktop Wallet User Guide

**Version**: 1.3.0-beta
**Platform**: Linux, macOS, FreeBSD, Windows
**Last Updated**: January 2, 2026

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Installation](#2-installation)
3. [Getting Started](#3-getting-started)
4. [Basic Operations](#4-basic-operations)
5. [Advanced Features](#5-advanced-features)
6. [Hardware Wallets](#6-hardware-wallets)
7. [Coin Control](#7-coin-control)
8. [Batch Payments](#8-batch-payments)
9. [Payment Requests & Invoices](#9-payment-requests--invoices)
10. [Security Best Practices](#10-security-best-practices)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Introduction

Welcome to the INTcoin Desktop Wallet! This guide will help you get started with managing your INTcoin using the Qt6-based desktop application.

### What's New in v1.3.0-beta

- ✅ **Advanced Coin Control** - Manual UTXO selection with privacy scoring
- ✅ **Hardware Wallet Support** - Ledger, Trezor, and Coldcard integration
- ✅ **Batch Payments** - Send to multiple recipients in one transaction
- ✅ **Payment Requests** - Generate QR codes and professional PDF invoices
- ✅ **Replace-By-Fee (RBF)** - Adjust transaction fees after broadcasting
- ✅ **Address Book** - Organize contacts with categories and tags
- ✅ **Modern UI** - Sleek Qt6 interface with dark mode support

### System Requirements

**Minimum**:
- CPU: Dual-core processor (2 GHz+)
- RAM: 4 GB
- Disk: 50 GB free space (SSD recommended)
- OS: Ubuntu 20.04+, macOS 12+, FreeBSD 13+, Windows 10 22H2+

**Recommended**:
- CPU: Quad-core processor (3 GHz+)
- RAM: 8 GB or more
- Disk: 256 GB SSD (NVMe for best performance)
- OS: Latest stable version

---

## 2. Installation

### 2.1 Linux Installation

#### Ubuntu/Debian

```bash
# Download the latest release
wget https://github.com/INT-devs/intcoin/releases/download/v1.3.0-beta/intcoin-1.3.0-beta-linux-x86_64.tar.gz

# Extract
tar -xzf intcoin-1.3.0-beta-linux-x86_64.tar.gz

# Install
cd intcoin-1.3.0-beta
sudo ./install.sh

# Launch wallet
intcoin-qt
```

#### From Source

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libboost-all-dev \
                 librocksdb-dev liboqs-dev libssl-dev

# Clone repository
git clone https://github.com/INT-devs/intcoin.git
cd intcoin
git checkout v1.3.0-beta

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WALLET_QT=ON ..
make -j$(nproc)
sudo make install
```

### 2.2 macOS Installation

```bash
# Download the DMG
curl -LO https://github.com/INT-devs/intcoin/releases/download/v1.3.0-beta/INTcoin-1.3.0-beta.dmg

# Open and drag to Applications
open INTcoin-1.3.0-beta.dmg

# Launch from Applications folder
open -a INTcoin
```

### 2.3 FreeBSD Installation

```bash
# Install from ports (if available)
cd /usr/ports/net-p2p/intcoin
make install clean

# Or build from source
pkg install cmake qt6 boost-libs rocksdb liboqs
git clone https://github.com/INT-devs/intcoin.git
cd intcoin && git checkout v1.3.0-beta
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WALLET_QT=ON ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```

### 2.4 Windows Installation

1. Download `INTcoin-1.3.0-beta-win64-setup.exe`
2. Run the installer
3. Follow the installation wizard
4. Launch INTcoin from Start Menu or Desktop shortcut

---

## 3. Getting Started

### 3.1 First Launch

When you first open the wallet, you'll see the **Welcome Screen** with two options:

1. **Create New Wallet** - Generate a new wallet with a BIP39 seed phrase
2. **Restore Wallet** - Recover an existing wallet from seed phrase

### 3.2 Creating a New Wallet

**Step 1**: Click "Create New Wallet"

**Step 2**: Set a strong password (minimum 12 characters recommended)
- Use a mix of uppercase, lowercase, numbers, and symbols
- Do not use common words or personal information
- Store your password securely (password manager recommended)

**Step 3**: Write down your **24-word seed phrase**
```
CRITICAL: Write these words on paper and store securely.
Never store digitally or share with anyone.
This is the ONLY way to recover your wallet if you lose access.
```

Example seed phrase:
```
abandon ability able about above absent absorb abstract
absurd abuse access accident account accuse achieve acid
acoustic acquire across act action actor actress actual
```

**Step 4**: Verify your seed phrase
- Enter words #3, #7, #15, and #22 to confirm you wrote them correctly

**Step 5**: Wallet creation complete!
- Your wallet is now ready to use
- Initial blockchain sync will begin automatically

### 3.3 Restoring an Existing Wallet

**Step 1**: Click "Restore Wallet"

**Step 2**: Enter your 24-word seed phrase
- Type or paste the words in order
- The wallet will validate the checksum

**Step 3**: Set a new password for this device

**Step 4**: Choose sync method:
- **Fast Sync** (Recommended): Uses AssumeUTXO, wallet usable in ~10 minutes
- **Full Sync**: Downloads and validates entire blockchain from genesis

**Step 5**: Restoration complete!
- Your wallet will begin scanning for transactions
- All previous balances and history will appear once sync completes

---

## 4. Basic Operations

### 4.1 Receiving INTcoin

**Step 1**: Go to the **Receive** tab

**Step 2**: Your receiving address is displayed
- Click **Copy Address** to copy to clipboard
- Click **Generate QR Code** for mobile scanning

**Step 3**: Optional: Add a label
- Label helps you remember what this payment is for
- Example: "Payment from Alice" or "Salary - January 2026"

**Step 4**: Share your address with the sender
- Never reuse addresses for privacy reasons
- Click **New Address** for each new payment

**Step 5**: Wait for confirmation
- Payments appear instantly in "Pending"
- After 6 confirmations (~60 minutes), funds are fully confirmed

### 4.2 Sending INTcoin

**Step 1**: Go to the **Send** tab

**Step 2**: Enter recipient's address
- Paste the address or scan QR code
- Wallet will validate the address format

**Step 3**: Enter amount to send
- Specify amount in INT or your local currency
- Wallet shows equivalent value in both

**Step 4**: Review transaction fee
- **Low Priority**: Confirm in ~6 blocks (1 hour) - Lowest fee
- **Normal**: Confirm in ~3 blocks (30 min) - Balanced
- **High Priority**: Confirm in ~1 block (10 min) - Higher fee
- **Custom**: Set your own fee rate

**Step 5**: Optional: Add a note
- Private note visible only to you
- Helps track what this payment was for

**Step 6**: Review and confirm
- Check all details carefully
- Once sent, transactions cannot be reversed

**Step 7**: Click **Send**
- Enter your wallet password
- Transaction broadcast to network

### 4.3 Transaction History

The **Transactions** tab shows all your activity:

**Columns**:
- **Status**: Pending (⏱), Confirmed (✓), or Failed (✗)
- **Date**: When transaction occurred
- **Type**: Received (↓) or Sent (↑)
- **Address**: Sender/recipient
- **Amount**: Value in INT
- **Balance**: Your balance after this transaction

**Filters**:
- All Transactions
- Received Only
- Sent Only
- Date Range
- Search by address or transaction ID

**Export**:
- CSV for spreadsheets
- PDF for records
- JSON for accounting software

---

## 5. Advanced Features

### 5.1 Address Book

Manage your contacts for quick and easy payments.

**Adding a Contact**:

1. Go to **Contacts** tab
2. Click **Add Contact**
3. Fill in details:
   - **Name**: Contact's name
   - **Address**: Their INTcoin address
   - **Category**: Personal, Business, Exchange, etc.
   - **Tags**: Custom tags (comma-separated)
   - **Notes**: Additional information
4. Click **Save**

**Using Contacts**:
- When sending, click the contact book icon
- Select contact from list
- Address auto-fills

**Import/Export**:
- **Import CSV**: Bulk import contacts
  ```csv
  Name,Address,Category,Tags
  Alice,INT1qxy...,Personal,"friend,frequent"
  Bob's Shop,INT1abc...,Business,"vendor,monthly"
  ```
- **Export vCard**: Export for other apps

**Transaction History per Contact**:
- Click on any contact
- View all transactions with that address
- See total sent/received

### 5.2 Replace-By-Fee (RBF)

Adjust transaction fees after broadcasting if confirmation is slow.

**When to Use**:
- Transaction stuck in mempool
- Need faster confirmation than originally selected
- Network fees increased after broadcasting

**How to Use**:

1. Go to **Transactions** tab
2. Find pending transaction (must have RBF enabled)
3. Right-click → **Bump Fee**
4. Choose new fee rate:
   - **+25%**: Modest increase
   - **+50%**: Significant increase
   - **+100%**: Double the fee
   - **Custom**: Set specific rate
5. Review new total fee
6. Click **Confirm**

**Important**:
- Original transaction will be replaced
- New transaction ID will be generated
- Cannot reduce fees, only increase

---

## 6. Hardware Wallets

### 6.1 Supported Devices

- **Ledger Nano S** (Firmware 2.1+)
- **Ledger Nano X** (Firmware 2.1+)
- **Trezor Model T** (Firmware 2.5+)
- **Trezor One** (Firmware 1.11+)
- **Coldcard Mk4** (Firmware 5.0+)

### 6.2 Setting Up Ledger

**Step 1**: Connect your Ledger device via USB

**Step 2**: Open INTcoin wallet
- Go to **Settings** → **Hardware Wallets**

**Step 3**: Click **Add Device**
- Wallet scans for connected devices
- Ledger should appear in list

**Step 4**: Initialize on Ledger
- Open the INTcoin app on your Ledger
- Confirm connection on device screen

**Step 5**: Verify address
- Generate first receiving address
- Confirm address matches on Ledger screen
- This ensures no man-in-the-middle attack

**Step 6**: You're ready!
- All transactions require device confirmation
- Private keys never leave the device

### 6.3 Signing Transactions

**Step 1**: Create transaction as normal

**Step 2**: Click **Send**

**Step 3**: Review on Ledger screen
- Recipient address (scroll through)
- Amount to send
- Fee amount
- Total (Amount + Fee)

**Step 4**: Confirm on device
- Press both buttons on "Approve" screen
- Device signs transaction
- Signed transaction broadcast to network

**Security**: Even if your computer is compromised, your funds are safe because:
- Transaction details shown on device screen (not computer)
- You manually verify and approve on device
- Private keys never leave the secure element

### 6.4 Troubleshooting Hardware Wallets

**Device Not Detected**:
- Ensure USB cable is fully connected
- Try a different USB port
- Update device firmware
- Install device drivers (Windows)

**Transaction Not Signing**:
- Ensure INTcoin app is open on device
- Check device isn't locked/timed out
- Verify firmware is up to date
- Try resetting USB connection

**Address Mismatch**:
- If addresses don't match, DO NOT PROCEED
- This indicates a security issue
- Disconnect and restart wallet
- Contact support if issue persists

---

## 7. Coin Control

Advanced UTXO (Unspent Transaction Output) management for privacy and fee optimization.

### 7.1 What is Coin Control?

Every time you receive INTcoin, it creates a separate "coin" (UTXO). When spending, the wallet selects which coins to use. Coin Control lets you manually choose.

**Why Use Coin Control?**:
- **Privacy**: Avoid linking separate payments
- **Fee Optimization**: Select coins to minimize transaction size
- **Organization**: Separate personal and business funds

### 7.2 Viewing Your Coins

**Step 1**: Go to **Send** tab

**Step 2**: Click **Coin Control** button

**Step 3**: View all your UTXOs:
- **Address**: Which address holds this coin
- **Amount**: Value of the coin
- **Confirmations**: How many blocks deep
- **Privacy Score**: 0.0 (low) to 1.0 (high)
- **Age**: How long since received

### 7.3 Selection Strategies

**AUTOMATIC** (Default):
- Wallet chooses optimal coins automatically
- Balances privacy, fees, and reliability

**MINIMIZE_FEE**:
- Select fewest/largest coins to minimize transaction size
- Best for: Routine payments where privacy isn't critical

**MAXIMIZE_PRIVACY**:
- Select coins with highest privacy scores
- Avoid linking different sources
- Best for: Privacy-sensitive payments

**OLDEST_FIRST**:
- Spend oldest coins first (FIFO)
- Reduces UTXO set size
- Best for: General housekeeping

**LARGEST_FIRST**:
- Spend largest coins first
- Consolidate small coins
- Best for: Reducing wallet clutter

**SMALLEST_FIRST**:
- Spend smallest coins first
- Reduce UTXO count
- Best for: Cleaning up dust

**MANUAL**:
- You choose exactly which coins to spend
- Maximum control
- Best for: Advanced users, specific requirements

### 7.4 Freezing Coins

Prevent specific coins from being spent.

**Why Freeze?**:
- Preserve coins for future specific purpose
- Exclude coins with low privacy scores
- Prevent accidental spending of "savings"

**How to Freeze**:
1. In Coin Control view
2. Right-click coin → **Freeze**
3. Frozen coins won't be selected for payments
4. Unfreeze when ready to use

### 7.5 Privacy Scores Explained

The wallet calculates a privacy score (0.0 - 1.0) for each coin based on:

- **Age**: Older coins = higher score (harder to link)
- **Amount**: Common amounts = lower score (fingerprintable)
- **Source Diversity**: Multiple sources = higher score
- **Reuse**: Address reuse = lower score

**Interpreting Scores**:
- **0.9 - 1.0**: Excellent privacy
- **0.7 - 0.9**: Good privacy
- **0.5 - 0.7**: Moderate privacy
- **0.0 - 0.5**: Poor privacy, avoid if possible

---

## 8. Batch Payments

Send to multiple recipients in a single transaction.

### 8.1 Manual Batch

**Step 1**: Go to **Send** tab

**Step 2**: Click **Add Recipient**

**Step 3**: Enter first recipient:
- Address
- Amount
- Label (optional)

**Step 4**: Click **Add Recipient** again for second recipient

**Step 5**: Repeat for all recipients (up to 100)

**Step 6**: Review total:
- Total amount to send
- Estimated fee (based on transaction size)
- Total (Amount + Fee)

**Step 7**: Click **Send Batch**

**Benefits**:
- Save on fees (one transaction vs. many)
- Faster (all payments in one block)
- Easier tracking

### 8.2 CSV Import

Perfect for payroll, dividends, or bulk payments.

**Step 1**: Prepare CSV file:
```csv
Address,Amount,Label
INT1qxy...,100.50,Alice - January Salary
INT1abc...,75.25,Bob - Contract Payment
INT1def...,50.00,Carol - Affiliate Commission
```

**Step 2**: Go to **Send** → **Import Batch**

**Step 3**: Select CSV file

**Step 4**: Preview import:
- Wallet shows all recipients
- Validates addresses
- Highlights any errors

**Step 5**: Confirm and send

**CSV Format**:
- Header row required
- Columns: `Address,Amount,Label`
- Amount in INT (decimals allowed)
- Label is optional

### 8.3 Batch Templates

Save frequently used batches for reuse.

**Creating a Template**:
1. Set up batch payment (manual or CSV)
2. Click **Save as Template**
3. Give it a name (e.g., "Monthly Payroll")
4. Template saved

**Using a Template**:
1. Click **Load Template**
2. Select saved template
3. Review and modify amounts if needed
4. Send

**Managing Templates**:
- Edit: Modify recipients or amounts
- Delete: Remove outdated templates
- Export: Save template as CSV

---

## 9. Payment Requests & Invoices

Create professional payment requests with QR codes and PDF invoices.

### 9.1 Simple Payment Request

**Step 1**: Go to **Receive** → **Create Request**

**Step 2**: Enter details:
- **Amount**: How much you're requesting
- **Label**: What this payment is for
- **Message**: Additional instructions

**Step 3**: Click **Create Request**

**Step 4**: Share with customer:
- **QR Code**: They scan with mobile wallet
- **BIP21 URI**: Clickable payment link
- **Address + Amount**: Manual entry

**Step 5**: Request expires after 24 hours (configurable)

### 9.2 Professional Invoices

**Step 1**: Go to **Receive** → **Create Invoice**

**Step 2**: Fill in invoice details:
- **Invoice Number**: Auto-generated or custom
- **Customer Name**: Who you're invoicing
- **Due Date**: When payment is expected

**Step 3**: Add line items:
| Item | Quantity | Unit Price | Total |
|------|----------|------------|-------|
| Web Design | 1 | 500 INT | 500 INT |
| Logo Design | 1 | 200 INT | 200 INT |
| Hosting (1 year) | 1 | 50 INT | 50 INT |

**Step 4**: Subtotal, Tax (if applicable), Total calculated automatically

**Step 5**: Add business details:
- Your business name
- Contact information
- Website URL
- Notes or payment terms

**Step 6**: Generate invoice:
- **View PDF**: Preview before sending
- **Download PDF**: Save professional PDF invoice
- **Email**: Send directly (if configured)
- **Print**: For physical delivery

**Step 7**: QR code automatically included in PDF for easy payment

### 9.3 Tracking Payment Status

**Dashboard View**:
- **Pending**: Awaiting payment (yellow)
- **Paid**: Payment received (green)
- **Expired**: Passed due date (red)
- **Cancelled**: Manually cancelled (gray)

**Notification**:
- Desktop notification when payment received
- Email notification (if configured)
- Mark as paid manually if paid off-chain

---

## 10. Security Best Practices

### 10.1 Wallet Security

**Password**:
- Use strong, unique password (12+ characters)
- Mix uppercase, lowercase, numbers, symbols
- Store in password manager
- Never share with anyone

**Seed Phrase**:
- Write on paper, store in safe or safety deposit box
- Never photograph or store digitally
- Consider metal backup for fire/water resistance
- Split storage (e.g., 12 words in two locations)

**Backup**:
- Regular wallet file backups
- Test restoration process
- Store backups offline
- Encrypt backup files

### 10.2 Transaction Security

**Verify Addresses**:
- Always double-check recipient address
- Use QR codes when possible (fewer errors)
- For large amounts, send small test first

**Hardware Wallet for Large Amounts**:
- If holding >$10,000, use hardware wallet
- Computer virus cannot steal from hardware wallet
- Small investment for peace of mind

**Beware of Phishing**:
- Only download wallet from official sources
- Verify signature of downloads
- Never enter seed phrase on websites
- INTcoin team will never ask for your seed phrase

### 10.3 Privacy Best Practices

**Address Reuse**:
- Never reuse receiving addresses
- Generate new address for each payment
- Improves privacy and security

**Coin Control**:
- Use "Maximize Privacy" for sensitive payments
- Separate personal and business addresses
- Avoid round numbers (more fingerprintable)

**Network Privacy** (When Tor Support Added):
- Use Tor to hide IP address
- Prevent network-level tracking
- Recommended for high-privacy needs

---

## 11. Troubleshooting

### 11.1 Common Issues

**Wallet Won't Sync**:
1. Check internet connection
2. Ensure firewall allows intcoind (port 8333)
3. Try different peers (Settings → Network)
4. Restart wallet

**Transaction Stuck**:
- If RBF enabled: Use "Bump Fee"
- If not: Wait for mempool to clear
- Contact support if stuck >3 days

**Forgot Password**:
- If you have seed phrase: Restore wallet
- If no seed phrase: Funds cannot be recovered
- Prevention: Use password manager

**Low Disk Space**:
- Prune old blocks (Settings → Storage)
- Keep last 5 GB of blocks only
- Saves 90% of disk space

### 11.2 Getting Help

**Documentation**:
- Wiki: https://docs.intcoin.org
- FAQ: https://intcoin.org/faq
- Video Tutorials: https://youtube.com/intcoin

**Community Support**:
- Discord: https://discord.gg/intcoin
- Reddit: https://reddit.com/r/INTcoin
- Telegram: https://t.me/intcoin

**Bug Reports**:
- GitHub Issues: https://github.com/INT-devs/intcoin/issues
- Include: OS, wallet version, error logs
- Attach logs from Help → Debug Log

**Email Support**:
- support@intcoin.org
- Response time: 24-48 hours
- For urgent security issues: security@intcoin.org

---

## Appendix

### A. Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New Receiving Address |
| Ctrl+S | Send Payment |
| Ctrl+R | Receive Payment |
| Ctrl+T | Transaction History |
| Ctrl+C | Copy Selected Address |
| Ctrl+V | Paste Address |
| F1 | Help Documentation |
| F5 | Refresh/Sync |
| Ctrl+Q | Quit Wallet |

### B. Configuration File

Location: `~/.intcoin/intcoin.conf` (Linux/macOS) or `%APPDATA%\INTcoin\intcoin.conf` (Windows)

Example configuration:
```ini
# Network
listen=1
server=1
maxconnections=50

# RPC (for advanced users)
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1

# Wallet
keypool=1000
addresstype=bech32

# Performance
dbcache=4000
maxmempool=300

# Privacy (when Tor support added)
#proxy=127.0.0.1:9050
#listen=0
```

### C. BIP39 Word List

The wallet uses the standard BIP39 English word list (2048 words). For the complete list, see:
https://github.com/bitcoin/bips/blob/master/bip-0039/english.txt

### D. Fee Estimation Guide

| Priority | Target | Fee Rate | Typical Cost (250 byte tx) |
|----------|--------|----------|----------------------------|
| Low | 6 blocks | 5 sat/byte | 0.0000125 INT |
| Normal | 3 blocks | 10 sat/byte | 0.000025 INT |
| High | 1 block | 20 sat/byte | 0.00005 INT |

Actual fees vary based on network congestion. The wallet's ML-based estimator provides real-time recommendations.

---

**Document Version**: 1.0
**Last Updated**: January 2, 2026
**For**: INTcoin v1.3.0-beta

**Support**: support@intcoin.org | https://discord.gg/intcoin
