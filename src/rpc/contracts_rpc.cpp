// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/rpc.h"
#include "intcoin/rpc/contracts_rpc.h"
#include "intcoin/contracts/vm.h"
#include "intcoin/contracts/storage.h"
#include "intcoin/contracts/transaction.h"
#include "intcoin/crypto.h"
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace rpc {

//
// Helper functions
//

static std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string hex_str = hex;

    // Remove 0x prefix if present
    if (hex_str.length() >= 2 && hex_str.substr(0, 2) == "0x") {
        hex_str = hex_str.substr(2);
    }

    for (size_t i = 0; i < hex_str.length(); i += 2) {
        if (i + 1 >= hex_str.length()) break;
        std::string byte_str = hex_str.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
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

[[maybe_unused]] static std::vector<std::string> JSONArrayToStringVector(const JSONValue& arr) {
    std::vector<std::string> result;
    if (arr.IsArray()) {
        for (size_t i = 0; i < arr.Size(); i++) {
            if (arr[i].IsString()) {
                result.push_back(arr[i].GetString());
            }
        }
    }
    return result;
}

//
// ContractsRPC Implementation
//

void ContractsRPC::RegisterMethods(RPCServer& server) {
    server.RegisterMethod({
        "deploycontract",
        "Deploy a smart contract\n"
        "Parameters: bytecode, constructor_args, gas_limit, [value]",
        {"bytecode", "constructor_args", "gas_limit", "value"},
        true,
        [](const JSONValue& params) { return deploycontract(params); }
    });

    server.RegisterMethod({
        "callcontract",
        "Call a smart contract function\n"
        "Parameters: contract_address, function_signature, args, gas_limit, [value]",
        {"contract_address", "function_signature", "args", "gas_limit", "value"},
        true,
        [](const JSONValue& params) { return callcontract(params); }
    });

    server.RegisterMethod({
        "callcontractstatic",
        "Perform a static call (read-only, no transaction)\n"
        "Parameters: contract_address, function_signature, args",
        {"contract_address", "function_signature", "args"},
        false,
        [](const JSONValue& params) { return callcontractstatic(params); }
    });

    server.RegisterMethod({
        "getcontractinfo",
        "Get contract information\n"
        "Parameters: contract_address",
        {"contract_address"},
        false,
        [](const JSONValue& params) { return getcontractinfo(params); }
    });

    server.RegisterMethod({
        "getcontractcode",
        "Get contract bytecode and disassembly\n"
        "Parameters: contract_address",
        {"contract_address"},
        false,
        [](const JSONValue& params) { return getcontractcode(params); }
    });

    server.RegisterMethod({
        "getcontractreceipt",
        "Get transaction receipt\n"
        "Parameters: txid",
        {"txid"},
        false,
        [](const JSONValue& params) { return getcontractreceipt(params); }
    });

    server.RegisterMethod({
        "estimatecontractgas",
        "Estimate gas for contract call\n"
        "Parameters: contract_address, function_signature, args, [value]",
        {"contract_address", "function_signature", "args", "value"},
        false,
        [](const JSONValue& params) { return estimatecontractgas(params); }
    });

    server.RegisterMethod({
        "getcontractlogs",
        "Get contract event logs\n"
        "Parameters: [contract_address], [from_block], [to_block]",
        {"contract_address", "from_block", "to_block"},
        false,
        [](const JSONValue& params) { return getcontractlogs(params); }
    });

    server.RegisterMethod({
        "listcontracts",
        "List deployed contracts\n"
        "Parameters: [limit], [offset]",
        {"limit", "offset"},
        false,
        [](const JSONValue& params) { return listcontracts(params); }
    });

    server.RegisterMethod({
        "getcontractstorage",
        "Get contract storage value\n"
        "Parameters: contract_address, storage_key",
        {"contract_address", "storage_key"},
        false,
        [](const JSONValue& params) { return getcontractstorage(params); }
    });

    server.RegisterMethod({
        "compilesolidity",
        "Compile Solidity source code\n"
        "Parameters: source_code, [compiler_version]",
        {"source_code", "compiler_version"},
        false,
        [](const JSONValue& params) { return compilesolidity(params); }
    });

    server.RegisterMethod({
        "verifycontract",
        "Verify contract source code\n"
        "Parameters: contract_address, source_code, constructor_args, compiler_version",
        {"contract_address", "source_code", "constructor_args", "compiler_version"},
        false,
        [](const JSONValue& params) { return verifycontract(params); }
    });
}

JSONValue ContractsRPC::deploycontract(const JSONValue& params) {
    try {
        // Validate parameters
        if (!params.IsArray() || params.Size() < 3) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: deploycontract <bytecode> <constructor_args> <gas_limit> [value]");
            return error;
        }

        // Parse parameters
        std::string bytecode_hex = params[0].GetString();
        // TODO: Parse constructor args from params[1]
        uint64_t gas_limit = static_cast<uint64_t>(params[2].GetInt());
        uint64_t value = (params.Size() > 3) ? static_cast<uint64_t>(params[3].GetInt()) : 0;

        // Convert bytecode
        std::vector<uint8_t> bytecode = HexToBytes(bytecode_hex);

        // TODO: Encode constructor arguments
        std::vector<intcoin::contracts::Word256> constructor_words;

        // Create deployment transaction
        // NOTE: In production, this would get the user's key from wallet
        PublicKey from{};  // Placeholder
        SecretKey secret_key{};  // Placeholder

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
        JSONValue response;
        response.type = JSONType::Object;
        response["txid"] = JSONValue("0x0000000000000000000000000000000000000000000000000000000000000000");  // TODO: Compute actual txid
        response["contract_address"] = JSONValue(contract_address);
        response["gas_estimated"] = JSONValue(static_cast<int64_t>(gas_estimate));
        response["gas_limit"] = JSONValue(static_cast<int64_t>(gas_limit));
        response["value"] = JSONValue(static_cast<int64_t>(value));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to deploy contract: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::callcontract(const JSONValue& params) {
    try {
        // Validate parameters
        if (!params.IsArray() || params.Size() < 4) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: callcontract <address> <function_sig> <args> <gas_limit> [value]");
            return error;
        }

        // Parse parameters
        std::string contract_address = params[0].GetString();
        std::string function_signature = params[1].GetString();
        // TODO: Parse args from params[2]
        uint64_t gas_limit = static_cast<uint64_t>(params[3].GetInt());
        uint64_t value = (params.Size() > 4) ? static_cast<uint64_t>(params[4].GetInt()) : 0;

        // Convert arguments to Word256
        std::vector<intcoin::contracts::Word256> args;
        // TODO: Decode args

        // Create call transaction
        PublicKey from{};  // Placeholder
        SecretKey secret_key{};  // Placeholder

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
        JSONValue response;
        response.type = JSONType::Object;
        response["txid"] = JSONValue("0x0000000000000000000000000000000000000000000000000000000000000000");  // TODO: Compute actual txid
        response["gas_estimated"] = JSONValue(static_cast<int64_t>(gas_estimate));
        response["gas_limit"] = JSONValue(static_cast<int64_t>(gas_limit));
        response["contract_address"] = JSONValue(contract_address);
        response["function"] = JSONValue(function_signature);
        response["value"] = JSONValue(static_cast<int64_t>(value));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to call contract: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::callcontractstatic(const JSONValue& params) {
    try {
        // Validate parameters
        if (!params.IsArray() || params.Size() < 3) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: callcontractstatic <address> <function_sig> <args>");
            return error;
        }

        std::string contract_address = params[0].GetString();
        std::string function_signature = params[1].GetString();

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
        JSONValue response;
        response.type = JSONType::Object;
        response["result"] = JSONValue(BytesToHex(result));

        JSONValue decoded_arr;
        decoded_arr.type = JSONType::Array;
        response["decoded"] = decoded_arr;  // TODO: Decode result

        response["gas_used"] = JSONValue(static_cast<int64_t>(0));  // TODO: Calculate gas used

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Static call failed: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::getcontractinfo(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 1) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: getcontractinfo <address>");
            return error;
        }

        std::string contract_address = params[0].GetString();

        // TODO: Query contract state from database
        // This is a placeholder implementation

        JSONValue response;
        response.type = JSONType::Object;
        response["address"] = JSONValue(contract_address);
        response["balance"] = JSONValue(static_cast<int64_t>(0));
        response["code_hash"] = JSONValue("0x0000000000000000000000000000000000000000000000000000000000000000");
        response["storage_root"] = JSONValue("0x0000000000000000000000000000000000000000000000000000000000000000");
        response["nonce"] = JSONValue(static_cast<int64_t>(0));
        response["creator"] = JSONValue("");
        response["creation_tx"] = JSONValue("");
        response["block_created"] = JSONValue(static_cast<int64_t>(0));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to get contract info: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::getcontractcode(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 1) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: getcontractcode <address>");
            return error;
        }

        std::string contract_address = params[0].GetString();

        // TODO: Retrieve bytecode from storage
        std::vector<uint8_t> bytecode;

        // Disassemble bytecode
        std::string disassembly = intcoin::contracts::IntSCVM::Disassemble(bytecode);

        JSONValue response;
        response.type = JSONType::Object;
        response["code"] = JSONValue(BytesToHex(bytecode));
        response["size"] = JSONValue(static_cast<int64_t>(bytecode.size()));
        response["disassembly"] = JSONValue(disassembly);

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to get contract code: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::getcontractreceipt(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 1) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: getcontractreceipt <txid>");
            return error;
        }

        std::string txid = params[0].GetString();

        // TODO: Query receipt from database

        JSONValue logs_arr;
        logs_arr.type = JSONType::Array;

        JSONValue response;
        response.type = JSONType::Object;
        response["txid"] = JSONValue(txid);
        response["contract_address"] = JSONValue("");
        response["from"] = JSONValue("");
        response["to"] = JSONValue("");
        response["gas_used"] = JSONValue(static_cast<int64_t>(0));
        response["gas_price"] = JSONValue(static_cast<int64_t>(1));
        response["total_fee"] = JSONValue(static_cast<int64_t>(0));
        response["status"] = JSONValue("SUCCESS");
        response["return_data"] = JSONValue("0x");
        response["logs"] = logs_arr;
        response["block_number"] = JSONValue(static_cast<int64_t>(0));
        response["block_timestamp"] = JSONValue(static_cast<int64_t>(0));
        response["tx_index"] = JSONValue(static_cast<int64_t>(0));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to get receipt: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::estimatecontractgas(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 3) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: estimatecontractgas <address> <function_sig> <args> [value]");
            return error;
        }

        std::string contract_address = params[0].GetString();
        std::string function_signature = params[1].GetString();
        uint64_t value = (params.Size() > 3) ? static_cast<uint64_t>(params[3].GetInt()) : 0;

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

        uint64_t gas_price = 1;  // TODO: Get from network

        JSONValue response;
        response.type = JSONType::Object;
        response["gas_estimate"] = JSONValue(static_cast<int64_t>(gas_estimate));
        response["gas_price"] = JSONValue(static_cast<int64_t>(gas_price));
        response["estimated_fee"] = JSONValue(static_cast<int64_t>(gas_estimate * gas_price));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Gas estimation failed: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::getcontractlogs(const JSONValue& params) {
    try {
        std::string contract_address;
        uint64_t from_block = 0;
        uint64_t to_block = UINT64_MAX;

        if (params.IsArray()) {
            if (params.Size() > 0 && params[0].IsString()) contract_address = params[0].GetString();
            if (params.Size() > 1 && params[1].IsNumber()) from_block = static_cast<uint64_t>(params[1].GetInt());
            if (params.Size() > 2 && params[2].IsNumber()) to_block = static_cast<uint64_t>(params[2].GetInt());
        }

        // Mark as used until TODO is implemented
        (void)from_block;
        (void)to_block;

        // TODO: Query logs from database

        JSONValue logs_arr;
        logs_arr.type = JSONType::Array;

        JSONValue response;
        response.type = JSONType::Object;
        response["logs"] = logs_arr;

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to get logs: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::listcontracts(const JSONValue& params) {
    try {
        uint32_t limit = 100;
        uint32_t offset = 0;

        if (params.IsArray()) {
            if (params.Size() > 0 && params[0].IsNumber()) limit = static_cast<uint32_t>(params[0].GetInt());
            if (params.Size() > 1 && params[1].IsNumber()) offset = static_cast<uint32_t>(params[1].GetInt());
        }

        // TODO: Query contracts from database

        JSONValue contracts_arr;
        contracts_arr.type = JSONType::Array;

        JSONValue response;
        response.type = JSONType::Object;
        response["contracts"] = contracts_arr;
        response["total"] = JSONValue(static_cast<int64_t>(0));
        response["offset"] = JSONValue(static_cast<int64_t>(offset));
        response["limit"] = JSONValue(static_cast<int64_t>(limit));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to list contracts: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::getcontractstorage(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 2) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: getcontractstorage <address> <key>");
            return error;
        }

        std::string contract_address = params[0].GetString();
        std::string storage_key_hex = params[1].GetString();

        // Convert key
        intcoin::contracts::Word256 storage_key = intcoin::contracts::HexToWord256(storage_key_hex);

        // Mark as used until TODO is implemented
        (void)storage_key;

        // TODO: Query storage from database
        intcoin::contracts::Word256 value{};

        JSONValue response;
        response.type = JSONType::Object;
        response["key"] = JSONValue(storage_key_hex);
        response["value"] = JSONValue(intcoin::contracts::Word256ToHex(value));

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Failed to get storage: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::compilesolidity(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 1) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: compilesolidity <source_code> [compiler_version]");
            return error;
        }

        // TODO: Integrate with Solidity compiler (solc)
        // This would require spawning solc process or using libsolc

        JSONValue abi_arr, warnings_arr, errors_arr;
        abi_arr.type = JSONType::Array;
        warnings_arr.type = JSONType::Array;
        errors_arr.type = JSONType::Array;

        JSONValue response;
        response.type = JSONType::Object;
        response["bytecode"] = JSONValue("0x");
        response["abi"] = abi_arr;
        response["warnings"] = warnings_arr;
        response["errors"] = errors_arr;
        response["error"] = JSONValue("Solidity compilation not yet implemented. Please compile offline and use deploycontract with bytecode.");

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Compilation failed: ") + e.what());
        return error;
    }
}

JSONValue ContractsRPC::verifycontract(const JSONValue& params) {
    try {
        if (!params.IsArray() || params.Size() < 4) {
            JSONValue error;
            error.type = JSONType::Object;
            error["error"] = JSONValue("Invalid parameters. Usage: verifycontract <address> <source_code> <constructor_args> <compiler_version>");
            return error;
        }

        // TODO: Compile source, compare bytecode

        JSONValue response;
        response.type = JSONType::Object;
        response["verified"] = JSONValue(false);
        response["source_hash"] = JSONValue("0x0000000000000000000000000000000000000000000000000000000000000000");
        response["bytecode_match"] = JSONValue(false);
        response["error"] = JSONValue("Contract verification not yet implemented.");

        return response;

    } catch (const std::exception& e) {
        JSONValue error;
        error.type = JSONType::Object;
        error["error"] = JSONValue(std::string("Verification failed: ") + e.what());
        return error;
    }
}

} // namespace rpc
} // namespace intcoin
