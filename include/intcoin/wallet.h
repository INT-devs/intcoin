/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Wallet - HD Wallet Implementation
 */

#ifndef INTCOIN_WALLET_H
#define INTCOIN_WALLET_H

#include "types.h"
#include "crypto.h"
#include "transaction.h"
#include "storage.h"
#include "blockchain.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>

namespace intcoin {

// Forward declarations
class Blockchain;

namespace wallet {

// ============================================================================
// BIP32 HD Wallet (Hierarchical Deterministic)
// ============================================================================

// BIP32 derivation path component
struct PathComponent {
    uint32_t index;
    bool hardened;

    PathComponent(uint32_t idx, bool hard = false)
        : index(idx), hardened(hard) {}

    // Get child index (with hardened flag if applicable)
    uint32_t GetChildIndex() const {
        return hardened ? (index | 0x80000000) : index;
    }
};

// BIP32 derivation path (e.g., m/44'/2210'/0'/0/0)
class DerivationPath {
public:
    DerivationPath() = default;
    explicit DerivationPath(const std::vector<PathComponent>& components);

    // Parse from string (e.g., "m/44'/2210'/0'/0/0")
    static Result<DerivationPath> Parse(const std::string& path_str);

    // Convert to string
    std::string ToString() const;

    // Get components
    const std::vector<PathComponent>& GetComponents() const { return components_; }

    // Append a component
    DerivationPath Append(uint32_t index, bool hardened = false) const;

private:
    std::vector<PathComponent> components_;
};

// BIP32 Extended Key (can be public or private)
struct ExtendedKey {
    uint8_t depth;                      // 0 for master, 1+ for derived
    uint32_t parent_fingerprint;        // First 4 bytes of parent's pubkey hash
    uint32_t child_index;               // Index of this child
    std::array<uint8_t, 32> chain_code; // For deriving child keys

    // Either private key or public key (not both)
    std::optional<SecretKey> private_key;
    std::optional<PublicKey> public_key;

    bool IsPrivate() const { return private_key.has_value(); }
    bool IsPublic() const { return public_key.has_value(); }

    // Serialize to base58check (xpub/xprv format)
    std::string SerializeBase58() const;

    // Deserialize from base58check
    static Result<ExtendedKey> DeserializeBase58(const std::string& str);
};

// BIP32 HD Key derivation
class HDKeyDerivation {
public:
    // Generate master key from seed
    static Result<ExtendedKey> GenerateMaster(const std::vector<uint8_t>& seed);

    // Derive child key from parent (single step)
    static Result<ExtendedKey> DeriveChild(const ExtendedKey& parent, uint32_t index, bool hardened = false);

    // Derive key from path (multiple steps)
    static Result<ExtendedKey> DerivePath(const ExtendedKey& master, const DerivationPath& path);

    // Get public key from private extended key
    static Result<ExtendedKey> Neuter(const ExtendedKey& private_key);
};

// ============================================================================
// BIP39 Mnemonic (Seed Phrase)
// ============================================================================

class Mnemonic {
public:
    // Generate new mnemonic (12, 15, 18, 21, or 24 words)
    static Result<std::vector<std::string>> Generate(size_t word_count = 24);

    // Convert mnemonic to seed
    static Result<std::vector<uint8_t>> ToSeed(
        const std::vector<std::string>& words,
        const std::string& passphrase = ""
    );

    // Validate mnemonic words
    static Result<void> Validate(const std::vector<std::string>& words);

    // Get word list (BIP39 English wordlist)
    static const std::vector<std::string>& GetWordList();
};

// ============================================================================
// Wallet Address
// ============================================================================

struct WalletAddress {
    std::string address;        // Bech32 address (int1...)
    PublicKey public_key;       // Dilithium3 public key
    DerivationPath path;        // BIP44 derivation path
    std::string label;          // User-defined label
    uint64_t creation_time;     // Unix timestamp
    uint64_t last_used_time;    // Last time used in transaction
    bool is_change;             // True if change address

    // Get address index (last component of path)
    uint32_t GetIndex() const;
};

// ============================================================================
// Coin Selection Strategy
// ============================================================================

/// Strategy for selecting UTXOs when creating transactions
enum class CoinSelectionStrategy {
    /// Select coins in order until target is reached (fast, simple)
    GREEDY,

    /// Select largest coins first (minimizes change, fewer inputs)
    LARGEST_FIRST,

    /// Select smallest coins first (reduces UTXO set, more inputs)
    SMALLEST_FIRST,

    /// Branch and bound algorithm (optimal selection, slower)
    BRANCH_AND_BOUND,

    /// Random selection (privacy-focused)
    RANDOM
};

// ============================================================================
// Wallet Transaction
// ============================================================================

struct WalletTransaction {
    uint256 txid;               // Transaction hash
    Transaction tx;             // Full transaction
    uint64_t block_height;      // 0 if unconfirmed
    uint256 block_hash;         // Zero if unconfirmed
    uint64_t timestamp;         // Time received/confirmed
    int64_t amount;             // Net amount (can be negative)
    uint64_t fee;               // Transaction fee
    std::string comment;        // User comment
    bool is_coinbase;           // True if coinbase transaction

    // Status
    bool IsConfirmed() const { return block_height > 0; }
    uint64_t GetConfirmations(uint64_t current_height) const {
        return IsConfirmed() ? (current_height - block_height + 1) : 0;
    }
};

// ============================================================================
// Wallet Configuration
// ============================================================================

struct WalletConfig {
    std::string data_dir = "~/.intcoin/wallet";  // Wallet data directory
    uint32_t keypool_size = 100;                  // Number of pre-generated keys
    uint32_t coin_type = 2210;                    // BIP44 coin type for INTcoin
    bool auto_backup = true;                      // Automatic backup on changes
    std::string backup_dir = "~/.intcoin/backups"; // Backup directory

    // Encryption
    bool encrypted = false;                       // Is wallet encrypted?
    uint32_t unlock_timeout = 600;                // Auto-lock after seconds (0 = never)
};

// ============================================================================
// Wallet Database Interface
// ============================================================================

class WalletDB {
public:
    explicit WalletDB(const std::string& wallet_path);
    ~WalletDB();

    // Open/Close
    Result<void> Open();
    Result<void> Close();
    bool IsOpen() const;

    // Keys and Addresses
    Result<void> WriteAddress(const WalletAddress& addr);
    Result<WalletAddress> ReadAddress(const std::string& address);
    Result<std::vector<WalletAddress>> ReadAllAddresses();
    Result<void> DeleteAddress(const std::string& address);

    // Transactions
    Result<void> WriteTransaction(const WalletTransaction& wtx);
    Result<WalletTransaction> ReadTransaction(const uint256& txid);
    Result<std::vector<WalletTransaction>> ReadAllTransactions();
    Result<void> DeleteTransaction(const uint256& txid);

    // Master key and metadata
    Result<void> WriteMasterKey(const std::vector<uint8_t>& encrypted_seed);
    Result<std::vector<uint8_t>> ReadMasterKey();
    Result<void> WriteMetadata(const std::string& key, const std::string& value);
    Result<std::string> ReadMetadata(const std::string& key);

    // Labels
    Result<void> WriteLabel(const std::string& address, const std::string& label);
    Result<std::string> ReadLabel(const std::string& address);

    // Backup
    Result<void> Backup(const std::string& backup_path);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Wallet Core
// ============================================================================

class Wallet {
public:
    explicit Wallet(const WalletConfig& config = WalletConfig());
    ~Wallet();

    // Lifecycle
    Result<void> Create(const std::vector<std::string>& mnemonic, const std::string& passphrase = "");
    Result<void> Load();
    Result<void> Close();
    bool IsLoaded() const;

    // Encryption
    Result<void> Encrypt(const std::string& passphrase);
    Result<void> Unlock(const std::string& passphrase, uint32_t timeout_seconds = 0);
    Result<void> Lock();
    bool IsEncrypted() const;
    bool IsLocked() const;
    Result<void> ChangePassphrase(const std::string& old_pass, const std::string& new_pass);

    // Address Management
    Result<std::string> GetNewAddress(const std::string& label = "");
    Result<std::string> GetNewChangeAddress();
    Result<std::vector<WalletAddress>> GetAddresses() const;
    Result<void> SetAddressLabel(const std::string& address, const std::string& label);
    Result<std::string> GetAddressLabel(const std::string& address) const;

    // Balance
    Result<uint64_t> GetBalance() const;
    Result<uint64_t> GetUnconfirmedBalance() const;
    Result<uint64_t> GetAddressBalance(const std::string& address) const;

    // Transactions
    Result<std::vector<WalletTransaction>> GetTransactions() const;
    Result<WalletTransaction> GetTransaction(const uint256& txid) const;

    // Transaction Creation
    struct Recipient {
        std::string address;
        uint64_t amount;
    };

    Result<Transaction> CreateTransaction(
        const std::vector<Recipient>& recipients,
        uint64_t fee_rate = 0,  // 0 = auto-estimate
        const std::string& comment = "",
        CoinSelectionStrategy strategy = CoinSelectionStrategy::GREEDY
    );

    Result<Transaction> SignTransaction(const Transaction& tx);
    Result<uint256> SendTransaction(const Transaction& tx, Blockchain& blockchain);

    // UTXO Management
    Result<std::vector<TxOut>> GetUTXOs() const;
    Result<void> UpdateUTXOs(Blockchain& blockchain);

    // Backup/Restore
    Result<std::vector<std::string>> GetMnemonic() const;
    Result<void> BackupWallet(const std::string& backup_path);
    Result<void> RestoreFromBackup(const std::string& backup_path);

    // Rescan blockchain for transactions
    Result<void> Rescan(Blockchain& blockchain, uint64_t start_height = 0);

    // Get wallet info
    struct WalletInfo {
        uint64_t balance;
        uint64_t unconfirmed_balance;
        size_t address_count;
        size_t transaction_count;
        size_t utxo_count;
        bool encrypted;
        bool locked;
        uint32_t keypool_size;
    };
    Result<WalletInfo> GetInfo() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Transaction Builder Helper
// ============================================================================

class WalletTransactionBuilder {
public:
    WalletTransactionBuilder(Wallet& wallet);

    // Add recipient
    WalletTransactionBuilder& AddRecipient(const std::string& address, uint64_t amount);

    // Set fee rate (per byte)
    WalletTransactionBuilder& SetFeeRate(uint64_t fee_rate);

    // Set comment
    WalletTransactionBuilder& SetComment(const std::string& comment);

    // Use specific UTXOs (coin control)
    WalletTransactionBuilder& UseUTXOs(const std::vector<OutPoint>& utxos);

    // Build unsigned transaction
    Result<Transaction> BuildUnsigned();

    // Build and sign transaction
    Result<Transaction> BuildAndSign();

private:
    Wallet& wallet_;
    std::vector<Wallet::Recipient> recipients_;
    uint64_t fee_rate_ = 0;
    std::string comment_;
    std::vector<OutPoint> utxos_;
};

} // namespace wallet
} // namespace intcoin

#endif // INTCOIN_WALLET_H
