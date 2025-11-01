// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// HD Wallet implementation with quantum-resistant cryptography.

#ifndef INTCOIN_WALLET_H
#define INTCOIN_WALLET_H

#include "primitives.h"
#include "transaction.h"
#include "blockchain.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace intcoin {

/**
 * Wallet key pair
 */
struct WalletKey {
    DilithiumPubKey public_key;
    std::vector<uint8_t> private_key;  // Encrypted in storage
    std::string address;
    std::string label;
    uint64_t creation_time;
    uint32_t index;  // HD derivation index

    WalletKey() : creation_time(0), index(0) {}
};

/**
 * HD Wallet (Hierarchical Deterministic)
 */
class HDWallet {
public:
    HDWallet();
    ~HDWallet();

    // Wallet creation
    static HDWallet create_new(const std::string& password);
    static HDWallet from_mnemonic(const std::string& mnemonic, const std::string& password);

    // Encryption
    bool encrypt(const std::string& password);
    bool decrypt(const std::string& password);
    bool change_password(const std::string& old_password, const std::string& new_password);
    bool is_encrypted() const { return encrypted_; }

    // Key management
    WalletKey generate_new_key(const std::string& label = "");
    std::vector<WalletKey> get_all_keys() const;
    std::optional<WalletKey> get_key_by_address(const std::string& address) const;

    // Address generation
    std::string get_new_address(const std::string& label = "");
    std::vector<std::string> get_all_addresses() const;

    // Balance
    uint64_t get_balance(const Blockchain& blockchain) const;
    uint64_t get_unconfirmed_balance() const;
    uint64_t get_address_balance(const std::string& address, const Blockchain& blockchain) const;

    // Transaction creation
    std::optional<Transaction> create_transaction(
        const std::string& to_address,
        uint64_t amount,
        uint64_t fee,
        const Blockchain& blockchain
    );

    // Transaction signing
    bool sign_transaction(Transaction& tx, const Blockchain& blockchain);

    // Transaction history
    struct TxHistoryEntry {
        Hash256 tx_hash;
        uint64_t amount;
        uint64_t fee;
        uint64_t timestamp;
        uint32_t confirmations;
        bool is_send;  // true = send, false = receive
        std::string address;
    };
    std::vector<TxHistoryEntry> get_transaction_history(const Blockchain& blockchain) const;

    // UTXO management
    std::vector<UTXO> get_utxos(const Blockchain& blockchain) const;

    // Backup & Recovery
    std::string get_mnemonic() const;
    std::vector<uint8_t> get_seed() const;
    bool backup_to_file(const std::string& filepath) const;
    static HDWallet restore_from_file(const std::string& filepath, const std::string& password);

    // Labels
    void set_address_label(const std::string& address, const std::string& label);
    std::string get_address_label(const std::string& address) const;

private:
    bool encrypted_;
    std::vector<uint8_t> master_seed_;
    std::string mnemonic_;
    std::map<uint32_t, WalletKey> keys_;  // index -> key
    uint32_t next_key_index_;

    // Address book
    std::map<std::string, std::string> address_labels_;

    // Encryption
    std::vector<uint8_t> encryption_key_;

    // Key derivation
    WalletKey derive_key(uint32_t index);
    DilithiumKeyPair derive_keypair_from_seed(const std::vector<uint8_t>& seed, uint32_t index);

    // Storage
    bool save_to_disk() const;
    bool load_from_disk();

    // Internal helpers
    std::vector<UTXO> select_coins(uint64_t target_amount, const Blockchain& blockchain) const;
};

/**
 * Simple wallet (non-HD, single key)
 */
class SimpleWallet {
public:
    SimpleWallet();

    // Create new wallet
    static SimpleWallet create_new();
    static SimpleWallet from_private_key(const std::vector<uint8_t>& private_key);

    // Key access
    DilithiumPubKey get_public_key() const { return public_key_; }
    std::string get_address() const { return address_; }

    // Transaction signing
    bool sign_transaction(Transaction& tx);

    // Balance
    uint64_t get_balance(const Blockchain& blockchain) const;

private:
    DilithiumPubKey public_key_;
    std::vector<uint8_t> private_key_;
    std::string address_;
};

/**
 * Wallet database
 */
class WalletDB {
public:
    explicit WalletDB(const std::string& filepath);

    bool save_wallet(const HDWallet& wallet);
    std::optional<HDWallet> load_wallet();

    bool save_transaction(const Transaction& tx);
    std::vector<Transaction> load_transactions();

private:
    std::string filepath_;
};

} // namespace intcoin

#endif // INTCOIN_WALLET_H
