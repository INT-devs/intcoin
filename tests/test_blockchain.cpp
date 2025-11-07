// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/blockchain.h"
#include "intcoin/block.h"
#include "intcoin/transaction.h"
#include "intcoin/crypto.h"
#include "intcoin/consensus.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace intcoin;

// Test utilities
int tests_passed = 0;
int tests_failed = 0;

void test_assert(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "[PASS] " << test_name << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] " << test_name << std::endl;
        tests_failed++;
    }
}

// Test block header creation and hashing
void test_block_header() {
    std::cout << "\n=== Testing Block Header ===" << std::endl;

    BlockHeader header;
    header.version = 1;
    header.height = 100;
    header.timestamp = 1234567890;
    header.nonce = 12345;
    header.bits = consensus::INITIAL_DIFFICULTY;

    // Set previous hash
    std::fill(header.prev_block.begin(), header.prev_block.end(), 0);

    // Set merkle root
    std::fill(header.merkle_root.begin(), header.merkle_root.end(), 1);

    // Calculate hash
    Hash256 hash = header.get_hash();

    test_assert(hash != Hash256{}, "Block header hash is not zero");
    test_assert(header.version == 1, "Block version is correct");
    test_assert(header.height == 100, "Block height is correct");
}

// Test block creation
void test_block_creation() {
    std::cout << "\n=== Testing Block Creation ===" << std::endl;

    Block block;
    block.header.version = 1;
    block.header.height = 1;
    block.header.timestamp = static_cast<uint64_t>(std::time(nullptr));

    test_assert(block.transactions.empty(), "New block has no transactions");
    test_assert(block.header.version == 1, "Block version set correctly");
}

// Test transaction creation and signing
void test_transaction() {
    std::cout << "\n=== Testing Transaction ===" << std::endl;

    // Generate keypair for signing
    auto keypair = crypto::Dilithium::generate_keypair();

    // Create transaction
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Add input
    TxInput input;
    std::fill(input.prev_tx_hash.begin(), input.prev_tx_hash.end(), 1);
    input.prev_output_index = 0;
    input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(input);

    // Add output
    TxOutput output;
    output.amount = 100 * COIN;
    output.recipient = keypair.public_key;
    tx.outputs.push_back(output);

    // Calculate hash
    Hash256 tx_hash = tx.get_hash();

    test_assert(tx_hash != Hash256{}, "Transaction hash is not zero");
    test_assert(tx.inputs.size() == 1, "Transaction has 1 input");
    test_assert(tx.outputs.size() == 1, "Transaction has 1 output");
    test_assert(tx.outputs[0].amount == 100 * COIN, "Output amount is correct");
}

// Test coinbase transaction
void test_coinbase_transaction() {
    std::cout << "\n=== Testing Coinbase Transaction ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();

    Transaction coinbase = create_coinbase_transaction(keypair.public_key, 0, 0);

    test_assert(coinbase.is_coinbase(), "Transaction is coinbase");
    test_assert(coinbase.inputs.size() == 1, "Coinbase has 1 input");
    test_assert(coinbase.outputs.size() == 1, "Coinbase has 1 output");
    test_assert(coinbase.inputs[0].prev_output_index == 0xFFFFFFFF, "Coinbase input index is max");
    test_assert(coinbase.outputs[0].amount == consensus::get_block_subsidy(0), "Coinbase reward is correct");
}

// Test genesis block
void test_genesis_block() {
    std::cout << "\n=== Testing Genesis Block ===" << std::endl;

    Block genesis = create_genesis_block();

    test_assert(genesis.header.height == 0, "Genesis block height is 0");
    test_assert(genesis.header.version == 1, "Genesis version is 1");
    test_assert(genesis.transactions.size() == 1, "Genesis has 1 transaction");
    test_assert(genesis.transactions[0].is_coinbase(), "Genesis transaction is coinbase");

    // Check previous hash is zero
    bool prev_is_zero = true;
    for (uint8_t b : genesis.header.prev_block) {
        if (b != 0) {
            prev_is_zero = false;
            break;
        }
    }
    test_assert(prev_is_zero, "Genesis previous hash is zero");
}

// Test block serialization
void test_block_serialization() {
    std::cout << "\n=== Testing Block Serialization ===" << std::endl;

    Block block = create_genesis_block();

    // Serialize
    std::vector<uint8_t> serialized = block.serialize();
    test_assert(!serialized.empty(), "Block serialization produces data");
    test_assert(serialized.size() > 100, "Serialized block has reasonable size");

    // Deserialize
    Block deserialized = Block::deserialize(serialized);
    test_assert(deserialized.header.height == block.header.height, "Deserialized height matches");
    test_assert(deserialized.header.version == block.header.version, "Deserialized version matches");
    test_assert(deserialized.transactions.size() == block.transactions.size(), "Transaction count matches");
}

// Test transaction serialization
void test_transaction_serialization() {
    std::cout << "\n=== Testing Transaction Serialization ===" << std::endl;

    auto keypair = crypto::Dilithium::generate_keypair();
    Transaction tx = create_coinbase_transaction(keypair.public_key, 0, 100);

    // Serialize
    std::vector<uint8_t> serialized = tx.serialize();
    test_assert(!serialized.empty(), "Transaction serialization produces data");

    // Deserialize
    Transaction deserialized = Transaction::deserialize(serialized);
    test_assert(deserialized.version == tx.version, "Deserialized version matches");
    test_assert(deserialized.inputs.size() == tx.inputs.size(), "Input count matches");
    test_assert(deserialized.outputs.size() == tx.outputs.size(), "Output count matches");
}

// Test merkle tree construction
void test_merkle_tree() {
    std::cout << "\n=== Testing Merkle Tree ===" << std::endl;

    // Create some transactions
    std::vector<Transaction> txs;
    auto keypair = crypto::Dilithium::generate_keypair();

    for (int i = 0; i < 4; i++) {
        txs.push_back(create_coinbase_transaction(keypair.public_key, i, i * 100));
    }

    // Build merkle tree
    Block block;
    block.transactions = txs;
    Hash256 merkle_root = block.calculate_merkle_root();

    test_assert(merkle_root != Hash256{}, "Merkle root is not zero");

    // Empty block should have zero merkle root
    Block empty_block;
    Hash256 empty_merkle = empty_block.calculate_merkle_root();
    test_assert(empty_merkle == Hash256{}, "Empty block has zero merkle root");

    // Single transaction
    Block single_tx_block;
    single_tx_block.transactions.push_back(txs[0]);
    Hash256 single_merkle = single_tx_block.calculate_merkle_root();
    test_assert(single_merkle == txs[0].get_hash(), "Single tx merkle equals tx hash");
}

// Test blockchain initialization
void test_blockchain_init() {
    std::cout << "\n=== Testing Blockchain Initialization ===" << std::endl;

    Blockchain blockchain(":memory:");  // Use in-memory database

    test_assert(blockchain.get_height() == 0, "Blockchain starts at height 0");
    test_assert(blockchain.get_block_count() == 1, "Blockchain has genesis block");

    Hash256 best_hash = blockchain.get_best_block_hash();
    test_assert(best_hash != Hash256{}, "Best block hash exists");
}

// Test block addition
void test_block_addition() {
    std::cout << "\n=== Testing Block Addition ===" << std::endl;

    Blockchain blockchain(":memory:");
    auto keypair = crypto::Dilithium::generate_keypair();

    // Get genesis
    Hash256 genesis_hash = blockchain.get_best_block_hash();

    // Create next block
    Block next_block;
    next_block.header.version = 1;
    next_block.header.height = 1;
    next_block.header.prev_block = genesis_hash;
    next_block.header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    next_block.header.bits = consensus::INITIAL_DIFFICULTY;

    // Add coinbase
    next_block.transactions.push_back(
        create_coinbase_transaction(keypair.public_key, 1, 0)
    );

    // Calculate merkle root
    next_block.header.merkle_root = next_block.calculate_merkle_root();

    // Add block
    bool added = blockchain.add_block(next_block);
    test_assert(added, "Block added successfully");
    test_assert(blockchain.get_height() == 1, "Blockchain height increased");
    test_assert(blockchain.get_block_count() == 2, "Block count increased");
}

// Test block validation
void test_block_validation() {
    std::cout << "\n=== Testing Block Validation ===" << std::endl;

    Blockchain blockchain(":memory:");
    auto keypair = crypto::Dilithium::generate_keypair();

    // Valid block
    Block valid_block = create_genesis_block();
    test_assert(blockchain.validate_block(valid_block), "Genesis block is valid");

    // Invalid block - wrong height
    Block invalid_height_block = valid_block;
    invalid_height_block.header.height = 999;
    test_assert(!blockchain.validate_block(invalid_height_block), "Invalid height rejected");

    // Invalid block - no coinbase
    Block no_coinbase_block;
    no_coinbase_block.header.version = 1;
    no_coinbase_block.header.height = 1;
    test_assert(!blockchain.validate_block(no_coinbase_block), "Block without coinbase rejected");
}

// Test transaction validation
void test_transaction_validation() {
    std::cout << "\n=== Testing Transaction Validation ===" << std::endl;

    Blockchain blockchain(":memory:");
    auto keypair = crypto::Dilithium::generate_keypair();

    // Valid coinbase
    Transaction valid_coinbase = create_coinbase_transaction(keypair.public_key, 0, 0);
    test_assert(blockchain.validate_transaction(valid_coinbase), "Valid coinbase accepted");

    // Invalid - double spend (would need UTXO set for full test)
    Transaction double_spend = valid_coinbase;
    // This is a simplified test - full double-spend detection requires UTXO tracking
}

// Test difficulty adjustment
void test_difficulty_adjustment() {
    std::cout << "\n=== Testing Difficulty Adjustment ===" << std::endl;

    uint32_t initial = consensus::INITIAL_DIFFICULTY;
    test_assert(initial > 0, "Initial difficulty is set");

    // Test difficulty remains stable with target block time
    uint64_t actual_time = consensus::DIFFICULTY_ADJUSTMENT_INTERVAL * consensus::TARGET_BLOCK_TIME;
    uint32_t new_diff = consensus::adjust_difficulty(initial, actual_time);
    test_assert(new_diff == initial, "Difficulty stable at target time");

    // Test difficulty increases when blocks too fast
    uint64_t fast_time = actual_time / 2;
    uint32_t harder = consensus::adjust_difficulty(initial, fast_time);
    test_assert(harder > initial, "Difficulty increases when blocks too fast");

    // Test difficulty decreases when blocks too slow
    uint64_t slow_time = actual_time * 2;
    uint32_t easier = consensus::adjust_difficulty(initial, slow_time);
    test_assert(easier < initial, "Difficulty decreases when blocks too slow");
}

// Test block subsidy halving
void test_block_subsidy() {
    std::cout << "\n=== Testing Block Subsidy ===" << std::endl;

    uint64_t genesis_subsidy = consensus::get_block_subsidy(0);
    test_assert(genesis_subsidy == consensus::INITIAL_BLOCK_REWARD, "Genesis subsidy is initial reward");

    // First halving
    uint64_t first_halving = consensus::get_block_subsidy(consensus::HALVING_INTERVAL);
    test_assert(first_halving == genesis_subsidy / 2, "First halving reduces reward by half");

    // Second halving
    uint64_t second_halving = consensus::get_block_subsidy(consensus::HALVING_INTERVAL * 2);
    test_assert(second_halving == genesis_subsidy / 4, "Second halving reduces reward to quarter");

    // Eventually goes to zero
    uint64_t far_future = consensus::get_block_subsidy(consensus::HALVING_INTERVAL * 100);
    test_assert(far_future == 0, "Far future subsidy is zero");
}

// Test chain reorganization (basic)
void test_chain_reorg() {
    std::cout << "\n=== Testing Chain Reorganization ===" << std::endl;

    Blockchain blockchain(":memory:");

    // This would require building competing chains
    // Simplified test for now
    test_assert(blockchain.get_height() == 0, "Blockchain at genesis");
}

// Test UTXO operations
void test_utxo_operations() {
    std::cout << "\n=== Testing UTXO Operations ===" << std::endl;

    Blockchain blockchain(":memory:");
    auto keypair = crypto::Dilithium::generate_keypair();

    // Get genesis coinbase UTXO
    auto genesis = blockchain.get_block_by_height(0);
    if (genesis && !genesis->transactions.empty()) {
        Hash256 tx_hash = genesis->transactions[0].get_hash();

        // Check if UTXO exists
        auto utxo = blockchain.get_utxo(tx_hash, 0);
        test_assert(utxo.has_value(), "Genesis coinbase UTXO exists");

        if (utxo) {
            test_assert(utxo->amount == consensus::get_block_subsidy(0), "UTXO amount matches subsidy");
        }
    }
}

// Test block retrieval
void test_block_retrieval() {
    std::cout << "\n=== Testing Block Retrieval ===" << std::endl;

    Blockchain blockchain(":memory:");

    // Get genesis by height
    auto genesis_by_height = blockchain.get_block_by_height(0);
    test_assert(genesis_by_height.has_value(), "Genesis retrieved by height");

    // Get genesis by hash
    Hash256 genesis_hash = blockchain.get_best_block_hash();
    auto genesis_by_hash = blockchain.get_block_by_hash(genesis_hash);
    test_assert(genesis_by_hash.has_value(), "Genesis retrieved by hash");

    // Invalid height
    auto invalid = blockchain.get_block_by_height(9999);
    test_assert(!invalid.has_value(), "Invalid height returns nullopt");
}

// Main test runner
int main() {
    std::cout << "INTcoin Blockchain Test Suite" << std::endl;
    std::cout << "==============================" << std::endl;

    // Run all tests
    test_block_header();
    test_block_creation();
    test_transaction();
    test_coinbase_transaction();
    test_genesis_block();
    test_block_serialization();
    test_transaction_serialization();
    test_merkle_tree();
    test_blockchain_init();
    test_block_addition();
    test_block_validation();
    test_transaction_validation();
    test_difficulty_adjustment();
    test_block_subsidy();
    test_chain_reorg();
    test_utxo_operations();
    test_block_retrieval();

    // Summary
    std::cout << "\n==============================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "==============================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
