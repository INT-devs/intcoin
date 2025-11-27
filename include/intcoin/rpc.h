/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * JSON-RPC Server Interface
 */

#ifndef INTCOIN_RPC_H
#define INTCOIN_RPC_H

#include "types.h"
#include "blockchain.h"
#include "network.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace intcoin {
namespace rpc {

// ============================================================================
// JSON Value Types
// ============================================================================

enum class JSONType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JSONValue {
public:
    JSONType type;

    // Value storage
    bool bool_value;
    double number_value;
    std::string string_value;
    std::vector<JSONValue> array_value;
    std::map<std::string, JSONValue> object_value;

    // Constructors
    JSONValue() : type(JSONType::Null), bool_value(false), number_value(0.0) {}
    explicit JSONValue(bool b) : type(JSONType::Bool), bool_value(b), number_value(0.0) {}
    explicit JSONValue(int64_t n) : type(JSONType::Number), bool_value(false), number_value(static_cast<double>(n)) {}
    explicit JSONValue(double n) : type(JSONType::Number), bool_value(false), number_value(n) {}
    explicit JSONValue(const std::string& s) : type(JSONType::String), bool_value(false), number_value(0.0), string_value(s) {}
    explicit JSONValue(const char* s) : type(JSONType::String), bool_value(false), number_value(0.0), string_value(s) {}
    explicit JSONValue(const std::vector<JSONValue>& arr) : type(JSONType::Array), bool_value(false), number_value(0.0), array_value(arr) {}
    explicit JSONValue(const std::map<std::string, JSONValue>& obj) : type(JSONType::Object), bool_value(false), number_value(0.0), object_value(obj) {}

    // Type checks
    bool IsNull() const { return type == JSONType::Null; }
    bool IsBool() const { return type == JSONType::Bool; }
    bool IsNumber() const { return type == JSONType::Number; }
    bool IsString() const { return type == JSONType::String; }
    bool IsArray() const { return type == JSONType::Array; }
    bool IsObject() const { return type == JSONType::Object; }

    // Value getters
    bool GetBool() const { return bool_value; }
    int64_t GetInt() const { return static_cast<int64_t>(number_value); }
    double GetDouble() const { return number_value; }
    const std::string& GetString() const { return string_value; }
    const std::vector<JSONValue>& GetArray() const { return array_value; }
    const std::map<std::string, JSONValue>& GetObject() const { return object_value; }

    // Object operations
    bool HasKey(const std::string& key) const {
        return object_value.find(key) != object_value.end();
    }

    const JSONValue& operator[](const std::string& key) const {
        static JSONValue null_value;
        auto it = object_value.find(key);
        return (it != object_value.end()) ? it->second : null_value;
    }

    JSONValue& operator[](const std::string& key) {
        return object_value[key];
    }

    // Array operations
    size_t Size() const {
        if (IsArray()) return array_value.size();
        if (IsObject()) return object_value.size();
        return 0;
    }

    const JSONValue& operator[](size_t index) const {
        static JSONValue null_value;
        return (index < array_value.size()) ? array_value[index] : null_value;
    }

    // Serialization
    std::string ToJSONString() const;

    // Parsing
    static Result<JSONValue> Parse(const std::string& json_str);
};

// ============================================================================
// RPC Request/Response
// ============================================================================

struct RPCRequest {
    std::string jsonrpc;  // "2.0"
    std::string method;
    JSONValue params;
    std::optional<JSONValue> id;

    // Parse from JSON string
    static Result<RPCRequest> Parse(const std::string& json_str);
};

struct RPCResponse {
    std::string jsonrpc;  // "2.0"
    JSONValue result;
    std::optional<JSONValue> error;
    std::optional<JSONValue> id;

    // Serialize to JSON string
    std::string ToJSONString() const;

    // Helper constructors
    static RPCResponse Success(const JSONValue& result, const std::optional<JSONValue>& id = std::nullopt);
    static RPCResponse Error(int code, const std::string& message, const std::optional<JSONValue>& id = std::nullopt);
};

// RPC Error codes (JSON-RPC 2.0 standard + Bitcoin-compatible)
namespace RPCErrorCode {
    constexpr int PARSE_ERROR = -32700;
    constexpr int INVALID_REQUEST = -32600;
    constexpr int METHOD_NOT_FOUND = -32601;
    constexpr int INVALID_PARAMS = -32602;
    constexpr int INTERNAL_ERROR = -32603;

    // INTcoin-specific errors (compatible with Bitcoin Core)
    constexpr int RPC_MISC_ERROR = -1;
    constexpr int RPC_TYPE_ERROR = -3;
    constexpr int RPC_INVALID_ADDRESS_OR_KEY = -5;
    constexpr int RPC_OUT_OF_MEMORY = -7;
    constexpr int RPC_INVALID_PARAMETER = -8;
    constexpr int RPC_DATABASE_ERROR = -20;
    constexpr int RPC_DESERIALIZATION_ERROR = -22;
    constexpr int RPC_VERIFY_ERROR = -25;
    constexpr int RPC_VERIFY_REJECTED = -26;
    constexpr int RPC_VERIFY_ALREADY_IN_CHAIN = -27;
    constexpr int RPC_IN_WARMUP = -28;
    constexpr int RPC_WALLET_ERROR = -4;
    constexpr int RPC_WALLET_INSUFFICIENT_FUNDS = -6;
    constexpr int RPC_WALLET_INVALID_LABEL_NAME = -11;
    constexpr int RPC_WALLET_KEYPOOL_RAN_OUT = -12;
    constexpr int RPC_WALLET_UNLOCK_NEEDED = -13;
    constexpr int RPC_WALLET_PASSPHRASE_INCORRECT = -14;
    constexpr int RPC_WALLET_WRONG_ENC_STATE = -15;
    constexpr int RPC_WALLET_ENCRYPTION_FAILED = -16;
    constexpr int RPC_WALLET_ALREADY_UNLOCKED = -17;
}

// ============================================================================
// RPC Method Handler
// ============================================================================

// RPC method handler function type
using RPCMethodHandler = std::function<JSONValue(const JSONValue& params)>;

// RPC method metadata
struct RPCMethodInfo {
    std::string name;
    std::string description;
    std::vector<std::string> param_names;
    bool requires_auth;
    RPCMethodHandler handler;
};

// ============================================================================
// RPC Server
// ============================================================================

struct RPCConfig {
    std::string bind_address = "127.0.0.1";  // Localhost only by default
    uint16_t port = 2211;  // Default RPC port (mainnet)
    std::string rpc_user;
    std::string rpc_password;
    bool allow_external = false;  // Allow connections from outside localhost
    size_t max_connections = 30;
    size_t timeout_seconds = 30;
};

class RPCServer {
public:
    RPCServer(const RPCConfig& config, Blockchain& blockchain, P2PNode& network);
    ~RPCServer();

    // Server lifecycle
    Result<void> Start();
    Result<void> Stop();
    bool IsRunning() const;

    // Register RPC methods
    void RegisterMethod(const RPCMethodInfo& method);
    void RegisterAllMethods();

    // Handle RPC request (for internal use or testing)
    RPCResponse HandleRequest(const RPCRequest& request);

    // Get server statistics
    struct Stats {
        uint64_t total_requests = 0;
        uint64_t successful_requests = 0;
        uint64_t failed_requests = 0;
        uint64_t auth_failures = 0;
        std::chrono::system_clock::time_point start_time;
    };
    Stats GetStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// RPC Method Categories
// ============================================================================

// All RPC methods are registered via helper classes
// This allows for organized grouping of related methods

class BlockchainRPC {
public:
    static void RegisterMethods(RPCServer& server, Blockchain& blockchain);

    // Blockchain query methods
    static JSONValue getblockcount(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getbestblockhash(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getblockhash(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getblock(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getblockheader(const JSONValue& params, Blockchain& blockchain);
    static JSONValue gettxout(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getchaintxstats(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getdifficulty(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getmempoolinfo(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getrawmempool(const JSONValue& params, Blockchain& blockchain);
};

class NetworkRPC {
public:
    static void RegisterMethods(RPCServer& server, P2PNode& network);

    // Network information methods
    static JSONValue getnetworkinfo(const JSONValue& params, P2PNode& network);
    static JSONValue getpeerinfo(const JSONValue& params, P2PNode& network);
    static JSONValue getconnectioncount(const JSONValue& params, P2PNode& network);
    static JSONValue addnode(const JSONValue& params, P2PNode& network);
    static JSONValue disconnectnode(const JSONValue& params, P2PNode& network);
    static JSONValue getaddednodeinfo(const JSONValue& params, P2PNode& network);
    static JSONValue setban(const JSONValue& params, P2PNode& network);
    static JSONValue listbanned(const JSONValue& params, P2PNode& network);
    static JSONValue clearbanned(const JSONValue& params, P2PNode& network);
};

class MiningRPC {
public:
    static void RegisterMethods(RPCServer& server, Blockchain& blockchain);

    // Mining methods
    static JSONValue getmininginfo(const JSONValue& params, Blockchain& blockchain);
    static JSONValue getblocktemplate(const JSONValue& params, Blockchain& blockchain);
    static JSONValue submitblock(const JSONValue& params, Blockchain& blockchain);
    static JSONValue generatetoaddress(const JSONValue& params, Blockchain& blockchain);
};

class UtilityRPC {
public:
    static void RegisterMethods(RPCServer& server);

    // Utility methods
    static JSONValue help(const JSONValue& params, RPCServer& server);
    static JSONValue uptime(const JSONValue& params);
    static JSONValue getinfo(const JSONValue& params, Blockchain& blockchain, P2PNode& network);
    static JSONValue validateaddress(const JSONValue& params);
    static JSONValue verifymessage(const JSONValue& params);
};

class RawTransactionRPC {
public:
    static void RegisterMethods(RPCServer& server, Blockchain& blockchain);

    // Raw transaction methods
    static JSONValue getrawtransaction(const JSONValue& params, Blockchain& blockchain);
    static JSONValue decoderawtransaction(const JSONValue& params);
    static JSONValue createrawtransaction(const JSONValue& params);
    static JSONValue signrawtransaction(const JSONValue& params);
    static JSONValue sendrawtransaction(const JSONValue& params, Blockchain& blockchain);
};

// ============================================================================
// Helper Functions
// ============================================================================

// Convert blockchain types to JSON
namespace json {
    JSONValue BlockHeaderToJSON(const BlockHeader& header);
    JSONValue BlockToJSON(const Block& block, bool verbose = true);
    JSONValue TransactionToJSON(const Transaction& tx);
    JSONValue TxOutToJSON(const TxOut& txout);
    JSONValue PeerToJSON(const Peer& peer);
    JSONValue NetworkAddressToJSON(const NetworkAddress& addr);
}

// ============================================================================
// HTTP Server (Internal)
// ============================================================================

// HTTP request structure
struct HTTPRequest {
    std::string method;  // "GET", "POST", etc.
    std::string uri;
    std::map<std::string, std::string> headers;
    std::string body;

    static Result<HTTPRequest> Parse(const std::string& raw_request);
};

// HTTP response structure
struct HTTPResponse {
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string ToString() const;

    // Helper constructors
    static HTTPResponse OK(const std::string& body, const std::string& content_type = "application/json");
    static HTTPResponse Error(int status_code, const std::string& message);
    static HTTPResponse Unauthorized();
};

// Simple HTTP server for RPC
class HTTPServer {
public:
    HTTPServer(const std::string& bind_address, uint16_t port);
    ~HTTPServer();

    Result<void> Start();
    Result<void> Stop();
    bool IsRunning() const;

    // Set request handler
    using RequestHandler = std::function<HTTPResponse(const HTTPRequest&)>;
    void SetRequestHandler(RequestHandler handler);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rpc
} // namespace intcoin

#endif // INTCOIN_RPC_H
