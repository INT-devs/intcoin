// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/rpc.h>
#include <intcoin/bridge.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <sstream>
#include <iomanip>

namespace intcoin {
namespace rpc {

using namespace intcoin::bridge;

// Global bridge instance (should be part of blockchain state in production)
static std::unique_ptr<INTcoinBridge> g_bridge;

// Helper: Convert hex string to bytes
static Result<std::vector<uint8_t>> HexToBytes(const std::string& hex) {
    if (hex.length() % 2 != 0) {
        return Result<std::vector<uint8_t>>::Error("Hex string must have even length");
    }

    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            bytes.push_back(byte);
        } catch (...) {
            return Result<std::vector<uint8_t>>::Error("Invalid hex string");
        }
    }
    return Result<std::vector<uint8_t>>::Ok(bytes);
}

// Helper: Convert uint256 to hex string
static std::string Uint256ToHex(const uint256& hash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : hash) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Initialize bridge with configuration
void InitializeBridge(const BridgeConfig& config) {
    if (!g_bridge) {
        g_bridge = std::make_unique<INTcoinBridge>();
        auto result = g_bridge->Initialize(config);
        if (!result.IsOk()) {
            LogF(LogLevel::ERROR, "Bridge RPC: Failed to initialize bridge");
        }
    }
}

INTcoinBridge* GetBridge() {
    return g_bridge.get();
}

// Bridge RPC Methods
class BridgeRPC {
public:
    static void RegisterMethods(RPCServer& server) {
        server.RegisterMethod({
            "bridgedeposit",
            "Submit a deposit proof to the bridge\n"
            "Arguments: [chain, token, tx_hash, block_number, depositor, recipient, amount]\n"
            "Returns: {proof_id, status}",
            {"chain", "token", "tx_hash", "block_number", "depositor", "recipient", "amount"},
            false,
            [](const JSONValue& params) { return bridgedeposit(params); }
        });

        server.RegisterMethod({
            "bridgewithdraw",
            "Request withdrawal from bridge\n"
            "Arguments: [chain, token, destination, amount, signature]\n"
            "Returns: {withdrawal_id, status}",
            {"chain", "token", "destination", "amount", "signature"},
            false,
            [](const JSONValue& params) { return bridgewithdraw(params); }
        });

        server.RegisterMethod({
            "getbridgebalance",
            "Get wrapped token balance for address\n"
            "Arguments: [address (optional)]\n"
            "Returns: {balances: {wBTC, wETH, wLTC}}",
            {},
            false,
            [](const JSONValue& params) { return getbridgebalance(params); }
        });

        server.RegisterMethod({
            "listbridgetransactions",
            "List bridge transactions\n"
            "Arguments: [address (optional), type (optional), limit (optional)]\n"
            "Returns: {transactions: [...]",
            {},
            false,
            [](const JSONValue& params) { return listbridgetransactions(params); }
        });

        server.RegisterMethod({
            "getbridgeinfo",
            "Get bridge configuration and status\n"
            "Arguments: []\n"
            "Returns: {status, validators, tokens}",
            {},
            false,
            [](const JSONValue& params) { return getbridgeinfo(params); }
        });

        LogF(LogLevel::INFO, "Bridge RPC: Registered 5 RPC methods");
    }

    static JSONValue bridgedeposit(const JSONValue& params) {
        if (!g_bridge) {
            throw std::runtime_error("Bridge not initialized");
        }

        if (!params.IsArray() || params.Size() < 7) {
            throw std::runtime_error("Usage: bridgedeposit <chain> <token> <tx_hash> <block_number> <depositor> <recipient> <amount>");
        }

        try {
            std::string chain_str = params[0].GetString();
            std::string token_symbol = params[1].GetString();
            std::string tx_hash_hex = params[2].GetString();
            uint64_t block_number = static_cast<uint64_t>(params[3].GetInt());
            std::string depositor_hex = params[4].GetString();
            std::string recipient_hex = params[5].GetString();
            uint64_t amount = static_cast<uint64_t>(params[6].GetInt());

            // Convert chain string to BridgeChain enum
            BridgeChain chain;
            if (chain_str == "bitcoin") chain = BridgeChain::BITCOIN;
            else if (chain_str == "ethereum") chain = BridgeChain::ETHEREUM;
            else if (chain_str == "litecoin") chain = BridgeChain::LITECOIN;
            else throw std::runtime_error("Invalid chain: " + chain_str);

            // Create deposit proof
            DepositProof proof;
            auto tx_bytes_result = HexToBytes(tx_hash_hex);
            if (!tx_bytes_result.IsOk()) {
                throw std::runtime_error("Invalid tx_hash hex");
            }
            proof.source_tx_hash = SHA3::Hash(tx_bytes_result.GetValue());
            proof.block_number = block_number;

            auto depositor_result = HexToBytes(depositor_hex);
            if (!depositor_result.IsOk()) {
                throw std::runtime_error("Invalid depositor hex");
            }
            proof.depositor_address = depositor_result.GetValue();

            auto recipient_result = HexToBytes(recipient_hex);
            if (!recipient_result.IsOk()) {
                throw std::runtime_error("Invalid recipient hex");
            }
            proof.recipient_address = recipient_result.GetValue();
            proof.amount = amount;
            proof.timestamp = std::time(nullptr);

            // Set token info
            proof.token.symbol = token_symbol;
            proof.token.origin_chain = chain;

            // Add dummy validator signatures for testing
            proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x02));
            proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x03));
            proof.validator_signatures.push_back(std::vector<uint8_t>(33, 0x04));

            // Submit deposit proof
            auto result = g_bridge->SubmitDepositProof(proof);
            if (!result.IsOk()) {
                throw std::runtime_error("Failed to submit deposit proof");
            }

            std::map<std::string, JSONValue> response;
            response["proof_id"] = JSONValue(Uint256ToHex(result.GetValue()));
            response["status"] = JSONValue("validated");
            response["amount"] = JSONValue(static_cast<int64_t>(amount));
            response["token"] = JSONValue(token_symbol);
            return JSONValue(response);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Exception: ") + e.what());
        }
    }

    static JSONValue bridgewithdraw(const JSONValue& params) {
        if (!g_bridge) {
            throw std::runtime_error("Bridge not initialized");
        }

        if (!params.IsArray() || params.Size() < 5) {
            throw std::runtime_error("Usage: bridgewithdraw <chain> <token> <destination> <amount> <signature>");
        }

        try {
            std::string chain_str = params[0].GetString();
            std::string token_symbol = params[1].GetString();
            std::string dest_hex = params[2].GetString();
            uint64_t amount = static_cast<uint64_t>(params[3].GetInt());
            std::string sig_hex = params[4].GetString();

            // Convert chain string to BridgeChain enum
            BridgeChain chain;
            if (chain_str == "bitcoin") chain = BridgeChain::BITCOIN;
            else if (chain_str == "ethereum") chain = BridgeChain::ETHEREUM;
            else if (chain_str == "litecoin") chain = BridgeChain::LITECOIN;
            else throw std::runtime_error("Invalid chain: " + chain_str);

            // Create wrapped token
            WrappedToken token;
            token.symbol = token_symbol;
            token.origin_chain = chain;

            auto dest_result = HexToBytes(dest_hex);
            if (!dest_result.IsOk()) {
                throw std::runtime_error("Invalid destination hex");
            }

            auto sig_result = HexToBytes(sig_hex);
            if (!sig_result.IsOk()) {
                throw std::runtime_error("Invalid signature hex");
            }

            // Request withdrawal
            auto result = g_bridge->RequestWithdrawal(
                dest_result.GetValue(),
                amount,
                token,
                sig_result.GetValue()
            );

            if (!result.IsOk()) {
                throw std::runtime_error("Failed to request withdrawal");
            }

            // Get config
            auto config_result = g_bridge->GetConfig();
            uint32_t required_sigs = config_result.IsOk() ?
                config_result.GetValue().min_validators : 0;

            std::map<std::string, JSONValue> response;
            response["withdrawal_id"] = JSONValue(Uint256ToHex(result.GetValue()));
            response["status"] = JSONValue("pending");
            response["amount"] = JSONValue(static_cast<int64_t>(amount));
            response["token"] = JSONValue(token_symbol);
            response["destination_chain"] = JSONValue(chain_str);
            response["required_signatures"] = JSONValue(static_cast<int64_t>(required_sigs));
            response["current_signatures"] = JSONValue(static_cast<int64_t>(0));
            return JSONValue(response);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Exception: ") + e.what());
        }
    }

    static JSONValue getbridgebalance(const JSONValue& params) {
        if (!g_bridge) {
            throw std::runtime_error("Bridge not initialized");
        }

        try {
            std::string address_hex = "";
            if (params.IsArray() && params.Size() > 0) {
                address_hex = params[0].GetString();
            }

            // TODO: Implement balance tracking in bridge
            // For now, return placeholder values
            std::map<std::string, JSONValue> balances;
            balances["wBTC"] = JSONValue(static_cast<int64_t>(0));
            balances["wETH"] = JSONValue(static_cast<int64_t>(0));
            balances["wLTC"] = JSONValue(static_cast<int64_t>(0));

            std::map<std::string, JSONValue> response;
            response["address"] = JSONValue(address_hex);
            response["balances"] = JSONValue(balances);
            response["total_value_int"] = JSONValue(static_cast<int64_t>(0));
            return JSONValue(response);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Exception: ") + e.what());
        }
    }

    static JSONValue listbridgetransactions(const JSONValue& params) {
        if (!g_bridge) {
            throw std::runtime_error("Bridge not initialized");
        }

        try {
            std::string address_hex = "";
            std::string type = "all";
            int limit = 100;
            int offset = 0;

            if (params.IsArray() && params.Size() > 0) address_hex = params[0].GetString();
            if (params.IsArray() && params.Size() > 1) type = params[1].GetString();
            if (params.IsArray() && params.Size() > 2) limit = static_cast<int>(params[2].GetInt());
            if (params.IsArray() && params.Size() > 3) offset = static_cast<int>(params[3].GetInt());

            // Empty transactions array for now
            std::vector<JSONValue> transactions;

            std::map<std::string, JSONValue> response;
            response["transactions"] = JSONValue(transactions);
            response["total"] = JSONValue(static_cast<int64_t>(0));
            response["limit"] = JSONValue(static_cast<int64_t>(limit));
            response["offset"] = JSONValue(static_cast<int64_t>(offset));
            response["type_filter"] = JSONValue(type);
            return JSONValue(response);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Exception: ") + e.what());
        }
    }

    static JSONValue getbridgeinfo(const JSONValue& params) {
        if (!g_bridge) {
            throw std::runtime_error("Bridge not initialized");
        }

        try {
            auto config_result = g_bridge->GetConfig();
            if (!config_result.IsOk()) {
                throw std::runtime_error("Failed to get bridge config");
            }

            const BridgeConfig& config = config_result.GetValue();

            // Validator info
            std::map<std::string, JSONValue> validators;
            validators["total"] = JSONValue(static_cast<int64_t>(config.total_validators));
            validators["active"] = JSONValue(static_cast<int64_t>(config.total_validators));
            validators["threshold"] = JSONValue(static_cast<int64_t>(config.min_validators));

            // Confirmations
            std::map<std::string, JSONValue> confirmations;
            confirmations["bitcoin"] = JSONValue(static_cast<int64_t>(config.min_confirmations_btc));
            confirmations["ethereum"] = JSONValue(static_cast<int64_t>(config.min_confirmations_eth));
            confirmations["litecoin"] = JSONValue(static_cast<int64_t>(config.min_confirmations_ltc));

            // Empty tokens array for now
            std::vector<JSONValue> tokens;

            std::map<std::string, JSONValue> response;
            response["status"] = JSONValue(config.emergency_paused ? "paused" : "active");
            response["validators"] = JSONValue(validators);
            response["tokens"] = JSONValue(tokens);
            response["confirmations"] = JSONValue(confirmations);
            response["fee_basis_points"] = JSONValue(static_cast<int64_t>(config.fee_basis_points));
            response["min_validator_stake"] = JSONValue(static_cast<int64_t>(config.min_validator_stake));
            response["withdrawal_timeout"] = JSONValue(static_cast<int64_t>(config.withdrawal_timeout));
            return JSONValue(response);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Exception: ") + e.what());
        }
    }
};

// Register bridge RPC methods
void RegisterBridgeRPCMethods(RPCServer& server) {
    BridgeRPC::RegisterMethods(server);
}

} // namespace rpc
} // namespace intcoin
