// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/blockchain.h"
#include "intcoin/mempool.h"
#include "intcoin/wallet.h"
#include "intcoin/miner.h"
#include "intcoin/p2p.h"
#include "intcoin/rpc.h"
#include "intcoin/version.h"

#include <iostream>
#include <fstream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <getopt.h>

using namespace intcoin;

// Global state for signal handling
std::atomic<bool> shutdown_requested(false);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received. Stopping daemon..." << std::endl;
        shutdown_requested = true;
    }
}

struct DaemonConfig {
    // Network
    uint16_t port = 9333;
    bool listen = true;
    std::vector<std::string> connect_nodes;
    std::vector<std::string> addnode;

    // RPC
    bool server = false;
    uint16_t rpc_port = 9332;
    std::string rpc_bind = "127.0.0.1";

    // Mining
    bool gen = false;
    size_t genproclimit = 0;  // 0 = auto-detect cores

    // Wallet
    std::string wallet_file = "wallet.dat";

    // Data directory
    std::string datadir = ".intcoin";

    // Logging
    bool debug = false;
    bool printtoconsole = true;

    // Daemon
    bool daemon_mode = false;
};

void print_help(const char* prog_name) {
    std::cout << "INTcoin Core Daemon v" << INTCOIN_VERSION_STRING << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << prog_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --version           Print version and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Network Options:" << std::endl;
    std::cout << "  -port=<port>            Listen on <port> for connections (default: 9333)" << std::endl;
    std::cout << "  -connect=<ip>           Connect only to specified node" << std::endl;
    std::cout << "  -addnode=<ip>           Add a node to connect to" << std::endl;
    std::cout << "  -listen=<0|1>           Accept connections from outside (default: 1)" << std::endl;
    std::cout << std::endl;
    std::cout << "RPC Server Options:" << std::endl;
    std::cout << "  -server                 Accept JSON-RPC commands" << std::endl;
    std::cout << "  -rpcport=<port>         Listen for JSON-RPC on <port> (default: 9332)" << std::endl;
    std::cout << "  -rpcbind=<addr>         Bind to given address (default: 127.0.0.1)" << std::endl;
    std::cout << std::endl;
    std::cout << "Mining Options:" << std::endl;
    std::cout << "  -gen                    Generate coins (mine)" << std::endl;
    std::cout << "  -genproclimit=<n>       Set processor limit for mining (default: auto)" << std::endl;
    std::cout << std::endl;
    std::cout << "Wallet Options:" << std::endl;
    std::cout << "  -wallet=<file>          Specify wallet file (default: wallet.dat)" << std::endl;
    std::cout << std::endl;
    std::cout << "Debugging/Testing Options:" << std::endl;
    std::cout << "  -debug                  Output debugging information" << std::endl;
    std::cout << "  -printtoconsole         Send trace/debug info to console" << std::endl;
    std::cout << "  -datadir=<dir>          Specify data directory (default: .intcoin)" << std::endl;
    std::cout << std::endl;
}

DaemonConfig parse_arguments(int argc, char* argv[]) {
    DaemonConfig config;

    // Simple argument parsing using string comparison
    // (getopt_long struct kept for reference but not used to avoid warning)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "INTcoin Core Daemon v" << INTCOIN_VERSION_STRING << std::endl;
            exit(EXIT_SUCCESS);
        } else if (arg.find("-port=") == 0) {
            config.port = std::stoi(arg.substr(6));
        } else if (arg.find("-connect=") == 0) {
            config.connect_nodes.push_back(arg.substr(9));
        } else if (arg.find("-addnode=") == 0) {
            config.addnode.push_back(arg.substr(9));
        } else if (arg.find("-listen=") == 0) {
            config.listen = (arg.substr(8) != "0");
        } else if (arg == "-server") {
            config.server = true;
        } else if (arg.find("-rpcport=") == 0) {
            config.rpc_port = std::stoi(arg.substr(9));
        } else if (arg.find("-rpcbind=") == 0) {
            config.rpc_bind = arg.substr(9);
        } else if (arg == "-gen") {
            config.gen = true;
        } else if (arg.find("-genproclimit=") == 0) {
            config.genproclimit = std::stoull(arg.substr(14));
        } else if (arg.find("-wallet=") == 0) {
            config.wallet_file = arg.substr(8);
        } else if (arg == "-debug") {
            config.debug = true;
        } else if (arg == "-printtoconsole") {
            config.printtoconsole = true;
        } else if (arg.find("-datadir=") == 0) {
            config.datadir = arg.substr(9);
        } else if (arg == "-daemon") {
            config.daemon_mode = true;
        }
    }

    return config;
}

void log_message(const DaemonConfig& config, const std::string& msg) {
    if (config.printtoconsole) {
        std::cout << "[" << std::time(nullptr) << "] " << msg << std::endl;

        // Also write to log file in datadir
        std::string log_file = config.data_dir + "/debug.log";
        std::ofstream log(log_file, std::ios::app);  // Append mode
        if (log.is_open()) {
            log << "[" << std::time(nullptr) << "] " << msg << std::endl;
            log.close();
        }
    }
}

void log_debug(const DaemonConfig& config, const std::string& msg) {
    if (config.debug) {
        log_message(config, "[DEBUG] " + msg);
    }
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    DaemonConfig config = parse_arguments(argc, argv);

    // Banner
    std::cout << "INTcoin Core Daemon v" << INTCOIN_VERSION_STRING << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core" << std::endl;
    std::cout << std::endl;

    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    log_message(config, "Starting INTcoin daemon...");
    log_debug(config, "Data directory: " + config.datadir);

    try {
        // Initialize blockchain
        log_message(config, "Initializing blockchain...");
        Blockchain blockchain;
        log_message(config, "Blockchain initialized. Height: " + std::to_string(blockchain.get_height()));

        // Initialize mempool
        log_message(config, "Initializing mempool...");
        Mempool mempool;
        log_message(config, "Mempool initialized.");

        // Initialize wallet
        log_message(config, "Loading wallet...");
        HDWallet* wallet = nullptr;
        std::string wallet_path = config.datadir + "/" + config.wallet_file;

        // Try to load existing wallet, create new one if not found
        std::ifstream wallet_test(wallet_path);
        if (wallet_test.good()) {
            wallet_test.close();
            log_message(config, "Wallet file found: " + wallet_path);

            // Load wallet from file
            wallet = new HDWallet();
            HDWallet loaded = HDWallet::restore_from_file(wallet_path, "");
            if (!loaded.get_seed().empty()) {
                *wallet = loaded;
                log_message(config, "Wallet loaded successfully.");
            } else {
                log_message(config, "ERROR: Failed to load wallet from file");
                delete wallet;
                return 1;
            }
        } else {
            log_message(config, "Creating new wallet...");
            wallet = new HDWallet();
            *wallet = HDWallet::create_new("");
            wallet->generate_new_key("Default");
            log_message(config, "New wallet created.");
            log_message(config, "Default address: " + wallet->get_all_addresses()[0]);

            // Save wallet to file
            if (wallet->backup_to_file(wallet_path)) {
                log_message(config, "Wallet saved to: " + wallet_path);
            } else {
                log_message(config, "WARNING: Failed to save wallet to file");
            }
        }

        // Initialize P2P network
        log_message(config, "Initializing P2P network...");
        p2p::Network network(config.port, false);

        // Add seed nodes
        for (const auto& node : config.addnode) {
            log_message(config, "Adding node: " + node);
            // Parse IP:port
            size_t colon_pos = node.find(':');
            std::string ip = node.substr(0, colon_pos);
            uint16_t port = (colon_pos != std::string::npos)
                ? std::stoi(node.substr(colon_pos + 1))
                : 9333;
            network.add_seed_node(p2p::PeerAddress(ip, port));
        }

        if (config.listen) {
            log_message(config, "Starting P2P network on port " + std::to_string(config.port) + "...");
            network.start();
            log_message(config, "P2P network started.");
        } else {
            log_message(config, "P2P listening disabled.");
        }

        // Initialize miner
        Miner* miner = nullptr;
        if (config.gen) {
            log_message(config, "Initializing miner...");
            miner = new Miner(blockchain, mempool);

            // Get mining address from wallet
            auto addresses = wallet->get_all_addresses();
            if (!addresses.empty()) {
                auto keys = wallet->get_all_keys();
                size_t threads = config.genproclimit;
                if (threads == 0) {
                    threads = std::thread::hardware_concurrency();
                    if (threads == 0) threads = 1;
                }

                log_message(config, "Starting miner with " + std::to_string(threads) + " threads...");
                log_message(config, "Mining to address: " + addresses[0]);
                miner->start(keys[0].public_key, threads);
                log_message(config, "Miner started.");
            } else {
                log_message(config, "ERROR: Cannot start mining - no addresses in wallet");
            }
        }

        // Initialize RPC server
        rpc::Server* rpc_server = nullptr;
        if (config.server) {
            log_message(config, "Initializing RPC server...");
            rpc_server = new rpc::Server(config.rpc_port, blockchain, mempool, wallet, miner, &network);
            rpc_server->start();
            log_message(config, "RPC server listening on " + config.rpc_bind + ":" + std::to_string(config.rpc_port));
            log_message(config, "RPC server started. Use intcoin-cli to send commands.");
        }

        // Main loop
        log_message(config, "");
        log_message(config, "INTcoin daemon is running. Press Ctrl+C to stop.");
        log_message(config, "");

        // Print status
        auto print_status = [&]() {
            log_message(config, "Status:");
            log_message(config, "  Blockchain height: " + std::to_string(blockchain.get_height()));
            log_message(config, "  Mempool size: " + std::to_string(mempool.size()) + " transactions");
            log_message(config, "  Network peers: " + std::to_string(network.peer_count()));
            if (wallet) {
                uint64_t balance = wallet->get_balance(blockchain);
                double balance_coins = static_cast<double>(balance) / COIN;
                log_message(config, "  Wallet balance: " + std::to_string(balance_coins) + " INT");
            }
            if (miner && miner->is_mining()) {
                auto stats = miner->get_stats();
                log_message(config, "  Mining: " + std::to_string(stats.hashes_per_second) + " H/s, "
                          + std::to_string(stats.blocks_found) + " blocks found");
            }
        };

        print_status();

        // Main daemon loop
        auto last_status_time = std::chrono::steady_clock::now();
        while (!shutdown_requested) {
            // Sleep for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Print status periodically (every 60 seconds)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_status_time).count();
            if (elapsed >= 60) {
                print_status();
                last_status_time = now;
            }

            // Process any pending network messages, mempool updates, etc.
            // Event loop processing
            if (network) {
                // Network is already processing in background threads
                // Check for new blocks or transactions via callbacks
            }

            // Process mempool - remove expired transactions
            if (mempool) {
                // Mempool management is handled automatically
                // Could add periodic cleanup or rebroadcast logic here
            }

            // Check blockchain sync status
            if (blockchain) {
                // Blockchain height is updated as blocks arrive
                // Could add sync progress logging here
            }
        }

        // Shutdown sequence
        log_message(config, "");
        log_message(config, "Shutting down...");

        // Stop miner
        if (miner) {
            log_message(config, "Stopping miner...");
            miner->stop();
            delete miner;
            log_message(config, "Miner stopped.");
        }

        // Stop RPC server
        if (rpc_server) {
            log_message(config, "Stopping RPC server...");
            rpc_server->stop();
            delete rpc_server;
            log_message(config, "RPC server stopped.");
        }

        // Stop network
        if (network.is_running()) {
            log_message(config, "Stopping P2P network...");
            network.stop();
            log_message(config, "P2P network stopped.");
        }

        // Save wallet
        if (wallet) {
            log_message(config, "Saving wallet...");
            if (wallet->backup_to_file(wallet_path)) {
                log_message(config, "Wallet saved successfully.");
            } else {
                log_message(config, "WARNING: Failed to save wallet.");
            }
            delete wallet;
        }

        log_message(config, "");
        log_message(config, "Shutdown complete. Goodbye!");

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
