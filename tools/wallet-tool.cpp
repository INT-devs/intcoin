// Copyright (c) 2025 INTcoin Developers
// Wallet management utility

#include "intcoin/intcoin.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace intcoin;
using namespace intcoin::wallet;

void print_usage() {
    std::cout << "INTcoin Wallet Tool v" << INTCOIN_VERSION_MAJOR << "."
              << INTCOIN_VERSION_MINOR << "." << INTCOIN_VERSION_PATCH << "\n\n";
    std::cout << "Usage: wallet-tool [options] <command>\n\n";
    std::cout << "Commands:\n";
    std::cout << "  create              Create new wallet\n";
    std::cout << "  info                Show wallet information\n";
    std::cout << "  newaddress [label]  Generate new address\n";
    std::cout << "  listaddresses       List all addresses\n";
    std::cout << "  showmnemonic        Display recovery phrase (KEEP SECRET!)\n\n";
    std::cout << "Options:\n";
    std::cout << "  -datadir=<dir>      Wallet data directory (default: ./wallet)\n";
    std::cout << "  -testnet            Use testnet wallet directory\n";
    std::cout << "  -h, --help          Show this help\n";
}

int main(int argc, char* argv[]) {
    std::string data_dir = "./wallet";
    std::string command;

    // Parse arguments
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        }
        else if (arg.find("-datadir=") == 0) {
            data_dir = arg.substr(9);
        }
        else if (arg == "-testnet") {
            data_dir = "./wallet_testnet";
        }
        else if (command.empty()) {
            command = arg;
        }
        else {
            args.push_back(arg);
        }
    }

    if (command.empty()) {
        std::cerr << "Error: No command specified\n";
        print_usage();
        return 1;
    }

    // Setup wallet configuration
    WalletConfig config;
    config.data_dir = data_dir;
    Wallet wallet(config);

    // Check if wallet exists
    bool wallet_exists = std::filesystem::exists(data_dir + "/wallet.db");

    // Handle commands
    if (command == "create") {
        if (wallet_exists) {
            std::cerr << "Error: Wallet already exists at " << data_dir << "\n";
            std::cerr << "Remove it first if you want to create a new wallet.\n";
            return 1;
        }

        std::cout << "Creating new wallet...\n\n";

        // Generate mnemonic
        auto mnemonic_result = Mnemonic::Generate(24);
        if (!mnemonic_result.IsOk()) {
            std::cerr << "ERROR: Failed to generate mnemonic: "
                      << mnemonic_result.error << "\n";
            return 1;
        }

        auto mnemonic = mnemonic_result.value.value();

        // Create wallet
        auto create_result = wallet.Create(mnemonic);
        if (!create_result.IsOk()) {
            std::cerr << "ERROR: Failed to create wallet: "
                      << create_result.error << "\n";
            return 1;
        }

        std::cout << "✓ Wallet created successfully!\n\n";
        std::cout << "IMPORTANT: Write down your recovery phrase and keep it safe!\n";
        std::cout << "Anyone with this phrase can access your funds.\n\n";
        std::cout << "Recovery Phrase (24 words):\n";
        std::cout << "─────────────────────────────────────────────────────\n";
        for (size_t i = 0; i < mnemonic.size(); i++) {
            std::cout << (i + 1) << ". " << mnemonic[i] << "\n";
        }
        std::cout << "─────────────────────────────────────────────────────\n\n";

        // Generate first address
        auto addr_result = wallet.GetNewAddress("default");
        if (addr_result.IsOk()) {
            std::cout << "First address: " << addr_result.value.value() << "\n";
        }

        std::cout << "\nWallet location: " << data_dir << "\n";
    }
    else if (command == "info") {
        if (!wallet_exists) {
            std::cerr << "Error: No wallet found at " << data_dir << "\n";
            std::cerr << "Create one with: wallet-tool create\n";
            return 1;
        }

        auto load_result = wallet.Load();
        if (!load_result.IsOk()) {
            std::cerr << "ERROR: Failed to load wallet: "
                      << load_result.error << "\n";
            return 1;
        }

        auto info_result = wallet.GetInfo();
        if (!info_result.IsOk()) {
            std::cerr << "ERROR: Failed to get wallet info: "
                      << info_result.error << "\n";
            return 1;
        }

        auto info = info_result.value.value();

        std::cout << "Wallet Information\n";
        std::cout << "══════════════════════════════════════════════════════\n";
        std::cout << "Balance:              " << info.balance << " satoshis\n";
        std::cout << "Unconfirmed Balance:  " << info.unconfirmed_balance << " satoshis\n";
        std::cout << "Addresses:            " << info.address_count << "\n";
        std::cout << "Transactions:         " << info.transaction_count << "\n";
        std::cout << "UTXOs:                " << info.utxo_count << "\n";
        std::cout << "Encrypted:            " << (info.encrypted ? "Yes" : "No") << "\n";
        std::cout << "Locked:               " << (info.locked ? "Yes" : "No") << "\n";
        std::cout << "Keypool Size:         " << info.keypool_size << "\n";
        std::cout << "══════════════════════════════════════════════════════\n";
        std::cout << "Location: " << data_dir << "\n";
    }
    else if (command == "newaddress") {
        if (!wallet_exists) {
            std::cerr << "Error: No wallet found. Create one with: wallet-tool create\n";
            return 1;
        }

        auto load_result = wallet.Load();
        if (!load_result.IsOk()) {
            std::cerr << "ERROR: Failed to load wallet: " << load_result.error << "\n";
            return 1;
        }

        std::string label = args.empty() ? "" : args[0];
        auto addr_result = wallet.GetNewAddress(label);
        if (!addr_result.IsOk()) {
            std::cerr << "ERROR: Failed to generate address: "
                      << addr_result.error << "\n";
            return 1;
        }

        std::cout << "New address: " << addr_result.value.value() << "\n";
        if (!label.empty()) {
            std::cout << "Label: " << label << "\n";
        }
    }
    else if (command == "listaddresses") {
        if (!wallet_exists) {
            std::cerr << "Error: No wallet found. Create one with: wallet-tool create\n";
            return 1;
        }

        auto load_result = wallet.Load();
        if (!load_result.IsOk()) {
            std::cerr << "ERROR: Failed to load wallet: " << load_result.error << "\n";
            return 1;
        }

        auto addresses_result = wallet.GetAddresses();
        if (!addresses_result.IsOk()) {
            std::cerr << "ERROR: Failed to get addresses: "
                      << addresses_result.error << "\n";
            return 1;
        }

        auto addresses = addresses_result.value.value();

        std::cout << "Wallet Addresses (" << addresses.size() << "):\n";
        std::cout << "══════════════════════════════════════════════════════\n";
        for (const auto& addr : addresses) {
            std::cout << addr.address;
            if (!addr.label.empty()) {
                std::cout << " (" << addr.label << ")";
            }
            if (addr.is_change) {
                std::cout << " [change]";
            }
            std::cout << "\n";
        }
    }
    else if (command == "showmnemonic") {
        if (!wallet_exists) {
            std::cerr << "Error: No wallet found. Create one with: wallet-tool create\n";
            return 1;
        }

        auto load_result = wallet.Load();
        if (!load_result.IsOk()) {
            std::cerr << "ERROR: Failed to load wallet: " << load_result.error << "\n";
            return 1;
        }

        auto mnemonic_result = wallet.GetMnemonic();
        if (!mnemonic_result.IsOk()) {
            std::cerr << "ERROR: Failed to get mnemonic: "
                      << mnemonic_result.error << "\n";
            return 1;
        }

        auto mnemonic = mnemonic_result.value.value();

        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════╗\n";
        std::cout << "║         WARNING: KEEP THIS PHRASE SECRET!         ║\n";
        std::cout << "║  Anyone with this phrase can steal your funds!    ║\n";
        std::cout << "╚════════════════════════════════════════════════════╝\n\n";

        std::cout << "Recovery Phrase (24 words):\n";
        std::cout << "─────────────────────────────────────────────────────\n";
        for (size_t i = 0; i < mnemonic.size(); i++) {
            std::cout << (i + 1) << ". " << mnemonic[i] << "\n";
        }
        std::cout << "─────────────────────────────────────────────────────\n\n";
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        print_usage();
        return 1;
    }

    return 0;
}
