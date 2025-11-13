// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Genesis Block Mining Tool
//
// This tool mines the official INTcoin mainnet genesis block by finding
// a valid nonce that satisfies the initial difficulty target.

#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include "intcoin/primitives.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace intcoin;
using namespace std::chrono;

// ANSI color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

void print_hash(const Hash256& hash) {
    for (size_t i = 0; i < hash.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(hash[i]);
    }
    std::cout << std::dec;
}

void print_header() {
    std::cout << BOLD << MAGENTA
              << "\n========================================\n"
              << "    INTcoin Genesis Block Miner\n"
              << "========================================\n"
              << RESET << std::endl;
}

void print_genesis_info(const Block& genesis) {
    std::cout << CYAN << "\nGenesis Block Configuration:" << RESET << std::endl;
    std::cout << "  Version:           " << genesis.header.version << std::endl;
    std::cout << "  Timestamp:         " << genesis.header.timestamp
              << " (Jan 1, 2025 00:00:00 UTC)" << std::endl;
    std::cout << "  Difficulty (bits): 0x" << std::hex << genesis.header.bits << std::dec << std::endl;
    std::cout << "  Merkle Root:       ";
    print_hash(genesis.header.merkle_root);
    std::cout << std::endl;
    std::cout << "  Message:           \"The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins\"" << std::endl;
    std::cout << std::endl;
}

bool is_valid_proof_of_work(const Hash256& hash, uint32_t bits) {
    // Convert bits to target
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00FFFFFF;

    // Target = mantissa * 2^(8 * (exponent - 3))
    Hash256 target{};
    if (exponent <= 3) {
        uint32_t shifted = mantissa >> (8 * (3 - exponent));
        target[31] = shifted & 0xFF;
        target[30] = (shifted >> 8) & 0xFF;
        target[29] = (shifted >> 16) & 0xFF;
    } else {
        size_t shift_bytes = exponent - 3;
        if (shift_bytes < 29) {
            target[31 - shift_bytes] = mantissa & 0xFF;
            target[31 - shift_bytes - 1] = (mantissa >> 8) & 0xFF;
            target[31 - shift_bytes - 2] = (mantissa >> 16) & 0xFF;
        }
    }

    // Check if hash <= target (comparing as big-endian)
    for (size_t i = 0; i < 32; ++i) {
        if (hash[i] < target[i]) {
            return true;
        }
        if (hash[i] > target[i]) {
            return false;
        }
    }

    return true;
}

void mine_genesis_block(Block& genesis) {
    print_header();
    print_genesis_info(genesis);

    std::cout << YELLOW << "Mining genesis block..." << RESET << std::endl;
    std::cout << "Target difficulty: 0x" << std::hex << genesis.header.bits << std::dec << std::endl;
    std::cout << std::endl;

    uint64_t nonce = 0;
    uint64_t hashes = 0;
    auto start_time = high_resolution_clock::now();
    auto last_update = start_time;

    while (true) {
        genesis.header.nonce = nonce;

        // Calculate hash using SHA-256 PoW (same as Block::check_proof_of_work)
        std::vector<uint8_t> serialized = genesis.header.serialize();
        Hash256 block_hash = crypto::SHA256_PoW::hash(serialized.data(), serialized.size());

        hashes++;

        // Check if valid
        if (is_valid_proof_of_work(block_hash, genesis.header.bits)) {
            auto end_time = high_resolution_clock::now();
            auto duration = duration_cast<seconds>(end_time - start_time);

            std::cout << GREEN << BOLD << "\n✓ Genesis block mined successfully!" << RESET << std::endl;
            std::cout << std::endl;
            std::cout << CYAN << "Results:" << RESET << std::endl;
            std::cout << "  Nonce:        " << BOLD << nonce << RESET << std::endl;
            std::cout << "  Block Hash:   ";
            print_hash(block_hash);
            std::cout << std::endl;
            std::cout << "  Hashes:       " << hashes << std::endl;
            std::cout << "  Time:         " << duration.count() << " seconds" << std::endl;
            std::cout << "  Hash Rate:    " << (hashes / std::max(duration.count(), 1LL)) << " H/s" << std::endl;
            std::cout << std::endl;

            return;
        }

        // Progress update every second
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - last_update).count() >= 1000) {
            auto elapsed = duration_cast<seconds>(now - start_time).count();
            double hash_rate = hashes / std::max(elapsed, 1LL);

            std::cout << "\r" << YELLOW << "Mining... " << RESET
                      << "Nonce: " << std::setw(12) << nonce
                      << " | Hashes: " << std::setw(12) << hashes
                      << " | Rate: " << std::setw(8) << std::fixed << std::setprecision(0)
                      << hash_rate << " H/s" << std::flush;

            last_update = now;
        }

        nonce++;

        // Safety check: stop after 2^32 attempts
        if (nonce == 0) {
            std::cout << RED << "\n\nError: Failed to find valid nonce after 2^64 attempts!" << RESET << std::endl;
            std::cout << "The difficulty may be too high, or there may be a bug in the mining logic." << std::endl;
            return;
        }
    }
}

void print_c_code(const Block& genesis) {
    std::cout << CYAN << "\nC++ Code for block.cpp:" << RESET << std::endl;
    std::cout << BLUE << "========================================" << RESET << std::endl;

    std::cout << "Block GenesisBlock::create_mainnet() {\n";
    std::cout << "    const std::string message = \"The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins\";\n";
    std::cout << "    const uint64_t timestamp = " << genesis.header.timestamp << ";  // January 1, 2025 00:00:00 UTC\n";
    std::cout << "    const uint64_t nonce = " << genesis.header.nonce << "ULL;  // Mined nonce\n";
    std::cout << "    const uint32_t bits = 0x" << std::hex << genesis.header.bits << ";  // Initial difficulty\n" << std::dec;
    std::cout << "\n";
    std::cout << "    return create_genesis(message, timestamp, nonce, bits);\n";
    std::cout << "}\n";

    std::cout << BLUE << "========================================" << RESET << std::endl;
}

int main() {
    try {
        // Create genesis block with nonce 0
        Block genesis = GenesisBlock::create_mainnet();

        // Mine it
        mine_genesis_block(genesis);

        // Print code to update
        print_c_code(genesis);

        // Verify final block
        std::cout << GREEN << "\nVerification:" << RESET << std::endl;
        if (genesis.header.check_proof_of_work()) {
            std::cout << "  " << GREEN << "✓ Proof of work valid" << RESET << std::endl;
        } else {
            std::cout << "  " << RED << "✗ Proof of work INVALID" << RESET << std::endl;
        }

        Hash256 final_hash = genesis.get_hash();
        std::cout << "  Block Hash: ";
        print_hash(final_hash);
        std::cout << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
        return 1;
    }
}
