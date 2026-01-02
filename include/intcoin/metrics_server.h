// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_METRICS_SERVER_H
#define INTCOIN_METRICS_SERVER_H

#include <intcoin/types.h>

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace intcoin {

/// Configuration for metrics HTTP server
struct MetricsServerConfig {
    /// Bind address (default: "127.0.0.1")
    std::string bind_address = "127.0.0.1";

    /// Port to listen on (default: 9090)
    uint16_t port = 9090;

    /// Enable/disable server
    bool enabled = true;

    /// Number of worker threads (default: 2)
    uint32_t num_threads = 2;
};

/// HTTP-based Prometheus metrics server
/// Serves metrics at GET /metrics endpoint
class MetricsServer {
public:
    MetricsServer();
    ~MetricsServer();

    /// Initialize and start the metrics server
    /// @param config Server configuration
    /// @return Result indicating success or error
    Result<void> Start(const MetricsServerConfig& config);

    /// Stop the metrics server
    /// @return Result indicating success or error
    Result<void> Stop();

    /// Check if server is running
    /// @return True if server is running
    bool IsRunning() const;

    /// Get server configuration
    /// @return Current server config
    MetricsServerConfig GetConfig() const;

    /// Get number of requests served
    /// @return Total request count
    uint64_t GetRequestCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace intcoin

#endif // INTCOIN_METRICS_SERVER_H
