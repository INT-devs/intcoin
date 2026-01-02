// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/metrics_server.h>
#include <intcoin/metrics.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <sstream>
#include <thread>
#include <vector>

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace intcoin {

// Helper function to close socket cross-platform
static void CloseSocket(int sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

struct MetricsServer::Impl {
    MetricsServerConfig config;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> request_count{0};
    std::vector<std::thread> worker_threads;
    int server_socket{-1};

    ~Impl() {
        if (running.load()) {
            Stop();
        }
    }

    Result<void> Start() {
#ifdef _WIN32
        // Initialize Winsock
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            return Result<void>::Error("Failed to initialize Winsock");
        }
#endif

        // Create socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            return Result<void>::Error("Failed to create socket");
        }

        // Set socket options
        int opt = 1;
#ifdef _WIN32
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        // Bind socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(config.bind_address.c_str());
        address.sin_port = htons(config.port);

        if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            CloseSocket(server_socket);
            return Result<void>::Error("Failed to bind socket");
        }

        // Listen
        if (listen(server_socket, 10) < 0) {
            CloseSocket(server_socket);
            return Result<void>::Error("Failed to listen on socket");
        }

        running.store(true);

        // Start worker threads
        for (uint32_t i = 0; i < config.num_threads; ++i) {
            worker_threads.emplace_back([this]() { WorkerLoop(); });
        }

        return Result<void>::Ok();
    }

    Result<void> Stop() {
        if (!running.load()) {
            return Result<void>::Error("Server not running");
        }

        running.store(false);

        // Close server socket to wake up accept()
        if (server_socket >= 0) {
            CloseSocket(server_socket);
            server_socket = -1;
        }

        // Wait for worker threads
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads.clear();

#ifdef _WIN32
        WSACleanup();
#endif

        return Result<void>::Ok();
    }

    void WorkerLoop() {
        while (running.load()) {
            // Accept connection
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running.load()) {
                    continue;  // Error, but keep trying
                } else {
                    break;  // Server is shutting down
                }
            }

            // Handle request
            HandleRequest(client_socket);

            // Close connection
            CloseSocket(client_socket);
        }
    }

    void HandleRequest(int client_socket) {
        // Read HTTP request
        char buffer[4096];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            return;
        }

        buffer[bytes_read] = '\0';

        // Parse request line
        std::string request(buffer);
        size_t first_space = request.find(' ');
        size_t second_space = request.find(' ', first_space + 1);

        if (first_space == std::string::npos || second_space == std::string::npos) {
            SendError(client_socket, 400, "Bad Request");
            return;
        }

        std::string method = request.substr(0, first_space);
        std::string path = request.substr(first_space + 1, second_space - first_space - 1);

        // Only support GET /metrics
        if (method != "GET") {
            SendError(client_socket, 405, "Method Not Allowed");
            return;
        }

        if (path != "/metrics") {
            SendError(client_socket, 404, "Not Found");
            return;
        }

        // Get metrics from registry
        std::string metrics = MetricsRegistry::Instance().ExportPrometheus();

        // Increment request counter
        request_count.fetch_add(1);

        // Send HTTP response
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain; version=0.0.4\r\n";
        response << "Content-Length: " << metrics.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << metrics;

        std::string response_str = response.str();
        send(client_socket, response_str.c_str(), response_str.size(), 0);
    }

    void SendError(int client_socket, int status_code, const std::string& status_text) {
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << status_text.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << status_text;

        std::string response_str = response.str();
        send(client_socket, response_str.c_str(), response_str.size(), 0);
    }
};

// ============================================================================
// MetricsServer Implementation
// ============================================================================

MetricsServer::MetricsServer() : impl_(std::make_unique<Impl>()) {}

MetricsServer::~MetricsServer() = default;

Result<void> MetricsServer::Start(const MetricsServerConfig& config) {
    if (impl_->running.load()) {
        return Result<void>::Error("Server already running");
    }

    if (!config.enabled) {
        return Result<void>::Error("Server disabled in config");
    }

    impl_->config = config;
    return impl_->Start();
}

Result<void> MetricsServer::Stop() {
    return impl_->Stop();
}

bool MetricsServer::IsRunning() const {
    return impl_->running.load();
}

MetricsServerConfig MetricsServer::GetConfig() const {
    return impl_->config;
}

uint64_t MetricsServer::GetRequestCount() const {
    return impl_->request_count.load();
}

} // namespace intcoin
