// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONSENSUS_VALIDATION_H
#define INTCOIN_CONSENSUS_VALIDATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <limits>
#include <stdexcept>

namespace intcoin {
namespace consensus {

// Consensus parameters for INTcoin
namespace params {
    // Block reward halving interval (blocks)
    constexpr uint32_t SUBSIDY_HALVING_INTERVAL = 210000;  // ~4 years

    // Initial block reward (in satoshis)
    constexpr uint64_t INITIAL_BLOCK_REWARD = 50 * 100000000ULL;  // 50 INT

    // Maximum money supply (21 million INT)
    constexpr uint64_t MAX_MONEY = 21000000ULL * 100000000ULL;

    // Coinbase maturity (blocks before coinbase can be spent)
    constexpr uint32_t COINBASE_MATURITY = 100;

    // Maximum block size
    constexpr uint32_t MAX_BLOCK_SIZE = 4000000;  // 4 MB

    // Maximum transaction size
    constexpr uint32_t MAX_TRANSACTION_SIZE = 1000000;  // 1 MB

    // Maximum number of signature operations per block
    constexpr uint32_t MAX_BLOCK_SIGOPS = 80000;

    // Minimum transaction output value (dust threshold)
    constexpr uint64_t DUST_THRESHOLD = 546;  // satoshis
}

// Safe arithmetic operations with overflow checking
class SafeMath {
public:
    // Safe addition with overflow check
    static std::optional<uint64_t> add(uint64_t a, uint64_t b) {
        if (a > std::numeric_limits<uint64_t>::max() - b) {
            return std::nullopt;  // Overflow would occur
        }
        return a + b;
    }

    // Safe subtraction with underflow check
    static std::optional<uint64_t> subtract(uint64_t a, uint64_t b) {
        if (a < b) {
            return std::nullopt;  // Underflow would occur
        }
        return a - b;
    }

    // Safe multiplication with overflow check
    static std::optional<uint64_t> multiply(uint64_t a, uint64_t b) {
        if (a == 0 || b == 0) {
            return 0;
        }
        if (a > std::numeric_limits<uint64_t>::max() / b) {
            return std::nullopt;  // Overflow would occur
        }
        return a * b;
    }

    // Safe division (no overflow possible, but check for division by zero)
    static std::optional<uint64_t> divide(uint64_t a, uint64_t b) {
        if (b == 0) {
            return std::nullopt;  // Division by zero
        }
        return a / b;
    }

    // Check if value is within valid range
    static bool is_valid_amount(uint64_t amount) {
        return amount <= params::MAX_MONEY;
    }

    // Check if value is above dust threshold
    static bool is_above_dust(uint64_t amount) {
        return amount >= params::DUST_THRESHOLD;
    }
};

// Block reward calculation
class BlockRewardCalculator {
public:
    // Calculate block subsidy for given height
    static uint64_t get_block_subsidy(uint32_t block_height) {
        // Calculate number of halvings
        uint32_t halvings = block_height / params::SUBSIDY_HALVING_INTERVAL;

        // After 64 halvings, subsidy is zero
        if (halvings >= 64) {
            return 0;
        }

        // Start with initial reward
        uint64_t subsidy = params::INITIAL_BLOCK_REWARD;

        // Right shift by number of halvings (equivalent to dividing by 2^halvings)
        // This is safe from overflow
        subsidy >>= halvings;

        return subsidy;
    }

    // Validate that reward doesn't exceed maximum
    static bool validate_block_reward(uint64_t claimed_reward, uint32_t block_height) {
        uint64_t expected_subsidy = get_block_subsidy(block_height);
        return claimed_reward <= expected_subsidy;
    }

    // Calculate total block reward including fees
    static std::optional<uint64_t> calculate_total_reward(
        uint32_t block_height,
        uint64_t total_fees
    ) {
        uint64_t subsidy = get_block_subsidy(block_height);
        return SafeMath::add(subsidy, total_fees);
    }

    // Get total supply at given height
    static std::optional<uint64_t> get_total_supply(uint32_t block_height) {
        uint64_t total = 0;
        uint32_t height = 0;

        while (height <= block_height) {
            uint64_t subsidy = get_block_subsidy(height);

            // Calculate how many blocks until next halving
            uint32_t blocks_until_halving = params::SUBSIDY_HALVING_INTERVAL -
                (height % params::SUBSIDY_HALVING_INTERVAL);
            uint32_t blocks_to_add = std::min(blocks_until_halving, block_height - height + 1);

            // Add subsidy for this period
            auto period_reward = SafeMath::multiply(subsidy, blocks_to_add);
            if (!period_reward) {
                return std::nullopt;  // Overflow
            }

            auto new_total = SafeMath::add(total, *period_reward);
            if (!new_total) {
                return std::nullopt;  // Overflow
            }

            total = *new_total;
            height += blocks_to_add;
        }

        return total;
    }
};

// Coinbase transaction validation
class CoinbaseValidator {
public:
    struct ValidationResult {
        bool valid;
        std::string error;

        ValidationResult() : valid(true) {}
        ValidationResult(const std::string& err) : valid(false), error(err) {}
    };

    struct CoinbaseTransaction {
        uint32_t version;
        std::vector<uint8_t> input_script;  // Coinbase script
        std::vector<std::pair<std::string, uint64_t>> outputs;  // Address, amount
        uint32_t lock_time;
        uint32_t block_height;  // Encoded in coinbase script
    };

    // Validate coinbase transaction structure
    static ValidationResult validate_structure(const CoinbaseTransaction& tx) {
        // Must have exactly one input
        if (tx.input_script.empty()) {
            return ValidationResult("Coinbase must have input script");
        }

        // Coinbase script must encode block height (BIP 34)
        if (!validate_height_in_coinbase(tx.input_script, tx.block_height)) {
            return ValidationResult("Block height not correctly encoded in coinbase");
        }

        // Must have at least one output
        if (tx.outputs.empty()) {
            return ValidationResult("Coinbase must have at least one output");
        }

        // Validate each output
        for (const auto& [address, amount] : tx.outputs) {
            if (!SafeMath::is_valid_amount(amount)) {
                return ValidationResult("Output amount exceeds maximum money");
            }
        }

        return ValidationResult();
    }

    // Validate coinbase reward amount
    static ValidationResult validate_reward(
        const CoinbaseTransaction& tx,
        uint64_t total_fees
    ) {
        // Calculate total output amount
        uint64_t total_output = 0;
        for (const auto& [address, amount] : tx.outputs) {
            auto new_total = SafeMath::add(total_output, amount);
            if (!new_total) {
                return ValidationResult("Integer overflow in coinbase outputs");
            }
            total_output = *new_total;
        }

        // Calculate maximum allowed reward
        auto max_reward = BlockRewardCalculator::calculate_total_reward(
            tx.block_height,
            total_fees
        );

        if (!max_reward) {
            return ValidationResult("Integer overflow calculating max reward");
        }

        // Total output must not exceed subsidy + fees
        if (total_output > *max_reward) {
            return ValidationResult(
                "Coinbase output (" + std::to_string(total_output) +
                ") exceeds allowed reward (" + std::to_string(*max_reward) + ")"
            );
        }

        return ValidationResult();
    }

    // Validate block height encoding (BIP 34)
    static bool validate_height_in_coinbase(
        const std::vector<uint8_t>& coinbase_script,
        uint32_t expected_height
    ) {
        if (coinbase_script.size() < 1) {
            return false;
        }

        // First byte is the length of the height encoding
        uint8_t height_len = coinbase_script[0];

        if (height_len < 1 || height_len > 4) {
            return false;
        }

        if (coinbase_script.size() < 1 + height_len) {
            return false;
        }

        // Decode height from coinbase script (little-endian)
        uint32_t decoded_height = 0;
        for (uint8_t i = 0; i < height_len; ++i) {
            decoded_height |= (static_cast<uint32_t>(coinbase_script[1 + i]) << (8 * i));
        }

        return decoded_height == expected_height;
    }

    // Complete coinbase validation
    static ValidationResult validate_coinbase(
        const CoinbaseTransaction& tx,
        uint64_t total_fees
    ) {
        // Validate structure
        auto structure_result = validate_structure(tx);
        if (!structure_result.valid) {
            return structure_result;
        }

        // Validate reward amount
        auto reward_result = validate_reward(tx, total_fees);
        if (!reward_result.valid) {
            return reward_result;
        }

        return ValidationResult();
    }
};

// Transaction fee validation
class FeeValidator {
public:
    struct FeeCalculation {
        uint64_t total_input;
        uint64_t total_output;
        uint64_t fee;
        bool valid;
        std::string error;
    };

    // Calculate and validate transaction fees
    static FeeCalculation calculate_fee(
        const std::vector<uint64_t>& inputs,
        const std::vector<uint64_t>& outputs
    ) {
        FeeCalculation result;
        result.total_input = 0;
        result.total_output = 0;
        result.valid = true;

        // Sum inputs with overflow checking
        for (uint64_t input : inputs) {
            if (!SafeMath::is_valid_amount(input)) {
                result.valid = false;
                result.error = "Input amount exceeds maximum";
                return result;
            }

            auto new_total = SafeMath::add(result.total_input, input);
            if (!new_total) {
                result.valid = false;
                result.error = "Integer overflow in input sum";
                return result;
            }
            result.total_input = *new_total;
        }

        // Sum outputs with overflow checking
        for (uint64_t output : outputs) {
            if (!SafeMath::is_valid_amount(output)) {
                result.valid = false;
                result.error = "Output amount exceeds maximum";
                return result;
            }

            auto new_total = SafeMath::add(result.total_output, output);
            if (!new_total) {
                result.valid = false;
                result.error = "Integer overflow in output sum";
                return result;
            }
            result.total_output = *new_total;
        }

        // Calculate fee (inputs - outputs)
        if (result.total_input < result.total_output) {
            result.valid = false;
            result.error = "Outputs exceed inputs (negative fee)";
            return result;
        }

        result.fee = result.total_input - result.total_output;

        // Sanity check: fee shouldn't exceed maximum money
        if (!SafeMath::is_valid_amount(result.fee)) {
            result.valid = false;
            result.error = "Fee exceeds maximum money";
            return result;
        }

        return result;
    }

    // Validate fee is reasonable (not absurdly high)
    static bool is_reasonable_fee(uint64_t fee, size_t tx_size_bytes) {
        // Maximum fee: 0.1 INT per kB
        constexpr uint64_t MAX_FEE_PER_KB = 10000000;  // 0.1 INT

        // Calculate maximum reasonable fee for this transaction
        uint64_t max_reasonable_fee = (tx_size_bytes * MAX_FEE_PER_KB) / 1000;

        return fee <= max_reasonable_fee;
    }
};

// Block validation
class BlockValidator {
public:
    struct BlockValidationResult {
        bool valid;
        std::string error;
        uint64_t total_fees;

        BlockValidationResult() : valid(true), total_fees(0) {}
        BlockValidationResult(const std::string& err) : valid(false), error(err), total_fees(0) {}
    };

    struct Block {
        uint32_t version;
        std::string prev_block_hash;
        std::string merkle_root;
        uint32_t timestamp;
        uint32_t bits;  // Difficulty target
        uint32_t nonce;
        uint32_t height;
        std::vector<CoinbaseValidator::CoinbaseTransaction> transactions;  // First is coinbase
    };

    // Validate complete block
    static BlockValidationResult validate_block(const Block& block) {
        BlockValidationResult result;

        // Block must have at least one transaction (coinbase)
        if (block.transactions.empty()) {
            return BlockValidationResult("Block has no transactions");
        }

        // First transaction must be coinbase
        const auto& coinbase = block.transactions[0];

        // Calculate total fees from all transactions (except coinbase)
        uint64_t total_fees = 0;
        for (size_t i = 1; i < block.transactions.size(); ++i) {
            // In a real implementation, we would calculate fees for each transaction
            // For now, we assume fees are provided separately
            // This is a simplified example
        }

        // Validate coinbase transaction
        auto coinbase_result = CoinbaseValidator::validate_coinbase(coinbase, total_fees);
        if (!coinbase_result.valid) {
            return BlockValidationResult("Coinbase validation failed: " + coinbase_result.error);
        }

        result.total_fees = total_fees;
        return result;
    }

    // Validate block reward calculation
    static bool validate_block_reward(uint32_t height, uint64_t claimed_reward, uint64_t fees) {
        auto max_reward = BlockRewardCalculator::calculate_total_reward(height, fees);
        if (!max_reward) {
            return false;  // Overflow
        }
        return claimed_reward <= *max_reward;
    }
};

// Consensus validation manager
class ConsensusValidationManager {
private:
    struct Statistics {
        uint64_t blocks_validated = 0;
        uint64_t coinbase_validations = 0;
        uint64_t fee_validations = 0;
        uint64_t overflow_prevented = 0;
        uint64_t invalid_rewards_rejected = 0;
        uint64_t invalid_coinbase_rejected = 0;
    };

    Statistics stats;

    ConsensusValidationManager() = default;

public:
    static ConsensusValidationManager& instance() {
        static ConsensusValidationManager instance;
        return instance;
    }

    // Validate block reward
    bool validate_block_reward(uint32_t height, uint64_t claimed_reward, uint64_t fees) {
        stats.blocks_validated++;

        auto max_reward = BlockRewardCalculator::calculate_total_reward(height, fees);
        if (!max_reward) {
            stats.overflow_prevented++;
            return false;
        }

        if (claimed_reward > *max_reward) {
            stats.invalid_rewards_rejected++;
            return false;
        }

        return true;
    }

    // Validate coinbase transaction
    CoinbaseValidator::ValidationResult validate_coinbase(
        const CoinbaseValidator::CoinbaseTransaction& tx,
        uint64_t total_fees
    ) {
        stats.coinbase_validations++;

        auto result = CoinbaseValidator::validate_coinbase(tx, total_fees);
        if (!result.valid) {
            stats.invalid_coinbase_rejected++;
        }

        return result;
    }

    // Calculate transaction fees
    FeeValidator::FeeCalculation calculate_fees(
        const std::vector<uint64_t>& inputs,
        const std::vector<uint64_t>& outputs
    ) {
        stats.fee_validations++;

        auto result = FeeValidator::calculate_fee(inputs, outputs);
        if (!result.valid && result.error.find("overflow") != std::string::npos) {
            stats.overflow_prevented++;
        }

        return result;
    }

    // Get block subsidy for height
    uint64_t get_block_subsidy(uint32_t height) const {
        return BlockRewardCalculator::get_block_subsidy(height);
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

} // namespace consensus
} // namespace intcoin

#endif // INTCOIN_CONSENSUS_VALIDATION_H
