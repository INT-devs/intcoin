# INTcoin Wallet System

**Author**: INTcoin Core Development Team
**Date**: December 2, 2025
**Status**: Implementation Complete (85%)

---

## Table of Contents

1. [Overview](#overview)
2. [HD Wallet Architecture](#hd-wallet-architecture)
3. [BIP39 Mnemonic Generation](#bip39-mnemonic-generation)
4. [BIP32/BIP44 Key Derivation](#bip32bip44-key-derivation)
5. [Wallet Operations](#wallet-operations)
6. [Address Management](#address-management)
7. [Transaction Management](#transaction-management)
8. [UTXO Management](#utxo-management)
9. [Database Storage](#database-storage)
10. [Encryption](#encryption)
11. [Backup and Recovery](#backup-and-recovery)
12. [API Reference](#api-reference)
13. [Security Considerations](#security-considerations)

---

## Overview

The INTcoin wallet implements a hierarchical deterministic (HD) wallet based on BIP32, BIP39, and BIP44 standards, adapted for post-quantum cryptography using CRYSTALS-Dilithium3 signatures.

### Key Features

- **HD Wallet**: Deterministic key derivation from a single seed
- **BIP39 Mnemonic**: 12/24-word recovery phrases
- **BIP44 Derivation**: Standard derivation paths (m/44'/2210'/0'/...)
- **Post-Quantum Keys**: Dilithium3 signatures (NIST Level 3)
- **RocksDB Storage**: Persistent, fast key-value storage
- **Encryption**: Optional wallet encryption with passphrase
- **Backup/Restore**: Full wallet backup and recovery

### Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Wallet Core                       │
│  - Create/Load/Close                                │
│  - Encrypt/Unlock/Lock                              │
│  - Address Generation                               │
│  - Transaction Creation & Signing                   │
└─────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
┌───────▼──────┐ ┌──────▼─────┐ ┌──────▼──────┐
│  HD Key      │ │  Wallet    │ │  UTXO       │
│  Derivation  │ │  Database  │ │  Manager    │
│              │ │            │ │             │
│ • BIP39      │ │ • Addresses│ │ • Track UTXOs│
│ • BIP32/44   │ │ • Txs      │ │ • Balances  │
│ • Dilithium3 │ │ • Metadata │ │ • Coin Select│
└──────────────┘ └────────────┘ └─────────────┘
```

---

## HD Wallet Architecture

### Hierarchical Deterministic (HD) Wallets

INTcoin uses HD wallets to generate a tree of key pairs from a single seed, providing:

1. **Single Backup**: One mnemonic phrase backs up all keys
2. **Organized Structure**: Logical key organization (accounts, change, etc.)
3. **Privacy**: Generate new addresses without exposing master key
4. **Deterministic**: Same seed always generates same keys

### Key Components

#### 1. Master Seed (64 bytes)
- Derived from BIP39 mnemonic phrase + passphrase
- Used to generate master extended key
- Never leaves the wallet

#### 2. Extended Keys
- **Private Extended Key (xprv)**: Contains private key + chain code
- **Public Extended Key (xpub)**: Contains public key + chain code (watch-only)

#### 3. Derivation Path (BIP44)
```
m / purpose' / coin_type' / account' / change / address_index

m/44'/2210'/0'/0/0  → First receiving address (account 0)
m/44'/2210'/0'/0/1  → Second receiving address
m/44'/2210'/0'/1/0  → First change address
```

**Hardened Derivation** (`'`):
- Uses private key in derivation
- Cannot derive child keys from public key alone
- Used for purpose, coin_type, and account levels

**Non-Hardened Derivation**:
- Uses public key in derivation
- Allows watch-only address generation
- Used for change and address_index levels

---

## BIP39 Mnemonic Generation

### Overview

BIP39 defines a standard for generating human-readable recovery phrases (mnemonics) from entropy.

### Mnemonic Generation Process

```cpp
// 1. Generate entropy (128-256 bits)
auto entropy = RandomGenerator::GetRandomBytes(16);  // 128 bits = 12 words

// 2. Calculate checksum
auto hash = SHA3::Hash(entropy);
size_t checksum_bits = entropy.size() * 8 / 32;  // First few bits of hash

// 3. Append checksum to entropy
// entropy || checksum = 132 bits (12 words)

// 4. Split into 11-bit groups (2048 word dictionary)
// 132 bits / 11 bits = 12 words

// 5. Map to mnemonic words
auto words = Mnemonic::Generate(12);  // Returns vector of 12 words
```

### Mnemonic to Seed Derivation

```cpp
// BIP39 seed derivation using PBKDF2-HMAC-SHA256
std::string mnemonic = "abandon ability able about above absent...";
std::string passphrase = "my_optional_passphrase";

// Salt = "mnemonic" + passphrase
std::string salt = "mnemonic" + passphrase;

// PBKDF2 (2048 rounds, 64-byte output)
auto seed = PBKDF2_HMAC_SHA256(mnemonic, salt, 2048, 64);
// seed is now a 512-bit (64-byte) master seed
```

### Supported Word Counts

| Entropy | Checksum | Total Bits | Mnemonic Length |
|---------|----------|------------|-----------------|
| 128 bits | 4 bits | 132 bits | 12 words |
| 160 bits | 5 bits | 165 bits | 15 words |
| 192 bits | 6 bits | 198 bits | 18 words |
| 224 bits | 7 bits | 231 bits | 21 words |
| 256 bits | 8 bits | 264 bits | 24 words |

**Recommended**: 12 words (128-bit entropy) or 24 words (256-bit entropy)

---

## BIP32/BIP44 Key Derivation

### BIP32 Master Key Generation

```cpp
// Generate master extended key from seed
auto seed = Mnemonic::ToSeed(words, passphrase);
auto master_result = HDKeyDerivation::GenerateMaster(seed);

ExtendedKey master = master_result.value.value();
// master.private_key = Dilithium3 secret key (2560 bytes)
// master.public_key = Dilithium3 public key (1312 bytes)
// master.chain_code = 32 bytes (for child derivation)
```

### Post-Quantum Adaptation

**Challenge**: Traditional BIP32 uses ECDSA where `pubkey = privkey * G` (elliptic curve math). Dilithium3 doesn't have this property - public keys cannot be derived from private keys alone.

**Solution**: Generate keypairs deterministically using HMAC output as seed:

```cpp
Result<ExtendedKey> HDKeyDerivation::DeriveChild(
    const ExtendedKey& parent,
    uint32_t index,
    bool hardened
) {
    // 1. Prepare derivation data
    std::vector<uint8_t> data;
    if (hardened) {
        data.push_back(0x00);
        data.insert(data.end(), parent.private_key.begin(), parent.private_key.end());
    } else {
        data.insert(data.end(), parent.public_key.begin(), parent.public_key.end());
    }

    // 2. Append child index
    uint32_t child_index = hardened ? (index | 0x80000000) : index;
    data.push_back((child_index >> 24) & 0xFF);
    data.push_back((child_index >> 16) & 0xFF);
    data.push_back((child_index >> 8) & 0xFF);
    data.push_back(child_index & 0xFF);

    // 3. Derive child using HMAC
    auto hmac = HMAC_SHA256(parent.chain_code, data);

    // 4. Generate deterministic keypair from HMAC
    auto keypair = DilithiumCrypto::GenerateDeterministicKeyPair(hmac);

    child.private_key = keypair.secret_key;
    child.public_key = keypair.public_key;
    std::copy_n(hmac.begin(), 32, child.chain_code.begin());

    return Result<ExtendedKey>::Ok(child);
}
```

### BIP44 Derivation Path

INTcoin follows BIP44 standard with **coin type 2210**:

```
m / 44' / 2210' / account' / change / address_index
```

**Levels**:
- **Purpose (44')**: BIP44 standard (hardened)
- **Coin Type (2210')**: INTcoin identifier (hardened)
- **Account (0'-2147483647')**: Separate accounts (hardened)
- **Change (0/1)**: 0=receive, 1=change (non-hardened)
- **Address Index (0-2147483647)**: Sequential addresses (non-hardened)

**Example Paths**:
```
m/44'/2210'/0'/0/0    → Account 0, first receiving address
m/44'/2210'/0'/0/1    → Account 0, second receiving address
m/44'/2210'/0'/1/0    → Account 0, first change address
m/44'/2210'/1'/0/0    → Account 1, first receiving address
```

---

## Wallet Operations

### Creating a New Wallet

```cpp
#include "intcoin/wallet.h"

// 1. Configure wallet
WalletConfig config;
config.data_dir = "/path/to/wallet";
config.coin_type = 2210;  // INTcoin
config.keypool_size = 100;  // Pre-generate 100 addresses

// 2. Generate mnemonic
auto mnemonic_result = Mnemonic::Generate(12);  // 12-word phrase
std::vector<std::string> words = mnemonic_result.value.value();

// Print mnemonic for user to back up
std::cout << "IMPORTANT: Write down these 12 words:\n";
for (size_t i = 0; i < words.size(); i++) {
    std::cout << (i + 1) << ". " << words[i] << "\n";
}

// 3. Create wallet
Wallet wallet(config);
std::string passphrase = "";  // Optional BIP39 passphrase
auto create_result = wallet.Create(words, passphrase);

if (create_result.IsOk()) {
    std::cout << "Wallet created successfully!\n";
} else {
    std::cerr << "Error: " << create_result.error << "\n";
}
```

### Loading an Existing Wallet

```cpp
WalletConfig config;
config.data_dir = "/path/to/wallet";

Wallet wallet(config);
auto load_result = wallet.Load();

if (load_result.IsOk()) {
    std::cout << "Wallet loaded successfully!\n";

    // Get wallet info
    auto info_result = wallet.GetInfo();
    if (info_result.IsOk()) {
        auto info = info_result.value.value();
        std::cout << "Balance: " << info.balance << " INTS\n";
        std::cout << "Addresses: " << info.address_count << "\n";
        std::cout << "Transactions: " << info.transaction_count << "\n";
    }
} else {
    std::cerr << "Error: " << load_result.error << "\n";
}
```

### Encrypting a Wallet

```cpp
// Encrypt wallet with passphrase
std::string passphrase = "my_secure_passphrase_2025";
auto encrypt_result = wallet.Encrypt(passphrase);

if (encrypt_result.IsOk()) {
    std::cout << "Wallet encrypted. It will be locked on next load.\n";
}

// Wallet is now encrypted and locked
assert(wallet.IsEncrypted());
assert(wallet.IsLocked());
```

### Unlocking an Encrypted Wallet

```cpp
// Unlock wallet for 300 seconds (5 minutes)
std::string passphrase = "my_secure_passphrase_2025";
uint32_t timeout_seconds = 300;

auto unlock_result = wallet.Unlock(passphrase, timeout_seconds);

if (unlock_result.IsOk()) {
    std::cout << "Wallet unlocked for 5 minutes\n";

    // Can now perform operations requiring private keys
    auto addr_result = wallet.GetNewAddress("Shopping");
    // ...
} else {
    std::cerr << "Incorrect passphrase\n";
}
```

---

## Address Management

### Generating New Addresses

```cpp
// Generate new receiving address
auto addr_result = wallet.GetNewAddress("Donation Address");

if (addr_result.IsOk()) {
    std::string address = addr_result.value.value();
    std::cout << "New address: " << address << "\n";
    // Example: int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh
}

// Generate change address (internal use)
auto change_result = wallet.GetNewChangeAddress();
std::string change_addr = change_result.value.value();
```

### Address Structure

```cpp
struct WalletAddress {
    std::string address;                    // Bech32 address (int1...)
    PublicKey public_key;                   // Dilithium3 public key (1312 bytes)
    DerivationPath path;                    // BIP44 path
    std::string label;                      // User-defined label
    uint64_t creation_time;                 // Unix timestamp
    uint64_t last_used_time;                // Last used in transaction
    bool is_change;                         // Internal change address?

    uint32_t GetIndex() const;              // Get address index from path
};
```

### Listing Addresses

```cpp
// Get all wallet addresses
auto addresses_result = wallet.GetAddresses();

if (addresses_result.IsOk()) {
    auto addresses = addresses_result.value.value();

    std::cout << "Wallet Addresses (" << addresses.size() << "):\n";
    for (const auto& addr : addresses) {
        std::cout << addr.address;
        if (!addr.label.empty()) {
            std::cout << " [" << addr.label << "]";
        }
        if (addr.is_change) {
            std::cout << " (change)";
        }
        std::cout << "\n";
    }
}
```

### Address Labels

```cpp
// Set label for address
std::string address = "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh";
wallet.SetAddressLabel(address, "Exchange Deposit");

// Get label
auto label_result = wallet.GetAddressLabel(address);
std::string label = label_result.value.value();  // "Exchange Deposit"
```

---

## Transaction Management

### Transaction Creation (TODO)

```cpp
// Create transaction
std::vector<Recipient> recipients = {
    {"int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh", 50000000},  // 50 INT
    {"int1q5v5c4d3wk4qx2xvjfg7h8j9k0l1m2n3p4q5r6s7t", 25000000}  // 25 INT
};

uint64_t fee_rate = 1000;  // INTS per byte
std::string comment = "Payment for services";

auto tx_result = wallet.CreateTransaction(recipients, fee_rate, comment);

if (tx_result.IsOk()) {
    Transaction unsigned_tx = tx_result.value.value();
    // Transaction created but not signed
}
```

### Transaction Signing (TODO)

```cpp
// Sign transaction with wallet's private keys
auto signed_result = wallet.SignTransaction(unsigned_tx);

if (signed_result.IsOk()) {
    Transaction signed_tx = signed_result.value.value();

    // Verify signatures
    for (const auto& input : signed_tx.inputs) {
        assert(input.signature.size() == DILITHIUM3_BYTES);  // 2420 bytes
    }
}
```

### Broadcasting Transaction

```cpp
// Send signed transaction to network
Blockchain& blockchain = GetBlockchain();  // From node

auto send_result = wallet.SendTransaction(signed_tx, blockchain);

if (send_result.IsOk()) {
    uint256 txid = send_result.value.value();
    std::cout << "Transaction broadcast: " << ToHex(txid) << "\n";
}
```

### Transaction History

```cpp
// Get all wallet transactions
auto txs_result = wallet.GetTransactions();

if (txs_result.IsOk()) {
    auto transactions = txs_result.value.value();

    std::cout << "Transaction History (" << transactions.size() << "):\n";
    for (const auto& wtx : transactions) {
        std::cout << "TXID: " << ToHex(wtx.txid) << "\n";
        std::cout << "Amount: " << wtx.amount << " INTS\n";
        std::cout << "Fee: " << wtx.fee << " INTS\n";
        std::cout << "Confirmations: " << (wtx.block_height == 0 ? 0 : blockchain.GetHeight() - wtx.block_height + 1) << "\n";
        if (!wtx.comment.empty()) {
            std::cout << "Comment: " << wtx.comment << "\n";
        }
        std::cout << "---\n";
    }
}
```

---

## UTXO Management

### UTXO Structure

```cpp
struct OutPoint {
    uint256 txid;        // Transaction ID
    uint32_t index;      // Output index
};

struct TxOut {
    uint64_t value;          // Amount in INTS
    PublicKey public_key;    // Recipient public key
    std::string address;     // Bech32 address
};
```

### Getting Wallet UTXOs (TODO)

```cpp
// Get all unspent transaction outputs
auto utxos_result = wallet.GetUTXOs();

if (utxos_result.IsOk()) {
    auto utxos = utxos_result.value.value();

    uint64_t total = 0;
    for (const auto& utxo : utxos) {
        total += utxo.value;
    }

    std::cout << "Total UTXOs: " << utxos.size() << "\n";
    std::cout << "Total Value: " << total << " INTS\n";
}
```

### Updating UTXO Set (TODO)

```cpp
// Scan blockchain for wallet UTXOs
Blockchain& blockchain = GetBlockchain();

auto update_result = wallet.UpdateUTXOs(blockchain);

if (update_result.IsOk()) {
    std::cout << "UTXO set updated\n";

    // Get updated balance
    auto balance_result = wallet.GetBalance();
    std::cout << "Balance: " << balance_result.value.value() << " INTS\n";
}
```

---

## Database Storage

### RocksDB Backend

INTcoin wallet uses RocksDB for persistent storage:

```
wallet.db/
├── addr_int1qxy...   → Serialized WalletAddress
├── addr_int1q5v...   → Serialized WalletAddress
├── tx_abc123...      → Serialized WalletTransaction
├── tx_def456...      → Serialized WalletTransaction
├── master_key        → Encrypted master seed
├── meta_version      → Wallet version
├── meta_coin_type    → Coin type (2210)
└── label_int1qxy...  → Address label
```

### Key Prefixes

| Prefix | Description | Value Type |
|--------|-------------|------------|
| `addr_` | Wallet addresses | WalletAddress |
| `tx_` | Transactions | WalletTransaction |
| `master_key` | Master seed | Encrypted bytes |
| `meta_` | Metadata | String |
| `label_` | Address labels | String |

### Database Operations

```cpp
// Write address
WalletDB db("/path/to/wallet.db");
db.Open();

WalletAddress addr;
addr.address = "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh";
// ... set other fields ...

db.WriteAddress(addr);

// Read address
auto addr_result = db.ReadAddress("int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh");
WalletAddress loaded = addr_result.value.value();

// Read all addresses
auto all_addrs = db.ReadAllAddresses();

db.Close();
```

---

## Encryption

### Wallet Encryption (TODO)

**Note**: Full encryption implementation is pending. Current design:

```cpp
// Encrypt wallet with Kyber768 or AES-256
std::string passphrase = "my_secure_passphrase_2025";

// Derive encryption key from passphrase
auto key = DeriveKeyFromPassphrase(passphrase);

// Encrypt master seed
auto encrypted_seed = EncryptData(master_seed, key);

// Store encrypted seed
db.WriteMasterKey(encrypted_seed);
```

### Unlock Mechanism

```cpp
// Unlock wallet temporarily
wallet.Unlock(passphrase, 300);  // 300 seconds

// Private keys accessible for 5 minutes
// Automatically locks after timeout

// Manual lock
wallet.Lock();
```

---

## Backup and Recovery

### Mnemonic Backup

**CRITICAL**: Users must back up their 12/24-word mnemonic phrase.

```cpp
// Export mnemonic (requires unlocked wallet)
auto mnemonic_result = wallet.GetMnemonic();

if (mnemonic_result.IsOk()) {
    auto words = mnemonic_result.value.value();

    std::cout << "BACKUP YOUR MNEMONIC PHRASE:\n";
    std::cout << "==============================\n";
    for (size_t i = 0; i < words.size(); i++) {
        std::cout << std::setw(2) << (i + 1) << ". " << words[i] << "\n";
    }
    std::cout << "==============================\n";
    std::cout << "Store in a safe place. Never share online.\n";
}
```

### Database Backup

```cpp
// Backup entire wallet database
std::string backup_path = "/path/to/backups/";
auto backup_result = wallet.BackupWallet(backup_path);

if (backup_result.IsOk()) {
    std::cout << "Wallet backed up to: " << backup_path << "\n";
}
```

### Wallet Recovery

```cpp
// Recover from mnemonic
std::vector<std::string> words = {
    "abandon", "ability", "able", "about", "above", "absent",
    "absorb", "abstract", "absurd", "abuse", "access", "accident"
};

WalletConfig config;
config.data_dir = "/path/to/new_wallet";

Wallet recovered_wallet(config);
auto create_result = recovered_wallet.Create(words, "");

if (create_result.IsOk()) {
    // Rescan blockchain to find transactions
    auto rescan_result = recovered_wallet.Rescan(blockchain, 0);

    std::cout << "Wallet recovered and rescanned\n";
}
```

---

## API Reference

### Wallet Class

```cpp
class Wallet {
public:
    Wallet(const WalletConfig& config);
    ~Wallet();

    // Lifecycle
    Result<void> Create(const std::vector<std::string>& mnemonic,
                       const std::string& passphrase = "");
    Result<void> Load();
    Result<void> Close();
    bool IsLoaded() const;

    // Encryption
    Result<void> Encrypt(const std::string& passphrase);
    Result<void> Unlock(const std::string& passphrase, uint32_t timeout_seconds);
    Result<void> Lock();
    bool IsEncrypted() const;
    bool IsLocked() const;
    Result<void> ChangePassphrase(const std::string& old_pass,
                                  const std::string& new_pass);

    // Address Management
    Result<std::string> GetNewAddress(const std::string& label = "");
    Result<std::string> GetNewChangeAddress();
    Result<std::vector<WalletAddress>> GetAddresses() const;
    Result<void> SetAddressLabel(const std::string& address,
                                  const std::string& label);
    Result<std::string> GetAddressLabel(const std::string& address) const;

    // Balance
    Result<uint64_t> GetBalance() const;
    Result<uint64_t> GetUnconfirmedBalance() const;
    Result<uint64_t> GetAddressBalance(const std::string& address) const;

    // Transactions
    Result<std::vector<WalletTransaction>> GetTransactions() const;
    Result<WalletTransaction> GetTransaction(const uint256& txid) const;
    Result<Transaction> CreateTransaction(const std::vector<Recipient>& recipients,
                                         uint64_t fee_rate,
                                         const std::string& comment = "");
    Result<Transaction> SignTransaction(const Transaction& tx);
    Result<uint256> SendTransaction(const Transaction& tx, Blockchain& blockchain);

    // UTXO Management
    Result<std::vector<TxOut>> GetUTXOs() const;
    Result<void> UpdateUTXOs(Blockchain& blockchain);

    // Backup/Recovery
    Result<std::vector<std::string>> GetMnemonic() const;
    Result<void> BackupWallet(const std::string& backup_path);
    Result<void> RestoreFromBackup(const std::string& backup_path);
    Result<void> Rescan(Blockchain& blockchain, uint64_t start_height = 0);

    // Info
    Result<WalletInfo> GetInfo() const;
};
```

### Mnemonic Class

```cpp
class Mnemonic {
public:
    // Generate new mnemonic (12, 15, 18, 21, or 24 words)
    static Result<std::vector<std::string>> Generate(size_t word_count = 12);

    // Convert mnemonic to seed
    static Result<std::vector<uint8_t>> ToSeed(
        const std::vector<std::string>& words,
        const std::string& passphrase = "");

    // Validate mnemonic
    static Result<void> Validate(const std::vector<std::string>& words);

    // Get BIP39 wordlist
    static const std::vector<std::string>& GetWordList();
};
```

### HDKeyDerivation Class

```cpp
class HDKeyDerivation {
public:
    // Generate master key from seed
    static Result<ExtendedKey> GenerateMaster(const std::vector<uint8_t>& seed);

    // Derive child key
    static Result<ExtendedKey> DeriveChild(const ExtendedKey& parent,
                                          uint32_t index,
                                          bool hardened);

    // Derive from full path
    static Result<ExtendedKey> DerivePath(const ExtendedKey& master,
                                         const DerivationPath& path);

    // Remove private key (create xpub)
    static Result<ExtendedKey> Neuter(const ExtendedKey& private_key);
};
```

---

## Security Considerations

### Mnemonic Security

1. **Write Down**: Users must write down mnemonic on paper
2. **Never Digital**: Don't store in files, screenshots, cloud, or emails
3. **Offline Storage**: Keep in safe, safety deposit box, or secure location
4. **No Photos**: Don't take photos of mnemonic
5. **Verify Backup**: Test recovery before depositing large amounts

### Passphrase Best Practices

1. **Strong Passphrases**: Minimum 16 characters, mix of letters/numbers/symbols
2. **Unique**: Don't reuse passphrases from other services
3. **Not Stored**: Never store passphrase with mnemonic
4. **BIP39 Passphrase**: Optional 25th word for additional security

### Operational Security

1. **Encrypted Wallets**: Always encrypt wallets with strong passphrase
2. **Auto-Lock**: Set short unlock timeout (5-10 minutes)
3. **Secure Backups**: Encrypt database backups
4. **Regular Backups**: Backup after generating new addresses
5. **Test Recovery**: Verify backup restoration works

### Post-Quantum Security

INTcoin wallet provides quantum-resistant security through:

1. **Dilithium3 Signatures**: NIST Level 3 post-quantum signatures
2. **SHA3-256**: Quantum-resistant hash function
3. **Future-Proof**: Protected against Shor's algorithm attacks

**Note**: Wallet encryption currently uses classical AES-256. Future versions will integrate Kyber768 for post-quantum key encapsulation.

---

## Implementation Status

### Completed ✅

- BIP39 mnemonic generation and validation
- BIP32/BIP44 key derivation adapted for Dilithium3
- Wallet database (RocksDB) with address/transaction storage
- Address generation (receive and change)
- Basic balance tracking
- Wallet encryption/unlock/lock infrastructure

### In Progress 🔄

- Transaction creation (UTXO selection, fee calculation)
- Transaction signing with Dilithium3
- UTXO management and blockchain scanning
- Full wallet encryption with Kyber768

### Planned 📋

- Watch-only wallet support (xpub only)
- Multi-signature wallets (m-of-n)
- Hardware wallet integration
- Coin control (manual UTXO selection)
- Replace-by-fee (RBF)

---

## References

- [BIP32: Hierarchical Deterministic Wallets](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)
- [BIP39: Mnemonic Code for Generating Deterministic Keys](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki)
- [BIP44: Multi-Account Hierarchy for Deterministic Wallets](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki)
- [NIST FIPS 204: ML-DSA (Dilithium)](https://csrc.nist.gov/publications/detail/fips/204/final)

---

**Last Updated**: December 2, 2025
**Version**: 1.0
