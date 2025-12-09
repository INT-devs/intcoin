/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Testnet Faucet Server
 */

#ifndef INTCOIN_FAUCET_H
#define INTCOIN_FAUCET_H

#include "intcoin/types.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <thread>

namespace intcoin {
namespace faucet {

/// Faucet configuration
struct FaucetConfig {
    /// Amount to send per request (in satoshis)
    uint64_t drip_amount;

    /// Cooldown period per IP address (seconds)
    uint32_t ip_cooldown;

    /// Cooldown period per address (seconds)
    uint32_t address_cooldown;

    /// HTTP server port
    uint16_t http_port;

    /// Bind address
    std::string bind_address;

    /// Maximum queue size
    size_t max_queue_size;

    /// Transaction fee (in satoshis)
    uint64_t transaction_fee;

    /// Enable CAPTCHA verification
    bool enable_captcha;

    /// CAPTCHA secret key
    std::string captcha_secret;

    FaucetConfig();
};

/// Distribution request
struct DistributionRequest {
    /// Recipient address
    std::string address;

    /// Requester IP address
    std::string ip_address;

    /// Request timestamp
    std::chrono::system_clock::time_point timestamp;

    /// Amount requested
    uint64_t amount;

    /// Transaction ID (if processed)
    uint256 txid;

    /// Status
    enum class Status {
        PENDING,
        PROCESSING,
        COMPLETED,
        FAILED,
        RATE_LIMITED
    };
    Status status;

    /// Error message (if failed)
    std::string error;

    DistributionRequest();
    DistributionRequest(const std::string& addr, const std::string& ip, uint64_t amt);
};

/// Rate limit tracker
class RateLimiter {
public:
    RateLimiter(uint32_t cooldown_seconds);

    /// Check if request is allowed
    bool IsAllowed(const std::string& key);

    /// Record a request
    void RecordRequest(const std::string& key);

    /// Get time until next allowed request
    uint32_t GetSecondsUntilAllowed(const std::string& key) const;

    /// Clear expired entries
    void CleanupExpired();

private:
    uint32_t cooldown_seconds_;
    std::map<std::string, std::chrono::system_clock::time_point> last_request_;
    mutable std::mutex mutex_;
};

/// Faucet statistics
struct FaucetStats {
    /// Total distributions
    uint64_t total_distributions;

    /// Total amount distributed
    uint64_t total_amount;

    /// Pending requests
    size_t pending_requests;

    /// Failed requests
    uint64_t failed_requests;

    /// Rate limited requests
    uint64_t rate_limited_requests;

    /// Faucet balance
    uint64_t faucet_balance;

    /// Uptime (seconds)
    uint64_t uptime;

    /// Last distribution time
    std::chrono::system_clock::time_point last_distribution;

    FaucetStats();
};

/// Testnet Faucet Server
class FaucetServer {
public:
    FaucetServer(wallet::Wallet* wallet, Blockchain* blockchain, const FaucetConfig& config);
    ~FaucetServer();

    /// Start the faucet server
    Result<void> Start();

    /// Stop the faucet server
    void Stop();

    /// Submit a distribution request
    Result<std::string> SubmitRequest(const std::string& address, const std::string& ip_address);

    /// Get request status
    Result<DistributionRequest> GetRequestStatus(const std::string& txid);

    /// Get recent distributions
    std::vector<DistributionRequest> GetRecentDistributions(size_t count = 10);

    /// Get statistics
    FaucetStats GetStats() const;

    /// Check if server is running
    bool IsRunning() const { return running_; }

    /// Get configuration
    const FaucetConfig& GetConfig() const { return config_; }

    /// Update configuration
    void UpdateConfig(const FaucetConfig& config);

private:
    /// Process pending requests
    void ProcessRequests();

    /// HTTP server thread
    void HttpServerThread();

    /// Handle HTTP request
    std::string HandleRequest(const std::string& method, const std::string& path,
                             const std::string& body, const std::string& client_ip);

    /// Generate HTML page
    std::string GenerateHtmlPage();

    /// Generate JSON response
    std::string GenerateJsonResponse(const std::string& status, const std::string& message,
                                     const std::string& txid = "");

    /// Validate address
    bool ValidateAddress(const std::string& address);

    /// Create and send distribution transaction
    Result<uint256> SendDistribution(const std::string& address, uint64_t amount);

    wallet::Wallet* wallet_;
    Blockchain* blockchain_;
    FaucetConfig config_;

    /// Rate limiters
    RateLimiter ip_limiter_;
    RateLimiter address_limiter_;

    /// Request queue
    std::vector<DistributionRequest> pending_requests_;
    std::vector<DistributionRequest> completed_requests_;

    /// Statistics
    FaucetStats stats_;

    /// Server state
    bool running_;
    std::chrono::system_clock::time_point start_time_;

    /// HTTP server socket
    int http_socket_;

    /// Worker threads
    std::thread http_thread_;
    std::thread processor_thread_;

    /// Thread synchronization
    mutable std::mutex mutex_;
};

} // namespace faucet
} // namespace intcoin

#endif // INTCOIN_FAUCET_H
