# INTcoin Address Encoding (Bech32)

**Status**: ✅ Complete (100%)
**Tests**: 8/8 passing
**Last Updated**: November 26, 2025

INTcoin uses Bech32 encoding for addresses, providing superior error detection and human readability compared to traditional Base58Check addresses.

---

## Table of Contents

1. [Overview](#overview)
2. [Address Format](#address-format)
3. [Bech32 Encoding](#bech32-encoding)
4. [Implementation](#implementation)
5. [Error Detection](#error-detection)
6. [Usage Examples](#usage-examples)
7. [Test Coverage](#test-coverage)

---

## Overview

### Why Bech32?

Bech32 (BIP-173) is a superior address format offering:

| Feature | Bech32 | Base58Check |
|---------|--------|-------------|
| **Error Detection** | BCH code (detects all single errors) | Simple checksum |
| **Character Set** | No confusing characters (0, O, I, l) | Contains all alphanumeric |
| **QR Codes** | More efficient (alphanumeric mode) | Less efficient |
| **Copy/Paste** | Double-click selects entire address | Often breaks on symbols |
| **Case** | Case-insensitive | Case-sensitive |
| **Future Proof** | Version byte for upgrades | Limited extensibility |

### Address Examples

```
# Mainnet P2PKH (version 0)
int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx
└───┘└──────────────────── data + checksum ───────────────────────┘
 HRP

# Mainnet - Different address
int1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vycz5qm7xte8

# Testnet P2PKH (version 0)
tint1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9c9vae
└────┘└──────────────────── data + checksum ───────────────────────┘
  HRP

# Testnet - Different address
tint1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vyczsr5hxv2

# Lightning Invoice (for Lightning payments)
lint1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vyczswk9eaj
└────┘└──────────────────── data + checksum ───────────────────────┘
  HRP

# Length: 57-61 characters typically
```

---

## Address Format

### Structure

```
<HRP> + "1" + <version> + <pubkey_hash> + <checksum>
```

| Component | Description | Mainnet | Testnet | Lightning |
|-----------|-------------|---------|---------|-----------|
| **HRP** | Human-Readable Part | `int` | `tint` | `lint` |
| **Separator** | Always "1" | `1` | `1` | `1` |
| **Version** | Address type (0 = P2PKH) | `q` (= 0) | `q` (= 0) | `q` (= 0) |
| **Pubkey Hash** | SHA3-256(PublicKey), 32 bytes | 52 chars | 52 chars | 52 chars |
| **Checksum** | BCH code, 6 characters | 6 chars | 6 chars | 6 chars |

**Network Prefixes**:
- **Mainnet**: `int1` - for production transactions with real value
- **Testnet**: `tint1` - for testing purposes only (coins have no value)
- **Lightning**: `lint1` - for Lightning Network invoices and payments

### Character Set

Bech32 uses a 32-character alphabet (base32):

```
qpzry9x8gf2tvdw0s3jn54khce6mua7l
```

**Excluded characters**: `1`, `b`, `i`, `o` (to avoid confusion)

### Version Byte

| Version | Type | Status |
|---------|------|--------|
| 0 | P2PKH (Pay to Public Key Hash) | ✅ Implemented |
| 1-15 | Reserved for future use | 🔄 Planned |

---

## Bech32 Encoding

### Algorithm Overview

```
1. Prepare data:
   - Combine version byte + pubkey hash (33 bytes)

2. Convert to base32:
   - Convert 8-bit bytes to 5-bit values
   - Results in ~53 base32 digits

3. Create checksum:
   - Expand HRP ("int" for mainnet, "tint" for testnet, "lint" for Lightning)
   - Combine with data
   - Calculate BCH polymod
   - Generate 6-character checksum

4. Encode:
   - HRP + "1" + base32(data) + base32(checksum)
   - Mainnet: "int" + "1" + data + checksum
   - Testnet: "tint" + "1" + data + checksum
   - Lightning: "lint" + "1" + data + checksum
```

### Step-by-Step Example

```cpp
// Input: 32-byte pubkey hash (all zeros for simplicity)
uint256 pubkey_hash = {0x00, 0x00, ..., 0x00};

// Step 1: Add version byte
data = [0x00] + pubkey_hash  // 33 bytes

// Step 2: Convert to 5-bit groups
// 33 bytes * 8 bits = 264 bits
// 264 bits / 5 bits = 52.8 → 53 base32 digits (+ padding)
data_5bit = ConvertBits(data, from=8, to=5, pad=true)
// Result: [0, 0, 0, 0, ..., 0] (53 digits)

// Step 3: Calculate checksum (Mainnet)
checksum_mainnet = CreateChecksum("int", data_5bit)
// Result: [6, 24, 2, 13, 27] (example)

// Step 3b: Calculate checksum (Testnet)
checksum_testnet = CreateChecksum("tint", data_5bit)
// Result: [9, 3, 9, 31, 28, 14] (example - different from mainnet!)

// Step 3c: Calculate checksum (Lightning)
checksum_lightning = CreateChecksum("lint", data_5bit)
// Result: [24, 22, 30, 9, 14, 28] (example - different from others!)

// Step 4: Encode to Bech32 (Mainnet)
result_mainnet = "int" + "1"
for digit in (data_5bit + checksum_mainnet):
    result_mainnet += CHARSET[digit]
// Final: int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx

// Step 4b: Encode to Bech32 (Testnet)
result_testnet = "tint" + "1"
for digit in (data_5bit + checksum_testnet):
    result_testnet += CHARSET[digit]
// Final: tint1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9c9vae

// Step 4c: Encode to Bech32 (Lightning)
result_lightning = "lint" + "1"
for digit in (data_5bit + checksum_lightning):
    result_lightning += CHARSET[digit]
// Final: lint1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqdj2mnl
```

### BCH Checksum

The checksum uses a BCH code with these generators:

```cpp
const uint32_t BECH32_GENERATOR[5] = {
    0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3
};

function Bech32Polymod(values):
    chk = 1
    for value in values:
        top = chk >> 25
        chk = (chk & 0x1ffffff) << 5 ^ value
        for i in 0..4:
            if (top >> i) & 1:
                chk ^= BECH32_GENERATOR[i]
    return chk

function CreateChecksum(hrp, data):
    values = ExpandHRP(hrp) + data + [0,0,0,0,0,0]
    polymod = Bech32Polymod(values) ^ 1
    checksum = []
    for i in 0..5:
        checksum[i] = (polymod >> (5 * (5 - i))) & 31
    return checksum
```

---

## Implementation

### Source Files

```
src/crypto/crypto.cpp       # Bech32 implementation
include/intcoin/crypto.h    # API definitions
tests/test_bech32.cpp       # Test suite
```

### API Reference

```cpp
#include "intcoin/crypto.h"

// Encode pubkey hash to Bech32 address
Result<std::string> AddressEncoder::EncodeAddress(const uint256& pubkey_hash);

// Decode Bech32 address to pubkey hash
Result<uint256> AddressEncoder::DecodeAddress(const std::string& address);

// Validate address format and checksum
bool AddressEncoder::ValidateAddress(const std::string& address);

// Get address version byte
std::optional<uint8_t> GetAddressVersion(const std::string& address);
```

### Helper Functions

```cpp
// Convert public key to hash
uint256 PublicKeyToHash(const PublicKey& pubkey);

// Convert pubkey hash to address
std::string PublicKeyHashToAddress(const uint256& pubkey_hash);

// Direct conversion: pubkey → address
std::string PublicKeyToAddress(const PublicKey& pubkey);
```

### Internal Functions

```cpp
namespace {
    // Find character index in Bech32 charset
    int CharsetIndex(char c);

    // Calculate BCH checksum
    uint32_t Bech32Polymod(const std::vector<uint8_t>& values);

    // Expand HRP for checksum
    std::vector<uint8_t> ExpandHRP(const std::string& hrp);

    // Create 6-character checksum
    std::vector<uint8_t> CreateChecksum(
        const std::string& hrp,
        const std::vector<uint8_t>& data
    );

    // Verify checksum
    bool VerifyChecksum(
        const std::string& hrp,
        const std::vector<uint8_t>& data
    );

    // Convert between 8-bit and 5-bit encoding
    std::vector<uint8_t> ConvertBits(
        const std::vector<uint8_t>& data,
        int frombits,
        int tobits,
        bool pad
    );
}
```

---

## Error Detection

### Checksum Properties

The Bech32 checksum provides:

1. **All single-character errors detected**
   - Test: 59/59 single-char errors detected ✅

2. **Almost all multi-character errors detected**
   - Probability of undetected error: ~1 in 10⁹

3. **Swap errors detected**
   - Transposing two characters is always detected

4. **Insertion/deletion errors detected**
   - Adding or removing characters is detected

### Validation Examples

```cpp
// Valid mainnet address
std::string valid_mainnet = "int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx";
assert(AddressEncoder::ValidateAddress(valid_mainnet) == true);

// Valid testnet address
std::string valid_testnet = "tint1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9c9vae";
assert(AddressEncoder::ValidateAddress(valid_testnet) == true);

// Valid Lightning address
std::string valid_lightning = "lint1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqdj2mnl";
assert(AddressEncoder::ValidateAddress(valid_lightning) == true);

// Invalid: wrong HRP (Bitcoin address)
std::string wrong_hrp = "bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx";
assert(AddressEncoder::ValidateAddress(wrong_hrp) == false);

// Invalid: corrupted checksum (changed last character)
std::string corrupted = "int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdz";
assert(AddressEncoder::ValidateAddress(corrupted) == false);

// Invalid: contains excluded character 'b'
std::string bad_char = "int1bqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx";
assert(AddressEncoder::ValidateAddress(bad_char) == false);

// Invalid: mixed case
std::string mixed = "int1QQQQQQQQQQQQQQQQqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx";
assert(AddressEncoder::ValidateAddress(mixed) == false);
```

---

## Usage Examples

### 1. Generate Address from Public Key

```cpp
#include "intcoin/crypto.h"

// Generate Dilithium keypair
auto keypair_result = DilithiumCrypto::GenerateKeyPair();
PublicKey pubkey = keypair_result.value->public_key;

// Method 1: Direct conversion
std::string address = PublicKeyToAddress(pubkey);
// Result: int1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vycz5qm7xte8

// Method 2: Step-by-step
uint256 pubkey_hash = PublicKeyToHash(pubkey);
std::string address2 = PublicKeyHashToAddress(pubkey_hash);

// Method 3: Using Result<T> for error handling
auto addr_result = AddressEncoder::EncodeAddress(pubkey_hash);
if (addr_result.IsOk()) {
    std::string address3 = *addr_result.value;
}
```

### 2. Validate Address

```cpp
std::string user_input = "int1qrzw0r5k56aghxrht9n7ewk9nekzhemg39pm96v84tp8pm7t0vycz5qm7xte8";

if (AddressEncoder::ValidateAddress(user_input)) {
    std::cout << "Address is valid!" << std::endl;

    // Decode to get pubkey hash
    auto decode_result = AddressEncoder::DecodeAddress(user_input);
    if (decode_result.IsOk()) {
        uint256 pubkey_hash = *decode_result.value;
        // Use pubkey_hash for payment
    }
} else {
    std::cout << "Invalid address!" << std::endl;
}
```

### 3. Handle Different Networks

```cpp
std::string mainnet_addr = "int1qrzw...";  // Mainnet address
std::string testnet_addr = "tint1qrzw..."; // Testnet address
std::string lightning_addr = "lint1qrzw..."; // Lightning address

// Detect network from address
if (mainnet_addr.substr(0, 4) == "int1") {
    std::cout << "Mainnet address" << std::endl;
} else if (mainnet_addr.substr(0, 5) == "tint1") {
    std::cout << "Testnet address" << std::endl;
} else if (mainnet_addr.substr(0, 5) == "lint1") {
    std::cout << "Lightning address" << std::endl;
}
```

### 4. Case Insensitivity

```cpp
std::string lowercase = "int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq6sqhdx";
std::string uppercase = "INT1QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ6SQHDX";

// Both decode to the same hash
auto hash1 = AddressEncoder::DecodeAddress(lowercase);
auto hash2 = AddressEncoder::DecodeAddress(uppercase);
assert(*hash1.value == *hash2.value);

// But mixed case is invalid
std::string mixed = "int1QQQQqqqq...";
assert(AddressEncoder::ValidateAddress(mixed) == false);
```

---

## Test Coverage

### Test Suite (tests/test_bech32.cpp)

**Status**: ✅ 8/8 tests passing

1. **Encode/Decode Round-Trip**
   - Encodes pubkey hash to address
   - Decodes address back to hash
   - Verifies hash matches original

2. **Different Hashes → Different Addresses**
   - Multiple hashes produce unique addresses
   - No collisions

3. **Address Validation**
   - Valid addresses pass
   - Invalid addresses fail (wrong HRP, checksum, etc.)

4. **Case Insensitivity**
   - Uppercase and lowercase decode identically
   - Mixed case is rejected

5. **Checksum Error Detection**
   - Single-character errors: 59/59 detected
   - Corrupted checksums rejected

6. **Public Key to Address**
   - Generates Dilithium keypair
   - Converts pubkey → hash → address
   - Round-trip validation

7. **Edge Cases**
   - All-zeros hash
   - All-ones hash
   - Different encodings

8. **Invalid Input Handling**
   - Empty string
   - Wrong HRP
   - Invalid characters (0, 1, b, i, o)
   - Too short/long addresses

### Running Tests

```bash
cd build
./tests/test_bech32

# Expected output:
# ========================================
# Bech32 Address Encoding Tests
# ========================================
# ✓ Test 1: Encode/Decode Round-Trip
# ✓ Test 2: Different Hashes Produce Different Addresses
# ✓ Test 3: Address Validation
# ✓ Test 4: Case Insensitivity
# ✓ Test 5: Checksum Error Detection
# ✓ Test 6: Public Key to Address Conversion
# ✓ Test 7: Edge Cases
# ✓ Test 8: Invalid Input Handling
# ========================================
# ✓ All Bech32 tests passed!
# ========================================
```

---

## Comparison with Other Formats

### Bech32 vs Base58Check

| Feature | Bech32 (INTcoin) | Base58Check (Bitcoin Legacy) |
|---------|------------------|------------------------------|
| **Example** | `int1qrzw0r5k...` | `1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa` |
| **Charset** | 32 chars (no 0, O, I, l, 1, b, i, o) | 58 chars (all alphanumeric) |
| **Case** | Case-insensitive | Case-sensitive |
| **Error Detection** | All single errors | ~29% single errors |
| **QR Efficiency** | Better (alphanumeric) | Worse (mixed case) |
| **Length** | 57-61 chars | 26-35 chars |
| **Readability** | High (no confusing chars) | Medium (0/O, l/1 confusion) |
| **Networks** | int1/tint1/lint1 | Different version bytes |

### Migration from Legacy Formats

INTcoin **does not support** legacy address formats (Base58Check). This is intentional to:

1. Simplify implementation
2. Avoid user confusion
3. Leverage Bech32 benefits from day one
4. Match modern cryptocurrency standards

---

## References

### Specifications

- [BIP-173: Bech32 Address Format](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki)
- [Bech32 Error Detection](https://github.com/sipa/bech32/tree/master/ref)

### Implementation

- [Bitcoin Core Bech32 Implementation](https://github.com/bitcoin/bitcoin/blob/master/src/bech32.cpp)
- [Rust Bitcoin Bech32](https://github.com/rust-bitcoin/rust-bech32)

### Papers

- [BCH Codes](https://en.wikipedia.org/wiki/BCH_code)
- [Error Detection in Bech32](https://gist.github.com/sipa/14c248c288c3880a3b191f978a34508e)

---

**Next**: [Transactions](TRANSACTIONS.md)
