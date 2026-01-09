// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/rpc/contracts_rpc.h"
#include "intcoin/contracts/vm.h"
#include "intcoin/contracts/storage.h"
#include "intcoin/contracts/transaction.h"
#include "intcoin/crypto.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace intcoin {
namespace rpc {
namespace contracts {

//
// Helper functions
//

static std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string hex_str = hex;

    // Remove 0x prefix if present
    if (hex_str.substr(0, 2) == "0x") {
        hex_str = hex_str.substr(2);
    }

    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        uint8_t byte = std::stoi(byte_str, nullptr, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

static std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << "0x";
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

//
// RPC Method Implementations
//

std::string DeployContract(const std::vector<std::string>& params) {
    // Parameter validation
    if (params.size() < 3) {
        json error;
        error["error"] = "Invalid parameters. Usage: deploycontract <bytecode> <constructor_args> <gas_limit> [value]";
        return error.dump();
    }

    try {
        // Parse parameters
        std::string bytecode_hex = params[0];
        std::vector<std::string> constructor_args_hex;
        if (params.size() > 1 && params[1] != "[]") {
            // TODO: Parse array of constructor args
        }
        uint64_t gas_limit = std::stoull(params[2]);
        uint64_t value = (params.size() > 3) ? std::stoull(params[3]) : 0;

        // Convert bytecode
        std::vector<uint8_t> bytecode = HexToBytes(bytecode_hex);

        // Encode constructor arguments
        std::vector<intcoin::contracts::Word256> constructor_words;
        // TODO: Decode constructor args from hex

        // Create deployment transaction
        // NOTE: In production, this would get the user's key from wallet
        PublicKey from{}; // Placeholder
        SecretKey secret_key{}; // Placeholder

        auto deploy_tx = intcoin::contracts::ContractDeployer::Deploy(
            bytecode,
            constructor_words,
            value,
            gas_limit,
            from,
            secret_key
        );

        // Calculate contract address
        std::string contract_address = deploy_tx.GetContractAddress();

        // Estimate gas
        uint64_t gas_estimate = intcoin::contracts::ContractDeployer::EstimateGas(
            bytecode,
            constructor_words
        );

        // Build response
        json response;
        response["txid"] = intcoin::Word256ToHex(deploy_tx.GetTxID());
        response["contract_address"] = contract_address;
        response["gas_estimated"] = gas_estimate;
        response["gas_limit"] = gas_limit;
        response["value"] = value;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to deploy contract: ") + e.what();
        return error.dump();
    }
}

std::string CallContract(const std::vector<std::string>& params) {
    // Parameter validation
    if (params.size() < 4) {
        json error;
        error["error"] = "Invalid parameters. Usage: callcontract <address> <function_sig> <args> <gas_limit> [value]";
        return error.dump();
    }

    try {
        // Parse parameters
        std::string contract_address = params[0];
        std::string function_signature = params[1];
        std::vector<std::string> args_hex;
        // TODO: Parse args array from params[2]
        uint64_t gas_limit = std::stoull(params[3]);
        uint64_t value = (params.size() > 4) ? std::stoull(params[4]) : 0;

        // Convert arguments to Word256
        std::vector<intcoin::contracts::Word256> args;
        // TODO: Decode args from hex

        // Create call transaction
        PublicKey from{}; // Placeholder
        SecretKey secret_key{}; // Placeholder

        auto call_tx = intcoin::contracts::ContractCaller::Call(
            contract_address,
            function_signature,
            args,
            value,
            gas_limit,
            from,
            secret_key
        );

        // Estimate gas
        uint64_t gas_estimate = intcoin::contracts::ContractCaller::EstimateGas(
            contract_address,
            function_signature,
            args,
            value
        );

        // Build response
        json response;
        response["txid"] = intcoin::Word256ToHex(call_tx.GetTxID());
        response["gas_estimated"] = gas_estimate;
        response["gas_limit"] = gas_limit;
        response["contract_address"] = contract_address;
        response["function"] = function_signature;
        response["value"] = value;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to call contract: ") + e.what();
        return error.dump();
    }
}

std::string CallContractStatic(const std::vector<std::string>& params) {
    // Parameter validation
    if (params.size() < 3) {
        json error;
        error["error"] = "Invalid parameters. Usage: callcontractstatic <address> <function_sig> <args>";
        return error.dump();
    }

    try {
        std::string contract_address = params[0];
        std::string function_signature = params[1];

        // Convert arguments
        std::vector<intcoin::contracts::Word256> args;
        // TODO: Parse args from params[2]

        // Perform static call
        auto result = intcoin::contracts::ContractCaller::StaticCall(
            contract_address,
            function_signature,
            args
        );

        // Build response
        json response;
        response["result"] = BytesToHex(result);
        response["decoded"] = json::array(); // TODO: Decode result
        response["gas_used"] = 0; // TODO: Calculate gas used

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Static call failed: ") + e.what();
        return error.dump();
    }
}

std::string GetContractInfo(const std::vector<std::string>& params) {
    if (params.empty()) {
        json error;
        error["error"] = "Invalid parameters. Usage: getcontractinfo <address>";
        return error.dump();
    }

    try {
        std::string contract_address = params[0];

        // TODO: Query contract state from database
        // This is a placeholder implementation

        json response;
        response["address"] = contract_address;
        response["balance"] = 0;
        response["code_hash"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        response["storage_root"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        response["nonce"] = 0;
        response["creator"] = "";
        response["creation_tx"] = "";
        response["block_created"] = 0;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to get contract info: ") + e.what();
        return error.dump();
    }
}

std::string GetContractCode(const std::vector<std::string>& params) {
    if (params.empty()) {
        json error;
        error["error"] = "Invalid parameters. Usage: getcontractcode <address>";
        return error.dump();
    }

    try {
        std::string contract_address = params[0];

        // TODO: Retrieve bytecode from storage
        std::vector<uint8_t> bytecode;

        // Disassemble bytecode
        std::string disassembly = intcoin::contracts::IntSCVM::Disassemble(bytecode);

        json response;
        response["code"] = BytesToHex(bytecode);
        response["size"] = bytecode.size();
        response["disassembly"] = disassembly;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to get contract code: ") + e.what();
        return error.dump();
    }
}

std::string GetContractReceipt(const std::vector<std::string>& params) {
    if (params.empty()) {
        json error;
        error["error"] = "Invalid parameters. Usage: getcontractreceipt <txid>";
        return error.dump();
    }

    try {
        std::string txid = params[0];

        // TODO: Query receipt from database

        json response;
        response["txid"] = txid;
        response["contract_address"] = "";
        response["from"] = "";
        response["to"] = "";
        response["gas_used"] = 0;
        response["gas_price"] = 1;
        response["total_fee"] = 0;
        response["status"] = "SUCCESS";
        response["return_data"] = "0x";
        response["logs"] = json::array();
        response["block_number"] = 0;
        response["block_timestamp"] = 0;
        response["tx_index"] = 0;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to get receipt: ") + e.what();
        return error.dump();
    }
}

std::string EstimateContractGas(const std::vector<std::string>& params) {
    if (params.size() < 3) {
        json error;
        error["error"] = "Invalid parameters. Usage: estimatecontractgas <address> <function_sig> <args> [value]";
        return error.dump();
    }

    try {
        std::string contract_address = params[0];
        std::string function_signature = params[1];
        uint64_t value = (params.size() > 3) ? std::stoull(params[3]) : 0;

        // Convert arguments
        std::vector<intcoin::contracts::Word256> args;
        // TODO: Parse args

        // Estimate gas
        uint64_t gas_estimate = intcoin::contracts::ContractCaller::EstimateGas(
            contract_address,
            function_signature,
            args,
            value
        );

        uint64_t gas_price = 1; // TODO: Get from network

        json response;
        response["gas_estimate"] = gas_estimate;
        response["gas_price"] = gas_price;
        response["estimated_fee"] = gas_estimate * gas_price;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Gas estimation failed: ") + e.what();
        return error.dump();
    }
}

std::string GetContractLogs(const std::vector<std::string>& params) {
    try {
        std::string contract_address;
        uint64_t from_block = 0;
        uint64_t to_block = UINT64_MAX;

        if (params.size() > 0) contract_address = params[0];
        if (params.size() > 1) from_block = std::stoull(params[1]);
        if (params.size() > 2) to_block = std::stoull(params[2]);

        // TODO: Query logs from database

        json response;
        response["logs"] = json::array();

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to get logs: ") + e.what();
        return error.dump();
    }
}

std::string ListContracts(const std::vector<std::string>& params) {
    try {
        uint32_t limit = 100;
        uint32_t offset = 0;

        if (params.size() > 0) limit = std::stoul(params[0]);
        if (params.size() > 1) offset = std::stoul(params[1]);

        // TODO: Query contracts from database

        json response;
        response["contracts"] = json::array();
        response["total"] = 0;
        response["offset"] = offset;
        response["limit"] = limit;

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to list contracts: ") + e.what();
        return error.dump();
    }
}

std::string GetContractStorage(const std::vector<std::string>& params) {
    if (params.size() < 2) {
        json error;
        error["error"] = "Invalid parameters. Usage: getcontractstorage <address> <key>";
        return error.dump();
    }

    try {
        std::string contract_address = params[0];
        std::string storage_key_hex = params[1];

        // Convert key
        intcoin::contracts::Word256 storage_key = intcoin::contracts::HexToWord256(storage_key_hex);

        // TODO: Query storage from database
        intcoin::contracts::Word256 value{};

        json response;
        response["key"] = storage_key_hex;
        response["value"] = intcoin::contracts::Word256ToHex(value);

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Failed to get storage: ") + e.what();
        return error.dump();
    }
}

std::string CompileSolidity(const std::vector<std::string>& params) {
    if (params.empty()) {
        json error;
        error["error"] = "Invalid parameters. Usage: compilesolidity <source_code> [compiler_version]";
        return error.dump();
    }

    try {
        // TODO: Integrate with Solidity compiler (solc)
        // This would require spawning solc process or using libsolc

        json response;
        response["bytecode"] = "0x";
        response["abi"] = json::array();
        response["warnings"] = json::array();
        response["errors"] = json::array();
        response["error"] = "Solidity compilation not yet implemented. Please compile offline and use deploycontract with bytecode.";

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Compilation failed: ") + e.what();
        return error.dump();
    }
}

std::string VerifyContract(const std::vector<std::string>& params) {
    if (params.size() < 4) {
        json error;
        error["error"] = "Invalid parameters. Usage: verifycontract <address> <source_code> <constructor_args> <compiler_version>";
        return error.dump();
    }

    try {
        // TODO: Compile source, compare bytecode

        json response;
        response["verified"] = false;
        response["source_hash"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        response["bytecode_match"] = false;
        response["error"] = "Contract verification not yet implemented.";

        return response.dump();

    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Verification failed: ") + e.what();
        return error.dump();
    }
}

} // namespace contracts
} // namespace rpc
} // namespace intcoin
