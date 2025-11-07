// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/primitives.h"
#include <iostream>
#include <cassert>
#include <filesystem>

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

// Test wallet creation
void test_wallet_creation() {
    std::cout << "\n=== Testing Wallet Creation ===" << std::endl;

    Wallet wallet(":memory:");

    test_assert(!wallet.is_encrypted(), "New wallet is not encrypted");
    test_assert(wallet.get_key_count() == 0, "New wallet has no keys");
}

// Test key generation
void test_key_generation() {
    std::cout << "\n=== Testing Key Generation ===" << std::endl;

    Wallet wallet(":memory:");

    // Generate key
    crypto::DilithiumPubKey pubkey = wallet.generate_new_key();

    test_assert(wallet.get_key_count() == 1, "Wallet has 1 key after generation");

    bool has_key = false;
    for (uint8_t b : pubkey) {
        if (b != 0) {
            has_key = true;
            break;
        }
    }
    test_assert(has_key, "Generated key is not zero");
}

// Test mnemonic generation
void test_mnemonic_generation() {
    std::cout << "\n=== Testing Mnemonic Generation ===" << std::endl;

    Wallet wallet(":memory:");

    std::string mnemonic = wallet.generate_mnemonic();

    test_assert(!mnemonic.empty(), "Mnemonic is not empty");

    // Count words (should be 12 or 24)
    size_t word_count = 1;
    for (char c : mnemonic) {
        if (c == ' ') word_count++;
    }

    test_assert(word_count == 12 || word_count == 24, "Mnemonic has correct word count");
}

// Test wallet from mnemonic
void test_wallet_from_mnemonic() {
    std::cout << "\n=== Testing Wallet from Mnemonic ===" << std::endl;

    Wallet wallet1(":memory:");
    std::string mnemonic = wallet1.generate_mnemonic();

    // Create second wallet from same mnemonic
    Wallet wallet2(":memory:");
    bool restored = wallet2.restore_from_mnemonic(mnemonic);

    test_assert(restored, "Wallet restored from mnemonic");
}

// Test wallet encryption
void test_wallet_encryption() {
    std::cout << "\n=== Testing Wallet Encryption ===" << std::endl;

    Wallet wallet(":memory:");
    wallet.generate_new_key();

    std::string password = "test_password_123";

    // Encrypt
    bool encrypted = wallet.encrypt(password);
    test_assert(encrypted, "Wallet encrypted successfully");
    test_assert(wallet.is_encrypted(), "Wallet is encrypted");

    // Unlock
    bool unlocked = wallet.unlock(password);
    test_assert(unlocked, "Wallet unlocked successfully");

    // Wrong password
    bool wrong_unlock = wallet.unlock("wrong_password");
    test_assert(!wrong_unlock, "Wrong password rejected");

    // Lock
    wallet.lock();
    test_assert(wallet.is_locked(), "Wallet is locked");
}

// Test address generation
void test_address_generation() {
    std::cout << "\n=== Testing Address Generation ===" << std::endl;

    Wallet wallet(":memory:");
    crypto::DilithiumPubKey pubkey = wallet.generate_new_key();

    std::string address = wallet.pubkey_to_address(pubkey);

    test_assert(!address.empty(), "Address is not empty");
    test_assert(address.length() > 20, "Address has reasonable length");
    test_assert(address[0] == 'I' || address[0] == 'i', "Address starts with 'I'");
}

// Test transaction creation
void test_transaction_creation() {
    std::cout << "\n=== Testing Transaction Creation ===" << std::endl;

    Wallet wallet(":memory:");
    crypto::DilithiumPubKey pubkey = wallet.generate_new_key();

    // Create a simple transaction
    std::vector<TxInput> inputs;
    TxInput input;
    std::fill(input.prev_tx_hash.begin(), input.prev_tx_hash.end(), 1);
    input.prev_output_index = 0;
    inputs.push_back(input);

    Transaction tx = wallet.create_transaction(inputs, pubkey, 100 * COIN);

    test_assert(tx.inputs.size() == 1, "Transaction has 1 input");
    test_assert(tx.outputs.size() >= 1, "Transaction has at least 1 output");
    test_assert(tx.version == 1, "Transaction version is 1");
}

// Test transaction signing
void test_transaction_signing() {
    std::cout << "\n=== Testing Transaction Signing ===" << std::endl;

    Wallet wallet(":memory:");
    crypto::DilithiumPubKey pubkey = wallet.generate_new_key();

    // Create transaction
    std::vector<TxInput> inputs;
    TxInput input;
    std::fill(input.prev_tx_hash.begin(), input.prev_tx_hash.end(), 1);
    input.prev_output_index = 0;
    inputs.push_back(input);

    Transaction tx = wallet.create_transaction(inputs, pubkey, 100 * COIN);

    // Sign
    bool signed_tx = wallet.sign_transaction(tx);
    test_assert(signed_tx, "Transaction signed successfully");

    // Check signature exists
    if (!tx.inputs.empty()) {
        bool has_signature = false;
        for (uint8_t b : tx.inputs[0].signature) {
            if (b != 0) {
                has_signature = true;
                break;
            }
        }
        test_assert(has_signature, "Transaction input has signature");
    }
}

// Test balance calculation
void test_balance_calculation() {
    std::cout << "\n=== Testing Balance Calculation ===" << std::endl;

    Wallet wallet(":memory:");

    uint64_t balance = wallet.get_balance();
    test_assert(balance == 0, "New wallet has zero balance");

    // Add UTXO (this would normally come from blockchain)
    auto pubkey = wallet.generate_new_key();
    UTXO utxo;
    utxo.tx_hash = Hash256{};
    utxo.output_index = 0;
    utxo.amount = 100 * COIN;
    utxo.recipient = pubkey;

    wallet.add_utxo(utxo);

    balance = wallet.get_balance();
    test_assert(balance == 100 * COIN, "Balance updated after adding UTXO");
}

// Test coin selection
void test_coin_selection() {
    std::cout << "\n=== Testing Coin Selection ===" << std::endl;

    Wallet wallet(":memory:");
    auto pubkey = wallet.generate_new_key();

    // Add multiple UTXOs
    for (int i = 0; i < 5; i++) {
        UTXO utxo;
        utxo.tx_hash = Hash256{};
        utxo.output_index = i;
        utxo.amount = (i + 1) * 10 * COIN;  // 10, 20, 30, 40, 50
        utxo.recipient = pubkey;
        wallet.add_utxo(utxo);
    }

    // Select coins for 75 COIN
    auto selected = wallet.select_coins(75 * COIN);

    test_assert(selected.has_value(), "Coins selected successfully");

    if (selected) {
        uint64_t total = 0;
        for (const auto& utxo : *selected) {
            total += utxo.amount;
        }
        test_assert(total >= 75 * COIN, "Selected coins cover required amount");
    }

    // Try to select more than available
    auto insufficient = wallet.select_coins(1000 * COIN);
    test_assert(!insufficient.has_value(), "Insufficient funds returns nullopt");
}

// Test key backup and restore
void test_key_backup_restore() {
    std::cout << "\n=== Testing Key Backup and Restore ===" << std::endl;

    Wallet wallet1(":memory:");
    auto pubkey1 = wallet1.generate_new_key();

    // Export key
    auto exported = wallet1.export_key(pubkey1);
    test_assert(exported.has_value(), "Key exported successfully");

    if (exported) {
        // Import into new wallet
        Wallet wallet2(":memory:");
        bool imported = wallet2.import_key(*exported);

        test_assert(imported, "Key imported successfully");
        test_assert(wallet2.get_key_count() == 1, "Imported wallet has 1 key");
    }
}

// Test HD wallet derivation
void test_hd_derivation() {
    std::cout << "\n=== Testing HD Wallet Derivation ===" << std::endl;

    Wallet wallet(":memory:");
    wallet.generate_mnemonic();

    // Derive multiple keys
    auto key1 = wallet.derive_key(0);
    auto key2 = wallet.derive_key(1);
    auto key3 = wallet.derive_key(0);  // Same as key1

    test_assert(key1 != key2, "Different indices produce different keys");
    test_assert(key1 == key3, "Same index produces same key");
}

// Test transaction history
void test_transaction_history() {
    std::cout << "\n=== Testing Transaction History ===" << std::endl;

    Wallet wallet(":memory:");

    auto history = wallet.get_transaction_history();
    test_assert(history.empty(), "New wallet has empty history");

    // Add transaction (normally from blockchain)
    Transaction tx;
    tx.version = 1;

    wallet.add_transaction(tx);

    history = wallet.get_transaction_history();
    test_assert(history.size() == 1, "History has 1 transaction");
}

// Test wallet persistence
void test_wallet_persistence() {
    std::cout << "\n=== Testing Wallet Persistence ===" << std::endl;

    std::string wallet_path = "/tmp/test_wallet.db";

    // Create wallet
    {
        Wallet wallet(wallet_path);
        wallet.generate_new_key();
        test_assert(wallet.get_key_count() == 1, "Wallet has 1 key");
    }

    // Reload wallet
    {
        Wallet wallet(wallet_path);
        test_assert(wallet.get_key_count() == 1, "Reloaded wallet has 1 key");
    }

    // Cleanup
    std::filesystem::remove(wallet_path);
}

// Test change address creation
void test_change_address() {
    std::cout << "\n=== Testing Change Address Creation ===" << std::endl;

    Wallet wallet(":memory:");

    auto change_addr = wallet.get_change_address();

    bool is_valid = false;
    for (uint8_t b : change_addr) {
        if (b != 0) {
            is_valid = true;
            break;
        }
    }

    test_assert(is_valid, "Change address is valid");
    test_assert(wallet.get_key_count() >= 1, "Change address creates new key");
}

// Test fee estimation
void test_fee_estimation() {
    std::cout << "\n=== Testing Fee Estimation ===" << std::endl;

    Wallet wallet(":memory:");

    // Create simple transaction
    std::vector<TxInput> inputs;
    TxInput input;
    inputs.push_back(input);

    auto pubkey = wallet.generate_new_key();
    Transaction tx = wallet.create_transaction(inputs, pubkey, 100 * COIN);

    uint64_t estimated_fee = wallet.estimate_fee(tx);

    test_assert(estimated_fee > 0, "Fee estimation returns non-zero");
    test_assert(estimated_fee < 1 * COIN, "Fee is reasonable");
}

// Main test runner
int main() {
    std::cout << "INTcoin Wallet Test Suite" << std::endl;
    std::cout << "=========================" << std::endl;

    // Run all tests
    test_wallet_creation();
    test_key_generation();
    test_mnemonic_generation();
    test_wallet_from_mnemonic();
    test_wallet_encryption();
    test_address_generation();
    test_transaction_creation();
    test_transaction_signing();
    test_balance_calculation();
    test_coin_selection();
    test_key_backup_restore();
    test_hd_derivation();
    test_transaction_history();
    test_wallet_persistence();
    test_change_address();
    test_fee_estimation();

    // Summary
    std::cout << "\n=========================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "=========================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
