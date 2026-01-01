// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/htlc.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <algorithm>

namespace intcoin {
namespace htlc {

// ============================================================================
// HTLCScript Implementation
// ============================================================================

Script HTLCScript::CreateHTLCScript(const HTLCParameters& params) {
    /*
     * HTLC Script Structure:
     *
     * OP_IF
     *     // Claim path (with preimage)
     *     OP_SHA3_256 (or OP_SHA256/OP_RIPEMD160)
     *     <hash_lock>
     *     OP_EQUALVERIFY
     *     <recipient_pubkey>
     *     OP_CHECKSIG
     * OP_ELSE
     *     // Refund path (after timeout)
     *     <locktime>
     *     OP_CHECKLOCKTIMEVERIFY
     *     OP_DROP
     *     <refund_pubkey>
     *     OP_CHECKSIG
     * OP_ENDIF
     */

    Script script;

    // OP_IF
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_IF));

    // Claim path
    // Hash algorithm (SHA3-256 for INTcoin, SHA-256/RIPEMD-160 for Bitcoin)
    // Note: This creates INTcoin scripts - for Bitcoin HTLCs, use BitcoinHTLCScript class
    switch (params.hash_algorithm) {
        case HTLCHashAlgorithm::SHA3_256:
            script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));  // INTcoin SHA3-256
            break;
        case HTLCHashAlgorithm::SHA256:
            // For Bitcoin cross-chain swaps, use BitcoinHTLCScript generator
            script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));  // Fallback to SHA3
            LogF(LogLevel::WARNING, "HTLC: SHA-256 not supported in INTcoin scripts, use BitcoinHTLCScript for Bitcoin");
            break;
        case HTLCHashAlgorithm::RIPEMD160:
            // For Bitcoin cross-chain swaps, use BitcoinHTLCScript generator
            script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_HASH));  // Fallback to SHA3
            LogF(LogLevel::WARNING, "HTLC: RIPEMD-160 not supported in INTcoin scripts, use BitcoinHTLCScript for Bitcoin");
            break;
    }

    // Push hash_lock
    script.bytes.push_back(static_cast<uint8_t>(params.hash_lock.size()));
    script.bytes.insert(script.bytes.end(), params.hash_lock.begin(), params.hash_lock.end());

    // OP_EQUALVERIFY
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_EQUALVERIFY));

    // Push recipient_pubkey
    script.bytes.push_back(static_cast<uint8_t>(params.recipient_pubkey.size()));
    script.bytes.insert(script.bytes.end(), params.recipient_pubkey.begin(), params.recipient_pubkey.end());

    // OP_CHECKSIG
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));

    // OP_ELSE
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ELSE));

    // Refund path
    // Push locktime (8 bytes, little-endian)
    std::vector<uint8_t> locktime_bytes;
    for (int i = 0; i < 8; i++) {
        locktime_bytes.push_back((params.locktime >> (i * 8)) & 0xFF);
    }
    script.bytes.push_back(static_cast<uint8_t>(locktime_bytes.size()));
    script.bytes.insert(script.bytes.end(), locktime_bytes.begin(), locktime_bytes.end());

    // OP_CHECKLOCKTIMEVERIFY
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKLOCKTIMEVERIFY));

    // OP_DROP
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_DROP));

    // Push refund_pubkey
    script.bytes.push_back(static_cast<uint8_t>(params.refund_pubkey.size()));
    script.bytes.insert(script.bytes.end(), params.refund_pubkey.begin(), params.refund_pubkey.end());

    // OP_CHECKSIG
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));

    // OP_ENDIF
    script.bytes.push_back(static_cast<uint8_t>(OpCode::OP_ENDIF));

    LogF(LogLevel::DEBUG, "HTLC: Created script (%zu bytes)", script.bytes.size());

    return script;
}

Script HTLCScript::CreateClaimWitness(const std::vector<uint8_t>& preimage,
                                     const std::vector<uint8_t>& signature) {
    /*
     * Claim Witness Stack:
     * <signature>
     * <preimage>
     * <1> (OP_TRUE for OP_IF)
     */

    Script witness;

    // Push signature
    witness.bytes.push_back(static_cast<uint8_t>(signature.size()));
    witness.bytes.insert(witness.bytes.end(), signature.begin(), signature.end());

    // Push preimage
    witness.bytes.push_back(static_cast<uint8_t>(preimage.size()));
    witness.bytes.insert(witness.bytes.end(), preimage.begin(), preimage.end());

    // Push OP_TRUE (1) for OP_IF
    witness.bytes.push_back(0x01);
    witness.bytes.push_back(0x01);

    return witness;
}

Script HTLCScript::CreateRefundWitness(const std::vector<uint8_t>& signature) {
    /*
     * Refund Witness Stack:
     * <signature>
     * <0> (OP_FALSE for OP_IF)
     */

    Script witness;

    // Push signature
    witness.bytes.push_back(static_cast<uint8_t>(signature.size()));
    witness.bytes.insert(witness.bytes.end(), signature.begin(), signature.end());

    // Push OP_FALSE (0) for OP_IF
    witness.bytes.push_back(0x00);

    return witness;
}

bool HTLCScript::IsHTLCScript(const Script& script) {
    // Check for OP_IF at start and OP_ENDIF at end
    if (script.bytes.empty() || script.bytes[0] != static_cast<uint8_t>(OpCode::OP_IF)) {
        return false;
    }

    if (script.bytes.back() != static_cast<uint8_t>(OpCode::OP_ENDIF)) {
        return false;
    }

    // Check for OP_ELSE in middle
    bool has_else = false;
    for (uint8_t byte : script.bytes) {
        if (byte == static_cast<uint8_t>(OpCode::OP_ELSE)) {
            has_else = true;
            break;
        }
    }

    return has_else;
}

Result<HTLCParameters> HTLCScript::ExtractHTLCParameters(const Script& script) {
    if (!IsHTLCScript(script)) {
        return Result<HTLCParameters>::Error("Not a valid HTLC script");
    }

    // TODO: Implement full script parsing
    // For now, return placeholder
    HTLCParameters params;
    return Result<HTLCParameters>::Error("HTLC parameter extraction not yet implemented");
}

std::vector<uint8_t> HTLCScript::HashPreimage(const std::vector<uint8_t>& preimage,
                                              HTLCHashAlgorithm algorithm) {
    switch (algorithm) {
        case HTLCHashAlgorithm::SHA3_256: {
            uint256 hash = SHA3_256(preimage);
            return std::vector<uint8_t>(hash.begin(), hash.end());
        }
        case HTLCHashAlgorithm::SHA256: {
            // Bitcoin-compatible SHA-256
            uint256 hash = BitcoinHash::SHA256(preimage);
            return std::vector<uint8_t>(hash.begin(), hash.end());
        }
        case HTLCHashAlgorithm::RIPEMD160: {
            // Bitcoin-compatible RIPEMD-160
            auto hash = BitcoinHash::RIPEMD160(preimage);
            return std::vector<uint8_t>(hash.begin(), hash.end());
        }
    }

    return {};
}

bool HTLCScript::VerifyPreimage(const std::vector<uint8_t>& preimage,
                               const std::vector<uint8_t>& hash,
                               HTLCHashAlgorithm algorithm) {
    std::vector<uint8_t> computed_hash = HashPreimage(preimage, algorithm);
    return computed_hash == hash;
}

// ============================================================================
// HTLCTransactionBuilder Implementation
// ============================================================================

HTLCTransactionBuilder::HTLCTransactionBuilder() {}

Result<Transaction> HTLCTransactionBuilder::CreateFundingTransaction(
    const std::vector<TxIn>& inputs,
    const HTLCParameters& htlc_params,
    uint64_t amount,
    const std::string& change_address,
    uint64_t fee_rate) {

    Transaction tx;
    tx.version = 1;
    tx.inputs = inputs;

    // Calculate total input value
    // TODO: Get input values from UTXO set
    uint64_t total_input = amount + 10000;  // Placeholder

    // Create HTLC output
    TxOut htlc_output;
    htlc_output.value = amount;
    htlc_output.script_pubkey = HTLCScript::CreateHTLCScript(htlc_params);
    tx.outputs.push_back(htlc_output);

    // Estimate transaction size
    size_t estimated_size = EstimateHTLCTransactionSize(inputs.size(), 2, false);
    uint64_t fee = CalculateFee(estimated_size, fee_rate);

    // Create change output if needed
    if (total_input > amount + fee) {
        TxOut change_output;
        change_output.value = total_input - amount - fee;
        // TODO: Parse change_address and create script_pubkey
        tx.outputs.push_back(change_output);
    }

    tx.locktime = 0;

    LogF(LogLevel::INFO, "HTLC: Created funding transaction for %llu INTS (fee: %llu INTS)",
         amount, fee);

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> HTLCTransactionBuilder::CreateClaimTransaction(
    const OutPoint& htlc_outpoint,
    uint64_t htlc_amount,
    const Script& htlc_script,
    const std::vector<uint8_t>& preimage,
    const std::string& recipient_address,
    uint64_t fee_rate) {

    Transaction tx;
    tx.version = 1;

    // Create input spending HTLC
    TxIn input;
    input.prev_tx_hash = htlc_outpoint.tx_hash;
    input.prev_tx_index = htlc_outpoint.index;
    // Witness will be added after signing
    tx.inputs.push_back(input);

    // Estimate size and calculate fee
    size_t estimated_size = EstimateHTLCTransactionSize(1, 1, true);
    uint64_t fee = CalculateFee(estimated_size, fee_rate);

    if (fee >= htlc_amount) {
        return Result<Transaction>::Error("Fee exceeds HTLC amount");
    }

    // Create output to recipient
    TxOut output;
    output.value = htlc_amount - fee;
    // TODO: Parse recipient_address and create script_pubkey
    tx.outputs.push_back(output);

    tx.locktime = 0;

    LogF(LogLevel::INFO, "HTLC: Created claim transaction for %llu INTS (fee: %llu INTS)",
         htlc_amount, fee);

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> HTLCTransactionBuilder::CreateRefundTransaction(
    const OutPoint& htlc_outpoint,
    uint64_t htlc_amount,
    const Script& htlc_script,
    const std::string& refund_address,
    uint64_t locktime,
    uint64_t fee_rate) {

    Transaction tx;
    tx.version = 1;

    // Create input spending HTLC
    TxIn input;
    input.prev_tx_hash = htlc_outpoint.tx_hash;
    input.prev_tx_index = htlc_outpoint.index;
    input.sequence = 0xFFFFFFFE;  // Enable locktime
    tx.inputs.push_back(input);

    // Estimate size and calculate fee
    size_t estimated_size = EstimateHTLCTransactionSize(1, 1, false);
    uint64_t fee = CalculateFee(estimated_size, fee_rate);

    if (fee >= htlc_amount) {
        return Result<Transaction>::Error("Fee exceeds HTLC amount");
    }

    // Create output to refund address
    TxOut output;
    output.value = htlc_amount - fee;
    // TODO: Parse refund_address and create script_pubkey
    tx.outputs.push_back(output);

    // Set locktime
    tx.locktime = locktime;

    LogF(LogLevel::INFO, "HTLC: Created refund transaction for %llu INTS (locktime: %llu, fee: %llu INTS)",
         htlc_amount, locktime, fee);

    return Result<Transaction>::Ok(tx);
}

size_t HTLCTransactionBuilder::EstimateHTLCTransactionSize(size_t num_inputs,
                                                           size_t num_outputs,
                                                           bool is_claim) {
    // Base transaction size
    size_t size = 10;  // version + locktime

    // Input size
    // Each input: outpoint (36) + script_sig length (1) + witness data + sequence (4)
    // Witness data: signature (~96 for Dilithium3) + preimage (32) for claim
    //               signature (~96) for refund
    size_t witness_size = is_claim ? (96 + 32 + 10) : (96 + 10);
    size += num_inputs * (36 + 1 + witness_size + 4);

    // Output size
    // Each output: value (8) + script_pubkey length (1) + script_pubkey (~40-100)
    size += num_outputs * (8 + 1 + 50);

    return size;
}

uint64_t HTLCTransactionBuilder::CalculateFee(size_t tx_size, uint64_t fee_rate) {
    // Fee = (tx_size / 1000) * fee_rate
    uint64_t fee = (tx_size * fee_rate) / 1000;

    // Minimum fee: 1000 INTS
    if (fee < 1000) {
        fee = 1000;
    }

    return fee;
}

// ============================================================================
// HTLCManager Implementation
// ============================================================================

HTLCManager::HTLCManager() {}

void HTLCManager::AddHTLC(const HTLCInfo& info) {
    htlcs_[info.outpoint] = info;

    LogF(LogLevel::INFO, "HTLC: Added HTLC %s:%u (%llu INTS)",
         BytesToHex(std::vector<uint8_t>(info.outpoint.tx_hash.begin(),
                                        info.outpoint.tx_hash.end())).substr(0, 16).c_str(),
         info.outpoint.index,
         info.amount);
}

void HTLCManager::UpdateHTLCState(const OutPoint& outpoint, HTLCState state) {
    auto it = htlcs_.find(outpoint);
    if (it != htlcs_.end()) {
        it->second.state = state;

        LogF(LogLevel::INFO, "HTLC: Updated state for %s:%u to %d",
             BytesToHex(std::vector<uint8_t>(outpoint.tx_hash.begin(),
                                            outpoint.tx_hash.end())).substr(0, 16).c_str(),
             outpoint.index,
             static_cast<int>(state));
    }
}

Result<HTLCInfo> HTLCManager::GetHTLC(const OutPoint& outpoint) const {
    auto it = htlcs_.find(outpoint);
    if (it == htlcs_.end()) {
        return Result<HTLCInfo>::Error("HTLC not found");
    }

    return Result<HTLCInfo>::Ok(it->second);
}

std::vector<HTLCInfo> HTLCManager::GetAllHTLCs() const {
    std::vector<HTLCInfo> result;
    result.reserve(htlcs_.size());

    for (const auto& [outpoint, info] : htlcs_) {
        result.push_back(info);
    }

    return result;
}

std::vector<HTLCInfo> HTLCManager::GetHTLCsByState(HTLCState state) const {
    std::vector<HTLCInfo> result;

    for (const auto& [outpoint, info] : htlcs_) {
        if (info.state == state) {
            result.push_back(info);
        }
    }

    return result;
}

std::vector<HTLCInfo> HTLCManager::GetExpiredHTLCs(uint64_t current_height,
                                                   uint64_t current_time) const {
    std::vector<HTLCInfo> result;

    for (const auto& [outpoint, info] : htlcs_) {
        if (info.state != HTLCState::FUNDED) {
            continue;
        }

        bool expired = false;
        if (info.params.is_block_height) {
            expired = current_height >= info.params.locktime;
        } else {
            expired = current_time >= info.params.locktime;
        }

        if (expired) {
            result.push_back(info);
        }
    }

    return result;
}

void HTLCManager::RemoveHTLC(const OutPoint& outpoint) {
    htlcs_.erase(outpoint);

    LogF(LogLevel::INFO, "HTLC: Removed HTLC %s:%u",
         BytesToHex(std::vector<uint8_t>(outpoint.tx_hash.begin(),
                                        outpoint.tx_hash.end())).substr(0, 16).c_str(),
         outpoint.index);
}

size_t HTLCManager::GetHTLCCount() const {
    return htlcs_.size();
}

size_t HTLCManager::GetHTLCCountByState(HTLCState state) const {
    size_t count = 0;
    for (const auto& [outpoint, info] : htlcs_) {
        if (info.state == state) {
            count++;
        }
    }
    return count;
}

}  // namespace htlc
}  // namespace intcoin
