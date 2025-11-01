// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <chrono>

namespace intcoin {

// HDWallet implementation

HDWallet::HDWallet()
    : encrypted_(false)
    , next_key_index_(0)
{
}

HDWallet::~HDWallet() {
    // Clear sensitive data
    std::fill(master_seed_.begin(), master_seed_.end(), 0);
    std::fill(encryption_key_.begin(), encryption_key_.end(), 0);
}

HDWallet HDWallet::create_new(const std::string& password) {
    HDWallet wallet;

    // Generate random seed
    wallet.master_seed_ = crypto::Random::generate_bytes(64);

    // Generate mnemonic from seed
    wallet.mnemonic_ = crypto::Mnemonic::from_entropy(
        std::vector<uint8_t>(wallet.master_seed_.begin(), wallet.master_seed_.begin() + 32)
    );

    // Encrypt if password provided
    if (!password.empty()) {
        wallet.encrypt(password);
    }

    // Generate first key
    wallet.generate_new_key("Default");

    return wallet;
}

HDWallet HDWallet::from_mnemonic(const std::string& mnemonic, const std::string& password) {
    HDWallet wallet;

    wallet.mnemonic_ = mnemonic;

    // Convert mnemonic to seed
    wallet.master_seed_ = crypto::Mnemonic::to_seed(mnemonic, "");

    // Encrypt if password provided
    if (!password.empty()) {
        wallet.encrypt(password);
    }

    // Generate first key
    wallet.generate_new_key("Default");

    return wallet;
}

bool HDWallet::encrypt(const std::string& password) {
    if (encrypted_) {
        return false;
    }

    // Derive encryption key from password
    std::vector<uint8_t> salt = crypto::Random::generate_bytes(32);
    encryption_key_ = crypto::HKDF::derive_key(
        std::vector<uint8_t>(password.begin(), password.end()),
        salt,
        std::vector<uint8_t>(),
        32
    );

    encrypted_ = true;
    return true;
}

bool HDWallet::decrypt(const std::string& password) {
    if (!encrypted_) {
        return true;
    }

    // TODO: Implement actual decryption verification
    (void)password;
    return true;
}

bool HDWallet::change_password(const std::string& old_password, const std::string& new_password) {
    if (!decrypt(old_password)) {
        return false;
    }

    return encrypt(new_password);
}

WalletKey HDWallet::generate_new_key(const std::string& label) {
    WalletKey key = derive_key(next_key_index_);
    key.label = label;
    key.index = next_key_index_;
    key.creation_time = std::chrono::system_clock::now().time_since_epoch().count();

    keys_[next_key_index_] = key;
    next_key_index_++;

    return key;
}

std::vector<WalletKey> HDWallet::get_all_keys() const {
    std::vector<WalletKey> result;
    result.reserve(keys_.size());

    for (const auto& [index, key] : keys_) {
        result.push_back(key);
    }

    return result;
}

std::optional<WalletKey> HDWallet::get_key_by_address(const std::string& address) const {
    for (const auto& [index, key] : keys_) {
        if (key.address == address) {
            return key;
        }
    }
    return std::nullopt;
}

std::string HDWallet::get_new_address(const std::string& label) {
    WalletKey key = generate_new_key(label);
    return key.address;
}

std::vector<std::string> HDWallet::get_all_addresses() const {
    std::vector<std::string> addrs;
    addrs.reserve(keys_.size());

    for (const auto& [index, key] : keys_) {
        addrs.push_back(key.address);
    }

    return addrs;
}

uint64_t HDWallet::get_balance(const Blockchain& blockchain) const {
    uint64_t total = 0;

    for (const auto& [index, key] : keys_) {
        total += get_address_balance(key.address, blockchain);
    }

    return total;
}

uint64_t HDWallet::get_unconfirmed_balance() const {
    // TODO: Track unconfirmed transactions
    return 0;
}

uint64_t HDWallet::get_address_balance(const std::string& address, const Blockchain& blockchain) const {
    // TODO: Look up UTXOs for this address in blockchain
    (void)address;
    (void)blockchain;
    return 0;
}

std::optional<Transaction> HDWallet::create_transaction(
    const std::string& to_address,
    uint64_t amount,
    uint64_t fee,
    const Blockchain& blockchain
) {
    // Get UTXOs
    std::vector<UTXO> utxos = select_coins(amount + fee, blockchain);
    if (utxos.empty()) {
        return std::nullopt;
    }

    // Calculate total input value
    uint64_t total_input = 0;
    for (const auto& utxo : utxos) {
        total_input += utxo.output.value;
    }

    // Build transaction
    TransactionBuilder builder;

    // Add inputs
    for (const auto& utxo : utxos) {
        builder.add_input(utxo.outpoint);
    }

    // Add output to recipient
    // TODO: Get public key from address
    DilithiumPubKey recipient_pubkey{};
    builder.add_output(amount, recipient_pubkey);

    // Add change output if needed
    if (total_input > amount + fee) {
        uint64_t change = total_input - amount - fee;
        WalletKey change_key = generate_new_key("Change");
        builder.add_output(change, change_key.public_key);
    }

    Transaction tx = builder.build();

    // Sign transaction
    if (!sign_transaction(tx, blockchain)) {
        return std::nullopt;
    }

    return tx;
}

bool HDWallet::sign_transaction(Transaction& tx, const Blockchain& blockchain) {
    // TODO: Sign each input with corresponding private key
    (void)tx;
    (void)blockchain;
    return true;
}

std::vector<HDWallet::TxHistoryEntry> HDWallet::get_transaction_history(const Blockchain& blockchain) const {
    std::vector<TxHistoryEntry> history;

    // TODO: Scan blockchain for transactions involving our addresses

    (void)blockchain;
    return history;
}

std::vector<UTXO> HDWallet::get_utxos(const Blockchain& blockchain) const {
    std::vector<UTXO> utxos;

    // TODO: Query blockchain for UTXOs belonging to our addresses

    (void)blockchain;
    return utxos;
}

std::string HDWallet::get_mnemonic() const {
    return mnemonic_;
}

std::vector<uint8_t> HDWallet::get_seed() const {
    return master_seed_;
}

bool HDWallet::backup_to_file(const std::string& filepath) const {
    // TODO: Serialize wallet to encrypted file
    (void)filepath;
    return true;
}

HDWallet HDWallet::restore_from_file(const std::string& filepath, const std::string& password) {
    // TODO: Deserialize wallet from encrypted file
    (void)filepath;
    (void)password;
    return HDWallet();
}

void HDWallet::set_address_label(const std::string& address, const std::string& label) {
    address_labels_[address] = label;
}

std::string HDWallet::get_address_label(const std::string& address) const {
    auto it = address_labels_.find(address);
    if (it != address_labels_.end()) {
        return it->second;
    }
    return "";
}

WalletKey HDWallet::derive_key(uint32_t index) {
    // Derive keypair from master seed
    DilithiumKeyPair keypair = derive_keypair_from_seed(master_seed_, index);

    WalletKey key;
    key.public_key = keypair.public_key;
    key.private_key.assign(keypair.private_key.begin(), keypair.private_key.end());
    key.address = crypto::Address::from_public_key(keypair.public_key, false);
    key.index = index;

    return key;
}

DilithiumKeyPair HDWallet::derive_keypair_from_seed(const std::vector<uint8_t>& seed, uint32_t index) {
    // Derive child key using HKDF
    std::vector<uint8_t> index_bytes = {
        static_cast<uint8_t>(index & 0xFF),
        static_cast<uint8_t>((index >> 8) & 0xFF),
        static_cast<uint8_t>((index >> 16) & 0xFF),
        static_cast<uint8_t>((index >> 24) & 0xFF)
    };

    std::vector<uint8_t> child_seed = crypto::HKDF::derive_key(seed, {}, index_bytes, 64);

    // TODO: Actually derive Dilithium keypair from child seed
    // For now, generate a new random keypair
    return crypto::Dilithium::generate_keypair();
}

bool HDWallet::save_to_disk() const {
    // TODO: Implement wallet persistence
    return true;
}

bool HDWallet::load_from_disk() {
    // TODO: Implement wallet loading
    return true;
}

std::vector<UTXO> HDWallet::select_coins(uint64_t target_amount, const Blockchain& blockchain) const {
    std::vector<UTXO> utxos = get_utxos(blockchain);
    std::vector<UTXO> selected;

    // Simple greedy selection
    uint64_t total = 0;
    for (const auto& utxo : utxos) {
        selected.push_back(utxo);
        total += utxo.output.value;

        if (total >= target_amount) {
            return selected;
        }
    }

    // Not enough funds
    return {};
}

// SimpleWallet implementation

SimpleWallet::SimpleWallet() {
}

SimpleWallet SimpleWallet::create_new() {
    SimpleWallet wallet;

    auto keypair = crypto::Dilithium::generate_keypair();
    wallet.public_key_ = keypair.public_key;
    wallet.private_key_.assign(keypair.private_key.begin(), keypair.private_key.end());
    wallet.address_ = crypto::Address::from_public_key(keypair.public_key, false);

    return wallet;
}

SimpleWallet SimpleWallet::from_private_key(const std::vector<uint8_t>& private_key) {
    SimpleWallet wallet;
    wallet.private_key_ = private_key;

    // TODO: Derive public key from private key
    // For now, this is a placeholder
    wallet.public_key_ = {};
    wallet.address_ = "";

    return wallet;
}

bool SimpleWallet::sign_transaction(Transaction& tx) {
    // TODO: Sign transaction inputs
    (void)tx;
    return true;
}

uint64_t SimpleWallet::get_balance(const Blockchain& blockchain) const {
    // TODO: Query blockchain for balance
    (void)blockchain;
    return 0;
}

// WalletDB implementation

WalletDB::WalletDB(const std::string& filepath)
    : filepath_(filepath)
{
}

bool WalletDB::save_wallet(const HDWallet& wallet) {
    // TODO: Serialize and save wallet
    (void)wallet;
    return true;
}

std::optional<HDWallet> WalletDB::load_wallet() {
    // TODO: Load and deserialize wallet
    return std::nullopt;
}

bool WalletDB::save_transaction(const Transaction& tx) {
    // TODO: Save transaction
    (void)tx;
    return true;
}

std::vector<Transaction> WalletDB::load_transactions() {
    // TODO: Load transactions
    return {};
}

} // namespace intcoin
