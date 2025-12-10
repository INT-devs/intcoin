/*
 * Genesis Block Miner
 * Mines the INTcoin genesis block and outputs values to hardcode
 */

#include "intcoin/block.h"
#include "intcoin/consensus.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <randomx.h>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace intcoin;

int main() {
    std::cout << "========================================\n";
    std::cout << "INTcoin Genesis Block Miner\n";
    std::cout << "========================================\n\n";

    // Initialize RandomX
    std::cout << "Initializing RandomX...\n";
    auto init_result = RandomXValidator::Initialize();
    if (init_result.IsError()) {
        std::cerr << "Failed to initialize RandomX: " << init_result.error << "\n";
        return 1;
    }

    // Create genesis block structure (same as CreateGenesisBlock but with mining)
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = uint256(); // Zero hash
    header.timestamp = 1732627080; // 26 November 2025 13:18:00 UTC
    header.bits = consensus::MIN_DIFFICULTY_BITS;
    header.nonce = 0; // Will be mined

    // Get RandomX key for height 0
    header.randomx_key = RandomXValidator::GetRandomXKey(0);

    // Create coinbase transaction with genesis message
    std::string genesis_message = "13:18, 26 November 2025 This Is Money, Financial markets in turmoil as Budget leak fiasco sends pound and gilts on rollercoaster ride";

    Transaction coinbase;
    coinbase.version = 1;

    // Coinbase input with genesis message
    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256();
    coinbase_input.prev_tx_index = 0xFFFFFFFF;
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase_input.script_sig = Script(std::vector<uint8_t>(genesis_message.begin(), genesis_message.end()));
    coinbase.inputs.push_back(coinbase_input);

    // Coinbase output to placeholder address
    PublicKey genesis_pubkey; // Zero pubkey
    uint256 pubkey_hash = PublicKeyToHash(genesis_pubkey);
    Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
    TxOut coinbase_output(consensus::INITIAL_BLOCK_REWARD, script_pubkey);
    coinbase.outputs.push_back(coinbase_output);

    coinbase.locktime = 0;

    // Calculate merkle root
    std::vector<uint256> tx_hashes;
    tx_hashes.push_back(coinbase.GetHash());
    header.merkle_root = CalculateMerkleRoot(tx_hashes);

    // Display pre-mining information
    std::cout << "Pre-mining values:\n";
    std::cout << "  Version:        " << header.version << "\n";
    std::cout << "  Timestamp:      " << header.timestamp << " (26 Nov 2025 13:18:00 UTC)\n";
    std::cout << "  Bits:           0x" << std::hex << header.bits << std::dec << "\n";
    std::cout << "  Prev Hash:      " << ToHex(header.prev_block_hash) << "\n";
    std::cout << "  Merkle Root:    " << ToHex(header.merkle_root) << "\n";
    std::cout << "  RandomX Key:    " << ToHex(header.randomx_key) << "\n";
    std::cout << "  Coinbase Value: " << coinbase_output.value << " INTS\n";
    std::cout << "  Genesis Msg:    \"" << genesis_message << "\"\n\n";

    // Calculate difficulty target
    uint256 target = DifficultyCalculator::CompactToTarget(header.bits);
    std::cout << "Target:         " << ToHex(target) << "\n\n";

    // Start mining
    std::cout << "Mining genesis block...\n";
    std::cout << "(This may take a while with minimum difficulty)\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t hash_count = 0;
    uint64_t nonce = 0;
    uint256 block_hash{};

    bool found = false;
    while (!found && nonce < consensus::MAX_NONCE) {
        header.nonce = nonce;

        // Calculate RandomX hash
        auto hash_result = RandomXValidator::CalculateHash(header);
        if (hash_result.IsError()) {
            std::cerr << "Error calculating hash: " << hash_result.error << "\n";
            return 1;
        }

        block_hash = *hash_result.value;
        hash_count++;

        // Check if hash meets target
        if (DifficultyCalculator::CheckProofOfWork(block_hash, header.bits)) {
            found = true;
            break;
        }

        // Progress update every 10,000 hashes
        if (hash_count % 10000 == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            double hashrate = (elapsed > 0) ? (double)hash_count / elapsed : 0.0;

            std::cout << "Hashes: " << hash_count
                      << " | Hashrate: " << std::fixed << std::setprecision(2) << hashrate << " H/s"
                      << " | Nonce: " << nonce << "      \r" << std::flush;
        }

        nonce++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    if (!found) {
        std::cerr << "\n\nFailed to find valid block hash!\n";
        RandomXValidator::Shutdown();
        return 1;
    }

    std::cout << "\n\n========================================\n";
    std::cout << "✓ Genesis block mined successfully!\n";
    std::cout << "========================================\n\n";

    std::cout << "Mining Statistics:\n";
    std::cout << "  Total Hashes:   " << hash_count << "\n";
    std::cout << "  Time Elapsed:   " << elapsed << " seconds\n";
    std::cout << "  Average Rate:   " << std::fixed << std::setprecision(2)
              << (elapsed > 0 ? (double)hash_count / elapsed : 0.0) << " H/s\n\n";

    std::cout << "========================================\n";
    std::cout << "VALUES TO HARDCODE:\n";
    std::cout << "========================================\n\n";

    std::cout << "// In src/blockchain/block.cpp - CreateGenesisBlock():\n\n";

    std::cout << "header.nonce = " << header.nonce << "ULL;\n";
    std::cout << "header.randomx_key = uint256(\"" << ToHex(header.randomx_key) << "\");\n";
    std::cout << "header.merkle_root = uint256(\"" << ToHex(header.merkle_root) << "\");\n\n";

    std::cout << "// Expected block hash:\n";
    std::cout << "// " << ToHex(block_hash) << "\n\n";

    std::cout << "========================================\n";
    std::cout << "Verification:\n";
    std::cout << "========================================\n\n";

    std::cout << "Block Hash:     " << ToHex(block_hash) << "\n";
    std::cout << "Target:         " << ToHex(target) << "\n";
    std::cout << "Hash < Target:  " << (DifficultyCalculator::CheckProofOfWork(block_hash, header.bits) ? "YES ✓" : "NO ✗") << "\n";
    std::cout << "Difficulty:     " << DifficultyCalculator::GetDifficulty(header.bits) << "\n\n";

    // Cleanup
    RandomXValidator::Shutdown();

    std::cout << "========================================\n";
    std::cout << "Done! Copy the values above into the\n";
    std::cout << "CreateGenesisBlock() function.\n";
    std::cout << "========================================\n";

    return 0;
}
