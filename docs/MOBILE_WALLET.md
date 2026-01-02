# INTcoin Mobile Wallet Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

---

## Table of Contents

1. [Introduction](#introduction)
2. [What is SPV](#what-is-spv)
3. [iOS Wallet Guide](#ios-wallet-guide)
4. [Android Wallet Guide](#android-wallet-guide)
5. [Security Best Practices](#security-best-practices)
6. [QR Code Payments](#qr-code-payments)
7. [Troubleshooting](#troubleshooting)
8. [FAQ](#faq)

---

## Introduction

The INTcoin mobile wallet brings the power of post-quantum cryptocurrency to your iOS and Android devices. Using **SPV (Simplified Payment Verification)** technology, mobile wallets can verify transactions without downloading the entire blockchain, making them lightweight and fast.

### Key Features

- 📱 **Native iOS and Android apps** - Built with Swift and Kotlin
- ⚡ **Fast synchronization** - Initial sync in <30 seconds
- 💾 **Minimal storage** - <10 MB vs 100+ GB for full node
- 🔒 **Biometric security** - Face ID, Touch ID, Fingerprint
- 🔑 **BIP39 mnemonics** - Standard 12 or 24-word recovery phrases
- 📊 **QR code payments** - Scan and pay instantly
- 🌐 **Multi-language support** - English, Spanish, Chinese (more coming)

### System Requirements

**iOS**:
- iOS 14.0 or later
- iPhone 6S or newer
- 50 MB free storage
- Internet connection

**Android**:
- Android 8.0 (Oreo) or later
- 50 MB free storage
- Internet connection

---

## What is SPV?

### Simplified Payment Verification Explained

SPV (Simplified Payment Verification) is a method for verifying bitcoin/cryptocurrency transactions without downloading the full blockchain. Introduced in the original Bitcoin whitepaper by Satoshi Nakamoto, SPV allows lightweight clients to verify that a transaction is included in a block without downloading all transactions in that block.

### How SPV Works

1. **Download Block Headers Only**
   - Block headers are 80 bytes each
   - Entire header chain: ~50 MB vs 100+ GB for full blockchain
   - Headers contain proof-of-work, allowing chain validation

2. **Request Merkle Proofs**
   - When receiving a payment, request merkle proof for that transaction
   - Merkle proof is logarithmic in size (log₂n where n = transactions per block)
   - Proof shows transaction is included in the block's merkle tree

3. **Verify with Bloom Filters**
   - Create a bloom filter containing your addresses
   - Full nodes filter transactions matching your addresses
   - Privacy-preserving through false positives

### Benefits vs Full Node

| Feature | SPV Wallet | Full Node |
|---------|------------|-----------|
| Initial Sync Time | <30 seconds | Hours to days |
| Bandwidth Usage | <1 MB/session | 100+ GB initial, GB/day ongoing |
| Storage Required | <10 MB | 100+ GB |
| Privacy | Good (bloom filters) | Excellent |
| Security | Very Good | Excellent |
| Validation | Headers + merkle proofs | Full transaction validation |

### SPV Security Model

**Trust Assumptions**:
- SPV clients trust that the longest proof-of-work chain is valid
- Merkle proofs verify transaction **inclusion**, not transaction **validity**
- SPV clients assume miners validate transactions correctly

**Security Guarantees**:
- ✅ Cannot be double-spent (transaction is in the longest chain)
- ✅ Cannot have invalid proof-of-work (header verification)
- ⚠️ Could receive invalid transaction if all miners collude (extremely unlikely)

**Mitigation**:
- Connect to multiple diverse peers
- Use checkpoint verification for old blocks
- Option to connect to trusted full node

### Bandwidth Savings

For a typical mobile user:
- **Full node**: 100 GB initial download + 1-5 GB/day
- **SPV wallet**: ~50 MB initial + <1 MB/day
- **Savings**: 99.9% bandwidth reduction

---

## iOS Wallet Guide

### Installation

#### From TestFlight (Beta)

1. Install TestFlight from the App Store
2. Open beta invite link: [TestFlight Invite](https://testflight.apple.com/join/intcoin)
3. Tap "Install" in TestFlight
4. Open "INTcoin Wallet" from home screen

#### From App Store (When Available)

1. Open App Store
2. Search "INTcoin Wallet"
3. Tap "Get" and authenticate
4. Open "INTcoin Wallet" from home screen

#### Build from Source

See [mobile/ios/README.md](../mobile/ios/README.md) for building from source.

### First-Time Setup

1. **Create New Wallet**
   - Open app and tap "Create New Wallet"
   - Choose 12-word or 24-word mnemonic (24-word recommended)
   - Write down your recovery phrase on paper (NEVER screenshot)
   - Confirm recovery phrase by selecting words in order

2. **Enable Biometric Authentication**
   - Tap "Settings" → "Security"
   - Enable "Face ID" or "Touch ID"
   - This protects access to your wallet

3. **Set Wallet Password**
   - Choose a strong password (8+ characters, mixed case, numbers, symbols)
   - This encrypts your wallet data
   - Store password safely (password manager recommended)

### Receiving INT

1. Tap "Receive" tab
2. Your address is displayed as QR code and text
3. Share address with sender:
   - Show QR code for in-person payment
   - Tap "Copy" to copy address to clipboard
   - Tap "Share" to send via Messages/Email

4. Wait for confirmations:
   - 1 confirmation: Payment received (appears in wallet)
   - 6 confirmations: Fully confirmed (recommended for large amounts)

### Sending INT

1. Tap "Send" tab
2. Enter recipient address:
   - Paste address from clipboard
   - Scan QR code with camera
   - Select from contacts (if saved)

3. Enter amount:
   - Enter amount in INT
   - Tap "USD" to enter USD amount (uses current exchange rate)
   - Review transaction fee

4. Review and confirm:
   - Verify address and amount
   - Check fee (adjust if needed)
   - Tap "Send"
   - Authenticate with Face ID/Touch ID or password

5. Monitor transaction:
   - Transaction appears in "Transactions" list
   - Status shows confirmations: "0/6", "1/6", "6/6"
   - Tap transaction for details

### Transaction History

- View all transactions in "Transactions" tab
- Filter by:
  - All transactions
  - Received only
  - Sent only
  - Date range
- Search by:
  - Address
  - Amount
  - Transaction ID

### Settings

**Security**:
- Enable/disable Face ID / Touch ID
- Change wallet password
- View recovery phrase (requires authentication)
- Auto-lock timeout (1 min, 5 min, 15 min, never)

**Network**:
- Testnet / Mainnet toggle
- Connect to custom full node
- View connected peers
- Sync status and progress

**Display**:
- Currency (INT, BTC, USD, EUR, etc.)
- Theme (Light, Dark, Auto)
- Language selection

**Advanced**:
- Bloom filter settings (size, false positive rate)
- Fee estimation (Economy, Normal, Priority)
- Export transaction history (CSV)

### Backup and Recovery

**Backup Recovery Phrase**:
1. Go to Settings → Security → View Recovery Phrase
2. Authenticate with Face ID/Touch ID
3. Write down all 12 or 24 words IN ORDER
4. Store in secure location:
   - Safe deposit box
   - Fireproof safe
   - Multiple physical copies in different locations
5. **NEVER**:
   - Screenshot recovery phrase
   - Email or message recovery phrase
   - Store in cloud storage
   - Share with anyone

**Restore Wallet**:
1. Open app and tap "Restore Wallet"
2. Enter your 12 or 24-word recovery phrase
3. Set new password
4. Enable biometric authentication
5. Wait for sync to complete

---

## Android Wallet Guide

### Installation

#### From Google Play (When Available)

1. Open Google Play Store
2. Search "INTcoin Wallet"
3. Tap "Install"
4. Open "INTcoin Wallet" from app drawer

#### Sideload APK (Beta)

1. Download APK: [Download Link](https://github.com/InternationalCoin/intcoin/releases)
2. Open Settings → Security → Install Unknown Apps
3. Enable for your browser/file manager
4. Open downloaded APK file
5. Tap "Install"

#### Build from Source

See [mobile/android/README.md](../mobile/android/README.md) for building from source.

### First-Time Setup

1. **Create New Wallet**
   - Open app and tap "Create New Wallet"
   - Choose 12-word or 24-word mnemonic
   - Write down recovery phrase on paper
   - Confirm recovery phrase

2. **Enable Biometric Authentication**
   - Tap ⋮ menu → Settings → Security
   - Enable "Fingerprint" or "Face Unlock"
   - Enroll biometric if not already set up

3. **Set Wallet PIN**
   - Choose 4-8 digit PIN
   - This is required if biometric fails
   - Remember this PIN

### Receiving INT

1. Tap "Receive" button
2. Your address is displayed as QR code and text
3. Share address:
   - Show QR code
   - Tap address to copy
   - Tap share icon for WhatsApp/Email

4. Optional: Set expected amount
   - Tap "Set Amount"
   - Enter expected INT amount
   - QR code updates with payment request

### Sending INT

1. Tap "Send" button
2. Enter recipient:
   - Tap QR code icon to scan
   - Paste address from clipboard
   - Select from address book

3. Enter amount and fee:
   - Enter INT amount
   - Select fee: Low, Medium, High
   - See estimated confirmation time

4. Review and send:
   - Verify all details
   - Tap "Send"
   - Authenticate with fingerprint/face/PIN

### Transaction History

- Swipe to refresh transaction list
- Tap transaction for details:
  - Transaction ID
  - Confirmations
  - Block height
  - Fee paid
  - Timestamp

### Settings

**Security**:
- Biometric authentication toggle
- Change PIN
- View/backup recovery phrase
- Screen lock timeout

**Network**:
- Mainnet/Testnet switch
- Custom node connection
- Peer list and status
- Synchronization settings

**Preferences**:
- Default currency
- Language
- Theme (Light/Dark/System)
- Decimal places

**Advanced**:
- Bloom filter configuration
- Fee estimation strategy
- Transaction history export
- Clear cache

### Backup and Recovery

**Backup**:
1. ⋮ Menu → Settings → Security → Backup Wallet
2. Authenticate
3. View and write down recovery phrase
4. Store securely

**Recovery**:
1. Open app → "Restore from Backup"
2. Enter 12 or 24-word recovery phrase
3. Set new PIN
4. Enable biometric
5. Wallet syncs automatically

---

## Security Best Practices

### Recovery Phrase Security

**DO**:
- ✅ Write recovery phrase on paper with pen
- ✅ Store in multiple secure physical locations
- ✅ Use steel backup plates for fire/water protection
- ✅ Consider using Shamir's Secret Sharing (split into multiple parts)

**DON'T**:
- ❌ Take screenshots or photos of recovery phrase
- ❌ Store in cloud (iCloud, Google Drive, Dropbox)
- ❌ Email or message recovery phrase
- ❌ Store in password manager (unless encrypted with strong master password)
- ❌ Tell anyone your recovery phrase

### Password and PIN Security

- Use strong, unique password (20+ characters recommended)
- Use password manager for passwords (not recovery phrases)
- Don't reuse passwords from other services
- Change password if device is compromised

### Device Security

- Keep iOS/Android updated to latest version
- Only install apps from App Store / Google Play
- Don't jailbreak (iOS) or root (Android) device with wallet
- Enable Find My iPhone / Find My Device
- Use strong device passcode (not just 4 digits)
- Disable iCloud/Google backup for wallet app

### Network Security

- Avoid public WiFi for transactions
- Use VPN on untrusted networks
- Verify SSL certificate when connecting to custom node
- Don't connect to unknown/suspicious full nodes

### Transaction Security

- Always verify recipient address carefully
- Double-check amount before sending
- Start with small test transaction for new recipients
- Be aware of transaction fees (higher fee = faster confirmation)
- Wait for 6 confirmations for large amounts

### App Permissions

**Required Permissions**:
- Camera (for QR code scanning)
- Internet (for synchronization)

**Not Required**:
- Location
- Contacts
- Microphone
- Photos (except to save QR code if user chooses)

### What to Do If Phone is Lost/Stolen

1. **Immediately**:
   - Don't panic - wallet is encrypted and biometric-protected
   - Use Find My iPhone/Android to locate or remotely wipe

2. **If Device is Wiped or Unrecoverable**:
   - Install INTcoin wallet on new device
   - Restore using recovery phrase
   - All funds will be recovered

3. **If Recovery Phrase is Also Lost**:
   - Unfortunately, funds cannot be recovered
   - This is why backup is critical

---

## QR Code Payments

### INTcoin URI Scheme

Format: `intcoin:<address>?amount=<amount>&label=<label>&message=<message>`

Example:
```
intcoin:int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh?amount=10.5&label=Coffee%20Shop&message=Latte
```

### Generating Payment QR Codes

**In Mobile App**:
1. Go to Receive tab
2. Tap "Request Amount"
3. Enter amount and optional message
4. QR code updates automatically
5. Share QR code with payer

**Programmatically** (for merchants):
```swift
// iOS Swift
let uri = "intcoin:\(address)?amount=\(amount)&label=\(label)"
let qrCode = generateQRCode(from: uri)
```

```kotlin
// Android Kotlin
val uri = "intcoin:$address?amount=$amount&label=$label"
val qrCode = generateQRCode(uri)
```

### Scanning QR Codes

1. Tap Send tab
2. Tap QR code scan icon
3. Point camera at QR code
4. Amount and address auto-populate
5. Review and confirm

---

## Troubleshooting

### Wallet Not Syncing

**Symptoms**: Block height not increasing, "Connecting..." message

**Solutions**:
1. Check internet connection
2. Try switching WiFi/cellular
3. Go to Settings → Network → Reconnect
4. Restart app
5. If persistent, connect to custom full node

### Transaction Not Appearing

**Symptoms**: Sent INT but transaction not showing in recipient's wallet

**Check**:
1. Verify transaction was broadcast:
   - Check on block explorer: [explorer.international-coin.org](https://explorer.international-coin.org)
   - Look up transaction ID or recipient address

2. Wait for sync:
   - Recipient wallet may still be syncing
   - Check sync status (Settings → Network)

3. Bloom filter issue:
   - Recipient: Clear bloom filter (Settings → Advanced → Clear Cache)
   - Restart app and resync

### Biometric Authentication Not Working

**iOS**:
1. Check Settings → Face ID & Passcode → INTcoin is enabled
2. Re-enroll Face ID if needed
3. Fallback to password authentication

**Android**:
1. Check Settings → Security → Fingerprint → INTcoin is enabled
2. Re-enroll fingerprint if needed
3. Use PIN as fallback

### "Insufficient Funds" Error

**Causes**:
1. Not enough INT to cover amount + fee
2. UTXO not confirmed yet (wait for confirmation)
3. Wallet not fully synced

**Solutions**:
1. Reduce amount or fee
2. Wait for pending transactions to confirm
3. Wait for sync to complete

### App Crashes on Startup

**Solutions**:
1. Force close and restart app
2. Update to latest version
3. Clear app cache (Android: Settings → Apps → INTcoin → Clear Cache)
4. Reinstall app (wallet data is safe if you have recovery phrase)
5. Restore from recovery phrase if needed

---

## FAQ

### Can I use the same wallet on multiple devices?

Yes! Use your recovery phrase to restore your wallet on multiple devices. All devices will show the same addresses and balance. However, transaction history may vary slightly depending on sync status.

### How do I get testnet coins?

Visit the testnet faucet: [https://faucet.intcoin.org](https://faucet.intcoin.org)
Enter your testnet address and request test INT.

### Can I import private keys?

Currently, mobile wallets use BIP39/BIP32 HD wallets only. Private key import is supported in the desktop Qt wallet.

### How do I change my recovery phrase?

You cannot change your recovery phrase. To use a new recovery phrase:
1. Create a new wallet
2. Send all funds from old wallet to new wallet
3. Old wallet can be deleted

### Is my wallet anonymous?

SPV wallets provide good privacy through bloom filters, but are not completely anonymous. For maximum privacy:
- Don't reuse addresses
- Use larger bloom filters (higher false positive rate)
- Consider using desktop wallet with Tor

### What happens if I lose my recovery phrase?

If you lose your recovery phrase AND your device, your funds are permanently lost. There is no recovery mechanism. Always backup your recovery phrase securely.

### Can I use Lightning Network on mobile?

Lightning Network is available in the desktop wallet. Mobile Lightning support is planned for v1.3.0.

### How much does it cost to send a transaction?

Fees vary based on network congestion. Typical fees:
- Low priority: 0.0001 INT per byte (~0.025 INT per transaction)
- Normal: 0.0005 INT per byte (~0.125 INT per transaction)
- High: 0.001 INT per byte (~0.25 INT per transaction)

---

## Resources

- **Mobile SDK Documentation**: [MOBILE_SDK.md](MOBILE_SDK.md)
- **SPV Technical Details**: [SPV_AND_BLOOM_FILTERS.md](SPV_AND_BLOOM_FILTERS.md)
- **iOS Source Code**: [mobile/ios/](../mobile/ios/)
- **Android Source Code**: [mobile/android/](../mobile/android/)
- **Block Explorer**: https://explorer.international-coin.org
- **Testnet Faucet**: https://faucet.intcoin.org

---

## Support

For assistance:
- **GitHub Issues**: https://github.com/InternationalCoin/intcoin/issues
- **Email**: support@international-coin.org
- **Community**: https://github.com/InternationalCoin/intcoin/discussions

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
