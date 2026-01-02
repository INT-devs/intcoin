# INTcoin Android SDK

Native Android SDK for building INTcoin lightweight wallet applications.

## Features

- ✅ **Lightweight SPV Client** - Download only headers (~152 bytes per block)
- ✅ **Secure Wallet** - BIP39 mnemonic, password encryption
- ✅ **Native Kotlin API** - Modern Kotlin interface with coroutines support
- ✅ **Small Footprint** - < 50 MB app size, < 1 MB bandwidth per session
- ✅ **Fast Sync** - Syncs in < 30 seconds on modern devices
- ✅ **QR Code Support** - Payment URI generation and scanning (ZXing)
- ✅ **Biometric Auth** - Fingerprint and face unlock support
- ✅ **Jetpack Compose** - Modern declarative UI framework

## Requirements

- Android 7.0 (API 24)+
- Kotlin 1.9+
- Android Studio Hedgehog (2023.1.1)+
- Gradle 8.0+

## Installation

### Gradle (build.gradle.kts)

```kotlin
dependencies {
    implementation("org.intcoin:intcoin-sdk-android:1.2.0-beta")
}
```

### Maven

```xml
<dependency>
    <groupId>org.intcoin</groupId>
    <artifactId>intcoin-sdk-android</artifactId>
    <version>1.2.0-beta</version>
</dependency>
```

## Quick Start

```kotlin
import org.intcoin.sdk.*

// Initialize SDK
val sdk = INTcoinSDK.create(context, network = "testnet")

// Create new wallet
val mnemonic = sdk.createWallet(password = "secure_password")
println("Save this mnemonic: $mnemonic")

// Get balance
val balance = sdk.getBalance()
println("Balance: ${balance.totalFormatted}")

// Generate receiving address
val address = sdk.getNewAddress()
println("Your address: $address")

// Send transaction
val txHash = sdk.sendTransaction(
    toAddress = "int1q...",
    amountINTS = 1_000_000L  // 1 INT = 1,000,000 INTS
)
println("Sent: ${txHash.toHex()}")

// Remember to close SDK when done
sdk.close()
```

## Currency Units

INTcoin uses **INTS** as the base unit (similar to Bitcoin's satoshis):

- 1 INT = 1,000,000 INTS
- Minimum amount: 1 INTS (0.000001 INT)

```kotlin
// Parse user input
val amountINTS = INTcoinSDK.parseINTAmount("1.5")  // Returns 1,500,000 INTS

// Format for display
val formatted = INTcoinSDK.formatINTS(1_500_000L)  // Returns "1.500000 INT"
```

## Address Validation

```kotlin
val isValid = INTcoinSDK.validateAddress("int1q...")
if (!isValid) {
    Toast.makeText(context, "Invalid INTcoin address", Toast.LENGTH_SHORT).show()
}
```

## QR Code Integration

### Generate QR Code

```kotlin
import com.journeyapps.barcodescanner.BarcodeEncoder

fun generateQRCode(address: String, amountINTS: Long): Bitmap {
    val uri = INTcoinSDK.generatePaymentURI(
        address = address,
        amountINTS = amountINTS,
        label = "Coffee Shop",
        message = "Payment for order #123"
    )

    val barcodeEncoder = BarcodeEncoder()
    return barcodeEncoder.encodeBitmap(
        uri,
        BarcodeFormat.QR_CODE,
        512,
        512
    )
}
```

### Scan QR Code

```kotlin
import com.journeyapps.barcodescanner.ScanContract
import com.journeyapps.barcodescanner.ScanOptions

class WalletActivity : AppCompatActivity() {
    private val qrScanLauncher = registerForActivityResult(ScanContract()) { result ->
        result.contents?.let { uri ->
            if (uri.startsWith("intcoin:")) {
                // Parse payment URI
                processPaymentURI(uri)
            }
        }
    }

    fun scanQRCode() {
        val options = ScanOptions().apply {
            setPrompt("Scan INTcoin address")
            setBeepEnabled(true)
            setOrientationLocked(false)
        }
        qrScanLauncher.launch(options)
    }
}
```

## Biometric Authentication

```kotlin
import androidx.biometric.BiometricPrompt
import androidx.core.content.ContextCompat

fun openWalletWithBiometrics(activity: FragmentActivity) {
    val executor = ContextCompat.getMainExecutor(activity)
    val biometricPrompt = BiometricPrompt(
        activity,
        executor,
        object : BiometricPrompt.AuthenticationCallback() {
            override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                // Load password from EncryptedSharedPreferences
                val password = loadPasswordFromKeystore()
                sdk.openWallet(password)
            }

            override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                Toast.makeText(activity, "Authentication error: $errString", Toast.LENGTH_SHORT).show()
            }
        }
    )

    val promptInfo = BiometricPrompt.PromptInfo.Builder()
        .setTitle("Unlock INTcoin Wallet")
        .setSubtitle("Use biometric to unlock")
        .setNegativeButtonText("Cancel")
        .build()

    biometricPrompt.authenticate(promptInfo)
}
```

## Secure Storage with EncryptedSharedPreferences

```kotlin
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

fun savePassword(context: Context, password: String) {
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

    prefs.edit().putString("wallet_password", password).apply()
}

fun loadPassword(context: Context): String? {
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

    return prefs.getString("wallet_password", null)
}
```

## Jetpack Compose Example

```kotlin
@Composable
fun WalletScreen(sdk: INTcoinSDK) {
    var balance by remember { mutableStateOf<Balance?>(null) }
    var syncProgress by remember { mutableStateOf(0.0) }

    LaunchedEffect(Unit) {
        balance = sdk.getBalance()
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // Balance Card
        Card(
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text("Total Balance", style = MaterialTheme.typography.bodySmall)
                Text(
                    balance?.totalFormatted ?: "Loading...",
                    style = MaterialTheme.typography.headlineLarge
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Sync Progress
        if (syncProgress < 1.0) {
            LinearProgressIndicator(
                progress = syncProgress.toFloat(),
                modifier = Modifier.fillMaxWidth()
            )
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Action Buttons
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            Button(onClick = { /* Show receive screen */ }) {
                Icon(Icons.Default.ArrowDownward, "Receive")
                Spacer(Modifier.width(8.dp))
                Text("Receive")
            }

            Button(onClick = { /* Show send screen */ }) {
                Icon(Icons.Default.ArrowUpward, "Send")
                Spacer(Modifier.width(8.dp))
                Text("Send")
            }
        }
    }
}
```

## Architecture

The SDK consists of three layers:

1. **C++ Core** (`libintcoin_core.so`) - SPV client, bloom filters, transaction validation
2. **JNI Bridge** - Native method interface between Kotlin and C++
3. **Kotlin Wrapper** (`INTcoinSDK.kt`) - Native Kotlin API

This architecture ensures:
- Maximum code reuse across platforms
- Native platform experience
- Easy maintenance and updates

## Performance Characteristics

- **Initial sync**: < 30 seconds (header-only)
- **Bandwidth**: < 1 MB per session
- **APK size**: < 50 MB (including SDK)
- **Memory**: < 100 MB typical usage
- **Battery**: Minimal impact (SPV mode)

## Permissions

Add to `AndroidManifest.xml`:

```xml
<!-- Network access for blockchain sync -->
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />

<!-- Camera for QR code scanning -->
<uses-permission android:name="android.permission.CAMERA" />

<!-- Biometric authentication -->
<uses-permission android:name="android.permission.USE_BIOMETRIC" />
```

## ProGuard Rules

If using ProGuard/R8, add to `proguard-rules.pro`:

```proguard
# INTcoin SDK
-keep class org.intcoin.sdk.** { *; }
-keepclassmembers class org.intcoin.sdk.** { *; }

# Native methods
-keepclasseswithmembernames class * {
    native <methods>;
}
```

## Error Handling

```kotlin
try {
    sdk.openWallet(password)
} catch (e: INTcoinException) {
    when {
        e.message?.contains("password") == true -> {
            Toast.makeText(context, "Incorrect password", Toast.LENGTH_SHORT).show()
        }
        else -> {
            Toast.makeText(context, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }
}
```

## Support

- **Documentation**: https://docs.intcoin.org/mobile-sdk/android
- **Issues**: https://github.com/intcoin/intcoin/issues
- **Discord**: https://discord.gg/intcoin

## License

MIT License - see [LICENSE](../../LICENSE) for details.

---

**Built with ❤️ by the INTcoin Core team**
