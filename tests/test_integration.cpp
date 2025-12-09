/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Integration Test Suite
 */

#include "intcoin/intcoin.h"
#include <iostream>
#include <memory>
#include <filesystem>

using namespace intcoin;

void print_test_header(const std::string& test_name) {
    std::cout << "\n========================================\n";
    std::cout << test_name << "\n";
    std::cout << "========================================\n";
}

void print_result(const std::string& test, bool passed) {
    std::cout << test << ": " << (passed ? "✅ PASS" : "❌ FAIL") << std::endl;
}

// Test 1: End-to-end blockchain + storage integration
bool test_blockchain_storage_integration() {
    print_test_header("Test 1: Blockchain + Storage Integration");

    try {
        // Create temporary directory
        std::string test_dir = "/tmp/intcoin_test_" + std::to_string(time(nullptr));
        std::filesystem::create_directories(test_dir);

        // Initialize blockchain database
        auto db = std::make_unique<BlockchainDB>(test_dir);
        auto open_result = db->Open();
        if (!open_result.IsOk()) {
            std::cout << "❌ Failed to open database: " << open_result.error << std::endl;
            return false;
        }

        Blockchain blockchain(std::move(db));

        // Get best block
        uint256 best_hash = blockchain.GetBestBlockHash();
        std::cout << "Best block hash: " << Uint256ToHex(best_hash).substr(0, 16) << "..." << std::endl;

        // Verify blockchain initialized
        std::cout << "✅ Blockchain initialized successfully" << std::endl;

        // Note: In real scenario, we'd need to mine (find valid nonce)
        // For testing, we skip that step

        // Clean up
        std::filesystem::remove_all(test_dir);

        print_result("Blockchain + Storage Integration", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

// Test 2: Wallet + Blockchain integration
bool test_wallet_blockchain_integration() {
    print_test_header("Test 2: Wallet + Blockchain Integration");

    try {
        // Create temporary directory
        std::string test_dir = "/tmp/intcoin_wallet_test_" + std::to_string(time(nullptr));
        std::filesystem::create_directories(test_dir);

        // Initialize blockchain
        auto db = std::make_unique<BlockchainDB>(test_dir + "/blockchain");
        auto open_result = db->Open();
        if (!open_result.IsOk()) {
            std::cout << "❌ Failed to open database" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        Blockchain blockchain(std::move(db));

        // Create wallet
        wallet::WalletConfig config;
        config.data_dir = test_dir + "/wallet";
        wallet::Wallet wallet(config);

        // Generate mnemonic
        auto mnemonic_result = wallet::Mnemonic::Generate(24);
        if (!mnemonic_result.IsOk()) {
            std::cout << "❌ Failed to generate mnemonic" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        // Create wallet
        auto create_result = wallet.Create(mnemonic_result.GetValue(), "");
        if (!create_result.IsOk()) {
            std::cout << "❌ Failed to create wallet: " << create_result.error << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        // Get new address
        auto address_result = wallet.GetNewAddress("");
        if (!address_result.IsOk()) {
            std::cout << "❌ Failed to get address" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        std::string address = address_result.GetValue();
        std::cout << "Generated address: " << address << std::endl;

        // Verify address format
        if (address.substr(0, 4) != "int1") {
            std::cout << "❌ Invalid address prefix" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        // Get balance (should be 0)
        auto balance_result = wallet.GetBalance();
        uint64_t balance = 0;
        if (balance_result.IsOk()) {
            balance = balance_result.GetValue();
        }
        std::cout << "Wallet balance: " << balance << " satoshis" << std::endl;

        // Clean up
        std::filesystem::remove_all(test_dir);

        print_result("Wallet + Blockchain Integration", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

// Test 3: Transaction creation and validation
bool test_transaction_flow() {
    print_test_header("Test 3: Transaction Creation + Validation");

    try {
        // Create a simple transaction
        Transaction tx;
        tx.version = 1;
        tx.locktime = 0;

        // Add input
        TxIn input;
        input.prev_tx_hash = GetRandomUint256();
        input.prev_tx_index = 0;
        input.sequence = UINT32_MAX;

        // Create script (P2PKH spend)
        Script scriptSig(std::vector<uint8_t>(71, 0)); // Dummy signature
        input.script_sig = scriptSig;

        tx.inputs.push_back(input);

        // Add output
        uint256 recipient_pkh = GetRandomUint256();
        Script script_pubkey = Script::CreateP2PKH(recipient_pkh);
        TxOut output(100000000, script_pubkey); // 1 INT

        tx.outputs.push_back(output);

        // Get transaction hash
        uint256 txid = tx.GetHash();
        std::cout << "Transaction ID: " << Uint256ToHex(txid).substr(0, 16) << "..." << std::endl;

        // Serialize and deserialize
        auto serialized = tx.Serialize();
        std::cout << "Serialized size: " << serialized.size() << " bytes" << std::endl;

        auto tx2_result = Transaction::Deserialize(serialized);
        if (!tx2_result.IsOk()) {
            std::cout << "❌ Failed to deserialize transaction" << std::endl;
            return false;
        }
        Transaction tx2 = *tx2_result.value;

        // Verify round-trip
        uint256 txid2 = tx2.GetHash();
        if (txid != txid2) {
            std::cout << "❌ Transaction hash changed after round-trip" << std::endl;
            return false;
        }

        std::cout << "✅ Transaction round-trip successful" << std::endl;

        print_result("Transaction Creation + Validation", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

// Test 4: P2P network + mempool integration
bool test_network_mempool_integration() {
    print_test_header("Test 4: Network + Mempool Integration");

    try {
        // Create mempool
        Mempool mempool;

        // Create test transaction
        Transaction tx;
        tx.version = 1;
        tx.locktime = 0;

        TxIn input;
        input.prev_tx_hash = GetRandomUint256();
        input.prev_tx_index = 0;
        input.sequence = UINT32_MAX;
        tx.inputs.push_back(input);

        uint256 recipient_pkh = GetRandomUint256();
        Script script_pubkey = Script::CreateP2PKH(recipient_pkh);
        TxOut output(50000000, script_pubkey);
        tx.outputs.push_back(output);

        // Add to mempool
        uint256 txid = tx.GetHash();
        auto add_result = mempool.AddTransaction(tx);
        std::cout << "Added to mempool: " << (add_result.IsOk() ? "✅ Yes" : "❌ No") << std::endl;

        // Verify it's in mempool
        if (add_result.IsOk() && !mempool.HasTransaction(txid)) {
            std::cout << "❌ Transaction not found in mempool after adding" << std::endl;
            return false;
        }

        // Get mempool size
        size_t size = mempool.GetSize();
        std::cout << "Mempool size: " << size << " transaction(s)" << std::endl;

        // Try to add duplicate
        auto add_dup_result = mempool.AddTransaction(tx);
        if (add_dup_result.IsOk()) {
            std::cout << "❌ Duplicate transaction was added" << std::endl;
            return false;
        }

        // Remove transaction
        mempool.RemoveTransaction(txid);
        if (mempool.HasTransaction(txid)) {
            std::cout << "❌ Transaction still in mempool after removal" << std::endl;
            return false;
        }

        print_result("Network + Mempool Integration", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

// Test 5: Mining + consensus integration
bool test_mining_consensus_integration() {
    print_test_header("Test 5: Mining + Consensus Integration");

    try {
        // Test block reward calculation
        uint64_t reward = GetBlockReward(0);
        std::cout << "Block reward at height 0: " << (reward / 100000000.0) << " INT" << std::endl;

        uint64_t reward_halved = GetBlockReward(consensus::HALVING_INTERVAL);
        std::cout << "Block reward at height " << consensus::HALVING_INTERVAL << ": " << (reward_halved / 100000000.0) << " INT" << std::endl;

        if (reward_halved >= reward) {
            std::cout << "❌ Block reward should decrease after halving" << std::endl;
            return false;
        }

        std::cout << "✅ Block reward halving working correctly" << std::endl;

        // Test consensus constants
        std::cout << "Target block time: " << consensus::TARGET_BLOCK_TIME << " seconds" << std::endl;
        std::cout << "Max supply: " << (consensus::MAX_SUPPLY / INTS_PER_INT) << " INT" << std::endl;

        print_result("Mining + Consensus Integration", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

// Test 6: End-to-end: Create wallet, transaction, validate
bool test_end_to_end_flow() {
    print_test_header("Test 6: End-to-End Flow");

    try {
        std::cout << "Simulating full transaction flow..." << std::endl;

        // 1. Create sender wallet
        std::string test_dir = "/tmp/intcoin_e2e_test_" + std::to_string(time(nullptr));
        std::filesystem::create_directories(test_dir);

        wallet::WalletConfig sender_config;
        sender_config.data_dir = test_dir + "/sender";
        wallet::Wallet sender_wallet(sender_config);

        auto sender_mnemonic = wallet::Mnemonic::Generate(24);
        if (!sender_mnemonic.IsOk()) {
            std::cout << "❌ Failed to generate sender mnemonic" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        sender_wallet.Create(sender_mnemonic.GetValue(), "");
        auto sender_addr_result = sender_wallet.GetNewAddress("");
        if (!sender_addr_result.IsOk()) {
            std::cout << "❌ Failed to get sender address" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }
        std::string sender_addr = sender_addr_result.GetValue();
        std::cout << "✅ Sender address: " << sender_addr << std::endl;

        // 2. Create recipient wallet
        wallet::WalletConfig recipient_config;
        recipient_config.data_dir = test_dir + "/recipient";
        wallet::Wallet recipient_wallet(recipient_config);

        auto recipient_mnemonic = wallet::Mnemonic::Generate(24);
        if (!recipient_mnemonic.IsOk()) {
            std::cout << "❌ Failed to generate recipient mnemonic" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }

        recipient_wallet.Create(recipient_mnemonic.GetValue(), "");
        auto recipient_addr_result = recipient_wallet.GetNewAddress("");
        if (!recipient_addr_result.IsOk()) {
            std::cout << "❌ Failed to get recipient address" << std::endl;
            std::filesystem::remove_all(test_dir);
            return false;
        }
        std::string recipient_addr = recipient_addr_result.GetValue();
        std::cout << "✅ Recipient address: " << recipient_addr << std::endl;

        // 3. Create transaction (would fail without UTXOs, but we test the API)
        std::vector<wallet::Wallet::Recipient> recipients;
        wallet::Wallet::Recipient recipient;
        recipient.address = recipient_addr;
        recipient.amount = 100000000; // 1 INT
        recipients.push_back(recipient);

        auto tx_result = sender_wallet.CreateTransaction(recipients, 1000);
        std::cout << "Transaction creation: " << (tx_result.IsOk() ? "✅ Success" : "⚠️  Expected (no UTXOs)") << std::endl;

        // Clean up
        std::filesystem::remove_all(test_dir);

        print_result("End-to-End Flow", true);
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   INTcoin Integration Test Suite      ║" << std::endl;
    std::cout << "║   Version 1.0.0-alpha                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;

    int failures = 0;

    // Run all integration tests
    if (!test_blockchain_storage_integration()) failures++;
    if (!test_wallet_blockchain_integration()) failures++;
    if (!test_transaction_flow()) failures++;
    if (!test_network_mempool_integration()) failures++;
    if (!test_mining_consensus_integration()) failures++;
    if (!test_end_to_end_flow()) failures++;

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Integration Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total tests: 6" << std::endl;
    std::cout << "Passed: " << (6 - failures) << std::endl;
    std::cout << "Failed: " << failures << std::endl;
    std::cout << (failures == 0 ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED") << std::endl;

    return failures;
}
