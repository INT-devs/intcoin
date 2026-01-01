// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/bridge.h>
#include <intcoin/crypto.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstring>

using namespace intcoin;
using namespace intcoin::bridge;

// Test helper: Convert bytes to hex
std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Test 1: Bridge Initialization
void TestBridgeInitialization() {
    std::cout << "Test 1: Bridge Initialization... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 3;
    config.total_validators = 5;
    config.min_confirmations_btc = 6;
    config.min_confirmations_eth = 12;
    config.min_confirmations_ltc = 24;
    config.fee_basis_points = 30;  // 0.3%
    config.emergency_paused = false;
    config.min_validator_stake = 10000000;  // 0.1 BTC
    config.withdrawal_timeout = 3600 * 24;  // 24 hours

    auto result = bridge.Initialize(config);
    assert(result.IsOk());

    // Verify config
    auto config_result = bridge.GetConfig();
    assert(config_result.IsOk());
    assert(config_result.GetValue().min_validators == 3);
    assert(config_result.GetValue().total_validators == 5);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 2: Wrapped Token Registration
void TestWrappedTokenRegistration() {
    std::cout << "Test 2: Wrapped Token Registration... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Register wBTC
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;

    auto register_result = bridge.RegisterWrappedToken(wbtc);
    assert(register_result.IsOk());

    // Register wETH
    WrappedToken weth;
    weth.symbol = "wETH";
    weth.origin_chain = BridgeChain::ETHEREUM;
    weth.decimals = 18;
    weth.total_supply = 0;

    auto register_result2 = bridge.RegisterWrappedToken(weth);
    assert(register_result2.IsOk());

    // Get all tokens
    auto tokens_result = bridge.GetWrappedTokens();
    assert(tokens_result.IsOk());
    assert(tokens_result.GetValue().size() == 2);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 3: Validator Management
void TestValidatorManagement() {
    std::cout << "Test 3: Validator Management... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 5;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Add validators
    for (int i = 0; i < 3; ++i) {
        BridgeValidator validator;
        validator.public_key = std::vector<uint8_t>(33, 0x02 + i);
        validator.address = std::vector<uint8_t>(20, 0x10 + i);
        validator.stake = 2000000;  // Above minimum
        validator.is_active = true;
        validator.joined_at = std::time(nullptr);
        validator.reputation = 100;
        validator.signatures_count = 0;

        auto add_result = bridge.AddValidator(validator);
        assert(add_result.IsOk());
    }

    // Get validators
    auto validators_result = bridge.GetValidators();
    assert(validators_result.IsOk());
    assert(validators_result.GetValue().size() == 3);

    // Check if validator exists
    std::vector<uint8_t> test_pubkey(33, 0x02);
    auto is_validator_result = bridge.IsValidator(test_pubkey);
    assert(is_validator_result.IsOk());
    assert(is_validator_result.GetValue() == true);

    // Remove validator
    auto remove_result = bridge.RemoveValidator(test_pubkey);
    assert(remove_result.IsOk());

    // Verify removal
    auto validators_result2 = bridge.GetValidators();
    assert(validators_result2.IsOk());
    assert(validators_result2.GetValue().size() == 2);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 4: Deposit Proof Submission
void TestDepositProofSubmission() {
    std::cout << "Test 4: Deposit Proof Submission... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Register wBTC
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;
    bridge.RegisterWrappedToken(wbtc);

    // Add validators
    for (int i = 0; i < 3; ++i) {
        BridgeValidator validator;
        validator.public_key = std::vector<uint8_t>(33, 0x02 + i);
        validator.stake = 2000000;
        validator.is_active = true;
        validator.joined_at = std::time(nullptr);
        bridge.AddValidator(validator);
    }

    // Create deposit proof
    DepositProof proof;
    std::vector<uint8_t> temp_data(32, 0xAA);
    proof.source_tx_hash = SHA3::Hash(temp_data);
    proof.block_number = 100000;
    proof.depositor_address = std::vector<uint8_t>(20, 0xBB);
    proof.recipient_address = std::vector<uint8_t>(20, 0xCC);
    proof.amount = 50000000;  // 0.5 BTC
    proof.token = wbtc;
    proof.timestamp = std::time(nullptr);

    // Add validator signatures (M-of-N threshold)
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x02));
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x03));

    auto submit_result = bridge.SubmitDepositProof(proof);
    assert(submit_result.IsOk());

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 5: Token Minting
void TestTokenMinting() {
    std::cout << "Test 5: Token Minting... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Register wBTC
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;
    bridge.RegisterWrappedToken(wbtc);

    // Add validators and submit deposit proof
    for (int i = 0; i < 3; ++i) {
        BridgeValidator validator;
        validator.public_key = std::vector<uint8_t>(33, 0x02 + i);
        validator.stake = 2000000;
        validator.is_active = true;
        validator.joined_at = std::time(nullptr);
        bridge.AddValidator(validator);
    }

    DepositProof proof;
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x02));
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x03));
    proof.token = wbtc;

    auto proof_id_result = bridge.SubmitDepositProof(proof);
    assert(proof_id_result.IsOk());

    // Mint tokens
    std::vector<uint8_t> recipient(20, 0xDD);
    uint64_t amount = 100000000;  // 1 BTC

    auto mint_result = bridge.MintWrappedTokens(proof_id_result.GetValue(), recipient, amount, wbtc);
    assert(mint_result.IsOk());

    // Check balance
    auto balance_result = bridge.GetWrappedBalance(recipient, "wBTC");
    assert(balance_result.IsOk());
    assert(balance_result.GetValue() == amount);

    // Check total supply
    auto supply_result = bridge.GetWrappedSupply("wBTC");
    assert(supply_result.IsOk());
    assert(supply_result.GetValue() == amount);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 6: Withdrawal Request
void TestWithdrawalRequest() {
    std::cout << "Test 6: Withdrawal Request... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.fee_basis_points = 30;  // 0.3%
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Register wBTC and mint some tokens first
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;
    bridge.RegisterWrappedToken(wbtc);

    for (int i = 0; i < 3; ++i) {
        BridgeValidator validator;
        validator.public_key = std::vector<uint8_t>(33, 0x02 + i);
        validator.stake = 2000000;
        validator.is_active = true;
        validator.joined_at = std::time(nullptr);
        bridge.AddValidator(validator);
    }

    DepositProof proof;
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x02));
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x03));
    proof.token = wbtc;

    auto proof_id_result = bridge.SubmitDepositProof(proof);
    std::vector<uint8_t> user_address(20, 0xEE);
    bridge.MintWrappedTokens(proof_id_result.GetValue(), user_address, 100000000, wbtc);

    // Request withdrawal
    std::vector<uint8_t> destination(20, 0xFF);
    std::vector<uint8_t> signature = user_address;  // Simplified

    auto withdrawal_result = bridge.RequestWithdrawal(destination, 50000000, wbtc, signature);
    assert(withdrawal_result.IsOk());

    // Check balance after withdrawal (tokens burned)
    auto balance_result = bridge.GetWrappedBalance(user_address, "wBTC");
    assert(balance_result.IsOk());
    assert(balance_result.GetValue() == 50000000);  // Half withdrawn

    // Get withdrawal request
    auto withdrawal_info = bridge.GetWithdrawal(withdrawal_result.GetValue());
    assert(withdrawal_info.IsOk());
    assert(withdrawal_info.GetValue().amount == 50000000);
    assert(withdrawal_info.GetValue().status == BridgeStatus::PENDING);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 7: Withdrawal Execution
void TestWithdrawalExecution() {
    std::cout << "Test 7: Withdrawal Execution... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.fee_basis_points = 30;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Setup: Register token, add validators, mint tokens
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;
    bridge.RegisterWrappedToken(wbtc);

    for (int i = 0; i < 3; ++i) {
        BridgeValidator validator;
        validator.public_key = std::vector<uint8_t>(33, 0x02 + i);
        validator.stake = 2000000;
        validator.is_active = true;
        validator.joined_at = std::time(nullptr);
        bridge.AddValidator(validator);
    }

    DepositProof proof;
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x02));
    proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x03));
    proof.token = wbtc;

    auto proof_id = bridge.SubmitDepositProof(proof);
    std::vector<uint8_t> user_address(20, 0xEE);
    bridge.MintWrappedTokens(proof_id.GetValue(), user_address, 100000000, wbtc);

    // Request withdrawal
    std::vector<uint8_t> destination(20, 0xFF);
    auto withdrawal_id = bridge.RequestWithdrawal(destination, 50000000, wbtc, user_address);

    // Validators sign withdrawal
    bridge.SignWithdrawal(withdrawal_id.GetValue(), std::vector<uint8_t>(64, 0x11));
    bridge.SignWithdrawal(withdrawal_id.GetValue(), std::vector<uint8_t>(64, 0x22));

    // Execute withdrawal
    auto execute_result = bridge.ExecuteWithdrawal(withdrawal_id.GetValue());
    assert(execute_result.IsOk());

    // Verify status changed to EXECUTED
    auto withdrawal_info = bridge.GetWithdrawal(withdrawal_id.GetValue());
    assert(withdrawal_info.IsOk());
    assert(withdrawal_info.GetValue().status == BridgeStatus::EXECUTED);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 8: Emergency Pause
void TestEmergencyPause() {
    std::cout << "Test 8: Emergency Pause... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Verify not paused initially
    auto paused_result = bridge.IsPaused();
    assert(paused_result.IsOk());
    assert(paused_result.GetValue() == false);

    // Activate emergency pause
    auto pause_result = bridge.EmergencyPause();
    assert(pause_result.IsOk());

    // Verify paused
    auto paused_result2 = bridge.IsPaused();
    assert(paused_result2.IsOk());
    assert(paused_result2.GetValue() == true);

    // Resume
    auto resume_result = bridge.EmergencyResume();
    assert(resume_result.IsOk());

    // Verify resumed
    auto paused_result3 = bridge.IsPaused();
    assert(paused_result3.IsOk());
    assert(paused_result3.GetValue() == false);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 9: Bridge Chain Name Conversion
void TestBridgeChainNames() {
    std::cout << "Test 9: Bridge Chain Name Conversion... ";

    assert(BridgeChainToString(BridgeChain::INTCOIN) == "INTcoin");
    assert(BridgeChainToString(BridgeChain::BITCOIN) == "Bitcoin");
    assert(BridgeChainToString(BridgeChain::ETHEREUM) == "Ethereum");
    assert(BridgeChainToString(BridgeChain::LITECOIN) == "Litecoin");
    assert(BridgeChainToString(BridgeChain::TESTNET_INT) == "INTcoin Testnet");
    assert(BridgeChainToString(BridgeChain::TESTNET_BTC) == "Bitcoin Testnet");
    assert(BridgeChainToString(BridgeChain::TESTNET_ETH) == "Ethereum Testnet");
    assert(BridgeChainToString(BridgeChain::TESTNET_LTC) == "Litecoin Testnet");

    std::cout << "PASSED" << std::endl;
}

// Test 10: Bridge Status Names
void TestBridgeStatusNames() {
    std::cout << "Test 10: Bridge Status Name Conversion... ";

    assert(BridgeStatusToString(BridgeStatus::PENDING) == "Pending");
    assert(BridgeStatusToString(BridgeStatus::CONFIRMING) == "Confirming");
    assert(BridgeStatusToString(BridgeStatus::VALIDATED) == "Validated");
    assert(BridgeStatusToString(BridgeStatus::EXECUTED) == "Executed");
    assert(BridgeStatusToString(BridgeStatus::FAILED) == "Failed");
    assert(BridgeStatusToString(BridgeStatus::EXPIRED) == "Expired");

    std::cout << "PASSED" << std::endl;
}

// Test 11: Multiple Token Support
void TestMultipleTokens() {
    std::cout << "Test 11: Multiple Token Support... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 1000000;
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Register multiple tokens
    WrappedToken wbtc;
    wbtc.symbol = "wBTC";
    wbtc.origin_chain = BridgeChain::BITCOIN;
    wbtc.decimals = 8;
    wbtc.total_supply = 0;
    bridge.RegisterWrappedToken(wbtc);

    WrappedToken weth;
    weth.symbol = "wETH";
    weth.origin_chain = BridgeChain::ETHEREUM;
    weth.decimals = 18;
    weth.total_supply = 0;
    bridge.RegisterWrappedToken(weth);

    WrappedToken wltc;
    wltc.symbol = "wLTC";
    wltc.origin_chain = BridgeChain::LITECOIN;
    wltc.decimals = 8;
    wltc.total_supply = 0;
    bridge.RegisterWrappedToken(wltc);

    // Verify all registered
    auto tokens = bridge.GetWrappedTokens();
    assert(tokens.IsOk());
    assert(tokens.GetValue().size() == 3);

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

// Test 12: Validator Stake Requirement
void TestValidatorStakeRequirement() {
    std::cout << "Test 12: Validator Stake Requirement... ";

    INTcoinBridge bridge;

    BridgeConfig config;
    config.min_validators = 2;
    config.total_validators = 3;
    config.emergency_paused = false;
    config.min_validator_stake = 10000000;  // 0.1 BTC minimum
    config.withdrawal_timeout = 3600;

    bridge.Initialize(config);

    // Try to add validator with insufficient stake
    BridgeValidator validator;
    validator.public_key = std::vector<uint8_t>(33, 0x02);
    validator.stake = 5000000;  // Below minimum
    validator.is_active = true;
    validator.joined_at = std::time(nullptr);

    auto result = bridge.AddValidator(validator);
    assert(result.IsError());  // Should fail

    // Add validator with sufficient stake
    validator.stake = 15000000;  // Above minimum
    auto result2 = bridge.AddValidator(validator);
    assert(result2.IsOk());  // Should succeed

    bridge.Shutdown();

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "===============================================" << std::endl;
    std::cout << "     Bridge Test Suite" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << std::endl;

    try {
        TestBridgeInitialization();
        TestWrappedTokenRegistration();
        TestValidatorManagement();
        TestDepositProofSubmission();
        TestTokenMinting();
        TestWithdrawalRequest();
        TestWithdrawalExecution();
        TestEmergencyPause();
        TestBridgeChainNames();
        TestBridgeStatusNames();
        TestMultipleTokens();
        TestValidatorStakeRequirement();

        std::cout << std::endl;
        std::cout << "===============================================" << std::endl;
        std::cout << "     All tests PASSED! ✓" << std::endl;
        std::cout << "===============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << "FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
