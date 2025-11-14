// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>

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

uint64_t HDWallet::get_unconfirmed_balance(const Mempool& mempool, const Blockchain& blockchain) const {
    // Get all wallet public keys for efficient filtering
    std::set<DilithiumPubKey> wallet_pubkeys = get_all_pubkeys();

    uint64_t unconfirmed_received = 0;
    uint64_t unconfirmed_spent = 0;

    // Get all transactions from mempool
    std::vector<Transaction> mempool_txs = mempool.get_all_transactions();

    for (const auto& tx : mempool_txs) {

        // Check outputs - are we receiving unconfirmed coins?
        for (const auto& output : tx.outputs) {
            if (output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                DilithiumPubKey pubkey;
                std::copy(output.script_pubkey.begin(), output.script_pubkey.end(), pubkey.begin());
                if (wallet_pubkeys.count(pubkey)) {
                    unconfirmed_received += output.value;
                }
            }
        }

        // Check inputs - are we spending coins?
        if (!tx.is_coinbase()) {
            for (const auto& input : tx.inputs) {
                auto utxo = blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);
                if (utxo && utxo->output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                    DilithiumPubKey pubkey;
                    std::copy(utxo->output.script_pubkey.begin(), utxo->output.script_pubkey.end(), pubkey.begin());
                    if (wallet_pubkeys.count(pubkey)) {
                        unconfirmed_spent += utxo->output.value;
                    }
                }
            }
        }
    }

    // Net unconfirmed balance = received - spent
    // This can be negative if we've spent more than received in unconfirmed transactions
    if (unconfirmed_received > unconfirmed_spent) {
        return unconfirmed_received - unconfirmed_spent;
    }
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
        std::optional<UTXO> utxo =
            blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);

        if (!utxo) {
            return false;  // Previous output not found
        }

        // Find the key that owns this output by matching public key
        const WalletKey* signing_key_info = nullptr;
        for (const auto& [index, wallet_key] : keys_) {
            if (wallet_key.public_key == utxo->output.pubkey) {
                signing_key_info = &wallet_key;
                break;
            }
        }

        if (!signing_key_info) {
            return false;  // We don't own this input
        }

        // Create keypair for signing
        crypto::DilithiumKeyPair signing_keypair;
        signing_keypair.public_key = signing_key_info->public_key;
        signing_keypair.private_key.fill(0);  // Initialize
        if (signing_key_info->private_key.size() == 4896) {
            std::copy(signing_key_info->private_key.begin(),
                     signing_key_info->private_key.end(),
                     signing_keypair.private_key.begin());
        } else {
            return false;  // Invalid private key size
        }

        // Create signature hash
        auto serialized = tx.serialize();
        auto sig_hash = crypto::SHA3_256::hash(serialized.data(), serialized.size());
        std::vector<uint8_t> sig_hash_vec(sig_hash.begin(), sig_hash.end());

        // Sign the hash with Dilithium
        DilithiumSignature signature = crypto::Dilithium::sign(
            sig_hash_vec,
            signing_keypair
        );

        // Set the signature in TxInput
        input.signature = signature;

        // Set the signature script (contains signature + public key for verification)
        input.script_sig.clear();
        input.script_sig.insert(
            input.script_sig.end(),
            signature.begin(),
            signature.end()
        );

        // Append public key
        input.script_sig.insert(
            input.script_sig.end(),
            signing_key_info->public_key.begin(),
            signing_key_info->public_key.end()
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

        // Calculate fee for sent transactions
        if (is_send) {
            uint64_t total_input = 0;
            uint64_t total_output = 0;

            for (const auto& input : tx.inputs) {
                auto utxo = blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);
                if (utxo) {
                    total_input += utxo->output.value;
                }
            }

            for (const auto& output : tx.outputs) {
                total_output += output.value;
            }

            entry.fee = (total_input > total_output) ? (total_input - total_output) : 0;
        } else {
            entry.fee = 0;
        }

        // Get timestamp from block containing this transaction
        uint32_t block_height = blockchain.get_transaction_block_height(tx_hash);
        if (block_height > 0) {
            Block block = blockchain.get_block_by_height(block_height);
            entry.timestamp = block.header.timestamp;

            // Calculate confirmations
            uint32_t current_height = blockchain.get_height();
            entry.confirmations = (current_height >= block_height) ?
                (current_height - block_height + 1) : 0;
        } else {
            // Transaction in mempool (unconfirmed)
            entry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            entry.confirmations = 0;
        }

        entry.is_send = is_send && !is_receive;

        // Extract counterparty address
        if (is_send) {
            // For send transactions, get the first output that's not to us
            for (const auto& output : tx.outputs) {
                if (output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                    DilithiumPubKey pubkey;
                    std::copy(output.script_pubkey.begin(), output.script_pubkey.end(), pubkey.begin());
                    std::string addr = crypto::Address::from_public_key(pubkey);

                    // Check if this is not our address
                    bool is_ours = false;
                    for (const auto& [idx, key] : keys_) {
                        if (key.address == addr) {
                            is_ours = true;
                            break;
                        }
                    }

                    if (!is_ours) {
                        entry.address = addr;
                        break;
                    }
                }
            }
        } else if (is_receive) {
            // For receive transactions, get the first input address
            if (!tx.inputs.empty()) {
                auto utxo = blockchain.get_utxo(tx.inputs[0].previous_output.tx_hash, tx.inputs[0].previous_output.index);
                if (utxo && utxo->output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                    DilithiumPubKey pubkey;
                    std::copy(utxo->output.script_pubkey.begin(), utxo->output.script_pubkey.end(), pubkey.begin());
                    entry.address = crypto::Address::from_public_key(pubkey);
                }
            }
        }

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
    // Serialize wallet data
    std::vector<uint8_t> wallet_data;

    // Serialize master seed (64 bytes)
    wallet_data.insert(wallet_data.end(), master_seed_.begin(), master_seed_.end());

    // Serialize mnemonic length and data
    uint32_t mnemonic_len = static_cast<uint32_t>(mnemonic_.size());
    wallet_data.insert(wallet_data.end(),
        reinterpret_cast<const uint8_t*>(&mnemonic_len),
        reinterpret_cast<const uint8_t*>(&mnemonic_len) + sizeof(mnemonic_len));
    wallet_data.insert(wallet_data.end(), mnemonic_.begin(), mnemonic_.end());

    // Serialize key count and next index
    uint32_t key_count = static_cast<uint32_t>(keys_.size());
    wallet_data.insert(wallet_data.end(),
        reinterpret_cast<const uint8_t*>(&key_count),
        reinterpret_cast<const uint8_t*>(&key_count) + sizeof(key_count));
    wallet_data.insert(wallet_data.end(),
        reinterpret_cast<const uint8_t*>(&next_key_index_),
        reinterpret_cast<const uint8_t*>(&next_key_index_) + sizeof(next_key_index_));

    // Serialize address labels
    uint32_t label_count = static_cast<uint32_t>(address_labels_.size());
    wallet_data.insert(wallet_data.end(),
        reinterpret_cast<const uint8_t*>(&label_count),
        reinterpret_cast<const uint8_t*>(&label_count) + sizeof(label_count));

    for (const auto& [address, label] : address_labels_) {
        uint32_t addr_len = static_cast<uint32_t>(address.size());
        wallet_data.insert(wallet_data.end(),
            reinterpret_cast<const uint8_t*>(&addr_len),
            reinterpret_cast<const uint8_t*>(&addr_len) + sizeof(addr_len));
        wallet_data.insert(wallet_data.end(), address.begin(), address.end());

        uint32_t label_len = static_cast<uint32_t>(label.size());
        wallet_data.insert(wallet_data.end(),
            reinterpret_cast<const uint8_t*>(&label_len),
            reinterpret_cast<const uint8_t*>(&label_len) + sizeof(label_len));
        wallet_data.insert(wallet_data.end(), label.begin(), label.end());
    }

    // Encrypt wallet data if encrypted
    std::vector<uint8_t> final_data;
    if (encrypted_ && !encryption_key_.empty()) {
        // Use encryption key to encrypt the wallet data
        // Note: In production, should use AES256_GCM with the encryption key
        // For now, we'll store encrypted flag and append encryption key
        uint8_t enc_flag = 1;
        final_data.push_back(enc_flag);
        final_data.insert(final_data.end(), encryption_key_.begin(), encryption_key_.end());
        final_data.insert(final_data.end(), wallet_data.begin(), wallet_data.end());
    } else {
        uint8_t enc_flag = 0;
        final_data.push_back(enc_flag);
        final_data.insert(final_data.end(), wallet_data.begin(), wallet_data.end());
    }

    // Write to file
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(final_data.data()), final_data.size());
    file.close();

    return true;
}

HDWallet HDWallet::restore_from_file(const std::string& filepath, const std::string& password) {
    // Read file
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return HDWallet();  // Return empty wallet on failure
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();

    if (file_data.empty()) {
        return HDWallet();
    }

    // Check encryption flag
    uint8_t enc_flag = file_data[0];
    size_t offset = 1;

    std::vector<uint8_t> wallet_data;

    if (enc_flag == 1) {
        // Wallet is encrypted
        // Extract encryption key (32 bytes)
        if (file_data.size() < 33) {
            return HDWallet();
        }

        std::vector<uint8_t> stored_enc_key(
            file_data.begin() + offset,
            file_data.begin() + offset + 32
        );
        offset += 32;

        wallet_data.assign(file_data.begin() + offset, file_data.end());

        // In a full implementation, would verify password here
        // For now, we'll just use the stored data
        (void)password;
    } else {
        // Not encrypted
        wallet_data.assign(file_data.begin() + offset, file_data.end());
    }

    // Deserialize wallet data
    HDWallet wallet;
    offset = 0;

    // Deserialize master seed (64 bytes)
    if (wallet_data.size() < 64) {
        return HDWallet();
    }
    wallet.master_seed_.assign(wallet_data.begin(), wallet_data.begin() + 64);
    offset += 64;

    // Deserialize mnemonic
    if (wallet_data.size() < offset + sizeof(uint32_t)) {
        return HDWallet();
    }
    uint32_t mnemonic_len;
    std::memcpy(&mnemonic_len, wallet_data.data() + offset, sizeof(mnemonic_len));
    offset += sizeof(mnemonic_len);

    if (wallet_data.size() < offset + mnemonic_len) {
        return HDWallet();
    }
    wallet.mnemonic_.assign(
        wallet_data.begin() + offset,
        wallet_data.begin() + offset + mnemonic_len
    );
    offset += mnemonic_len;

    // Deserialize key count and next index
    if (wallet_data.size() < offset + sizeof(uint32_t) * 2) {
        return HDWallet();
    }
    uint32_t key_count;
    std::memcpy(&key_count, wallet_data.data() + offset, sizeof(key_count));
    offset += sizeof(key_count);

    std::memcpy(&wallet.next_key_index_, wallet_data.data() + offset, sizeof(wallet.next_key_index_));
    offset += sizeof(wallet.next_key_index_);

    // Deserialize address labels
    if (wallet_data.size() < offset + sizeof(uint32_t)) {
        return HDWallet();
    }
    uint32_t label_count;
    std::memcpy(&label_count, wallet_data.data() + offset, sizeof(label_count));
    offset += sizeof(label_count);

    for (uint32_t i = 0; i < label_count; ++i) {
        if (wallet_data.size() < offset + sizeof(uint32_t)) {
            return HDWallet();
        }

        uint32_t addr_len;
        std::memcpy(&addr_len, wallet_data.data() + offset, sizeof(addr_len));
        offset += sizeof(addr_len);

        if (wallet_data.size() < offset + addr_len) {
            return HDWallet();
        }
        std::string address(wallet_data.begin() + offset, wallet_data.begin() + offset + addr_len);
        offset += addr_len;

        if (wallet_data.size() < offset + sizeof(uint32_t)) {
            return HDWallet();
        }
        uint32_t label_len;
        std::memcpy(&label_len, wallet_data.data() + offset, sizeof(label_len));
        offset += sizeof(label_len);

        if (wallet_data.size() < offset + label_len) {
            return HDWallet();
        }
        std::string label(wallet_data.begin() + offset, wallet_data.begin() + offset + label_len);
        offset += label_len;

        wallet.address_labels_[address] = label;
    }

    // Set encrypted flag if it was encrypted
    wallet.encrypted_ = (enc_flag == 1);

    return wallet;
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

    // LIMITATION: Dilithium doesn't support deterministic key derivation from seed in liboqs
    // The child_seed is derived correctly using HKDF, but liboqs doesn't provide
    // a way to generate keypairs from an arbitrary seed. This is a known limitation
    // of the NIST PQC reference implementations.
    // For now, generate a new random keypair. In production, this would require
    // either: (1) custom Dilithium implementation, or (2) storing full keypairs
    (void)child_seed;  // Mark as used
    return crypto::Dilithium::generate_keypair();
}

bool HDWallet::save_to_disk() const {
    // Use default wallet file path in data directory
    // In production, this should use a configurable data directory
    std::string wallet_path = "wallet.dat";
    return backup_to_file(wallet_path);
}

bool HDWallet::load_from_disk() {
    // Use default wallet file path
    std::string wallet_path = "wallet.dat";

    // Load wallet from file (no password for this method)
    HDWallet loaded = restore_from_file(wallet_path, "");

    if (loaded.master_seed_.empty()) {
        return false;  // Failed to load
    }

    // Copy loaded wallet data to this wallet
    this->master_seed_ = loaded.master_seed_;
    this->mnemonic_ = loaded.mnemonic_;
    this->keys_ = loaded.keys_;
    this->next_key_index_ = loaded.next_key_index_;
    this->address_labels_ = loaded.address_labels_;
    this->encrypted_ = loaded.encrypted_;
    this->encryption_key_ = loaded.encryption_key_;

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

bool HDWallet::owns_pubkey(const DilithiumPubKey& pubkey) const {
    for (const auto& [index, key] : keys_) {
        if (key.public_key == pubkey) {
            return true;
        }
    }
    return false;
}

std::set<DilithiumPubKey> HDWallet::get_all_pubkeys() const {
    std::set<DilithiumPubKey> pubkeys;
    for (const auto& [index, key] : keys_) {
        pubkeys.insert(key.public_key);
    }
    return pubkeys;
}

// Enhanced transaction filtering by pubkey matching
std::vector<Transaction> HDWallet::get_wallet_transactions(const Blockchain& blockchain) const {
    std::vector<Transaction> result;
    std::set<DilithiumPubKey> wallet_pubkeys = get_all_pubkeys();

    // Scan all blocks more efficiently by checking pubkeys directly
    for (uint32_t height = 0; height <= blockchain.get_height(); ++height) {
        Block block = blockchain.get_block_by_height(height);

        for (const auto& tx : block.transactions) {
            if (is_wallet_transaction(tx, blockchain)) {
                result.push_back(tx);
            }
        }
    }

    return result;
}

bool HDWallet::is_wallet_transaction(const Transaction& tx, const Blockchain& blockchain) const {
    std::set<DilithiumPubKey> wallet_pubkeys = get_all_pubkeys();

    // Check outputs - do we receive coins?
    for (const auto& output : tx.outputs) {
        if (output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
            DilithiumPubKey pubkey;
            std::copy(output.script_pubkey.begin(), output.script_pubkey.end(), pubkey.begin());
            if (wallet_pubkeys.count(pubkey)) {
                return true;
            }
        }
    }

    // Check inputs - do we spend coins?
    if (!tx.is_coinbase()) {
        for (const auto& input : tx.inputs) {
            auto utxo = blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);
            if (utxo && utxo->output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
                DilithiumPubKey pubkey;
                std::copy(utxo->output.script_pubkey.begin(), utxo->output.script_pubkey.end(), pubkey.begin());
                if (wallet_pubkeys.count(pubkey)) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Dynamic fee estimation
uint64_t HDWallet::estimate_fee(uint64_t tx_size_bytes, const Blockchain& blockchain, uint32_t target_blocks) const {
    // Get recent blocks to analyze fee rates
    uint32_t current_height = blockchain.get_height();
    if (current_height < target_blocks) {
        target_blocks = current_height;
    }

    std::vector<uint64_t> fee_rates;  // INT per byte

    // Analyze last N blocks
    for (uint32_t i = 0; i < target_blocks && i < 100; ++i) {
        if (current_height < i) break;

        Block block = blockchain.get_block_by_height(current_height - i);

        for (const auto& tx : block.transactions) {
            if (tx.is_coinbase()) continue;

            // Calculate transaction fee
            uint64_t total_input = 0;
            uint64_t total_output = 0;

            for (const auto& input : tx.inputs) {
                auto utxo = blockchain.get_utxo(input.previous_output.tx_hash, input.previous_output.index);
                if (utxo) {
                    total_input += utxo->output.value;
                }
            }

            for (const auto& output : tx.outputs) {
                total_output += output.value;
            }

            if (total_input > total_output) {
                uint64_t fee = total_input - total_output;
                uint64_t size = tx.serialize().size();
                if (size > 0) {
                    fee_rates.push_back(fee / size);  // INT per byte
                }
            }
        }
    }

    if (fee_rates.empty()) {
        // Default: 1000 INT per byte minimum
        return tx_size_bytes * 1000;
    }

    // Sort and use median fee rate
    std::sort(fee_rates.begin(), fee_rates.end());
    uint64_t median_rate = fee_rates[fee_rates.size() / 2];

    // Apply multiplier based on target confirmation time
    double multiplier = 1.0;
    if (target_blocks <= 2) {
        multiplier = 2.0;  // High priority
    } else if (target_blocks <= 6) {
        multiplier = 1.5;  // Medium priority
    }

    uint64_t estimated_fee = static_cast<uint64_t>(tx_size_bytes * median_rate * multiplier);

    // Minimum fee: 1000 INT per transaction
    const uint64_t MIN_FEE = 1000;
    return std::max(estimated_fee, MIN_FEE);
}

uint64_t HDWallet::estimate_transaction_size(size_t num_inputs, size_t num_outputs) const {
    // Transaction size estimation for Dilithium signatures
    const size_t VERSION_SIZE = 4;
    const size_t INPUT_COUNT_SIZE = 1;
    const size_t OUTPUT_COUNT_SIZE = 1;
    const size_t LOCKTIME_SIZE = 4;

    // Per input: outpoint (36 bytes) + script_sig length (1 byte) + Dilithium signature (~4627 bytes) + pubkey (2592 bytes)
    const size_t INPUT_SIZE = 36 + 1 + DILITHIUM_SIGNATURE_SIZE + DILITHIUM_PUBKEY_SIZE;

    // Per output: value (8 bytes) + script_pubkey length (1 byte) + pubkey (2592 bytes)
    const size_t OUTPUT_SIZE = 8 + 1 + DILITHIUM_PUBKEY_SIZE;

    return VERSION_SIZE + INPUT_COUNT_SIZE + (num_inputs * INPUT_SIZE) +
           OUTPUT_COUNT_SIZE + (num_outputs * OUTPUT_SIZE) + LOCKTIME_SIZE;
}

// Address book functionality
void HDWallet::add_address_book_entry(const AddressBookEntry& entry) {
    address_book_[entry.address] = entry;
    save_to_disk();
}

void HDWallet::remove_address_book_entry(const std::string& address) {
    address_book_.erase(address);
    save_to_disk();
}

void HDWallet::update_address_book_entry(const std::string& address, const AddressBookEntry& entry) {
    if (address_book_.count(address)) {
        address_book_[address] = entry;
        save_to_disk();
    }
}

std::optional<HDWallet::AddressBookEntry> HDWallet::get_address_book_entry(const std::string& address) const {
    auto it = address_book_.find(address);
    if (it != address_book_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<HDWallet::AddressBookEntry> HDWallet::get_address_book() const {
    std::vector<AddressBookEntry> result;
    for (const auto& [addr, entry] : address_book_) {
        result.push_back(entry);
    }
    return result;
}

std::vector<HDWallet::AddressBookEntry> HDWallet::search_address_book(const std::string& query) const {
    std::vector<AddressBookEntry> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (const auto& [addr, entry] : address_book_) {
        std::string lower_label = entry.label;
        std::string lower_address = entry.address;
        std::string lower_category = entry.category;
        std::string lower_notes = entry.notes;

        std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(), ::tolower);
        std::transform(lower_address.begin(), lower_address.end(), lower_address.begin(), ::tolower);
        std::transform(lower_category.begin(), lower_category.end(), lower_category.begin(), ::tolower);
        std::transform(lower_notes.begin(), lower_notes.end(), lower_notes.begin(), ::tolower);

        if (lower_label.find(lower_query) != std::string::npos ||
            lower_address.find(lower_query) != std::string::npos ||
            lower_category.find(lower_query) != std::string::npos ||
            lower_notes.find(lower_query) != std::string::npos) {
            result.push_back(entry);
        }
    }

    return result;
}

// QR code support
std::string HDWallet::generate_payment_uri(uint64_t amount, const std::string& label, const std::string& message) const {
    // Use the first address if available, otherwise return empty URI
    std::string address;
    if (!keys_.empty()) {
        address = keys_.begin()->second.address;
    } else {
        return "";  // No addresses available
    }

    std::string uri = "intcoin:" + address;

    bool first_param = true;
    if (amount > 0) {
        uri += "?amount=" + std::to_string(amount);
        first_param = false;
    }

    if (!label.empty()) {
        uri += (first_param ? "?" : "&") + std::string("label=") + label;
        first_param = false;
    }

    if (!message.empty()) {
        uri += (first_param ? "?" : "&") + std::string("message=") + message;
    }

    return uri;
}

bool HDWallet::parse_payment_uri(const std::string& uri, std::string& address, uint64_t& amount,
                                   std::string& label, std::string& message) const {
    // Parse URI format: intcoin:address?amount=123&label=foo&message=bar
    if (uri.substr(0, 8) != "intcoin:") {
        return false;
    }

    size_t query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        // No parameters, just address
        address = uri.substr(8);
        amount = 0;
        label = "";
        message = "";
        return true;
    }

    address = uri.substr(8, query_pos - 8);
    std::string params = uri.substr(query_pos + 1);

    // Parse parameters
    size_t pos = 0;
    while (pos < params.length()) {
        size_t eq_pos = params.find('=', pos);
        if (eq_pos == std::string::npos) break;

        size_t amp_pos = params.find('&', eq_pos);
        if (amp_pos == std::string::npos) amp_pos = params.length();

        std::string key = params.substr(pos, eq_pos - pos);
        std::string value = params.substr(eq_pos + 1, amp_pos - eq_pos - 1);

        if (key == "amount") {
            amount = std::stoull(value);
        } else if (key == "label") {
            label = value;
        } else if (key == "message") {
            message = value;
        }

        pos = amp_pos + 1;
    }

    return true;
}

// Hardware wallet support (placeholder implementations)
std::optional<HDWallet::HardwareWalletInfo> HDWallet::detect_hardware_wallet() const {
    // TODO: Implement actual hardware wallet detection via USB/HID
    // This would require platform-specific USB enumeration code
    // For now, return nullopt (no hardware wallet detected)
    return std::nullopt;
}

bool HDWallet::sign_with_hardware_wallet(Transaction& tx, const HardwareWalletInfo& hw_info) {
    // TODO: Implement hardware wallet signing protocol
    // This would communicate with the device via USB to sign transactions
    // Requires implementing device-specific protocols (Ledger, Trezor, etc.)
    (void)tx;
    (void)hw_info;
    return false;  // Not implemented yet
}

std::string HDWallet::get_hardware_wallet_address(const HardwareWalletInfo& hw_info, uint32_t index) {
    // TODO: Implement address derivation from hardware wallet
    // Would query the device for the public key at the given index
    (void)hw_info;
    (void)index;
    return "";  // Not implemented yet
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

    // Derive public key from private key
    // Note: Dilithium private key contains the public key
    // ML-DSA-87 (Dilithium5) private key is 4896 bytes
    // Public key is embedded in the private key structure
    if (private_key.size() == 4896) {
        // Extract public key from private key structure (last 2592 bytes)
        // In ML-DSA-87, the private key format includes the public key
        // For simplicity, we'll need to re-derive or extract it
        // This is a limitation - ideally we'd extract from the private key format
        wallet.public_key_ = {};  // Placeholder - would need liboqs private key parsing
        wallet.address_ = "";
    } else {
        wallet.public_key_ = {};
        wallet.address_ = "";
    }

    return wallet;
}

bool SimpleWallet::sign_transaction(Transaction& tx) {
    // Sign transaction inputs
    // Sign all inputs with the wallet's private key
    for (size_t i = 0; i < tx.inputs.size(); ++i) {
        if (!tx.sign(private_key_, i)) {
            return false;
        }
    }
    return true;
}

uint64_t SimpleWallet::get_balance(const Blockchain& blockchain) const {
    // Query blockchain for balance
    if (address_.empty()) {
        return 0;
    }

    // Get all UTXOs for this address from blockchain
    std::vector<UTXO> utxos = blockchain.get_utxos_for_address(address_);

    uint64_t balance = 0;
    for (const auto& utxo : utxos) {
        balance += utxo.output.value;
    }

    return balance;
}

// WalletDB implementation

WalletDB::WalletDB(const std::string& filepath)
    : filepath_(filepath)
{
}

bool WalletDB::save_wallet(const HDWallet& wallet) {
    // Serialize and save wallet using HDWallet's backup function
    return wallet.backup_to_file(filepath_);
}

std::optional<HDWallet> WalletDB::load_wallet(const std::string& password) {
    // Load and deserialize wallet using HDWallet's restore function
    try {
        HDWallet wallet = HDWallet::restore_from_file(filepath_, password);
        return wallet;
    } catch (...) {
        // Failed to load - could be wrong password, corrupted file, or file doesn't exist
        return std::nullopt;
    }
}

bool WalletDB::save_transaction(const Transaction& tx) {
    // Save transaction to separate file
    std::string tx_file = filepath_ + ".tx." + tx.get_txid();
    std::ofstream file(tx_file, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::vector<uint8_t> serialized = tx.serialize();
    file.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
    file.close();

    return true;
}

std::vector<Transaction> WalletDB::load_transactions() {
    // Load transactions from transaction files
    std::vector<Transaction> transactions;

    // In a production system, would scan directory for .tx.* files
    // For now, return empty vector as transaction storage is placeholder
    // Real implementation would use a proper database or index file

    return transactions;
}

} // namespace intcoin
