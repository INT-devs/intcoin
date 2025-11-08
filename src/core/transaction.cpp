// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/transaction.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <cstring>
#include <ctime>

namespace intcoin {

// OutPoint implementation

std::vector<uint8_t> OutPoint::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), tx_hash.begin(), tx_hash.end());
    buffer.push_back(static_cast<uint8_t>(index & 0xFF));
    buffer.push_back(static_cast<uint8_t>((index >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((index >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((index >> 24) & 0xFF));
    return buffer;
}

// TxInput implementation

std::vector<uint8_t> TxInput::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize outpoint
    auto outpoint_data = previous_output.serialize();
    buffer.insert(buffer.end(), outpoint_data.begin(), outpoint_data.end());

    // Script sig length and data
    uint32_t script_len = static_cast<uint32_t>(script_sig.size());
    buffer.push_back(static_cast<uint8_t>(script_len & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 24) & 0xFF));
    buffer.insert(buffer.end(), script_sig.begin(), script_sig.end());

    // Signature
    buffer.insert(buffer.end(), signature.begin(), signature.end());

    // Sequence
    buffer.push_back(static_cast<uint8_t>(sequence & 0xFF));
    buffer.push_back(static_cast<uint8_t>((sequence >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((sequence >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((sequence >> 24) & 0xFF));

    return buffer;
}

// TxOutput implementation

std::vector<uint8_t> TxOutput::serialize() const {
    std::vector<uint8_t> buffer;

    // Value
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }

    // Script pubkey length and data
    uint32_t script_len = static_cast<uint32_t>(script_pubkey.size());
    buffer.push_back(static_cast<uint8_t>(script_len & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 24) & 0xFF));
    buffer.insert(buffer.end(), script_pubkey.begin(), script_pubkey.end());

    // Public key
    buffer.insert(buffer.end(), pubkey.begin(), pubkey.end());

    return buffer;
}

// Transaction implementation

std::vector<uint8_t> Transaction::serialize() const {
    std::vector<uint8_t> buffer;

    // Version
    buffer.push_back(static_cast<uint8_t>(version & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 24) & 0xFF));

    // Input count
    uint32_t input_count = static_cast<uint32_t>(inputs.size());
    buffer.push_back(static_cast<uint8_t>(input_count & 0xFF));
    buffer.push_back(static_cast<uint8_t>((input_count >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((input_count >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((input_count >> 24) & 0xFF));

    // Inputs
    for (const auto& input : inputs) {
        auto input_data = input.serialize();
        buffer.insert(buffer.end(), input_data.begin(), input_data.end());
    }

    // Output count
    uint32_t output_count = static_cast<uint32_t>(outputs.size());
    buffer.push_back(static_cast<uint8_t>(output_count & 0xFF));
    buffer.push_back(static_cast<uint8_t>((output_count >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((output_count >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((output_count >> 24) & 0xFF));

    // Outputs
    for (const auto& output : outputs) {
        auto output_data = output.serialize();
        buffer.insert(buffer.end(), output_data.begin(), output_data.end());
    }

    // Locktime
    buffer.push_back(static_cast<uint8_t>(lock_time & 0xFF));
    buffer.push_back(static_cast<uint8_t>((lock_time >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((lock_time >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((lock_time >> 24) & 0xFF));

    // Timestamp
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }

    return buffer;
}

Transaction Transaction::deserialize(const std::vector<uint8_t>& data) {
    Transaction tx;

    if (data.size() < 8) return tx;  // Minimum size check

    size_t offset = 0;

    // Version (4 bytes)
    tx.version = data[offset] |
                 (data[offset + 1] << 8) |
                 (data[offset + 2] << 16) |
                 (data[offset + 3] << 24);
    offset += 4;

    // Timestamp (8 bytes)
    tx.timestamp = 0;
    for (int i = 0; i < 8 && offset + i < data.size(); i++) {
        tx.timestamp |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    if (offset >= data.size()) return tx;

    // Input count (1 byte varint - simplified)
    uint8_t input_count = data[offset++];

    // Inputs
    for (uint8_t i = 0; i < input_count && offset < data.size(); i++) {
        TxInput input;

        // Previous output hash (32 bytes)
        if (offset + 32 > data.size()) break;
        std::copy(data.begin() + offset, data.begin() + offset + 32,
                 input.previous_output.tx_hash.begin());
        offset += 32;

        // Previous output index (4 bytes)
        if (offset + 4 > data.size()) break;
        input.previous_output.index = data[offset] |
                                     (data[offset + 1] << 8) |
                                     (data[offset + 2] << 16) |
                                     (data[offset + 3] << 24);
        offset += 4;

        // Signature script length (4 bytes)
        if (offset + 4 > data.size()) break;
        uint32_t script_len = data[offset] |
                             (data[offset + 1] << 8) |
                             (data[offset + 2] << 16) |
                             (data[offset + 3] << 24);
        offset += 4;

        // Signature script data
        if (offset + script_len > data.size()) break;
        input.script_sig.assign(data.begin() + offset,
                                     data.begin() + offset + script_len);
        offset += script_len;

        // Sequence (4 bytes)
        if (offset + 4 > data.size()) break;
        input.sequence = data[offset] |
                        (data[offset + 1] << 8) |
                        (data[offset + 2] << 16) |
                        (data[offset + 3] << 24);
        offset += 4;

        tx.inputs.push_back(input);
    }

    if (offset >= data.size()) return tx;

    // Output count (1 byte varint - simplified)
    uint8_t output_count = data[offset++];

    // Outputs
    for (uint8_t i = 0; i < output_count && offset < data.size(); i++) {
        TxOutput output;

        // Value (8 bytes)
        if (offset + 8 > data.size()) break;
        output.value = 0;
        for (int j = 0; j < 8; j++) {
            output.value |= (static_cast<int64_t>(data[offset + j]) << (j * 8));
        }
        offset += 8;

        // Pubkey script length (4 bytes)
        if (offset + 4 > data.size()) break;
        uint32_t script_len = data[offset] |
                             (data[offset + 1] << 8) |
                             (data[offset + 2] << 16) |
                             (data[offset + 3] << 24);
        offset += 4;

        // Pubkey script data
        if (offset + script_len > data.size()) break;
        output.script_pubkey.assign(data.begin() + offset,
                                   data.begin() + offset + script_len);
        offset += script_len;

        tx.outputs.push_back(output);
    }

    // Locktime (4 bytes)
    if (offset + 4 <= data.size()) {
        tx.lock_time = data[offset] |
                      (data[offset + 1] << 8) |
                      (data[offset + 2] << 16) |
                      (data[offset + 3] << 24);
    }

    return tx;
}

Hash256 Transaction::get_hash() const {
    std::vector<uint8_t> serialized = serialize();
    return crypto::SHA3_256::hash(serialized.data(), serialized.size());
}

std::string Transaction::get_txid() const {
    // Implement hex conversion
    Hash256 hash = get_hash();

    const char* hex_chars = "0123456789abcdef";
    std::string result;
    result.reserve(64);  // 32 bytes = 64 hex characters

    for (uint8_t byte : hash) {
        result += hex_chars[(byte >> 4) & 0x0F];
        result += hex_chars[byte & 0x0F];
    }

    return result;
}

size_t Transaction::get_size() const {
    return serialize().size();
}

size_t Transaction::get_weight() const {
    return get_size();
}

uint64_t Transaction::get_input_value() const {
    // NOTE: This requires UTXO set lookup from blockchain context
    // Cannot be implemented at transaction level without blockchain reference
    // Call Blockchain::get_transaction_input_value() instead
    // Returning 0 as placeholder - this should not be used directly
    return 0;
}

uint64_t Transaction::get_output_value() const {
    uint64_t total = 0;
    for (const auto& output : outputs) {
        total += output.value;
    }
    return total;
}

bool Transaction::validate_structure() const {
    if (inputs.empty() || outputs.empty()) {
        return false;
    }
    return true;
}

bool Transaction::sign(const std::vector<uint8_t>& private_key, size_t input_index) {
    if (input_index >= inputs.size()) {
        return false;
    }

    // Create signature hash for this input
    auto serialized = serialize();
    auto sig_hash = crypto::SHA3_256::hash(serialized.data(), serialized.size());
    std::vector<uint8_t> sig_hash_vec(sig_hash.begin(), sig_hash.end());

    // Convert private key to Dilithium keypair structure
    if (private_key.size() != 4896) {  // Dilithium5 private key size
        return false;
    }

    crypto::DilithiumKeyPair keypair;
    std::memcpy(keypair.private_key.data(), private_key.data(), private_key.size());

    // Sign the hash
    DilithiumSignature signature = crypto::Dilithium::sign(sig_hash_vec, keypair);

    // Set signature in TxInput
    inputs[input_index].signature = signature;

    // Set script_sig (signature + pubkey)
    inputs[input_index].script_sig.clear();
    inputs[input_index].script_sig.insert(
        inputs[input_index].script_sig.end(),
        signature.begin(),
        signature.end()
    );

    // Append public key
    inputs[input_index].script_sig.insert(
        inputs[input_index].script_sig.end(),
        keypair.public_key.begin(),
        keypair.public_key.end()
    );

    return true;
}

bool Transaction::verify_signature(size_t input_index) const {
    if (input_index >= inputs.size()) {
        return false;
    }

    const auto& input = inputs[input_index];

    // Signature script should contain: signature (4627 bytes) + pubkey (2592 bytes)
    constexpr size_t DILITHIUM_SIG_SIZE = 4627;
    constexpr size_t DILITHIUM_PUBKEY_SIZE = 2592;
    constexpr size_t EXPECTED_SIZE = DILITHIUM_SIG_SIZE + DILITHIUM_PUBKEY_SIZE;

    if (input.script_sig.size() != EXPECTED_SIZE) {
        return false;  // Invalid signature script size
    }

    // Extract signature
    DilithiumSignature signature;
    std::memcpy(signature.data(), input.script_sig.data(), DILITHIUM_SIG_SIZE);

    // Extract public key
    DilithiumPubKey pubkey;
    std::memcpy(pubkey.data(), input.script_sig.data() + DILITHIUM_SIG_SIZE, DILITHIUM_PUBKEY_SIZE);

    // Get signature hash
    auto serialized = serialize();
    auto sig_hash = crypto::SHA3_256::hash(serialized.data(), serialized.size());
    std::vector<uint8_t> sig_hash_vec(sig_hash.begin(), sig_hash.end());

    // Verify signature
    return crypto::Dilithium::verify(sig_hash_vec, signature, pubkey);
}

bool Transaction::verify_all_signatures() const {
    // Verify all input signatures
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (!verify_signature(i)) {
            return false;
        }
    }
    return true;
}

Transaction Transaction::create_coinbase(
    uint32_t height,
    uint64_t reward,
    const DilithiumPubKey& miner_pubkey,
    const std::string& extra_data
) {
    Transaction tx;
    tx.version = 1;
    tx.lock_time = 0;
    // Set current timestamp
    tx.timestamp = static_cast<uint64_t>(std::time(nullptr));

    // Coinbase input
    TxInput coinbase_input;
    coinbase_input.previous_output = OutPoint(Hash256{}, 0xFFFFFFFF);
    coinbase_input.script_sig.assign(extra_data.begin(), extra_data.end());
    coinbase_input.sequence = 0xFFFFFFFF;

    tx.inputs.push_back(coinbase_input);

    // Coinbase output
    TxOutput coinbase_output;
    coinbase_output.value = reward;
    coinbase_output.pubkey = miner_pubkey;

    tx.outputs.push_back(coinbase_output);

    return tx;
}

// UTXO implementation

std::vector<uint8_t> UTXO::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize outpoint
    auto outpoint_data = outpoint.serialize();
    buffer.insert(buffer.end(), outpoint_data.begin(), outpoint_data.end());

    // Serialize block height (4 bytes)
    buffer.push_back(static_cast<uint8_t>(height & 0xFF));
    buffer.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((height >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((height >> 24) & 0xFF));

    // Serialize is_coinbase (1 byte)
    buffer.push_back(is_coinbase ? 1 : 0);

    // Serialize output value (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((output.value >> (i * 8)) & 0xFF));
    }

    // Serialize script length (4 bytes)
    uint32_t script_len = static_cast<uint32_t>(output.script_pubkey.size());
    buffer.push_back(static_cast<uint8_t>(script_len & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((script_len >> 24) & 0xFF));

    // Serialize script
    buffer.insert(buffer.end(), output.script_pubkey.begin(), output.script_pubkey.end());

    return buffer;
}

UTXO UTXO::deserialize(const std::vector<uint8_t>& data) {
    UTXO utxo;

    if (data.size() < 49) return utxo;  // Minimum: 32+4+4+1+8 = 49 bytes

    size_t offset = 0;

    // Deserialize transaction hash (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, utxo.outpoint.tx_hash.begin());
    offset += 32;

    // Deserialize output index (4 bytes)
    utxo.outpoint.index = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Deserialize block height (4 bytes)
    utxo.height = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Deserialize is_coinbase (1 byte)
    utxo.is_coinbase = (data[offset] != 0);
    offset += 1;

    // Deserialize output value (8 bytes)
    utxo.output.value = 0;
    for (int i = 0; i < 8; i++) {
        utxo.output.value |= (static_cast<int64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    if (data.size() < offset + 4) return utxo;

    // Deserialize script length (4 bytes)
    uint32_t script_len = static_cast<uint32_t>(data[offset]) |
                          (static_cast<uint32_t>(data[offset + 1]) << 8) |
                          (static_cast<uint32_t>(data[offset + 2]) << 16) |
                          (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    if (data.size() < offset + script_len) return utxo;

    // Deserialize script
    utxo.output.script_pubkey.assign(data.begin() + offset, data.begin() + offset + script_len);

    return utxo;
}

// TransactionBuilder implementation

TransactionBuilder& TransactionBuilder::add_input(const OutPoint& outpoint) {
    TxInput input;
    input.previous_output = outpoint;
    tx_.inputs.push_back(input);
    return *this;
}

TransactionBuilder& TransactionBuilder::add_output(uint64_t value, const DilithiumPubKey& pubkey) {
    TxOutput output;
    output.value = value;
    output.pubkey = pubkey;
    tx_.outputs.push_back(output);
    return *this;
}

TransactionBuilder& TransactionBuilder::set_lock_time(uint32_t lock_time) {
    tx_.lock_time = lock_time;
    return *this;
}

Transaction TransactionBuilder::build() {
    return tx_;
}

} // namespace intcoin
