/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Testnet Faucet Server Implementation
 */

#include "intcoin/faucet.h"
#include "intcoin/util.h"
#include <sstream>
#include <algorithm>

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
#include <fcntl.h>
#endif

namespace intcoin {
namespace faucet {

// ============================================================================
// Configuration
// ============================================================================

FaucetConfig::FaucetConfig()
    : drip_amount(1000000000)  // 10 INT (1 billion INTS)
    , ip_cooldown(3600)         // 1 hour
    , address_cooldown(86400)   // 24 hours
    , http_port(2215)           // Faucet HTTP port
    , bind_address("0.0.0.0")
    , max_queue_size(1000)
    , transaction_fee(1000)     // 0.00001 INT
    , enable_captcha(false)
    , captcha_secret("") {}

// ============================================================================
// Distribution Request
// ============================================================================

DistributionRequest::DistributionRequest()
    : timestamp(std::chrono::system_clock::now())
    , amount(0)
    , status(Status::PENDING) {}

DistributionRequest::DistributionRequest(const std::string& addr, const std::string& ip, uint64_t amt)
    : address(addr)
    , ip_address(ip)
    , timestamp(std::chrono::system_clock::now())
    , amount(amt)
    , status(Status::PENDING) {}

// ============================================================================
// Rate Limiter
// ============================================================================

RateLimiter::RateLimiter(uint32_t cooldown_seconds)
    : cooldown_seconds_(cooldown_seconds) {}

bool RateLimiter::IsAllowed(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = last_request_.find(key);
    if (it == last_request_.end()) {
        return true;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);

    return elapsed.count() >= static_cast<int64_t>(cooldown_seconds_);
}

void RateLimiter::RecordRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_request_[key] = std::chrono::system_clock::now();
}

uint32_t RateLimiter::GetSecondsUntilAllowed(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = last_request_.find(key);
    if (it == last_request_.end()) {
        return 0;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
    int64_t seconds_elapsed = elapsed.count();
    int64_t cooldown = static_cast<int64_t>(cooldown_seconds_);

    if (seconds_elapsed >= cooldown) {
        return 0;
    }

    return static_cast<uint32_t>(cooldown - seconds_elapsed);
}

void RateLimiter::CleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto it = last_request_.begin();

    while (it != last_request_.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
        if (elapsed.count() >= static_cast<int64_t>(cooldown_seconds_)) {
            it = last_request_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Faucet Statistics
// ============================================================================

FaucetStats::FaucetStats()
    : total_distributions(0)
    , total_amount(0)
    , pending_requests(0)
    , failed_requests(0)
    , rate_limited_requests(0)
    , faucet_balance(0)
    , uptime(0)
    , last_distribution(std::chrono::system_clock::now()) {}

// ============================================================================
// Faucet Server
// ============================================================================

FaucetServer::FaucetServer(wallet::Wallet* wallet, Blockchain* blockchain, const FaucetConfig& config)
    : wallet_(wallet)
    , blockchain_(blockchain)
    , config_(config)
    , ip_limiter_(config.ip_cooldown)
    , address_limiter_(config.address_cooldown)
    , running_(false)
    , start_time_(std::chrono::system_clock::now())
    , http_socket_(-1) {}

FaucetServer::~FaucetServer() {
    Stop();
}

Result<void> FaucetServer::Start() {
    if (running_) {
        return Result<void>::Error("Faucet server already running");
    }

    if (!wallet_) {
        return Result<void>::Error("Wallet not initialized");
    }

    if (!blockchain_) {
        return Result<void>::Error("Blockchain not initialized");
    }

    // Create HTTP server socket
    http_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (http_socket_ < 0) {
        return Result<void>::Error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    setsockopt(http_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.http_port);
    addr.sin_addr.s_addr = inet_addr(config_.bind_address.c_str());

    if (bind(http_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(http_socket_);
        http_socket_ = -1;
        return Result<void>::Error("Failed to bind to port " + std::to_string(config_.http_port));
    }

    // Listen for connections
    if (listen(http_socket_, 10) < 0) {
        close(http_socket_);
        http_socket_ = -1;
        return Result<void>::Error("Failed to listen on socket");
    }

    running_ = true;
    start_time_ = std::chrono::system_clock::now();

    // Start threads
    http_thread_ = std::thread(&FaucetServer::HttpServerThread, this);
    processor_thread_ = std::thread(&FaucetServer::ProcessRequests, this);

    return Result<void>::Ok();
}

void FaucetServer::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Close socket
    if (http_socket_ >= 0) {
        close(http_socket_);
        http_socket_ = -1;
    }

    // Join threads
    if (http_thread_.joinable()) {
        http_thread_.join();
    }
    if (processor_thread_.joinable()) {
        processor_thread_.join();
    }
}

Result<std::string> FaucetServer::SubmitRequest(const std::string& address, const std::string& ip_address) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate address
    if (!ValidateAddress(address)) {
        return Result<std::string>::Error("Invalid address");
    }

    // Check IP rate limit
    if (!ip_limiter_.IsAllowed(ip_address)) {
        uint32_t seconds = ip_limiter_.GetSecondsUntilAllowed(ip_address);
        stats_.rate_limited_requests++;
        return Result<std::string>::Error("IP rate limited. Try again in " + std::to_string(seconds) + " seconds");
    }

    // Check address rate limit
    if (!address_limiter_.IsAllowed(address)) {
        uint32_t seconds = address_limiter_.GetSecondsUntilAllowed(address);
        stats_.rate_limited_requests++;
        return Result<std::string>::Error("Address rate limited. Try again in " + std::to_string(seconds) + " seconds");
    }

    // Check queue size
    if (pending_requests_.size() >= config_.max_queue_size) {
        return Result<std::string>::Error("Faucet queue full. Please try again later");
    }

    // Create distribution request
    DistributionRequest request(address, ip_address, config_.drip_amount);
    pending_requests_.push_back(request);

    // Record rate limits
    ip_limiter_.RecordRequest(ip_address);
    address_limiter_.RecordRequest(address);

    stats_.pending_requests = pending_requests_.size();

    return Result<std::string>::Ok("Request queued successfully");
}

Result<DistributionRequest> FaucetServer::GetRequestStatus(const std::string& txid) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Search completed requests
    for (const auto& req : completed_requests_) {
        if (Uint256ToHex(req.txid) == txid) {
            return Result<DistributionRequest>::Ok(req);
        }
    }

    return Result<DistributionRequest>::Error("Request not found");
}

std::vector<DistributionRequest> FaucetServer::GetRecentDistributions(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<DistributionRequest> recent;
    size_t start = completed_requests_.size() > count ? completed_requests_.size() - count : 0;

    for (size_t i = start; i < completed_requests_.size(); ++i) {
        recent.push_back(completed_requests_[i]);
    }

    std::reverse(recent.begin(), recent.end());
    return recent;
}

FaucetStats FaucetServer::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    FaucetStats stats = stats_;
    stats.pending_requests = pending_requests_.size();

    auto balance_result = wallet_->GetBalance();
    if (balance_result.IsOk()) {
        stats.faucet_balance = balance_result.GetValue();
    } else {
        stats.faucet_balance = 0;
    }

    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    stats.uptime = uptime.count();

    return stats;
}

void FaucetServer::UpdateConfig(const FaucetConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void FaucetServer::ProcessRequests() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::lock_guard<std::mutex> lock(mutex_);

        if (pending_requests_.empty()) {
            continue;
        }

        // Process first pending request
        DistributionRequest& request = pending_requests_.front();
        request.status = DistributionRequest::Status::PROCESSING;

        // Send distribution
        auto result = SendDistribution(request.address, request.amount);

        if (result.IsOk()) {
            request.txid = result.GetValue();
            request.status = DistributionRequest::Status::COMPLETED;
            stats_.total_distributions++;
            stats_.total_amount += request.amount;
            stats_.last_distribution = std::chrono::system_clock::now();
        } else {
            request.status = DistributionRequest::Status::FAILED;
            request.error = result.error;
            stats_.failed_requests++;
        }

        // Move to completed
        completed_requests_.push_back(request);
        pending_requests_.erase(pending_requests_.begin());

        // Keep only last 1000 completed requests
        if (completed_requests_.size() > 1000) {
            completed_requests_.erase(completed_requests_.begin());
        }

        stats_.pending_requests = pending_requests_.size();
    }
}

void FaucetServer::HttpServerThread() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(http_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Get client IP
        std::string client_ip = inet_ntoa(client_addr.sin_addr);

        // Read HTTP request
        char buffer[4096];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request_str(buffer);

            // Parse HTTP request
            std::istringstream iss(request_str);
            std::string method, path, version;
            iss >> method >> path >> version;

            // Extract body (for POST requests)
            std::string body;
            size_t body_start = request_str.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                body = request_str.substr(body_start + 4);
            }

            // Handle request
            std::string response = HandleRequest(method, path, body, client_ip);

            // Send response
            send(client_socket, response.c_str(), response.length(), 0);
        }

        close(client_socket);
    }
}

std::string FaucetServer::HandleRequest(const std::string& method, const std::string& path,
                                       const std::string& body, const std::string& client_ip) {
    // Handle GET requests (serve HTML page)
    if (method == "GET" && path == "/") {
        std::string html = GenerateHtmlPage();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << html.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << html;
        return response.str();
    }

    // Handle POST requests (submit address)
    if (method == "POST" && path == "/request") {
        // Parse address from body
        std::string address;
        size_t addr_start = body.find("address=");
        if (addr_start != std::string::npos) {
            size_t addr_end = body.find('&', addr_start);
            if (addr_end == std::string::npos) {
                addr_end = body.length();
            }
            address = body.substr(addr_start + 8, addr_end - addr_start - 8);
        }

        // Submit request
        auto result = SubmitRequest(address, client_ip);

        std::string json;
        if (result.IsOk()) {
            json = GenerateJsonResponse("success", result.GetValue());
        } else {
            json = GenerateJsonResponse("error", result.error);
        }

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << json;
        return response.str();
    }

    // Handle stats request
    if (method == "GET" && path == "/stats") {
        FaucetStats stats = GetStats();
        std::ostringstream json;
        json << "{"
             << "\"total_distributions\":" << stats.total_distributions << ","
             << "\"total_amount\":" << stats.total_amount << ","
             << "\"pending_requests\":" << stats.pending_requests << ","
             << "\"failed_requests\":" << stats.failed_requests << ","
             << "\"rate_limited\":" << stats.rate_limited_requests << ","
             << "\"faucet_balance\":" << stats.faucet_balance << ","
             << "\"uptime\":" << stats.uptime
             << "}";

        std::string json_str = json.str();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json_str.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << json_str;
        return response.str();
    }

    // 404 Not Found
    std::string not_found = "404 Not Found";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n";
    response << "Content-Length: " << not_found.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << not_found;
    return response.str();
}

std::string FaucetServer::GenerateHtmlPage() {
    FaucetStats stats = GetStats();

    // Calculate uptime display
    uint64_t hours = stats.uptime / 3600;
    uint64_t minutes = (stats.uptime % 3600) / 60;

    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>INTcoin Testnet Faucet - Free INT Testnet Coins</title>\n";
    html << "    <meta name=\"description\" content=\"Get free INTcoin testnet coins for development and testing purposes. Fast, reliable, and easy to use.\">\n";
    html << "    <style>\n";
    html << "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html << "        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }\n";
    html << "        .container { max-width: 700px; margin: 0 auto; }\n";
    html << "        .header { text-align: center; color: white; margin-bottom: 30px; }\n";
    html << "        .header h1 { font-size: 2.5em; margin-bottom: 10px; text-shadow: 0 2px 4px rgba(0,0,0,0.2); }\n";
    html << "        .header p { font-size: 1.1em; opacity: 0.9; }\n";
    html << "        .card { background: white; border-radius: 16px; padding: 30px; box-shadow: 0 20px 60px rgba(0,0,0,0.3); margin-bottom: 20px; }\n";
    html << "        .amount-badge { display: inline-block; background: #10b981; color: white; padding: 8px 16px; border-radius: 20px; font-weight: bold; font-size: 1.1em; margin: 15px 0; }\n";
    html << "        .form-group { margin: 20px 0; }\n";
    html << "        label { display: block; margin-bottom: 8px; font-weight: 600; color: #374151; }\n";
    html << "        input { width: 100%; padding: 14px; border: 2px solid #e5e7eb; border-radius: 8px; font-size: 1em; transition: border-color 0.3s; }\n";
    html << "        input:focus { outline: none; border-color: #667eea; }\n";
    html << "        .btn { width: 100%; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 16px; border: none; border-radius: 8px; font-size: 1.1em; font-weight: 600; cursor: pointer; transition: transform 0.2s, box-shadow 0.2s; }\n";
    html << "        .btn:hover { transform: translateY(-2px); box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4); }\n";
    html << "        .btn:active { transform: translateY(0); }\n";
    html << "        .btn:disabled { background: #9ca3af; cursor: not-allowed; transform: none; }\n";
    html << "        .message { padding: 15px; margin: 15px 0; border-radius: 8px; font-weight: 500; display: none; animation: slideIn 0.3s; }\n";
    html << "        @keyframes slideIn { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }\n";
    html << "        .success { background: #d1fae5; color: #065f46; border-left: 4px solid #10b981; display: block; }\n";
    html << "        .error { background: #fee2e2; color: #991b1b; border-left: 4px solid #ef4444; display: block; }\n";
    html << "        .info { background: #dbeafe; color: #1e40af; border-left: 4px solid #3b82f6; display: block; }\n";
    html << "        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; margin-top: 20px; }\n";
    html << "        .stat-box { background: #f9fafb; padding: 15px; border-radius: 8px; text-align: center; }\n";
    html << "        .stat-box .value { font-size: 1.8em; font-weight: bold; color: #667eea; }\n";
    html << "        .stat-box .label { font-size: 0.9em; color: #6b7280; margin-top: 5px; }\n";
    html << "        .recent-distributions { margin-top: 15px; max-height: 200px; overflow-y: auto; }\n";
    html << "        .distribution-item { background: #f9fafb; padding: 10px; margin: 5px 0; border-radius: 6px; font-size: 0.9em; display: flex; justify-content: space-between; }\n";
    html << "        .distribution-item .address { font-family: monospace; color: #667eea; flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }\n";
    html << "        .distribution-item .amount { font-weight: 600; color: #10b981; margin-left: 10px; }\n";
    html << "        .footer { text-align: center; color: white; margin-top: 30px; opacity: 0.8; }\n";
    html << "        .rules { background: #fffbeb; border: 1px solid #fbbf24; border-radius: 8px; padding: 15px; margin: 15px 0; }\n";
    html << "        .rules h3 { color: #92400e; margin-bottom: 10px; font-size: 1.1em; }\n";
    html << "        .rules ul { margin-left: 20px; color: #78350f; }\n";
    html << "        .rules li { margin: 5px 0; }\n";
    html << "        .spinner { border: 3px solid #f3f4f6; border-top: 3px solid #667eea; border-radius: 50%; width: 20px; height: 20px; animation: spin 0.6s linear infinite; display: inline-block; vertical-align: middle; margin-left: 10px; }\n";
    html << "        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"container\">\n";
    html << "        <div class=\"header\">\n";
    html << "            <h1>⚡ INTcoin Testnet Faucet</h1>\n";
    html << "            <p>Get free testnet coins for development and testing</p>\n";
    html << "        </div>\n";
    html << "        \n";
    html << "        <div class=\"card\">\n";
    html << "            <div style=\"text-align: center;\">\n";
    html << "                <div class=\"amount-badge\">🪙 " << (config_.drip_amount / 100000000.0) << " INT per request</div>\n";
    html << "            </div>\n";
    html << "            \n";
    html << "            <div class=\"rules\">\n";
    html << "                <h3>📋 Faucet Rules</h3>\n";
    html << "                <ul>\n";
    html << "                    <li>One request per IP every " << (config_.ip_cooldown / 3600) << " hour(s)</li>\n";
    html << "                    <li>One request per address every " << (config_.address_cooldown / 86400) << " day(s)</li>\n";
    html << "                    <li>Only valid testnet addresses (starting with 'tint1')</li>\n";
    html << "                    <li>Testnet coins have no real value</li>\n";
    html << "                </ul>\n";
    html << "            </div>\n";
    html << "            \n";
    html << "            <form id=\"faucetForm\">\n";
    html << "                <div class=\"form-group\">\n";
    html << "                    <label for=\"address\">Your Testnet Address</label>\n";
    html << "                    <input type=\"text\" id=\"address\" placeholder=\"tint1...\" required pattern=\"tint1[a-z0-9]{39,90}\" title=\"Please enter a valid testnet address starting with tint1\">\n";
    html << "                </div>\n";
    html << "                <button type=\"submit\" class=\"btn\" id=\"submitBtn\">\n";
    html << "                    <span id=\"btnText\">🚀 Request Testnet Coins</span>\n";
    html << "                </button>\n";
    html << "            </form>\n";
    html << "            \n";
    html << "            <div id=\"message\"></div>\n";
    html << "        </div>\n";
    html << "        \n";
    html << "        <div class=\"card\">\n";
    html << "            <h2 style=\"margin-bottom: 15px; color: #374151;\">📊 Faucet Statistics</h2>\n";
    html << "            <div class=\"stats-grid\">\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << stats.total_distributions << "</div>\n";
    html << "                    <div class=\"label\">Total Drips</div>\n";
    html << "                </div>\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << (stats.total_amount / 100000000.0) << "</div>\n";
    html << "                    <div class=\"label\">INT Distributed</div>\n";
    html << "                </div>\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << stats.pending_requests << "</div>\n";
    html << "                    <div class=\"label\">Pending</div>\n";
    html << "                </div>\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << (stats.faucet_balance / 100000000.0) << "</div>\n";
    html << "                    <div class=\"label\">Balance (INT)</div>\n";
    html << "                </div>\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << hours << "h " << minutes << "m</div>\n";
    html << "                    <div class=\"label\">Uptime</div>\n";
    html << "                </div>\n";
    html << "                <div class=\"stat-box\">\n";
    html << "                    <div class=\"value\">" << stats.rate_limited_requests << "</div>\n";
    html << "                    <div class=\"label\">Rate Limited</div>\n";
    html << "                </div>\n";
    html << "            </div>\n";
    html << "        </div>\n";
    html << "        \n";
    html << "        <div class=\"footer\">\n";
    html << "            <p>💜 Built with love for the INTcoin community</p>\n";
    html << "            <p style=\"margin-top: 10px; font-size: 0.9em;\">Visit <a href=\"http://international-coin.org\" style=\"color: white; text-decoration: underline;\">international-coin.org</a></p>\n";
    html << "            <div style=\"margin-top: 15px; font-size: 0.9em;\">\n";
    html << "                <a href=\"https://x.com/INTcoin_team\" target=\"_blank\" style=\"color: white; margin: 0 10px; text-decoration: none;\">🐦 Twitter</a> | \n";
    html << "                <a href=\"https://www.reddit.com/r/INTcoin\" target=\"_blank\" style=\"color: white; margin: 0 10px; text-decoration: none;\">📱 Reddit</a> | \n";
    html << "                <a href=\"https://discord.gg/jCy3eNgx\" target=\"_blank\" style=\"color: white; margin: 0 10px; text-decoration: none;\">💬 Discord</a>\n";
    html << "            </div>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    \n";
    html << "    <script>\n";
    html << "        document.getElementById('faucetForm').addEventListener('submit', function(e) {\n";
    html << "            e.preventDefault();\n";
    html << "            \n";
    html << "            var address = document.getElementById('address').value.trim();\n";
    html << "            var messageDiv = document.getElementById('message');\n";
    html << "            var submitBtn = document.getElementById('submitBtn');\n";
    html << "            var btnText = document.getElementById('btnText');\n";
    html << "            \n";
    html << "            // Validate address format\n";
    html << "            if (!address.startsWith('tint1') || address.length < 42) {\n";
    html << "                messageDiv.className = 'message error';\n";
    html << "                messageDiv.textContent = '❌ Invalid testnet address. Must start with \\'tint1\\' and be at least 42 characters long.';\n";
    html << "                return;\n";
    html << "            }\n";
    html << "            \n";
    html << "            // Disable button and show loading\n";
    html << "            submitBtn.disabled = true;\n";
    html << "            btnText.innerHTML = 'Processing... <span class=\"spinner\"></span>';\n";
    html << "            messageDiv.style.display = 'none';\n";
    html << "            \n";
    html << "            fetch('/request', {\n";
    html << "                method: 'POST',\n";
    html << "                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },\n";
    html << "                body: 'address=' + encodeURIComponent(address)\n";
    html << "            })\n";
    html << "            .then(response => response.json())\n";
    html << "            .then(data => {\n";
    html << "                messageDiv.className = 'message ' + data.status;\n";
    html << "                if (data.status === 'success') {\n";
    html << "                    messageDiv.textContent = '✅ ' + data.message;\n";
    html << "                    document.getElementById('address').value = '';\n";
    html << "                    // Refresh stats after 2 seconds\n";
    html << "                    setTimeout(() => location.reload(), 2000);\n";
    html << "                } else {\n";
    html << "                    messageDiv.textContent = '❌ ' + data.message;\n";
    html << "                }\n";
    html << "                submitBtn.disabled = false;\n";
    html << "                btnText.innerHTML = '🚀 Request Testnet Coins';\n";
    html << "            })\n";
    html << "            .catch(error => {\n";
    html << "                messageDiv.className = 'message error';\n";
    html << "                messageDiv.textContent = '❌ Network error. Please try again later.';\n";
    html << "                submitBtn.disabled = false;\n";
    html << "                btnText.innerHTML = '🚀 Request Testnet Coins';\n";
    html << "            });\n";
    html << "        });\n";
    html << "        \n";
    html << "        // Auto-refresh stats every 30 seconds\n";
    html << "        setInterval(() => {\n";
    html << "            if (!document.getElementById('submitBtn').disabled) {\n";
    html << "                location.reload();\n";
    html << "            }\n";
    html << "        }, 30000);\n";
    html << "    </script>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string FaucetServer::GenerateJsonResponse(const std::string& status, const std::string& message,
                                               const std::string& txid) {
    std::ostringstream json;
    json << "{\"status\":\"" << status << "\",\"message\":\"" << message << "\"";
    if (!txid.empty()) {
        json << ",\"txid\":\"" << txid << "\"";
    }
    json << "}";
    return json.str();
}

bool FaucetServer::ValidateAddress(const std::string& address) {
    // Basic validation - address should start with "tint1" for testnet and be proper length
    if (address.length() < 42 || address.substr(0, 5) != "tint1") {
        return false;
    }
    return true;
}

Result<uint256> FaucetServer::SendDistribution(const std::string& address, uint64_t amount) {
    if (!wallet_ || !blockchain_) {
        return Result<uint256>::Error("Wallet or blockchain not initialized");
    }

    // Create recipient
    std::vector<wallet::Wallet::Recipient> recipients;
    wallet::Wallet::Recipient recipient;
    recipient.address = address;
    recipient.amount = amount;
    recipients.push_back(recipient);

    // Create transaction
    auto tx_result = wallet_->CreateTransaction(recipients, config_.transaction_fee);
    if (!tx_result.IsOk()) {
        return Result<uint256>::Error("Failed to create transaction: " + tx_result.error);
    }

    Transaction tx = tx_result.GetValue();

    // Sign transaction
    auto sign_result = wallet_->SignTransaction(tx);
    if (!sign_result.IsOk()) {
        return Result<uint256>::Error("Failed to sign transaction: " + sign_result.error);
    }

    Transaction signed_tx = sign_result.GetValue();

    // Broadcast transaction (would need to add to mempool/broadcast)
    // For now, just return the transaction ID
    uint256 txid = signed_tx.GetHash();

    return Result<uint256>::Ok(txid);
}

} // namespace faucet
} // namespace intcoin
