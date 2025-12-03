// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/intcoin.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

using namespace intcoin;
using namespace intcoin::mining;

// Global mining manager
static MiningManager* g_mining_manager = nullptr;

// Signal handler
void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", stopping miner...\n";
    if (g_mining_manager) {
        g_mining_manager->Stop();
    }
}

void print_banner() {
    std::cout << "========================================\n";
    std::cout << "INTcoin CPU Miner v" << INTCOIN_VERSION_MAJOR << "."
              << INTCOIN_VERSION_MINOR << "." << INTCOIN_VERSION_PATCH << "\n";
    std::cout << "Post-Quantum Cryptocurrency Miner\n";
    std::cout << "RandomX Proof-of-Work\n";
    std::cout << "========================================\n\n";
}

void print_usage() {
    std::cout << "Usage: intcoin-miner [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -v, --version           Show version information\n";
    std::cout << "  -t, --threads=<n>       Number of mining threads (default: auto-detect)\n";
    std::cout << "  -a, --address=<addr>    Mining address (required for solo mining)\n";
    std::cout << "  --testnet               Mine on testnet\n";
    std::cout << "\n";
    std::cout << "Solo Mining:\n";
    std::cout << "  --daemon-host=<host>    intcoind RPC host (default: 127.0.0.1)\n";
    std::cout << "  --daemon-port=<port>    intcoind RPC port (default: 2211)\n";
    std::cout << "  --rpc-user=<user>       RPC username\n";
    std::cout << "  --rpc-password=<pass>   RPC password\n";
    std::cout << "\n";
    std::cout << "Pool Mining:\n";
    std::cout << "  --pool                  Enable pool mining\n";
    std::cout << "  --pool-host=<host>      Pool hostname\n";
    std::cout << "  --pool-port=<port>      Pool port (default: 3333)\n";
    std::cout << "  --pool-user=<user>      Pool username/worker name\n";
    std::cout << "  --pool-pass=<pass>      Pool password (default: x)\n";
    std::cout << "\n";
    std::cout << "Performance:\n";
    std::cout << "  --affinity              Enable CPU affinity\n";
    std::cout << "  --batch-size=<n>        Nonces per batch (default: 100)\n";
    std::cout << "  --update-interval=<n>   Stats update interval in seconds (default: 5)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  # Solo mining on mainnet\n";
    std::cout << "  intcoin-miner -a int1qxyz... --rpc-user=user --rpc-password=pass\n";
    std::cout << "\n";
    std::cout << "  # Solo mining on testnet with 4 threads\n";
    std::cout << "  intcoin-miner -a int1qxyz... -t 4 --testnet\n";
    std::cout << "\n";
    std::cout << "  # Pool mining\n";
    std::cout << "  intcoin-miner --pool --pool-host=pool.intcoin.org --pool-user=worker1\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    MiningConfig config;
    std::string daemon_host = "127.0.0.1";
    uint16_t daemon_port = 2211;
    std::string rpc_user;
    std::string rpc_password;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_banner();
            print_usage();
            return 0;
        }
        else if (arg == "-v" || arg == "--version") {
            std::cout << "INTcoin CPU Miner v" << INTCOIN_VERSION << "\n";
            return 0;
        }
        else if (arg.find("-t=") == 0 || arg.find("--threads=") == 0) {
            size_t eq_pos = arg.find('=');
            config.thread_count = std::stoi(arg.substr(eq_pos + 1));
        }
        else if (arg.find("-a=") == 0 || arg.find("--address=") == 0) {
            size_t eq_pos = arg.find('=');
            config.mining_address = arg.substr(eq_pos + 1);
        }
        else if (arg == "--testnet") {
            config.testnet = true;
            daemon_port = 12211;
        }
        else if (arg.find("--daemon-host=") == 0) {
            daemon_host = arg.substr(14);
        }
        else if (arg.find("--daemon-port=") == 0) {
            daemon_port = std::stoi(arg.substr(14));
        }
        else if (arg.find("--rpc-user=") == 0) {
            rpc_user = arg.substr(11);
        }
        else if (arg.find("--rpc-password=") == 0) {
            rpc_password = arg.substr(15);
        }
        else if (arg == "--pool") {
            config.pool_mining = true;
        }
        else if (arg.find("--pool-host=") == 0) {
            config.pool_host = arg.substr(12);
        }
        else if (arg.find("--pool-port=") == 0) {
            config.pool_port = std::stoi(arg.substr(12));
        }
        else if (arg.find("--pool-user=") == 0) {
            config.pool_username = arg.substr(12);
        }
        else if (arg.find("--pool-pass=") == 0) {
            config.pool_password = arg.substr(12);
        }
        else if (arg == "--affinity") {
            config.affinity_enabled = true;
        }
        else if (arg.find("--batch-size=") == 0) {
            config.batch_size = std::stoi(arg.substr(13));
        }
        else if (arg.find("--update-interval=") == 0) {
            config.update_interval = std::stoi(arg.substr(18));
        }
    }

    // Validate configuration
    if (!config.pool_mining && config.mining_address.empty()) {
        std::cerr << "ERROR: Mining address required for solo mining\n";
        std::cerr << "Use --address=<addr> or enable pool mining with --pool\n";
        return 1;
    }

    if (config.pool_mining && config.pool_host.empty()) {
        std::cerr << "ERROR: Pool host required for pool mining\n";
        std::cerr << "Use --pool-host=<host>\n";
        return 1;
    }

    // Auto-detect thread count if not specified
    if (config.thread_count == 0) {
        config.thread_count = DetectOptimalThreadCount();
    }

    // Print banner
    print_banner();

    // Print configuration
    std::cout << "Configuration:\n";
    std::cout << "  Mode: " << (config.pool_mining ? "Pool Mining" : "Solo Mining") << "\n";
    std::cout << "  Network: " << (config.testnet ? "Testnet" : "Mainnet") << "\n";
    std::cout << "  Threads: " << config.thread_count << "\n";

    if (!config.pool_mining) {
        std::cout << "  Mining Address: " << config.mining_address << "\n";
        std::cout << "  Daemon: " << daemon_host << ":" << daemon_port << "\n";
    } else {
        std::cout << "  Pool: " << config.pool_host << ":" << config.pool_port << "\n";
        std::cout << "  Worker: " << config.pool_username << "\n";
    }

    std::cout << "\n";

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize blockchain (for solo mining)
    std::unique_ptr<Blockchain> blockchain;

    if (!config.pool_mining) {
        std::cout << "Connecting to intcoind...\n";

        // For solo mining, we need to connect to intcoind via RPC
        // In a complete implementation, would use RPC client to get block templates

        std::cout << "NOTE: Solo mining via standalone miner requires RPC integration\n";
        std::cout << "For now, use: ./intcoin-cli generatetoaddress <blocks> <address>\n";
        std::cout << "Or mine through intcoind directly\n\n";

        // Fallback: Initialize blockchain for testing
        std::string data_dir = config.testnet ? "./data/testnet" : "./data/mainnet";
        auto db = std::make_shared<BlockchainDB>(data_dir + "/blockchain");

        auto db_result = db->Open();
        if (!db_result.IsOk()) {
            std::cerr << "ERROR: Failed to open database: " << db_result.error << "\n";
            std::cerr << "Make sure intcoind is running or database exists\n";
            return 1;
        }

        blockchain = std::make_unique<Blockchain>(db);
        auto init_result = blockchain->Initialize();
        if (!init_result.IsOk()) {
            std::cerr << "ERROR: Failed to initialize blockchain: " << init_result.error << "\n";
            return 1;
        }

        std::cout << "✓ Connected to blockchain (height: " << blockchain->GetBestHeight() << ")\n\n";
    }

    // Create mining manager
    MiningManager manager(config);
    g_mining_manager = &manager;

    // Setup callbacks
    manager.SetBlockFoundCallback([&](const Block& block) {
        std::cout << "\n*** BLOCK FOUND! ***\n";
        std::cout << "Block Hash: " << ToHex(block.GetHash()) << "\n";
        std::cout << "Height: " << (blockchain ? blockchain->GetBestHeight() + 1 : 0) << "\n";
        std::cout << "Nonce: " << block.header.nonce << "\n\n";

        // Submit block to blockchain
        if (blockchain) {
            auto result = blockchain->AddBlock(block);
            if (result.IsOk()) {
                std::cout << "✓ Block added to blockchain\n\n";
            } else {
                std::cout << "✗ Failed to add block: " << result.error << "\n\n";
            }
        }
    });

    // Pool mining
    std::unique_ptr<StratumClient> stratum_client;

    if (config.pool_mining) {
        std::cout << "Connecting to pool...\n";

        stratum_client = std::make_unique<StratumClient>(config);

        auto connect_result = stratum_client->Connect();
        if (!connect_result.IsOk()) {
            std::cerr << "ERROR: " << connect_result.error << "\n";
            return 1;
        }

        auto subscribe_result = stratum_client->Subscribe();
        if (!subscribe_result.IsOk()) {
            std::cerr << "ERROR: " << subscribe_result.error << "\n";
            return 1;
        }

        auto auth_result = stratum_client->Authorize();
        if (!auth_result.IsOk()) {
            std::cerr << "ERROR: " << auth_result.error << "\n";
            return 1;
        }

        std::cout << "✓ Connected to pool\n\n";

        // Setup callbacks
        stratum_client->SetJobCallback([&](const MiningJob& job) {
            std::cout << "New job received: " << job.job_id << "\n";
        });

        stratum_client->SetAcceptCallback([&](bool accepted, const std::string& reason) {
            if (accepted) {
                std::cout << "✓ Share accepted: " << reason << "\n";
            } else {
                std::cout << "✗ Share rejected: " << reason << "\n";
            }
        });

        manager.SetShareFoundCallback([&](const MiningResult& result) {
            std::cout << "Share found! Submitting...\n";
            auto job = stratum_client->GetCurrentJob();
            stratum_client->SubmitShare(result, job.job_id);
        });
    }

    // Start mining
    std::cout << "========================================\n";
    std::cout << "Starting miner...\n";
    std::cout << "Press Ctrl+C to stop\n";
    std::cout << "========================================\n\n";

    auto start_result = blockchain ? manager.Start(*blockchain) : Result<void>::Error("No blockchain");

    if (!start_result.IsOk() && !config.pool_mining) {
        std::cerr << "ERROR: Failed to start mining: " << start_result.error << "\n";
        return 1;
    }

    // Main loop
    while (manager.IsMining()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    std::cout << "\nStopping miner...\n";
    manager.Stop();

    if (stratum_client) {
        stratum_client->Disconnect();
    }

    // Print final stats
    auto stats = manager.GetStats();
    std::cout << "\n========================================\n";
    std::cout << "Mining Statistics:\n";
    std::cout << "========================================\n";
    std::cout << "Total Hashes: " << stats.hashes_computed << "\n";
    std::cout << "Blocks Found: " << stats.blocks_found << "\n";
    std::cout << "Shares Submitted: " << stats.shares_submitted << "\n";
    std::cout << "Shares Accepted: " << stats.shares_accepted << "\n";
    std::cout << "Shares Rejected: " << stats.shares_rejected << "\n";
    std::cout << "Average Hashrate: " << FormatHashrate(stats.average_hashrate) << "\n";
    std::cout << "Uptime: " << stats.uptime << " seconds\n";
    std::cout << "========================================\n\n";

    std::cout << "Miner stopped. Goodbye!\n";

    return 0;
}
