// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/miner.h"
#include "intcoin/blockchain.h"
#include "intcoin/mempool.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
#include <iomanip>
#include <getopt.h>

using namespace intcoin;

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown(false);

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    g_shutdown = true;
}

// Helper to convert hash to hex string
std::string hash_to_hex(const Hash256& hash) {
    static const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (uint8_t byte : hash) {
        result += hex_chars[byte >> 4];
        result += hex_chars[byte & 0x0F];
    }
    return result;
}

void print_usage(const char* program_name) {
    std::cout << "INTcoin CPU Miner v0.1.0-alpha" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a, --address <address>   Mining reward address (required)" << std::endl;
    std::cout << "  -t, --threads <n>         Number of mining threads (default: auto-detect)" << std::endl;
    std::cout << "  -d, --data-dir <path>     Data directory (default: ~/.intcoin)" << std::endl;
    std::cout << "  -n, --extra-nonce <text>  Extra nonce text (default: empty)" << std::endl;
    std::cout << "  -v, --verbose             Verbose output" << std::endl;
    std::cout << "  -h, --help                Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " --address INT1qw508d6qejxtdg4y5r3zarvary0c5xw7k --threads 4" << std::endl;
    std::cout << std::endl;
}

void print_stats(const MiningStats& stats) {
    std::cout << "\r";
    std::cout << "Hashrate: " << std::fixed << std::setprecision(2)
              << (stats.hashes_per_second / 1000000.0) << " MH/s | ";
    std::cout << "Total: " << (stats.total_hashes / 1000000) << "M hashes | ";
    std::cout << "Blocks: " << stats.blocks_found;
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Parse command line options
    std::string address_str;
    std::string data_dir;
    std::string extra_nonce;
    size_t num_threads = 0;
    bool verbose = false;

    static struct option long_options[] = {
        {"address",     required_argument, 0, 'a'},
        {"threads",     required_argument, 0, 't'},
        {"data-dir",    required_argument, 0, 'd'},
        {"extra-nonce", required_argument, 0, 'n'},
        {"verbose",     no_argument,       0, 'v'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "a:t:d:n:vh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'a':
                address_str = optarg;
                break;
            case 't':
                num_threads = std::stoul(optarg);
                break;
            case 'd':
                data_dir = optarg;
                break;
            case 'n':
                extra_nonce = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Validate required parameters
    if (address_str.empty()) {
        std::cerr << "Error: Mining address is required" << std::endl;
        std::cerr << "Use --help for usage information" << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize data directory
    if (data_dir.empty()) {
        const char* home = std::getenv("HOME");
        if (home) {
            data_dir = std::string(home) + "/.intcoin";
        } else {
            data_dir = ".intcoin";
        }
    }

    // Print startup banner
    std::cout << "INTcoin CPU Miner v0.1.0-alpha" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize blockchain and mempool
        if (verbose) {
            std::cout << "Initializing blockchain..." << std::endl;
            std::cout << "Data directory: " << data_dir << std::endl;
        }
        Blockchain blockchain;

        if (verbose) {
            std::cout << "Initializing mempool..." << std::endl;
        }
        Mempool mempool;

        // Parse mining address
        // For now, create a dummy Dilithium public key
        // TODO: Implement proper address parsing when address.cpp is complete
        DilithiumPubKey reward_address;
        if (verbose) {
            std::cout << "Mining to address: " << address_str << std::endl;
        }

        // Initialize miner
        if (verbose) {
            std::cout << "Initializing miner..." << std::endl;
        }
        Miner miner(blockchain, mempool);

        if (!extra_nonce.empty()) {
            miner.set_extra_nonce(extra_nonce);
            if (verbose) {
                std::cout << "Extra nonce: " << extra_nonce << std::endl;
            }
        }

        // Set up block found callback
        miner.set_block_found_callback([&](const Block& block) {
            std::cout << std::endl;
            std::cout << "*** BLOCK FOUND! ***" << std::endl;
            std::cout << "Height: " << blockchain.get_height() + 1 << std::endl;
            std::cout << "Hash: " << hash_to_hex(block.get_hash()) << std::endl;
            std::cout << "Nonce: " << block.header.nonce << std::endl;
            std::cout << "Transactions: " << block.transactions.size() << std::endl;
            std::cout << std::endl;

            // Add block to blockchain
            if (blockchain.add_block(block)) {
                std::cout << "Block added to blockchain" << std::endl;
            } else {
                std::cout << "Failed to add block to blockchain" << std::endl;
            }
        });

        // Determine thread count
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 1;
        }

        std::cout << "Starting miner with " << num_threads << " thread(s)..." << std::endl;
        std::cout << "Current height: " << blockchain.get_height() << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << std::endl;

        // Start mining
        if (!miner.start(reward_address, num_threads)) {
            std::cerr << "Failed to start miner" << std::endl;
            return EXIT_FAILURE;
        }

        // Main loop - print statistics
        while (!g_shutdown && miner.is_mining()) {
            if (verbose) {
                MiningStats stats = miner.get_stats();
                print_stats(stats);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop mining
        std::cout << std::endl;
        std::cout << "Stopping miner..." << std::endl;
        miner.stop();

        // Print final statistics
        MiningStats final_stats = miner.get_stats();
        std::cout << std::endl;
        std::cout << "Mining Statistics:" << std::endl;
        std::cout << "  Total hashes: " << final_stats.total_hashes << std::endl;
        std::cout << "  Blocks found: " << final_stats.blocks_found << std::endl;
        std::cout << "  Average hashrate: " << std::fixed << std::setprecision(2)
                  << (final_stats.hashes_per_second / 1000000.0) << " MH/s" << std::endl;
        std::cout << std::endl;
        std::cout << "Shutdown complete" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
