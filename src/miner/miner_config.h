/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Enhanced Miner Configuration
 */

#ifndef INTCOIN_MINER_CONFIG_H
#define INTCOIN_MINER_CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

namespace intcoin {
namespace mining {

class MinerConfig {
public:
    // General settings
    uint32_t thread_count = 0;
    std::string mining_address;
    bool testnet = false;
    uint32_t update_interval = 5;
    bool verbose = false;
    std::string log_file;

    // Solo mining
    std::string daemon_host = "127.0.0.1";
    uint16_t daemon_port = 2211;
    std::string rpc_user;
    std::string rpc_password;
    uint32_t daemon_retry_delay = 5;

    // Pool mining
    bool pool_mining = false;
    std::vector<std::string> pool_urls; // Format: host:port
    std::string pool_username;
    std::string pool_password = "x";
    uint32_t pool_keepalive = 60;
    bool pool_failover = true;

    // Performance
    uint32_t batch_size = 100;
    bool affinity_enabled = false;
    uint32_t priority = 0; // 0=normal, 1=below normal, 2=idle
    bool huge_pages = false;

    // Advanced
    uint32_t max_retries = 3;
    uint32_t retry_pause = 5;
    bool benchmark_mode = false;
    uint32_t benchmark_duration = 60;

    /// Load configuration from file
    bool LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Parse key=value
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            // Parse config values
            if (key == "threads") thread_count = std::stoi(value);
            else if (key == "address") mining_address = value;
            else if (key == "testnet") testnet = (value == "true" || value == "1");
            else if (key == "update_interval") update_interval = std::stoi(value);
            else if (key == "verbose") verbose = (value == "true" || value == "1");
            else if (key == "log_file") log_file = value;
            else if (key == "daemon_host") daemon_host = value;
            else if (key == "daemon_port") daemon_port = std::stoi(value);
            else if (key == "rpc_user") rpc_user = value;
            else if (key == "rpc_password") rpc_password = value;
            else if (key == "pool") pool_mining = (value == "true" || value == "1");
            else if (key == "pool_url") pool_urls.push_back(value);
            else if (key == "pool_user") pool_username = value;
            else if (key == "pool_pass") pool_password = value;
            else if (key == "batch_size") batch_size = std::stoi(value);
            else if (key == "affinity") affinity_enabled = (value == "true" || value == "1");
            else if (key == "priority") priority = std::stoi(value);
            else if (key == "huge_pages") huge_pages = (value == "true" || value == "1");
            else if (key == "benchmark") benchmark_mode = (value == "true" || value == "1");
        }

        return true;
    }

    /// Save configuration to file
    bool SaveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << "# INTcoin Miner Configuration\n\n";

        file << "# General Settings\n";
        file << "threads=" << thread_count << "\n";
        file << "address=" << mining_address << "\n";
        file << "testnet=" << (testnet ? "true" : "false") << "\n";
        file << "update_interval=" << update_interval << "\n";
        file << "verbose=" << (verbose ? "true" : "false") << "\n";
        file << "log_file=" << log_file << "\n\n";

        file << "# Solo Mining\n";
        file << "daemon_host=" << daemon_host << "\n";
        file << "daemon_port=" << daemon_port << "\n";
        file << "rpc_user=" << rpc_user << "\n";
        file << "rpc_password=" << rpc_password << "\n\n";

        file << "# Pool Mining\n";
        file << "pool=" << (pool_mining ? "true" : "false") << "\n";
        for (const auto& url : pool_urls) {
            file << "pool_url=" << url << "\n";
        }
        file << "pool_user=" << pool_username << "\n";
        file << "pool_pass=" << pool_password << "\n\n";

        file << "# Performance\n";
        file << "batch_size=" << batch_size << "\n";
        file << "affinity=" << (affinity_enabled ? "true" : "false") << "\n";
        file << "priority=" << priority << "\n";
        file << "huge_pages=" << (huge_pages ? "true" : "false") << "\n\n";

        file << "# Advanced\n";
        file << "benchmark=" << (benchmark_mode ? "true" : "false") << "\n";

        return true;
    }

    /// Generate sample config file
    static bool GenerateSampleConfig(const std::string& filename) {
        MinerConfig sample;
        sample.thread_count = 4;
        sample.mining_address = "int1qxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        sample.pool_urls.push_back("pool.international-coin.org:3333");
        sample.pool_username = "worker1";

        return sample.SaveToFile(filename);
    }
};

} // namespace mining
} // namespace intcoin

#endif // INTCOIN_MINER_CONFIG_H
