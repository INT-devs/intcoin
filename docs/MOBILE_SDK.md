# INTcoin Mobile SDK Documentation

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [iOS SDK](#ios-sdk)
4. [Android SDK](#android-sdk)
5. [Core Features](#core-features)
6. [API Reference](#api-reference)
7. [Examples](#examples)
8. [Best Practices](#best-practices)

---

## Introduction

The INTcoin Mobile SDK enables developers to integrate post-quantum cryptocurrency functionality into iOS and Android applications. Built on SPV technology, the SDK provides a lightweight, secure, and easy-to-use interface for wallet operations.

### Features

- 📱 **Native iOS and Android** - Swift and Kotlin APIs
- ⚡ **SPV Technology** - Fast sync, minimal bandwidth
- 🔒 **BIP39/BIP32 HD Wallets** - Industry standard
- 🔐 **Secure Key Storage** - Keychain (iOS) / Keystore (Android)
- 💳 **Transaction Management** - Send, receive, track
- 🌐 **Network Flexibility** - Mainnet, testnet, custom nodes
- 📊 **QR Code Support** - Built-in payment URI handling
- 🔔 **Real-time Updates** - Transaction notifications

### Requirements

**iOS**:
- iOS 14.0+
- Xcode 13.0+
- Swift 5.5+
- CocoaPods or Swift Package Manager

**Android**:
- Android API 26+ (Android 8.0 Oreo)
- Android Studio 2021.1.1+
- Kotlin 1.6+
- Gradle 7.0+

---

## Getting Started

### iOS Installation

#### Swift Package Manager (Recommended)

Add to `Package.swift`:

```swift
dependencies: [
    .package(url: "https://github.com/InternationalCoin/intcoin-ios-sdk.git", from: "1.2.0")
]
```

Or in Xcode:
1. File → Add Packages
2. Enter: `https://github.com/InternationalCoin/intcoin-ios-sdk.git`
3. Select version: 1.2.0-beta

#### CocoaPods

Add to `Podfile`:

```ruby
pod 'INTcoinKit', '~> 1.2.0'
```

Install:

```bash
pod install
```

#### Manual Installation

1. Download SDK: [intcoin-ios-sdk-1.2.0.zip](https://github.com/InternationalCoin/intcoin/releases)
2. Drag `INTcoinKit.framework` into Xcode project
3. Embed & Sign framework in project settings

### Android Installation

#### Gradle (Recommended)

Add to `build.gradle` (project level):

```gradle
allprojects {
    repositories {
        maven { url 'https://jitpack.io' }
    }
}
```

Add to `build.gradle` (app level):

```gradle
dependencies {
    implementation 'com.github.InternationalCoin:intcoin-android-sdk:1.2.0'
}
```

#### Manual Installation

1. Download AAR: [intcoin-sdk-1.2.0.aar](https://github.com/InternationalCoin/intcoin/releases)
2. Place in `app/libs/`
3. Add to `build.gradle`:

```gradle
dependencies {
    implementation files('libs/intcoin-sdk-1.2.0.aar')
}
```

---

## iOS SDK

### Quick Start

```swift
import INTcoinKit

// Initialize SDK
INTcoinSDK.shared.configure(network: .mainnet)

// Create or restore wallet
let mnemonic = Mnemonic.generate(strength: .words24)
let wallet = try Wallet.create(mnemonic: mnemonic)

// Start synchronization
wallet.startSync { progress in
    print("Sync: \(progress)%")
}

// Get balance
let balance = wallet.balance

// Receive address
let address = wallet.receiveAddress

// Send transaction
let tx = try wallet.send(
    to: "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    amount: 1.5,
    fee: .normal
)
```

### Wallet Creation

#### Generate New Wallet

```swift
// Generate 24-word mnemonic (recommended)
let mnemonic = Mnemonic.generate(strength: .words24)

// Or 12-word mnemonic
let mnemonic12 = Mnemonic.generate(strength: .words12)

// Create wallet
let wallet = try Wallet.create(
    mnemonic: mnemonic,
    passphrase: "",  // Optional BIP39 passphrase
    network: .mainnet
)

// Store mnemonic securely (user's responsibility)
saveToSecureLocation(mnemonic.words)
```

#### Restore from Mnemonic

```swift
// Restore wallet
let words = ["word1", "word2", ... "word24"]
let mnemonic = try Mnemonic(words: words)
let wallet = try Wallet.restore(mnemonic: mnemonic, network: .mainnet)
```

### Synchronization

```swift
// Start sync
wallet.startSync { progress in
    print("Syncing: \(progress)%")
}

// Check sync status
if wallet.isSynced {
    print("Wallet is synced")
}

// Get current height
let height = wallet.blockHeight

// Stop sync
wallet.stopSync()
```

### Balance and Transactions

```swift
// Get confirmed balance
let confirmed = wallet.balance.confirmed

// Get unconfirmed balance
let unconfirmed = wallet.balance.unconfirmed

// Get total balance
let total = wallet.balance.total

// Get transaction history
let transactions = wallet.transactions

// Filter transactions
let received = wallet.transactions.filter { $0.direction == .received }
let sent = wallet.transactions.filter { $0.direction == .sent }

// Get specific transaction
if let tx = wallet.transaction(id: txHash) {
    print("Amount: \(tx.amount)")
    print("Confirmations: \(tx.confirmations)")
}
```

### Sending Transactions

```swift
// Send with specific fee
let tx = try wallet.send(
    to: "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    amount: 1.5,
    feeRate: FeeRate.normal
)

// Send with custom fee
let customTx = try wallet.send(
    to: address,
    amount: 2.0,
    feeRate: FeeRate.custom(satoshisPerByte: 100)
)

// Send all (sweep)
let sweepTx = try wallet.sendAll(
    to: address,
    feeRate: .normal
)
```

### Fee Estimation

```swift
// Get recommended fees
let fees = wallet.estimateFees()
print("Low: \(fees.low) sat/byte")
print("Normal: \(fees.normal) sat/byte")
print("High: \(fees.high) sat/byte")

// Estimate fee for transaction
let estimatedFee = wallet.estimateFee(
    to: address,
    amount: 1.5,
    feeRate: .normal
)
```

### Notifications

```swift
// Transaction received
wallet.onTransactionReceived = { transaction in
    print("Received \(transaction.amount) INT")
    showNotification(title: "Payment Received", amount: transaction.amount)
}

// Transaction sent
wallet.onTransactionSent = { transaction in
    print("Sent \(transaction.amount) INT")
}

// Sync completed
wallet.onSyncCompleted = {
    print("Sync complete!")
}

// Balance changed
wallet.onBalanceChanged = { balance in
    updateUI(balance: balance)
}
```

### QR Code Generation

```swift
import INTcoinKit

// Simple address QR code
let qrCode = QRCodeGenerator.generate(address: wallet.receiveAddress)
imageView.image = qrCode

// Payment request QR code
let paymentURI = PaymentURI(
    address: wallet.receiveAddress,
    amount: 1.5,
    label: "Coffee Shop",
    message: "Grande Latte"
)
let paymentQR = QRCodeGenerator.generate(uri: paymentURI)
```

### QR Code Scanning

```swift
import AVFoundation
import INTcoinKit

// Scan QR code
let scanner = QRCodeScanner()
scanner.onCodeScanned = { code in
    if let paymentURI = PaymentURI.parse(code) {
        // Auto-fill send form
        addressField.text = paymentURI.address
        amountField.text = "\(paymentURI.amount ?? 0)"
        memoField.text = paymentURI.message
    }
}
scanner.startScanning()
```

---

## Android SDK

### Quick Start

```kotlin
import org.intcoin.sdk.*

// Initialize SDK
INTcoinSDK.initialize(context, Network.MAINNET)

// Create or restore wallet
val mnemonic = Mnemonic.generate(WordCount.WORDS_24)
val wallet = Wallet.create(mnemonic)

// Start sync
wallet.startSync { progress ->
    println("Sync: $progress%")
}

// Get balance
val balance = wallet.balance

// Receive address
val address = wallet.receiveAddress

// Send transaction
val tx = wallet.send(
    to = "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    amount = 1.5,
    feeRate = FeeRate.NORMAL
)
```

### Wallet Creation

#### Generate New Wallet

```kotlin
// Generate 24-word mnemonic
val mnemonic = Mnemonic.generate(WordCount.WORDS_24)

// Or 12-word
val mnemonic12 = Mnemonic.generate(WordCount.WORDS_12)

// Create wallet
val wallet = Wallet.create(
    mnemonic = mnemonic,
    passphrase = "",  // Optional BIP39 passphrase
    network = Network.MAINNET
)

// Store mnemonic securely
secureStorage.save("mnemonic", mnemonic.words.joinToString(" "))
```

#### Restore from Mnemonic

```kotlin
// Restore wallet
val words = listOf("word1", "word2", ... "word24")
val mnemonic = Mnemonic(words)
val wallet = Wallet.restore(mnemonic, Network.MAINNET)
```

### Synchronization

```kotlin
// Start sync with callback
wallet.startSync { progress ->
    println("Syncing: $progress%")
    updateProgressBar(progress)
}

// Check sync status
if (wallet.isSynced) {
    println("Wallet is synced")
}

// Get current height
val height = wallet.blockHeight

// Stop sync
wallet.stopSync()
```

### Balance and Transactions

```kotlin
// Get balances
val confirmed = wallet.balance.confirmed
val unconfirmed = wallet.balance.unconfirmed
val total = wallet.balance.total

// Get transactions
val transactions = wallet.transactions

// Filter transactions
val received = wallet.transactions.filter { it.direction == Direction.RECEIVED }
val sent = wallet.transactions.filter { it.direction == Direction.SENT }

// Get specific transaction
wallet.getTransaction(txHash)?.let { tx ->
    println("Amount: ${tx.amount}")
    println("Confirmations: ${tx.confirmations}")
}
```

### Sending Transactions

```kotlin
// Send with fee rate
val tx = wallet.send(
    to = "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh",
    amount = 1.5,
    feeRate = FeeRate.NORMAL
)

// Send with custom fee
val customTx = wallet.send(
    to = address,
    amount = 2.0,
    feeRate = FeeRate.custom(satoshisPerByte = 100)
)

// Send all
val sweepTx = wallet.sendAll(
    to = address,
    feeRate = FeeRate.NORMAL
)
```

### Fee Estimation

```kotlin
// Get recommended fees
val fees = wallet.estimateFees()
println("Low: ${fees.low} sat/byte")
println("Normal: ${fees.normal} sat/byte")
println("High: ${fees.high} sat/byte")

// Estimate fee for specific transaction
val fee = wallet.estimateFee(
    to = address,
    amount = 1.5,
    feeRate = FeeRate.NORMAL
)
```

### Notifications (LiveData)

```kotlin
// Observe transaction events
wallet.transactionReceived.observe(lifecycleOwner) { transaction ->
    println("Received ${transaction.amount} INT")
    showNotification("Payment Received", transaction.amount)
}

wallet.transactionSent.observe(lifecycleOwner) { transaction ->
    println("Sent ${transaction.amount} INT")
}

// Observe sync completion
wallet.syncCompleted.observe(lifecycleOwner) {
    println("Sync complete!")
}

// Observe balance changes
wallet.balanceChanged.observe(lifecycleOwner) { balance ->
    updateUI(balance)
}
```

### QR Code Generation

```kotlin
import org.intcoin.sdk.qr.*

// Simple address QR
val qrBitmap = QRCodeGenerator.generate(wallet.receiveAddress)
imageView.setImageBitmap(qrBitmap)

// Payment request QR
val paymentURI = PaymentURI(
    address = wallet.receiveAddress,
    amount = 1.5,
    label = "Coffee Shop",
    message = "Grande Latte"
)
val paymentQR = QRCodeGenerator.generate(paymentURI.toString())
```

### QR Code Scanning

```kotlin
import com.google.zxing.*
import org.intcoin.sdk.qr.*

// Using ZXing
val scanner = IntentIntegrator(activity)
scanner.setDesiredBarcodeFormats(IntentIntegrator.QR_CODE)
scanner.setPrompt("Scan INT payment QR code")
scanner.initiateScan()

// Handle result
override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
    val result = IntentIntegrator.parseActivityResult(requestCode, resultCode, data)
    result?.contents?.let { code ->
        PaymentURI.parse(code)?.let { uri ->
            addressField.setText(uri.address)
            amountField.setText(uri.amount.toString())
            memoField.setText(uri.message)
        }
    }
}
```

---

## Core Features

### BIP39 Mnemonic Support

Both SDKs support standard BIP39 mnemonics:

- **12 words**: 128 bits entropy
- **24 words**: 256 bits entropy (recommended)

**Languages supported**: English (more coming)

### BIP32 HD Wallet

Hierarchical Deterministic wallet structure:

```
m/44'/INT_COIN_TYPE'/0'/0/0  - First receiving address
m/44'/INT_COIN_TYPE'/0'/0/1  - Second receiving address
m/44'/INT_COIN_TYPE'/0'/1/0  - First change address
```

### Secure Key Storage

**iOS**: Keychain Services
- Keys encrypted by iOS Secure Enclave
- Protected by device passcode/biometrics
- Survives app deletion (optional)

**Android**: Android Keystore
- Hardware-backed encryption (if available)
- Keys never leave secure hardware
- Protected by device lock screen

### SPV Security

- Header-only sync (minimal bandwidth)
- Merkle proof verification
- Bloom filter privacy
- Checkpoint verification
- Multi-peer validation

---

## API Reference

### Wallet Class

#### Properties

```swift
// iOS
var balance: Balance { get }
var receiveAddress: String { get }
var transactions: [Transaction] { get }
var isSynced: Bool { get }
var blockHeight: Int { get }
```

```kotlin
// Android
val balance: Balance
val receiveAddress: String
val transactions: List<Transaction>
val isSynced: Boolean
val blockHeight: Int
```

#### Methods

```swift
// iOS
func startSync(progressCallback: @escaping (Int) -> Void)
func stopSync()
func send(to: String, amount: Double, feeRate: FeeRate) throws -> Transaction
func sendAll(to: String, feeRate: FeeRate) throws -> Transaction
func estimateFee(to: String, amount: Double, feeRate: FeeRate) -> Int64
func transaction(id: String) -> Transaction?
```

```kotlin
// Android
fun startSync(progressCallback: (Int) -> Unit)
fun stopSync()
fun send(to: String, amount: Double, feeRate: FeeRate): Transaction
fun sendAll(to: String, feeRate: FeeRate): Transaction
fun estimateFee(to: String, amount: Double, feeRate: FeeRate): Long
fun getTransaction(id: String): Transaction?
```

### Transaction Class

```swift
// iOS
struct Transaction {
    let id: String
    let amount: Double
    let fee: Int64
    let direction: Direction  // .sent or .received
    let timestamp: Date
    let confirmations: Int
    let blockHeight: Int?
    let address: String
}
```

```kotlin
// Android
data class Transaction(
    val id: String,
    val amount: Double,
    val fee: Long,
    val direction: Direction,  // SENT or RECEIVED
    val timestamp: Long,
    val confirmations: Int,
    val blockHeight: Int?,
    val address: String
)
```

### Balance Class

```swift
// iOS
struct Balance {
    let confirmed: Double
    let unconfirmed: Double
    var total: Double { confirmed + unconfirmed }
}
```

```kotlin
// Android
data class Balance(
    val confirmed: Double,
    val unconfirmed: Double
) {
    val total: Double get() = confirmed + unconfirmed
}
```

---

## Examples

### Complete iOS App Example

```swift
import UIKit
import INTcoinKit

class WalletViewController: UIViewController {
    var wallet: Wallet!

    @IBOutlet weak var balanceLabel: UILabel!
    @IBOutlet weak var addressLabel: UILabel!
    @IBOutlet weak var tableView: UITableView!

    override func viewDidLoad() {
        super.viewDidLoad()

        // Initialize or restore wallet
        if let savedMnemonic = KeychainHelper.load("mnemonic") {
            let mnemonic = try! Mnemonic(words: savedMnemonic.components(separatedBy: " "))
            wallet = try! Wallet.restore(mnemonic: mnemonic)
        } else {
            let mnemonic = Mnemonic.generate(strength: .words24)
            wallet = try! Wallet.create(mnemonic: mnemonic)
            KeychainHelper.save("mnemonic", mnemonic.words.joined(separator: " "))
        }

        // Setup callbacks
        wallet.onBalanceChanged = { [weak self] balance in
            DispatchQueue.main.async {
                self?.balanceLabel.text = "\(balance.total) INT"
            }
        }

        wallet.onTransactionReceived = { [weak self] tx in
            DispatchQueue.main.async {
                self?.tableView.reloadData()
                self?.showNotification("Received \(tx.amount) INT")
            }
        }

        // Start sync
        wallet.startSync { progress in
            print("Sync: \(progress)%")
        }

        // Update UI
        addressLabel.text = wallet.receiveAddress
        balanceLabel.text = "\(wallet.balance.total) INT"
    }

    @IBAction func sendTapped() {
        let alert = UIAlertController(title: "Send INT", message: nil, preferredStyle: .alert)
        alert.addTextField { $0.placeholder = "Address" }
        alert.addTextField { $0.placeholder = "Amount"; $0.keyboardType = .decimalPad }

        alert.addAction(UIAlertAction(title: "Send", style: .default) { [weak self] _ in
            guard let address = alert.textFields?[0].text,
                  let amountText = alert.textFields?[1].text,
                  let amount = Double(amountText) else { return }

            do {
                let tx = try self?.wallet.send(to: address, amount: amount, feeRate: .normal)
                self?.showAlert("Transaction sent: \(tx?.id ?? "")")
            } catch {
                self?.showAlert("Error: \(error.localizedDescription)")
            }
        })

        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))
        present(alert, animated: true)
    }
}
```

### Complete Android App Example

```kotlin
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import org.intcoin.sdk.*
import kotlinx.android.synthetic.main.activity_wallet.*

class WalletActivity : AppCompatActivity() {
    private lateinit var wallet: Wallet
    private val adapter = TransactionAdapter()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_wallet)

        // Initialize or restore wallet
        val savedMnemonic = getSharedPreferences("wallet", MODE_PRIVATE)
            .getString("mnemonic", null)

        wallet = if (savedMnemonic != null) {
            val words = savedMnemonic.split(" ")
            Wallet.restore(Mnemonic(words), Network.MAINNET)
        } else {
            val mnemonic = Mnemonic.generate(WordCount.WORDS_24)
            getSharedPreferences("wallet", MODE_PRIVATE).edit()
                .putString("mnemonic", mnemonic.words.joinToString(" "))
                .apply()
            Wallet.create(mnemonic)
        }

        // Setup UI
        recyclerView.layoutManager = LinearLayoutManager(this)
        recyclerView.adapter = adapter

        // Observe wallet events
        wallet.balanceChanged.observe(this) { balance ->
            balanceText.text = "${balance.total} INT"
        }

        wallet.transactionReceived.observe(this) { tx ->
            adapter.addTransaction(tx)
            showNotification("Received ${tx.amount} INT")
        }

        // Start sync
        wallet.startSync { progress ->
            syncProgress.progress = progress
        }

        // Update UI
        addressText.text = wallet.receiveAddress
        balanceText.text = "${wallet.balance.total} INT"
        adapter.setTransactions(wallet.transactions)

        // Send button
        sendButton.setOnClickListener {
            showSendDialog()
        }
    }

    private fun showSendDialog() {
        val dialog = AlertDialog.Builder(this)
            .setTitle("Send INT")
            .setView(R.layout.dialog_send)
            .setPositiveButton("Send") { dialog, _ ->
                val view = (dialog as AlertDialog).findViewById<View>(R.id.dialogView)
                val address = view.findViewById<EditText>(R.id.addressInput).text.toString()
                val amount = view.findViewById<EditText>(R.id.amountInput).text.toString().toDouble()

                try {
                    val tx = wallet.send(address, amount, FeeRate.NORMAL)
                    Toast.makeText(this, "Sent: ${tx.id}", Toast.LENGTH_SHORT).show()
                } catch (e: Exception) {
                    Toast.makeText(this, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
            .setNegativeButton("Cancel", null)
            .create()
        dialog.show()
    }
}
```

---

## Best Practices

### Security

1. **Never log sensitive data**
   ```swift
   // DON'T
   print("Mnemonic: \(mnemonic.words)")

   // DO
   print("Wallet created successfully")
   ```

2. **Use secure storage**
   ```swift
   // iOS: Use Keychain
   KeychainHelper.save("mnemonic", mnemonic)

   // Android: Use EncryptedSharedPreferences
   securePrefs.edit().putString("mnemonic", mnemonic).apply()
   ```

3. **Validate addresses**
   ```swift
   if !Address.isValid(addressString) {
       throw WalletError.invalidAddress
   }
   ```

### Performance

1. **Sync on background thread**
   ```swift
   DispatchQueue.global(qos: .background).async {
       wallet.startSync { progress in
           DispatchQueue.main.async {
               updateUI(progress)
           }
       }
   }
   ```

2. **Cache balances**
   ```swift
   private var cachedBalance: Balance?
   private var cacheTime: Date?

   func getBalance() -> Balance {
       if let cached = cachedBalance,
          let time = cacheTime,
          Date().timeIntervalSince(time) < 10 {
           return cached
       }
       cachedBalance = wallet.balance
       cacheTime = Date()
       return cachedBalance!
   }
   ```

3. **Batch updates**
   ```kotlin
   wallet.transactionReceived.observe(this) { tx ->
       pendingUpdates.add(tx)
   }

   // Update UI in batches
   handler.postDelayed({
       adapter.addTransactions(pendingUpdates)
       pendingUpdates.clear()
   }, 1000)
   ```

### Error Handling

```swift
do {
    let tx = try wallet.send(to: address, amount: amount, feeRate: .normal)
} catch WalletError.insufficientFunds {
    showAlert("Insufficient funds")
} catch WalletError.invalidAddress {
    showAlert("Invalid address")
} catch WalletError.networkError(let message) {
    showAlert("Network error: \(message)")
} catch {
    showAlert("Unknown error: \(error)")
}
```

---

## Resources

- **GitHub Repositories**:
  - iOS SDK: https://github.com/InternationalCoin/intcoin-ios-sdk
  - Android SDK: https://github.com/InternationalCoin/intcoin-android-sdk
- **Documentation**:
  - [Mobile Wallet Guide](MOBILE_WALLET.md)
  - [SPV and Bloom Filters](SPV_AND_BLOOM_FILTERS.md)
- **Examples**:
  - iOS App: [mobile/ios/](../mobile/ios/)
  - Android App: [mobile/android/](../mobile/android/)
- **Support**:
  - Issues: https://github.com/InternationalCoin/intcoin/issues
  - Discussions: https://github.com/InternationalCoin/intcoin/discussions

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
