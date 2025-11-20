// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_TRANSACTION_VALIDATION_H
#define INTCOIN_TRANSACTION_VALIDATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <algorithm>
#include <limits>

namespace intcoin {
namespace transaction {

// Forward declarations
class Transaction;
class TransactionInput;
class TransactionOutput;

// Transaction input (spends a previous output)
struct TransactionInput {
    std::string prev_tx_hash;      // Hash of transaction being spent
    uint32_t prev_output_index;    // Index of output in previous transaction
    std::vector<uint8_t> script_sig; // Signature script (unlocking script)
    uint32_t sequence;             // Sequence number (for RBF, timelocks)

    // Witness data for segregated witness (malleability fix)
    std::vector<std::vector<uint8_t>> witness;

    // Unique identifier for this input
    std::string get_outpoint() const {
        return prev_tx_hash + ":" + std::to_string(prev_output_index);
    }
};

// Transaction output (creates new spendable outputs)
struct TransactionOutput {
    uint64_t value;                     // Amount in satoshis
    std::vector<uint8_t> script_pubkey; // Locking script (public key script)

    // Check if this is a dust output
    bool is_dust() const {
        constexpr uint64_t DUST_THRESHOLD = 546;  // satoshis
        return value < DUST_THRESHOLD;
    }
};

// Complete transaction
struct Transaction {
    uint32_t version;
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
    uint32_t lock_time;

    // Cached hash (for performance)
    mutable std::string tx_hash;

    // Check if this is a coinbase transaction
    bool is_coinbase() const {
        return inputs.size() == 1 &&
               inputs[0].prev_tx_hash == std::string(64, '0') &&
               inputs[0].prev_output_index == 0xFFFFFFFF;
    }

    // Calculate transaction hash
    std::string calculate_hash() const {
        // In production, this would hash the serialized transaction
        // For now, return cached value or placeholder
        if (tx_hash.empty()) {
            tx_hash = "tx_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        }
        return tx_hash;
    }

    // Get witness hash (for segregated witness)
    std::string calculate_witness_hash() const {
        // In production, this would hash transaction including witness data
        return "wtx_" + calculate_hash();
    }
};

// UTXO (Unspent Transaction Output)
struct UTXO {
    std::string tx_hash;
    uint32_t output_index;
    uint64_t value;
    std::vector<uint8_t> script_pubkey;
    uint32_t block_height;        // Height when created
    bool is_coinbase;             // Coinbase outputs have maturity requirement

    std::string get_outpoint() const {
        return tx_hash + ":" + std::to_string(output_index);
    }
};

// UTXO Set - maintains all unspent transaction outputs
class UTXOSet {
private:
    // Map of outpoint -> UTXO
    std::unordered_map<std::string, UTXO> utxos;

    // Total value in UTXO set (for sanity checking)
    uint64_t total_value = 0;

    // Statistics
    struct Statistics {
        uint64_t utxos_created = 0;
        uint64_t utxos_spent = 0;
        uint64_t utxos_current = 0;
        uint64_t total_value_satoshis = 0;
    } stats;

public:
    // Add a new UTXO
    bool add_utxo(const UTXO& utxo) {
        std::string outpoint = utxo.get_outpoint();

        // Check if already exists (shouldn't happen)
        if (utxos.count(outpoint) > 0) {
            return false;  // UTXO already exists
        }

        // Check for overflow
        if (total_value > std::numeric_limits<uint64_t>::max() - utxo.value) {
            return false;  // Would overflow
        }

        utxos[outpoint] = utxo;
        total_value += utxo.value;
        stats.utxos_created++;
        stats.utxos_current++;
        stats.total_value_satoshis = total_value;

        return true;
    }

    // Spend a UTXO (remove from set)
    std::optional<UTXO> spend_utxo(const std::string& outpoint) {
        auto it = utxos.find(outpoint);
        if (it == utxos.end()) {
            return std::nullopt;  // UTXO doesn't exist
        }

        UTXO utxo = it->second;
        utxos.erase(it);
        total_value -= utxo.value;
        stats.utxos_spent++;
        stats.utxos_current--;
        stats.total_value_satoshis = total_value;

        return utxo;
    }

    // Check if UTXO exists
    bool exists(const std::string& outpoint) const {
        return utxos.count(outpoint) > 0;
    }

    // Get UTXO without spending
    std::optional<UTXO> get_utxo(const std::string& outpoint) const {
        auto it = utxos.find(outpoint);
        if (it == utxos.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // Get total value in UTXO set
    uint64_t get_total_value() const {
        return total_value;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Validate UTXO set integrity
    bool validate_integrity() const {
        uint64_t calculated_total = 0;

        for (const auto& [outpoint, utxo] : utxos) {
            // Check for overflow
            if (calculated_total > std::numeric_limits<uint64_t>::max() - utxo.value) {
                return false;
            }
            calculated_total += utxo.value;
        }

        return calculated_total == total_value;
    }
};

// Double-spend detector
class DoubleSpendDetector {
private:
    // Track which outputs are being spent in current mempool/block
    std::unordered_set<std::string> spent_outputs;

    struct Statistics {
        uint64_t transactions_checked = 0;
        uint64_t double_spends_detected = 0;
        uint64_t inputs_validated = 0;
    } stats;

public:
    // Check if transaction contains any double-spends
    struct DoubleSpendResult {
        bool is_double_spend;
        std::vector<std::string> conflicting_outpoints;
        std::string error;
    };

    DoubleSpendResult check_transaction(const Transaction& tx) {
        DoubleSpendResult result;
        result.is_double_spend = false;

        stats.transactions_checked++;

        // Coinbase transactions can't double-spend
        if (tx.is_coinbase()) {
            return result;
        }

        // Check each input
        for (const auto& input : tx.inputs) {
            stats.inputs_validated++;

            std::string outpoint = input.get_outpoint();

            // Check if already spent in this batch
            if (spent_outputs.count(outpoint) > 0) {
                result.is_double_spend = true;
                result.conflicting_outpoints.push_back(outpoint);
                stats.double_spends_detected++;
            }
        }

        if (result.is_double_spend) {
            result.error = "Transaction attempts to double-spend " +
                          std::to_string(result.conflicting_outpoints.size()) +
                          " outputs";
        }

        return result;
    }

    // Mark transaction inputs as spent
    void mark_spent(const Transaction& tx) {
        if (tx.is_coinbase()) {
            return;
        }

        for (const auto& input : tx.inputs) {
            spent_outputs.insert(input.get_outpoint());
        }
    }

    // Clear spent outputs (after block is processed)
    void clear() {
        spent_outputs.clear();
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Transaction input validator
class InputValidator {
public:
    struct ValidationResult {
        bool valid;
        std::string error;

        ValidationResult() : valid(true) {}
        ValidationResult(const std::string& err) : valid(false), error(err) {}
    };

    // Validate transaction inputs
    static ValidationResult validate_inputs(
        const Transaction& tx,
        const UTXOSet& utxo_set,
        uint32_t current_block_height
    ) {
        // Coinbase transactions have special input rules
        if (tx.is_coinbase()) {
            return validate_coinbase_input(tx);
        }

        // Non-coinbase must have at least one input
        if (tx.inputs.empty()) {
            return ValidationResult("Transaction has no inputs");
        }

        // Validate each input
        for (size_t i = 0; i < tx.inputs.size(); ++i) {
            const auto& input = tx.inputs[i];

            // Get the UTXO being spent
            auto utxo = utxo_set.get_utxo(input.get_outpoint());
            if (!utxo) {
                return ValidationResult(
                    "Input " + std::to_string(i) +
                    " references non-existent UTXO: " + input.get_outpoint()
                );
            }

            // Check coinbase maturity (100 blocks)
            if (utxo->is_coinbase) {
                constexpr uint32_t COINBASE_MATURITY = 100;
                if (current_block_height < utxo->block_height + COINBASE_MATURITY) {
                    return ValidationResult(
                        "Input " + std::to_string(i) +
                        " spends immature coinbase (needs " +
                        std::to_string(COINBASE_MATURITY) + " confirmations)"
                    );
                }
            }

            // Validate script signature (simplified - real implementation would verify crypto)
            if (input.script_sig.empty() && input.witness.empty()) {
                return ValidationResult(
                    "Input " + std::to_string(i) + " has no signature"
                );
            }
        }

        return ValidationResult();
    }

private:
    static ValidationResult validate_coinbase_input(const Transaction& tx) {
        if (tx.inputs.size() != 1) {
            return ValidationResult("Coinbase must have exactly one input");
        }

        const auto& input = tx.inputs[0];

        // Coinbase input must reference null hash and max index
        if (input.prev_tx_hash != std::string(64, '0')) {
            return ValidationResult("Coinbase input must reference null hash");
        }

        if (input.prev_output_index != 0xFFFFFFFF) {
            return ValidationResult("Coinbase input must have index 0xFFFFFFFF");
        }

        // Coinbase script must be 2-100 bytes
        if (input.script_sig.size() < 2 || input.script_sig.size() > 100) {
            return ValidationResult(
                "Coinbase script must be 2-100 bytes (got " +
                std::to_string(input.script_sig.size()) + ")"
            );
        }

        return ValidationResult();
    }
};

// Transaction output validator
class OutputValidator {
public:
    struct ValidationResult {
        bool valid;
        std::string error;
        uint64_t total_output_value;

        ValidationResult() : valid(true), total_output_value(0) {}
        ValidationResult(const std::string& err) : valid(false), error(err), total_output_value(0) {}
    };

    // Validate transaction outputs
    static ValidationResult validate_outputs(const Transaction& tx) {
        ValidationResult result;
        result.total_output_value = 0;

        // Must have at least one output
        if (tx.outputs.empty()) {
            return ValidationResult("Transaction has no outputs");
        }

        // Validate each output
        for (size_t i = 0; i < tx.outputs.size(); ++i) {
            const auto& output = tx.outputs[i];

            // Value must not be zero (unless OP_RETURN)
            if (output.value == 0) {
                // Allow zero-value OP_RETURN outputs for data storage
                if (output.script_pubkey.empty() || output.script_pubkey[0] != 0x6a) {
                    return ValidationResult(
                        "Output " + std::to_string(i) + " has zero value"
                    );
                }
            }

            // Value must not exceed maximum money
            constexpr uint64_t MAX_MONEY = 21000000ULL * 100000000ULL;
            if (output.value > MAX_MONEY) {
                return ValidationResult(
                    "Output " + std::to_string(i) +
                    " value exceeds maximum money"
                );
            }

            // Script must not be empty (unless special case)
            if (output.script_pubkey.empty()) {
                return ValidationResult(
                    "Output " + std::to_string(i) + " has empty script"
                );
            }

            // Check for overflow in total
            if (result.total_output_value > std::numeric_limits<uint64_t>::max() - output.value) {
                return ValidationResult("Total output value overflows");
            }

            result.total_output_value += output.value;
        }

        // Total output value must not exceed maximum money
        constexpr uint64_t MAX_MONEY = 21000000ULL * 100000000ULL;
        if (result.total_output_value > MAX_MONEY) {
            return ValidationResult("Total output value exceeds maximum money");
        }

        return result;
    }

    // Check for duplicate outputs (potential attack)
    static bool has_duplicate_outputs(const Transaction& tx) {
        std::unordered_set<std::string> seen_scripts;

        for (const auto& output : tx.outputs) {
            // Create script hash
            std::string script_str(output.script_pubkey.begin(), output.script_pubkey.end());
            std::string script_key = script_str + "_" + std::to_string(output.value);

            if (seen_scripts.count(script_key) > 0) {
                return true;  // Duplicate found
            }

            seen_scripts.insert(script_key);
        }

        return false;
    }
};

// Transaction malleability prevention
class MalleabilityValidator {
public:
    struct MalleabilityCheck {
        bool is_malleable;
        std::vector<std::string> issues;
    };

    // Check for transaction malleability issues
    static MalleabilityCheck check_malleability(const Transaction& tx) {
        MalleabilityCheck result;
        result.is_malleable = false;

        // Coinbase transactions are inherently malleable (extra nonce)
        if (tx.is_coinbase()) {
            return result;  // Expected behavior
        }

        // Check for canonical signatures (BIP 66)
        for (size_t i = 0; i < tx.inputs.size(); ++i) {
            const auto& input = tx.inputs[i];

            // If using witness, check witness data
            if (!input.witness.empty()) {
                // Segregated witness fixes malleability
                continue;  // SegWit transaction, not malleable
            }

            // Check script signature for canonical encoding
            if (!is_canonical_signature(input.script_sig)) {
                result.is_malleable = true;
                result.issues.push_back(
                    "Input " + std::to_string(i) +
                    " has non-canonical signature (BIP 66 violation)"
                );
            }

            // Check for low-s signatures (BIP 146)
            if (!has_low_s_signature(input.script_sig)) {
                result.is_malleable = true;
                result.issues.push_back(
                    "Input " + std::to_string(i) +
                    " signature not using low-S form (BIP 146)"
                );
            }
        }

        // Check sequence numbers for malleability
        for (size_t i = 0; i < tx.inputs.size(); ++i) {
            const auto& input = tx.inputs[i];

            // Sequence 0xFFFFFFFF is final, not malleable
            if (input.sequence == 0xFFFFFFFF) {
                continue;
            }

            // Non-final sequence numbers can be malleable
            // (though this is intentional for RBF/timelocks)
        }

        return result;
    }

    // Check if transaction uses segregated witness (malleability fix)
    static bool uses_segwit(const Transaction& tx) {
        for (const auto& input : tx.inputs) {
            if (!input.witness.empty()) {
                return true;
            }
        }
        return false;
    }

private:
    // Check if signature is canonical (BIP 66)
    static bool is_canonical_signature(const std::vector<uint8_t>& script_sig) {
        if (script_sig.empty()) {
            return false;
        }

        // Simplified check - real implementation would parse DER encoding
        // and verify canonical form

        // Must start with 0x30 (DER sequence tag)
        if (script_sig[0] != 0x30) {
            return false;
        }

        // Length must be reasonable
        if (script_sig.size() < 8 || script_sig.size() > 73) {
            return false;
        }

        return true;  // Simplified check
    }

    // Check if signature uses low-S form (BIP 146)
    static bool has_low_s_signature(const std::vector<uint8_t>& script_sig) {
        // Simplified check - real implementation would parse signature
        // and verify S value is in lower half of curve order

        if (script_sig.size() < 8) {
            return false;
        }

        return true;  // Simplified check
    }
};

// Complete transaction validator
class TransactionValidator {
private:
    struct Statistics {
        uint64_t transactions_validated = 0;
        uint64_t transactions_accepted = 0;
        uint64_t transactions_rejected = 0;
        uint64_t double_spends_prevented = 0;
        uint64_t malleability_issues_found = 0;
    } stats;

public:
    struct ValidationResult {
        bool valid;
        std::string error;
        uint64_t total_input_value;
        uint64_t total_output_value;
        uint64_t fee;

        ValidationResult() : valid(true), total_input_value(0),
                           total_output_value(0), fee(0) {}
        ValidationResult(const std::string& err) : valid(false), error(err),
                                                   total_input_value(0),
                                                   total_output_value(0), fee(0) {}
    };

    // Validate complete transaction
    ValidationResult validate_transaction(
        const Transaction& tx,
        UTXOSet& utxo_set,
        DoubleSpendDetector& double_spend_detector,
        uint32_t current_block_height,
        bool check_malleability = true
    ) {
        stats.transactions_validated++;
        ValidationResult result;

        // Basic structure validation
        if (tx.inputs.empty() && !tx.is_coinbase()) {
            stats.transactions_rejected++;
            return ValidationResult("Transaction has no inputs");
        }

        if (tx.outputs.empty()) {
            stats.transactions_rejected++;
            return ValidationResult("Transaction has no outputs");
        }

        // Check for double-spends
        auto double_spend_result = double_spend_detector.check_transaction(tx);
        if (double_spend_result.is_double_spend) {
            stats.transactions_rejected++;
            stats.double_spends_prevented++;
            return ValidationResult("Double-spend detected: " + double_spend_result.error);
        }

        // Validate inputs
        auto input_result = InputValidator::validate_inputs(tx, utxo_set, current_block_height);
        if (!input_result.valid) {
            stats.transactions_rejected++;
            return ValidationResult("Input validation failed: " + input_result.error);
        }

        // Validate outputs
        auto output_result = OutputValidator::validate_outputs(tx);
        if (!output_result.valid) {
            stats.transactions_rejected++;
            return ValidationResult("Output validation failed: " + output_result.error);
        }

        result.total_output_value = output_result.total_output_value;

        // Calculate total input value (except for coinbase)
        if (!tx.is_coinbase()) {
            for (const auto& input : tx.inputs) {
                auto utxo = utxo_set.get_utxo(input.get_outpoint());
                if (!utxo) {
                    stats.transactions_rejected++;
                    return ValidationResult("Input references non-existent UTXO");
                }

                // Check for overflow
                if (result.total_input_value > std::numeric_limits<uint64_t>::max() - utxo->value) {
                    stats.transactions_rejected++;
                    return ValidationResult("Total input value overflows");
                }

                result.total_input_value += utxo->value;
            }

            // Inputs must be >= outputs (fee can be zero for tests)
            if (result.total_input_value < result.total_output_value) {
                stats.transactions_rejected++;
                return ValidationResult(
                    "Outputs (" + std::to_string(result.total_output_value) +
                    ") exceed inputs (" + std::to_string(result.total_input_value) + ")"
                );
            }

            result.fee = result.total_input_value - result.total_output_value;
        }

        // Check for malleability issues (optional)
        if (check_malleability) {
            auto malleability_result = MalleabilityValidator::check_malleability(tx);
            if (malleability_result.is_malleable) {
                stats.malleability_issues_found++;
                // Note: Malleability doesn't make transaction invalid,
                // but it's worth tracking
            }
        }

        stats.transactions_accepted++;
        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

    // Reset statistics
    void reset_statistics() {
        stats = Statistics();
    }
};

// Transaction validation manager
class TransactionValidationManager {
private:
    UTXOSet utxo_set;
    DoubleSpendDetector double_spend_detector;
    TransactionValidator validator;
    uint32_t current_block_height = 0;

    TransactionValidationManager() = default;

public:
    static TransactionValidationManager& instance() {
        static TransactionValidationManager instance;
        return instance;
    }

    // Validate transaction
    TransactionValidator::ValidationResult validate_transaction(
        const Transaction& tx,
        bool check_malleability = true
    ) {
        return validator.validate_transaction(
            tx,
            utxo_set,
            double_spend_detector,
            current_block_height,
            check_malleability
        );
    }

    // Apply transaction (add to UTXO set)
    bool apply_transaction(const Transaction& tx) {
        // Remove spent UTXOs
        if (!tx.is_coinbase()) {
            for (const auto& input : tx.inputs) {
                if (!utxo_set.spend_utxo(input.get_outpoint())) {
                    return false;  // UTXO doesn't exist
                }
            }
        }

        // Add new UTXOs
        std::string tx_hash = tx.calculate_hash();
        for (size_t i = 0; i < tx.outputs.size(); ++i) {
            UTXO utxo;
            utxo.tx_hash = tx_hash;
            utxo.output_index = static_cast<uint32_t>(i);
            utxo.value = tx.outputs[i].value;
            utxo.script_pubkey = tx.outputs[i].script_pubkey;
            utxo.block_height = current_block_height;
            utxo.is_coinbase = tx.is_coinbase();

            if (!utxo_set.add_utxo(utxo)) {
                return false;  // Failed to add UTXO
            }
        }

        // Mark as spent in double-spend detector
        double_spend_detector.mark_spent(tx);

        return true;
    }

    // Set current block height
    void set_block_height(uint32_t height) {
        current_block_height = height;
    }

    // Get UTXO set
    const UTXOSet& get_utxo_set() const {
        return utxo_set;
    }

    // Get statistics
    struct CombinedStatistics {
        UTXOSet::Statistics utxo_stats;
        DoubleSpendDetector::Statistics double_spend_stats;
        TransactionValidator::Statistics validator_stats;
    };

    CombinedStatistics get_statistics() const {
        CombinedStatistics stats;
        stats.utxo_stats = utxo_set.get_statistics();
        stats.double_spend_stats = double_spend_detector.get_statistics();
        stats.validator_stats = validator.get_statistics();
        return stats;
    }

    // Validate UTXO set integrity
    bool validate_utxo_integrity() const {
        return utxo_set.validate_integrity();
    }

    // Clear double-spend detector (after block processing)
    void clear_double_spend_detector() {
        double_spend_detector.clear();
    }
};

} // namespace transaction
} // namespace intcoin

#endif // INTCOIN_TRANSACTION_VALIDATION_H
