/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Transaction and Block Validation
 */

#include "intcoin/blockchain.h"
#include "intcoin/consensus.h"
#include "intcoin/util.h"
#include <algorithm>
#include <set>

namespace intcoin {

// ============================================================================
// BlockValidator Implementation
// ============================================================================

BlockValidator::BlockValidator(const Blockchain& chain)
    : chain_(chain) {}

Result<void> BlockValidator::Validate(const Block& block) const {
    // 1. Validate header
    auto header_result = ValidateHeader(block.header);
    if (header_result.IsError()) {
        return header_result;
    }

    // 2. Validate merkle root
    auto merkle_result = ValidateMerkleRoot(block);
    if (merkle_result.IsError()) {
        return merkle_result;
    }

    // 3. Validate proof of work
    auto pow_result = ValidateProofOfWork(block.header);
    if (pow_result.IsError()) {
        return pow_result;
    }

    // 4. Validate timestamp
    auto timestamp_result = ValidateTimestamp(block.header);
    if (timestamp_result.IsError()) {
        return timestamp_result;
    }

    // 5. Validate difficulty
    auto difficulty_result = ValidateDifficulty(block.header);
    if (difficulty_result.IsError()) {
        return difficulty_result;
    }

    // 6. Validate transactions
    auto tx_result = ValidateTransactions(block);
    if (tx_result.IsError()) {
        return tx_result;
    }

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateHeader(const BlockHeader& header) const {
    // Check version
    if (header.version == 0) {
        return Result<void>::Error("Invalid block version: 0");
    }

    // Check timestamp is not too far in the future
    uint64_t current_time = std::time(nullptr);
    if (header.timestamp > current_time + consensus::MAX_FUTURE_BLOCK_TIME) {
        return Result<void>::Error("Block timestamp too far in future");
    }

    // Check bits is valid
    if (header.bits == 0) {
        return Result<void>::Error("Invalid difficulty bits: 0");
    }

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateTransactions(const Block& block) const {
    if (block.transactions.empty()) {
        return Result<void>::Error("Block has no transactions");
    }

    // First transaction must be coinbase
    if (!block.transactions[0].IsCoinbase()) {
        return Result<void>::Error("First transaction is not coinbase");
    }

    // Only first transaction can be coinbase
    for (size_t i = 1; i < block.transactions.size(); i++) {
        if (block.transactions[i].IsCoinbase()) {
            return Result<void>::Error("Non-first transaction is coinbase");
        }
    }

    // Validate each transaction
    TxValidator tx_validator(chain_);
    for (size_t i = 0; i < block.transactions.size(); i++) {
        auto result = tx_validator.Validate(block.transactions[i]);
        if (result.IsError()) {
            return Result<void>::Error("Transaction " + std::to_string(i) + " invalid: " + result.error);
        }
    }

    // Validate coinbase reward
    uint64_t height = chain_.GetBestHeight() + 1;
    uint64_t expected_reward = GetBlockReward(height);

    uint64_t total_fees = 0;
    // TODO: Calculate total fees from transactions

    uint64_t coinbase_value = block.transactions[0].GetTotalOutputValue();
    if (coinbase_value > expected_reward + total_fees) {
        return Result<void>::Error("Coinbase value exceeds reward + fees");
    }

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateMerkleRoot(const Block& block) const {
    uint256 calculated = block.CalculateMerkleRoot();
    if (calculated != block.header.merkle_root) {
        return Result<void>::Error("Invalid merkle root");
    }
    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateProofOfWork(const BlockHeader& header) const {
    // Get target from bits
    uint256 target = DifficultyCalculator::CompactToTarget(header.bits);
    (void)target; // Suppress unused warning for now

    // Calculate block hash (PoW hash)
    uint256 block_hash = header.GetHash();

    // Check if hash meets target
    if (!DifficultyCalculator::CheckProofOfWork(block_hash, header.bits)) {
        return Result<void>::Error("Proof of work failed");
    }

    // TODO: Validate RandomX hash
    // For now, we just check the basic PoW

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateTimestamp(const BlockHeader& header) const {
    // Check timestamp is not too far in the future
    uint64_t current_time = std::time(nullptr);
    if (header.timestamp > current_time + consensus::MAX_FUTURE_BLOCK_TIME) {
        return Result<void>::Error("Block timestamp too far in future");
    }

    // Check timestamp is greater than median of last 11 blocks
    // TODO: Implement median time past check
    // For now, just check it's not zero
    if (header.timestamp == 0) {
        return Result<void>::Error("Block timestamp is zero");
    }

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateDifficulty(const BlockHeader& header) const {
    // Get expected difficulty for next block
    auto best_block_result = chain_.GetBestBlock();
    if (best_block_result.IsError()) {
        // If no best block, this is genesis, accept minimum difficulty
        if (header.bits == consensus::MIN_DIFFICULTY_BITS) {
            return Result<void>::Ok();
        }
        return Result<void>::Error("Invalid difficulty for genesis block");
    }

    // Calculate required difficulty using Digishield V3
    uint32_t required_bits = DifficultyCalculator::GetNextWorkRequired(
        best_block_result.value->header, chain_);

    if (header.bits != required_bits) {
        return Result<void>::Error("Invalid difficulty bits");
    }

    return Result<void>::Ok();
}

// ============================================================================
// TxValidator Implementation
// ============================================================================

TxValidator::TxValidator(const Blockchain& chain)
    : chain_(chain) {}

Result<void> TxValidator::Validate(const Transaction& tx) const {
    // 1. Validate structure
    auto structure_result = ValidateStructure(tx);
    if (structure_result.IsError()) {
        return structure_result;
    }

    // Skip further validation for coinbase
    if (tx.IsCoinbase()) {
        return Result<void>::Ok();
    }

    // 2. Validate inputs
    auto inputs_result = ValidateInputs(tx);
    if (inputs_result.IsError()) {
        return inputs_result;
    }

    // 3. Validate outputs
    auto outputs_result = ValidateOutputs(tx);
    if (outputs_result.IsError()) {
        return outputs_result;
    }

    // 4. Validate fees
    auto fees_result = ValidateFees(tx);
    if (fees_result.IsError()) {
        return fees_result;
    }

    // 5. Check double spend
    auto double_spend_result = CheckDoubleSpend(tx);
    if (double_spend_result.IsError()) {
        return double_spend_result;
    }

    // 6. Validate signature
    auto signature_result = ValidateSignature(tx);
    if (signature_result.IsError()) {
        return signature_result;
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateStructure(const Transaction& tx) const {
    // Check version
    if (tx.version == 0) {
        return Result<void>::Error("Invalid transaction version: 0");
    }

    // Check inputs
    if (!tx.IsCoinbase() && tx.inputs.empty()) {
        return Result<void>::Error("Non-coinbase transaction has no inputs");
    }

    // Check outputs
    if (tx.outputs.empty()) {
        return Result<void>::Error("Transaction has no outputs");
    }

    // Check for duplicate inputs
    std::set<OutPoint> seen_outpoints;
    for (const auto& input : tx.inputs) {
        OutPoint outpoint;
        outpoint.tx_hash = input.prev_tx_hash;
        outpoint.index = input.prev_tx_index;

        if (seen_outpoints.count(outpoint)) {
            return Result<void>::Error("Duplicate input in transaction");
        }
        seen_outpoints.insert(outpoint);
    }

    // Check output values are not negative (uint64_t can't be negative, but check for overflow)
    uint64_t total_output = 0;
    for (const auto& output : tx.outputs) {
        if (output.value == 0) {
            return Result<void>::Error("Output value is zero");
        }

        if (total_output + output.value < total_output) {
            return Result<void>::Error("Output value overflow");
        }
        total_output += output.value;
    }

    // Check total output doesn't exceed max supply
    if (total_output > consensus::MAX_SUPPLY) {
        return Result<void>::Error("Total output exceeds max supply");
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateInputs(const Transaction& tx) const {
    if (tx.IsCoinbase()) {
        return Result<void>::Ok();
    }

    // Validate each input
    for (size_t i = 0; i < tx.inputs.size(); i++) {
        const auto& input = tx.inputs[i];

        // Check that previous output exists (UTXO validation)
        OutPoint outpoint;
        outpoint.tx_hash = input.prev_tx_hash;
        outpoint.index = input.prev_tx_index;

        auto utxo = chain_.GetUTXO(outpoint);
        if (!utxo.has_value()) {
            return Result<void>::Error("Input " + std::to_string(i) + " references non-existent UTXO");
        }

        // TODO: Validate script_sig against script_pubkey
        // For now, we just check the UTXO exists
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateOutputs(const Transaction& tx) const {
    for (size_t i = 0; i < tx.outputs.size(); i++) {
        const auto& output = tx.outputs[i];

        // Check value is not zero
        if (output.value == 0) {
            return Result<void>::Error("Output " + std::to_string(i) + " has zero value");
        }

        // Check script is valid
        if (output.script_pubkey.IsEmpty()) {
            return Result<void>::Error("Output " + std::to_string(i) + " has empty script");
        }

        // TODO: Validate script is well-formed
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateSignature(const Transaction& tx) const {
    // TODO: Implement Dilithium signature validation
    // For now, this is a stub
    (void)tx;
    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateFees(const Transaction& tx) const {
    if (tx.IsCoinbase()) {
        return Result<void>::Ok();
    }

    // Calculate total input value
    uint64_t total_input = 0;
    for (const auto& input : tx.inputs) {
        OutPoint outpoint;
        outpoint.tx_hash = input.prev_tx_hash;
        outpoint.index = input.prev_tx_index;

        auto utxo = chain_.GetUTXO(outpoint);
        if (!utxo.has_value()) {
            return Result<void>::Error("Input references non-existent UTXO");
        }

        total_input += utxo->value;
    }

    // Calculate total output value
    uint64_t total_output = tx.GetTotalOutputValue();

    // Check input >= output (fee is the difference)
    if (total_input < total_output) {
        return Result<void>::Error("Total input less than total output");
    }

    // Check fee is reasonable (not excessive)
    uint64_t fee = total_input - total_output;
    uint64_t max_fee = total_input / 2; // Max fee is 50% of input

    if (fee > max_fee) {
        return Result<void>::Error("Transaction fee too high");
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::CheckDoubleSpend(const Transaction& tx) const {
    if (tx.IsCoinbase()) {
        return Result<void>::Ok();
    }

    // Check each input is not already spent
    for (const auto& input : tx.inputs) {
        OutPoint outpoint;
        outpoint.tx_hash = input.prev_tx_hash;
        outpoint.index = input.prev_tx_index;

        // Check if UTXO exists
        if (!chain_.HasUTXO(outpoint)) {
            return Result<void>::Error("Double spend detected: UTXO already spent");
        }
    }

    return Result<void>::Ok();
}

} // namespace intcoin
