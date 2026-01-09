// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/contracts/transaction.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace intcoin {
namespace contracts {

//
// ContractDeploymentTx Implementation
//

uint256 ContractDeploymentTx::GetTxID() const {
    auto serialized = Serialize();
    return SHA3::Hash(serialized.data(), serialized.size());
}

std::string ContractDeploymentTx::GetContractAddress() const {
    // Contract address = Bech32(SHA3-256(from || nonce)[:20])
    std::vector<uint8_t> data;
    data.reserve(from.size() + 8);

    // Append from public key
    data.insert(data.end(), from.begin(), from.end());

    // Append nonce (big-endian)
    for (int i = 7; i >= 0; --i) {
        data.push_back((nonce >> (i * 8)) & 0xff);
    }

    // Hash and take first 20 bytes
    uint256 hash = SHA3::Hash(data.data(), data.size());
    std::vector<uint8_t> addr_bytes(hash.begin(), hash.begin() + 20);

    // Convert to Bech32 with 'int1' prefix (contracts)
    return Bech32Encode("int1", addr_bytes);
}

std::vector<uint8_t> ContractDeploymentTx::Serialize() const {
    std::vector<uint8_t> result;

    // Type (1 byte)
    result.push_back(type);

    // Version (4 bytes)
    for (int i = 3; i >= 0; --i) {
        result.push_back((version >> (i * 8)) & 0xff);
    }

    // From public key (DILITHIUM3_PUBLICKEYBYTES)
    result.insert(result.end(), from.begin(), from.end());

    // Nonce (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((nonce >> (i * 8)) & 0xff);
    }

    // Value (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((value >> (i * 8)) & 0xff);
    }

    // Bytecode length (4 bytes)
    uint32_t bytecode_len = bytecode.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back((bytecode_len >> (i * 8)) & 0xff);
    }

    // Bytecode
    result.insert(result.end(), bytecode.begin(), bytecode.end());

    // Constructor args length (4 bytes)
    uint32_t args_len = constructor_args.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back((args_len >> (i * 8)) & 0xff);
    }

    // Constructor args
    result.insert(result.end(), constructor_args.begin(), constructor_args.end());

    // Gas limit (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((gas_limit >> (i * 8)) & 0xff);
    }

    // Gas price (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((gas_price >> (i * 8)) & 0xff);
    }

    // Signature (DILITHIUM3_BYTES)
    result.insert(result.end(), signature.begin(), signature.end());

    return result;
}

std::optional<ContractDeploymentTx> ContractDeploymentTx::Deserialize(const std::vector<uint8_t>& data) {
    // Simplified deserialization - full implementation would include proper error checking
    if (data.size() < 13) {
        return std::nullopt;
    }

    ContractDeploymentTx tx;
    size_t offset = 0;

    // Type
    tx.type = data[offset++];
    if (tx.type != 2) {
        return std::nullopt;
    }

    // Version
    tx.version = 0;
    for (int i = 0; i < 4; ++i) {
        tx.version = (tx.version << 8) | data[offset++];
    }

    // From public key
    if (offset + DILITHIUM3_PUBLICKEYBYTES > data.size()) {
        return std::nullopt;
    }
    std::copy(data.begin() + offset, data.begin() + offset + DILITHIUM3_PUBLICKEYBYTES, tx.from.begin());
    offset += DILITHIUM3_PUBLICKEYBYTES;

    // Nonce
    tx.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        tx.nonce = (tx.nonce << 8) | data[offset++];
    }

    // Value
    tx.value = 0;
    for (int i = 0; i < 8; ++i) {
        tx.value = (tx.value << 8) | data[offset++];
    }

    // Bytecode length
    uint32_t bytecode_len = 0;
    for (int i = 0; i < 4; ++i) {
        bytecode_len = (bytecode_len << 8) | data[offset++];
    }

    // Bytecode
    if (offset + bytecode_len > data.size()) {
        return std::nullopt;
    }
    tx.bytecode.assign(data.begin() + offset, data.begin() + offset + bytecode_len);
    offset += bytecode_len;

    // Constructor args length
    uint32_t args_len = 0;
    for (int i = 0; i < 4; ++i) {
        args_len = (args_len << 8) | data[offset++];
    }

    // Constructor args
    if (offset + args_len > data.size()) {
        return std::nullopt;
    }
    tx.constructor_args.assign(data.begin() + offset, data.begin() + offset + args_len);
    offset += args_len;

    // Gas limit
    tx.gas_limit = 0;
    for (int i = 0; i < 8; ++i) {
        tx.gas_limit = (tx.gas_limit << 8) | data[offset++];
    }

    // Gas price
    tx.gas_price = 0;
    for (int i = 0; i < 8; ++i) {
        tx.gas_price = (tx.gas_price << 8) | data[offset++];
    }

    // Signature
    if (offset + DILITHIUM3_BYTES > data.size()) {
        return std::nullopt;
    }
    std::copy(data.begin() + offset, data.begin() + offset + DILITHIUM3_BYTES, tx.signature.begin());

    return tx;
}

uint256 ContractDeploymentTx::GetSigningHash() const {
    std::vector<uint8_t> data;

    // Type
    data.push_back(type);

    // From public key
    data.insert(data.end(), from.begin(), from.end());

    // Nonce
    for (int i = 7; i >= 0; --i) {
        data.push_back((nonce >> (i * 8)) & 0xff);
    }

    // Value
    for (int i = 7; i >= 0; --i) {
        data.push_back((value >> (i * 8)) & 0xff);
    }

    // Bytecode
    data.insert(data.end(), bytecode.begin(), bytecode.end());

    // Constructor args
    data.insert(data.end(), constructor_args.begin(), constructor_args.end());

    // Gas limit
    for (int i = 7; i >= 0; --i) {
        data.push_back((gas_limit >> (i * 8)) & 0xff);
    }

    // Gas price
    for (int i = 7; i >= 0; --i) {
        data.push_back((gas_price >> (i * 8)) & 0xff);
    }

    return SHA3::Hash(data.data(), data.size());
}

bool ContractDeploymentTx::Sign(const SecretKey& secret_key) {
    uint256 signing_hash = GetSigningHash();
    auto result = DilithiumCrypto::SignHash(signing_hash, secret_key);
    if (!result.IsOk()) {
        return false;
    }
    signature = result.GetValue();
    return true;
}

bool ContractDeploymentTx::Verify() const {
    uint256 signing_hash = GetSigningHash();
    auto result = DilithiumCrypto::VerifyHash(signing_hash, signature, from);
    return result.IsOk();
}

size_t ContractDeploymentTx::GetSize() const {
    return Serialize().size();
}

uint64_t ContractDeploymentTx::GetMinimumFee() const {
    // Base fee (21,000 gas) + bytecode cost (200 gas per byte)
    uint64_t base_fee = 21000;
    uint64_t bytecode_cost = bytecode.size() * 200;
    return base_fee + bytecode_cost;
}

//
// ContractCallTx Implementation
//

uint256 ContractCallTx::GetTxID() const {
    auto serialized = Serialize();
    return SHA3::Hash(serialized.data(), serialized.size());
}

std::vector<uint8_t> ContractCallTx::Serialize() const {
    std::vector<uint8_t> result;

    // Type (1 byte)
    result.push_back(type);

    // Version (4 bytes)
    for (int i = 3; i >= 0; --i) {
        result.push_back((version >> (i * 8)) & 0xff);
    }

    // From public key
    result.insert(result.end(), from.begin(), from.end());

    // Nonce (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((nonce >> (i * 8)) & 0xff);
    }

    // To address (Bech32 string - length prefixed)
    uint16_t to_len = to.size();
    result.push_back((to_len >> 8) & 0xff);
    result.push_back(to_len & 0xff);
    result.insert(result.end(), to.begin(), to.end());

    // Value (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((value >> (i * 8)) & 0xff);
    }

    // Data length (4 bytes)
    uint32_t data_len = data.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back((data_len >> (i * 8)) & 0xff);
    }

    // Data
    result.insert(result.end(), data.begin(), data.end());

    // Gas limit (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((gas_limit >> (i * 8)) & 0xff);
    }

    // Gas price (8 bytes)
    for (int i = 7; i >= 0; --i) {
        result.push_back((gas_price >> (i * 8)) & 0xff);
    }

    // Signature
    result.insert(result.end(), signature.begin(), signature.end());

    return result;
}

std::optional<ContractCallTx> ContractCallTx::Deserialize(const std::vector<uint8_t>& data_in) {
    if (data_in.size() < 13) {
        return std::nullopt;
    }

    ContractCallTx tx;
    size_t offset = 0;

    // Type
    tx.type = data_in[offset++];
    if (tx.type != 3) {
        return std::nullopt;
    }

    // Version
    tx.version = 0;
    for (int i = 0; i < 4; ++i) {
        tx.version = (tx.version << 8) | data_in[offset++];
    }

    // From public key
    if (offset + DILITHIUM3_PUBLICKEYBYTES > data_in.size()) {
        return std::nullopt;
    }
    std::copy(data_in.begin() + offset, data_in.begin() + offset + DILITHIUM3_PUBLICKEYBYTES, tx.from.begin());
    offset += DILITHIUM3_PUBLICKEYBYTES;

    // Nonce
    tx.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        tx.nonce = (tx.nonce << 8) | data_in[offset++];
    }

    // To address length
    uint16_t to_len = (data_in[offset] << 8) | data_in[offset + 1];
    offset += 2;

    // To address
    if (offset + to_len > data_in.size()) {
        return std::nullopt;
    }
    tx.to.assign(data_in.begin() + offset, data_in.begin() + offset + to_len);
    offset += to_len;

    // Value
    tx.value = 0;
    for (int i = 0; i < 8; ++i) {
        tx.value = (tx.value << 8) | data_in[offset++];
    }

    // Data length
    uint32_t data_len = 0;
    for (int i = 0; i < 4; ++i) {
        data_len = (data_len << 8) | data_in[offset++];
    }

    // Data
    if (offset + data_len > data_in.size()) {
        return std::nullopt;
    }
    tx.data.assign(data_in.begin() + offset, data_in.begin() + offset + data_len);
    offset += data_len;

    // Gas limit
    tx.gas_limit = 0;
    for (int i = 0; i < 8; ++i) {
        tx.gas_limit = (tx.gas_limit << 8) | data_in[offset++];
    }

    // Gas price
    tx.gas_price = 0;
    for (int i = 0; i < 8; ++i) {
        tx.gas_price = (tx.gas_price << 8) | data_in[offset++];
    }

    // Signature
    if (offset + DILITHIUM3_BYTES > data_in.size()) {
        return std::nullopt;
    }
    std::copy(data_in.begin() + offset, data_in.begin() + offset + DILITHIUM3_BYTES, tx.signature.begin());

    return tx;
}

uint256 ContractCallTx::GetSigningHash() const {
    std::vector<uint8_t> data_out;

    // Type
    data_out.push_back(type);

    // From public key
    data_out.insert(data_out.end(), from.begin(), from.end());

    // Nonce
    for (int i = 7; i >= 0; --i) {
        data_out.push_back((nonce >> (i * 8)) & 0xff);
    }

    // To address
    data_out.insert(data_out.end(), to.begin(), to.end());

    // Value
    for (int i = 7; i >= 0; --i) {
        data_out.push_back((value >> (i * 8)) & 0xff);
    }

    // Data
    data_out.insert(data_out.end(), data.begin(), data.end());

    // Gas limit
    for (int i = 7; i >= 0; --i) {
        data_out.push_back((gas_limit >> (i * 8)) & 0xff);
    }

    // Gas price
    for (int i = 7; i >= 0; --i) {
        data_out.push_back((gas_price >> (i * 8)) & 0xff);
    }

    return SHA3::Hash(data_out.data(), data_out.size());
}

bool ContractCallTx::Sign(const SecretKey& secret_key) {
    uint256 signing_hash = GetSigningHash();
    auto result = DilithiumCrypto::SignHash(signing_hash, secret_key);
    if (!result.IsOk()) {
        return false;
    }
    signature = result.GetValue();
    return true;
}

bool ContractCallTx::Verify() const {
    uint256 signing_hash = GetSigningHash();
    auto result = DilithiumCrypto::VerifyHash(signing_hash, signature, from);
    return result.IsOk();
}

size_t ContractCallTx::GetSize() const {
    return Serialize().size();
}

uint64_t ContractCallTx::GetMinimumFee() const {
    // Base fee (21,000 gas) + data cost (68 gas per non-zero byte, 4 gas per zero byte)
    uint64_t base_fee = 21000;
    uint64_t data_cost = 0;
    for (uint8_t byte : data) {
        data_cost += (byte == 0) ? 4 : 68;
    }
    return base_fee + data_cost;
}

//
// ContractReceipt Implementation
//

std::string ContractReceipt::GetStatusString() const {
    switch (status) {
        case ExecutionResult::SUCCESS: return "SUCCESS";
        case ExecutionResult::OUT_OF_GAS: return "OUT_OF_GAS";
        case ExecutionResult::STACK_OVERFLOW: return "STACK_OVERFLOW";
        case ExecutionResult::STACK_UNDERFLOW: return "STACK_UNDERFLOW";
        case ExecutionResult::INVALID_OPCODE: return "INVALID_OPCODE";
        case ExecutionResult::INVALID_JUMP: return "INVALID_JUMP";
        case ExecutionResult::REVERT: return "REVERT";
        case ExecutionResult::CONTRACT_CREATION_FAILED: return "CONTRACT_CREATION_FAILED";
        case ExecutionResult::CALL_FAILED: return "CALL_FAILED";
        case ExecutionResult::INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: return "UNKNOWN";
    }
}

std::vector<uint8_t> ContractReceipt::Serialize() const {
    // Simplified serialization
    std::vector<uint8_t> result;
    // TODO: Implement full serialization
    return result;
}

std::optional<ContractReceipt> ContractReceipt::Deserialize(const std::vector<uint8_t>& data) {
    // TODO: Implement deserialization
    return std::nullopt;
}

//
// ContractABI Implementation
//

std::array<uint8_t, 4> ContractABI::GetFunctionSelector(const std::string& function_signature) {
    // Selector = first 4 bytes of SHA3-256(function_signature)
    uint256 hash = SHA3::Hash(
        reinterpret_cast<const uint8_t*>(function_signature.data()),
        function_signature.size()
    );

    std::array<uint8_t, 4> selector;
    std::copy(hash.begin(), hash.begin() + 4, selector.begin());
    return selector;
}

std::vector<uint8_t> ContractABI::EncodeFunction(
    const std::string& function_signature,
    const std::vector<Word256>& args
) {
    std::vector<uint8_t> result;

    // Function selector (4 bytes)
    auto selector = GetFunctionSelector(function_signature);
    result.insert(result.end(), selector.begin(), selector.end());

    // Encode arguments
    for (const auto& arg : args) {
        result.insert(result.end(), arg.begin(), arg.end());
    }

    return result;
}

std::vector<Word256> ContractABI::DecodeReturn(const std::vector<uint8_t>& return_data) {
    std::vector<Word256> result;

    // Each Word256 is 32 bytes
    for (size_t i = 0; i + 32 <= return_data.size(); i += 32) {
        Word256 word;
        std::copy(return_data.begin() + i, return_data.begin() + i + 32, word.begin());
        result.push_back(word);
    }

    return result;
}

std::vector<uint8_t> ContractABI::EncodeConstructor(const std::vector<Word256>& args) {
    std::vector<uint8_t> result;

    // Encode arguments (no selector for constructor)
    for (const auto& arg : args) {
        result.insert(result.end(), arg.begin(), arg.end());
    }

    return result;
}

std::vector<uint8_t> ContractABI::EncodeValue(const Word256& value) {
    return std::vector<uint8_t>(value.begin(), value.end());
}

std::vector<uint8_t> ContractABI::EncodeAddress(const std::string& address) {
    // Decode Bech32 address to bytes and left-pad to 32 bytes
    std::vector<uint8_t> result(32, 0);
    // TODO: Decode Bech32 and fill result
    return result;
}

std::vector<uint8_t> ContractABI::EncodeUint256(uint64_t value) {
    Word256 word = Uint64ToWord256(value);
    return std::vector<uint8_t>(word.begin(), word.end());
}

std::vector<uint8_t> ContractABI::EncodeBytes(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;

    // Length (32 bytes, padded)
    Word256 length = Uint64ToWord256(data.size());
    result.insert(result.end(), length.begin(), length.end());

    // Data (padded to 32-byte boundary)
    result.insert(result.end(), data.begin(), data.end());

    // Pad to 32-byte boundary
    size_t padding = (32 - (data.size() % 32)) % 32;
    result.resize(result.size() + padding, 0);

    return result;
}

std::vector<uint8_t> ContractABI::EncodeString(const std::string& str) {
    return EncodeBytes(std::vector<uint8_t>(str.begin(), str.end()));
}

std::string ContractABI::DecodeAddress(const std::vector<uint8_t>& data) {
    // TODO: Decode address bytes and convert to Bech32
    return "int1q...";
}

uint64_t ContractABI::DecodeUint256(const std::vector<uint8_t>& data) {
    if (data.size() < 32) {
        return 0;
    }
    Word256 word;
    std::copy(data.begin(), data.begin() + 32, word.begin());
    return Word256ToUint64(word);
}

std::vector<uint8_t> ContractABI::DecodeBytes(const std::vector<uint8_t>& data) {
    if (data.size() < 32) {
        return {};
    }

    // Length
    uint64_t length = DecodeUint256(data);

    // Data
    if (32 + length > data.size()) {
        return {};
    }

    return std::vector<uint8_t>(data.begin() + 32, data.begin() + 32 + length);
}

std::string ContractABI::DecodeString(const std::vector<uint8_t>& data) {
    auto bytes = DecodeBytes(data);
    return std::string(bytes.begin(), bytes.end());
}

//
// ContractDeployer Implementation
//

ContractDeploymentTx ContractDeployer::Deploy(
    const std::vector<uint8_t>& bytecode,
    const std::vector<Word256>& constructor_args,
    uint64_t value,
    uint64_t gas_limit,
    const PublicKey& from,
    const SecretKey& secret_key
) {
    ContractDeploymentTx tx;
    tx.from = from;
    tx.value = value;
    tx.bytecode = bytecode;
    tx.constructor_args = ContractABI::EncodeConstructor(constructor_args);
    tx.gas_limit = gas_limit;
    tx.gas_price = 1;  // TODO: Get from network
    tx.nonce = 0;  // TODO: Get from account state
    tx.Sign(secret_key);
    return tx;
}

uint64_t ContractDeployer::EstimateGas(
    const std::vector<uint8_t>& bytecode,
    const std::vector<Word256>& constructor_args
) {
    // Base cost + bytecode cost
    uint64_t base = 32000;
    uint64_t bytecode_cost = bytecode.size() * 200;
    uint64_t constructor_cost = constructor_args.size() * 32 * 68;  // Approximate
    return base + bytecode_cost + constructor_cost;
}

//
// ContractCaller Implementation
//

ContractCallTx ContractCaller::Call(
    const std::string& contract_address,
    const std::string& function_signature,
    const std::vector<Word256>& args,
    uint64_t value,
    uint64_t gas_limit,
    const PublicKey& from,
    const SecretKey& secret_key
) {
    ContractCallTx tx;
    tx.from = from;
    tx.to = contract_address;
    tx.value = value;
    tx.data = ContractABI::EncodeFunction(function_signature, args);
    tx.gas_limit = gas_limit;
    tx.gas_price = 1;  // TODO: Get from network
    tx.nonce = 0;  // TODO: Get from account state
    tx.Sign(secret_key);
    return tx;
}

uint64_t ContractCaller::EstimateGas(
    const std::string& contract_address,
    const std::string& function_signature,
    const std::vector<Word256>& args,
    uint64_t value
) {
    (void)contract_address;
    (void)function_signature;
    (void)value;

    // Base cost + data cost
    uint64_t base = 21000;
    uint64_t data_cost = args.size() * 32 * 68;  // Approximate
    return base + data_cost;
}

std::vector<uint8_t> ContractCaller::StaticCall(
    const std::string& contract_address,
    const std::string& function_signature,
    const std::vector<Word256>& args
) {
    (void)contract_address;
    (void)function_signature;
    (void)args;

    // TODO: Implement static call execution
    return {};
}

} // namespace contracts
} // namespace intcoin
