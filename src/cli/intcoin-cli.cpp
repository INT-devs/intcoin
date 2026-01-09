// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/intcoin.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Simple HTTP client for RPC
class RPCClient {
public:
    RPCClient(const std::string& host, uint16_t port,
              const std::string& user, const std::string& pass)
        : host_(host), port_(port), username_(user), password_(pass) {}

    std::string SendRequest(const std::string& method, const std::string& params = "[]") {
        // Create JSON-RPC request
        std::ostringstream json;
        json << "{"
             << "\"jsonrpc\":\"2.0\","
             << "\"id\":\"cli\","
             << "\"method\":\"" << method << "\","
             << "\"params\":" << params
             << "}";

        std::string request_body = json.str();

        // Create HTTP request
        std::ostringstream http;
        http << "POST / HTTP/1.1\r\n";
        http << "Host: " << host_ << ":" << port_ << "\r\n";
        http << "Content-Type: application/json\r\n";
        http << "Content-Length: " << request_body.length() << "\r\n";

        // Add Basic Auth if credentials provided
        if (!username_.empty()) {
            std::string auth = username_ + ":" + password_;
            // Simple base64-like encoding (not secure, just for demo)
            http << "Authorization: Basic " << Base64Encode(auth) << "\r\n";
        }

        http << "Connection: close\r\n";
        http << "\r\n";
        http << request_body;

        std::string request = http.str();

        // Connect to server
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return "{\"error\":\"Failed to create socket\"}";
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(sock);
            return "{\"error\":\"Invalid address\"}";
        }

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "{\"error\":\"Connection failed. Is intcoind running?\"}";
        }

        // Send request
        send(sock, request.c_str(), request.length(), 0);

        // Receive response
        std::string response;
        char buffer[4096];
        ssize_t bytes_read;

        while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_read] = '\0';
            response += buffer;
        }

        close(sock);

        // Extract JSON from HTTP response
        size_t json_start = response.find("\r\n\r\n");
        if (json_start != std::string::npos) {
            return response.substr(json_start + 4);
        }

        return response;
    }

private:
    std::string host_;
    uint16_t port_;
    std::string username_;
    std::string password_;

    // Simple base64 encoding (minimal implementation)
    std::string Base64Encode(const std::string& input) {
        static const char* base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string output;
        int val = 0;
        int valb = -6;

        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                output.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (output.size() % 4) {
            output.push_back('=');
        }

        return output;
    }
};

// Format JSON for pretty printing (simple version)
std::string FormatJSON(const std::string& json) {
    std::string result;
    int indent = 0;
    bool in_string = false;

    for (size_t i = 0; i < json.length(); i++) {
        char c = json[i];

        if (c == '"' && (i == 0 || json[i-1] != '\\')) {
            in_string = !in_string;
        }

        if (!in_string) {
            if (c == '{' || c == '[') {
                result += c;
                result += '\n';
                indent += 2;
                result.append(indent, ' ');
            }
            else if (c == '}' || c == ']') {
                result += '\n';
                indent -= 2;
                result.append(indent, ' ');
                result += c;
            }
            else if (c == ',') {
                result += c;
                result += '\n';
                result.append(indent, ' ');
            }
            else if (c == ':') {
                result += c;
                result += ' ';
            }
            else if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                result += c;
            }
        }
        else {
            result += c;
        }
    }

    return result;
}

int main(int argc, char* argv[]) {
    // Default configuration
    std::string host = "127.0.0.1";
    uint16_t port = 2211;
    std::string rpc_user;
    std::string rpc_password;

    // Parse arguments
    std::string method;
    std::vector<std::string> params;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            std::cout << "INTcoin RPC Client v" << INTCOIN_VERSION_MAJOR << "."
                      << INTCOIN_VERSION_MINOR << "." << INTCOIN_VERSION_PATCH << "\n\n";
            std::cout << "Usage: intcoin-cli [options] <command> [params...]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -rpcconnect=<ip>        RPC server IP (default: 127.0.0.1)\n";
            std::cout << "  -rpcport=<port>         RPC server port (default: 2211)\n";
            std::cout << "  -rpcuser=<user>         RPC username\n";
            std::cout << "  -rpcpassword=<pass>     RPC password\n";
            std::cout << "  -testnet                Connect to testnet (port 12211)\n";
            std::cout << "  -h, --help              Show this help\n\n";
            std::cout << "Common commands:\n";
            std::cout << "  getblockcount           Get current block height\n";
            std::cout << "  getbestblockhash        Get hash of best block\n";
            std::cout << "  getblockchaininfo       Get blockchain information\n";
            std::cout << "  getnetworkinfo          Get network information\n";
            std::cout << "  getpeerinfo             Get peer connection info\n";
            std::cout << "  getmempoolinfo          Get mempool information\n";
            std::cout << "  help                    List all available RPC commands\n\n";
            std::cout << "Examples:\n";
            std::cout << "  intcoin-cli getblockcount\n";
            std::cout << "  intcoin-cli getblock <blockhash>\n";
            std::cout << "  intcoin-cli -rpcuser=user -rpcpassword=pass getblockcount\n";
            return 0;
        }
        else if (arg.find("-rpcconnect=") == 0) {
            host = arg.substr(12);
        }
        else if (arg.find("-rpcport=") == 0) {
            port = std::stoi(arg.substr(9));
        }
        else if (arg.find("-rpcuser=") == 0) {
            rpc_user = arg.substr(9);
        }
        else if (arg.find("-rpcpassword=") == 0) {
            rpc_password = arg.substr(13);
        }
        else if (arg == "-testnet") {
            port = 12211;  // Default testnet RPC port
        }
        else if (method.empty()) {
            method = arg;
        }
        else {
            params.push_back(arg);
        }
    }

    if (method.empty()) {
        std::cerr << "Error: No command specified\n";
        std::cerr << "Use 'intcoin-cli --help' for usage information\n";
        return 1;
    }

    // Build params JSON array
    std::ostringstream params_json;
    params_json << "[";
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) params_json << ",";

        // Try to parse as number
        bool is_number = true;
        for (char c : params[i]) {
            if (!isdigit(c) && c != '.' && c != '-') {
                is_number = false;
                break;
            }
        }

        if (is_number) {
            params_json << params[i];
        } else {
            params_json << "\"" << params[i] << "\"";
        }
    }
    params_json << "]";

    // Create RPC client
    RPCClient client(host, port, rpc_user, rpc_password);

    // Send request
    std::string response = client.SendRequest(method, params_json.str());

    // Check for connection error
    if (response.find("Connection failed") != std::string::npos) {
        std::cerr << "Error: Cannot connect to intcoind at " << host << ":" << port << "\n";
        std::cerr << "Make sure intcoind is running with RPC enabled.\n";
        return 1;
    }

    // Print formatted response
    std::cout << FormatJSON(response) << "\n";

    return 0;
}
