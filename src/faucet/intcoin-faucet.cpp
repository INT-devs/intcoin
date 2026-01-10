/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Testnet Faucet Server
 */

#include "intcoin/intcoin.h"
#include "intcoin/mining.h"
#include <iostream>
#include <string>
#include <csignal>
#include <cstring>

using namespace intcoin;

// Global instances
static faucet::FaucetServer* g_faucet = nullptr;
static mining::MiningManager* g_miner = nullptr;

// Signal handler for graceful shutdown
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down faucet server..." << std::endl;
        if (g_miner) {
            g_miner->Stop();
        }
        if (g_faucet) {
            g_faucet->Stop();
        }
        exit(0);
    }
}

void PrintUsage(const char* program_name) {
    std::cout << "INTcoin Testnet Faucet Server v" << INTCOIN_VERSION << std::endl;
    std::cout << INTCOIN_COPYRIGHT << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << "  --version           Show version information" << std::endl;
    std::cout << "  --datadir=<dir>     Data directory (default: ./data)" << std::endl;
    std::cout << "  --port=<port>       HTTP server port (default: 2215)" << std::endl;
    std::cout << "  --drip=<amount>     Amount per request in INT (default: 10)" << std::endl;
    std::cout << "  --ip-cooldown=<s>   IP cooldown in seconds (default: 3600)" << std::endl;
    std::cout << "  --addr-cooldown=<s> Address cooldown in seconds (default: 86400)" << std::endl;
    std::cout << "  --bind=<addr>       Bind address (default: 0.0.0.0)" << std::endl;
    std::cout << "  --fee=<amount>      Transaction fee in INTS (default: 1000)" << std::endl;
    std::cout << "  --mine              Enable background mining to fund faucet" << std::endl;
    std::cout << "  --threads=<n>       Number of mining threads (default: 1)" << std::endl;
    std::cout << std::endl;
}

void PrintVersion() {
    std::cout << "INTcoin Testnet Faucet v" << INTCOIN_VERSION << std::endl;
    std::cout << INTCOIN_COPYRIGHT << std::endl;
    std::cout << INTCOIN_LICENSE << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    std::string datadir = "./data";
    faucet::FaucetConfig config;
    bool enable_mining = false;
    uint32_t mining_threads = 1;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            PrintVersion();
            return 0;
        }
        else if (arg.substr(0, 10) == "--datadir=") {
            datadir = arg.substr(10);
        }
        else if (arg.substr(0, 7) == "--port=") {
            config.http_port = std::stoi(arg.substr(7));
        }
        else if (arg.substr(0, 7) == "--drip=") {
            double amount = std::stod(arg.substr(7));
            config.drip_amount = static_cast<uint64_t>(amount * 100000000);
        }
        else if (arg.substr(0, 14) == "--ip-cooldown=") {
            config.ip_cooldown = std::stoi(arg.substr(14));
        }
        else if (arg.substr(0, 16) == "--addr-cooldown=") {
            config.address_cooldown = std::stoi(arg.substr(16));
        }
        else if (arg.substr(0, 7) == "--bind=") {
            config.bind_address = arg.substr(7);
        }
        else if (arg.substr(0, 6) == "--fee=") {
            config.transaction_fee = std::stoull(arg.substr(6));
        }
        else if (arg == "--mine") {
            enable_mining = true;
        }
        else if (arg.substr(0, 10) == "--threads=") {
            mining_threads = std::stoi(arg.substr(10));
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return 1;
        }
    }

    // Print banner
    std::cout << "========================================" << std::endl;
    std::cout << "INTcoin Testnet Faucet Server" << std::endl;
    std::cout << "Version: " << INTCOIN_VERSION << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Set up signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        // Initialize blockchain
        std::cout << "Initializing blockchain..." << std::endl;
        std::string blockchain_dir = datadir + "/blockchain";

        auto blockchain_db = std::make_unique<BlockchainDB>(blockchain_dir);
        auto result = blockchain_db->Open();
        if (!result.IsOk()) {
            std::cerr << "Failed to open blockchain database: " << result.error << std::endl;
            return 1;
        }

        Blockchain blockchain(std::move(blockchain_db));

        // Initialize wallet
        std::cout << "Initializing faucet wallet..." << std::endl;

        wallet::WalletConfig wallet_config;
        wallet_config.data_dir = datadir + "/faucet_wallet";
        wallet_config.encrypted = false;

        wallet::Wallet wallet(wallet_config);

        // Check if wallet exists, if not create it
        auto wallet_result = wallet.Load();
        if (!wallet_result.IsOk()) {
            std::cout << "Creating new faucet wallet..." << std::endl;

            // Generate new mnemonic
            auto mnemonic_result = wallet::Mnemonic::Generate(24);
            if (!mnemonic_result.IsOk()) {
                std::cerr << "Failed to generate mnemonic: " << mnemonic_result.error << std::endl;
                return 1;
            }

            auto create_result = wallet.Create(mnemonic_result.GetValue(), "");  // No passphrase
            if (!create_result.IsOk()) {
                std::cerr << "Failed to create wallet: " << create_result.error << std::endl;
                return 1;
            }
        }

        // Get wallet balance
        auto balance_result = wallet.GetBalance();
        uint64_t balance = 0;
        if (balance_result.IsOk()) {
            balance = balance_result.GetValue();
        }
        std::cout << "Faucet wallet balance: " << (balance / 100000000.0) << " INT" << std::endl;

        if (balance == 0) {
            std::cout << "WARNING: Faucet wallet has zero balance!" << std::endl;
            std::cout << "Please send testnet coins to this address:" << std::endl;

            auto address_result = wallet.GetNewAddress("");
            if (address_result.IsOk()) {
                std::string address = address_result.GetValue();
                std::cout << "  " << address << std::endl;
            } else {
                std::cout << "  Failed to get address: " << address_result.error << std::endl;
            }
            std::cout << std::endl;
        }

        // Create faucet server
        std::cout << "Starting faucet server..." << std::endl;
        std::cout << "  HTTP Port: " << config.http_port << std::endl;
        std::cout << "  Drip Amount: " << (config.drip_amount / 100000000.0) << " INT" << std::endl;
        std::cout << "  IP Cooldown: " << config.ip_cooldown << " seconds" << std::endl;
        std::cout << "  Address Cooldown: " << config.address_cooldown << " seconds" << std::endl;
        std::cout << "  Bind Address: " << config.bind_address << std::endl;
        std::cout << std::endl;

        faucet::FaucetServer faucet(&wallet, &blockchain, config);
        g_faucet = &faucet;

        auto start_result = faucet.Start();
        if (!start_result.IsOk()) {
            std::cerr << "Failed to start faucet server: " << start_result.error << std::endl;
            return 1;
        }

        std::cout << "Faucet server running!" << std::endl;
        std::cout << "Web interface: http://" << config.bind_address << ":" << config.http_port << "/" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << std::endl;

        // Start mining if enabled
        std::unique_ptr<mining::MiningManager> miner;
        if (enable_mining) {
            std::cout << "Starting background mining..." << std::endl;

            // Get faucet wallet address for mining rewards
            std::string mining_address;
            auto addr_result = wallet.GetNewAddress("mining");
            if (addr_result.IsOk()) {
                mining_address = addr_result.GetValue();
            } else {
                std::cerr << "Failed to get mining address: " << addr_result.error << std::endl;
                return 1;
            }

            std::cout << "  Mining Address: " << mining_address << std::endl;
            std::cout << "  Mining Threads: " << mining_threads << std::endl;

            // Configure mining
            mining::MiningConfig mining_config;
            mining_config.mining_address = mining_address;
            mining_config.thread_count = mining_threads;
            mining_config.update_interval = 10;

            miner = std::make_unique<mining::MiningManager>(mining_config);
            g_miner = miner.get();

            // Set callback for when blocks are found
            miner->SetBlockFoundCallback([&blockchain](const Block& block) {
                std::cout << "[Mining] Block found! Height: " << (blockchain.GetBestHeight() + 1) << std::endl;
                auto add_result = blockchain.AddBlock(block);
                if (add_result.IsOk()) {
                    std::cout << "[Mining] Block added to chain successfully!" << std::endl;
                    std::cout << "[Mining] New height: " << blockchain.GetBestHeight() << std::endl;
                } else {
                    std::cerr << "[Mining] Failed to add block: " << add_result.error << std::endl;
                }
            });

            // Start mining
            auto mining_result = miner->Start(blockchain);
            if (!mining_result.IsOk()) {
                std::cerr << "Failed to start mining: " << mining_result.error << std::endl;
                std::cerr << "Continuing without mining..." << std::endl;
                miner.reset();
                g_miner = nullptr;
            } else {
                std::cout << "Background mining started!" << std::endl;
                std::cout << std::endl;
            }
        }

        // Main loop - print statistics periodically
        while (faucet.IsRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(60));

            auto stats = faucet.GetStats();
            std::cout << "[" << std::time(nullptr) << "] Faucet Stats:" << std::endl;
            std::cout << "  Total Distributions: " << stats.total_distributions << std::endl;
            std::cout << "  Total Amount: " << (stats.total_amount / 100000000.0) << " INT" << std::endl;
            std::cout << "  Pending Requests: " << stats.pending_requests << std::endl;
            std::cout << "  Failed Requests: " << stats.failed_requests << std::endl;
            std::cout << "  Rate Limited: " << stats.rate_limited_requests << std::endl;
            std::cout << "  Faucet Balance: " << (stats.faucet_balance / 100000000.0) << " INT" << std::endl;
            std::cout << "  Uptime: " << stats.uptime << " seconds" << std::endl;

            // Show mining stats if mining is enabled
            if (miner && miner->IsMining()) {
                auto mining_stats = miner->GetStats();
                std::cout << "  [Mining] Hashrate: " << mining::FormatHashrate(mining_stats.hashrate);
                std::cout << " | Blocks: " << mining_stats.blocks_found;
                std::cout << " | Chain Height: " << blockchain.GetBestHeight() << std::endl;
            }

            std::cout << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
