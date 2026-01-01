// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/bridge.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace bridge {

// Helper functions
std::string BridgeChainToString(BridgeChain chain) {
    switch (chain) {
        case BridgeChain::INTCOIN:      return "INTcoin";
        case BridgeChain::BITCOIN:      return "Bitcoin";
        case BridgeChain::ETHEREUM:     return "Ethereum";
        case BridgeChain::LITECOIN:     return "Litecoin";
        case BridgeChain::TESTNET_INT:  return "INTcoin Testnet";
        case BridgeChain::TESTNET_BTC:  return "Bitcoin Testnet";
        case BridgeChain::TESTNET_ETH:  return "Ethereum Testnet";
        case BridgeChain::TESTNET_LTC:  return "Litecoin Testnet";
    }
    return "Unknown";
}

std::string BridgeStatusToString(BridgeStatus status) {
    switch (status) {
        case BridgeStatus::PENDING:     return "Pending";
        case BridgeStatus::CONFIRMING:  return "Confirming";
        case BridgeStatus::VALIDATED:   return "Validated";
        case BridgeStatus::EXECUTED:    return "Executed";
        case BridgeStatus::FAILED:      return "Failed";
        case BridgeStatus::EXPIRED:     return "Expired";
    }
    return "Unknown";
}

std::string BridgeOperationToString(BridgeOperation op) {
    switch (op) {
        case BridgeOperation::DEPOSIT:           return "Deposit";
        case BridgeOperation::WITHDRAW:          return "Withdraw";
        case BridgeOperation::VALIDATOR_ADD:     return "Add Validator";
        case BridgeOperation::VALIDATOR_REMOVE:  return "Remove Validator";
        case BridgeOperation::EMERGENCY_PAUSE:   return "Emergency Pause";
        case BridgeOperation::EMERGENCY_RESUME:  return "Emergency Resume";
    }
    return "Unknown";
}

// INTcoinBridge implementation
struct INTcoinBridge::Impl {
    BridgeConfig config;
    bool is_initialized;
    std::mutex mutex;

    // Storage
    std::unordered_map<std::string, WrappedToken> wrapped_tokens;
    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> balances; // address -> token -> amount
    std::unordered_map<std::string, BridgeValidator> validators; // pubkey_hex -> validator
    std::unordered_map<std::string, DepositProof> deposit_proofs; // proof_id_hex -> proof
    std::unordered_map<std::string, WithdrawalRequest> withdrawals; // withdrawal_id_hex -> request

    // Callbacks
    DepositDetectedCallback deposit_callback;
    WithdrawalRequestedCallback withdrawal_callback;
    ValidatorSignedCallback validator_signed_callback;
    BridgeExecutedCallback bridge_executed_callback;

    Impl() : is_initialized(false) {}

    // Helper: Convert bytes to hex
    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }

    // Helper: Convert uint256 to hex
    std::string Uint256ToHex(const uint256& hash) {
        return BytesToHex(std::vector<uint8_t>(hash.begin(), hash.end()));
    }

    // Helper: Generate unique ID
    uint256 GenerateID() {
        std::vector<uint8_t> random_data(32);
        for (size_t i = 0; i < 32; ++i) {
            random_data[i] = static_cast<uint8_t>(rand() % 256);
        }
        uint256 id = SHA3_256(random_data);
        return id;
    }

    // Helper: Verify validator signature count meets threshold
    bool HasValidatorThreshold(const std::vector<std::vector<uint8_t>>& signatures) {
        return signatures.size() >= config.min_validators;
    }

    // Helper: Get balance key
    std::string GetBalanceKey(const std::vector<uint8_t>& address, const std::string& token_symbol) {
        return BytesToHex(address) + ":" + token_symbol;
    }
};

INTcoinBridge::INTcoinBridge() : impl_(std::make_unique<Impl>()) {}

INTcoinBridge::~INTcoinBridge() {
    Shutdown();
}

Result<void> INTcoinBridge::Initialize(const BridgeConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->is_initialized) {
        return Result<void>::Error("Bridge already initialized");
    }

    // Validate config
    if (config.min_validators == 0 || config.total_validators == 0) {
        return Result<void>::Error("Invalid validator configuration");
    }

    if (config.min_validators > config.total_validators) {
        return Result<void>::Error("min_validators cannot exceed total_validators");
    }

    impl_->config = config;
    impl_->is_initialized = true;

    LogF(LogLevel::INFO, "Bridge: Initialized with M-of-N = %u-of-%u validators",
         config.min_validators, config.total_validators);

    return Result<void>::Ok();
}

Result<void> INTcoinBridge::Shutdown() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Ok();
    }

    impl_->is_initialized = false;
    LogF(LogLevel::INFO, "Bridge: Shutdown complete");

    return Result<void>::Ok();
}

Result<uint256> INTcoinBridge::SubmitDepositProof(const DepositProof& proof) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint256>::Error("Bridge not initialized");
    }

    if (impl_->config.emergency_paused) {
        return Result<uint256>::Error("Bridge is paused");
    }

    // Verify proof validity
    auto verify_result = VerifyDepositProof(proof);
    if (verify_result.IsError()) {
        return Result<uint256>::Error("Invalid deposit proof: " + verify_result.error);
    }

    // Generate proof ID
    uint256 proof_id = impl_->GenerateID();

    // Store deposit proof
    impl_->deposit_proofs[impl_->Uint256ToHex(proof_id)] = proof;

    // Trigger callback
    if (impl_->deposit_callback) {
        impl_->deposit_callback(proof);
    }

    LogF(LogLevel::INFO, "Bridge: Deposit proof submitted for %s %s",
         proof.token.symbol.c_str(), BridgeChainToString(proof.token.origin_chain).c_str());

    return Result<uint256>::Ok(proof_id);
}

Result<bool> INTcoinBridge::VerifyDepositProof(const DepositProof& proof) {
    // Verify validator signatures meet threshold
    if (!impl_->HasValidatorThreshold(proof.validator_signatures)) {
        return Result<bool>::Error("Insufficient validator signatures");
    }

    // Verify all signatures are from active validators
    for (const auto& sig : proof.validator_signatures) {
        std::string pubkey_hex = impl_->BytesToHex(sig);
        if (impl_->validators.find(pubkey_hex) == impl_->validators.end()) {
            return Result<bool>::Error("Invalid validator signature");
        }
    }

    // TODO: Verify Merkle proof (SPV verification)
    // For now, assume proof is valid if validators signed it

    return Result<bool>::Ok(true);
}

Result<void> INTcoinBridge::MintWrappedTokens(
    const uint256& proof_id,
    const std::vector<uint8_t>& recipient,
    uint64_t amount,
    const WrappedToken& token) {

    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    // Verify proof exists
    std::string proof_hex = impl_->Uint256ToHex(proof_id);
    if (impl_->deposit_proofs.find(proof_hex) == impl_->deposit_proofs.end()) {
        return Result<void>::Error("Deposit proof not found");
    }

    // Check if token is registered
    if (impl_->wrapped_tokens.find(token.symbol) == impl_->wrapped_tokens.end()) {
        return Result<void>::Error("Token not registered: " + token.symbol);
    }

    // Mint tokens (increase balance)
    std::string address_hex = impl_->BytesToHex(recipient);
    if (impl_->balances.find(address_hex) == impl_->balances.end()) {
        impl_->balances[address_hex] = {};
    }

    impl_->balances[address_hex][token.symbol] += amount;

    // Update total supply
    impl_->wrapped_tokens[token.symbol].total_supply += amount;

    LogF(LogLevel::INFO, "Bridge: Minted %lu %s to address %s",
         amount, token.symbol.c_str(), address_hex.substr(0, 16).c_str());

    return Result<void>::Ok();
}

Result<uint256> INTcoinBridge::RequestWithdrawal(
    const std::vector<uint8_t>& destination,
    uint64_t amount,
    const WrappedToken& token,
    const std::vector<uint8_t>& requester_signature) {

    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint256>::Error("Bridge not initialized");
    }

    if (impl_->config.emergency_paused) {
        return Result<uint256>::Error("Bridge is paused");
    }

    // Check token is registered
    if (impl_->wrapped_tokens.find(token.symbol) == impl_->wrapped_tokens.end()) {
        return Result<uint256>::Error("Token not registered: " + token.symbol);
    }

    // TODO: Verify requester signature and extract address
    // For now, extract address from signature (simplified)
    std::vector<uint8_t> requester_address = requester_signature; // Placeholder

    // Check balance
    std::string address_hex = impl_->BytesToHex(requester_address);
    if (impl_->balances.find(address_hex) == impl_->balances.end() ||
        impl_->balances[address_hex].find(token.symbol) == impl_->balances[address_hex].end()) {
        return Result<uint256>::Error("Insufficient balance");
    }

    uint64_t balance = impl_->balances[address_hex][token.symbol];
    if (balance < amount) {
        return Result<uint256>::Error("Insufficient balance: " + std::to_string(balance));
    }

    // Create withdrawal request
    WithdrawalRequest request;
    request.withdrawal_id = impl_->GenerateID();
    request.requester_address = requester_address;
    request.destination_address = destination;
    request.amount = amount;
    request.token = token;
    request.bridge_fee = (amount * impl_->config.fee_basis_points) / 10000;
    request.status = BridgeStatus::PENDING;
    request.timestamp = std::time(nullptr);
    request.expiration = request.timestamp + impl_->config.withdrawal_timeout;

    // Burn tokens (decrease balance)
    impl_->balances[address_hex][token.symbol] -= amount;
    impl_->wrapped_tokens[token.symbol].total_supply -= amount;

    // Store withdrawal request
    impl_->withdrawals[impl_->Uint256ToHex(request.withdrawal_id)] = request;

    // Trigger callback
    if (impl_->withdrawal_callback) {
        impl_->withdrawal_callback(request);
    }

    LogF(LogLevel::INFO, "Bridge: Withdrawal requested - %lu %s to %s",
         amount, token.symbol.c_str(), BridgeChainToString(token.origin_chain).c_str());

    return Result<uint256>::Ok(request.withdrawal_id);
}

Result<void> INTcoinBridge::SignWithdrawal(
    const uint256& withdrawal_id,
    const std::vector<uint8_t>& validator_signature) {

    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    // Get withdrawal request
    std::string withdrawal_hex = impl_->Uint256ToHex(withdrawal_id);
    if (impl_->withdrawals.find(withdrawal_hex) == impl_->withdrawals.end()) {
        return Result<void>::Error("Withdrawal not found");
    }

    // Add signature
    impl_->withdrawals[withdrawal_hex].validator_signatures.push_back(validator_signature);

    // Check if threshold reached
    if (impl_->HasValidatorThreshold(impl_->withdrawals[withdrawal_hex].validator_signatures)) {
        impl_->withdrawals[withdrawal_hex].status = BridgeStatus::VALIDATED;

        LogF(LogLevel::INFO, "Bridge: Withdrawal %s reached validator threshold",
             withdrawal_hex.substr(0, 16).c_str());
    }

    // Trigger callback
    if (impl_->validator_signed_callback) {
        impl_->validator_signed_callback(withdrawal_id, validator_signature);
    }

    return Result<void>::Ok();
}

Result<uint256> INTcoinBridge::ExecuteWithdrawal(const uint256& withdrawal_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint256>::Error("Bridge not initialized");
    }

    // Get withdrawal request
    std::string withdrawal_hex = impl_->Uint256ToHex(withdrawal_id);
    if (impl_->withdrawals.find(withdrawal_hex) == impl_->withdrawals.end()) {
        return Result<uint256>::Error("Withdrawal not found");
    }

    auto& withdrawal = impl_->withdrawals[withdrawal_hex];

    // Verify status
    if (withdrawal.status != BridgeStatus::VALIDATED) {
        return Result<uint256>::Error("Withdrawal not validated");
    }

    // Check expiration
    uint64_t now = std::time(nullptr);
    if (now > withdrawal.expiration) {
        withdrawal.status = BridgeStatus::EXPIRED;
        return Result<uint256>::Error("Withdrawal expired");
    }

    // TODO: Execute transaction on target chain
    // For now, mark as executed
    withdrawal.target_tx_hash = impl_->GenerateID();
    withdrawal.status = BridgeStatus::EXECUTED;

    // Trigger callback
    if (impl_->bridge_executed_callback) {
        impl_->bridge_executed_callback(withdrawal_id, true);
    }

    LogF(LogLevel::INFO, "Bridge: Withdrawal executed - %s",
         withdrawal_hex.substr(0, 16).c_str());

    return Result<uint256>::Ok(withdrawal.target_tx_hash);
}

Result<WithdrawalRequest> INTcoinBridge::GetWithdrawal(const uint256& withdrawal_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::string withdrawal_hex = impl_->Uint256ToHex(withdrawal_id);
    if (impl_->withdrawals.find(withdrawal_hex) == impl_->withdrawals.end()) {
        return Result<WithdrawalRequest>::Error("Withdrawal not found");
    }

    return Result<WithdrawalRequest>::Ok(impl_->withdrawals[withdrawal_hex]);
}

Result<void> INTcoinBridge::RegisterWrappedToken(const WrappedToken& token) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    // Check if already registered
    if (impl_->wrapped_tokens.find(token.symbol) != impl_->wrapped_tokens.end()) {
        return Result<void>::Error("Token already registered: " + token.symbol);
    }

    impl_->wrapped_tokens[token.symbol] = token;

    LogF(LogLevel::INFO, "Bridge: Registered wrapped token %s (%s)",
         token.symbol.c_str(), BridgeChainToString(token.origin_chain).c_str());

    return Result<void>::Ok();
}

Result<uint64_t> INTcoinBridge::GetWrappedBalance(
    const std::vector<uint8_t>& address,
    const std::string& token_symbol) {

    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::string address_hex = impl_->BytesToHex(address);

    if (impl_->balances.find(address_hex) == impl_->balances.end() ||
        impl_->balances[address_hex].find(token_symbol) == impl_->balances[address_hex].end()) {
        return Result<uint64_t>::Ok(0);
    }

    return Result<uint64_t>::Ok(impl_->balances[address_hex][token_symbol]);
}

Result<uint64_t> INTcoinBridge::GetWrappedSupply(const std::string& token_symbol) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->wrapped_tokens.find(token_symbol) == impl_->wrapped_tokens.end()) {
        return Result<uint64_t>::Error("Token not registered: " + token_symbol);
    }

    return Result<uint64_t>::Ok(impl_->wrapped_tokens[token_symbol].total_supply);
}

Result<std::vector<WrappedToken>> INTcoinBridge::GetWrappedTokens() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<WrappedToken> tokens;
    for (const auto& pair : impl_->wrapped_tokens) {
        tokens.push_back(pair.second);
    }

    return Result<std::vector<WrappedToken>>::Ok(tokens);
}

Result<void> INTcoinBridge::AddValidator(const BridgeValidator& validator) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    std::string pubkey_hex = impl_->BytesToHex(validator.public_key);

    // Check if already exists
    if (impl_->validators.find(pubkey_hex) != impl_->validators.end()) {
        return Result<void>::Error("Validator already exists");
    }

    // Verify stake meets minimum
    if (validator.stake < impl_->config.min_validator_stake) {
        return Result<void>::Error("Insufficient stake");
    }

    impl_->validators[pubkey_hex] = validator;

    LogF(LogLevel::INFO, "Bridge: Added validator %s (stake: %lu)",
         pubkey_hex.substr(0, 16).c_str(), validator.stake);

    return Result<void>::Ok();
}

Result<void> INTcoinBridge::RemoveValidator(const std::vector<uint8_t>& validator_pubkey) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    std::string pubkey_hex = impl_->BytesToHex(validator_pubkey);

    if (impl_->validators.find(pubkey_hex) == impl_->validators.end()) {
        return Result<void>::Error("Validator not found");
    }

    impl_->validators.erase(pubkey_hex);

    LogF(LogLevel::INFO, "Bridge: Removed validator %s", pubkey_hex.substr(0, 16).c_str());

    return Result<void>::Ok();
}

Result<std::vector<BridgeValidator>> INTcoinBridge::GetValidators() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<BridgeValidator> validators;
    for (const auto& pair : impl_->validators) {
        if (pair.second.is_active) {
            validators.push_back(pair.second);
        }
    }

    return Result<std::vector<BridgeValidator>>::Ok(validators);
}

Result<bool> INTcoinBridge::IsValidator(const std::vector<uint8_t>& pubkey) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::string pubkey_hex = impl_->BytesToHex(pubkey);
    bool is_validator = impl_->validators.find(pubkey_hex) != impl_->validators.end();

    return Result<bool>::Ok(is_validator);
}

Result<void> INTcoinBridge::EmergencyPause() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    impl_->config.emergency_paused = true;

    LogF(LogLevel::WARNING, "Bridge: EMERGENCY PAUSE activated");

    return Result<void>::Ok();
}

Result<void> INTcoinBridge::EmergencyResume() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    impl_->config.emergency_paused = false;

    LogF(LogLevel::INFO, "Bridge: Emergency pause lifted, operations resumed");

    return Result<void>::Ok();
}

Result<bool> INTcoinBridge::IsPaused() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    return Result<bool>::Ok(impl_->config.emergency_paused);
}

Result<BridgeConfig> INTcoinBridge::GetConfig() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<BridgeConfig>::Error("Bridge not initialized");
    }

    return Result<BridgeConfig>::Ok(impl_->config);
}

Result<void> INTcoinBridge::UpdateConfig(const BridgeConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Bridge not initialized");
    }

    // Validate new config
    if (config.min_validators > config.total_validators) {
        return Result<void>::Error("Invalid validator configuration");
    }

    impl_->config = config;

    LogF(LogLevel::INFO, "Bridge: Configuration updated");

    return Result<void>::Ok();
}

void INTcoinBridge::OnDepositDetected(DepositDetectedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->deposit_callback = callback;
}

void INTcoinBridge::OnWithdrawalRequested(WithdrawalRequestedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->withdrawal_callback = callback;
}

void INTcoinBridge::OnValidatorSigned(ValidatorSignedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->validator_signed_callback = callback;
}

void INTcoinBridge::OnBridgeExecuted(BridgeExecutedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->bridge_executed_callback = callback;
}

}  // namespace bridge
}  // namespace intcoin
