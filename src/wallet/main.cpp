// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <getopt.h>

using namespace intcoin;

void print_usage(const char* program_name) {
    std::cout << "INTcoin Wallet Tool v0.1.0-alpha" << std::endl;
    std::cout << "Copyright (c) 2025 INTcoin Core (Maddison Lane)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [command] [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  create                     Create a new wallet" << std::endl;
    std::cout << "  restore                    Restore wallet from mnemonic" << std::endl;
    std::cout << "  address                    Generate new address" << std::endl;
    std::cout << "  balance                    Show wallet balance" << std::endl;
    std::cout << "  list                       List all addresses" << std::endl;
    std::cout << "  send <address> <amount>    Send INT to address" << std::endl;
    std::cout << "  history                    Show transaction history" << std::endl;
    std::cout << "  backup <file>              Backup wallet to file" << std::endl;
    std::cout << "  mnemonic                   Show wallet mnemonic phrase" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -w, --wallet <file>        Wallet file (default: ~/.intcoin/wallet.dat)" << std::endl;
    std::cout << "  -p, --password <pass>      Wallet password" << std::endl;
    std::cout << "  -l, --label <label>        Address label" << std::endl;
    std::cout << "  -h, --help                 Show this help message" << std::endl;
    std::cout << std::endl;
}

int cmd_create(const std::string& wallet_file, const std::string& password) {
    std::cout << "Creating new wallet..." << std::endl;

    // Create new HD wallet
    HDWallet wallet = HDWallet::create_new(password);

    std::cout << "Wallet created successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "IMPORTANT: Write down your mnemonic phrase and keep it safe!" << std::endl;
    std::cout << "This is the ONLY way to recover your wallet if you lose it." << std::endl;
    std::cout << std::endl;
    std::cout << "Mnemonic phrase:" << std::endl;
    std::cout << wallet.get_mnemonic() << std::endl;
    std::cout << std::endl;

    // Get first address
    std::vector<std::string> addresses = wallet.get_all_addresses();
    if (!addresses.empty()) {
        std::cout << "Default address: " << addresses[0] << std::endl;
    }

    // Save wallet
    if (wallet.backup_to_file(wallet_file)) {
        std::cout << "Wallet saved to: " << wallet_file << std::endl;
    } else {
        std::cerr << "Failed to save wallet" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int cmd_restore(const std::string& wallet_file, const std::string& password) {
    std::cout << "Restore wallet from mnemonic phrase" << std::endl;
    std::cout << "Enter your 24-word mnemonic phrase:" << std::endl;

    std::string mnemonic;
    std::getline(std::cin, mnemonic);

    if (!crypto::Mnemonic::validate(mnemonic)) {
        std::cerr << "Error: Invalid mnemonic phrase" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Restoring wallet..." << std::endl;

    HDWallet wallet = HDWallet::from_mnemonic(mnemonic, password);

    std::cout << "Wallet restored successfully!" << std::endl;

    // Get first address
    std::vector<std::string> addresses = wallet.get_all_addresses();
    if (!addresses.empty()) {
        std::cout << "Default address: " << addresses[0] << std::endl;
    }

    // Save wallet
    if (wallet.backup_to_file(wallet_file)) {
        std::cout << "Wallet saved to: " << wallet_file << std::endl;
    } else {
        std::cerr << "Failed to save wallet" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int cmd_new_address(HDWallet& wallet, const std::string& label) {
    std::string address = wallet.get_new_address(label);
    std::cout << "New address: " << address << std::endl;
    if (!label.empty()) {
        std::cout << "Label: " << label << std::endl;
    }
    return EXIT_SUCCESS;
}

int cmd_balance(const HDWallet& wallet) {
    // Initialize blockchain and mempool - in production this would connect to node
    Blockchain blockchain;  // Use default constructor
    Mempool mempool;        // Use default constructor
    uint64_t balance = wallet.get_balance(blockchain);

    std::cout << "Balance: " << (balance / static_cast<double>(COIN)) << " INT" << std::endl;
    std::cout << "         " << balance << " satoshis" << std::endl;

    uint64_t unconfirmed = wallet.get_unconfirmed_balance(mempool, blockchain);
    if (unconfirmed > 0) {
        std::cout << "Unconfirmed: " << (unconfirmed / static_cast<double>(COIN)) << " INT" << std::endl;
    }

    return EXIT_SUCCESS;
}

int cmd_list_addresses(const HDWallet& wallet) {
    std::vector<std::string> addresses = wallet.get_all_addresses();

    std::cout << "Addresses (" << addresses.size() << "):" << std::endl;
    for (const auto& addr : addresses) {
        std::string label = wallet.get_address_label(addr);
        std::cout << "  " << addr;
        if (!label.empty()) {
            std::cout << " (" << label << ")";
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}

int cmd_send(HDWallet& wallet, const std::string& to_address, double amount_int) {
    uint64_t amount = static_cast<uint64_t>(amount_int * COIN);
    uint64_t fee = 10000;  // 0.0001 INT default fee

    std::cout << "Sending " << amount_int << " INT to " << to_address << std::endl;
    std::cout << "Fee: " << (fee / static_cast<double>(COIN)) << " INT" << std::endl;

    // Initialize blockchain - in production this would connect to node
    Blockchain blockchain;  // Use default constructor

    auto tx = wallet.create_transaction(to_address, amount, fee, blockchain);
    if (!tx) {
        std::cerr << "Error: Failed to create transaction (insufficient funds?)" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Transaction created successfully!" << std::endl;
    std::cout << "TXID: " << tx->get_txid() << std::endl;
    std::cout << "Note: Broadcast to network via RPC or daemon" << std::endl;

    return EXIT_SUCCESS;
}

int cmd_history(const HDWallet& wallet) {
    // Initialize blockchain - in production this would connect to node
    Blockchain blockchain;  // Use default constructor
    auto history = wallet.get_transaction_history(blockchain);

    std::cout << "Transaction History (" << history.size() << " transactions):" << std::endl;
    for (const auto& entry : history) {
        std::cout << "  " << (entry.is_send ? "SEND" : "RECV") << " ";
        std::cout << (entry.amount / static_cast<double>(COIN)) << " INT ";
        std::cout << "(" << entry.confirmations << " confirmations)" << std::endl;
        std::cout << "    Address: " << entry.address << std::endl;
    }

    return EXIT_SUCCESS;
}

int cmd_backup(const HDWallet& wallet, const std::string& backup_file) {
    if (wallet.backup_to_file(backup_file)) {
        std::cout << "Wallet backed up to: " << backup_file << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cerr << "Error: Failed to backup wallet" << std::endl;
        return EXIT_FAILURE;
    }
}

int cmd_show_mnemonic(const HDWallet& wallet) {
    std::cout << "WARNING: Never share your mnemonic phrase with anyone!" << std::endl;
    std::cout << "Anyone with this phrase can access your funds." << std::endl;
    std::cout << std::endl;
    std::cout << "Mnemonic phrase:" << std::endl;
    std::cout << wallet.get_mnemonic() << std::endl;
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    // Default options
    std::string wallet_file;
    std::string password;
    std::string label;

    // Determine default wallet file
    const char* home = std::getenv("HOME");
    if (home) {
        wallet_file = std::string(home) + "/.intcoin/wallet.dat";
    } else {
        wallet_file = ".intcoin/wallet.dat";
    }

    // Parse options
    static struct option long_options[] = {
        {"wallet",   required_argument, 0, 'w'},
        {"password", required_argument, 0, 'p'},
        {"label",    required_argument, 0, 'l'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "w:p:l:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'w':
                wallet_file = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'l':
                label = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check for command
    if (optind >= argc) {
        std::cerr << "Error: No command specified" << std::endl;
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string command = argv[optind];

    // Commands that don't need existing wallet
    if (command == "create") {
        return cmd_create(wallet_file, password);
    } else if (command == "restore") {
        return cmd_restore(wallet_file, password);
    } else if (command == "help") {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    // Load existing wallet for other commands
    HDWallet wallet = HDWallet::restore_from_file(wallet_file, password);

    // Execute command
    if (command == "address") {
        return cmd_new_address(wallet, label);
    } else if (command == "balance") {
        return cmd_balance(wallet);
    } else if (command == "list") {
        return cmd_list_addresses(wallet);
    } else if (command == "send") {
        if (optind + 2 >= argc) {
            std::cerr << "Error: send requires <address> and <amount>" << std::endl;
            return EXIT_FAILURE;
        }
        std::string to_address = argv[optind + 1];
        double amount = std::stod(argv[optind + 2]);
        return cmd_send(wallet, to_address, amount);
    } else if (command == "history") {
        return cmd_history(wallet);
    } else if (command == "backup") {
        if (optind + 1 >= argc) {
            std::cerr << "Error: backup requires <file>" << std::endl;
            return EXIT_FAILURE;
        }
        std::string backup_file = argv[optind + 1];
        return cmd_backup(wallet, backup_file);
    } else if (command == "mnemonic") {
        return cmd_show_mnemonic(wallet);
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
