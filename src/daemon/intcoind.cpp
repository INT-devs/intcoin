// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/intcoin.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <filesystem>

using namespace intcoin;

// Global shutdown flag
static volatile bool g_shutdown_requested = false;

// Signal handler
void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down...\n";
    g_shutdown_requested = true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "INTcoin Daemon v" << INTCOIN_VERSION_MAJOR << "."
              << INTCOIN_VERSION_MINOR << "." << INTCOIN_VERSION_PATCH << "\n";
    std::cout << "Post-Quantum Cryptocurrency\n";
    std::cout << "========================================\n\n";

    // Simple argument parsing
    std::string data_dir = "./data";
    bool testnet = false;
    uint16_t p2p_port = 2210;
    uint16_t rpc_port = 2211;
    std::string rpc_user = "";
    std::string rpc_password = "";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: intcoind [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -h, --help              Show this help message\n";
            std::cout << "  -v, --version           Show version information\n";
            std::cout << "  -datadir=<dir>          Specify data directory (default: ./data)\n";
            std::cout << "  -testnet                Run on testnet\n";
            std::cout << "  -port=<port>            P2P port (default: 2210)\n";
            std::cout << "  -rpcport=<port>         RPC port (default: 2211)\n";
            std::cout << "  -rpcuser=<user>         RPC username\n";
            std::cout << "  -rpcpassword=<pass>     RPC password\n";
            return 0;
        }
        else if (arg == "-v" || arg == "--version") {
            std::cout << "INTcoin Daemon v" << INTCOIN_VERSION_MAJOR << "."
                      << INTCOIN_VERSION_MINOR << "." << INTCOIN_VERSION_PATCH << "\n";
            return 0;
        }
        else if (arg.find("-datadir=") == 0) {
            data_dir = arg.substr(9);
        }
        else if (arg == "-testnet") {
            testnet = true;
            p2p_port = 12210;
            rpc_port = 12211;
        }
        else if (arg.find("-port=") == 0) {
            p2p_port = std::stoi(arg.substr(6));
        }
        else if (arg.find("-rpcport=") == 0) {
            rpc_port = std::stoi(arg.substr(9));
        }
        else if (arg.find("-rpcuser=") == 0) {
            rpc_user = arg.substr(9);
        }
        else if (arg.find("-rpcpassword=") == 0) {
            rpc_password = arg.substr(13);
        }
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create data directories
    std::string blockchain_dir = data_dir + "/blockchain";
    std::filesystem::create_directories(blockchain_dir);

    std::cout << "Network: " << (testnet ? "testnet" : "mainnet") << "\n";
    std::cout << "P2P Port: " << p2p_port << "\n";
    std::cout << "RPC Port: " << rpc_port << "\n";
    std::cout << "Data Directory: " << data_dir << "\n\n";

    // Initialize blockchain database
    std::cout << "Initializing blockchain...\n";
    auto db = std::make_shared<BlockchainDB>(blockchain_dir);
    auto db_result = db->Open();
    if (!db_result.IsOk()) {
        std::cerr << "ERROR: Failed to open database: " << db_result.error << "\n";
        return 1;
    }

    // Initialize blockchain
    Blockchain blockchain(db);
    auto init_result = blockchain.Initialize();
    if (!init_result.IsOk()) {
        std::cerr << "ERROR: Failed to initialize blockchain: " << init_result.error << "\n";
        return 1;
    }

    std::cout << "✓ Blockchain initialized\n";
    std::cout << "  Block height: " << blockchain.GetBestHeight() << "\n";
    std::cout << "  Best block: " << ToHex(blockchain.GetBestBlockHash()) << "\n\n";

    // Initialize P2P network
    std::cout << "Starting P2P network...\n";
    uint32_t network_magic = testnet ? 0xA1B2C3D5 : 0xA1B2C3D4;
    P2PNode p2p_node(network_magic, p2p_port);

    auto p2p_result = p2p_node.Start();
    if (!p2p_result.IsOk()) {
        std::cerr << "ERROR: Failed to start P2P network: " << p2p_result.error << "\n";
        return 1;
    }

    std::cout << "✓ P2P network started on port " << p2p_port << "\n\n";

    // Initialize RPC server
    std::cout << "Starting RPC server...\n";
    rpc::RPCConfig rpc_config;
    rpc_config.port = rpc_port;
    rpc_config.bind_address = "127.0.0.1";
    rpc_config.rpc_user = rpc_user;
    rpc_config.rpc_password = rpc_password;
    rpc_config.allow_external = false;

    rpc::RPCServer rpc_server(rpc_config, blockchain, p2p_node);
    auto rpc_result = rpc_server.Start();
    if (!rpc_result.IsOk()) {
        std::cerr << "ERROR: Failed to start RPC server: " << rpc_result.error << "\n";
        return 1;
    }

    std::cout << "✓ RPC server started on port " << rpc_port << "\n";
    if (!rpc_user.empty()) {
        std::cout << "  Authentication: enabled\n";
    } else {
        std::cout << "  WARNING: No RPC authentication configured!\n";
        std::cout << "  Set -rpcuser and -rpcpassword for security\n";
    }
    std::cout << "\n";

    std::cout << "========================================\n";
    std::cout << "INTcoin daemon is running\n";
    std::cout << "Press Ctrl+C to stop\n";
    std::cout << "========================================\n\n";

    // Main loop
    auto last_status = std::chrono::steady_clock::now();
    while (!g_shutdown_requested) {
        // Sleep for a short period
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Periodically log status (every 60 seconds)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_status).count();

        if (elapsed >= 60) {
            size_t peer_count = p2p_node.GetPeerCount();
            const Mempool& mempool = blockchain.GetMempool();
            std::cout << "[Status] Height: " << blockchain.GetBestHeight()
                      << " | Peers: " << peer_count
                      << " | Mempool: " << mempool.GetSize() << "\n";
            last_status = now;
        }
    }

    // Shutdown
    std::cout << "\nShutting down...\n";

    std::cout << "Stopping RPC server...\n";
    rpc_server.Stop();

    std::cout << "Stopping P2P network...\n";
    p2p_node.Stop();

    std::cout << "Closing blockchain...\n";
    // Blockchain and database will close when they go out of scope

    std::cout << "Shutdown complete.\n";
    std::cout << "Goodbye!\n";

    return 0;
}
