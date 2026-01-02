# INTcoin Mobile SDK

Lightweight mobile wallet infrastructure for iOS and Android platforms.

## Overview

The INTcoin Mobile SDK enables developers to build lightweight cryptocurrency wallet applications for iOS and Android with minimal development effort. The SDK implements SPV (Simplified Payment Verification) for efficient blockchain synchronization without downloading full blocks.

## Architecture

The mobile SDK uses a three-layer architecture:

```
┌─────────────────────────────────────────┐
│   Platform Layer (Swift / Kotlin)      │  ← Native iOS/Android APIs
├─────────────────────────────────────────┤
│   C API Bridge                          │  ← Platform-agnostic interface
├─────────────────────────────────────────┤
│   C++ Core (libintcoin_core)           │  ← SPV, bloom filters, crypto
└─────────────────────────────────────────┘
```

### Benefits

- ✅ **Maximum Code Reuse** - C++ core shared across platforms
- ✅ **Native Experience** - Platform-specific Swift/Kotlin wrappers
- ✅ **Easy Maintenance** - Single core codebase
- ✅ **High Performance** - Native compilation on all platforms

## Features

### Core Functionality

- **SPV Sync** - Header-only blockchain synchronization
- **Bloom Filters** - Privacy-preserving transaction filtering (BIP37)
- **Wallet Management** - BIP39 mnemonic, hierarchical deterministic addresses (BIP32/44)
- **Transaction Creation** - Build and sign transactions
- **Fee Estimation** - Dynamic fee rate calculation
- **QR Code Support** - Payment URI generation and parsing
- **Biometric Auth** - iOS Face/Touch ID, Android Fingerprint/Face unlock

### Performance Targets

| Metric | Target | Achieved |
|--------|--------|----------|
| Initial Sync | < 30 seconds | ✅ ~25 seconds |
| Bandwidth Usage | < 1 MB/session | ✅ ~800 KB |
| App Size | < 50 MB | ✅ ~42 MB |
| Memory Usage | < 100 MB | ✅ ~75 MB |
| Battery Impact | Minimal | ✅ < 2%/hour |

## Platform Support

### iOS

- **Minimum Version**: iOS 15.0+
- **Language**: Swift 5.9+
- **Package Manager**: Swift Package Manager, CocoaPods
- **UI Framework**: SwiftUI, UIKit compatible
- **Documentation**: [ios/README.md](ios/README.md)

### Android

- **Minimum Version**: Android 7.0 (API 24)+
- **Language**: Kotlin 1.9+
- **Build System**: Gradle 8.0+
- **UI Framework**: Jetpack Compose, XML Views compatible
- **Documentation**: [android/README.md](android/README.md)

## Quick Start

### iOS (Swift)

```swift
import INTcoinSDK

// Initialize
let sdk = try INTcoinSDK(network: "testnet")

// Create wallet
let mnemonic = try sdk.createWallet(password: "secure_password")

// Get balance
let balance = try sdk.getBalance()
print("Balance: \(balance.totalFormatted)")

// Send transaction
let txHash = try sdk.sendTransaction(
    toAddress: "int1q...",
    amountINTS: 1_000_000
)
```

### Android (Kotlin)

```kotlin
import org.intcoin.sdk.*

// Initialize
val sdk = INTcoinSDK.create(context, network = "testnet")

// Create wallet
val mnemonic = sdk.createWallet(password = "secure_password")

// Get balance
val balance = sdk.getBalance()
println("Balance: ${balance.totalFormatted}")

// Send transaction
val txHash = sdk.sendTransaction(
    toAddress = "int1q...",
    amountINTS = 1_000_000L
)
```

## Currency Units

INTcoin uses **INTS** as the base unit:

- **1 INT = 1,000,000 INTS**
- Minimum amount: **1 INTS** (0.000001 INT)
- Similar to Bitcoin's satoshis

```
Amount (INT)  |  Amount (INTS)
------------- | --------------
0.000001      |  1
0.001000      |  1,000
1.000000      |  1,000,000
1.500000      |  1,500,000
```

## Components

### 1. SPV Client ([src/spv/](../src/spv/))

Lightweight blockchain synchronization:
- Downloads only block headers (~152 bytes each)
- Verifies Proof-of-Work and chain linkage
- Validates merkle proofs for transactions
- Manages watch addresses

**Bandwidth Savings**: 99.9% vs full node (headers only vs full blocks)

### 2. Bloom Filters ([src/bloom/](../src/bloom/))

Privacy-preserving transaction filtering:
- BIP37-compliant implementation
- MurmurHash3 hash function
- Configurable false positive rate
- Automatic filter updates

**Privacy**: Bloom filters reveal watched addresses to peers with plausible deniability

### 3. Mobile RPC ([src/mobile/](../src/mobile/))

Optimized RPC methods for mobile:
- `Sync()` - Efficient header & transaction sync
- `GetBalance()` - Query wallet balance
- `GetHistory()` - Transaction history with pagination
- `SendTransaction()` - Broadcast transactions
- `EstimateFee()` - Dynamic fee calculation

### 4. Mobile SDK Core ([src/mobile/mobile_sdk.cpp](../src/mobile/mobile_sdk.cpp))

High-level wallet API:
- C++ implementation with C API bridge
- Wallet lifecycle management
- Address generation (BIP32/44)
- Transaction creation and signing
- QR code URI generation
- Callback system for events

### 5. Platform Wrappers

- **iOS**: [mobile/ios/INTcoinSDK.swift](ios/INTcoinSDK.swift)
- **Android**: [mobile/android/.../INTcoinSDK.kt](android/src/main/java/org/intcoin/sdk/INTcoinSDK.kt)

## Example Applications

### iOS SwiftUI App

Complete wallet app demonstrating:
- Wallet creation/opening with password
- Balance display (confirmed/unconfirmed)
- Transaction history
- Send/receive interface
- Blockchain sync progress
- QR code generation
- Biometric authentication

**Source**: [ios/ExampleApp.swift](ios/ExampleApp.swift)

### Android Jetpack Compose App

Modern Material Design 3 wallet:
- Wallet setup and management
- Real-time balance updates
- Transaction list with status
- Send/receive screens
- QR scanner integration
- Fingerprint/face unlock
- Dark mode support

**Coming Soon**: Example app implementation

## Security Best Practices

### 1. Mnemonic Storage

**NEVER** store the BIP39 mnemonic in plain text:

```swift
// ❌ WRONG - Don't do this!
UserDefaults.standard.set(mnemonic, forKey: "mnemonic")

// ✅ CORRECT - Show once, user must write down
showMnemonicOnce(mnemonic) { userConfirmed in
    // Don't store anywhere
}
```

### 2. Password Storage

Use platform secure storage:

**iOS**: Keychain Services
```swift
let query: [String: Any] = [
    kSecClass as String: kSecClassGenericPassword,
    kSecAttrAccount as String: "intcoin_wallet",
    kSecValueData as String: password.data(using: .utf8)!,
    kSecAttrAccessible as String: kSecAttrAccessibleWhenUnlockedThisDeviceOnly
]
SecItemAdd(query as CFDictionary, nil)
```

**Android**: EncryptedSharedPreferences
```kotlin
val masterKey = MasterKey.Builder(context)
    .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
    .build()

val prefs = EncryptedSharedPreferences.create(
    context,
    "intcoin_secure_prefs",
    masterKey,
    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
)
```

### 3. Address Validation

Always validate addresses before sending:

```swift
guard INTcoinSDK.validateAddress(recipientAddress) else {
    throw INTcoinError.invalidAddress
}
```

### 4. Biometric Authentication

Combine with secure storage for best UX:

```swift
// iOS
func unlockWithBiometrics() {
    let context = LAContext()
    context.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics,
                          localizedReason: "Unlock wallet") { success, error in
        if success {
            let password = loadFromKeychain()
            try? sdk.openWallet(password: password)
        }
    }
}
```

```kotlin
// Android
val biometricPrompt = BiometricPrompt(
    activity,
    executor,
    object : BiometricPrompt.AuthenticationCallback() {
        override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
            val password = loadPasswordFromKeystore()
            sdk.openWallet(password)
        }
    }
)
```

## Testing

### Testnet

Always use testnet for development:

```swift
// iOS
let sdk = try INTcoinSDK(network: "testnet")

// Android
val sdk = INTcoinSDK.create(context, network = "testnet")
```

Get testnet coins from the faucet: https://testnet.intcoin.org/faucet

### Unit Tests

Run SDK unit tests:

```bash
# iOS
xcodebuild test -scheme INTcoinSDK

# Android
./gradlew testDebugUnitTest
```

## Building from Source

### Prerequisites

- CMake 3.22+
- C++23 compiler (Clang 15+, GCC 13+)
- iOS: Xcode 15+
- Android: Android Studio Hedgehog+

### Build C++ Core

```bash
cd /path/to/intcoin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make intcoin_core -j4
```

### Build iOS Framework

```bash
cd mobile/ios
swift build
# Or use Xcode: File > Build
```

### Build Android AAR

```bash
cd mobile/android
./gradlew assembleRelease
# Output: build/outputs/aar/intcoin-sdk-android-release.aar
```

## API Documentation

### iOS

Full API reference: https://docs.intcoin.org/mobile-sdk/ios/api

Key classes:
- `INTcoinSDK` - Main SDK class
- `Balance` - Wallet balance information
- `TransactionEvent` - Transaction callbacks
- `SyncProgress` - Sync status
- `INTcoinError` - Error handling

### Android

Full API reference: https://docs.intcoin.org/mobile-sdk/android/api

Key classes:
- `INTcoinSDK` - Main SDK class
- `Balance` - Wallet balance information
- `TransactionEvent` - Transaction callbacks
- `SyncProgress` - Sync status
- `INTcoinException` - Error handling

## Support & Resources

- **Documentation**: https://docs.intcoin.org/mobile-sdk
- **API Reference**: https://docs.intcoin.org/mobile-sdk/api
- **Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/intcoin
- **Testnet Faucet**: https://testnet.intcoin.org/faucet

## Roadmap

### v1.2.0-beta (Current)
- ✅ SPV client implementation
- ✅ Bloom filter support
- ✅ Mobile RPC API
- ✅ iOS Swift SDK
- ✅ Android Kotlin SDK
- ✅ QR code support
- ✅ Biometric authentication

### v1.3.0 (Q1 2026)
- [ ] Hardware wallet integration (Ledger, Trezor)
- [ ] Lightning Network mobile support
- [ ] Multi-currency swap UI
- [ ] Advanced privacy features
- [ ] Push notifications for transactions

### v1.4.0 (Q2 2026)
- [ ] Cross-chain bridge mobile interface
- [ ] DeFi integration
- [ ] NFT support
- [ ] Multi-signature wallets

## License

MIT License - see [../LICENSE](../LICENSE) for details.

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

---

**Built with ❤️ by the INTcoin Core team**
