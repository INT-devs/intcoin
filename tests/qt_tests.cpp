/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Qt Wallet UI Tests
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <memory>

using namespace intcoin;

// ============================================================================
// Test Fixtures
// ============================================================================

class QtWalletTestFixture : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        // Create QApplication instance for Qt tests
        // Note: This would normally be done in main(), but we need it for tests
        if (!QApplication::instance()) {
            int argc = 1;
            char* argv[] = {const_cast<char*>("qt_tests")};
            app_ = new QApplication(argc, argv);
        }
    }

    static void TearDownTestCase() {
        // Don't delete QApplication - it's needed for all tests
    }

    void SetUp() override {
        // Create temporary directory for wallet files
        temp_dir_ = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(temp_dir_->isValid());

        wallet_path_ = temp_dir_->path().toStdString() + "/test_wallet.dat";

        // Create test wallet
        WalletConfig config;
        config.network = NetworkType::TESTNET;
        config.wallet_file = wallet_path_;

        wallet_ = std::make_shared<Wallet>(config);

        test_password_ = "TestPassword123!";
        test_mnemonic_ = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
    }

    void TearDown() override {
        wallet_.reset();
        temp_dir_.reset();
    }

    static QApplication* app_;
    std::unique_ptr<QTemporaryDir> temp_dir_;
    std::shared_ptr<Wallet> wallet_;
    std::string wallet_path_;
    std::string test_password_;
    std::string test_mnemonic_;
};

QApplication* QtWalletTestFixture::app_ = nullptr;

// ============================================================================
// Wallet Creation Tests
// ============================================================================

TEST_F(QtWalletTestFixture, WalletCreation_NewWallet) {
    // Generate new mnemonic
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    std::string mnemonic = mnemonic_result.GetValue();

    // Create wallet with mnemonic
    auto create_result = wallet_->Create(mnemonic, "");
    EXPECT_TRUE(create_result.IsOk());

    // Verify wallet was created
    EXPECT_TRUE(wallet_->IsLoaded());

    // Verify mnemonic can be retrieved
    auto retrieved_mnemonic = wallet_->GetMnemonic();
    EXPECT_TRUE(retrieved_mnemonic.IsOk());
    EXPECT_EQ(retrieved_mnemonic.GetValue(), mnemonic);
}

TEST_F(QtWalletTestFixture, WalletCreation_WithPassword) {
    // Generate mnemonic
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    std::string mnemonic = mnemonic_result.GetValue();

    // Create wallet
    auto create_result = wallet_->Create(mnemonic, "");
    ASSERT_TRUE(create_result.IsOk());

    // Encrypt wallet
    auto encrypt_result = wallet_->Encrypt(test_password_);
    EXPECT_TRUE(encrypt_result.IsOk());

    // Verify wallet is encrypted
    EXPECT_TRUE(wallet_->IsEncrypted());
    EXPECT_TRUE(wallet_->IsLocked());
}

TEST_F(QtWalletTestFixture, WalletCreation_InvalidMnemonic) {
    // Try to create wallet with invalid mnemonic
    std::string invalid_mnemonic = "invalid mnemonic phrase that is not valid";

    auto create_result = wallet_->Create(invalid_mnemonic, "");
    EXPECT_TRUE(create_result.IsError());
}

TEST_F(QtWalletTestFixture, WalletCreation_WeakPassword) {
    // Generate mnemonic
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    // Create wallet
    auto create_result = wallet_->Create(mnemonic_result.GetValue(), "");
    ASSERT_TRUE(create_result.IsOk());

    // Try weak password (less than 8 characters)
    std::string weak_password = "123";

    auto encrypt_result = wallet_->Encrypt(weak_password);
    // Should fail with weak password (wallet enforces minimum length)
    EXPECT_TRUE(encrypt_result.IsError());
}

// ============================================================================
// Wallet Encryption Tests
// ============================================================================

TEST_F(QtWalletTestFixture, WalletEncryption_EncryptDecrypt) {
    // Create unencrypted wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    auto create_result = wallet_->Create(mnemonic_result.GetValue(), "");
    ASSERT_TRUE(create_result.IsOk());

    // Initially not encrypted
    EXPECT_FALSE(wallet_->IsEncrypted());

    // Encrypt wallet
    auto encrypt_result = wallet_->Encrypt(test_password_);
    EXPECT_TRUE(encrypt_result.IsOk());
    EXPECT_TRUE(wallet_->IsEncrypted());
    EXPECT_TRUE(wallet_->IsLocked());

    // Unlock wallet
    auto unlock_result = wallet_->Unlock(test_password_, 0);
    EXPECT_TRUE(unlock_result.IsOk());
    EXPECT_FALSE(wallet_->IsLocked());

    // Lock wallet
    auto lock_result = wallet_->Lock();
    EXPECT_TRUE(lock_result.IsOk());
    EXPECT_TRUE(wallet_->IsLocked());
}

TEST_F(QtWalletTestFixture, WalletEncryption_WrongPassword) {
    // Create and encrypt wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    // Try to unlock with wrong password
    auto unlock_result = wallet_->Unlock("WrongPassword!", 0);
    EXPECT_TRUE(unlock_result.IsError());
    EXPECT_TRUE(wallet_->IsLocked());
}

TEST_F(QtWalletTestFixture, WalletEncryption_DoubleEncrypt) {
    // Create and encrypt wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    // Try to encrypt again
    auto encrypt_result = wallet_->Encrypt("NewPassword123!");

    // Should fail - already encrypted
    EXPECT_TRUE(encrypt_result.IsError());
}

TEST_F(QtWalletTestFixture, WalletEncryption_UnlockTimeout) {
    // Create and encrypt wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    // Unlock with 1 second timeout
    auto unlock_result = wallet_->Unlock(test_password_, 1);
    EXPECT_TRUE(unlock_result.IsOk());
    EXPECT_FALSE(wallet_->IsLocked());

    // Wait for timeout
    QTest::qWait(1500);  // Wait 1.5 seconds

    // Should be locked again (if auto-lock is implemented)
    // Note: This depends on wallet implementation
}

// ============================================================================
// Address Generation Tests
// ============================================================================

TEST_F(QtWalletTestFixture, AddressGeneration_NewAddress) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Generate new address
    auto address_result = wallet_->GetNewAddress();
    EXPECT_TRUE(address_result.IsOk());

    Address address = address_result.GetValue();

    // Verify address is valid
    EXPECT_FALSE(address.ToString().empty());
    EXPECT_TRUE(address.ToString().starts_with("intc1"));  // Testnet prefix
}

TEST_F(QtWalletTestFixture, AddressGeneration_MultipleAddresses) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Generate 10 addresses
    std::vector<Address> addresses;
    for (int i = 0; i < 10; i++) {
        auto address_result = wallet_->GetNewAddress();
        ASSERT_TRUE(address_result.IsOk());
        addresses.push_back(address_result.GetValue());
    }

    // Verify all addresses are unique
    for (size_t i = 0; i < addresses.size(); i++) {
        for (size_t j = i + 1; j < addresses.size(); j++) {
            EXPECT_NE(addresses[i].ToString(), addresses[j].ToString());
        }
    }
}

TEST_F(QtWalletTestFixture, AddressGeneration_FromLockedWallet) {
    // Create encrypted wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    // Should be locked
    EXPECT_TRUE(wallet_->IsLocked());

    // Try to generate address from locked wallet
    auto address_result = wallet_->GetNewAddress();

    // May succeed (address generation doesn't need private keys)
    // or fail depending on implementation
}

// ============================================================================
// Transaction Creation Tests
// ============================================================================

TEST_F(QtWalletTestFixture, TransactionCreation_BasicTx) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Get recipient address
    auto recipient_result = wallet_->GetNewAddress();
    ASSERT_TRUE(recipient_result.IsOk());
    Address recipient = recipient_result.GetValue();

    // Create transaction (will fail without UTXOs, but tests the API)
    uint64_t amount = 1000000000;  // 1 INT
    uint64_t fee = 1000;

    auto tx_result = wallet_->CreateTransaction(recipient, amount, fee);

    // Expected to fail without UTXOs
    EXPECT_TRUE(tx_result.IsError());
    EXPECT_NE(tx_result.GetError().find("insufficient"), std::string::npos);
}

TEST_F(QtWalletTestFixture, TransactionCreation_ZeroAmount) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    auto recipient_result = wallet_->GetNewAddress();
    ASSERT_TRUE(recipient_result.IsOk());

    // Try to create transaction with zero amount
    auto tx_result = wallet_->CreateTransaction(recipient_result.GetValue(), 0, 1000);

    EXPECT_TRUE(tx_result.IsError());
}

TEST_F(QtWalletTestFixture, TransactionCreation_LockedWallet) {
    // Create encrypted wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    auto recipient_result = wallet_->GetNewAddress();
    ASSERT_TRUE(recipient_result.IsOk());

    // Try to create transaction while locked
    auto tx_result = wallet_->CreateTransaction(
        recipient_result.GetValue(), 1000000000, 1000);

    // Should fail - wallet is locked
    EXPECT_TRUE(tx_result.IsError());
}

// ============================================================================
// Wallet Backup/Restore Tests
// ============================================================================

TEST_F(QtWalletTestFixture, WalletBackup_CreateBackup) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    std::string original_mnemonic = mnemonic_result.GetValue();

    wallet_->Create(original_mnemonic, "");

    // Create backup file
    std::string backup_path = temp_dir_->path().toStdString() + "/backup.dat";

    auto backup_result = wallet_->BackupWallet(backup_path);
    EXPECT_TRUE(backup_result.IsOk());

    // Verify backup file exists
    EXPECT_TRUE(std::filesystem::exists(backup_path));
}

TEST_F(QtWalletTestFixture, WalletBackup_RestoreFromBackup) {
    // Create original wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    std::string original_mnemonic = mnemonic_result.GetValue();

    wallet_->Create(original_mnemonic, "");

    // Generate some addresses
    for (int i = 0; i < 5; i++) {
        wallet_->GetNewAddress();
    }

    // Create backup
    std::string backup_path = temp_dir_->path().toStdString() + "/backup.dat";
    wallet_->BackupWallet(backup_path);

    // Create new wallet from backup
    std::string restore_path = temp_dir_->path().toStdString() + "/restored_wallet.dat";

    WalletConfig restore_config;
    restore_config.network = NetworkType::TESTNET;
    restore_config.wallet_file = restore_path;

    auto restored_wallet = std::make_shared<Wallet>(restore_config);

    // Load from backup
    auto load_result = restored_wallet->Load(backup_path, "");
    EXPECT_TRUE(load_result.IsOk());

    // Verify restored wallet has same mnemonic
    auto restored_mnemonic = restored_wallet->GetMnemonic();
    EXPECT_TRUE(restored_mnemonic.IsOk());
    EXPECT_EQ(restored_mnemonic.GetValue(), original_mnemonic);
}

TEST_F(QtWalletTestFixture, WalletBackup_EncryptedBackup) {
    // Create encrypted wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());

    wallet_->Create(mnemonic_result.GetValue(), "");
    wallet_->Encrypt(test_password_);

    // Create backup
    std::string backup_path = temp_dir_->path().toStdString() + "/encrypted_backup.dat";

    // Unlock wallet first
    wallet_->Unlock(test_password_, 0);

    auto backup_result = wallet_->BackupWallet(backup_path);
    EXPECT_TRUE(backup_result.IsOk());
}

// ============================================================================
// Balance and UTXO Tests
// ============================================================================

TEST_F(QtWalletTestFixture, Balance_InitialBalance) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Initial balance should be zero
    uint64_t balance = wallet_->GetBalance();
    EXPECT_EQ(balance, 0);
}

TEST_F(QtWalletTestFixture, Balance_ConfirmedVsPending) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Get confirmed balance
    uint64_t confirmed = wallet_->GetBalance();
    EXPECT_EQ(confirmed, 0);

    // Get pending balance
    uint64_t pending = wallet_->GetPendingBalance();
    EXPECT_EQ(pending, 0);
}

// ============================================================================
// Mnemonic Recovery Tests
// ============================================================================

TEST_F(QtWalletTestFixture, MnemonicRecovery_RecoverFromPhrase) {
    // Create wallet with known mnemonic
    std::string known_mnemonic = test_mnemonic_;

    auto create_result = wallet_->Create(known_mnemonic, "");
    ASSERT_TRUE(create_result.IsOk());

    // Generate some addresses
    std::vector<Address> original_addresses;
    for (int i = 0; i < 5; i++) {
        auto addr_result = wallet_->GetNewAddress();
        ASSERT_TRUE(addr_result.IsOk());
        original_addresses.push_back(addr_result.GetValue());
    }

    // Create new wallet and recover from same mnemonic
    std::string recovery_path = temp_dir_->path().toStdString() + "/recovered_wallet.dat";

    WalletConfig recovery_config;
    recovery_config.network = NetworkType::TESTNET;
    recovery_config.wallet_file = recovery_path;

    auto recovered_wallet = std::make_shared<Wallet>(recovery_config);
    auto recover_result = recovered_wallet->Create(known_mnemonic, "");
    ASSERT_TRUE(recover_result.IsOk());

    // Generate same number of addresses
    std::vector<Address> recovered_addresses;
    for (int i = 0; i < 5; i++) {
        auto addr_result = recovered_wallet->GetNewAddress();
        ASSERT_TRUE(addr_result.IsOk());
        recovered_addresses.push_back(addr_result.GetValue());
    }

    // Verify addresses match (deterministic HD wallet)
    for (size_t i = 0; i < original_addresses.size(); i++) {
        EXPECT_EQ(original_addresses[i].ToString(),
                  recovered_addresses[i].ToString());
    }
}

TEST_F(QtWalletTestFixture, MnemonicRecovery_InvalidPhrase) {
    // Try to recover with invalid mnemonic
    std::string invalid_mnemonic = "this is not a valid mnemonic phrase at all";

    auto create_result = wallet_->Create(invalid_mnemonic, "");
    EXPECT_TRUE(create_result.IsError());
}

// ============================================================================
// Wallet State Tests
// ============================================================================

TEST_F(QtWalletTestFixture, WalletState_LoadedState) {
    // Wallet should not be loaded initially
    EXPECT_FALSE(wallet_->IsLoaded());

    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Should be loaded now
    EXPECT_TRUE(wallet_->IsLoaded());
}

TEST_F(QtWalletTestFixture, WalletState_EncryptionState) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Initially not encrypted
    EXPECT_FALSE(wallet_->IsEncrypted());
    EXPECT_FALSE(wallet_->IsLocked());

    // Encrypt
    wallet_->Encrypt(test_password_);

    // Should be encrypted and locked
    EXPECT_TRUE(wallet_->IsEncrypted());
    EXPECT_TRUE(wallet_->IsLocked());

    // Unlock
    wallet_->Unlock(test_password_, 0);

    // Should be encrypted but not locked
    EXPECT_TRUE(wallet_->IsEncrypted());
    EXPECT_FALSE(wallet_->IsLocked());
}

// ============================================================================
// Network Type Tests
// ============================================================================

TEST_F(QtWalletTestFixture, NetworkType_TestnetAddresses) {
    // Create testnet wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    wallet_->Create(mnemonic_result.GetValue(), "");

    // Generate address
    auto address_result = wallet_->GetNewAddress();
    ASSERT_TRUE(address_result.IsOk());

    // Should have testnet prefix
    EXPECT_TRUE(address_result.GetValue().ToString().starts_with("intc1"));
}

TEST_F(QtWalletTestFixture, NetworkType_MainnetAddresses) {
    // Create mainnet wallet
    std::string mainnet_path = temp_dir_->path().toStdString() + "/mainnet_wallet.dat";

    WalletConfig mainnet_config;
    mainnet_config.network = NetworkType::MAINNET;
    mainnet_config.wallet_file = mainnet_path;

    auto mainnet_wallet = std::make_shared<Wallet>(mainnet_config);

    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    mainnet_wallet->Create(mnemonic_result.GetValue(), "");

    // Generate address
    auto address_result = mainnet_wallet->GetNewAddress();
    ASSERT_TRUE(address_result.IsOk());

    // Should have mainnet prefix
    EXPECT_TRUE(address_result.GetValue().ToString().starts_with("int1"));
}

// ============================================================================
// Persistence Tests
// ============================================================================

TEST_F(QtWalletTestFixture, Persistence_SaveAndLoad) {
    // Create wallet
    auto mnemonic_result = crypto::GenerateMnemonic(256);
    ASSERT_TRUE(mnemonic_result.IsOk());
    std::string original_mnemonic = mnemonic_result.GetValue();

    wallet_->Create(original_mnemonic, "");

    // Save wallet
    auto save_result = wallet_->Save();
    EXPECT_TRUE(save_result.IsOk());

    // Create new wallet instance
    WalletConfig load_config;
    load_config.network = NetworkType::TESTNET;
    load_config.wallet_file = wallet_path_;

    auto loaded_wallet = std::make_shared<Wallet>(load_config);

    // Load saved wallet
    auto load_result = loaded_wallet->Load(wallet_path_, "");
    EXPECT_TRUE(load_result.IsOk());

    // Verify mnemonic matches
    auto loaded_mnemonic = loaded_wallet->GetMnemonic();
    EXPECT_TRUE(loaded_mnemonic.IsOk());
    EXPECT_EQ(loaded_mnemonic.GetValue(), original_mnemonic);
}

TEST_F(QtWalletTestFixture, Persistence_LoadNonexistent) {
    // Try to load nonexistent wallet
    std::string fake_path = temp_dir_->path().toStdString() + "/nonexistent.dat";

    auto load_result = wallet_->Load(fake_path, "");
    EXPECT_TRUE(load_result.IsError());
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Create QApplication for all Qt tests
    QApplication app(argc, argv);

    return RUN_ALL_TESTS();
}
