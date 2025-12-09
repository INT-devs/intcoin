/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Transaction Implementation
 */

#include "intcoin/transaction.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <algorithm>

namespace intcoin {

// ============================================================================
// TxIn Implementation
// ============================================================================

std::vector<uint8_t> TxIn::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint256(result, prev_tx_hash);
    SerializeUint32(result, prev_tx_index);
    auto script_bytes = script_sig.Serialize();
    SerializeUint64(result, script_bytes.size());
    result.insert(result.end(), script_bytes.begin(), script_bytes.end());
    SerializeUint32(result, sequence);
    return result;
}

Result<TxIn> TxIn::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    TxIn txin;

    // Deserialize prev_tx_hash (32 bytes)
    auto hash_result = DeserializeUint256(data, pos);
    if (hash_result.IsError()) {
        return Result<TxIn>::Error("Failed to deserialize prev_tx_hash: " + hash_result.error);
    }
    txin.prev_tx_hash = *hash_result.value;

    // Deserialize prev_tx_index (4 bytes)
    auto index_result = DeserializeUint32(data, pos);
    if (index_result.IsError()) {
        return Result<TxIn>::Error("Failed to deserialize prev_tx_index: " + index_result.error);
    }
    txin.prev_tx_index = *index_result.value;

    // Deserialize script_sig length (8 bytes)
    auto script_len_result = DeserializeUint64(data, pos);
    if (script_len_result.IsError()) {
        return Result<TxIn>::Error("Failed to deserialize script_sig length: " + script_len_result.error);
    }
    uint64_t script_len = *script_len_result.value;

    // Deserialize script_sig bytes
    if (pos + script_len > data.size()) {
        return Result<TxIn>::Error("Buffer underflow: not enough bytes for script_sig");
    }
    std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
    txin.script_sig = Script::Deserialize(script_bytes);
    pos += script_len;

    // Deserialize sequence (4 bytes)
    auto seq_result = DeserializeUint32(data, pos);
    if (seq_result.IsError()) {
        return Result<TxIn>::Error("Failed to deserialize sequence: " + seq_result.error);
    }
    txin.sequence = *seq_result.value;

    return Result<TxIn>::Ok(std::move(txin));
}

size_t TxIn::GetSerializedSize() const {
    return 32 + 4 + 8 + script_sig.GetSize() + 4;
}

// ============================================================================
// TxOut Implementation
// ============================================================================

std::vector<uint8_t> TxOut::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint64(result, value);
    auto script_bytes = script_pubkey.Serialize();
    SerializeUint64(result, script_bytes.size());
    result.insert(result.end(), script_bytes.begin(), script_bytes.end());
    return result;
}

Result<TxOut> TxOut::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    TxOut txout;

    // Deserialize value (8 bytes)
    auto value_result = DeserializeUint64(data, pos);
    if (value_result.IsError()) {
        return Result<TxOut>::Error("Failed to deserialize value: " + value_result.error);
    }
    txout.value = *value_result.value;

    // Deserialize script_pubkey length (8 bytes)
    auto script_len_result = DeserializeUint64(data, pos);
    if (script_len_result.IsError()) {
        return Result<TxOut>::Error("Failed to deserialize script_pubkey length: " + script_len_result.error);
    }
    uint64_t script_len = *script_len_result.value;

    // Deserialize script_pubkey bytes
    if (pos + script_len > data.size()) {
        return Result<TxOut>::Error("Buffer underflow: not enough bytes for script_pubkey");
    }
    std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
    txout.script_pubkey = Script::Deserialize(script_bytes);
    pos += script_len;

    return Result<TxOut>::Ok(std::move(txout));
}

size_t TxOut::GetSerializedSize() const {
    return 8 + 8 + script_pubkey.GetSize();
}

// ============================================================================
// OutPoint Implementation
// ============================================================================

bool OutPoint::operator==(const OutPoint& other) const {
    return tx_hash == other.tx_hash && index == other.index;
}

bool OutPoint::operator<(const OutPoint& other) const {
    if (tx_hash < other.tx_hash) return true;
    if (tx_hash == other.tx_hash) return index < other.index;
    return false;
}

std::vector<uint8_t> OutPoint::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint256(result, tx_hash);
    SerializeUint32(result, index);
    return result;
}

Result<OutPoint> OutPoint::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    OutPoint outpoint;

    // Deserialize tx_hash (32 bytes)
    auto hash_result = DeserializeUint256(data, pos);
    if (hash_result.IsError()) {
        return Result<OutPoint>::Error("Failed to deserialize tx_hash: " + hash_result.error);
    }
    outpoint.tx_hash = *hash_result.value;

    // Deserialize index (4 bytes)
    auto index_result = DeserializeUint32(data, pos);
    if (index_result.IsError()) {
        return Result<OutPoint>::Error("Failed to deserialize index: " + index_result.error);
    }
    outpoint.index = *index_result.value;

    return Result<OutPoint>::Ok(std::move(outpoint));
}

std::size_t OutPointHash::operator()(const OutPoint& outpoint) const noexcept {
    // Simple hash combination
    size_t h1 = std::hash<std::string>{}(ToHex(outpoint.tx_hash));
    size_t h2 = std::hash<uint32_t>{}(outpoint.index);
    return h1 ^ (h2 << 1);
}

// ============================================================================
// Transaction Implementation
// ============================================================================

uint256 Transaction::GetHash() const {
    if (cached_hash_.has_value()) {
        return *cached_hash_;
    }

    auto serialized = Serialize();
    uint256 hash = SHA3::Hash(serialized);
    cached_hash_ = hash;
    return hash;
}

uint256 Transaction::GetHashForSigning(uint8_t sighash_type, size_t input_index) const {
    // Create a copy of the transaction for signing
    std::vector<uint8_t> signing_data;

    // Serialize version
    SerializeUint32(signing_data, version);

    // Determine base SIGHASH type
    SigHashType base_type = GetBaseSigHashType(sighash_type);
    bool anyonecanpay = HasAnyoneCanPay(sighash_type);

    // Serialize inputs based on ANYONECANPAY flag
    if (anyonecanpay) {
        // ANYONECANPAY: Only serialize the input being signed
        SerializeUint64(signing_data, 1);
        if (input_index < inputs.size()) {
            auto input_bytes = inputs[input_index].Serialize();
            signing_data.insert(signing_data.end(), input_bytes.begin(), input_bytes.end());
        }
    } else {
        // Normal: Serialize all inputs (with script_sig cleared for others)
        SerializeUint64(signing_data, inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            TxIn input = inputs[i];
            if (i != input_index) {
                // Clear script_sig for inputs other than the one being signed
                input.script_sig = Script(std::vector<uint8_t>());
            }
            auto input_bytes = input.Serialize();
            signing_data.insert(signing_data.end(), input_bytes.begin(), input_bytes.end());
        }
    }

    // Serialize outputs based on SIGHASH type
    switch (base_type) {
        case SigHashType::ALL:
            // SIGHASH_ALL: Include all outputs
            SerializeUint64(signing_data, outputs.size());
            for (const auto& output : outputs) {
                auto output_bytes = output.Serialize();
                signing_data.insert(signing_data.end(), output_bytes.begin(), output_bytes.end());
            }
            break;

        case SigHashType::NONE:
            // SIGHASH_NONE: No outputs
            SerializeUint64(signing_data, 0);
            break;

        case SigHashType::SINGLE:
            // SIGHASH_SINGLE: Only output at same index
            if (input_index < outputs.size()) {
                SerializeUint64(signing_data, 1);
                auto output_bytes = outputs[input_index].Serialize();
                signing_data.insert(signing_data.end(), output_bytes.begin(), output_bytes.end());
            } else {
                // If no corresponding output, serialize 0 outputs
                SerializeUint64(signing_data, 0);
            }
            break;

        case SigHashType::ANYONECANPAY:
            // ANYONECANPAY is a modifier, not a base type
            // It should have been filtered out by GetBaseSigHashType
            // Default to ALL behavior
            SerializeUint64(signing_data, outputs.size());
            for (const auto& output : outputs) {
                auto output_bytes = output.Serialize();
                signing_data.insert(signing_data.end(), output_bytes.begin(), output_bytes.end());
            }
            break;
    }

    // Serialize locktime
    SerializeUint64(signing_data, locktime);

    // Append SIGHASH type
    signing_data.push_back(sighash_type);

    // Hash the signing data
    return SHA3::Hash(signing_data);
}

Result<void> Transaction::Sign(const SecretKey& secret_key, uint8_t sighash_type) {
    // Get the hash to sign (SIGHASH_ALL by default, signs entire transaction)
    uint256 hash = GetHashForSigning(sighash_type, 0);

    // Sign the hash using Dilithium3
    auto sign_result = DilithiumCrypto::SignHash(hash, secret_key);
    if (!sign_result.IsOk()) {
        return Result<void>::Error("Failed to sign transaction: " + sign_result.error);
    }

    // Store the signature
    signature = *sign_result.value;

    // Clear cached hash since signature changed
    cached_hash_.reset();

    return Result<void>::Ok();
}

Result<void> Transaction::VerifySignature(const PublicKey& public_key, uint8_t sighash_type) const {
    // Get the hash that was signed
    uint256 hash = GetHashForSigning(sighash_type, 0);

    // Verify the signature using Dilithium3
    auto verify_result = DilithiumCrypto::VerifyHash(hash, signature, public_key);
    if (!verify_result.IsOk()) {
        return Result<void>::Error("Signature verification failed: " + verify_result.error);
    }

    return Result<void>::Ok();
}

bool Transaction::IsCoinbase() const {
    if (inputs.size() != 1) return false;
    // Check if prev_tx_hash is all zeros
    return std::all_of(inputs[0].prev_tx_hash.begin(), inputs[0].prev_tx_hash.end(),
                      [](uint8_t b) { return b == 0; });
}

uint64_t Transaction::GetTotalOutputValue() const {
    uint64_t total = 0;
    for (const auto& output : outputs) {
        total += output.value;
    }
    return total;
}

std::vector<uint8_t> Transaction::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint32(result, version);
    SerializeUint64(result, inputs.size());
    for (const auto& input : inputs) {
        auto input_bytes = input.Serialize();
        result.insert(result.end(), input_bytes.begin(), input_bytes.end());
    }
    SerializeUint64(result, outputs.size());
    for (const auto& output : outputs) {
        auto output_bytes = output.Serialize();
        result.insert(result.end(), output_bytes.begin(), output_bytes.end());
    }
    SerializeUint64(result, locktime);
    // Serialize signature (convert std::array to bytes)
    result.insert(result.end(), signature.begin(), signature.end());
    return result;
}

Result<Transaction> Transaction::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    Transaction tx;

    // Deserialize version (4 bytes)
    auto version_result = DeserializeUint32(data, pos);
    if (version_result.IsError()) {
        return Result<Transaction>::Error("Failed to deserialize version: " + version_result.error);
    }
    tx.version = *version_result.value;

    // Deserialize inputs count (8 bytes)
    auto inputs_count_result = DeserializeUint64(data, pos);
    if (inputs_count_result.IsError()) {
        return Result<Transaction>::Error("Failed to deserialize inputs count: " + inputs_count_result.error);
    }
    uint64_t inputs_count = *inputs_count_result.value;

    // Deserialize each input
    tx.inputs.reserve(inputs_count);
    for (uint64_t i = 0; i < inputs_count; ++i) {
        TxIn txin;

        // Deserialize prev_tx_hash (32 bytes)
        auto hash_result = DeserializeUint256(data, pos);
        if (hash_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize input " + std::to_string(i) + " prev_tx_hash: " + hash_result.error);
        }
        txin.prev_tx_hash = *hash_result.value;

        // Deserialize prev_tx_index (4 bytes)
        auto index_result = DeserializeUint32(data, pos);
        if (index_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize input " + std::to_string(i) + " prev_tx_index: " + index_result.error);
        }
        txin.prev_tx_index = *index_result.value;

        // Deserialize script_sig length (8 bytes)
        auto script_len_result = DeserializeUint64(data, pos);
        if (script_len_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize input " + std::to_string(i) + " script_sig length: " + script_len_result.error);
        }
        uint64_t script_len = *script_len_result.value;

        // Deserialize script_sig bytes
        if (pos + script_len > data.size()) {
            return Result<Transaction>::Error("Buffer underflow: not enough bytes for input " + std::to_string(i) + " script_sig");
        }
        std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
        txin.script_sig = Script::Deserialize(script_bytes);
        pos += script_len;

        // Deserialize sequence (4 bytes)
        auto seq_result = DeserializeUint32(data, pos);
        if (seq_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize input " + std::to_string(i) + " sequence: " + seq_result.error);
        }
        txin.sequence = *seq_result.value;

        tx.inputs.push_back(std::move(txin));
    }

    // Deserialize outputs count (8 bytes)
    auto outputs_count_result = DeserializeUint64(data, pos);
    if (outputs_count_result.IsError()) {
        return Result<Transaction>::Error("Failed to deserialize outputs count: " + outputs_count_result.error);
    }
    uint64_t outputs_count = *outputs_count_result.value;

    // Deserialize each output
    tx.outputs.reserve(outputs_count);
    for (uint64_t i = 0; i < outputs_count; ++i) {
        TxOut txout;

        // Deserialize value (8 bytes)
        auto value_result = DeserializeUint64(data, pos);
        if (value_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize output " + std::to_string(i) + " value: " + value_result.error);
        }
        txout.value = *value_result.value;

        // Deserialize script_pubkey length (8 bytes)
        auto script_len_result = DeserializeUint64(data, pos);
        if (script_len_result.IsError()) {
            return Result<Transaction>::Error("Failed to deserialize output " + std::to_string(i) + " script_pubkey length: " + script_len_result.error);
        }
        uint64_t script_len = *script_len_result.value;

        // Deserialize script_pubkey bytes
        if (pos + script_len > data.size()) {
            return Result<Transaction>::Error("Buffer underflow: not enough bytes for output " + std::to_string(i) + " script_pubkey");
        }
        std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
        txout.script_pubkey = Script::Deserialize(script_bytes);
        pos += script_len;

        tx.outputs.push_back(std::move(txout));
    }

    // Deserialize locktime (8 bytes)
    auto locktime_result = DeserializeUint64(data, pos);
    if (locktime_result.IsError()) {
        return Result<Transaction>::Error("Failed to deserialize locktime: " + locktime_result.error);
    }
    tx.locktime = *locktime_result.value;

    // Deserialize signature (DILITHIUM3_BYTES = 3293 bytes)
    if (pos + DILITHIUM3_BYTES > data.size()) {
        return Result<Transaction>::Error("Buffer underflow: not enough bytes for signature");
    }
    std::copy(data.begin() + pos, data.begin() + pos + DILITHIUM3_BYTES, tx.signature.begin());
    pos += DILITHIUM3_BYTES;

    return Result<Transaction>::Ok(std::move(tx));
}

size_t Transaction::GetSerializedSize() const {
    size_t size = 4 + 8 + 8 + 8 + DILITHIUM3_BYTES;
    for (const auto& input : inputs) {
        size += input.GetSerializedSize();
    }
    for (const auto& output : outputs) {
        size += output.GetSerializedSize();
    }
    return size;
}

// ============================================================================
// TransactionBuilder Implementation
// ============================================================================

TransactionBuilder& TransactionBuilder::AddInput(const OutPoint& outpoint,
                                                const Script& script_sig) {
    TxIn input;
    input.prev_tx_hash = outpoint.tx_hash;
    input.prev_tx_index = outpoint.index;
    input.script_sig = script_sig;
    tx_.inputs.push_back(input);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddOutput(uint64_t value,
                                                 const Script& script_pubkey) {
    TxOut output(value, script_pubkey);
    tx_.outputs.push_back(output);
    return *this;
}

TransactionBuilder& TransactionBuilder::SetLocktime(uint64_t locktime) {
    tx_.locktime = locktime;
    return *this;
}

Result<Transaction> TransactionBuilder::Build(const SecretKey& secret_key) {
    // Sign transaction
    auto result = tx_.Sign(secret_key);
    if (result.IsError()) {
        return Result<Transaction>::Error(result.error);
    }
    return Result<Transaction>::Ok(tx_);
}

// ============================================================================
// Coinbase Transaction
// ============================================================================

Transaction CreateCoinbaseTransaction(uint64_t height, uint64_t block_reward,
                                     const PublicKey& miner_pubkey) {
    Transaction tx;
    tx.version = 1;

    // Coinbase input (null input)
    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256(); // Zero hash
    coinbase_input.prev_tx_index = 0xFFFFFFFF;
    coinbase_input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(coinbase_input);

    // Coinbase output
    uint256 pubkey_hash = PublicKeyToHash(miner_pubkey);
    Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
    TxOut coinbase_output(block_reward, script_pubkey);
    tx.outputs.push_back(coinbase_output);

    tx.locktime = 0;

    return tx;
}

} // namespace intcoin
