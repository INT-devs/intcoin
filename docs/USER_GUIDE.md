# INTcoin Desktop Wallet User Guide
## Version 1.3.0-beta

**Welcome to the INTcoin Desktop Wallet!**

This guide will help you get started with the INTcoin wallet and take full advantage of its features.

---

## Table of Contents

1. [Installation](#installation)
2. [Getting Started](#getting-started)
3. [Wallet Basics](#wallet-basics)
4. [Sending and Receiving](#sending-and-receiving)
5. [Lightning Network](#lightning-network)
6. [Advanced Features](#advanced-features)
7. [Security Best Practices](#security-best-practices)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

---

## Installation

### macOS

1. Download the latest INTcoin-v1.3.0-beta.dmg from the official website
2. Open the DMG file
3. Drag INTcoin to your Applications folder
4. Launch INTcoin from Applications

### Linux

```bash
# Download the AppImage
wget https://intcoin.org/downloads/intcoin-1.3.0-beta-x86_64.AppImage

# Make it executable
chmod +x intcoin-1.3.0-beta-x86_64.AppImage

# Run the wallet
./intcoin-1.3.0-beta-x86_64.AppImage
```

### FreeBSD

```bash
# Install from ports
cd /usr/ports/finance/intcoin
make install clean

# Or install from package
pkg install intcoin
```

### Windows

1. Download INTcoin-v1.3.0-beta-Setup.exe
2. Run the installer
3. Follow the installation wizard
4. Launch INTcoin from the Start menu

### Building from Source

See [BUILD.md](BUILD.md) for detailed build instructions.

---

## Getting Started

### First Launch

When you first launch the INTcoin wallet, you'll be prompted to:

1. **Choose a data directory** - This is where your blockchain data and wallet will be stored
2. **Create or restore a wallet** - Choose "Create New Wallet" or "Restore from Backup"
3. **Set a strong password** - This encrypts your wallet file

### Initial Blockchain Sync

The wallet needs to download and verify the entire blockchain. This can take several hours depending on your internet speed and computer performance.

**v1.3.0 Features**:
- **AssumeUTXO Fast Sync**: Skip to a recent snapshot and validate in the background
- **Parallel Block Validation**: 8-thread validation for ~2x faster sync
- **Progress indicators**: Real-time sync progress and estimated time remaining

To use AssumeUTXO fast sync:
1. Go to **Settings > Advanced**
2. Enable "AssumeUTXO Fast Sync"
3. The wallet will download a verified snapshot and start immediately

---

## Wallet Basics

### Overview Tab

The Overview tab shows:
- **Total Balance**: Your available, pending, and immature coins
- **Recent Transactions**: Latest sends and receives
- **Network Status**: Connections, block height, sync status
- **Mempool Analytics** (v1.3.0): Real-time transaction flow metrics

### Transactions Tab

View all your wallet transactions with:
- Transaction ID (clickable to view in block explorer)
- Date and time
- Amount (positive for receives, negative for sends)
- Confirmations
- Status (confirmed/pending)
- Labels and categories

**Filter and Search**:
- Filter by date range, amount, or type
- Search by transaction ID or address
- Export to CSV for accounting

---

## Sending and Receiving

### Receiving Coins

1. Click the **Receive** tab
2. (Optional) Enter a label for this address (e.g., "Payment from Alice")
3. (Optional) Enter an amount to request
4. Click **Generate New Address**
5. Share the address or QR code with the sender

**Address Format**:
- **Bech32 (Mainnet)**: Starts with "int1" - INTcoin native address format
- **Bech32 (Testnet)**: Starts with "tint1" - for testing purposes only
- **Bech32 (Lightning)**: Starts with "lint1" - Lightning Network invoices

**Example Addresses**:
```
# Mainnet
int1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vycz5qm7xte8

# Testnet
tint1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vyczsr5hxv2

# Lightning
lint1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vyczswk9eaj
```

**Privacy Tip**: Use a new address for each transaction to maintain privacy.

### Sending Coins

1. Click the **Send** tab
2. Enter the recipient's address
3. Enter the amount to send
4. Choose your fee:
   - **Economy**: ~30 min confirmation (22 sat/byte)
   - **Normal**: ~10 min confirmation (27 sat/byte)
   - **Fast**: ~3 min confirmation (31 sat/byte)
   - **Custom**: Set your own fee rate
5. (Optional) Add a label/description
6. Click **Send**
7. Confirm the transaction details
8. Enter your wallet password

**Smart Fee Estimation** (v1.3.0):
- Analyzes recent blocks and mempool state
- Provides accurate fee estimates with confidence levels
- Automatically adjusts based on network congestion

**Coin Control** (Advanced):
- Select specific UTXOs to spend
- Useful for privacy or consolidating small inputs
- Access via **Settings > Advanced > Enable Coin Control**

---

## Lightning Network

### What is the Lightning Network?

The Lightning Network is a Layer 2 scaling solution that enables:
- **Instant payments**: Transactions settle in milliseconds
- **Microtransactions**: Send fractions of a cent
- **Low fees**: Typically <1 sat per transaction
- **Scalability**: Millions of transactions per second

### Getting Started with Lightning

#### 1. Open a Channel

1. Go to the **Lightning** tab
2. Click **Open Channel**
3. Enter:
   - **Node Public Key** or **Node Alias**
   - **Channel Capacity** (amount to lock in channel)
   - **Push Amount** (optional - initial balance for peer)
4. Click **Open Channel**
5. Wait for 3 confirmations (~30 minutes)

**Recommendations**:
- Start with a small channel (0.001-0.01 BTC)
- Connect to well-connected nodes for better routing
- Use multiple channels for redundancy

#### 2. Make a Lightning Payment

1. Get a Lightning invoice from the recipient
2. Click **Pay Invoice**
3. Paste the invoice (starts with "lnbc")
4. Review amount and description
5. Click **Pay**
6. Payment completes instantly!

#### 3. Receive a Lightning Payment

1. Click **Create Invoice**
2. Enter amount and description
3. Share the invoice with the payer
4. Wait for payment notification

### Advanced Lightning Features (v1.3.0)

#### Multi-Path Payments (MPP)
- Split large payments across multiple channels
- Improves success rate and privacy
- Automatically used when beneficial

#### Channel Rebalancing
- Keep channels balanced for optimal routing
- Circular rebalancing (self-payment)
- Automated recommendations
- Access via **Lightning > Manage Channels > Rebalance**

#### Submarine Swaps
- Move funds between on-chain and Lightning seamlessly
- **Swap In**: Send on-chain, receive in Lightning channel
- **Swap Out**: Send Lightning, receive on-chain
- No custodian required

#### Watchtower Protection
- Protect against channel breaches while offline
- Encrypted justice transactions stored with watchtower
- Enable in **Lightning Settings > Security > Enable Watchtower**

---

## Advanced Features

### Mempool Analytics (v1.3.0)

Real-time mempool monitoring and analysis:

**Access**: **Tools > Mempool Analytics**

**Features**:
- **Transaction Flow**: Real-time inflow/outflow rates
- **Fee Distribution**: Histogram of pending transaction fees
- **Network Health**: Congestion indicators and recommendations
- **Historical Data**: Snapshots of mempool state over time
- **Fee Recommendations**: Optimal fees for different confirmation targets

**Use Cases**:
- Choose the best time to send transactions (low fees)
- Predict confirmation times
- Monitor network congestion
- Analyze transaction patterns

### Coin Control

Select specific UTXOs (coins) to spend:

1. Enable in **Settings > Advanced > Enable Coin Control**
2. In the **Send** tab, click **Inputs**
3. Select which coins to spend
4. Useful for:
   - Privacy (avoid linking addresses)
   - Fee optimization (consolidate small inputs)
   - Dust management

### Address Labels and Contacts

Organize your transactions:

1. **Add Contact**: **File > Address Book > Add**
2. **Label Addresses**: Right-click any address and select "Edit Label"
3. **Categories**: Assign transactions to categories for accounting

### Backup and Restore

**Manual Backup**:
1. **File > Backup Wallet**
2. Save to a secure location (USB drive, encrypted cloud storage)
3. **Never store backups unencrypted!**

**Automatic Backups**:
- Enabled by default in `~/.intcoin/backups/`
- 10 most recent backups retained
- Configure in **Settings > Backup**

**Restore from Backup**:
1. **File > Restore Wallet**
2. Select your backup file
3. Enter the backup password
4. Restart the wallet

**Seed Phrase** (BIP39):
- 12 or 24 words that can restore your wallet
- **Write down and store securely offline**
- Never share your seed phrase
- View in **Settings > Security > Show Seed Phrase**

---

## Security Best Practices

### Password Security

- Use a strong, unique password (16+ characters)
- Include uppercase, lowercase, numbers, and symbols
- Never reuse passwords from other services
- Use a password manager (e.g., KeePassXC, 1Password)

### Wallet Encryption

- Always encrypt your wallet (enabled by default)
- Change password: **Settings > Security > Change Passphrase**
- Unlock only when needed
- Auto-lock timeout: **Settings > Security > Auto-lock After**

### Backup Strategy

**3-2-1 Rule**:
- **3** copies of your backup
- **2** different media types (USB + paper seed phrase)
- **1** offsite backup (safe deposit box, trusted location)

**What to back up**:
- Encrypted wallet.dat file
- Seed phrase (written on paper)
- Lightning channel backups (if using Lightning)

### Network Security

- Always use the latest wallet version
- Enable automatic updates: **Settings > General > Auto-update**
- Use Tor for enhanced privacy: **Settings > Network > Use Tor**
- Verify download signatures before installing

### Recognizing Scams

**Warning Signs**:
- Unsolicited messages asking for seed phrase or password
- "Support" contacts asking you to send coins
- Websites offering to "double your Bitcoin"
- Fake wallet software or updates

**Remember**:
- INTcoin support will NEVER ask for your seed phrase or password
- No legitimate service will ask you to send coins first
- Always verify URLs (intcoin.org is official)
- Download only from official sources

---

## Troubleshooting

### Wallet Won't Sync

**Symptoms**: Stuck at a specific block, not connecting to peers

**Solutions**:
1. Check your internet connection
2. Allow INTcoin through your firewall
3. Try different peers: **Settings > Network > Add Node**
4. Rescan blockchain: **Tools > Repair > Rescan Blockchain**
5. Reindex blockchain: **Tools > Repair > Reindex**

### Transaction Not Confirming

**Causes**:
- Fee too low for current network conditions
- Network congestion

**Solutions**:
1. Wait (low-fee transactions eventually confirm)
2. Replace-by-Fee (RBF): Right-click transaction > Bump Fee
3. Child-Pays-For-Parent (CPFP): Send coins to yourself with higher fee

### Lightning Channel Issues

**Channel Stuck Opening**:
- Wait for 3 confirmations
- Check if opening transaction confirmed
- Contact peer if channel not appearing after confirmations

**Payment Failures**:
- Insufficient channel balance
- No route to destination
- Try different route or amount
- Consider opening more channels

### High Memory Usage

**Solutions**:
1. Reduce database cache: **Settings > Advanced > Database Cache**
2. Prune old blocks: **Settings > Advanced > Enable Pruning**
3. Close Lightning channels if not using

### Lost Password

**Unfortunately**: If you lose your wallet password, your funds are **permanently lost**.

**Prevention**:
- Write down password in secure location
- Use password manager
- Test restore procedure with small amount first
- Back up seed phrase as ultimate recovery

---

## FAQ

### General Questions

**Q: How do I get INTcoin?**

A: You can:
- Receive from another user
- Mine using the built-in miner (Tools > Mining)
- Purchase from a cryptocurrency exchange

**Q: How long do transactions take to confirm?**

A:
- On-chain: ~10 minutes for first confirmation (recommended: 3-6 confirmations)
- Lightning: Instant (milliseconds)

**Q: What are confirmations?**

A: Each confirmation is a new block added after your transaction. More confirmations = more security against double-spends.

**Q: Can I cancel a transaction?**

A: Once broadcast, transactions cannot be cancelled. However, if you enabled Replace-by-Fee (RBF), you can bump the fee or redirect to yourself.

**Q: Is my wallet anonymous?**

A: Bitcoin/INTcoin transactions are pseudonymous, not anonymous:
- Transactions are public on the blockchain
- Addresses can be linked to identities
- Use new addresses for each transaction for better privacy
- Consider using Tor, CoinJoin, or Lightning for enhanced privacy

### Technical Questions

**Q: What's the difference between Bitcoin and INTcoin?**

A: INTcoin is based on Bitcoin but includes:
- Advanced mempool analytics
- Lightning Network v2 with MPP, submarine swaps, watchtowers
- AssumeUTXO fast sync
- Parallel block validation
- DeFi primitives (v1.4.0 planned)
- Privacy features (v1.4.0 planned)

**Q: Can I use my Bitcoin wallet seed phrase in INTcoin?**

A: Yes, if it's a BIP39 seed phrase. However, be cautious as the derivation paths may differ.

**Q: Does INTcoin support hardware wallets?**

A: Hardware wallet support is planned for v1.5.0.

**Q: How much disk space do I need?**

A:
- Full node: ~500GB (and growing)
- Pruned node: ~10GB
- AssumeUTXO: ~5GB initial + ~10GB pruned

**Q: Can I run multiple wallets?**

A: Yes, use different data directories:
```bash
intcoin -datadir=/path/to/wallet1
intcoin -datadir=/path/to/wallet2
```

### Lightning Network Questions

**Q: How much should I put in a Lightning channel?**

A: Start small (0.001-0.01 BTC) to learn. For regular use, consider your typical payment sizes × 10-20 transactions.

**Q: Can I close a channel anytime?**

A: Yes, but:
- Cooperative close: Fast (1 confirmation), both parties agree
- Force close: Slower (waiting period), if peer offline
- Funds return to on-chain wallet after close

**Q: What happens if my Lightning node goes offline?**

A:
- Short outages: No problem, channels remain active
- Long outages: Watchtower protects against breaches
- Very long outages (weeks): Channel may be force-closed by peer

**Q: Are Lightning payments safe?**

A: Yes, as safe as on-chain payments:
- Cryptographically secured
- No custodian (you control your keys)
- Watchtowers protect against fraud
- Smart contracts enforce rules

---

## Getting Help

### Resources

- **Official Website**: https://intcoin.org
- **Documentation**: https://docs.intcoin.org
- **GitHub**: https://github.com/intcoin/intcoin
- **Discord**: https://discord.gg/intcoin
- **Reddit**: https://reddit.com/r/intcoin
- **Twitter**: @intcoin

### Reporting Issues

Found a bug? Report it on GitHub:
1. Go to https://github.com/intcoin/intcoin/issues
2. Click "New Issue"
3. Provide:
   - Wallet version
   - Operating system
   - Steps to reproduce
   - Error messages
   - Debug log (Tools > Debug > Debug Log)

### Contributing

INTcoin is open-source! Contributions welcome:
- Code improvements
- Bug fixes
- Documentation
- Translations
- Testing

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

---

## Glossary

**Address**: A string of letters and numbers representing a destination for coins

**ASIC**: Specialized hardware for cryptocurrency mining

**Bech32**: INTcoin native address format (starts with int1 for mainnet, tint1 for testnet, lint1 for Lightning)

**Block**: A group of transactions recorded on the blockchain

**Blockchain**: A distributed ledger of all transactions

**Channel**: Lightning Network payment channel between two nodes

**Confirmation**: Inclusion of a transaction in a block

**Fee Rate**: Transaction fee per byte (sat/byte)

**HTLC**: Hashed Time-Locked Contract (used in Lightning)

**Mempool**: Pool of unconfirmed transactions

**Mining**: Process of adding new blocks to the blockchain

**Node**: A computer running the INTcoin software

**Private Key**: Secret key that controls access to your coins

**Public Key**: Derived from private key, used to create addresses

**Satoshi (sat)**: Smallest unit of Bitcoin/INTcoin (0.00000001)

**Seed Phrase**: 12 or 24 words that can restore your wallet

**SegWit**: Segregated Witness - transaction format upgrade

**UTXO**: Unspent Transaction Output (a "coin")

**Watchtower**: Service that monitors Lightning channels while offline

---

**Document Version**: 1.0
**Last Updated**: January 3, 2026
**INTcoin Version**: 1.3.0-beta

**Happy Transacting! 🚀**
