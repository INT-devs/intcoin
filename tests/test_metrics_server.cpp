/**
 * @file test_metrics_server.cpp
 * @brief Metrics HTTP server test suite for INTcoin
 * @author INTcoin Core Developers
 * @date 2026-01-02
 * @version 1.2.0-beta
 */

#include "intcoin/metrics_server.h"
#include "intcoin/metrics.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <sstream>

// Platform-specific includes for HTTP client
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

using namespace intcoin;

// Helper function to close socket cross-platform
static void CloseSocket(int sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

// Helper function to make HTTP GET request
std::string HttpGet(const std::string& host, uint16_t port, const std::string& path) {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return "";
    }

    // Connect
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        CloseSocket(sock);
        return "";
    }

    // Send HTTP GET request
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";

    std::string request_str = request.str();
    send(sock, request_str.c_str(), request_str.size(), 0);

    // Read response
    std::string response;
    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes_read);
    }

    CloseSocket(sock);

#ifdef _WIN32
    WSACleanup();
#endif

    return response;
}

// Test 1: Server start/stop
void TestServerStartStop() {
    std::cout << "Test 1: Server Start/Stop..." << std::endl;

    MetricsServer server;
    assert(!server.IsRunning());

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19090;  // Use different port for testing
    config.num_threads = 2;

    auto result = server.Start(config);
    assert(result.IsOk());
    assert(server.IsRunning());

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto stop_result = server.Stop();
    assert(stop_result.IsOk());
    assert(!server.IsRunning());

    std::cout << "✓ Server start/stop working correctly" << std::endl;
}

// Test 2: HTTP GET /metrics
void TestMetricsEndpoint() {
    std::cout << "\nTest 2: HTTP GET /metrics..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19091;
    config.num_threads = 2;

    auto result = server.Start(config);
    assert(result.IsOk());

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Make HTTP request
    std::string response = HttpGet("127.0.0.1", 19091, "/metrics");

    // Verify response
    assert(!response.empty());
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("Content-Type: text/plain") != std::string::npos);
    assert(response.find("# HELP") != std::string::npos);
    assert(response.find("# TYPE") != std::string::npos);

    server.Stop();

    std::cout << "✓ Metrics endpoint returning valid data" << std::endl;
}

// Test 3: Metrics content validation
void TestMetricsContent() {
    std::cout << "\nTest 3: Metrics Content Validation..." << std::endl;

    // Set some metrics values
    using namespace metrics;
    blocks_processed.Inc();
    blockchain_height.Set(12345);
    mempool_size.Set(42);

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19092;
    config.num_threads = 2;

    server.Start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Make HTTP request
    std::string response = HttpGet("127.0.0.1", 19092, "/metrics");

    // Verify metrics are present
    assert(response.find("intcoin_blocks_processed_total") != std::string::npos);
    assert(response.find("intcoin_blockchain_height 12345") != std::string::npos);
    assert(response.find("intcoin_mempool_size 42") != std::string::npos);

    server.Stop();

    std::cout << "✓ Metrics content validated" << std::endl;
}

// Test 4: Invalid HTTP method
void TestInvalidMethod() {
    std::cout << "\nTest 4: Invalid HTTP Method..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19093;
    config.num_threads = 2;

    server.Start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Try POST request (should fail)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(19093);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    std::string request = "POST /metrics HTTP/1.1\r\n\r\n";
    send(sock, request.c_str(), request.size(), 0);

    char buffer[1024];
    recv(sock, buffer, sizeof(buffer), 0);
    std::string response(buffer);

    assert(response.find("405") != std::string::npos ||
           response.find("Method Not Allowed") != std::string::npos);

    CloseSocket(sock);
    server.Stop();

    std::cout << "✓ Invalid method rejected correctly" << std::endl;
}

// Test 5: Invalid path
void TestInvalidPath() {
    std::cout << "\nTest 5: Invalid Path..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19094;
    config.num_threads = 2;

    server.Start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Request invalid path
    std::string response = HttpGet("127.0.0.1", 19094, "/invalid");

    assert(response.find("404") != std::string::npos ||
           response.find("Not Found") != std::string::npos);

    server.Stop();

    std::cout << "✓ Invalid path rejected correctly" << std::endl;
}

// Test 6: Multiple concurrent requests
void TestConcurrentRequests() {
    std::cout << "\nTest 6: Concurrent Requests..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19095;
    config.num_threads = 4;

    server.Start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Make multiple concurrent requests
    std::vector<std::thread> threads;
    const int num_requests = 10;

    for (int i = 0; i < num_requests; ++i) {
        threads.emplace_back([&]() {
            std::string response = HttpGet("127.0.0.1", 19095, "/metrics");
            assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify request count
    assert(server.GetRequestCount() == num_requests);

    server.Stop();

    std::cout << "✓ Concurrent requests handled correctly" << std::endl;
    std::cout << "  - Requests served: " << num_requests << std::endl;
}

// Test 7: Server config
void TestServerConfig() {
    std::cout << "\nTest 7: Server Configuration..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19096;
    config.num_threads = 3;
    config.enabled = true;

    server.Start(config);

    MetricsServerConfig retrieved_config = server.GetConfig();
    assert(retrieved_config.bind_address == "127.0.0.1");
    assert(retrieved_config.port == 19096);
    assert(retrieved_config.num_threads == 3);
    assert(retrieved_config.enabled == true);

    server.Stop();

    std::cout << "✓ Server configuration working correctly" << std::endl;
}

// Test 8: Prometheus format validation
void TestPrometheusFormat() {
    std::cout << "\nTest 8: Prometheus Format Validation..." << std::endl;

    MetricsServer server;

    MetricsServerConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 19097;
    config.num_threads = 2;

    server.Start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string response = HttpGet("127.0.0.1", 19097, "/metrics");

    // Verify Prometheus format
    assert(response.find("Content-Type: text/plain; version=0.0.4") != std::string::npos);

    // Verify metric format
    assert(response.find("# HELP") != std::string::npos);
    assert(response.find("# TYPE") != std::string::npos);
    assert(response.find("counter") != std::string::npos ||
           response.find("gauge") != std::string::npos);

    server.Stop();

    std::cout << "✓ Prometheus format validated" << std::endl;
}

// Main test runner
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "INTcoin Metrics Server Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        TestServerStartStop();
        TestMetricsEndpoint();
        TestMetricsContent();
        TestInvalidMethod();
        TestInvalidPath();
        TestConcurrentRequests();
        TestServerConfig();
        TestPrometheusFormat();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All metrics server tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
