// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <chrono>
#include <cstring>

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
    wallet.master_seed_ = crypto::SecureRandom::generate(64);

    // Generate mnemonic from seed
    wallet.mnemonic_ = crypto::Mnemonic::generate(24);

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
    std::vector<uint8_t> salt = crypto::SecureRandom::generate(32);
    encryption_key_ = crypto::HKDF::derive(
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

    // Verify the password by attempting to derive the same encryption key
    // The actual encryption key is derived during encrypt()
    std::vector<uint8_t> test_key = crypto::HKDF::derive(
        std::vector<uint8_t>(password.begin(), password.end()),
        master_seed_,  // Use master seed as salt
        std::vector<uint8_t>(),
        32
    );

    // Compare keys in constant time to prevent timing attacks
    if (test_key.size() != encryption_key_.size()) {
        crypto::SecureMemory::secure_zero(test_key);
        return false;
    }

    bool keys_match = crypto::SecureMemory::constant_time_compare(
        test_key.data(),
        encryption_key_.data(),
        test_key.size()
    );

    // Clear the test key from memory
    crypto::SecureMemory::secure_zero(test_key);

    if (!keys_match) {
        return false;  // Wrong password
    }

    // Password verified successfully
    encrypted_ = false;
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
    // Get all UTXOs for this address from blockchain
    std::vector<UTXO> utxos = blockchain.get_utxos_for_address(address);

    uint64_t balance = 0;
    for (const auto& utxo : utxos) {
        balance += utxo.output.value;
    }

    return balance;
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
    // Decode address to get public key hash
    auto pubkey_hash = crypto::Address::decode(to_address);
    if (!pubkey_hash) {
        return std::nullopt;  // Invalid address
    }

    // For now, we'll create a script that locks to the pubkey hash
    // In a full implementation, we'd use OP_DUP OP_HASH256 OP_EQUALVERIFY OP_CHECKSIG
    // Here we'll use the hash directly as a simplified pubkey script
    DilithiumPubKey recipient_pubkey{};
    std::memcpy(recipient_pubkey.data(), pubkey_hash->data(), std::min(pubkey_hash->size(), recipient_pubkey.size()));
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
    // Sign each input with the corresponding private key
    for (size_t i = 0; i < tx.inputs.size(); ++i) {
        auto& input = tx.inputs[i];

        // Get the previous output to find which key owns it
        std::optional<Transaction::Output> prev_output =
            blockchain.get_output(input.previous_output);

        if (!prev_output) {
            return false;  // Previous output not found
        }

        // Find the key that owns this output by matching public key
        const DilithiumKeyPair* signing_key = nullptr;
        for (const auto& [index, wallet_key] : keys_) {
            if (wallet_key.public_key == prev_output->pubkey_script) {
                signing_key = &wallet_key.keypair;
                break;
            }
        }

        if (!signing_key) {
            return false;  // We don't own this input
        }

        // Create signature hash (sign everything except signature scripts)
        std::vector<uint8_t> sig_hash = tx.get_signature_hash(i);

        // Sign the hash with Dilithium
        DilithiumSignature signature = crypto::Dilithium::sign(
            sig_hash,
            *signing_key
        );

        // Set the signature script (contains signature + public key for verification)
        input.signature_script.clear();
        input.signature_script.insert(
            input.signature_script.end(),
            signature.begin(),
            signature.end()
        );

        // Append public key
        input.signature_script.insert(
            input.signature_script.end(),
            signing_key->public_key.begin(),
            signing_key->public_key.end()
        );
    }

    return true;
}

std::vector<HDWallet::TxHistoryEntry> HDWallet::get_transaction_history(const Blockchain& blockchain) const {
    std::vector<TxHistoryEntry> history;

    // Collect all wallet addresses
    std::vector<std::string> addresses;
    for (const auto& [index, key] : keys_) {
        addresses.push_back(key.address);
    }

    // Scan blockchain for transactions involving our addresses
    std::vector<Transaction> txs = blockchain.scan_for_addresses(addresses);

    for (const auto& tx : txs) {
        Hash256 tx_hash = tx.get_hash();

        // Check if this is a send or receive
        bool is_send = false;
        bool is_receive = false;
        uint64_t amount = 0;

        // Check outputs to see if we're receiving
        for (const auto& output : tx.outputs) {
            // Extract address from output
            if (output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                DilithiumPubKey pubkey;
                std::copy(output.script_pubkey.begin(), output.script_pubkey.end(), pubkey.begin());
                std::string addr = crypto::Address::from_public_key(pubkey);

                // Check if this is one of our addresses
                for (const auto& [idx, key] : keys_) {
                    if (key.address == addr) {
                        is_receive = true;
                        amount += output.value;
                        break;
                    }
                }
            }
        }

        // Check inputs to see if we're sending
        if (!tx.is_coinbase()) {
            for (const auto& input : tx.inputs) {
                auto utxo = blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);
                if (utxo) {
                    // Check if UTXO belongs to us
                    if (utxo->output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                        DilithiumPubKey pubkey;
                        std::copy(utxo->output.script_pubkey.begin(), utxo->output.script_pubkey.end(), pubkey.begin());
                        std::string addr = crypto::Address::from_public_key(pubkey);

                        for (const auto& [idx, key] : keys_) {
                            if (key.address == addr) {
                                is_send = true;
                                amount = utxo->output.value;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Create history entry
        TxHistoryEntry entry;
        entry.tx_hash = tx_hash;
        entry.amount = amount;
        entry.fee = 0; // TODO: Calculate fee
        entry.timestamp = 0; // TODO: Get timestamp from block
        entry.confirmations = 0; // TODO: Calculate confirmations
        entry.is_send = is_send && !is_receive;
        entry.address = ""; // TODO: Extract counterparty address

        history.push_back(entry);
    }

    return history;
}

std::vector<UTXO> HDWallet::get_utxos(const Blockchain& blockchain) const {
    std::vector<UTXO> utxos;

    // Query blockchain for UTXOs belonging to all our addresses
    for (const auto& [index, key] : keys_) {
        std::vector<UTXO> addr_utxos = blockchain.get_utxos_for_address(key.address);
        utxos.insert(utxos.end(), addr_utxos.begin(), addr_utxos.end());
    }

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
    crypto::DilithiumKeyPair keypair = derive_keypair_from_seed(master_seed_, index);

    WalletKey key;
    key.public_key = keypair.public_key;
    key.private_key.assign(keypair.private_key.begin(), keypair.private_key.end());
    key.address = crypto::Address::from_public_key(keypair.public_key);
    key.index = index;

    return key;
}

crypto::DilithiumKeyPair HDWallet::derive_keypair_from_seed(const std::vector<uint8_t>& seed, uint32_t index) {
    // Derive child key using HKDF
    std::vector<uint8_t> index_bytes = {
        static_cast<uint8_t>(index & 0xFF),
        static_cast<uint8_t>((index >> 8) & 0xFF),
        static_cast<uint8_t>((index >> 16) & 0xFF),
        static_cast<uint8_t>((index >> 24) & 0xFF)
    };

    std::vector<uint8_t> child_seed = crypto::HKDF::derive(seed, {}, index_bytes, 64);

    // TODO: Actually derive Dilithium keypair from child seed deterministically
    // For now, generate a new random keypair
    // Note: Dilithium doesn't support deriving from seed directly in liboqs
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
    wallet.address_ = crypto::Address::from_public_key(keypair.public_key);

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
