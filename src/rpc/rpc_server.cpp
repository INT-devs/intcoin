// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/rpc.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>

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
    Request req;

    // Simple JSON parsing (sufficient for RPC needs)
    // Extract method
    size_t method_pos = json.find("\"method\":");
    if (method_pos != std::string::npos) {
        size_t start = json.find("\"", method_pos + 9) + 1;
        size_t end = json.find("\"", start);
        req.method = json.substr(start, end - start);
    }

    // Extract params array
    size_t params_pos = json.find("\"params\":");
    if (params_pos != std::string::npos) {
        size_t start = json.find("[", params_pos);
        size_t end = json.find("]", start);
        std::string params_str = json.substr(start + 1, end - start - 1);

        // Parse individual params (comma-separated quoted strings)
        size_t pos = 0;
        while (pos < params_str.length()) {
            size_t quote1 = params_str.find("\"", pos);
            if (quote1 == std::string::npos) break;
            size_t quote2 = params_str.find("\"", quote1 + 1);
            if (quote2 == std::string::npos) break;

            std::string param = params_str.substr(quote1 + 1, quote2 - quote1 - 1);
            req.params.push_back(param);
            pos = quote2 + 1;
        }
    }

    // Extract id
    size_t id_pos = json.find("\"id\":");
    if (id_pos != std::string::npos) {
        size_t start = json.find("\"", id_pos + 5) + 1;
        size_t end = json.find("\"", start);
        req.id = json.substr(start, end - start);
    }

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
    Response resp;

    // Check for error field
    size_t error_pos = json.find("\"error\":");
    if (error_pos != std::string::npos) {
        size_t start = json.find("\"", error_pos + 8) + 1;
        size_t end = json.find("\"", start);
        if (start != std::string::npos + 1 && end != std::string::npos) {
            resp.error = json.substr(start, end - start);
            resp.success = false;
        }
    }

    // Extract result field
    size_t result_pos = json.find("\"result\":");
    if (result_pos != std::string::npos) {
        size_t start = result_pos + 9;
        // Skip whitespace
        while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) {
            start++;
        }

        // Find end of result value (could be string, number, object, array)
        size_t end = start;
        int brace_count = 0;
        int bracket_count = 0;
        bool in_string = false;

        while (end < json.length()) {
            char c = json[end];
            if (c == '"' && (end == start || json[end-1] != '\\')) {
                in_string = !in_string;
            } else if (!in_string) {
                if (c == '{') brace_count++;
                else if (c == '}') {
                    if (brace_count > 0) brace_count--;
                    else break;  // End of response object
                }
                else if (c == '[') bracket_count++;
                else if (c == ']') {
                    if (bracket_count > 0) bracket_count--;
                    else if (brace_count == 0) break;
                }
                else if (c == ',' && brace_count == 0 && bracket_count == 0) {
                    break;  // End of result field
                }
            }
            end++;
        }

        resp.result = json.substr(start, end - start);
        resp.success = true;
    }

    // Extract id
    size_t id_pos = json.find("\"id\":");
    if (id_pos != std::string::npos) {
        size_t start = json.find("\"", id_pos + 5) + 1;
        size_t end = json.find("\"", start);
        if (start != std::string::npos + 1 && end != std::string::npos) {
            resp.id = json.substr(start, end - start);
        }
    }

    return resp;
}

// Server implementation

Server::Server(uint16_t /* port */, Blockchain& blockchain, Mempool& mempool,
               HDWallet* wallet, Miner* miner, p2p::Network* network,
               bridge::BridgeManager* bridge_manager)
    : running_(false)
    , blockchain_(blockchain)
    , mempool_(mempool)
    , wallet_(wallet)
    , miner_(miner)
    , network_(network)
    , bridge_manager_(bridge_manager)
{
    register_blockchain_commands();
    register_wallet_commands();
    register_mining_commands();
    register_network_commands();
    register_bridge_commands();

    // Register utility commands
    register_command("help", [this](const auto& p) { return help(p); });
    register_command("stop", [this](const auto& p) { return stop_server(p); });
    register_command("getmempoolinfo", [this](const auto& p) { return getmempoolinfo(p); });
    register_command("getrawmempool", [this](const auto& p) { return getrawmempool(p); });
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) return false;

    running_ = true;
    // HTTP server implementation deferred to Phase 9 (will use database backend)
    // For now, RPC server works via direct execute() calls from intcoin-cli
    // Future: Implement using Boost.Beast or similar HTTP library

    return true;
}

void Server::stop() {
    if (!running_) return;

    running_ = false;
    // HTTP server stop implementation deferred (see start() for details)
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

void Server::register_bridge_commands() {
    register_command("getbridgeinfo", [this](const auto& p) { return getbridgeinfo(p); });
    register_command("listbridges", [this](const auto& p) { return listbridges(p); });
    register_command("startbridge", [this](const auto& p) { return startbridge(p); });
    register_command("stopbridge", [this](const auto& p) { return stopbridge(p); });
    register_command("initiateswap", [this](const auto& p) { return initiateswap(p); });
    register_command("completeswap", [this](const auto& p) { return completeswap(p); });
    register_command("refundswap", [this](const auto& p) { return refundswap(p); });
    register_command("getswapinfo", [this](const auto& p) { return getswapinfo(p); });
    register_command("getbridgestats", [this](const auto& p) { return getbridgestats(p); });
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

    // Calculate difficulty from latest block's bits field
    double difficulty = 1.0;
    try {
        Block block = blockchain_.get_block(best_block);
        // Difficulty = max_target / current_target
        // For simplicity, use bits field as proxy
        if (block.header.bits > 0) {
            difficulty = static_cast<double>(0xFFFFFFFF) / static_cast<double>(block.header.bits);
        }
    } catch (...) {
        // Use default difficulty if block not found
    }

    std::stringstream ss;
    ss << "{";
    ss << "\"chain\":\"main\",";
    ss << "\"blocks\":" << height << ",";
    ss << "\"bestblockhash\":\"" << hash_to_hex(best_block) << "\",";
    ss << "\"difficulty\":" << std::fixed << std::setprecision(8) << difficulty << ",";
    ss << "\"chainwork\":\"" << height << "\"";  // Approximate chainwork
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

    // Parse IP:port and connect
    std::string node = params[0];
    size_t colon_pos = node.find(':');

    if (colon_pos == std::string::npos) {
        return Response(true, "Invalid node format. Use IP:port", "");
    }

    std::string ip = node.substr(0, colon_pos);
    std::string port_str = node.substr(colon_pos + 1);

    try {
        uint16_t port = static_cast<uint16_t>(std::stoi(port_str));

        p2p::PeerAddress addr(ip, port);
        if (network_->connect_to_peer(addr)) {
            return Response("true", "");
        } else{
            return Response(true, "Failed to connect to peer", "");
        }
    } catch (const std::exception& e) {
        return Response(true, std::string("Invalid port: ") + e.what(), "");
    }
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
    , socket_fd_(-1)
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
    // Establish HTTP connection
    // Parse host:port
    std::string hostname = host_;
    uint16_t port = port_;

    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Resolve hostname
    struct hostent* server = gethostbyname(hostname.c_str());
    if (!server) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    connected_ = true;
    return true;
}

void Client::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

std::string Client::send_request(const std::string& json_request) {
    // Implement HTTP POST request
    if (!connected_) {
        if (!connect()) {
            return "{\"error\":\"Connection failed\"}";
        }
    }

    // Build HTTP POST request
    std::stringstream http_request;
    http_request << "POST / HTTP/1.1\r\n";
    http_request << "Host: " << host_ << "\r\n";
    http_request << "Content-Type: application/json\r\n";
    http_request << "Content-Length: " << json_request.length() << "\r\n";
    http_request << "Connection: keep-alive\r\n";
    http_request << "\r\n";
    http_request << json_request;

    std::string request_str = http_request.str();

    // Send request
    ssize_t sent = send(socket_fd_, request_str.c_str(), request_str.length(), 0);
    if (sent < 0) {
        disconnect();
        return "{\"error\":\"Send failed\"}";
    }

    // Receive response
    char buffer[8192];
    std::string response;
    ssize_t received;

    while ((received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        response += buffer;

        // Check if we've received the complete response
        if (response.find("\r\n\r\n") != std::string::npos) {
            // Found end of headers, check if we have the body
            size_t header_end = response.find("\r\n\r\n");
            size_t content_length_pos = response.find("Content-Length:");

            if (content_length_pos != std::string::npos) {
                size_t length_start = content_length_pos + 15;
                size_t length_end = response.find("\r\n", length_start);
                std::string length_str = response.substr(length_start, length_end - length_start);

                // Trim whitespace
                length_str.erase(0, length_str.find_first_not_of(" \t"));
                length_str.erase(length_str.find_last_not_of(" \t") + 1);

                int content_length = std::stoi(length_str);
                size_t body_start = header_end + 4;
                size_t body_length = response.length() - body_start;

                if (static_cast<int>(body_length) >= content_length) {
                    break;  // Complete response received
                }
            }
        }

        // Prevent infinite loop
        if (response.length() > 1024 * 1024) {  // 1 MB limit
            break;
        }
    }

    if (received < 0) {
        disconnect();
        return "{\"error\":\"Receive failed\"}";
    }

    // Extract JSON body from HTTP response
    size_t body_start = response.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        return response.substr(body_start + 4);
    }

    return "{}";
}

// Bridge RPC methods

Response Server::getbridgeinfo(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.empty()) {
        return Response(true, "Missing bridge chain parameter (bitcoin, ethereum, etc.)", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    auto bridge = bridge_manager_->get_bridge(chain_type.value());
    if (!bridge) {
        return Response(true, "Bridge not found for chain: " + params[0], "");
    }

    std::stringstream ss;
    ss << "{";
    ss << "\"chain\":\"" << bridge->get_chain_name() << "\",";
    ss << "\"status\":";
    switch (bridge->get_status()) {
        case bridge::BridgeStatus::OFFLINE: ss << "\"offline\""; break;
        case bridge::BridgeStatus::SYNCING: ss << "\"syncing\""; break;
        case bridge::BridgeStatus::ONLINE: ss << "\"online\""; break;
        case bridge::BridgeStatus::ERROR: ss << "\"error\""; break;
    }
    ss << ",";
    ss << "\"running\":" << (bridge->is_running() ? "true" : "false") << ",";
    ss << "\"chain_height\":" << bridge->get_chain_height() << ",";
    ss << "\"sync_height\":" << bridge->get_sync_height();
    ss << "}";

    return Response(ss.str(), "");
}

Response Server::listbridges(const std::vector<std::string>& params) {
    (void)params;

    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    auto chains = bridge_manager_->get_available_chains();

    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < chains.size(); ++i) {
        ss << "{";
        ss << "\"chain\":\"" << bridge::BridgeUtils::chain_type_to_string(chains[i]) << "\"";

        auto bridge = bridge_manager_->get_bridge(chains[i]);
        if (bridge) {
            ss << ",\"status\":";
            switch (bridge->get_status()) {
                case bridge::BridgeStatus::OFFLINE: ss << "\"offline\""; break;
                case bridge::BridgeStatus::SYNCING: ss << "\"syncing\""; break;
                case bridge::BridgeStatus::ONLINE: ss << "\"online\""; break;
                case bridge::BridgeStatus::ERROR: ss << "\"error\""; break;
            }
            ss << ",\"running\":" << (bridge->is_running() ? "true" : "false");
        }

        ss << "}";
        if (i < chains.size() - 1) ss << ",";
    }
    ss << "]";

    return Response(ss.str(), "");
}

Response Server::startbridge(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.empty()) {
        return Response(true, "Missing bridge chain parameter", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    auto bridge = bridge_manager_->get_bridge(chain_type.value());
    if (!bridge) {
        return Response(true, "Bridge not found for chain: " + params[0], "");
    }

    if (bridge->start()) {
        return Response("\"Bridge started successfully\"", "");
    } else {
        return Response(true, "Failed to start bridge", "");
    }
}

Response Server::stopbridge(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.empty()) {
        return Response(true, "Missing bridge chain parameter", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    auto bridge = bridge_manager_->get_bridge(chain_type.value());
    if (!bridge) {
        return Response(true, "Bridge not found for chain: " + params[0], "");
    }

    bridge->stop();
    return Response("\"Bridge stopped successfully\"", "");
}

Response Server::initiateswap(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.size() < 3) {
        return Response(true, "Usage: initiateswap <chain> <recipient_address> <amount>", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    // Parse recipient address (simplified - in production, decode properly)
    DilithiumPubKey recipient{};
    // In production: decode address to public key

    // Parse amount
    uint64_t amount = std::stoull(params[2]);

    try {
        Hash256 swap_id = bridge_manager_->create_cross_chain_swap(
            chain_type.value(),
            recipient,
            amount
        );

        std::string swap_id_hex = hash_to_hex(swap_id);
        return Response("\"" + swap_id_hex + "\"", "");
    } catch (const std::exception& e) {
        return Response(true, std::string("Swap initiation failed: ") + e.what(), "");
    }
}

Response Server::completeswap(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.size() < 3) {
        return Response(true, "Usage: completeswap <chain> <swap_id> <secret>", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    Hash256 swap_id = hex_to_hash(params[1]);
    Hash256 secret = hex_to_hash(params[2]);

    if (bridge_manager_->complete_cross_chain_swap(chain_type.value(), swap_id, secret)) {
        return Response("\"Swap completed successfully\"", "");
    } else {
        return Response(true, "Failed to complete swap", "");
    }
}

Response Server::refundswap(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.size() < 2) {
        return Response(true, "Usage: refundswap <chain> <swap_id>", "");
    }

    auto chain_type = bridge::BridgeUtils::string_to_chain_type(params[0]);
    if (!chain_type.has_value()) {
        return Response(true, "Unknown chain type: " + params[0], "");
    }

    auto bridge = bridge_manager_->get_bridge(chain_type.value());
    if (!bridge) {
        return Response(true, "Bridge not found for chain: " + params[0], "");
    }

    Hash256 swap_id = hex_to_hash(params[1]);

    if (bridge->refund_swap(swap_id)) {
        return Response("\"Swap refunded successfully\"", "");
    } else {
        return Response(true, "Failed to refund swap", "");
    }
}

Response Server::getswapinfo(const std::vector<std::string>& params) {
    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    if (params.size() < 2) {
        return Response(true, "Usage: getswapinfo <chain> <swap_id>", "");
    }

    // In production: retrieve swap info from bridge
    // For now, return placeholder
    return Response("{\"swap_id\":\"" + params[1] + "\",\"status\":\"unknown\"}", "");
}

Response Server::getbridgestats(const std::vector<std::string>& params) {
    (void)params;

    if (!bridge_manager_) {
        return Response(true, "Bridge manager not initialized", "");
    }

    auto all_stats = bridge_manager_->get_all_stats();

    std::stringstream ss;
    ss << "{";
    ss << "\"total_bridges\":" << all_stats.total_bridges << ",";
    ss << "\"online_bridges\":" << all_stats.online_bridges << ",";
    ss << "\"total_swaps\":" << all_stats.total_swaps << ",";
    ss << "\"total_volume\":" << all_stats.total_volume << ",";
    ss << "\"per_chain\":{";

    size_t count = 0;
    for (const auto& [chain, stats] : all_stats.per_chain_stats) {
        if (count > 0) ss << ",";
        ss << "\"" << bridge::BridgeUtils::chain_type_to_string(chain) << "\":{";
        ss << "\"total_swaps\":" << stats.total_swaps << ",";
        ss << "\"completed_swaps\":" << stats.completed_swaps << ",";
        ss << "\"failed_swaps\":" << stats.failed_swaps << ",";
        ss << "\"total_volume_sent\":" << stats.total_volume_sent << ",";
        ss << "\"total_volume_received\":" << stats.total_volume_received << ",";
        ss << "\"success_rate\":" << std::fixed << std::setprecision(2) << (stats.success_rate * 100) << "%";
        ss << "}";
        count++;
    }

    ss << "}}";

    return Response(ss.str(), "");
}

} // namespace rpc
} // namespace intcoin
