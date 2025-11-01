// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/rpc.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace intcoin {
namespace rpc {

// Request implementation

std::string Request::to_json() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"jsonrpc\":\"2.0\",";
    ss << "\"method\":\"" << method << "\",";
    ss << "\"params\":[";
    for (size_t i = 0; i < params.size(); ++i) {
        ss << "\"" << params[i] << "\"";
        if (i < params.size() - 1) ss << ",";
    }
    ss << "],";
    ss << "\"id\":\"" << id << "\"";
    ss << "}";
    return ss.str();
}

Request Request::from_json(const std::string& json) {
    // TODO: Implement JSON parsing
    Request req;
    (void)json;
    return req;
}

// Response implementation

std::string Response::to_json() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"jsonrpc\":\"2.0\",";
    if (success) {
        ss << "\"result\":" << result << ",";
    } else {
        ss << "\"error\":\"" << error << "\",";
    }
    ss << "\"id\":\"" << id << "\"";
    ss << "}";
    return ss.str();
}

Response Response::from_json(const std::string& json) {
    // TODO: Implement JSON parsing
    Response resp;
    (void)json;
    return resp;
}

// Server implementation

Server::Server(uint16_t port, Blockchain& blockchain, Mempool& mempool,
               HDWallet* wallet, Miner* miner, p2p::Network* network)
    : port_(port)
    , running_(false)
    , blockchain_(blockchain)
    , mempool_(mempool)
    , wallet_(wallet)
    , miner_(miner)
    , network_(network)
{
    register_blockchain_commands();
    register_wallet_commands();
    register_mining_commands();
    register_network_commands();

    // Register utility commands
    register_command("help", [this](const auto& p) { return help(p); });
    register_command("stop", [this](const auto& p) { return stop_server(p); });
    register_command("getmempoolinfo", [this](const auto& p) { return getmempoolinfo(p); });
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) return false;

    running_ = true;
    // TODO: Start HTTP server on port_

    return true;
}

void Server::stop() {
    if (!running_) return;

    running_ = false;
    // TODO: Stop HTTP server
}

void Server::register_command(const std::string& name, CommandHandler handler) {
    commands_[name] = handler;
}

Response Server::execute(const Request& request) {
    auto it = commands_.find(request.method);
    if (it == commands_.end()) {
        return Response(true, "Method not found: " + request.method, request.id);
    }

    try {
        return it->second(request.params);
    } catch (const std::exception& e) {
        return Response(true, std::string("Error: ") + e.what(), request.id);
    }
}

void Server::register_blockchain_commands() {
    register_command("getblockcount", [this](const auto& p) { return getblockcount(p); });
    register_command("getblockhash", [this](const auto& p) { return getblockhash(p); });
    register_command("getblock", [this](const auto& p) { return getblock(p); });
    register_command("getblockchaininfo", [this](const auto& p) { return getblockchaininfo(p); });
}

void Server::register_wallet_commands() {
    register_command("getnewaddress", [this](const auto& p) { return getnewaddress(p); });
    register_command("getbalance", [this](const auto& p) { return getbalance(p); });
    register_command("sendtoaddress", [this](const auto& p) { return sendtoaddress(p); });
    register_command("listtransactions", [this](const auto& p) { return listtransactions(p); });
    register_command("listaddresses", [this](const auto& p) { return listaddresses(p); });
}

void Server::register_mining_commands() {
    register_command("getmininginfo", [this](const auto& p) { return getmininginfo(p); });
    register_command("startmining", [this](const auto& p) { return startmining(p); });
    register_command("stopmining", [this](const auto& p) { return stopmining(p); });
}

void Server::register_network_commands() {
    register_command("getpeerinfo", [this](const auto& p) { return getpeerinfo(p); });
    register_command("getnetworkinfo", [this](const auto& p) { return getnetworkinfo(p); });
    register_command("addnode", [this](const auto& p) { return addnode(p); });
}

// Blockchain RPC methods

Response Server::getblockcount(const std::vector<std::string>& params) {
    (void)params;
    uint32_t height = blockchain_.get_height();
    return Response(std::to_string(height), "");
}

Response Server::getblockhash(const std::vector<std::string>& params) {
    if (params.empty()) {
        return Response(true, "Missing height parameter", "");
    }

    uint32_t height = std::stoul(params[0]);
    try {
        Block block = blockchain_.get_block_by_height(height);
        std::string hash_hex = hash_to_hex(block.get_hash());
        return Response("\"" + hash_hex + "\"", "");
    } catch (...) {
        return Response(true, "Block not found at height " + params[0], "");
    }
}

Response Server::getblock(const std::vector<std::string>& params) {
    if (params.empty()) {
        return Response(true, "Missing block hash parameter", "");
    }

    Hash256 hash = hex_to_hash(params[0]);
    try {
        Block block = blockchain_.get_block(hash);

        std::stringstream ss;
        ss << "{";
        ss << "\"hash\":\"" << hash_to_hex(block.get_hash()) << "\",";
        ss << "\"version\":" << block.header.version << ",";
        ss << "\"previousblockhash\":\"" << hash_to_hex(block.header.previous_block_hash) << "\",";
        ss << "\"merkleroot\":\"" << hash_to_hex(block.header.merkle_root) << "\",";
        ss << "\"time\":" << block.header.timestamp << ",";
        ss << "\"bits\":" << block.header.bits << ",";
        ss << "\"nonce\":" << block.header.nonce << ",";
        ss << "\"tx\":" << block.transactions.size();
        ss << "}";

        return Response(ss.str(), "");
    } catch (...) {
        return Response(true, "Block not found", "");
    }
}

Response Server::getblockchaininfo(const std::vector<std::string>& params) {
    (void)params;

    uint32_t height = blockchain_.get_height();
    Hash256 best_block = blockchain_.get_best_block_hash();

    std::stringstream ss;
    ss << "{";
    ss << "\"chain\":\"main\",";
    ss << "\"blocks\":" << height << ",";
    ss << "\"bestblockhash\":\"" << hash_to_hex(best_block) << "\",";
    ss << "\"difficulty\":" << 0 << ",";  // TODO: Calculate actual difficulty
    ss << "\"chainwork\":\"" << 0 << "\"";
    ss << "}";

    return Response(ss.str(), "");
}

// Wallet RPC methods

Response Server::getnewaddress(const std::vector<std::string>& params) {
    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    std::string label = params.empty() ? "" : params[0];
    std::string address = wallet_->get_new_address(label);
    return Response("\"" + address + "\"", "");
}

Response Server::getbalance(const std::vector<std::string>& params) {
    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    (void)params;
    uint64_t balance = wallet_->get_balance(blockchain_);
    double balance_int = static_cast<double>(balance) / COIN;
    return Response(std::to_string(balance_int), "");
}

Response Server::sendtoaddress(const std::vector<std::string>& params) {
    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    if (params.size() < 2) {
        return Response(true, "Usage: sendtoaddress <address> <amount>", "");
    }

    std::string address = params[0];
    uint64_t amount = std::stoull(params[1]);

    auto tx = wallet_->create_transaction(address, amount, 1000, blockchain_);
    if (!tx) {
        return Response(true, "Failed to create transaction", "");
    }

    return Response("\"" + hash_to_hex(tx->get_hash()) + "\"", "");
}

Response Server::listtransactions(const std::vector<std::string>& params) {
    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    (void)params;
    auto history = wallet_->get_transaction_history(blockchain_);

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < history.size(); ++i) {
        ss << "{";
        ss << "\"txid\":\"" << hash_to_hex(history[i].tx_hash) << "\",";
        ss << "\"amount\":" << (static_cast<double>(history[i].amount) / COIN) << ",";
        ss << "\"confirmations\":" << history[i].confirmations;
        ss << "}";
        if (i < history.size() - 1) ss << ",";
    }
    ss << "]";

    return Response(ss.str(), "");
}

Response Server::listaddresses(const std::vector<std::string>& params) {
    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    (void)params;
    auto addresses = wallet_->get_all_addresses();

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < addresses.size(); ++i) {
        ss << "\"" << addresses[i] << "\"";
        if (i < addresses.size() - 1) ss << ",";
    }
    ss << "]";

    return Response(ss.str(), "");
}

// Mining RPC methods

Response Server::getmininginfo(const std::vector<std::string>& params) {
    if (!miner_) {
        return Response(true, "Miner not available", "");
    }

    (void)params;
    auto stats = miner_->get_stats();

    std::stringstream ss;
    ss << "{";
    ss << "\"mining\":" << (miner_->is_mining() ? "true" : "false") << ",";
    ss << "\"hashrate\":" << stats.hashes_per_second << ",";
    ss << "\"blocks\":" << stats.blocks_found << ",";
    ss << "\"difficulty\":" << stats.current_difficulty;
    ss << "}";

    return Response(ss.str(), "");
}

Response Server::startmining(const std::vector<std::string>& params) {
    if (!miner_) {
        return Response(true, "Miner not available", "");
    }

    if (!wallet_) {
        return Response(true, "Wallet not loaded", "");
    }

    size_t threads = params.empty() ? 0 : std::stoull(params[0]);

    auto keys = wallet_->get_all_keys();
    if (keys.empty()) {
        return Response(true, "No addresses in wallet", "");
    }

    bool started = miner_->start(keys[0].public_key, threads);
    return Response(started ? "true" : "false", "");
}

Response Server::stopmining(const std::vector<std::string>& params) {
    if (!miner_) {
        return Response(true, "Miner not available", "");
    }

    (void)params;
    miner_->stop();
    return Response("true", "");
}

// Network RPC methods

Response Server::getpeerinfo(const std::vector<std::string>& params) {
    if (!network_) {
        return Response(true, "Network not available", "");
    }

    (void)params;
    auto peers = network_->get_peers();

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < peers.size(); ++i) {
        ss << "{";
        ss << "\"addr\":\"" << peers[i].to_string() << "\",";
        ss << "\"services\":" << peers[i].services;
        ss << "}";
        if (i < peers.size() - 1) ss << ",";
    }
    ss << "]";

    return Response(ss.str(), "");
}

Response Server::getnetworkinfo(const std::vector<std::string>& params) {
    if (!network_) {
        return Response(true, "Network not available", "");
    }

    (void)params;

    std::stringstream ss;
    ss << "{";
    ss << "\"version\":1,";
    ss << "\"connections\":" << network_->peer_count() << ",";
    ss << "\"networkactive\":" << (network_->is_running() ? "true" : "false");
    ss << "}";

    return Response(ss.str(), "");
}

Response Server::addnode(const std::vector<std::string>& params) {
    if (!network_) {
        return Response(true, "Network not available", "");
    }

    if (params.empty()) {
        return Response(true, "Usage: addnode <node>", "");
    }

    // TODO: Parse IP:port and connect
    return Response("true", "");
}

// Mempool RPC methods

Response Server::getmempoolinfo(const std::vector<std::string>& params) {
    (void)params;

    std::stringstream ss;
    ss << "{";
    ss << "\"size\":" << mempool_.size() << ",";
    ss << "\"bytes\":" << mempool_.total_size_bytes() << ",";
    ss << "\"usage\":" << mempool_.total_size_bytes() << ",";
    ss << "\"total_fee\":" << (static_cast<double>(mempool_.total_fees()) / COIN);
    ss << "}";

    return Response(ss.str(), "");
}

Response Server::getrawmempool(const std::vector<std::string>& params) {
    (void)params;

    auto txs = mempool_.get_all_transactions();

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < txs.size(); ++i) {
        ss << "\"" << hash_to_hex(txs[i].get_hash()) << "\"";
        if (i < txs.size() - 1) ss << ",";
    }
    ss << "]";

    return Response(ss.str(), "");
}

// Utility methods

Response Server::help(const std::vector<std::string>& params) {
    (void)params;

    std::stringstream ss;
    ss << "[";
    for (const auto& [name, handler] : commands_) {
        ss << "\"" << name << "\",";
    }
    std::string result = ss.str();
    if (!result.empty() && result.back() == ',') {
        result.pop_back();
    }
    result += "]";

    return Response(result, "");
}

Response Server::stop_server(const std::vector<std::string>& params) {
    (void)params;
    stop();
    return Response("\"Server stopping\"", "");
}

// Helper functions

std::string Server::hash_to_hex(const Hash256& hash) const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

Hash256 Server::hex_to_hash(const std::string& hex) const {
    Hash256 hash{};
    for (size_t i = 0; i < 32 && i * 2 < hex.length(); ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        hash[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
    }
    return hash;
}

// Client implementation

Client::Client(const std::string& host, uint16_t port)
    : host_(host)
    , port_(port)
    , connected_(false)
{
}

Response Client::call(const std::string& method, const std::vector<std::string>& params) {
    Request request(method, params, "1");
    std::string json_request = request.to_json();

    try {
        std::string json_response = send_request(json_request);
        return Response::from_json(json_response);
    } catch (...) {
        return Response(true, "Connection error", request.id);
    }
}

bool Client::connect() {
    // TODO: Establish HTTP connection
    connected_ = true;
    return true;
}

void Client::disconnect() {
    connected_ = false;
}

std::string Client::send_request(const std::string& json_request) {
    // TODO: Implement HTTP POST request
    (void)json_request;
    return "{}";
}

} // namespace rpc
} // namespace intcoin
