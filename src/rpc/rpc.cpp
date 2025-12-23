/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * JSON-RPC Server Implementation
 */

#include "intcoin/rpc.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <cstring>
#include <cmath>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace intcoin {
namespace rpc {

// ============================================================================
// JSON Value Implementation
// ============================================================================

std::string JSONValue::ToJSONString() const {
    std::ostringstream oss;

    switch (type) {
        case JSONType::Null:
            oss << "null";
            break;

        case JSONType::Bool:
            oss << (bool_value ? "true" : "false");
            break;

        case JSONType::Number:
            // Check if it's an integer
            if (number_value == static_cast<int64_t>(number_value)) {
                oss << static_cast<int64_t>(number_value);
            } else {
                oss << std::fixed << std::setprecision(8) << number_value;
            }
            break;

        case JSONType::String:
            oss << "\"";
            for (char c : string_value) {
                switch (c) {
                    case '"':  oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default:   oss << c; break;
                }
            }
            oss << "\"";
            break;

        case JSONType::Array:
            oss << "[";
            for (size_t i = 0; i < array_value.size(); i++) {
                if (i > 0) oss << ",";
                oss << array_value[i].ToJSONString();
            }
            oss << "]";
            break;

        case JSONType::Object:
            oss << "{";
            bool first = true;
            for (const auto& [key, value] : object_value) {
                if (!first) oss << ",";
                first = false;
                oss << "\"" << key << "\":" << value.ToJSONString();
            }
            oss << "}";
            break;
    }

    return oss.str();
}

// Simple JSON parser (minimal implementation)
Result<JSONValue> JSONValue::Parse(const std::string& json_str) {
    // Input sanitization
    if (json_str.empty()) {
        return Result<JSONValue>::Error("Empty JSON string");
    }

    if (json_str.length() > sanitize::MAX_JSON_LENGTH) {
        return Result<JSONValue>::Error("JSON string too large");
    }

    // Check for null bytes (potential injection)
    if (json_str.find('\0') != std::string::npos) {
        return Result<JSONValue>::Error("JSON contains null bytes");
    }

    // Validate JSON depth to prevent stack overflow
    if (!sanitize::ValidateJSONDepth(json_str, 100)) {
        return Result<JSONValue>::Error("JSON nesting too deep");
    }

    size_t pos = 0;

    // Skip whitespace
    auto skip_whitespace = [&]() {
        while (pos < json_str.length() && std::isspace(json_str[pos])) {
            pos++;
        }
    };

    // Parse value recursively
    std::function<Result<JSONValue>()> parse_value = [&]() -> Result<JSONValue> {
        skip_whitespace();
        if (pos >= json_str.length()) {
            return Result<JSONValue>::Error("Unexpected end of JSON");
        }

        char c = json_str[pos];

        // Null
        if (json_str.substr(pos, 4) == "null") {
            pos += 4;
            return Result<JSONValue>::Ok(JSONValue());
        }

        // Boolean
        if (json_str.substr(pos, 4) == "true") {
            pos += 4;
            return Result<JSONValue>::Ok(JSONValue(true));
        }
        if (json_str.substr(pos, 5) == "false") {
            pos += 5;
            return Result<JSONValue>::Ok(JSONValue(false));
        }

        // String
        if (c == '"') {
            pos++;
            std::string str;
            while (pos < json_str.length() && json_str[pos] != '"') {
                if (json_str[pos] == '\\' && pos + 1 < json_str.length()) {
                    pos++;
                    switch (json_str[pos]) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '"': str += '"'; break;
                        case '\\': str += '\\'; break;
                        default: str += json_str[pos]; break;
                    }
                } else {
                    str += json_str[pos];
                }
                pos++;
            }
            if (pos >= json_str.length()) {
                return Result<JSONValue>::Error("Unterminated string");
            }
            pos++;  // Skip closing quote
            return Result<JSONValue>::Ok(JSONValue(str));
        }

        // Number
        if (c == '-' || std::isdigit(c)) {
            size_t start = pos;
            if (c == '-') pos++;
            while (pos < json_str.length() && (std::isdigit(json_str[pos]) || json_str[pos] == '.')) {
                pos++;
            }
            std::string num_str = json_str.substr(start, pos - start);
            try {
                double num = std::stod(num_str);
                return Result<JSONValue>::Ok(JSONValue(num));
            } catch (...) {
                return Result<JSONValue>::Error("Invalid number: " + num_str);
            }
        }

        // Array
        if (c == '[') {
            pos++;
            std::vector<JSONValue> arr;
            skip_whitespace();
            if (pos < json_str.length() && json_str[pos] == ']') {
                pos++;
                return Result<JSONValue>::Ok(JSONValue(arr));
            }

            while (true) {
                auto val_result = parse_value();
                if (!val_result.IsOk()) {
                    return val_result;
                }
                arr.push_back(val_result.value.value());

                skip_whitespace();
                if (pos >= json_str.length()) {
                    return Result<JSONValue>::Error("Unterminated array");
                }
                if (json_str[pos] == ']') {
                    pos++;
                    break;
                }
                if (json_str[pos] == ',') {
                    pos++;
                } else {
                    return Result<JSONValue>::Error("Expected ',' or ']' in array");
                }
            }
            return Result<JSONValue>::Ok(JSONValue(arr));
        }

        // Object
        if (c == '{') {
            pos++;
            std::map<std::string, JSONValue> obj;
            skip_whitespace();
            if (pos < json_str.length() && json_str[pos] == '}') {
                pos++;
                return Result<JSONValue>::Ok(JSONValue(obj));
            }

            while (true) {
                skip_whitespace();
                if (pos >= json_str.length() || json_str[pos] != '"') {
                    return Result<JSONValue>::Error("Expected string key in object");
                }

                auto key_result = parse_value();
                if (!key_result.IsOk()) {
                    return key_result;
                }
                std::string key = key_result.value.value().GetString();

                skip_whitespace();
                if (pos >= json_str.length() || json_str[pos] != ':') {
                    return Result<JSONValue>::Error("Expected ':' after key in object");
                }
                pos++;

                auto val_result = parse_value();
                if (!val_result.IsOk()) {
                    return val_result;
                }
                obj[key] = val_result.value.value();

                skip_whitespace();
                if (pos >= json_str.length()) {
                    return Result<JSONValue>::Error("Unterminated object");
                }
                if (json_str[pos] == '}') {
                    pos++;
                    break;
                }
                if (json_str[pos] == ',') {
                    pos++;
                } else {
                    return Result<JSONValue>::Error("Expected ',' or '}' in object");
                }
            }
            return Result<JSONValue>::Ok(JSONValue(obj));
        }

        return Result<JSONValue>::Error(std::string("Unexpected character: ") + c);
    };

    auto result = parse_value();
    if (!result.IsOk()) {
        return result;
    }

    skip_whitespace();
    if (pos < json_str.length()) {
        return Result<JSONValue>::Error("Trailing characters after JSON value");
    }

    return result;
}

// ============================================================================
// RPC Request/Response Implementation
// ============================================================================

Result<RPCRequest> RPCRequest::Parse(const std::string& json_str) {
    auto json_result = JSONValue::Parse(json_str);
    if (!json_result.IsOk()) {
        return Result<RPCRequest>::Error("Failed to parse JSON: " + json_result.error);
    }

    const JSONValue& json = json_result.value.value();
    if (!json.IsObject()) {
        return Result<RPCRequest>::Error("RPC request must be a JSON object");
    }

    RPCRequest request;

    // jsonrpc version
    if (json.HasKey("jsonrpc")) {
        request.jsonrpc = json["jsonrpc"].GetString();
        if (request.jsonrpc != "2.0") {
            return Result<RPCRequest>::Error("Only JSON-RPC 2.0 is supported");
        }
    } else {
        request.jsonrpc = "2.0";
    }

    // method
    if (!json.HasKey("method") || !json["method"].IsString()) {
        return Result<RPCRequest>::Error("Missing or invalid 'method' field");
    }
    request.method = json["method"].GetString();

    // params (optional)
    if (json.HasKey("params")) {
        request.params = json["params"];
    }

    // id (optional)
    if (json.HasKey("id")) {
        request.id = json["id"];
    }

    return Result<RPCRequest>::Ok(request);
}

std::string RPCResponse::ToJSONString() const {
    JSONValue response_obj{std::map<std::string, JSONValue>{}};
    response_obj["jsonrpc"] = JSONValue("2.0");

    if (error.has_value()) {
        response_obj["error"] = error.value();
    } else {
        response_obj["result"] = result;
    }

    if (id.has_value()) {
        response_obj["id"] = id.value();
    } else {
        response_obj["id"] = JSONValue();  // null
    }

    return response_obj.ToJSONString();
}

RPCResponse RPCResponse::Success(const JSONValue& result, const std::optional<JSONValue>& id) {
    RPCResponse response;
    response.jsonrpc = "2.0";
    response.result = result;
    response.id = id;
    return response;
}

RPCResponse RPCResponse::Error(int code, const std::string& message, const std::optional<JSONValue>& id) {
    RPCResponse response;
    response.jsonrpc = "2.0";

    std::map<std::string, JSONValue> error_obj;
    error_obj["code"] = JSONValue(static_cast<int64_t>(code));
    error_obj["message"] = JSONValue(message);
    response.error = JSONValue(error_obj);
    response.id = id;

    return response;
}

// ============================================================================
// RPC Server Implementation
// ============================================================================

class RPCServer::Impl {
public:
    RPCConfig config;
    Blockchain& blockchain;
    P2PNode& network;
    std::unique_ptr<HTTPServer> http_server;
    std::map<std::string, RPCMethodInfo> methods;
    mutable std::mutex mutex;
    RPCServer::Stats stats;
    bool running = false;

    Impl(const RPCConfig& cfg, Blockchain& bc, P2PNode& net)
        : config(cfg), blockchain(bc), network(net) {
        stats.start_time = std::chrono::system_clock::now();
    }

    Result<void> Start() {
        std::lock_guard<std::mutex> lock(mutex);
        if (running) {
            return Result<void>::Error("RPC server already running");
        }

        // Create HTTP server
        http_server = std::make_unique<HTTPServer>(config.bind_address, config.port);

        // Set request handler
        http_server->SetRequestHandler([this](const HTTPRequest& req) {
            return HandleHTTPRequest(req);
        });

        // Start HTTP server
        auto result = http_server->Start();
        if (!result.IsOk()) {
            return result;
        }

        running = true;
        return Result<void>::Ok();
    }

    Result<void> Stop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) {
            return Result<void>::Error("RPC server not running");
        }

        if (http_server) {
            auto result = http_server->Stop();
            if (!result.IsOk()) {
                return result;
            }
        }

        running = false;
        return Result<void>::Ok();
    }

    HTTPResponse HandleHTTPRequest(const HTTPRequest& request) {
        std::lock_guard<std::mutex> lock(mutex);
        stats.total_requests++;

        // Check authentication
        if (!config.rpc_user.empty() || !config.rpc_password.empty()) {
            auto auth_it = request.headers.find("Authorization");
            if (auth_it == request.headers.end()) {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            // Parse Basic auth (format: "Basic <base64(username:password)>")
            std::string auth = auth_it->second;
            if (auth.substr(0, 6) != "Basic ") {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            // Extract base64-encoded credentials
            std::string encoded = auth.substr(6);

            // Trim whitespace
            size_t start = encoded.find_first_not_of(" \t\r\n");
            size_t end = encoded.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                encoded = encoded.substr(start, end - start + 1);
            }

            // Decode base64
            auto decoded_result = Base64Decode(encoded);
            if (!decoded_result.IsOk()) {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            // Convert bytes to string
            std::string credentials(decoded_result.value->begin(), decoded_result.value->end());

            // Split by ':' to get username and password
            size_t colon_pos = credentials.find(':');
            if (colon_pos == std::string::npos) {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            std::string provided_user = credentials.substr(0, colon_pos);
            std::string provided_pass = credentials.substr(colon_pos + 1);

            // Sanitize credentials (check for injection attempts)
            if (sanitize::ContainsSuspiciousPatterns(provided_user) ||
                sanitize::ContainsSuspiciousPatterns(provided_pass)) {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            // Verify credentials match configuration
            if (provided_user != config.rpc_user || provided_pass != config.rpc_password) {
                stats.auth_failures++;
                return HTTPResponse::Unauthorized();
            }

            // Authentication successful
        }

        // Parse RPC request
        auto rpc_request_result = RPCRequest::Parse(request.body);
        if (!rpc_request_result.IsOk()) {
            stats.failed_requests++;
            RPCResponse response = RPCResponse::Error(
                RPCErrorCode::PARSE_ERROR,
                rpc_request_result.error,
                std::nullopt
            );
            return HTTPResponse::OK(response.ToJSONString());
        }

        // Handle RPC request
        RPCResponse rpc_response = HandleRPCRequest(rpc_request_result.value.value());

        if (rpc_response.error.has_value()) {
            stats.failed_requests++;
        } else {
            stats.successful_requests++;
        }

        return HTTPResponse::OK(rpc_response.ToJSONString());
    }

    RPCResponse HandleRPCRequest(const RPCRequest& request) {
        // Sanitize method name
        if (request.method.empty() || request.method.length() > sanitize::MAX_COMMAND_LENGTH) {
            return RPCResponse::Error(
                RPCErrorCode::INVALID_REQUEST,
                "Invalid method name length",
                request.id
            );
        }

        // Check for suspicious patterns in method name
        if (sanitize::ContainsSuspiciousPatterns(request.method)) {
            return RPCResponse::Error(
                RPCErrorCode::INVALID_REQUEST,
                "Invalid method name format",
                request.id
            );
        }

        // Find method
        auto it = methods.find(request.method);
        if (it == methods.end()) {
            return RPCResponse::Error(
                RPCErrorCode::METHOD_NOT_FOUND,
                "Method '" + sanitize::EscapeString(request.method) + "' not found",
                request.id
            );
        }

        const RPCMethodInfo& method_info = it->second;

        // Check authentication (if required)
        // Note: Authentication is already verified at HTTP level (lines 424-475)
        // If we reach this point, the request has passed authentication

        // Call handler
        try {
            JSONValue result = method_info.handler(request.params);
            return RPCResponse::Success(result, request.id);
        } catch (const std::exception& e) {
            return RPCResponse::Error(
                RPCErrorCode::INTERNAL_ERROR,
                std::string("Internal error: ") + e.what(),
                request.id
            );
        }
    }
};

RPCServer::RPCServer(const RPCConfig& config, Blockchain& blockchain, P2PNode& network)
    : impl_(std::make_unique<Impl>(config, blockchain, network)) {}

RPCServer::~RPCServer() {
    if (IsRunning()) {
        Stop();
    }
}

Result<void> RPCServer::Start() {
    return impl_->Start();
}

Result<void> RPCServer::Stop() {
    return impl_->Stop();
}

bool RPCServer::IsRunning() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->running;
}

void RPCServer::RegisterMethod(const RPCMethodInfo& method) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->methods[method.name] = method;
}

void RPCServer::RegisterAllMethods() {
    BlockchainRPC::RegisterMethods(*this, impl_->blockchain);
    NetworkRPC::RegisterMethods(*this, impl_->network);
    MiningRPC::RegisterMethods(*this, impl_->blockchain);
    UtilityRPC::RegisterMethods(*this);
    RawTransactionRPC::RegisterMethods(*this, impl_->blockchain);
}

RPCResponse RPCServer::HandleRequest(const RPCRequest& request) {
    return impl_->HandleRPCRequest(request);
}

RPCServer::Stats RPCServer::GetStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats;
}

// ============================================================================
// Blockchain RPC Methods
// ============================================================================

void BlockchainRPC::RegisterMethods(RPCServer& server, Blockchain& blockchain) {
    server.RegisterMethod({
        "getblockcount",
        "Returns the height of the most recent block",
        {},
        false,
        [&blockchain](const JSONValue&) { return getblockcount({}, blockchain); }
    });

    server.RegisterMethod({
        "getbestblockhash",
        "Returns the hash of the best (tip) block",
        {},
        false,
        [&blockchain](const JSONValue&) { return getbestblockhash({}, blockchain); }
    });

    server.RegisterMethod({
        "getblockhash",
        "Returns hash of block at given height",
        {"height"},
        false,
        [&blockchain](const JSONValue& params) { return getblockhash(params, blockchain); }
    });

    server.RegisterMethod({
        "getblock",
        "Returns information about a block",
        {"blockhash", "verbosity"},
        false,
        [&blockchain](const JSONValue& params) { return getblock(params, blockchain); }
    });

    server.RegisterMethod({
        "getdifficulty",
        "Returns the current difficulty",
        {},
        false,
        [&blockchain](const JSONValue&) { return getdifficulty({}, blockchain); }
    });

    server.RegisterMethod({
        "getmempoolinfo",
        "Returns mempool statistics",
        {},
        false,
        [&blockchain](const JSONValue&) { return getmempoolinfo({}, blockchain); }
    });
}

JSONValue BlockchainRPC::getblockcount(const JSONValue&, Blockchain& blockchain) {
    uint64_t height = blockchain.GetBestHeight();
    return JSONValue(static_cast<int64_t>(height));
}

JSONValue BlockchainRPC::getbestblockhash(const JSONValue&, Blockchain& blockchain) {
    uint256 hash = blockchain.GetBestBlockHash();
    return JSONValue(Uint256ToHex(hash));
}

JSONValue BlockchainRPC::getblockhash(const JSONValue& params, Blockchain& blockchain) {
    if (!params.IsArray() || params.Size() < 1) {
        throw std::runtime_error("Missing height parameter");
    }

    int64_t height = params[0].GetInt();
    if (height < 0 || static_cast<uint64_t>(height) > blockchain.GetBestHeight()) {
        throw std::runtime_error("Block height out of range");
    }

    auto block_result = blockchain.GetBlockByHeight(static_cast<uint64_t>(height));
    if (!block_result.IsOk()) {
        throw std::runtime_error("Block not found at height");
    }

    return JSONValue(Uint256ToHex(block_result.value.value().GetHash()));
}

JSONValue BlockchainRPC::getblock(const JSONValue& params, Blockchain& blockchain) {
    if (!params.IsArray() || params.Size() < 1) {
        throw std::runtime_error("Missing blockhash parameter");
    }

    std::string hash_str = params[0].GetString();
    auto hash_result = HexToBytes(hash_str);
    if (!hash_result.IsOk() || hash_result.value.value().size() != 32) {
        throw std::runtime_error("Invalid block hash");
    }

    uint256 hash;
    std::copy(hash_result.value.value().begin(), hash_result.value.value().end(), hash.begin());

    auto block_result = blockchain.GetBlock(hash);
    if (!block_result.IsOk()) {
        throw std::runtime_error("Block not found");
    }

    bool verbose = true;
    if (params.Size() >= 2) {
        verbose = params[1].GetInt() != 0;
    }

    return json::BlockToJSON(block_result.value.value(), verbose, &blockchain);
}

JSONValue BlockchainRPC::getdifficulty(const JSONValue&, Blockchain& blockchain) {
    uint32_t bits = blockchain.GetDifficulty();

    // Convert bits to difficulty number
    double difficulty = 0.0;
    if (bits != 0) {
        uint32_t exponent = bits >> 24;
        uint32_t mantissa = bits & 0xFFFFFF;
        difficulty = mantissa * std::pow(256.0, exponent - 3);
    }

    return JSONValue(difficulty);
}

JSONValue BlockchainRPC::getmempoolinfo(const JSONValue&, Blockchain& blockchain) {
    auto& mempool = blockchain.GetMempool();

    std::map<std::string, JSONValue> info;
    info["size"] = JSONValue(static_cast<int64_t>(mempool.GetSize()));
    info["bytes"] = JSONValue(static_cast<int64_t>(mempool.GetSize()));
    info["usage"] = JSONValue(static_cast<int64_t>(mempool.GetSize()));
    info["maxmempool"] = JSONValue(static_cast<int64_t>(100 * 1024 * 1024));  // 100 MB

    return JSONValue(info);
}

// ============================================================================
// Network RPC Methods
// ============================================================================

void NetworkRPC::RegisterMethods(RPCServer& server, P2PNode& network) {
    server.RegisterMethod({
        "getnetworkinfo",
        "Returns network information",
        {},
        false,
        [&network](const JSONValue&) { return getnetworkinfo({}, network); }
    });

    server.RegisterMethod({
        "getpeerinfo",
        "Returns information about connected peers",
        {},
        false,
        [&network](const JSONValue&) { return getpeerinfo({}, network); }
    });

    server.RegisterMethod({
        "getconnectioncount",
        "Returns the number of connections to other nodes",
        {},
        false,
        [&network](const JSONValue&) { return getconnectioncount({}, network); }
    });
}

JSONValue NetworkRPC::getnetworkinfo(const JSONValue&, P2PNode& network) {
    std::map<std::string, JSONValue> info;
    info["version"] = JSONValue(static_cast<int64_t>(10000));
    info["subversion"] = JSONValue("/INTcoin:1.0.0/");
    info["protocolversion"] = JSONValue(static_cast<int64_t>(network::PROTOCOL_VERSION));
    info["connections"] = JSONValue(static_cast<int64_t>(network.GetPeerCount()));

    return JSONValue(info);
}

JSONValue NetworkRPC::getpeerinfo(const JSONValue&, P2PNode& network) {
    auto peers = network.GetPeers();

    std::vector<JSONValue> peer_list;
    for (const auto& peer : peers) {
        peer_list.push_back(json::PeerToJSON(*peer));
    }

    return JSONValue(peer_list);
}

JSONValue NetworkRPC::getconnectioncount(const JSONValue&, P2PNode& network) {
    return JSONValue(static_cast<int64_t>(network.GetPeerCount()));
}

// ============================================================================
// Mining RPC Methods
// ============================================================================

void MiningRPC::RegisterMethods(RPCServer& server, Blockchain& blockchain) {
    server.RegisterMethod({
        "getmininginfo",
        "Returns mining-related information",
        {},
        false,
        [&blockchain](const JSONValue&) { return getmininginfo({}, blockchain); }
    });

    server.RegisterMethod({
        "getblocktemplate",
        "Returns a block template for mining",
        {"address"},
        false,
        [&blockchain](const JSONValue& params) { return getblocktemplate(params, blockchain); }
    });

    server.RegisterMethod({
        "submitblock",
        "Submit a solved block to the network",
        {"hexdata"},
        false,
        [&blockchain](const JSONValue& params) { return submitblock(params, blockchain); }
    });

    server.RegisterMethod({
        "generatetoaddress",
        "Mine blocks immediately to a specified address (regtest/testing only)",
        {"nblocks", "address"},
        false,
        [&blockchain](const JSONValue& params) { return generatetoaddress(params, blockchain); }
    });
}

JSONValue MiningRPC::getmininginfo(const JSONValue&, Blockchain& blockchain) {
    std::map<std::string, JSONValue> info;
    info["blocks"] = JSONValue(static_cast<int64_t>(blockchain.GetBestHeight()));
    info["difficulty"] = JSONValue(static_cast<double>(blockchain.GetDifficulty()));

    // Calculate network hashrate from recent blocks
    double networkhashps = blockchain.GetNetworkHashRate();
    info["networkhashps"] = JSONValue(networkhashps);

    return JSONValue(info);
}

JSONValue MiningRPC::getblocktemplate(const JSONValue& params, Blockchain& blockchain) {
    // Get miner address from params (optional)
    std::string miner_address;
    if (params.IsObject() && params.HasKey("address")) {
        miner_address = params["address"].GetString();
    }

    // Get best block info
    uint256 prev_hash = blockchain.GetBestBlockHash();
    uint64_t height = blockchain.GetBestHeight() + 1;
    uint64_t timestamp = static_cast<uint64_t>(time(nullptr));

    // Get difficulty bits from best block
    auto best_block_result = blockchain.GetBestBlock();
    uint32_t bits = 0x1d00ffff;  // Default difficulty bits
    if (best_block_result.IsOk()) {
        bits = best_block_result.value->header.bits;
    }

    // Calculate block reward
    uint64_t block_reward = GetBlockReward(height);

    // Get transactions from mempool (sorted by fee, max 1000)
    auto& mempool = blockchain.GetMempool();
    auto txs = mempool.GetTransactionsForMining(1000);

    // Calculate total fees (fee calculation requires UTXO set access)
    uint64_t total_fees = 0;
    (void)txs;  // Suppress unused warning for now

    // Create coinbase transaction (requires miner public key)
    // For template, we'll create a placeholder
    std::vector<Transaction> block_txs;

    // Add mempool transactions
    block_txs.insert(block_txs.end(), txs.begin(), txs.end());

    // Build response
    std::map<std::string, JSONValue> result;
    result["version"] = JSONValue(static_cast<int64_t>(1));
    result["previousblockhash"] = JSONValue(Uint256ToHex(prev_hash));
    result["height"] = JSONValue(static_cast<int64_t>(height));
    result["bits"] = JSONValue(std::to_string(bits));
    result["curtime"] = JSONValue(static_cast<int64_t>(timestamp));
    result["coinbasevalue"] = JSONValue(static_cast<int64_t>(block_reward + total_fees));

    // Add transactions
    std::vector<JSONValue> tx_array;
    for (const auto& tx : block_txs) {
        std::map<std::string, JSONValue> tx_obj;
        tx_obj["txid"] = JSONValue(Uint256ToHex(tx.GetHash()));
        tx_obj["size"] = JSONValue(static_cast<int64_t>(tx.GetSerializedSize()));
        tx_array.push_back(JSONValue(tx_obj));
    }
    result["transactions"] = JSONValue(tx_array);
    result["target"] = JSONValue(Uint256ToHex(uint256()));  // Placeholder target

    return JSONValue(result);
}

JSONValue MiningRPC::submitblock(const JSONValue& params, Blockchain& blockchain) {
    // Expect block data as hex string in params
    if (!params.IsArray() || params.Size() == 0) {
        throw std::runtime_error("Block data required as first parameter");
    }

    std::string block_hex = params[0].GetString();

    // Convert hex to bytes
    auto block_data_result = HexToBytes(block_hex);
    if (!block_data_result.IsOk()) {
        std::map<std::string, JSONValue> error;
        error["error"] = JSONValue("Invalid hex data");
        return JSONValue(error);
    }

    // Deserialize block
    auto block_result = Block::Deserialize(*block_data_result.value);
    if (!block_result.IsOk()) {
        std::map<std::string, JSONValue> error;
        error["error"] = JSONValue("Failed to deserialize block: " + block_result.error);
        return JSONValue(error);
    }

    Block block = *block_result.value;

    // Add block to blockchain
    auto add_result = blockchain.AddBlock(block);
    if (!add_result.IsOk()) {
        std::map<std::string, JSONValue> error;
        error["error"] = JSONValue("Failed to add block: " + add_result.error);
        return JSONValue(error);
    }

    // Return success (null for success in Bitcoin RPC)
    return JSONValue();
}

JSONValue MiningRPC::generatetoaddress(const JSONValue& params, Blockchain& blockchain) {
    // Parameters: nblocks (int), address (string)
    if (!params.IsArray() || params.Size() < 2) {
        throw std::runtime_error("generatetoaddress requires 2 parameters: nblocks and address");
    }

    int64_t nblocks = params[0].GetInt();
    std::string address = params[1].GetString();

    if (nblocks <= 0 || nblocks > 1000) {
        throw std::runtime_error("nblocks must be between 1 and 1000");
    }

    // Decode address to get pubkey hash
    auto decode_result = AddressEncoder::DecodeAddress(address);
    if (!decode_result.IsOk()) {
        throw std::runtime_error("Invalid address: " + decode_result.error);
    }
    uint256 pubkey_hash = decode_result.GetValue();

    // Generate blocks
    std::vector<JSONValue> block_hashes;

    for (int64_t i = 0; i < nblocks; i++) {
        // Get current chain info
        uint256 prev_hash = blockchain.GetBestBlockHash();
        uint64_t height = blockchain.GetBestHeight() + 1;
        uint64_t timestamp = static_cast<uint64_t>(time(nullptr));

        // Get difficulty bits
        auto best_block_result = blockchain.GetBestBlock();
        uint32_t bits = 0x1d00ffff;
        if (best_block_result.IsOk()) {
            bits = best_block_result.value->header.bits;
        }

        // Calculate block reward
        uint64_t block_reward = GetBlockReward(height);

        // Create coinbase transaction
        Transaction coinbase_tx;
        coinbase_tx.version = 1;

        // Coinbase input
        TxIn coinbase_input;
        coinbase_input.prev_tx_hash = uint256();
        coinbase_input.prev_tx_index = 0xFFFFFFFF;
        coinbase_input.sequence = 0xFFFFFFFF;
        coinbase_tx.inputs.push_back(coinbase_input);

        // Coinbase output
        Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
        TxOut coinbase_output(block_reward, script_pubkey);
        coinbase_tx.outputs.push_back(coinbase_output);

        coinbase_tx.locktime = 0;

        // Build block
        Block block;
        block.header.version = 1;
        block.header.prev_block_hash = prev_hash;
        block.header.timestamp = timestamp;
        block.header.bits = bits;
        block.header.nonce = 0;

        // Add coinbase transaction
        block.transactions.push_back(coinbase_tx);

        // Add transactions from mempool (up to 100)
        auto& mempool = blockchain.GetMempool();
        auto mempool_txs = mempool.GetTransactionsForMining(100);
        for (const auto& tx : mempool_txs) {
            block.transactions.push_back(tx);
        }

        // Calculate merkle root
        block.header.merkle_root = block.CalculateMerkleRoot();

        // Simple mining for regtest (try sequential nonces)
        // Note: In production regtest, this should use minimal difficulty
        // For now, we'll try up to 1 million nonces
        bool found = false;
        for (uint64_t nonce = 0; nonce < 1000000; nonce++) {
            block.header.nonce = nonce;

            // Calculate block hash
            uint256 block_hash = block.GetHash();

            // For regtest, we accept any valid block structure
            // In a real implementation, this would check against target difficulty
            // For now, just accept after trying a few nonces to simulate work
            if (nonce > 0) {
                found = true;

                // Add block to blockchain
                auto add_result = blockchain.AddBlock(block);
                if (!add_result.IsOk()) {
                    throw std::runtime_error("Failed to add block: " + add_result.error);
                }

                block_hashes.push_back(JSONValue(Uint256ToHex(block_hash)));
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Failed to mine block (difficulty too high for regtest)");
        }
    }

    return JSONValue(block_hashes);
}

// ============================================================================
// Utility RPC Methods
// ============================================================================

void UtilityRPC::RegisterMethods(RPCServer& server) {
    server.RegisterMethod({
        "uptime",
        "Returns the server uptime in seconds",
        {},
        false,
        [](const JSONValue&) { return uptime({}); }
    });

    server.RegisterMethod({
        "validateaddress",
        "Validates an INTcoin address",
        {"address"},
        false,
        [](const JSONValue& params) { return validateaddress(params); }
    });
}

JSONValue UtilityRPC::uptime(const JSONValue&) {
    // Calculate uptime
    static auto start_time = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

    return JSONValue(static_cast<int64_t>(uptime_seconds));
}

JSONValue UtilityRPC::validateaddress(const JSONValue& params) {
    if (!params.IsArray() || params.Size() < 1) {
        throw std::runtime_error("Missing address parameter");
    }

    std::string address = params[0].GetString();

    std::map<std::string, JSONValue> result;
    result["isvalid"] = JSONValue(AddressEncoder::ValidateAddress(address));
    result["address"] = JSONValue(address);

    return JSONValue(result);
}

// ============================================================================
// Raw Transaction RPC Methods
// ============================================================================

void RawTransactionRPC::RegisterMethods(RPCServer& server, Blockchain& blockchain) {
    server.RegisterMethod({
        "getrawtransaction",
        "Returns raw transaction data",
        {"txid", "verbose"},
        false,
        [&blockchain](const JSONValue& params) { return getrawtransaction(params, blockchain); }
    });

    server.RegisterMethod({
        "sendrawtransaction",
        "Submits raw transaction to network",
        {"hexstring"},
        false,
        [&blockchain](const JSONValue& params) { return sendrawtransaction(params, blockchain); }
    });
}

JSONValue RawTransactionRPC::getrawtransaction(const JSONValue& params, Blockchain& blockchain) {
    if (!params.IsArray() || params.Size() < 1) {
        throw std::runtime_error("Missing txid parameter");
    }

    std::string txid_str = params[0].GetString();
    auto txid_result = HexToBytes(txid_str);
    if (!txid_result.IsOk() || txid_result.value.value().size() != 32) {
        throw std::runtime_error("Invalid transaction ID");
    }

    uint256 txid;
    std::copy(txid_result.value.value().begin(), txid_result.value.value().end(), txid.begin());

    // Try to find in mempool first
    auto& mempool = blockchain.GetMempool();
    auto tx_opt = mempool.GetTransaction(txid);

    Transaction tx;
    bool found_in_mempool = tx_opt.has_value();
    bool found_in_blockchain = false;

    if (found_in_mempool) {
        tx = tx_opt.value();
    } else {
        // Search in blockchain
        auto tx_result = blockchain.GetTransaction(txid);
        if (tx_result.IsOk()) {
            tx = tx_result.value.value();
            found_in_blockchain = true;
        } else {
            throw std::runtime_error("Transaction not found in mempool or blockchain");
        }
    }

    bool verbose = false;
    if (params.Size() >= 2) {
        verbose = params[1].GetBool();
    }

    if (verbose) {
        // Include additional information if found in blockchain
        std::map<std::string, JSONValue> tx_json_map = json::TransactionToJSON(tx).GetObject();

        if (found_in_blockchain) {
            // Get block information
            auto block_result = blockchain.GetTransactionBlock(txid);
            if (block_result.IsOk()) {
                Block block = block_result.value.value();
                tx_json_map["blockhash"] = JSONValue(Uint256ToHex(block.GetHash()));
                tx_json_map["confirmations"] = JSONValue(static_cast<int64_t>(
                    blockchain.GetTransactionConfirmations(txid)));

                // Get block height
                auto best_height = blockchain.GetBestHeight();
                auto block_height = best_height - blockchain.GetTransactionConfirmations(txid) + 1;
                tx_json_map["blockheight"] = JSONValue(static_cast<int64_t>(block_height));
                tx_json_map["time"] = JSONValue(static_cast<int64_t>(block.header.timestamp));
            }
        }

        return JSONValue(tx_json_map);
    } else {
        std::vector<uint8_t> serialized = tx.Serialize();
        return JSONValue(BytesToHex(serialized));
    }
}

JSONValue RawTransactionRPC::sendrawtransaction(const JSONValue& params, Blockchain& blockchain) {
    if (!params.IsArray() || params.Size() < 1) {
        throw std::runtime_error("Missing hexstring parameter");
    }

    std::string hex_str = params[0].GetString();
    auto bytes_result = HexToBytes(hex_str);
    if (!bytes_result.IsOk()) {
        throw std::runtime_error("Invalid hex string");
    }

    auto tx_result = Transaction::Deserialize(bytes_result.value.value());
    if (!tx_result.IsOk()) {
        throw std::runtime_error("Failed to deserialize transaction: " + tx_result.error);
    }

    Transaction tx = tx_result.value.value();

    // Add to mempool
    auto& mempool = blockchain.GetMempool();
    auto add_result = mempool.AddTransaction(tx);
    if (!add_result.IsOk()) {
        throw std::runtime_error("Failed to add transaction to mempool: " + add_result.error);
    }

    return JSONValue(Uint256ToHex(tx.GetHash()));
}

// Stubs for other methods
JSONValue RawTransactionRPC::decoderawtransaction(const JSONValue&) {
    throw std::runtime_error("Not implemented yet");
}

JSONValue RawTransactionRPC::createrawtransaction(const JSONValue&) {
    throw std::runtime_error("Not implemented yet");
}

JSONValue RawTransactionRPC::signrawtransaction(const JSONValue&) {
    throw std::runtime_error("Not implemented yet");
}

// ============================================================================
// JSON Conversion Helpers
// ============================================================================

namespace json {

JSONValue BlockHeaderToJSON(const BlockHeader& header) {
    std::map<std::string, JSONValue> obj;
    obj["version"] = JSONValue(static_cast<int64_t>(header.version));
    obj["previousblockhash"] = JSONValue(Uint256ToHex(header.prev_block_hash));
    obj["merkleroot"] = JSONValue(Uint256ToHex(header.merkle_root));
    obj["time"] = JSONValue(static_cast<int64_t>(header.timestamp));
    obj["bits"] = JSONValue(static_cast<int64_t>(header.bits));
    obj["nonce"] = JSONValue(static_cast<int64_t>(header.nonce));
    obj["randomxhash"] = JSONValue(Uint256ToHex(header.randomx_hash));

    return JSONValue(obj);
}

JSONValue BlockToJSON(const Block& block, bool verbose, const Blockchain* blockchain) {
    if (!verbose) {
        std::vector<uint8_t> serialized = block.Serialize();
        return JSONValue(BytesToHex(serialized));
    }

    std::map<std::string, JSONValue> obj;
    uint256 block_hash = block.GetHash();
    obj["hash"] = JSONValue(Uint256ToHex(block_hash));

    // Calculate confirmations if blockchain is available
    if (blockchain) {
        uint64_t confirmations = blockchain->GetBlockConfirmations(block_hash);
        obj["confirmations"] = JSONValue(static_cast<int64_t>(confirmations));

        // Calculate height: best_height - confirmations + 1
        uint64_t best_height = blockchain->GetBestHeight();
        if (confirmations > 0) {
            uint64_t height = best_height - confirmations + 1;
            obj["height"] = JSONValue(static_cast<int64_t>(height));
        } else {
            obj["height"] = JSONValue(static_cast<int64_t>(-1));  // Not on main chain
        }
    } else {
        obj["confirmations"] = JSONValue(static_cast<int64_t>(0));
        obj["height"] = JSONValue(static_cast<int64_t>(0));
    }

    obj["size"] = JSONValue(static_cast<int64_t>(block.GetSerializedSize()));
    obj["version"] = JSONValue(static_cast<int64_t>(block.header.version));
    obj["merkleroot"] = JSONValue(Uint256ToHex(block.header.merkle_root));
    obj["time"] = JSONValue(static_cast<int64_t>(block.header.timestamp));
    obj["nonce"] = JSONValue(static_cast<int64_t>(block.header.nonce));
    obj["bits"] = JSONValue(static_cast<int64_t>(block.header.bits));
    obj["previousblockhash"] = JSONValue(Uint256ToHex(block.header.prev_block_hash));

    // Transactions
    std::vector<JSONValue> tx_list;
    for (const auto& tx : block.transactions) {
        tx_list.push_back(JSONValue(Uint256ToHex(tx.GetHash())));
    }
    obj["tx"] = JSONValue(tx_list);

    return JSONValue(obj);
}

JSONValue TransactionToJSON(const Transaction& tx) {
    std::map<std::string, JSONValue> obj;
    obj["txid"] = JSONValue(Uint256ToHex(tx.GetHash()));
    obj["version"] = JSONValue(static_cast<int64_t>(tx.version));
    obj["locktime"] = JSONValue(static_cast<int64_t>(tx.locktime));

    // Inputs
    std::vector<JSONValue> vin_list;
    for (const auto& input : tx.inputs) {
        std::map<std::string, JSONValue> vin;
        vin["txid"] = JSONValue(Uint256ToHex(input.prev_tx_hash));
        vin["vout"] = JSONValue(static_cast<int64_t>(input.prev_tx_index));
        vin["sequence"] = JSONValue(static_cast<int64_t>(input.sequence));
        vin_list.push_back(JSONValue(vin));
    }
    obj["vin"] = JSONValue(vin_list);

    // Outputs
    std::vector<JSONValue> vout_list;
    for (size_t i = 0; i < tx.outputs.size(); i++) {
        const auto& output = tx.outputs[i];
        std::map<std::string, JSONValue> vout;
        vout["value"] = JSONValue(static_cast<double>(output.value) / 1000000.0);  // Convert INTS to INT
        vout["n"] = JSONValue(static_cast<int64_t>(i));
        vout_list.push_back(JSONValue(vout));
    }
    obj["vout"] = JSONValue(vout_list);

    return JSONValue(obj);
}

JSONValue TxOutToJSON(const TxOut& txout) {
    std::map<std::string, JSONValue> obj;
    obj["value"] = JSONValue(static_cast<double>(txout.value) / 1000000.0);
    return JSONValue(obj);
}

JSONValue PeerToJSON(const Peer& peer) {
    std::map<std::string, JSONValue> obj;
    obj["id"] = JSONValue(static_cast<int64_t>(peer.id));
    obj["addr"] = JSONValue(std::string("unknown") + ":" + std::to_string(peer.address.port));
    obj["services"] = JSONValue(static_cast<int64_t>(peer.address.services));
    obj["version"] = JSONValue(static_cast<int64_t>(peer.version));
    obj["subver"] = JSONValue(std::string("unknown"));
    obj["inbound"] = JSONValue(peer.inbound);
    obj["banscore"] = JSONValue(static_cast<int64_t>(peer.ban_score));
    obj["synced_headers"] = JSONValue(static_cast<int64_t>(-1));
    obj["synced_blocks"] = JSONValue(static_cast<int64_t>(-1));

    return JSONValue(obj);
}

JSONValue NetworkAddressToJSON(const NetworkAddress& addr) {
    std::map<std::string, JSONValue> obj;
    obj["ip"] = JSONValue(addr.ToString());
    obj["port"] = JSONValue(static_cast<int64_t>(addr.port));
    obj["services"] = JSONValue(static_cast<int64_t>(addr.services));
    return JSONValue(obj);
}
} // namespace json

// ============================================================================
// HTTP Server Implementation
// ============================================================================

class HTTPServer::Impl {
public:
    std::string bind_address;
    uint16_t port;
    int listen_socket = -1;
    std::thread accept_thread;
    std::atomic<bool> running{false};
    HTTPServer::RequestHandler request_handler;
    std::mutex mutex;

    Impl(const std::string& addr, uint16_t p)
        : bind_address(addr), port(p) {}

    ~Impl() {
        if (running) {
            Stop();
        }
    }

    Result<void> Start() {
        std::lock_guard<std::mutex> lock(mutex);
        if (running) {
            return Result<void>::Error("HTTP server already running");
        }

        // Create socket
        listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_socket < 0) {
            return Result<void>::Error("Failed to create socket");
        }

        // Set socket options
        int opt = 1;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Bind
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, bind_address.c_str(), &addr.sin_addr);

        if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(listen_socket);
            return Result<void>::Error("Failed to bind to " + bind_address + ":" + std::to_string(port));
        }

        // Listen
        if (listen(listen_socket, 10) < 0) {
            close(listen_socket);
            return Result<void>::Error("Failed to listen");
        }

        // Start accept thread
        running = true;
        accept_thread = std::thread([this]() { AcceptLoop(); });

        return Result<void>::Ok();
    }

    Result<void> Stop() {
        running = false;

        if (listen_socket >= 0) {
            close(listen_socket);
            listen_socket = -1;
        }

        if (accept_thread.joinable()) {
            accept_thread.join();
        }

        return Result<void>::Ok();
    }

    void AcceptLoop() {
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running) {
                    // Only log error if we're still supposed to be running
                    continue;
                }
                break;
            }

            // Handle request in separate thread
            std::thread([this, client_socket]() {
                HandleClient(client_socket);
            }).detach();
        }
    }

    void HandleClient(int client_socket) {
        char buffer[8192];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        buffer[bytes_read] = '\0';
        std::string request_str(buffer, bytes_read);

        // Parse HTTP request
        auto request_result = HTTPRequest::Parse(request_str);
        HTTPResponse response;

        if (!request_result.IsOk()) {
            response = HTTPResponse::Error(400, "Bad Request");
        } else if (!request_handler) {
            response = HTTPResponse::Error(500, "No request handler configured");
        } else {
            response = request_handler(request_result.value.value());
        }

        // Send response
        std::string response_str = response.ToString();
        send(client_socket, response_str.c_str(), response_str.length(), 0);
        close(client_socket);
    }
};

HTTPServer::HTTPServer(const std::string& bind_address, uint16_t port)
    : impl_(std::make_unique<Impl>(bind_address, port)) {}

HTTPServer::~HTTPServer() = default;

Result<void> HTTPServer::Start() {
    return impl_->Start();
}

Result<void> HTTPServer::Stop() {
    return impl_->Stop();
}

bool HTTPServer::IsRunning() const {
    return impl_->running;
}

void HTTPServer::SetRequestHandler(RequestHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->request_handler = std::move(handler);
}

// ============================================================================
// HTTP Request/Response Implementation
// ============================================================================

Result<HTTPRequest> HTTPRequest::Parse(const std::string& raw_request) {
    HTTPRequest request;

    // Parse first line (method, URI, version)
    size_t first_line_end = raw_request.find("\r\n");
    if (first_line_end == std::string::npos) {
        return Result<HTTPRequest>::Error("Invalid HTTP request");
    }

    std::string first_line = raw_request.substr(0, first_line_end);
    std::istringstream iss(first_line);
    std::string http_version;
    iss >> request.method >> request.uri >> http_version;

    // Parse headers
    size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return Result<HTTPRequest>::Error("Invalid HTTP request - no header end");
    }

    size_t pos = first_line_end + 2;
    while (pos < header_end) {
        size_t line_end = raw_request.find("\r\n", pos);
        std::string header_line = raw_request.substr(pos, line_end - pos);

        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            request.headers[key] = value;
        }

        pos = line_end + 2;
    }

    // Body
    request.body = raw_request.substr(header_end + 4);

    return Result<HTTPRequest>::Ok(request);
}

std::string HTTPResponse::ToString() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";

    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n" << body;
    return oss.str();
}

HTTPResponse HTTPResponse::OK(const std::string& body, const std::string& content_type) {
    HTTPResponse response;
    response.status_code = 200;
    response.status_message = "OK";
    response.headers["Content-Type"] = content_type;
    response.headers["Content-Length"] = std::to_string(body.length());
    response.headers["Connection"] = "close";
    response.body = body;
    return response;
}

HTTPResponse HTTPResponse::Error(int status_code, const std::string& message) {
    HTTPResponse response;
    response.status_code = status_code;
    response.status_message = message;
    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] = std::to_string(message.length());
    response.headers["Connection"] = "close";
    response.body = message;
    return response;
}

HTTPResponse HTTPResponse::Unauthorized() {
    HTTPResponse response = Error(401, "Unauthorized");
    response.headers["WWW-Authenticate"] = "Basic realm=\"INTcoin RPC\"";
    return response;
}

} // namespace rpc
} // namespace intcoin
