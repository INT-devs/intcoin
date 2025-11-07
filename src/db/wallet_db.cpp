// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet_db.h"
#include "intcoin/crypto.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <pwd.h>
#endif

namespace intcoin {

// WalletDatabase implementation

bool WalletDatabase::open(const std::string& filepath, bool create_if_missing) {
    return db_.open(filepath, create_if_missing);
}

void WalletDatabase::close() {
    db_.close();
}

bool WalletDatabase::save_wallet(const HDWallet& wallet, const std::string& passphrase) {
    // Use wallet's built-in backup method
    // This is a placeholder - actual implementation would serialize wallet data
    (void)wallet;
    (void)passphrase;
    return false;
}

std::optional<HDWallet> WalletDatabase::load_wallet(const std::string& passphrase) {
    // Use wallet's built-in restore method  
    // This is a placeholder - actual implementation would deserialize wallet data
    (void)passphrase;
    return std::nullopt;
}

bool WalletDatabase::write_metadata(const std::string& key, const std::string& value) {
    if (!db_.is_open()) return false;
    return db_.write("meta_" + key, value);
}

std::optional<std::string> WalletDatabase::read_metadata(const std::string& key) const {
    if (!db_.is_open()) return std::nullopt;
    return db_.read("meta_" + key);
}

bool WalletDatabase::save_key(uint32_t index, const WalletKey& key) {
    if (!db_.is_open()) return false;

    Database::Batch batch;
    std::string key_prefix = "key_" + std::to_string(index) + "_";

    batch.write(key_prefix + "address", key.address);
    batch.write(key_prefix + "pubkey",
               std::vector<uint8_t>(key.public_key.begin(), key.public_key.end()));
    batch.write(key_prefix + "privkey", key.private_key);

    if (!key.label.empty()) {
        batch.write(key_prefix + "label", key.label);
    }

    return db_.write_batch(batch);
}

std::vector<std::pair<uint32_t, WalletKey>> WalletDatabase::load_keys() const {
    std::vector<std::pair<uint32_t, WalletKey>> result;
    if (!db_.is_open()) return result;

    auto key_count_str = db_.read("key_count");
    if (!key_count_str) return result;

    size_t key_count = std::stoull(*key_count_str);

    for (size_t i = 0; i < key_count; ++i) {
        std::string key_prefix = "key_" + std::to_string(i) + "_";

        auto address = db_.read(key_prefix + "address");
        auto pubkey_data = db_.read_bytes(key_prefix + "pubkey");
        auto privkey_data = db_.read_bytes(key_prefix + "privkey");

        if (!address || !pubkey_data || !privkey_data) continue;
        if (pubkey_data->size() != DILITHIUM_PUBKEY_SIZE) continue;

        WalletKey key;
        key.address = *address;
        std::copy(pubkey_data->begin(), pubkey_data->end(), key.public_key.begin());
        key.private_key = *privkey_data;

        auto label = db_.read(key_prefix + "label");
        if (label) {
            key.label = *label;
        }

        result.emplace_back(i, key);
    }

    return result;
}

bool WalletDatabase::is_encrypted() const {
    if (!db_.is_open()) return false;
    auto encrypted = db_.read("encrypted");
    return (encrypted && *encrypted == "1");
}

bool WalletDatabase::encrypt_wallet(const std::string& passphrase) {
    (void)passphrase;
    return false;
}

bool WalletDatabase::change_passphrase(const std::string& old_pass, const std::string& new_pass) {
    (void)old_pass;
    (void)new_pass;
    return false;
}

bool WalletDatabase::backup(const std::string& backup_path) const {
    if (!db_.is_open()) return false;

    try {
        std::filesystem::copy(db_.get_stats().db_path, backup_path,
                            std::filesystem::copy_options::recursive);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Backup failed: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint8_t> WalletDatabase::encrypt_data(const std::vector<uint8_t>& data,
                                                   const std::string& passphrase) const {
    auto pass_hash = crypto::SHA256_PoW::hash(std::vector<uint8_t>(passphrase.begin(), passphrase.end()));

    std::vector<uint8_t> encrypted = data;
    for (size_t i = 0; i < encrypted.size(); ++i) {
        encrypted[i] ^= pass_hash[i % pass_hash.size()];
    }

    return encrypted;
}

std::optional<std::vector<uint8_t>> WalletDatabase::decrypt_data(
    const std::vector<uint8_t>& encrypted,
    const std::string& passphrase) const {
    return encrypt_data(encrypted, passphrase);
}


// ConfigManager implementation

std::optional<ConfigManager::Config> ConfigManager::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Config file not found: " << filepath << std::endl;
        return std::nullopt;
    }

    Config config;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto [key, value] = parse_line(line);

        if (key.empty()) continue;

        if (key == "port") {
            config.port = std::stoul(value);
        } else if (key == "listen") {
            config.listen = (value == "1" || value == "true");
        } else if (key == "connect") {
            config.connect.push_back(value);
        } else if (key == "addnode") {
            config.addnode.push_back(value);
        } else if (key == "testnet") {
            config.testnet = (value == "1" || value == "true");
        } else if (key == "server") {
            config.server = (value == "1" || value == "true");
        } else if (key == "rpcport") {
            config.rpc_port = std::stoul(value);
        } else if (key == "rpcuser") {
            config.rpc_user = value;
        } else if (key == "rpcpassword") {
            config.rpc_password = value;
        } else if (key == "rpcallowip") {
            config.rpc_allow_ip.push_back(value);
        } else if (key == "gen") {
            config.gen = (value == "1" || value == "true");
        } else if (key == "genproclimit") {
            config.genproclimit = std::stoull(value);
        } else if (key == "wallet") {
            config.wallet_file = value;
        } else if (key == "datadir") {
            config.datadir = value;
        } else if (key == "debug") {
            config.debug = (value == "1" || value == "true");
        } else if (key == "printtoconsole") {
            config.printtoconsole = (value == "1" || value == "true");
        }
    }

    if (config.datadir.empty()) {
        config.datadir = get_default_datadir();
    }

    return config;
}

bool ConfigManager::save(const Config& config, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to write config file: " << filepath << std::endl;
        return false;
    }

    file << "# INTcoin Configuration File\n\n";

    file << "# Network\n";
    file << "port=" << config.port << "\n";
    file << "listen=" << (config.listen ? "1" : "0") << "\n";

    for (const auto& node : config.connect) {
        file << "connect=" << node << "\n";
    }
    for (const auto& node : config.addnode) {
        file << "addnode=" << node << "\n";
    }

    file << "testnet=" << (config.testnet ? "1" : "0") << "\n\n";

    file << "# RPC\n";
    file << "server=" << (config.server ? "1" : "0") << "\n";
    file << "rpcport=" << config.rpc_port << "\n";

    if (!config.rpc_user.empty()) {
        file << "rpcuser=" << config.rpc_user << "\n";
    }
    if (!config.rpc_password.empty()) {
        file << "rpcpassword=" << config.rpc_password << "\n";
    }

    for (const auto& ip : config.rpc_allow_ip) {
        file << "rpcallowip=" << ip << "\n";
    }
    file << "\n";

    file << "# Mining\n";
    file << "gen=" << (config.gen ? "1" : "0") << "\n";
    if (config.genproclimit > 0) {
        file << "genproclimit=" << config.genproclimit << "\n";
    }
    file << "\n";

    file << "# Wallet\n";
    file << "wallet=" << config.wallet_file << "\n\n";

    file << "# Data directory\n";
    file << "datadir=" << config.datadir << "\n\n";

    file << "# Logging\n";
    file << "debug=" << (config.debug ? "1" : "0") << "\n";
    file << "printtoconsole=" << (config.printtoconsole ? "1" : "0") << "\n";

    return true;
}

std::string ConfigManager::get_default_config_path() {
    return get_default_datadir() + "/intcoin.conf";
}

std::string ConfigManager::get_default_datadir() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "\\INTcoin";
    }
    return ".intcoin";
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            home = pwd->pw_dir;
        }
    }

    if (home) {
#ifdef __APPLE__
        return std::string(home) + "/Library/Application Support/INTcoin";
#else
        return std::string(home) + "/.intcoin";
#endif
    }

    return ".intcoin";
#endif
}

std::string ConfigManager::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::pair<std::string, std::string> ConfigManager::parse_line(const std::string& line) {
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
        return {"", ""};
    }

    std::string key = trim(line.substr(0, eq_pos));
    std::string value = trim(line.substr(eq_pos + 1));

    return {key, value};
}

} // namespace intcoin
