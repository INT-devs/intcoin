// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/contracts/validator.h"
#include "intcoin/blockchain.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include "intcoin/contracts/storage.h"
#include <algorithm>

namespace intcoin {
namespace contracts {

// Helper: Convert PublicKey to Bech32 address
static std::string PublicKeyToAddress(const PublicKey& pubkey) {
    // Hash the public key
    uint256 hash = SHA3::Hash(std::vector<uint8_t>(pubkey.begin(), pubkey.end()));

    // Take first 20 bytes
    std::vector<uint8_t> hash_20(hash.begin(), hash.begin() + 20);

    // Encode as Bech32 with "int1" prefix
    return Bech32Encode("int1", hash_20);
}

// ============================================================================
// ContractTxValidator Implementation
// ============================================================================

ContractTxValidator::ContractTxValidator(const Blockchain& chain)
    : chain_(chain) {
    // Note: chain_ will be used for balance/nonce checks in future enhancements
    (void)chain_;
}

Result<void> ContractTxValidator::Validate(const Transaction& tx) const {
    if (!tx.IsContractTransaction()) {
        return Result<void>::Error("Not a contract transaction");
    }

    if (tx.IsContractDeployment()) {
        // Deserialize deployment transaction
        auto deploy_result = ContractDeploymentTx::Deserialize(tx.contract_data);
        if (!deploy_result.has_value()) {
            return Result<void>::Error("Failed to deserialize contract deployment transaction");
        }
        return ValidateDeployment(deploy_result.value());
    }

    if (tx.IsContractCall()) {
        // Deserialize call transaction
        auto call_result = ContractCallTx::Deserialize(tx.contract_data);
        if (!call_result.has_value()) {
            return Result<void>::Error("Failed to deserialize contract call transaction");
        }
        return ValidateCall(call_result.value());
    }

    return Result<void>::Error("Unknown contract transaction type");
}

Result<void> ContractTxValidator::ValidateDeployment(const ContractDeploymentTx& tx) const {
    // 1. Check bytecode is not empty
    if (tx.bytecode.empty()) {
        return Result<void>::Error("Contract bytecode is empty");
    }

    // 2. Check bytecode size limit (24 KB, EVM compatible)
    constexpr size_t MAX_BYTECODE_SIZE = 24 * 1024;
    if (tx.bytecode.size() > MAX_BYTECODE_SIZE) {
        return Result<void>::Error("Contract bytecode exceeds maximum size: " +
                                  std::to_string(tx.bytecode.size()) + " > " +
                                  std::to_string(MAX_BYTECODE_SIZE));
    }

    // 3. Verify signature
    if (!tx.Verify()) {
        return Result<void>::Error("Invalid signature on contract deployment transaction");
    }

    // 4. Check gas limit is sufficient for deployment
    // Base cost: 32,000 gas + 200 gas per byte of bytecode
    constexpr uint64_t BASE_DEPLOYMENT_GAS = 32000;
    constexpr uint64_t GAS_PER_BYTE = 200;
    uint64_t min_gas = BASE_DEPLOYMENT_GAS + (tx.bytecode.size() * GAS_PER_BYTE);

    if (tx.gas_limit < min_gas) {
        return Result<void>::Error("Gas limit too low for deployment: " +
                                  std::to_string(tx.gas_limit) + " < " +
                                  std::to_string(min_gas));
    }

    // 5. Check gas limit is not excessive (prevent DoS)
    constexpr uint64_t MAX_GAS_LIMIT = 30000000; // 30M gas per transaction
    if (tx.gas_limit > MAX_GAS_LIMIT) {
        return Result<void>::Error("Gas limit exceeds maximum: " +
                                  std::to_string(tx.gas_limit) + " > " +
                                  std::to_string(MAX_GAS_LIMIT));
    }

    // 6. Check gas price is reasonable
    constexpr uint64_t MIN_GAS_PRICE = 1; // 1 satINT per gas
    if (tx.gas_price < MIN_GAS_PRICE) {
        return Result<void>::Error("Gas price too low: " +
                                  std::to_string(tx.gas_price) + " < " +
                                  std::to_string(MIN_GAS_PRICE));
    }

    // Note: Balance and nonce checks are performed during block processing
    // when we have access to the current account state

    return Result<void>::Ok();
}

Result<void> ContractTxValidator::ValidateCall(const ContractCallTx& tx) const {
    // 1. Check contract address is valid Bech32
    if (tx.to.empty()) {
        return Result<void>::Error("Contract address is empty");
    }

    // Basic Bech32 format check (should start with "int1")
    if (tx.to.substr(0, 4) != "int1") {
        return Result<void>::Error("Invalid contract address format (must start with 'int1')");
    }

    // 2. Verify signature
    if (!tx.Verify()) {
        return Result<void>::Error("Invalid signature on contract call transaction");
    }

    // 3. Check gas limit is reasonable
    constexpr uint64_t MIN_GAS_LIMIT = 21000; // Minimum gas for any transaction
    constexpr uint64_t MAX_GAS_LIMIT = 30000000; // 30M gas per transaction

    if (tx.gas_limit < MIN_GAS_LIMIT) {
        return Result<void>::Error("Gas limit too low: " +
                                  std::to_string(tx.gas_limit) + " < " +
                                  std::to_string(MIN_GAS_LIMIT));
    }

    if (tx.gas_limit > MAX_GAS_LIMIT) {
        return Result<void>::Error("Gas limit exceeds maximum: " +
                                  std::to_string(tx.gas_limit) + " > " +
                                  std::to_string(MAX_GAS_LIMIT));
    }

    // 4. Check gas price is reasonable
    constexpr uint64_t MIN_GAS_PRICE = 1; // 1 satINT per gas
    if (tx.gas_price < MIN_GAS_PRICE) {
        return Result<void>::Error("Gas price too low: " +
                                  std::to_string(tx.gas_price) + " < " +
                                  std::to_string(MIN_GAS_PRICE));
    }

    // Note: Contract existence, balance, and nonce checks are performed
    // during block processing when we have access to the current state

    return Result<void>::Ok();
}

// ============================================================================
// ContractExecutor Implementation
// ============================================================================

ContractExecutor::ContractExecutor(ContractDatabase& db)
    : db_(db) {}

Result<TransactionReceipt> ContractExecutor::ExecuteDeployment(
    const ContractDeploymentTx& deploy_tx,
    const uint256& tx_hash,
    uint64_t block_height,
    uint64_t block_timestamp,
    uint32_t tx_index) {

    // Get contract address
    std::string contract_address = deploy_tx.GetContractAddress();

    // Convert deployer public key to address
    std::string from_address = PublicKeyToAddress(deploy_tx.from);

    // Create execution context
    ExecutionContext ctx;
    ctx.caller = from_address;
    ctx.origin = from_address;
    ctx.address = contract_address;
    ctx.value = deploy_tx.value;
    ctx.calldata = deploy_tx.constructor_args;
    ctx.gas_limit = deploy_tx.gas_limit;
    ctx.gas_price = deploy_tx.gas_price;
    ctx.block_number = block_height;
    ctx.block_timestamp = block_timestamp;

    // Create storage backend for this contract
    auto storage = std::make_shared<MemoryStorage>();

    // Create VM with storage
    IntSCVM vm(storage);

    // Execute constructor
    ExecutionResult result = vm.Execute(deploy_tx.bytecode, ctx);

    // Create contract account
    ContractAccount account;
    account.address = contract_address;
    account.balance = deploy_tx.value;
    account.nonce = 0;
    account.bytecode = deploy_tx.bytecode;

    // Calculate code hash
    uint256 code_hash = SHA3::Hash(deploy_tx.bytecode);
    account.code_hash = code_hash;

    // Storage root (simplified for now)
    account.storage_root.fill(0);

    account.creator = from_address;
    account.creation_tx = tx_hash;
    account.block_created = block_height;
    account.block_updated = block_height;

    // Store contract account
    auto store_result = db_.PutContractAccount(account);
    if (store_result.IsError()) {
        return Result<TransactionReceipt>::Error("Failed to store contract account: " + store_result.error);
    }

    // Create receipt
    TransactionReceipt receipt;
    receipt.txid = tx_hash;
    receipt.contract_address = contract_address;
    receipt.from = from_address;
    receipt.to = ""; // Empty for deployment
    receipt.gas_used = ctx.gas_limit - vm.GetGasRemaining();
    receipt.gas_price = deploy_tx.gas_price;
    receipt.total_fee = receipt.gas_used * receipt.gas_price;
    receipt.status = result;
    receipt.return_data = vm.GetReturnData();
    receipt.block_number = block_height;
    receipt.block_timestamp = block_timestamp;
    receipt.tx_index = tx_index;

    // Store event logs
    const auto& logs = vm.GetLogs();
    for (size_t i = 0; i < logs.size(); i++) {
        EventLogEntry entry;
        entry.contract_address = contract_address;
        entry.topics = logs[i].topics;
        entry.data = logs[i].data;
        entry.block_number = block_height;
        entry.transaction_hash = tx_hash;
        entry.log_index = static_cast<uint32_t>(i);

        auto log_result = db_.PutEventLog(entry);
        if (log_result.IsError()) {
            return Result<TransactionReceipt>::Error("Failed to store event log: " + log_result.error);
        }

        receipt.logs.push_back(entry);
    }

    // Store receipt
    auto receipt_result = db_.PutReceipt(receipt);
    if (receipt_result.IsError()) {
        return Result<TransactionReceipt>::Error("Failed to store receipt: " + receipt_result.error);
    }

    return Result<TransactionReceipt>::Ok(std::move(receipt));
}

Result<TransactionReceipt> ContractExecutor::ExecuteCall(
    const ContractCallTx& call_tx,
    const uint256& tx_hash,
    uint64_t block_height,
    uint64_t block_timestamp,
    uint32_t tx_index) {

    // Get contract account
    auto account_result = db_.GetContractAccount(call_tx.to);
    if (account_result.IsError()) {
        return Result<TransactionReceipt>::Error("Contract does not exist: " + call_tx.to);
    }

    ContractAccount account = account_result.GetValue();

    // Convert caller public key to address
    std::string from_address = PublicKeyToAddress(call_tx.from);

    // Create execution context
    ExecutionContext ctx;
    ctx.caller = from_address;
    ctx.origin = from_address;
    ctx.address = call_tx.to;
    ctx.value = call_tx.value;
    ctx.calldata = call_tx.data;
    ctx.gas_limit = call_tx.gas_limit;
    ctx.gas_price = call_tx.gas_price;
    ctx.block_number = block_height;
    ctx.block_timestamp = block_timestamp;

    // Create storage backend for this contract
    auto storage = std::make_shared<MemoryStorage>();

    // Create VM with storage
    IntSCVM vm(storage);

    // Execute contract
    ExecutionResult result = vm.Execute(account.bytecode, ctx);

    // Update account balance and timestamp
    account.balance += call_tx.value;
    account.block_updated = block_height;

    auto update_result = db_.PutContractAccount(account);
    if (update_result.IsError()) {
        return Result<TransactionReceipt>::Error("Failed to update contract account: " + update_result.error);
    }

    // Create receipt
    TransactionReceipt receipt;
    receipt.txid = tx_hash;
    receipt.contract_address = ""; // Empty for call
    receipt.from = from_address;
    receipt.to = call_tx.to;
    receipt.gas_used = ctx.gas_limit - vm.GetGasRemaining();
    receipt.gas_price = call_tx.gas_price;
    receipt.total_fee = receipt.gas_used * receipt.gas_price;
    receipt.status = result;
    receipt.return_data = vm.GetReturnData();
    receipt.block_number = block_height;
    receipt.block_timestamp = block_timestamp;
    receipt.tx_index = tx_index;

    // Store event logs
    const auto& logs = vm.GetLogs();
    for (size_t i = 0; i < logs.size(); i++) {
        EventLogEntry entry;
        entry.contract_address = call_tx.to;
        entry.topics = logs[i].topics;
        entry.data = logs[i].data;
        entry.block_number = block_height;
        entry.transaction_hash = tx_hash;
        entry.log_index = static_cast<uint32_t>(i);

        auto log_result = db_.PutEventLog(entry);
        if (log_result.IsError()) {
            return Result<TransactionReceipt>::Error("Failed to store event log: " + log_result.error);
        }

        receipt.logs.push_back(entry);
    }

    // Store receipt
    auto receipt_result = db_.PutReceipt(receipt);
    if (receipt_result.IsError()) {
        return Result<TransactionReceipt>::Error("Failed to store receipt: " + receipt_result.error);
    }

    return Result<TransactionReceipt>::Ok(std::move(receipt));
}

} // namespace contracts
} // namespace intcoin
