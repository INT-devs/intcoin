// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/rpc.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

using namespace intcoin;
using namespace intcoin::rpc;

void print_help() {
    std::cout << "INTcoin CLI v0.1.0" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: intcoin-cli [options] <command> [params]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -rpcconnect=<ip>   Connect to RPC server (default: 127.0.0.1)" << std::endl;
    std::cout << "  -rpcport=<port>    Connect to RPC port (default: 9332)" << std::endl;
    std::cout << "  -h, --help         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Blockchain Commands:" << std::endl;
    std::cout << "  getblockcount                    Get current block height" << std::endl;
    std::cout << "  getblockhash <height>            Get block hash at height" << std::endl;
    std::cout << "  getblock <hash>                  Get block information" << std::endl;
    std::cout << "  getblockchaininfo                Get blockchain status" << std::endl;
    std::cout << std::endl;
    std::cout << "Wallet Commands:" << std::endl;
    std::cout << "  getnewaddress [label]            Generate new address" << std::endl;
    std::cout << "  getbalance                       Get wallet balance" << std::endl;
    std::cout << "  listaddresses                    List all wallet addresses" << std::endl;
    std::cout << "  sendtoaddress <addr> <amount>    Send coins to address" << std::endl;
    std::cout << "  listtransactions                 List wallet transactions" << std::endl;
    std::cout << std::endl;
    std::cout << "Mining Commands:" << std::endl;
    std::cout << "  getmininginfo                    Get mining information" << std::endl;
    std::cout << "  startmining [threads]            Start mining" << std::endl;
    std::cout << "  stopmining                       Stop mining" << std::endl;
    std::cout << std::endl;
    std::cout << "Network Commands:" << std::endl;
    std::cout << "  getpeerinfo                      Get peer information" << std::endl;
    std::cout << "  getnetworkinfo                   Get network status" << std::endl;
    std::cout << "  addnode <node>                   Add network node" << std::endl;
    std::cout << std::endl;
    std::cout << "Mempool Commands:" << std::endl;
    std::cout << "  getmempoolinfo                   Get mempool information" << std::endl;
    std::cout << "  getrawmempool                    List mempool transactions" << std::endl;
    std::cout << std::endl;
    std::cout << "Utility Commands:" << std::endl;
    std::cout << "  help                             List available commands" << std::endl;
    std::cout << "  stop                             Stop RPC server" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse options
    std::string rpc_host = "127.0.0.1";
    uint16_t rpc_port = 9332;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help();
            return EXIT_SUCCESS;
        } else if (arg.find("-rpcconnect=") == 0) {
            rpc_host = arg.substr(12);
        } else if (arg.find("-rpcport=") == 0) {
            rpc_port = static_cast<uint16_t>(std::stoi(arg.substr(9)));
        } else {
            args.push_back(arg);
        }
    }

    if (args.empty()) {
        print_help();
        return EXIT_FAILURE;
    }

    // Extract command and params
    std::string command = args[0];
    std::vector<std::string> params(args.begin() + 1, args.end());

    // Create RPC client
    Client client(rpc_host, rpc_port);

    // Attempt to connect
    if (!client.connect()) {
        std::cerr << "Error: Could not connect to RPC server at " << rpc_host << ":" << rpc_port << std::endl;
        std::cerr << "Make sure intcoind is running with -server option" << std::endl;
        return EXIT_FAILURE;
    }

    // Execute RPC command
    Response response = client.call(command, params);

    // Handle response
    if (response.success) {
        std::cout << response.result << std::endl;
    } else {
        std::cerr << "Error: " << response.error << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
