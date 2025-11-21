// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BRIDGE_VALIDATION_H
#define INTCOIN_BRIDGE_VALIDATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <mutex>

namespace intcoin {
namespace bridge {

// Bitcoin SPV proof validator (extended implementation)
class BitcoinSPVValidator {
private:
    struct BitcoinBlockHeader {
        uint32_t version;
        std::array<uint8_t, 32> prev_block_hash;
        std::array<uint8_t, 32> merkle_root;
        uint32_t timestamp;
        uint32_t bits;              // Compact difficulty target
        uint32_t nonce;
        uint32_t height;
        std::array<uint8_t, 32> block_hash;  // Computed hash
    };

    struct BitcoinTransaction {
        uint32_t version;
        std::vector<uint8_t> tx_data;        // Full transaction data
        std::array<uint8_t, 32> tx_hash;     // TXID
        uint64_t locktime;
        bool is_segwit;
    };

    struct MerkleProof {
        std::array<uint8_t, 32> tx_hash;
        std::vector<std::array<uint8_t, 32>> sibling_hashes;
        std::vector<bool> is_right;           // Direction: false=left, true=right
        uint32_t position;                    // Transaction index in block
        uint32_t total_transactions;
    };

    // Stored block headers for SPV validation
    std::unordered_map<uint32_t, BitcoinBlockHeader> headers;
    std::unordered_map<std::string, BitcoinBlockHeader> headers_by_hash;

    mutable std::mutex mtx;

    struct Statistics {
        uint64_t proofs_validated = 0;
        uint64_t proofs_valid = 0;
        uint64_t proofs_invalid = 0;
        uint64_t headers_added = 0;
        uint64_t difficulty_checks = 0;
    } stats;

public:
    // Minimum confirmations for finality
    static constexpr uint32_t MIN_CONFIRMATIONS = 6;
    static constexpr uint32_t SAFE_CONFIRMATIONS = 12;  // For large amounts

    // Add Bitcoin block header
    bool add_header(const BitcoinBlockHeader& header) {
        std::lock_guard<std::mutex> lock(mtx);

        // Validate header
        if (!validate_header_structure(header)) {
            return false;
        }

        // Check if we have previous block (except genesis)
        if (header.height > 0) {
            auto prev_it = headers.find(header.height - 1);
            if (prev_it == headers.end()) {
                return false;  // Missing previous block
            }

            // Verify prev_block_hash matches
            if (header.prev_block_hash != prev_it->second.block_hash) {
                return false;  // Chain discontinuity
            }
        }

        headers[header.height] = header;
        std::string hash_str = hash_to_string(header.block_hash);
        headers_by_hash[hash_str] = header;
        stats.headers_added++;

        return true;
    }

    // Validate SPV proof
    struct ValidationResult {
        bool is_valid;
        std::string error;
        uint32_t confirmations;
        std::array<uint8_t, 32> block_hash;
    };

    ValidationResult validate_spv_proof(
        const MerkleProof& proof,
        uint32_t block_height,
        uint32_t current_height
    ) {
        std::lock_guard<std::mutex> lock(mtx);
        stats.proofs_validated++;

        ValidationResult result;
        result.is_valid = false;
        result.confirmations = 0;

        // Check 1: Block header exists
        auto header_it = headers.find(block_height);
        if (header_it == headers.end()) {
            result.error = "Block header not found";
            stats.proofs_invalid++;
            return result;
        }

        const BitcoinBlockHeader& header = header_it->second;
        result.block_hash = header.block_hash;

        // Check 2: Compute merkle root from proof
        std::array<uint8_t, 32> computed_root = compute_merkle_root(
            proof.tx_hash,
            proof.sibling_hashes,
            proof.is_right
        );

        // Check 3: Verify computed root matches block's merkle root
        if (computed_root != header.merkle_root) {
            result.error = "Merkle root mismatch";
            stats.proofs_invalid++;
            return result;
        }

        // Check 4: Verify proof position is valid
        if (proof.position >= proof.total_transactions) {
            result.error = "Invalid transaction position";
            stats.proofs_invalid++;
            return result;
        }

        // Check 5: Verify proof length matches tree depth
        uint32_t expected_depth = calculate_tree_depth(proof.total_transactions);
        if (proof.sibling_hashes.size() != expected_depth) {
            result.error = "Proof length mismatch (expected " +
                         std::to_string(expected_depth) + " hashes)";
            stats.proofs_invalid++;
            return result;
        }

        // Check 6: Calculate confirmations
        if (current_height < block_height) {
            result.error = "Block is in future";
            stats.proofs_invalid++;
            return result;
        }

        result.confirmations = current_height - block_height + 1;

        // Check 7: Sufficient confirmations
        if (result.confirmations < MIN_CONFIRMATIONS) {
            result.error = "Insufficient confirmations (" +
                         std::to_string(result.confirmations) + "/" +
                         std::to_string(MIN_CONFIRMATIONS) + ")";
            stats.proofs_invalid++;
            return result;
        }

        // Check 8: Validate difficulty (proof of work)
        if (!validate_difficulty(header)) {
            result.error = "Invalid proof of work";
            stats.proofs_invalid++;
            return result;
        }

        // All checks passed
        result.is_valid = true;
        stats.proofs_valid++;

        return result;
    }

    // Validate header chain
    struct ChainValidation {
        bool is_valid;
        std::string error;
        uint32_t validated_blocks;
    };

    ChainValidation validate_chain(uint32_t start_height, uint32_t end_height) {
        std::lock_guard<std::mutex> lock(mtx);

        ChainValidation result;
        result.is_valid = true;
        result.validated_blocks = 0;

        for (uint32_t height = start_height; height < end_height; ++height) {
            auto current_it = headers.find(height);
            auto next_it = headers.find(height + 1);

            if (current_it == headers.end()) {
                result.is_valid = false;
                result.error = "Missing header at height " + std::to_string(height);
                return result;
            }

            if (next_it == headers.end()) {
                break;  // End of available chain
            }

            // Verify next block links to current
            if (next_it->second.prev_block_hash != current_it->second.block_hash) {
                result.is_valid = false;
                result.error = "Chain break at height " + std::to_string(height + 1);
                return result;
            }

            // Verify difficulty
            if (!validate_difficulty(next_it->second)) {
                result.is_valid = false;
                result.error = "Invalid PoW at height " + std::to_string(height + 1);
                return result;
            }

            result.validated_blocks++;
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

private:
    // Validate header structure
    bool validate_header_structure(const BitcoinBlockHeader& header) const {
        // Check version (Bitcoin uses version 1-4 typically)
        if (header.version == 0 || header.version > 0x20000000) {
            return false;
        }

        // Check timestamp is reasonable (after Bitcoin genesis)
        if (header.timestamp < 1231006505) {  // Bitcoin genesis: 2009-01-03
            return false;
        }

        // Check timestamp not too far in future (2 hours)
        uint64_t now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000ULL;
        if (header.timestamp > now + 7200) {
            return false;
        }

        return true;
    }

    // Validate proof of work (difficulty)
    bool validate_difficulty(const BitcoinBlockHeader& header) {
        stats.difficulty_checks++;

        // Extract target from compact bits format
        uint32_t compact = header.bits;
        uint32_t exponent = compact >> 24;
        uint32_t mantissa = compact & 0x00FFFFFF;

        // In production, would:
        // 1. Expand compact target to 256-bit target
        // 2. Verify block_hash < target
        // 3. Verify target matches network difficulty

        // For now, simplified check
        if (exponent == 0 || exponent > 34) {
            return false;  // Invalid exponent
        }

        if (mantissa == 0) {
            return false;  // Invalid mantissa
        }

        return true;
    }

    // Compute merkle root from proof
    std::array<uint8_t, 32> compute_merkle_root(
        const std::array<uint8_t, 32>& tx_hash,
        const std::vector<std::array<uint8_t, 32>>& siblings,
        const std::vector<bool>& directions
    ) {
        std::array<uint8_t, 32> current = tx_hash;

        for (size_t i = 0; i < siblings.size() && i < directions.size(); ++i) {
            if (directions[i]) {
                // Current is left, sibling is right
                current = double_sha256_pair(current, siblings[i]);
            } else {
                // Sibling is left, current is right
                current = double_sha256_pair(siblings[i], current);
            }
        }

        return current;
    }

    // Calculate merkle tree depth
    uint32_t calculate_tree_depth(uint32_t num_transactions) const {
        if (num_transactions == 0) return 0;
        uint32_t depth = 0;
        uint32_t n = num_transactions;
        while (n > 1) {
            n = (n + 1) / 2;  // Round up
            depth++;
        }
        return depth;
    }

    // Double SHA-256 hash pair (Bitcoin style)
    static std::array<uint8_t, 32> double_sha256_pair(
        const std::array<uint8_t, 32>& left,
        const std::array<uint8_t, 32>& right
    ) {
        // In production: SHA-256(SHA-256(left || right))
        // For now, XOR as placeholder
        std::array<uint8_t, 32> result;
        for (size_t i = 0; i < 32; ++i) {
            result[i] = left[i] ^ right[i];
        }
        return result;
    }

    // Convert hash to string
    static std::string hash_to_string(const std::array<uint8_t, 32>& hash) {
        std::string result;
        result.reserve(64);
        for (uint8_t byte : hash) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", byte);
            result += hex;
        }
        return result;
    }
};

// Ethereum smart contract integration validator
class EthereumContractValidator {
private:
    struct ContractState {
        std::string contract_address;  // 0x... format
        std::string abi_hash;           // ABI specification hash
        uint64_t deployed_block;
        bool is_verified;
    };

    struct FunctionCall {
        std::string function_signature;  // e.g., "transfer(address,uint256)"
        std::vector<uint8_t> encoded_params;
        uint64_t gas_limit;
        uint64_t value;  // ETH value sent
    };

    std::unordered_map<std::string, ContractState> verified_contracts;
    mutable std::mutex mtx;

    struct Statistics {
        uint64_t calls_validated = 0;
        uint64_t calls_valid = 0;
        uint64_t calls_invalid = 0;
        uint64_t contracts_verified = 0;
    } stats;

public:
    // Register verified contract
    bool register_contract(const ContractState& contract) {
        std::lock_guard<std::mutex> lock(mtx);

        // Validate Ethereum address format
        if (!validate_eth_address(contract.contract_address)) {
            return false;
        }

        // Check not already registered
        if (verified_contracts.find(contract.contract_address) != verified_contracts.end()) {
            return false;
        }

        verified_contracts[contract.contract_address] = contract;
        stats.contracts_verified++;

        return true;
    }

    // Validate function call
    struct CallValidation {
        bool is_valid;
        std::string error;
        uint64_t estimated_gas;
    };

    CallValidation validate_call(
        const std::string& contract_address,
        const FunctionCall& call
    ) {
        std::lock_guard<std::mutex> lock(mtx);
        stats.calls_validated++;

        CallValidation result;
        result.is_valid = false;
        result.estimated_gas = 0;

        // Check 1: Contract is registered
        auto contract_it = verified_contracts.find(contract_address);
        if (contract_it == verified_contracts.end()) {
            result.error = "Contract not verified";
            stats.calls_invalid++;
            return result;
        }

        const ContractState& contract = contract_it->second;

        // Check 2: Contract is verified
        if (!contract.is_verified) {
            result.error = "Contract not verified on Etherscan";
            stats.calls_invalid++;
            return result;
        }

        // Check 3: Function signature is valid
        if (!validate_function_signature(call.function_signature)) {
            result.error = "Invalid function signature";
            stats.calls_invalid++;
            return result;
        }

        // Check 4: Gas limit is reasonable
        if (call.gas_limit < 21000) {
            result.error = "Gas limit too low (minimum 21000)";
            stats.calls_invalid++;
            return result;
        }

        if (call.gas_limit > 10000000) {
            result.error = "Gas limit too high (maximum 10M)";
            stats.calls_invalid++;
            return result;
        }

        // Check 5: Encoded parameters length is reasonable
        if (call.encoded_params.size() > 1024) {
            result.error = "Encoded parameters too large";
            stats.calls_invalid++;
            return result;
        }

        // Check 6: Value is reasonable (if transferring ETH)
        // In production, would validate against user's balance

        // Estimate gas
        result.estimated_gas = estimate_gas(call);

        result.is_valid = true;
        stats.calls_valid++;

        return result;
    }

    // Validate event log
    struct EventLog {
        std::string contract_address;
        std::vector<std::array<uint8_t, 32>> topics;  // Indexed params
        std::vector<uint8_t> data;                     // Non-indexed params
        uint64_t block_number;
        uint32_t log_index;
    };

    struct EventValidation {
        bool is_valid;
        std::string error;
        std::string event_name;
    };

    EventValidation validate_event(const EventLog& log) {
        std::lock_guard<std::mutex> lock(mtx);

        EventValidation result;
        result.is_valid = false;

        // Check 1: Contract is verified
        auto contract_it = verified_contracts.find(log.contract_address);
        if (contract_it == verified_contracts.end()) {
            result.error = "Contract not verified";
            return result;
        }

        // Check 2: Event has at least one topic (event signature)
        if (log.topics.empty()) {
            result.error = "No topics (event signature missing)";
            return result;
        }

        // Check 3: Topics count is reasonable (max 4 in Ethereum)
        if (log.topics.size() > 4) {
            result.error = "Too many topics (maximum 4)";
            return result;
        }

        // Check 4: Data size is reasonable
        if (log.data.size() > 4096) {
            result.error = "Event data too large";
            return result;
        }

        // In production, would decode event based on ABI
        result.is_valid = true;
        result.event_name = "UnknownEvent";  // Would decode from topics[0]

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

private:
    // Validate Ethereum address format
    bool validate_eth_address(const std::string& address) const {
        // Must start with 0x
        if (address.length() != 42) return false;
        if (address.substr(0, 2) != "0x") return false;

        // Must be valid hex
        for (size_t i = 2; i < address.length(); ++i) {
            char c = address[i];
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }

        return true;
    }

    // Validate function signature format
    bool validate_function_signature(const std::string& sig) const {
        // Format: "functionName(type1,type2,...)"
        if (sig.empty()) return false;

        // Must contain opening parenthesis
        size_t paren_pos = sig.find('(');
        if (paren_pos == std::string::npos || paren_pos == 0) {
            return false;
        }

        // Must end with closing parenthesis
        if (sig.back() != ')') {
            return false;
        }

        // Function name must be valid identifier
        std::string func_name = sig.substr(0, paren_pos);
        for (char c : func_name) {
            if (!(std::isalnum(c) || c == '_')) {
                return false;
            }
        }

        return true;
    }

    // Estimate gas for function call
    uint64_t estimate_gas(const FunctionCall& call) const {
        // Simplified gas estimation
        // In production, would simulate call or use eth_estimateGas

        uint64_t base_gas = 21000;  // Base transaction cost

        // Add gas for function call overhead
        base_gas += 5000;

        // Add gas for parameter data
        base_gas += call.encoded_params.size() * 68;  // 68 gas per byte

        return std::min(base_gas, call.gas_limit);
    }
};

// Replay attack preventer
class ReplayAttackPreventer {
private:
    // Nonce tracking per address
    std::unordered_map<std::string, uint64_t> nonces;

    // Transaction ID tracking (prevents duplicate submissions)
    std::unordered_set<std::string> processed_txids;

    // Chain ID enforcement (EIP-155)
    uint64_t chain_id;

    mutable std::mutex mtx;

    struct Statistics {
        uint64_t transactions_validated = 0;
        uint64_t replay_attacks_prevented = 0;
        uint64_t duplicate_submissions = 0;
        uint64_t invalid_chain_ids = 0;
    } stats;

public:
    explicit ReplayAttackPreventer(uint64_t cid) : chain_id(cid) {}

    // Validate transaction for replay protection
    struct ReplayValidation {
        bool is_valid;
        std::string error;
        uint64_t expected_nonce;
    };

    ReplayValidation validate_transaction(
        const std::string& from_address,
        uint64_t tx_nonce,
        const std::string& tx_id,
        uint64_t tx_chain_id
    ) {
        std::lock_guard<std::mutex> lock(mtx);
        stats.transactions_validated++;

        ReplayValidation result;
        result.is_valid = false;

        // Check 1: Chain ID matches (EIP-155)
        if (tx_chain_id != chain_id) {
            result.error = "Chain ID mismatch (expected " +
                         std::to_string(chain_id) + ", got " +
                         std::to_string(tx_chain_id) + ")";
            stats.invalid_chain_ids++;
            stats.replay_attacks_prevented++;
            return result;
        }

        // Check 2: Transaction not already processed
        if (processed_txids.find(tx_id) != processed_txids.end()) {
            result.error = "Transaction already processed (duplicate submission)";
            stats.duplicate_submissions++;
            stats.replay_attacks_prevented++;
            return result;
        }

        // Check 3: Nonce is correct
        uint64_t expected_nonce = nonces[from_address];
        result.expected_nonce = expected_nonce;

        if (tx_nonce != expected_nonce) {
            result.error = "Invalid nonce (expected " +
                         std::to_string(expected_nonce) + ", got " +
                         std::to_string(tx_nonce) + ")";
            stats.replay_attacks_prevented++;
            return result;
        }

        // Validation successful
        result.is_valid = true;

        // Update state
        nonces[from_address]++;
        processed_txids.insert(tx_id);

        return result;
    }

    // Validate signature includes chain ID (EIP-155)
    struct SignatureValidation {
        bool is_valid;
        std::string error;
        uint64_t recovered_chain_id;
    };

    SignatureValidation validate_signature_eip155(
        uint8_t v_value,
        const std::array<uint8_t, 32>& r_value,
        const std::array<uint8_t, 32>& s_value
    ) {
        SignatureValidation result;
        result.is_valid = false;

        // EIP-155: v = chain_id * 2 + 35 + {0, 1}
        // Or legacy: v = 27 + {0, 1}

        if (v_value >= 35) {
            // EIP-155 signature
            uint64_t recovered_chain_id = (v_value - 35) / 2;
            result.recovered_chain_id = recovered_chain_id;

            if (recovered_chain_id != chain_id) {
                result.error = "Chain ID in signature doesn't match";
                stats.replay_attacks_prevented++;
                return result;
            }

            result.is_valid = true;
        } else if (v_value == 27 || v_value == 28) {
            // Legacy signature (no replay protection)
            result.error = "Legacy signature (no replay protection)";
            result.recovered_chain_id = 0;
            stats.replay_attacks_prevented++;
            return result;
        } else {
            result.error = "Invalid v value";
            return result;
        }

        // Validate r and s are non-zero
        bool r_zero = true, s_zero = true;
        for (uint8_t byte : r_value) {
            if (byte != 0) r_zero = false;
        }
        for (uint8_t byte : s_value) {
            if (byte != 0) s_zero = false;
        }

        if (r_zero || s_zero) {
            result.error = "Invalid signature (r or s is zero)";
            result.is_valid = false;
            return result;
        }

        return result;
    }

    // Check for transaction hash collision
    bool check_txid_uniqueness(const std::string& tx_id) {
        std::lock_guard<std::mutex> lock(mtx);
        return processed_txids.find(tx_id) == processed_txids.end();
    }

    // Get next nonce for address
    uint64_t get_next_nonce(const std::string& address) {
        std::lock_guard<std::mutex> lock(mtx);
        return nonces[address];
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Error handler for bridge operations
class BridgeErrorHandler {
private:
    enum class ErrorSeverity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    struct ErrorRecord {
        std::string error_code;
        std::string error_message;
        ErrorSeverity severity;
        uint64_t timestamp;
        std::string context;
        bool requires_intervention;
    };

    std::vector<ErrorRecord> error_log;
    mutable std::mutex mtx;

    struct Statistics {
        uint64_t total_errors = 0;
        uint64_t warnings = 0;
        uint64_t errors = 0;
        uint64_t critical_errors = 0;
        uint64_t handled_errors = 0;
    } stats;

public:
    // Error codes
    static constexpr const char* ERR_SPV_VALIDATION_FAILED = "SPV_001";
    static constexpr const char* ERR_INSUFFICIENT_CONFIRMATIONS = "SPV_002";
    static constexpr const char* ERR_MERKLE_PROOF_INVALID = "SPV_003";
    static constexpr const char* ERR_CONTRACT_CALL_FAILED = "ETH_001";
    static constexpr const char* ERR_INVALID_CONTRACT = "ETH_002";
    static constexpr const char* ERR_REPLAY_ATTACK = "SEC_001";
    static constexpr const char* ERR_NONCE_MISMATCH = "SEC_002";
    static constexpr const char* ERR_TIMEOUT_EXPIRED = "TIME_001";

    // Handle error
    struct ErrorHandling {
        bool can_retry;
        bool requires_user_action;
        std::string suggested_action;
    };

    ErrorHandling handle_error(
        const std::string& error_code,
        const std::string& error_message,
        const std::string& context = ""
    ) {
        std::lock_guard<std::mutex> lock(mtx);
        stats.total_errors++;

        ErrorHandling handling;
        handling.can_retry = false;
        handling.requires_user_action = false;

        // Determine severity
        ErrorSeverity severity = classify_error(error_code);

        // Log error
        ErrorRecord record;
        record.error_code = error_code;
        record.error_message = error_message;
        record.severity = severity;
        record.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        record.context = context;
        record.requires_intervention = (severity == ErrorSeverity::CRITICAL);

        error_log.push_back(record);

        // Update statistics
        switch (severity) {
            case ErrorSeverity::WARNING:
                stats.warnings++;
                break;
            case ErrorSeverity::ERROR:
                stats.errors++;
                break;
            case ErrorSeverity::CRITICAL:
                stats.critical_errors++;
                break;
            default:
                break;
        }

        // Determine handling strategy
        if (error_code == ERR_INSUFFICIENT_CONFIRMATIONS) {
            handling.can_retry = true;
            handling.suggested_action = "Wait for more confirmations";
        } else if (error_code == ERR_SPV_VALIDATION_FAILED) {
            handling.can_retry = false;
            handling.requires_user_action = true;
            handling.suggested_action = "Verify transaction on block explorer";
        } else if (error_code == ERR_CONTRACT_CALL_FAILED) {
            handling.can_retry = true;
            handling.suggested_action = "Retry with higher gas limit";
        } else if (error_code == ERR_REPLAY_ATTACK) {
            handling.can_retry = false;
            handling.requires_user_action = true;
            handling.suggested_action = "Transaction rejected for security";
        } else if (error_code == ERR_NONCE_MISMATCH) {
            handling.can_retry = true;
            handling.suggested_action = "Update nonce and retry";
        } else if (error_code == ERR_TIMEOUT_EXPIRED) {
            handling.can_retry = false;
            handling.requires_user_action = true;
            handling.suggested_action = "Initiate refund process";
        }

        stats.handled_errors++;

        return handling;
    }

    // Get recent errors
    std::vector<ErrorRecord> get_recent_errors(size_t count = 10) const {
        std::lock_guard<std::mutex> lock(mtx);

        size_t start = (error_log.size() > count) ? (error_log.size() - count) : 0;
        return std::vector<ErrorRecord>(error_log.begin() + start, error_log.end());
    }

    // Check for critical errors
    bool has_critical_errors() const {
        std::lock_guard<std::mutex> lock(mtx);
        return stats.critical_errors > 0;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }

private:
    ErrorSeverity classify_error(const std::string& error_code) {
        if (error_code == ERR_REPLAY_ATTACK ||
            error_code == ERR_TIMEOUT_EXPIRED) {
            return ErrorSeverity::CRITICAL;
        } else if (error_code == ERR_SPV_VALIDATION_FAILED ||
                   error_code == ERR_MERKLE_PROOF_INVALID ||
                   error_code == ERR_INVALID_CONTRACT) {
            return ErrorSeverity::ERROR;
        } else if (error_code == ERR_INSUFFICIENT_CONFIRMATIONS ||
                   error_code == ERR_NONCE_MISMATCH) {
            return ErrorSeverity::WARNING;
        }

        return ErrorSeverity::INFO;
    }
};

// Bridge validation manager
class BridgeValidationManager {
private:
    BitcoinSPVValidator btc_validator;
    EthereumContractValidator eth_validator;
    ReplayAttackPreventer replay_preventer;
    BridgeErrorHandler error_handler;

    BridgeValidationManager() : replay_preventer(1) {  // Chain ID 1 for INTcoin mainnet
    }

public:
    static BridgeValidationManager& instance() {
        static BridgeValidationManager instance;
        return instance;
    }

    // Get Bitcoin SPV validator
    BitcoinSPVValidator& get_btc_validator() {
        return btc_validator;
    }

    // Get Ethereum contract validator
    EthereumContractValidator& get_eth_validator() {
        return eth_validator;
    }

    // Get replay attack preventer
    ReplayAttackPreventer& get_replay_preventer() {
        return replay_preventer;
    }

    // Get error handler
    BridgeErrorHandler& get_error_handler() {
        return error_handler;
    }
};

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_VALIDATION_H
