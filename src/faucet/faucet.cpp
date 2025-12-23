/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Testnet Faucet Server Implementation
 */

#include "intcoin/faucet.h"
#include "intcoin/util.h"
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

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

    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "    <title>INTcoin Testnet Faucet</title>\n";
    html << "    <style>\n";
    html << "        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html << "        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "        h1 { color: #6366f1; }\n";
    html << "        input { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; }\n";
    html << "        button { background: #6366f1; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }\n";
    html << "        button:hover { background: #4f46e5; }\n";
    html << "        .stats { margin-top: 20px; padding: 15px; background: #f9fafb; border-radius: 5px; }\n";
    html << "        .message { padding: 10px; margin: 10px 0; border-radius: 5px; }\n";
    html << "        .success { background: #d1fae5; color: #065f46; }\n";
    html << "        .error { background: #fee2e2; color: #991b1b; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"container\">\n";
    html << "        <h1>INTcoin Testnet Faucet</h1>\n";
    html << "        <p>Get free testnet INT coins for testing purposes.</p>\n";
    html << "        <p>Each request gives you <strong>" << (config_.drip_amount / 100000000.0) << " INT</strong></p>\n";
    html << "        <form id=\"faucetForm\">\n";
    html << "            <input type=\"text\" id=\"address\" placeholder=\"Enter your INT testnet address\" required>\n";
    html << "            <button type=\"submit\">Request Testnet Coins</button>\n";
    html << "        </form>\n";
    html << "        <div id=\"message\"></div>\n";
    html << "        <div class=\"stats\">\n";
    html << "            <h3>Statistics</h3>\n";
    html << "            <p>Total Distributions: <strong>" << stats.total_distributions << "</strong></p>\n";
    html << "            <p>Total Amount Distributed: <strong>" << (stats.total_amount / 100000000.0) << " INT</strong></p>\n";
    html << "            <p>Pending Requests: <strong>" << stats.pending_requests << "</strong></p>\n";
    html << "            <p>Faucet Balance: <strong>" << (stats.faucet_balance / 100000000.0) << " INT</strong></p>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    <script>\n";
    html << "        document.getElementById('faucetForm').addEventListener('submit', function(e) {\n";
    html << "            e.preventDefault();\n";
    html << "            var address = document.getElementById('address').value;\n";
    html << "            fetch('/request', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'address=' + encodeURIComponent(address) })\n";
    html << "                .then(response => response.json())\n";
    html << "                .then(data => {\n";
    html << "                    var messageDiv = document.getElementById('message');\n";
    html << "                    messageDiv.className = 'message ' + data.status;\n";
    html << "                    messageDiv.textContent = data.message;\n";
    html << "                    if (data.status === 'success') document.getElementById('address').value = '';\n";
    html << "                });\n";
    html << "        });\n";
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
    // Basic validation - address should start with "int1" and be proper length
    if (address.length() < 42 || address.substr(0, 4) != "int1") {
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
