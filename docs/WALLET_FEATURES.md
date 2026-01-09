# INTcoin Desktop Wallet Features

**Version**: 1.3.0-beta
**Status**: ✅ Production Beta
**Last Updated**: January 9, 2026

The INTcoin Qt6 Desktop Wallet provides a comprehensive, user-friendly interface for managing your INTcoin funds with advanced features including hardware wallet support, integrated mining, QR codes, and PDF invoicing.

---

## Table of Contents

1. [Overview](#overview)
2. [QR Code Generation](#qr-code-generation)
3. [Hardware Wallet Support](#hardware-wallet-support)
4. [PDF Invoice Generation](#pdf-invoice-generation)
5. [Integrated Mining](#integrated-mining)
6. [Lightning Network](#lightning-network)
7. [Security Features](#security-features)

---

## Overview

### Key Features

| Feature | Status | Description |
|---------|--------|-------------|
| **HD Wallet (BIP32/39/44)** | ✅ Complete | Hierarchical deterministic wallet with mnemonic seed |
| **QR Code Generation** | ✅ v1.3.0 | Generate QR codes for addresses and Lightning invoices |
| **Hardware Wallets** | ✅ v1.3.0 | Ledger Nano S/X, Trezor, Coldcard support |
| **PDF Invoices** | ✅ v1.3.0 | Professional PDF invoice generation |
| **Integrated Mining** | ✅ v1.3.0 | Solo and pool mining from wallet |
| **Lightning Network** | ✅ Complete | Full Lightning Network support (BOLT 1-11) |
| **Post-Quantum Crypto** | ✅ Complete | Dilithium3 + Kyber768 |

---

## QR Code Generation

**New in v1.3.0**

### Overview

Generate QR codes for easy payment receiving using industry-standard libqrencode. Supports both on-chain addresses and Lightning invoices.

### Features

- **Multiple Formats**: SVG export, PNG rendering, on-screen display
- **URI Schemes**: `intcoin:address?amount=X&label=Y` format
- **Error Correction**: 4 levels (LOW, MEDIUM, QUARTILE, HIGH)
- **Lightning Support**: Full Lightning invoice QR code generation
- **Copy & Save**: Copy to clipboard or save as image file

### Usage

#### Generate Address QR Code

1. Navigate to **Receive** tab
2. Enter optional amount and label
3. Click **Show QR Code**
4. QR code displays with options to:
   - Copy image to clipboard
   - Copy text address to clipboard
   - Save as PNG file

#### Generate Lightning Invoice QR Code

1. Navigate to **Lightning** tab
2. Click **Create Invoice**
3. Enter amount and description
4. QR code automatically generated
5. Scan with mobile wallet to pay

### API Reference

```cpp
#include <intcoin/qrcode.h>

// Generate QR code for address
auto qr = QRCode::GenerateAddress(
    "int1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080",
    1.5,  // amount in INT
    "Payment for services"  // label
);

// Generate SVG
auto svg = QRCode::GenerateSVG(
    "intcoin:int1qw...",
    4,  // module size
    4,  // border
    QRCode::ECLevel::HIGH
);
```

### Technical Details

- **Library**: libqrencode 4.1.1
- **Standards**: ISO/IEC 18004:2015 (QR Code specification)
- **Performance**: ~5ms generation time for typical addresses
- **Max Data**: Up to 2,953 bytes (version 40, LOW EC)

---

## Hardware Wallet Support

**New in v1.3.0**

### Overview

Integrate hardware wallets for maximum security. Private keys never leave the device, and all transactions require physical confirmation.

### Supported Devices

| Device | Status | Features |
|--------|--------|----------|
| **Ledger Nano S** | ✅ Full Support | APDU protocol, address derivation, signing |
| **Ledger Nano X** | ✅ Full Support | Bluetooth + USB, same features as Nano S |
| **Trezor One** | 🟡 Partial | Address derivation (signing in development) |
| **Trezor Model T** | 🟡 Partial | Touchscreen verification |
| **Coldcard Mk4** | 🟡 Partial | Air-gapped signing planned |

### Setup Process

1. **Connect Device**: Plug in hardware wallet via USB
2. **Auto-Detection**: Wallet automatically detects device
3. **Unlock**: Enter PIN on device
4. **Derive Addresses**: Generate INTcoin addresses using BIP44 path `m/44'/2210'/0'/0/0`
5. **Sign Transactions**: Confirm on device screen

### Features

- **USB HID Communication**: Using hidapi library
- **BIP44 Derivation**: Standard HD wallet paths
- **On-Device Verification**: All addresses shown on device screen
- **Multi-Device**: Switch between multiple hardware wallets
- **Device Monitoring**: Auto-reconnect when device plugged in

### Security Model

**What stays on device:**
- Private keys (never exported)
- Seed phrase (stored in secure element)

**What device verifies:**
- Transaction recipients
- Transaction amounts
- Fee amounts

**Physical confirmation required for:**
- Every transaction
- Address verification (optional)
- Device wipe/reset

### API Usage

```cpp
#include <intcoin/qt/hardware_wallet.h>

HardwareWalletInterface hw;

// Enumerate devices
auto devices = hw.EnumerateDevices();

// Connect
hw.ConnectDevice(devices[0].device_id);

// Get address
DerivationPath path{44, 2210, 0, 0, 0};
std::string address = hw.GetAddress(path, true); // verify on device

// Sign transaction
HWSigningRequest request;
request.raw_transaction = /* ... */;
request.input_paths = {path};
request.verify_on_device = true;

auto result = hw.SignTransaction(request);
```

### Troubleshooting

**Device not detected:**
- Ensure udev rules installed (Linux)
- Check USB cable (data transfer capable)
- Try different USB port

**Transaction signing fails:**
- Verify device is unlocked
- Check firmware is up to date
- Ensure correct app installed (Ledger: INTcoin app)

---

## PDF Invoice Generation

**New in v1.3.0**

### Overview

Generate professional PDF invoices for payments using Qt's built-in PDF support. Perfect for merchants and freelancers accepting INTcoin.

### Features

- **Professional Layout**: Clean, business-ready invoice design
- **QR Code Integration**: Embedded payment QR codes
- **Multiple Items**: Support for line items with quantities and prices
- **Tax Calculation**: Automatic tax computation
- **Custom Branding**: Optional company logo
- **Payment URI**: Embedded `intcoin:` payment URI with amount

### Invoice Components

**Header:**
- Company logo (optional)
- "INVOICE" title
- Invoice number (auto-generated: `INV-YYYYMMDD-XXXX`)

**Metadata:**
- Invoice date
- Due date (optional)

**Parties:**
- From: Sender/merchant information (name, address, email, phone)
- Bill To: Customer/recipient information

**Line Items:**
- Description
- Quantity
- Unit price (in INT)
- Total (quantity × unit price)

**Totals:**
- Subtotal
- Tax (configurable percentage)
- Grand total

**Payment Information:**
- INTcoin payment address
- Embedded QR code for easy scanning
- Payment URI with amount

**Footer:**
- Notes (optional)
- Terms & conditions (optional)

### Usage Example

```cpp
#include <intcoin/qt/pdf_invoice.h>

PDFInvoiceGenerator generator;

// Create invoice data
InvoiceData data;
data.invoice_number = PDFInvoiceGenerator::GenerateInvoiceNumber();
data.timestamp = std::time(nullptr);
data.due_date = data.timestamp + (30 * 24 * 3600); // 30 days

// Sender
data.from_name = "Acme Corp";
data.from_address_line1 = "123 Main St";
data.from_address_line2 = "Springfield, IL 62701";
data.from_email = "billing@acmecorp.com";
data.from_phone = "+1 (555) 123-4567";

// Customer
data.to_name = "John Doe";
data.to_address_line1 = "456 Oak Ave";
data.to_email = "john@example.com";

// Payment
data.payment_address = "int1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080";
data.payment_uri = "intcoin:int1qw...?amount=150.5";

// Line items
data.items.push_back(InvoiceItem("Web Development Services", 40, 100.0));
data.items.push_back(InvoiceItem("Server Hosting (1 month)", 1, 50.0));
data.items.push_back(InvoiceItem("Domain Registration", 1, 15.0));

// Tax
data.tax_rate = 0.10; // 10%

// Calculate totals
data.CalculateTotals();

// Generate PDF
bool success = generator.Generate(data, "invoice_2026.pdf", true);
```

### Invoice Styles

Choose from 4 professional styles:
- **PROFESSIONAL**: Clean corporate design (default)
- **MODERN**: Contemporary colorful theme
- **MINIMAL**: Black & white minimalist
- **CLASSIC**: Traditional invoice layout

```cpp
generator.SetStyle(PDFInvoiceGenerator::Style::MODERN);
```

### Custom Branding

Add your company logo:
```cpp
generator.SetLogo("/path/to/logo.png");
```

Supported formats: PNG, JPG, SVG

---

## Integrated Mining

**New in v1.3.0**

### Overview

Mine INTcoin directly from the wallet using your CPU. Supports both solo mining and pool mining with real-time hashrate monitoring.

### Features

- **Solo Mining**: Mine directly to your wallet address
- **Pool Mining**: Connect to mining pools (Stratum protocol)
- **Multi-Threaded**: Utilize all CPU cores
- **Real-Time Stats**: Live hashrate, blocks found, uptime
- **Auto-Configuration**: Optimal thread count detection
- **Persistent Settings**: Configuration saved between sessions

### Mining Page Interface

**Status Section:**
- Mining status (Running/Stopped)
- Current hashrate (H/s, KH/s, MH/s, etc.)
- Blocks found
- Uptime
- Total hashes computed

**Configuration:**
- Thread count (1 to max CPU cores)
- Mining address (generate new or use existing)
- Pool mining toggle

**Pool Configuration:**
- Pool host (e.g., pool.intcoin.org)
- Pool port (default: 3333)
- Username (your address or pool username)
- Password (usually "x" or worker name)

**Controls:**
- Start Mining button
- Stop Mining button
- Auto-saves configuration

**Log:**
- Real-time mining events
- Block found notifications
- Share submissions (pool mining)
- Error messages

### Solo Mining

**Pros:**
- Keep entire block reward (105,113,636 INT)
- No pool fees
- Direct to wallet

**Cons:**
- Requires significant hashrate
- Irregular payouts
- May not find blocks for long periods

**Setup:**
1. Open **Mining** tab
2. Enter or generate mining address
3. Set thread count
4. Click **Start Mining**

### Pool Mining

**Pros:**
- Regular, predictable payouts
- Suitable for any hashrate
- Immediate earnings

**Cons:**
- Pool fees (typically 1-2%)
- Requires pool trust
- Network latency

**Setup:**
1. Enable **Pool Mining** checkbox
2. Enter pool details:
   - Host: `pool.intcoin.org`
   - Port: `3333`
   - Username: Your INTcoin address
   - Password: `x`
3. Click **Start Mining**

### Performance Optimization

**Thread Count:**
- **Light usage**: 50% of CPU cores
- **Moderate**: 75% of cores
- **Maximum**: 100% of cores

**System Impact:**
- Mining uses 100% of allocated cores
- Expect higher power consumption
- May cause system slowdown
- Laptop cooling considerations

### Expected Hashrate

| CPU | Cores | Expected Hashrate |
|-----|-------|-------------------|
| Intel i5 (10th gen) | 6 | ~2-3 KH/s |
| Intel i7 (10th gen) | 8 | ~3-5 KH/s |
| AMD Ryzen 5 5600X | 6 | ~4-6 KH/s |
| AMD Ryzen 9 5950X | 16 | ~12-15 KH/s |
| Apple M1 | 8 | ~5-7 KH/s |
| Apple M1 Pro | 10 | ~7-9 KH/s |

*Note: RandomX is optimized for CPUs and ASIC-resistant*

### Profitability

Calculate mining profitability:
- **Block reward**: 105,113,636 INT
- **Block time**: 2 minutes (avg)
- **Your hashrate**: X H/s
- **Network hashrate**: Y H/s
- **Your share**: (X / Y) × 100%

**Example:**
- Your hashrate: 5 KH/s (5,000 H/s)
- Network hashrate: 100 MH/s (100,000,000 H/s)
- Your share: 0.005%
- Expected earnings: ~262,784 INT per day (pool mining)

---

## Lightning Network

**Fully Implemented**

### Features

- **Instant Payments**: Sub-second transactions
- **Low Fees**: Minimal routing fees
- **Channel Management**: Open, close, rebalance channels
- **Invoice Generation**: Create payment requests with QR codes
- **Multi-Path Payments**: BOLT 14 support (coming soon)
- **Watchtowers**: Breach detection and remediation (coming soon)

### Usage

Navigate to **Lightning** tab for:
- Channel list with balances
- Payment sending
- Invoice creation
- Node information

---

## Security Features

### Encryption

- **Wallet Encryption**: AES-256-GCM
- **Post-Quantum**: Dilithium3 + Kyber768
- **Secure Memory**: Key wiping on deallocation

### Backup

- **Mnemonic Seed**: 24-word BIP39 seed phrase
- **Wallet File**: Encrypted wallet.dat backup
- **Channel Backups**: Lightning channel state (static)

### Best Practices

1. **Enable Wallet Encryption**: Settings → Encrypt Wallet
2. **Backup Mnemonic**: Write down 24 words, store securely
3. **Use Hardware Wallet**: For large amounts
4. **Verify Addresses**: Always check on device screen
5. **Test Small First**: Send small amount before large
6. **Update Software**: Keep wallet up to date

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Q` | Quit application |
| `Ctrl+O` | Open wallet |
| `Ctrl+N` | New address |
| `Ctrl+C` | Copy address |
| `Ctrl+S` | Send coins |
| `Ctrl+R` | Receive coins |
| `Ctrl+M` | Mining page |
| `Ctrl+L` | Lightning page |

---

## Troubleshooting

### Common Issues

**QR Code won't generate:**
- Ensure libqrencode is installed
- Check address format is valid
- Try regenerating address

**Hardware wallet not detected:**
- Install udev rules (Linux)
- Check USB permissions
- Update firmware

**Mining not starting:**
- Check mining address is valid
- Verify pool connection (pool mining)
- Check firewall settings

**Invoice PDF fails:**
- Ensure write permissions
- Check disk space
- Verify invoice data is complete

---

## Future Enhancements

**v1.4.0 (Planned):**
- [ ] Multi-signature wallets
- [ ] Coin control features
- [ ] Advanced fee estimation
- [ ] Transaction batch processing

**v1.5.0 (Planned):**
- [ ] Mobile wallet sync
- [ ] Watch-only wallets
- [ ] Tor integration
- [ ] I2P integration

---

## References

- [HD Wallet (BIP32/39/44)](https://github.com/bitcoin/bips)
- [libqrencode](https://fukuchi.org/works/qrencode/)
- [hidapi](https://github.com/libusb/hidapi)
- [Qt6 Documentation](https://doc.qt.io/qt-6/)
- [RandomX](https://github.com/tevador/RandomX)

---

**Last Updated**: January 9, 2026
**Maintainer**: Neil Adamson
**License**: MIT
