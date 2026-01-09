/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Transaction and Block Validation
 */

#include "intcoin/blockchain.h"
#include "intcoin/consensus.h"
#include "intcoin/util.h"
#include "intcoin/contracts/validator.h"
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

    // Calculate total fees from all non-coinbase transactions
    uint64_t total_fees = 0;
    for (size_t i = 1; i < block.transactions.size(); i++) {
        const auto& tx = block.transactions[i];

        // Calculate total input value
        uint64_t total_input = 0;
        for (const auto& input : tx.inputs) {
            OutPoint outpoint;
            outpoint.tx_hash = input.prev_tx_hash;
            outpoint.index = input.prev_tx_index;

            auto utxo = chain_.GetUTXO(outpoint);
            if (utxo.has_value()) {
                total_input += utxo->value;
            }
        }

        // Calculate total output value
        uint64_t total_output = tx.GetTotalOutputValue();

        // Fee = input - output
        if (total_input >= total_output) {
            total_fees += (total_input - total_output);
        }
    }

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

    // Validate RandomX hash (ASIC-resistant mining)
    auto randomx_result = RandomXValidator::ValidateBlockHash(header);
    if (randomx_result.IsError()) {
        return Result<void>::Error("RandomX validation failed: " + randomx_result.error);
    }

    return Result<void>::Ok();
}

Result<void> BlockValidator::ValidateTimestamp(const BlockHeader& header) const {
    // Check timestamp is not too far in the future
    uint64_t current_time = std::time(nullptr);
    if (header.timestamp > current_time + consensus::MAX_FUTURE_BLOCK_TIME) {
        return Result<void>::Error("Block timestamp too far in future");
    }

    // Check timestamp is greater than median of last 11 blocks (BIP 113)
    uint64_t height = chain_.GetBestHeight() + 1;
    if (height > 0) {  // Skip genesis block
        uint64_t median_time_past = ConsensusValidator::GetMedianTimePast(chain_, height, 11);
        if (header.timestamp <= median_time_past) {
            return Result<void>::Error("Block timestamp must be greater than median time past");
        }
    }

    // Check timestamp is not zero
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
    // Handle contract transactions separately
    if (tx.IsContractTransaction()) {
        contracts::ContractTxValidator contract_validator(chain_);
        return contract_validator.Validate(tx);
    }

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

        // Validate script_sig against script_pubkey
        auto script_result = ExecuteScript(input.script_sig, utxo->script_pubkey, tx, i);
        if (!script_result.success) {
            return Result<void>::Error("Input " + std::to_string(i) + " script validation failed: " +
                                      script_result.error);
        }
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

        // Validate script is well-formed
        auto script_validation = ValidateScript(output.script_pubkey, i);
        if (script_validation.IsError()) {
            return script_validation;
        }
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateScript(const Script& script, size_t output_index) const {
    const auto& bytes = script.Serialize();

    // Maximum script size (10,000 bytes, same as Bitcoin)
    constexpr size_t MAX_SCRIPT_SIZE = 10000;
    if (bytes.size() > MAX_SCRIPT_SIZE) {
        return Result<void>::Error("Output " + std::to_string(output_index) +
                                  " script exceeds maximum size: " +
                                  std::to_string(bytes.size()) + " > " +
                                  std::to_string(MAX_SCRIPT_SIZE));
    }

    // Validate script opcodes
    size_t pc = 0; // Program counter
    int if_depth = 0; // Track IF/ENDIF nesting depth

    while (pc < bytes.size()) {
        uint8_t opcode = bytes[pc];
        pc++;

        // Check for push data operations
        if (opcode >= 0x01 && opcode <= 0x4B) {
            // Push N bytes (opcode is the length)
            size_t push_size = opcode;

            if (pc + push_size > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete push data at position " +
                                          std::to_string(pc - 1));
            }

            pc += push_size;
            continue;
        }

        // Special push data opcodes
        if (opcode == 0x4C) { // OP_PUSHDATA1
            if (pc >= bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA1");
            }

            size_t push_size = bytes[pc++];
            if (pc + push_size > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA1 data");
            }

            pc += push_size;
            continue;
        }

        if (opcode == 0x4D) { // OP_PUSHDATA2
            if (pc + 2 > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA2");
            }

            size_t push_size = bytes[pc] | (bytes[pc + 1] << 8);
            pc += 2;

            if (pc + push_size > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA2 data");
            }

            pc += push_size;
            continue;
        }

        if (opcode == 0x4E) { // OP_PUSHDATA4
            if (pc + 4 > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA4");
            }

            size_t push_size = bytes[pc] |
                              (bytes[pc + 1] << 8) |
                              (bytes[pc + 2] << 16) |
                              (bytes[pc + 3] << 24);
            pc += 4;

            if (pc + push_size > bytes.size()) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has incomplete OP_PUSHDATA4 data");
            }

            pc += push_size;
            continue;
        }

        // Track IF/ENDIF nesting
        if (opcode == 0x63 || opcode == 0x64) { // OP_IF or OP_NOTIF
            if_depth++;
        } else if (opcode == 0x68) { // OP_ENDIF
            if_depth--;
            if (if_depth < 0) {
                return Result<void>::Error("Output " + std::to_string(output_index) +
                                          " script has unmatched OP_ENDIF");
            }
        }

        // Check for disabled opcodes (same as Bitcoin)
        // INTcoin disables certain opcodes for security
        std::set<uint8_t> disabled_opcodes = {
            0x7D, // OP_2OVER
            0x7E, // OP_2ROT
            0x7F, // OP_2SWAP
            0x80, // OP_IFDUP
            0x81, // OP_DEPTH
            0x89, // OP_NUMEQUAL
            0x8A, // OP_NUMEQUALVERIFY
            0x8B, // OP_NUMNOTEQUAL
            0x93, // OP_ADD
            0x94, // OP_SUB
            0x95, // OP_MUL (disabled for DoS)
            0x96, // OP_DIV (disabled for DoS)
            0x97, // OP_MOD (disabled for DoS)
            0x98, // OP_LSHIFT (disabled for DoS)
            0x99, // OP_RSHIFT (disabled for DoS)
        };

        if (disabled_opcodes.count(opcode) > 0) {
            return Result<void>::Error("Output " + std::to_string(output_index) +
                                      " script contains disabled opcode: 0x" +
                                      BytesToHex(std::vector<uint8_t>{opcode}));
        }

        // Validate known opcodes
        std::set<uint8_t> valid_opcodes = {
            0x00, // OP_0
            0x51, 0x52, // OP_1, OP_2
            0x63, 0x64, 0x67, 0x68, 0x69, // OP_IF, OP_NOTIF, OP_ELSE, OP_ENDIF, OP_VERIFY
            0x6A, // OP_RETURN
            0x75, 0x76, 0x7C, 0x82, // OP_DROP, OP_DUP, OP_SWAP, OP_SIZE
            0x87, 0x88, // OP_EQUAL, OP_EQUALVERIFY
            0xA9, // OP_HASH
            0xAC, // OP_CHECKSIG
            0xAE, // OP_CHECKMULTISIG
            0xB1, 0xB2, // OP_CHECKLOCKTIMEVERIFY, OP_CHECKSEQUENCEVERIFY
        };

        // Allow push data range
        if (opcode < 0x4F) {
            continue; // Already handled push data above
        }

        if (valid_opcodes.count(opcode) == 0) {
            return Result<void>::Error("Output " + std::to_string(output_index) +
                                      " script contains unknown opcode: 0x" +
                                      BytesToHex(std::vector<uint8_t>{opcode}));
        }
    }

    // Check for unmatched IF/ENDIF
    if (if_depth != 0) {
        return Result<void>::Error("Output " + std::to_string(output_index) +
                                  " script has unmatched IF/ENDIF (depth=" +
                                  std::to_string(if_depth) + ")");
    }

    return Result<void>::Ok();
}

Result<void> TxValidator::ValidateSignature(const Transaction& tx) const {
    // Signature validation is performed during script execution in ValidateInputs()
    // The script VM executes OP_CHECKSIG which verifies Dilithium3 signatures
    // This method exists for explicit validation ordering but the actual work
    // is done by ExecuteScript() called in ValidateInputs()

    if (tx.IsCoinbase()) {
        return Result<void>::Ok();  // Coinbase transactions have no signatures to validate
    }

    // All input signatures were already validated in ValidateInputs() via script execution
    // If we reached here, validation passed
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
