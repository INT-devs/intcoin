// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_TRANSACTION_H
#define INTCOIN_CONTRACTS_TRANSACTION_H

#include "intcoin/types.h"
#include "intcoin/contracts/vm.h"
#include <vector>
#include <string>
#include <optional>

namespace intcoin {
namespace contracts {

/**
 * Contract transaction type
 */
enum class ContractTxType : uint8_t {
    CONTRACT_DEPLOYMENT = 2,     // Deploy new contract
    CONTRACT_CALL = 3            // Call existing contract
};

/**
 * Contract Deployment Transaction (Type 2)
 *
 * Creates a new smart contract on-chain
 */
struct ContractDeploymentTx {
    // Transaction header
    uint8_t type{2};                      // Transaction type (CONTRACT_DEPLOYMENT)
    uint32_t version{1};                  // Transaction version

    // Sender
    PublicKey from;                       // Deployer's public key
    uint64_t nonce{0};                    // Transaction counter

    // Contract deployment
    uint64_t value{0};                    // Initial contract balance (satINT)
    std::vector<uint8_t> bytecode;        // Contract bytecode
    std::vector<uint8_t> constructor_args; // Constructor arguments (ABI-encoded)

    // Gas parameters
    uint64_t gas_limit{0};                // Maximum gas to use
    uint64_t gas_price{0};                // Gas price in satINT

    // Signature
    Signature signature;                  // Dilithium3 signature

    /**
     * Calculate transaction ID
     */
    uint256 GetTxID() const;

    /**
     * Calculate contract address
     *
     * Address = Bech32(SHA3-256(from || nonce)[:20])
     */
    std::string GetContractAddress() const;

    /**
     * Serialize transaction
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize transaction
     */
    static std::optional<ContractDeploymentTx> Deserialize(const std::vector<uint8_t>& data);

    /**
     * Get signing hash
     *
     * Hash = SHA3-256(type || from || nonce || value || bytecode || constructor_args || gas_limit || gas_price)
     */
    uint256 GetSigningHash() const;

    /**
     * Sign transaction
     */
    bool Sign(const SecretKey& secret_key);

    /**
     * Verify signature
     */
    bool Verify() const;

    /**
     * Get transaction size
     */
    size_t GetSize() const;

    /**
     * Get minimum fee
     */
    uint64_t GetMinimumFee() const;
};

/**
 * Contract Call Transaction (Type 3)
 *
 * Executes a function on an existing smart contract
 */
struct ContractCallTx {
    // Transaction header
    uint8_t type{3};                      // Transaction type (CONTRACT_CALL)
    uint32_t version{1};                  // Transaction version

    // Sender
    PublicKey from;                       // Caller's public key
    uint64_t nonce{0};                    // Transaction counter

    // Contract call
    std::string to;                       // Contract address (Bech32)
    uint64_t value{0};                    // Amount to send (satINT)
    std::vector<uint8_t> data;            // Function call data (ABI-encoded)

    // Gas parameters
    uint64_t gas_limit{0};                // Maximum gas to use
    uint64_t gas_price{0};                // Gas price in satINT

    // Signature
    Signature signature;                  // Dilithium3 signature

    /**
     * Calculate transaction ID
     */
    uint256 GetTxID() const;

    /**
     * Serialize transaction
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize transaction
     */
    static std::optional<ContractCallTx> Deserialize(const std::vector<uint8_t>& data);

    /**
     * Get signing hash
     *
     * Hash = SHA3-256(type || from || nonce || to || value || data || gas_limit || gas_price)
     */
    uint256 GetSigningHash() const;

    /**
     * Sign transaction
     */
    bool Sign(const SecretKey& secret_key);

    /**
     * Verify signature
     */
    bool Verify() const;

    /**
     * Get transaction size
     */
    size_t GetSize() const;

    /**
     * Get minimum fee
     */
    uint64_t GetMinimumFee() const;
};

/**
 * Contract execution receipt
 */
struct ContractReceipt {
    std::string tx_hash;                  // Transaction hash
    std::string contract_address;         // Contract address (for deployment)
    std::string from;                     // Sender address
    std::string to;                       // Recipient address (empty for deployment)

    uint64_t gas_used{0};                 // Gas consumed
    uint64_t gas_price{0};                // Gas price used
    uint64_t total_fee{0};                // Total fee paid

    ExecutionResult status{ExecutionResult::SUCCESS};
    std::vector<uint8_t> return_data;     // Contract return data
    std::vector<IntSCVM::Log> logs;       // Event logs

    uint64_t block_number{0};             // Block number
    uint64_t block_timestamp{0};          // Block timestamp
    uint32_t tx_index{0};                 // Transaction index in block

    /**
     * Check if execution was successful
     */
    bool IsSuccess() const {
        return status == ExecutionResult::SUCCESS;
    }

    /**
     * Get status string
     */
    std::string GetStatusString() const;

    /**
     * Serialize receipt
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * Deserialize receipt
     */
    static std::optional<ContractReceipt> Deserialize(const std::vector<uint8_t>& data);
};

/**
 * Contract ABI encoder/decoder
 */
class ContractABI {
public:
    /**
     * Encode function call
     *
     * @param function_signature Function signature (e.g., "transfer(address,uint256)")
     * @param args Function arguments
     * @return ABI-encoded call data
     */
    static std::vector<uint8_t> EncodeFunction(
        const std::string& function_signature,
        const std::vector<Word256>& args
    );

    /**
     * Decode function return data
     *
     * @param return_data Contract return data
     * @return Decoded values
     */
    static std::vector<Word256> DecodeReturn(const std::vector<uint8_t>& return_data);

    /**
     * Get function selector
     *
     * Selector = first 4 bytes of SHA3-256(function_signature)
     *
     * @param function_signature Function signature
     * @return 4-byte selector
     */
    static std::array<uint8_t, 4> GetFunctionSelector(const std::string& function_signature);

    /**
     * Encode constructor arguments
     *
     * @param args Constructor arguments
     * @return ABI-encoded constructor data
     */
    static std::vector<uint8_t> EncodeConstructor(const std::vector<Word256>& args);

    /**
     * Encode single value
     *
     * @param value Value to encode
     * @return ABI-encoded value (32 bytes)
     */
    static std::vector<uint8_t> EncodeValue(const Word256& value);

    /**
     * Encode address
     *
     * @param address Bech32 address
     * @return ABI-encoded address (32 bytes, left-padded)
     */
    static std::vector<uint8_t> EncodeAddress(const std::string& address);

    /**
     * Encode uint256
     *
     * @param value Uint256 value
     * @return ABI-encoded value
     */
    static std::vector<uint8_t> EncodeUint256(uint64_t value);

    /**
     * Encode bytes
     *
     * @param data Byte array
     * @return ABI-encoded bytes (dynamic)
     */
    static std::vector<uint8_t> EncodeBytes(const std::vector<uint8_t>& data);

    /**
     * Encode string
     *
     * @param str String value
     * @return ABI-encoded string
     */
    static std::vector<uint8_t> EncodeString(const std::string& str);

    /**
     * Decode address
     *
     * @param data ABI-encoded address
     * @return Bech32 address
     */
    static std::string DecodeAddress(const std::vector<uint8_t>& data);

    /**
     * Decode uint256
     *
     * @param data ABI-encoded uint256
     * @return Uint256 value
     */
    static uint64_t DecodeUint256(const std::vector<uint8_t>& data);

    /**
     * Decode bytes
     *
     * @param data ABI-encoded bytes
     * @return Decoded byte array
     */
    static std::vector<uint8_t> DecodeBytes(const std::vector<uint8_t>& data);

    /**
     * Decode string
     *
     * @param data ABI-encoded string
     * @return Decoded string
     */
    static std::string DecodeString(const std::vector<uint8_t>& data);
};

/**
 * Contract deployment helper
 */
class ContractDeployer {
public:
    /**
     * Deploy contract
     *
     * @param bytecode Contract bytecode
     * @param constructor_args Constructor arguments
     * @param value Initial balance
     * @param gas_limit Gas limit
     * @param from Deployer public key
     * @param secret_key Deployer secret key
     * @return Deployment transaction
     */
    static ContractDeploymentTx Deploy(
        const std::vector<uint8_t>& bytecode,
        const std::vector<Word256>& constructor_args,
        uint64_t value,
        uint64_t gas_limit,
        const PublicKey& from,
        const SecretKey& secret_key
    );

    /**
     * Estimate deployment gas
     *
     * @param bytecode Contract bytecode
     * @param constructor_args Constructor arguments
     * @return Estimated gas
     */
    static uint64_t EstimateGas(
        const std::vector<uint8_t>& bytecode,
        const std::vector<Word256>& constructor_args
    );
};

/**
 * Contract caller helper
 */
class ContractCaller {
public:
    /**
     * Call contract function
     *
     * @param contract_address Contract address
     * @param function_signature Function signature
     * @param args Function arguments
     * @param value Amount to send
     * @param gas_limit Gas limit
     * @param from Caller public key
     * @param secret_key Caller secret key
     * @return Call transaction
     */
    static ContractCallTx Call(
        const std::string& contract_address,
        const std::string& function_signature,
        const std::vector<Word256>& args,
        uint64_t value,
        uint64_t gas_limit,
        const PublicKey& from,
        const SecretKey& secret_key
    );

    /**
     * Estimate call gas
     *
     * @param contract_address Contract address
     * @param function_signature Function signature
     * @param args Function arguments
     * @param value Amount to send
     * @return Estimated gas
     */
    static uint64_t EstimateGas(
        const std::string& contract_address,
        const std::string& function_signature,
        const std::vector<Word256>& args,
        uint64_t value
    );

    /**
     * Static call (read-only, no state changes)
     *
     * @param contract_address Contract address
     * @param function_signature Function signature
     * @param args Function arguments
     * @return Return data
     */
    static std::vector<uint8_t> StaticCall(
        const std::string& contract_address,
        const std::string& function_signature,
        const std::vector<Word256>& args
    );
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_TRANSACTION_H
