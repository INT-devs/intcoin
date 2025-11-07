#include "intcoin/transaction.h"
#include "intcoin/validation.h"
#include "intcoin/safe_math.h"
#include "intcoin/memory_safety.h"
#include "intcoin/crypto.h"
#include <optional>
#include <cstring>

namespace intcoin {

using namespace validation;
using namespace safe_math;
using namespace memory_safety;

/**
 * Example: Safe Transaction Validation
 * Demonstrates how to use the new security features
 */

// Validate transaction input with comprehensive checks
std::optional<bool> validate_transaction_input_safe(
    const Transaction::Input& input
) {
    // 1. Input Validation: Check previous transaction hash
    auto hash_result = BinaryValidator::validate_hash(input.previous_output.hash);
    if (!hash_result) {
        return std::nullopt;  // Invalid hash
    }

    // 2. Input Validation: Check output index with overflow protection
    auto index_result = NumericValidator::validate_range<uint32_t>(
        input.previous_output.index,
        0,
        100000,  // Maximum outputs per transaction
        "output index"
    );
    if (!index_result) {
        return std::nullopt;
    }

    // 3. Memory Safety: Check signature script size
    auto script_result = BinaryValidator::validate_length_range(
        input.signature_script,
        0,
        StringValidator::MAX_SCRIPT_LENGTH,
        "signature script"
    );
    if (!script_result) {
        return std::nullopt;
    }

    // 4. Input Validation: Check sequence number
    // (0xFFFFFFFF is maximum valid value)
    if (input.sequence > 0xFFFFFFFF) {
        return std::nullopt;
    }

    return true;
}

// Validate transaction output with comprehensive checks
std::optional<bool> validate_transaction_output_safe(
    const Transaction::Output& output
) {
    // 1. Integer Overflow Protection: Validate amount
    auto amount_result = NumericValidator::validate_amount(output.value);
    if (!amount_result) {
        return std::nullopt;  // Invalid amount
    }

    // 2. Memory Safety: Check pubkey script size
    auto script_result = BinaryValidator::validate_length_range(
        output.pubkey_script,
        0,
        StringValidator::MAX_SCRIPT_LENGTH,
        "pubkey script"
    );
    if (!script_result) {
        return std::nullopt;
    }

    return true;
}

// Safe transaction validation with all security features
std::optional<bool> validate_transaction_safe(const Transaction& tx) {
    // 1. Input Validation: Check transaction version
    auto version_result = NumericValidator::validate_range<uint32_t>(
        tx.version,
        1,
        10,  // Reasonable version range
        "transaction version"
    );
    if (!version_result) {
        return std::nullopt;
    }

    // 2. Memory Safety: Check input count (prevent DoS)
    if (tx.inputs.empty() || tx.inputs.size() > 10000) {
        return std::nullopt;  // Coinbase must have 1 input, others limited
    }

    // 3. Memory Safety: Check output count (prevent DoS)
    if (tx.outputs.empty() || tx.outputs.size() > 10000) {
        return std::nullopt;  // Must have outputs, but not too many
    }

    // 4. Validate all inputs
    for (const auto& input : tx.inputs) {
        auto input_valid = validate_transaction_input_safe(input);
        if (!input_valid || !*input_valid) {
            return std::nullopt;
        }
    }

    // 5. Validate all outputs
    for (const auto& output : tx.outputs) {
        auto output_valid = validate_transaction_output_safe(output);
        if (!output_valid || !*output_valid) {
            return std::nullopt;
        }
    }

    // 6. Integer Overflow Protection: Sum all output amounts safely
    std::vector<int64_t> output_amounts;
    output_amounts.reserve(tx.outputs.size());
    for (const auto& output : tx.outputs) {
        output_amounts.push_back(output.value);
    }

    auto total_output = amount::sum_amounts(output_amounts);
    if (!total_output) {
        return std::nullopt;  // Overflow in output sum
    }

    // 7. Check total output doesn't exceed maximum supply
    if (!amount::is_valid_amount(*total_output)) {
        return std::nullopt;
    }

    // 8. Input Validation: Check locktime
    auto locktime_result = NumericValidator::validate_timestamp(tx.lock_time);
    if (!locktime_result && tx.lock_time != 0) {
        // Allow 0 (no locktime) or valid timestamp
        // For block height locktime, use different validation
        if (tx.lock_time < 500000000) {
            // Block height locktime
            auto height_result = NumericValidator::validate_block_height(tx.lock_time);
            if (!height_result) {
                return std::nullopt;
            }
        } else {
            // Invalid locktime
            return std::nullopt;
        }
    }

    return true;
}

// Example: Safe block header validation
std::optional<bool> validate_block_header_safe(
    uint32_t version,
    const std::vector<uint8_t>& prev_block_hash,
    const std::vector<uint8_t>& merkle_root,
    uint64_t timestamp,
    uint32_t bits,
    uint32_t nonce
) {
    // Use composite validator
    auto result = CompositeValidator::validate_block_header(
        version,
        prev_block_hash,
        merkle_root,
        timestamp,
        bits,
        nonce
    );

    return result ? std::optional<bool>(true) : std::nullopt;
}

// Example: Safe amount calculation
std::optional<int64_t> calculate_transaction_fee_safe(
    const std::vector<int64_t>& input_amounts,
    const std::vector<int64_t>& output_amounts
) {
    // 1. Integer Overflow Protection: Sum inputs safely
    auto total_input = amount::sum_amounts(input_amounts);
    if (!total_input) {
        return std::nullopt;  // Overflow
    }

    // 2. Integer Overflow Protection: Sum outputs safely
    auto total_output = amount::sum_amounts(output_amounts);
    if (!total_output) {
        return std::nullopt;  // Overflow
    }

    // 3. Integer Overflow Protection: Calculate fee safely
    auto fee = amount::sub_amounts(*total_input, *total_output);
    if (!fee) {
        return std::nullopt;  // Underflow or invalid
    }

    // 4. Sanity check: Fee should be reasonable (not negative, not > 1 BTC)
    if (*fee < 0 || *fee > amount::COIN) {
        return std::nullopt;  // Unreasonable fee
    }

    return fee;
}

// Example: Safe signature verification with memory bounds
std::optional<bool> verify_signature_safe(
    const std::vector<uint8_t>& message_hash,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& public_key
) {
    // 1. Memory Safety: Validate hash
    auto hash_result = BinaryValidator::validate_hash(message_hash);
    if (!hash_result) {
        return std::nullopt;
    }

    // 2. Memory Safety: Validate signature
    auto sig_result = BinaryValidator::validate_signature(signature);
    if (!sig_result) {
        return std::nullopt;
    }

    // 3. Memory Safety: Validate public key
    auto pubkey_result = BinaryValidator::validate_pubkey(public_key);
    if (!pubkey_result) {
        return std::nullopt;
    }

    // 4. Use SafeBuffer for intermediate operations
    SafeBuffer sig_buffer(StringValidator::MAX_SIGNATURE_LENGTH);
    if (!sig_buffer.append(signature)) {
        return std::nullopt;  // Shouldn't happen after validation
    }

    // 5. Actual cryptographic verification
    // Convert message hash to DilithiumSignature structure
    if (signature.size() != 4627) {
        // Dilithium5 signatures are 4627 bytes
        return std::nullopt;
    }

    DilithiumSignature dilithium_sig;
    std::memcpy(dilithium_sig.data(), signature.data(), signature.size());

    // Convert public key to DilithiumPubKey structure
    if (public_key.size() != 2592) {
        // Dilithium5 public keys are 2592 bytes
        return std::nullopt;
    }

    DilithiumPubKey dilithium_pubkey;
    std::memcpy(dilithium_pubkey.data(), public_key.data(), public_key.size());

    // Verify signature using Dilithium
    bool verification_result = crypto::Dilithium::verify(
        message_hash,
        dilithium_sig,
        dilithium_pubkey
    );

    if (!verification_result) {
        return std::nullopt;  // Signature verification failed
    }

    return true;
}

// Example: Safe network message parsing
std::optional<std::vector<uint8_t>> parse_network_message_safe(
    const uint8_t* data,
    size_t data_len,
    size_t max_message_size = 32 * 1024 * 1024  // 32 MB max
) {
    // 1. Input Validation: Check null pointer
    if (!data) {
        return std::nullopt;
    }

    // 2. Memory Safety: Check message size
    if (data_len == 0 || data_len > max_message_size) {
        return std::nullopt;
    }

    // 3. Memory Safety: Use SafeMemory for copying
    auto result = SafeMemory::copy_to_vector(data, data_len, max_message_size);
    if (!result) {
        return std::nullopt;
    }

    // 4. Additional validation could go here
    // (e.g., check message header, verify checksum, etc.)

    return result;
}

// Example: Safe peer address validation
std::optional<bool> validate_peer_address_safe(
    const std::string& address,
    uint16_t port
) {
    // 1. Input Validation: Check address format
    auto address_result = NetworkValidator::validate_peer_address(address, port);
    if (!address_result) {
        return std::nullopt;
    }

    // 2. Additional checks: Reject private IP ranges in production
    // (This is simplified - real implementation would be more comprehensive)
    if (address.find("127.") == 0 || address.find("192.168.") == 0) {
        // Localhost or private network - reject in production
        // (but allow in testnet/regtest)
        return std::nullopt;
    }

    return true;
}

// Example: Safe script parsing with bounds checking
std::optional<std::vector<std::vector<uint8_t>>> parse_script_safe(
    const std::vector<uint8_t>& script
) {
    // 1. Memory Safety: Validate script length
    auto length_result = BinaryValidator::validate_length_range(
        script,
        0,
        StringValidator::MAX_SCRIPT_LENGTH,
        "script"
    );
    if (!length_result) {
        return std::nullopt;
    }

    // 2. Use BoundedVector to prevent unbounded growth
    BoundedVector<std::vector<uint8_t>> opcodes(1000);  // Max 1000 opcodes

    // 3. Parse script with bounds checking
    size_t pos = 0;
    while (pos < script.size()) {
        // Memory Safety: Check remaining bytes
        if (pos >= script.size()) {
            return std::nullopt;  // Unexpected end
        }

        uint8_t opcode = script[pos++];

        // Handle push data opcodes
        if (opcode > 0 && opcode <= 75) {
            // Push next opcode bytes
            size_t push_size = opcode;

            // Integer Overflow Protection: Check bounds
            if (pos + push_size > script.size()) {
                return std::nullopt;  // Not enough data
            }

            std::vector<uint8_t> data(script.begin() + pos,
                                     script.begin() + pos + push_size);

            if (!opcodes.push_back(std::move(data))) {
                return std::nullopt;  // Too many opcodes
            }

            // Integer Overflow Protection: Safe addition
            auto new_pos = safe_add(pos, push_size);
            if (!new_pos) {
                return std::nullopt;  // Overflow
            }
            pos = *new_pos;
        } else {
            // Single opcode
            if (!opcodes.push_back(std::vector<uint8_t>{opcode})) {
                return std::nullopt;  // Too many opcodes
            }
        }
    }

    // Convert to standard vector
    std::vector<std::vector<uint8_t>> result;
    result.reserve(opcodes.size());
    for (const auto& op : opcodes) {
        result.push_back(op);
    }

    return result;
}

// Example: Safe serialization with overflow checks
std::optional<std::vector<uint8_t>> serialize_transaction_safe(
    const Transaction& tx
) {
    // Use SafeBuffer with reasonable max size (1 MB per transaction)
    constexpr size_t MAX_TX_SIZE = 1024 * 1024;
    SafeBuffer buffer(MAX_TX_SIZE);

    // Serialize version (4 bytes)
    uint32_t version = tx.version;
    if (!buffer.append(reinterpret_cast<uint8_t*>(&version), sizeof(version))) {
        return std::nullopt;
    }

    // Serialize input count (varint)
    // Safe cast to check overflow
    auto input_count = safe_size_cast<uint32_t>(tx.inputs.size());
    if (!input_count) {
        return std::nullopt;  // Too many inputs
    }

    // Write varint (simplified - real implementation would encode properly)
    if (!buffer.append(reinterpret_cast<uint8_t*>(&*input_count),
                      sizeof(*input_count))) {
        return std::nullopt;
    }

    // Serialize each input
    for (const auto& input : tx.inputs) {
        // Previous output hash
        if (!buffer.append(input.previous_output.hash)) {
            return std::nullopt;  // Buffer full
        }

        // Previous output index
        uint32_t index = input.previous_output.index;
        if (!buffer.append(reinterpret_cast<uint8_t*>(&index), sizeof(index))) {
            return std::nullopt;
        }

        // Signature script length and data
        auto script_len = safe_size_cast<uint32_t>(input.signature_script.size());
        if (!script_len) {
            return std::nullopt;
        }

        if (!buffer.append(reinterpret_cast<uint8_t*>(&*script_len),
                          sizeof(*script_len))) {
            return std::nullopt;
        }

        if (!buffer.append(input.signature_script)) {
            return std::nullopt;
        }

        // Sequence
        uint32_t sequence = input.sequence;
        if (!buffer.append(reinterpret_cast<uint8_t*>(&sequence),
                          sizeof(sequence))) {
            return std::nullopt;
        }
    }

    // Similar for outputs...
    auto output_count = safe_size_cast<uint32_t>(tx.outputs.size());
    if (!output_count) {
        return std::nullopt;
    }

    if (!buffer.append(reinterpret_cast<uint8_t*>(&*output_count),
                      sizeof(*output_count))) {
        return std::nullopt;
    }

    for (const auto& output : tx.outputs) {
        // Value
        int64_t value = output.value;
        if (!buffer.append(reinterpret_cast<uint8_t*>(&value), sizeof(value))) {
            return std::nullopt;
        }

        // Pubkey script
        auto script_len = safe_size_cast<uint32_t>(output.pubkey_script.size());
        if (!script_len) {
            return std::nullopt;
        }

        if (!buffer.append(reinterpret_cast<uint8_t*>(&*script_len),
                          sizeof(*script_len))) {
            return std::nullopt;
        }

        if (!buffer.append(output.pubkey_script)) {
            return std::nullopt;
        }
    }

    // Locktime
    uint32_t locktime = tx.lock_time;
    if (!buffer.append(reinterpret_cast<uint8_t*>(&locktime),
                      sizeof(locktime))) {
        return std::nullopt;
    }

    return buffer.to_vector();
}

} // namespace intcoin
