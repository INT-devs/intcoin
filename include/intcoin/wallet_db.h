// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_WALLET_DB_H
#define INTCOIN_WALLET_DB_H

#include "db.h"
#include "wallet.h"
#include <string>
#include <optional>

namespace intcoin {

/**
 * Wallet database for persistent storage of HD wallet data
 *
 * Stores:
 * - Master seed (encrypted)
 * - Derived keys and addresses
 * - Transaction history
 * - Address labels
 * - Wallet metadata (version, creation time, etc.)
 */
class WalletDatabase {
public:
    WalletDatabase() = default;

    /**
     * Open or create a wallet file
     * @param filepath Path to wallet file
     * @param create_if_missing Create new wallet if file doesn't exist
     * @return true if successful
     */
    bool open(const std::string& filepath, bool create_if_missing = true);

    /**
     * Close the wallet database
     */
    void close();

    /**
     * Check if wallet is open
     */
    bool is_open() const { return db_.is_open(); }

    /**
     * Save HD wallet to database
     * @param wallet The wallet to save
     * @param passphrase Optional passphrase for encryption
     * @return true if successful
     */
    bool save_wallet(const HDWallet& wallet, const std::string& passphrase = "");

    /**
     * Load HD wallet from database
     * @param passphrase Optional passphrase for decryption
     * @return Wallet if successful, std::nullopt otherwise
     */
    std::optional<HDWallet> load_wallet(const std::string& passphrase = "");

    /**
     * Write wallet metadata
     */
    bool write_metadata(const std::string& key, const std::string& value);

    /**
     * Read wallet metadata
     */
    std::optional<std::string> read_metadata(const std::string& key) const;

    /**
     * Save a derived key
     */
    bool save_key(uint32_t index, const WalletKey& key);

    /**
     * Load all derived keys
     */
    std::vector<std::pair<uint32_t, WalletKey>> load_keys() const;

    /**
     * Check if wallet is encrypted
     */
    bool is_encrypted() const;

    /**
     * Encrypt wallet with passphrase
     */
    bool encrypt_wallet(const std::string& passphrase);

    /**
     * Change wallet passphrase
     */
    bool change_passphrase(const std::string& old_pass, const std::string& new_pass);

    /**
     * Backup wallet to file
     */
    bool backup(const std::string& backup_path) const;

private:
    Database db_;

    // Serialize/deserialize helpers
    std::vector<uint8_t> serialize_wallet(const HDWallet& wallet) const;
    std::optional<HDWallet> deserialize_wallet(const std::vector<uint8_t>& data) const;

    std::vector<uint8_t> serialize_key(const WalletKey& key) const;
    std::optional<WalletKey> deserialize_key(const std::vector<uint8_t>& data) const;

    // Encryption helpers
    std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& data,
                                      const std::string& passphrase) const;
    std::optional<std::vector<uint8_t>> decrypt_data(const std::vector<uint8_t>& encrypted,
                                                     const std::string& passphrase) const;
};

/**
 * Configuration file manager
 *
 * Handles reading/writing intcoin.conf configuration
 */
class ConfigManager {
public:
    struct Config {
        // Network settings
        uint16_t port = 9333;
        bool listen = true;
        std::vector<std::string> connect;
        std::vector<std::string> addnode;
        bool testnet = false;

        // RPC settings
        bool server = false;
        uint16_t rpc_port = 9332;
        std::string rpc_user;
        std::string rpc_password;
        std::vector<std::string> rpc_allow_ip;

        // Mining settings
        bool gen = false;
        size_t genproclimit = 0;

        // Wallet settings
        std::string wallet_file = "wallet.dat";

        // Data directory
        std::string datadir;

        // Logging
        bool debug = false;
        bool printtoconsole = true;
    };

    ConfigManager() = default;

    /**
     * Load configuration from file
     * @param filepath Path to config file
     * @return Configuration if successful
     */
    static std::optional<Config> load(const std::string& filepath);

    /**
     * Save configuration to file
     * @param config Configuration to save
     * @param filepath Path to config file
     * @return true if successful
     */
    static bool save(const Config& config, const std::string& filepath);

    /**
     * Get default config path
     */
    static std::string get_default_config_path();

    /**
     * Get default data directory
     */
    static std::string get_default_datadir();

private:
    static std::string trim(const std::string& str);
    static std::pair<std::string, std::string> parse_line(const std::string& line);
};

} // namespace intcoin

#endif // INTCOIN_WALLET_DB_H
