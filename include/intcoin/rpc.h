// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// JSON-RPC server and client for remote control.

#ifndef INTCOIN_RPC_H
#define INTCOIN_RPC_H

#include "blockchain.h"
#include "wallet.h"
#include "mempool.h"
#include "miner.h"
#include "p2p.h"
#include <string>
#include <functional>
#include <map>
#include <memory>
#include <optional>

namespace intcoin {
namespace rpc {

/**
 * JSON-RPC request
 */
struct Request {
    std::string method;
    std::vector<std::string> params;
    std::string id;

    Request() = default;
    Request(const std::string& m, const std::vector<std::string>& p, const std::string& i = "")
        : method(m), params(p), id(i) {}

    std::string to_json() const;
    static Request from_json(const std::string& json);
};

/**
 * JSON-RPC response
 */
struct Response {
    std::string result;
    std::string error;
    std::string id;
    bool success;

    Response() : success(false) {}
    Response(const std::string& res, const std::string& req_id)
        : result(res), id(req_id), success(true) {}
    Response(bool is_error, const std::string& msg, const std::string& req_id)
        : error(msg), id(req_id), success(!is_error) {}

    std::string to_json() const;
    static Response from_json(const std::string& json);
};

/**
 * RPC command handler
 */
using CommandHandler = std::function<Response(const std::vector<std::string>&)>;

/**
 * RPC Server
 */
class Server {
public:
    Server(uint16_t port, Blockchain& blockchain, Mempool& mempool,
           HDWallet* wallet = nullptr, Miner* miner = nullptr, p2p::Network* network = nullptr);
    ~Server();

    // Server control
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Command registration
    void register_command(const std::string& name, CommandHandler handler);

    // Execute command (for internal use or testing)
    Response execute(const Request& request);

private:
    [[maybe_unused]] uint16_t port_;
    bool running_;

    // Core components
    Blockchain& blockchain_;
    Mempool& mempool_;
    HDWallet* wallet_;
    Miner* miner_;
    p2p::Network* network_;

    // Command handlers
    std::map<std::string, CommandHandler> commands_;

    // Built-in command handlers
    void register_blockchain_commands();
    void register_wallet_commands();
    void register_mining_commands();
    void register_network_commands();

    // Blockchain RPC methods
    Response getblockcount(const std::vector<std::string>& params);
    Response getblockhash(const std::vector<std::string>& params);
    Response getblock(const std::vector<std::string>& params);
    Response getblockchaininfo(const std::vector<std::string>& params);

    // Wallet RPC methods
    Response getnewaddress(const std::vector<std::string>& params);
    Response getbalance(const std::vector<std::string>& params);
    Response sendtoaddress(const std::vector<std::string>& params);
    Response listtransactions(const std::vector<std::string>& params);
    Response listaddresses(const std::vector<std::string>& params);

    // Mining RPC methods
    Response getmininginfo(const std::vector<std::string>& params);
    Response startmining(const std::vector<std::string>& params);
    Response stopmining(const std::vector<std::string>& params);

    // Network RPC methods
    Response getpeerinfo(const std::vector<std::string>& params);
    Response getnetworkinfo(const std::vector<std::string>& params);
    Response addnode(const std::vector<std::string>& params);

    // Mempool RPC methods
    Response getmempoolinfo(const std::vector<std::string>& params);
    Response getrawmempool(const std::vector<std::string>& params);

    // Utility methods
    Response help(const std::vector<std::string>& params);
    Response stop_server(const std::vector<std::string>& params);

    // Helper functions
    std::string hash_to_hex(const Hash256& hash) const;
    Hash256 hex_to_hash(const std::string& hex) const;
};

/**
 * RPC Client
 */
class Client {
public:
    Client(const std::string& host, uint16_t port);

    // Execute RPC command
    Response call(const std::string& method, const std::vector<std::string>& params = {});

    // Connection
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }

private:
    std::string host_;
    [[maybe_unused]] uint16_t port_;
    bool connected_;

    // HTTP client functionality
    std::string send_request(const std::string& json_request);
};

/**
 * Common RPC methods (convenience wrappers)
 */
namespace methods {
    // Blockchain
    inline Request getblockcount() {
        return Request("getblockcount", {});
    }

    inline Request getblockhash(uint32_t height) {
        return Request("getblockhash", {std::to_string(height)});
    }

    inline Request getblock(const std::string& hash) {
        return Request("getblock", {hash});
    }

    // Wallet
    inline Request getnewaddress(const std::string& label = "") {
        return Request("getnewaddress", {label});
    }

    inline Request getbalance() {
        return Request("getbalance", {});
    }

    inline Request sendtoaddress(const std::string& address, uint64_t amount) {
        return Request("sendtoaddress", {address, std::to_string(amount)});
    }

    // Mining
    inline Request getmininginfo() {
        return Request("getmininginfo", {});
    }

    inline Request startmining(size_t threads = 0) {
        return Request("startmining", {std::to_string(threads)});
    }

    inline Request stopmining() {
        return Request("stopmining", {});
    }

    // Network
    inline Request getpeerinfo() {
        return Request("getpeerinfo", {});
    }

    inline Request addnode(const std::string& node) {
        return Request("addnode", {node});
    }
}

} // namespace rpc
} // namespace intcoin

#endif // INTCOIN_RPC_H
