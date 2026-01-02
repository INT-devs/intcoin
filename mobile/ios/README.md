# INTcoin iOS SDK

Native iOS SDK for building INTcoin lightweight wallet applications.

## Features

- ✅ **Lightweight SPV Client** - Download only headers (~152 bytes per block)
- ✅ **Secure Wallet** - BIP39 mnemonic, password encryption
- ✅ **Native Swift API** - Modern Swift interface with async/await support
- ✅ **Small Footprint** - < 50 MB app size, < 1 MB bandwidth per session
- ✅ **Fast Sync** - Syncs in < 30 seconds on modern devices
- ✅ **QR Code Support** - Payment URI generation and scanning
- ✅ **SwiftUI Ready** - Example app with modern SwiftUI interface

## Requirements

- iOS 15.0+ / macOS 12.0+
- Xcode 15.0+
- Swift 5.9+

## Installation

### Swift Package Manager

Add the following to your `Package.swift`:

```swift
dependencies: [
    .package(url: "https://github.com/intcoin/intcoin-ios-sdk.git", from: "1.2.0")
]
```

Or add via Xcode:
1. File > Add Packages...
2. Enter package URL: `https://github.com/intcoin/intcoin-ios-sdk.git`
3. Select version 1.2.0+

### CocoaPods

```ruby
pod 'INTcoinSDK', '~> 1.2.0'
```

## Quick Start

```swift
import INTcoinSDK

// Initialize SDK
let sdk = try INTcoinSDK(network: "testnet")

// Create new wallet
let mnemonic = try sdk.createWallet(password: "secure_password")
print("Save this mnemonic: \(mnemonic)")

// Get balance
let balance = try sdk.getBalance()
print("Balance: \(balance.totalFormatted)")

// Generate receiving address
let address = try sdk.getNewAddress()
print("Your address: \(address)")

// Send transaction
let txHash = try sdk.sendTransaction(
    toAddress: "int1q...",
    amountINTS: 1_000_000  // 1 INT = 1,000,000 INTS
)
```

## Currency Units

INTcoin uses **INTS** as the base unit (similar to Bitcoin's satoshis):

- 1 INT = 1,000,000 INTS
- Minimum amount: 1 INTS (0.000001 INT)

```swift
// Parse user input
let amountINTS = INTcoinSDK.parseINTAmount("1.5")  // Returns 1,500,000 INTS

// Format for display
let formatted = INTcoinSDK.formatINTS(1_500_000)  // Returns "1.500000 INT"
```

## Address Validation

```swift
let isValid = INTcoinSDK.validateAddress("int1q...")
if !isValid {
    print("Invalid INTcoin address")
}
```

## QR Code Integration

### Generate Payment URI

```swift
let uri = INTcoinSDK.generatePaymentURI(
    address: "int1q...",
    amountINTS: 1_500_000,
    label: "Coffee Shop",
    message: "Payment for order #123"
)
// Returns: "intcoin:int1q...?amount=1.500000&label=Coffee%20Shop&message=Payment%20for%20order%20%23123"

// Convert to QR code (using CoreImage)
let data = uri.data(using: .utf8)
let filter = CIFilter(name: "CIQRCodeGenerator")
filter?.setValue(data, forKey: "inputMessage")
let qrImage = filter?.outputImage
```

### Scan QR Code

Use AVFoundation or Vision framework to scan QR codes containing `intcoin:` URIs.

## Biometric Authentication

Integrate with iOS biometric authentication:

```swift
import LocalAuthentication

func openWalletWithBiometrics() {
    let context = LAContext()
    var error: NSError?

    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: &error) {
        context.evaluatePolicy(
            .deviceOwnerAuthenticationWithBiometrics,
            localizedReason: "Unlock your INTcoin wallet"
        ) { success, error in
            if success {
                // Load password from Keychain
                let password = loadFromKeychain()
                try? sdk.openWallet(password: password)
            }
        }
    }
}
```

## Example App

See [ExampleApp.swift](ExampleApp.swift) for a complete SwiftUI wallet application demonstrating:

- Wallet creation and opening
- Balance display
- Transaction history
- Send/receive interface
- Blockchain sync progress
- QR code support

## Architecture

The SDK consists of three layers:

1. **C++ Core** (`libintcoin_core.a`) - SPV client, bloom filters, transaction validation
2. **C API** (`mobile_sdk.h`) - Platform-agnostic C interface
3. **Swift Wrapper** (`INTcoinSDK.swift`) - Native Swift API

This architecture ensures:
- Maximum code reuse across platforms
- Native platform experience
- Easy maintenance and updates

## Performance Characteristics

- **Initial sync**: < 30 seconds (header-only)
- **Bandwidth**: < 1 MB per session
- **App size**: < 50 MB (including SDK)
- **Memory**: < 100 MB typical usage
- **Battery**: Minimal impact (SPV mode)

## Security Best Practices

1. **Never store mnemonic in UserDefaults** - Use Keychain Services
2. **Enable biometric authentication** - Improves UX and security
3. **Validate addresses** - Always validate before sending
4. **Use testnet for development** - Prevent accidental loss
5. **Show mnemonic once** - During wallet creation only

## Keychain Storage Example

```swift
import Security

func saveToKeychain(password: String) {
    let data = password.data(using: .utf8)!
    let query: [String: Any] = [
        kSecClass as String: kSecClassGenericPassword,
        kSecAttrAccount as String: "intcoin_wallet",
        kSecValueData as String: data,
        kSecAttrAccessible as String: kSecAttrAccessibleWhenUnlockedThisDeviceOnly
    ]
    SecItemAdd(query as CFDictionary, nil)
}

func loadFromKeychain() -> String? {
    let query: [String: Any] = [
        kSecClass as String: kSecClassGenericPassword,
        kSecAttrAccount as String: "intcoin_wallet",
        kSecReturnData as String: true
    ]

    var result: AnyObject?
    let status = SecItemCopyMatching(query as CFDictionary, &result)

    guard status == errSecSuccess,
          let data = result as? Data,
          let password = String(data: data, encoding: .utf8) else {
        return nil
    }

    return password
}
```

## Error Handling

All SDK methods that can fail throw `INTcoinError`:

```swift
do {
    try sdk.openWallet(password: password)
} catch INTcoinError.walletOpenFailed {
    print("Incorrect password")
} catch {
    print("Unexpected error: \(error.localizedDescription)")
}
```

## Support

- **Documentation**: https://docs.intcoin.org/mobile-sdk/ios
- **Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/intcoin

## License

MIT License - see [LICENSE](../../LICENSE) for details.

---

**Built with ❤️ by the INTcoin Core team**
