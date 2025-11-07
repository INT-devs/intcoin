// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/pool/stratum.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

namespace intcoin {
namespace pool {

// StratumClient implementation

StratumClient::StratumClient(const std::string& pool_url, uint16_t port,
                             const std::string& username, const std::string& password)
    : pool_url_(pool_url), port_(port), username_(username), password_(password),
      connected_(false), socket_fd_(-1), current_difficulty_(1.0), extranonce2_size_(4) {
}

StratumClient::~StratumClient() {
    disconnect();
}

bool StratumClient::connect() {
    if (connected_) {
        return true;
    }

    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Resolve hostname
    struct hostent* host = gethostbyname(pool_url_.c_str());
    if (!host) {
        std::cerr << "Failed to resolve pool hostname: " << pool_url_ << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Connect to pool
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to pool at " << pool_url_ << ":" << port_ << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    std::cout << "Connected to pool " << pool_url_ << ":" << port_ << std::endl;

    // Send subscribe message
    send_stratum_message(subscribe_message());
    json subscribe_response = receive_stratum_message();

    if (subscribe_response.is_null() || !subscribe_response.contains("result")) {
        std::cerr << "Failed to subscribe to pool" << std::endl;
        disconnect();
        return false;
    }

    // Parse extranonce
    auto result = subscribe_response["result"];
    if (result.is_array() && result.size() >= 2) {
        extranonce1_ = result[1].get<std::string>();
        if (result.size() >= 3) {
            extranonce2_size_ = result[2].get<size_t>();
        }
    }

    // Send authorize message
    send_stratum_message(authorize_message());
    json auth_response = receive_stratum_message();

    if (auth_response.is_null() || !auth_response["result"].get<bool>()) {
        std::cerr << "Failed to authorize with pool" << std::endl;
        disconnect();
        return false;
    }

    std::cout << "Authorized as " << username_ << std::endl;

    connected_ = true;

    // Start receive loop in separate thread
    std::thread([this]() {
        receive_loop();
    }).detach();

    return true;
}

void StratumClient::disconnect() {
    if (!connected_) {
        return;
    }

    connected_ = false;
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    std::cout << "Disconnected from pool" << std::endl;
}

void StratumClient::receive_loop() {
    while (connected_) {
        try {
            json message = receive_stratum_message();
            if (message.is_null()) {
                break;
            }

            // Handle different message types
            if (message.contains("method")) {
                std::string method = message["method"].get<std::string>();

                if (method == "mining.notify") {
                    handle_notify(message["params"]);
                } else if (method == "mining.set_difficulty") {
                    handle_set_difficulty(message["params"]);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Receive loop error: " << e.what() << std::endl;
            break;
        }
    }

    disconnect();
}

void StratumClient::send_stratum_message(const json& message) {
    if (socket_fd_ < 0) {
        return;
    }

    std::string msg_str = message.dump() + "\n";
    send(socket_fd_, msg_str.c_str(), msg_str.length(), 0);
}

json StratumClient::receive_stratum_message() {
    if (socket_fd_ < 0) {
        return json();
    }

    char buffer[4096];
    ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        return json();
    }

    buffer[bytes_read] = '\0';
    try {
        return json::parse(buffer);
    } catch (...) {
        return json();
    }
}

void StratumClient::handle_notify(const json& params) {
    if (params.size() < 9) {
        return;
    }

    std::lock_guard<std::mutex> lock(job_mutex_);

    try {
        current_job_.job_id = params[0].get<std::string>();
        current_job_.prev_block_hash = Hash256::from_string(params[1].get<std::string>());
        current_job_.coinbase1 = params[2].get<std::string>();
        current_job_.coinbase2 = params[3].get<std::string>();

        // Parse merkle branch
        current_job_.merkle_branch.clear();
        if (params[4].is_array()) {
            for (const auto& hash_str : params[4]) {
                current_job_.merkle_branch.push_back(
                    Hash256::from_string(hash_str.get<std::string>())
                );
            }
        }

        current_job_.version = std::stoul(params[5].get<std::string>(), nullptr, 16);
        current_job_.bits = std::stoul(params[6].get<std::string>(), nullptr, 16);
        current_job_.timestamp = std::stoull(params[7].get<std::string>(), nullptr, 16);
        current_job_.clean_jobs = params[8].get<bool>();

        std::cout << "Received new job: " << current_job_.job_id << std::endl;

        // Notify callback
        if (job_callback_) {
            job_callback_(current_job_);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing job: " << e.what() << std::endl;
    }
}

void StratumClient::handle_set_difficulty(const json& params) {
    if (params.empty()) {
        return;
    }

    try {
        double new_diff = params[0].get<double>();

        {
            std::lock_guard<std::mutex> lock(job_mutex_);
            current_difficulty_ = new_diff;
        }

        std::cout << "Difficulty set to " << new_diff << std::endl;

        // Notify callback
        if (difficulty_callback_) {
            difficulty_callback_(new_diff);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing difficulty: " << e.what() << std::endl;
    }
}

json StratumClient::subscribe_message() const {
    json msg;
    msg["id"] = 1;
    msg["method"] = "mining.subscribe";
    msg["params"] = json::array();
    return msg;
}

json StratumClient::authorize_message() const {
    json msg;
    msg["id"] = 2;
    msg["method"] = "mining.authorize";
    msg["params"] = json::array({username_, password_});
    return msg;
}

json StratumClient::submit_message(const MiningShare& share) const {
    json msg;
    msg["id"] = 4;
    msg["method"] = "mining.submit";

    // Format parameters as hex strings
    char nonce_hex[17];
    char time_hex[17];
    snprintf(nonce_hex, sizeof(nonce_hex), "%016lx", share.nonce);
    snprintf(time_hex, sizeof(time_hex), "%016lx", share.timestamp);

    msg["params"] = json::array({
        username_,
        share.job_id,
        share.extranonce2,
        time_hex,
        nonce_hex
    });

    return msg;
}

MiningJob StratumClient::get_current_job() const {
    std::lock_guard<std::mutex> lock(job_mutex_);
    return current_job_;
}

bool StratumClient::submit_share(const MiningShare& share) {
    if (!connected_) {
        return false;
    }

    send_stratum_message(submit_message(share));

    // Wait for response
    json response = receive_stratum_message();
    if (response.is_null()) {
        return false;
    }

    bool accepted = response.value("result", false);

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (accepted) {
            stats_.shares_accepted++;
        } else {
            stats_.shares_rejected++;
        }
        stats_.last_share_time = std::chrono::system_clock::now().time_since_epoch().count();
    }

    if (accepted) {
        std::cout << "Share accepted!" << std::endl;
    } else {
        std::cout << "Share rejected" << std::endl;
    }

    return accepted;
}

void StratumClient::set_job_callback(std::function<void(const MiningJob&)> callback) {
    job_callback_ = callback;
}

void StratumClient::set_difficulty_callback(std::function<void(double)> callback) {
    difficulty_callback_ = callback;
}

} // namespace pool
} // namespace intcoin
