/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Wallet Test Suite
 */

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace intcoin;
using namespace intcoin::wallet;

// Test helper: Clean up test wallet directory
void cleanup_test_wallet(const std::string& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    // Create the directory for the test
    std::filesystem::create_directories(path);
}

// ============================================================================
// BIP39 Mnemonic Tests
// ============================================================================

void test_mnemonic_generation() {
    std::cout << "Testing mnemonic generation...\n";

    // Test 12-word mnemonic
    auto result_12 = Mnemonic::Generate(12);
    assert(result_12.IsOk());
    auto words_12 = result_12.value.value();
    assert(words_12.size() == 12);

    // Test 24-word mnemonic
    auto result_24 = Mnemonic::Generate(24);
    assert(result_24.IsOk());
    auto words_24 = result_24.value.value();
    assert(words_24.size() == 24);

    // Test invalid word count (should fail)
    auto result_invalid = Mnemonic::Generate(10);
    assert(result_invalid.IsError());

    // Test that generated words are from wordlist
    const auto& wordlist = Mnemonic::GetWordList();
    for (const auto& word : words_12) {
        bool found = false;
        for (const auto& valid_word : wordlist) {
            if (word == valid_word) {
                found = true;
                break;
            }
        }
        assert(found);
    }

    std::cout << "  ✓ Mnemonic generation tests passed\n";
}

void test_mnemonic_validation() {
    std::cout << "Testing mnemonic validation...\n";

    // Valid 12-word mnemonic
    std::vector<std::string> valid_mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };

    auto validate_result = Mnemonic::Validate(valid_mnemonic);
    assert(validate_result.IsOk());

    // Invalid word count
    std::vector<std::string> invalid_count = {
        "abandon", "ability", "able"
    };
    auto invalid_result = Mnemonic::Validate(invalid_count);
    assert(invalid_result.IsError());

    // Invalid word (not in wordlist)
    std::vector<std::string> invalid_word = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "notaword"
    };
    auto invalid_word_result = Mnemonic::Validate(invalid_word);
    assert(invalid_word_result.IsError());

    std::cout << "  ✓ Mnemonic validation tests passed\n";
}

void test_mnemonic_to_seed() {
    std::cout << "Testing mnemonic to seed conversion...\n";

    std::vector<std::string> mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };

    // Test without passphrase
    auto seed_result = Mnemonic::ToSeed(mnemonic, "");
    assert(seed_result.IsOk());
    auto seed = seed_result.value.value();
    assert(seed.size() == 64);  // BIP39 produces 512-bit (64-byte) seed

    // Test with passphrase
    auto seed_pass_result = Mnemonic::ToSeed(mnemonic, "my_passphrase");
    assert(seed_pass_result.IsOk());
    auto seed_pass = seed_pass_result.value.value();
    assert(seed_pass.size() == 64);

    // Different passphrases should produce different seeds
    assert(seed != seed_pass);

    // Same mnemonic + passphrase should produce same seed (deterministic)
    auto seed_pass_2 = Mnemonic::ToSeed(mnemonic, "my_passphrase");
    assert(seed_pass_2.IsOk());
    assert(seed_pass == seed_pass_2.value.value());

    std::cout << "  ✓ Mnemonic to seed tests passed\n";
}

// ============================================================================
// HD Key Derivation Tests
// ============================================================================

void test_master_key_generation() {
    std::cout << "Testing master key generation...\n";

    // Generate seed
    std::vector<std::string> mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };
    auto seed_result = Mnemonic::ToSeed(mnemonic, "");
    assert(seed_result.IsOk());
    auto seed = seed_result.value.value();

    // Generate master key
    auto master_result = HDKeyDerivation::GenerateMaster(seed);
    assert(master_result.IsOk());

    ExtendedKey master = master_result.value.value();
    assert(master.depth == 0);
    assert(master.parent_fingerprint == 0);
    assert(master.child_index == 0);
    assert(master.private_key.has_value());
    assert(master.public_key.has_value());
    assert(master.private_key.value().size() == DILITHIUM3_SECRETKEYBYTES);
    assert(master.public_key.value().size() == DILITHIUM3_PUBLICKEYBYTES);

    // Same seed should produce same master key (deterministic)
    auto master_2 = HDKeyDerivation::GenerateMaster(seed);
    assert(master_2.IsOk());
    assert(master.private_key == master_2.value.value().private_key);
    assert(master.public_key == master_2.value.value().public_key);

    std::cout << "  ✓ Master key generation tests passed\n";
}

void test_child_key_derivation() {
    std::cout << "Testing child key derivation...\n";

    // Generate master key
    std::vector<std::string> mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };
    auto seed = Mnemonic::ToSeed(mnemonic, "").value.value();
    auto master = HDKeyDerivation::GenerateMaster(seed).value.value();

    // Derive hardened child (m/0')
    auto child_0h_result = HDKeyDerivation::DeriveChild(master, 0, true);
    assert(child_0h_result.IsOk());
    ExtendedKey child_0h = child_0h_result.value.value();

    assert(child_0h.depth == 1);
    assert(child_0h.child_index == (0 | 0x80000000));
    assert(child_0h.private_key.has_value());
    assert(child_0h.public_key.has_value());

    // Derive non-hardened child (m/0)
    auto child_0_result = HDKeyDerivation::DeriveChild(master, 0, false);
    assert(child_0_result.IsOk());
    ExtendedKey child_0 = child_0_result.value.value();

    assert(child_0.depth == 1);
    assert(child_0.child_index == 0);

    // Hardened and non-hardened children should be different
    assert(child_0h.private_key != child_0.private_key);

    // Derive multiple children
    auto child_1 = HDKeyDerivation::DeriveChild(master, 1, false).value.value();
    auto child_2 = HDKeyDerivation::DeriveChild(master, 2, false).value.value();

    // All children should be different
    assert(child_0.private_key != child_1.private_key);
    assert(child_1.private_key != child_2.private_key);

    // Same parent + index should produce same child (deterministic)
    auto child_0_again = HDKeyDerivation::DeriveChild(master, 0, false).value.value();
    assert(child_0.private_key == child_0_again.private_key);

    std::cout << "  ✓ Child key derivation tests passed\n";
}

void test_derivation_path() {
    std::cout << "Testing derivation path...\n";

    // Parse derivation path
    auto path_result = DerivationPath::Parse("m/44'/2210'/0'/0/0");
    assert(path_result.IsOk());
    DerivationPath path = path_result.value.value();

    auto components = path.GetComponents();
    assert(components.size() == 5);
    assert(components[0].index == 44 && components[0].hardened == true);
    assert(components[1].index == 2210 && components[1].hardened == true);
    assert(components[2].index == 0 && components[2].hardened == true);
    assert(components[3].index == 0 && components[3].hardened == false);
    assert(components[4].index == 0 && components[4].hardened == false);

    // Test path to string
    std::string path_str = path.ToString();
    assert(path_str == "m/44'/2210'/0'/0/0");

    // Test invalid paths
    auto invalid_1 = DerivationPath::Parse("44'/2210'/0'/0/0");  // Missing 'm'
    assert(invalid_1.IsError());

    auto invalid_2 = DerivationPath::Parse("m/44/2210/0/0/0");  // Missing hardened markers
    assert(invalid_2.IsOk());  // This is actually valid, just non-hardened

    // Test path append
    DerivationPath base_path;
    base_path = base_path.Append(44, true);
    base_path = base_path.Append(2210, true);
    assert(base_path.GetComponents().size() == 2);
    assert(base_path.ToString() == "m/44'/2210'");

    std::cout << "  ✓ Derivation path tests passed\n";
}

void test_full_path_derivation() {
    std::cout << "Testing full BIP44 path derivation...\n";

    // Generate master key
    std::vector<std::string> mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };
    auto seed = Mnemonic::ToSeed(mnemonic, "").value.value();
    auto master = HDKeyDerivation::GenerateMaster(seed).value.value();

    // Derive BIP44 path: m/44'/2210'/0'/0/0
    auto path = DerivationPath::Parse("m/44'/2210'/0'/0/0").value.value();
    auto key_result = HDKeyDerivation::DerivePath(master, path);
    assert(key_result.IsOk());

    ExtendedKey derived = key_result.value.value();
    assert(derived.depth == 5);
    assert(derived.private_key.has_value());
    assert(derived.public_key.has_value());

    // Same path should produce same key
    auto derived_2 = HDKeyDerivation::DerivePath(master, path).value.value();
    assert(derived.private_key == derived_2.private_key);

    // Different paths should produce different keys
    auto path_2 = DerivationPath::Parse("m/44'/2210'/0'/0/1").value.value();
    auto derived_different = HDKeyDerivation::DerivePath(master, path_2).value.value();
    assert(derived.private_key != derived_different.private_key);

    std::cout << "  ✓ Full path derivation tests passed\n";
}

// ============================================================================
// Wallet Tests
// ============================================================================

void test_wallet_creation() {
    std::cout << "Testing wallet creation...\n";

    std::string test_dir = "/tmp/intcoin_test_wallet_create";
    cleanup_test_wallet(test_dir);

    WalletConfig config;
    config.data_dir = test_dir;
    config.coin_type = 2210;
    config.keypool_size = 10;

    // Generate mnemonic
    auto mnemonic_result = Mnemonic::Generate(12);
    assert(mnemonic_result.IsOk());
    auto mnemonic = mnemonic_result.value.value();

    // Create wallet
    Wallet wallet(config);
    auto create_result = wallet.Create(mnemonic, "");
    if (!create_result.IsOk()) {
        std::cerr << "ERROR: Wallet creation failed: " << create_result.error << "\n";
    }
    assert(create_result.IsOk());
    assert(wallet.IsLoaded());
    assert(!wallet.IsEncrypted());
    assert(!wallet.IsLocked());

    // Get wallet info
    auto info_result = wallet.GetInfo();
    assert(info_result.IsOk());
    auto info = info_result.value.value();
    assert(info.address_count >= config.keypool_size);  // At least keypool size

    // Get addresses
    auto addrs_result = wallet.GetAddresses();
    assert(addrs_result.IsOk());
    auto addresses = addrs_result.value.value();
    assert(addresses.size() >= config.keypool_size);

    // Verify addresses are valid Bech32
    for (const auto& addr : addresses) {
        assert(addr.address.substr(0, 4) == "int1");
        assert(addr.public_key.size() == DILITHIUM3_PUBLICKEYBYTES);
    }

    // Close wallet
    auto close_result = wallet.Close();
    assert(close_result.IsOk());
    assert(!wallet.IsLoaded());

    cleanup_test_wallet(test_dir);
    std::cout << "  ✓ Wallet creation tests passed\n";
}

void test_wallet_load() {
    std::cout << "Testing wallet load/persistence...\n";

    std::string test_dir = "/tmp/intcoin_test_wallet_load";
    cleanup_test_wallet(test_dir);

    WalletConfig config;
    config.data_dir = test_dir;
    config.coin_type = 2210;
    config.keypool_size = 5;

    // Create wallet
    auto mnemonic = Mnemonic::Generate(12).value.value();
    Wallet wallet1(config);
    wallet1.Create(mnemonic, "");

    // Get first address
    auto addr1_result = wallet1.GetNewAddress("Test Address");
    assert(addr1_result.IsOk());
    std::string first_address = addr1_result.value.value();

    // Close wallet
    wallet1.Close();

    // Load wallet again
    Wallet wallet2(config);
    auto load_result = wallet2.Load();
    assert(load_result.IsOk());
    assert(wallet2.IsLoaded());

    // Verify addresses persisted
    auto addrs_result = wallet2.GetAddresses();
    assert(addrs_result.IsOk());
    auto addresses = addrs_result.value.value();

    bool found_first_address = false;
    for (const auto& addr : addresses) {
        if (addr.address == first_address) {
            found_first_address = true;
            assert(addr.label == "Test Address");
            break;
        }
    }
    assert(found_first_address);

    wallet2.Close();
    cleanup_test_wallet(test_dir);
    std::cout << "  ✓ Wallet load/persistence tests passed\n";
}

void test_address_generation() {
    std::cout << "Testing address generation...\n";

    std::string test_dir = "/tmp/intcoin_test_wallet_address";
    cleanup_test_wallet(test_dir);

    WalletConfig config;
    config.data_dir = test_dir;
    config.coin_type = 2210;
    config.keypool_size = 5;

    auto mnemonic = Mnemonic::Generate(12).value.value();
    Wallet wallet(config);
    wallet.Create(mnemonic, "");

    // Generate new receiving address
    auto addr1_result = wallet.GetNewAddress("Address 1");
    assert(addr1_result.IsOk());
    std::string addr1 = addr1_result.value.value();
    assert(addr1.substr(0, 4) == "int1");

    // Generate another address
    auto addr2_result = wallet.GetNewAddress("Address 2");
    assert(addr2_result.IsOk());
    std::string addr2 = addr2_result.value.value();

    // Addresses should be different
    assert(addr1 != addr2);

    // Generate change address
    auto change_result = wallet.GetNewChangeAddress();
    assert(change_result.IsOk());
    std::string change_addr = change_result.value.value();
    assert(change_addr.substr(0, 4) == "int1");

    // Change address should be different from receiving addresses
    assert(change_addr != addr1);
    assert(change_addr != addr2);

    // Verify all addresses are in wallet
    auto addrs_result = wallet.GetAddresses();
    auto all_addresses = addrs_result.value.value();

    bool found_addr1 = false, found_addr2 = false, found_change = false;
    for (const auto& addr : all_addresses) {
        if (addr.address == addr1) {
            found_addr1 = true;
            if (addr.label != "Address 1") {
                std::cerr << "ERROR: Expected label 'Address 1', got '" << addr.label << "'\n";
            }
            assert(addr.label == "Address 1");
            assert(!addr.is_change);
        }
        if (addr.address == addr2) {
            found_addr2 = true;
            assert(addr.label == "Address 2");
            assert(!addr.is_change);
        }
        if (addr.address == change_addr) {
            found_change = true;
            assert(addr.is_change);
        }
    }
    assert(found_addr1 && found_addr2 && found_change);

    wallet.Close();
    cleanup_test_wallet(test_dir);
    std::cout << "  ✓ Address generation tests passed\n";
}

void test_address_labels() {
    std::cout << "Testing address labels...\n";

    std::string test_dir = "/tmp/intcoin_test_wallet_labels";
    cleanup_test_wallet(test_dir);

    WalletConfig config;
    config.data_dir = test_dir;
    config.coin_type = 2210;

    auto mnemonic = Mnemonic::Generate(12).value.value();
    Wallet wallet(config);
    wallet.Create(mnemonic, "");

    // Generate address with label
    auto addr_result = wallet.GetNewAddress("My Savings");
    assert(addr_result.IsOk());
    std::string address = addr_result.value.value();

    // Get label
    auto label_result = wallet.GetAddressLabel(address);
    assert(label_result.IsOk());
    assert(label_result.value.value() == "My Savings");

    // Change label
    auto set_label_result = wallet.SetAddressLabel(address, "My Checking");
    assert(set_label_result.IsOk());

    // Verify new label
    auto new_label_result = wallet.GetAddressLabel(address);
    assert(new_label_result.IsOk());
    assert(new_label_result.value.value() == "My Checking");

    wallet.Close();
    cleanup_test_wallet(test_dir);
    std::cout << "  ✓ Address label tests passed\n";
}

void test_mnemonic_export() {
    std::cout << "Testing mnemonic export...\n";

    std::string test_dir = "/tmp/intcoin_test_wallet_mnemonic";
    cleanup_test_wallet(test_dir);

    WalletConfig config;
    config.data_dir = test_dir;
    config.coin_type = 2210;

    // Create wallet with known mnemonic
    std::vector<std::string> original_mnemonic = {
        "abandon", "ability", "able", "about", "above", "absent",
        "absorb", "abstract", "absurd", "abuse", "access", "accident"
    };

    Wallet wallet(config);
    wallet.Create(original_mnemonic, "");

    // Export mnemonic
    auto mnemonic_result = wallet.GetMnemonic();
    assert(mnemonic_result.IsOk());
    auto exported_mnemonic = mnemonic_result.value.value();

    // Verify it matches
    assert(exported_mnemonic.size() == original_mnemonic.size());
    for (size_t i = 0; i < original_mnemonic.size(); i++) {
        assert(exported_mnemonic[i] == original_mnemonic[i]);
    }

    wallet.Close();
    cleanup_test_wallet(test_dir);
    std::cout << "  ✓ Mnemonic export tests passed\n";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "INTcoin Wallet Test Suite\n";
    std::cout << "========================================\n\n";

    try {
        // BIP39 Mnemonic Tests
        test_mnemonic_generation();
        test_mnemonic_validation();
        test_mnemonic_to_seed();

        // HD Key Derivation Tests
        test_master_key_generation();
        test_child_key_derivation();
        test_derivation_path();
        test_full_path_derivation();

        // Wallet Tests
        test_wallet_creation();
        test_wallet_load();
        test_address_generation();
        test_address_labels();
        test_mnemonic_export();

        std::cout << "\n========================================\n";
        std::cout << "✅ All wallet tests passed!\n";
        std::cout << "========================================\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception\n";
        return 1;
    }
}
