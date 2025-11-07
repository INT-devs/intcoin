// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/pool/stratum.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace intcoin {
namespace pool {

// MiningJob implementation

json MiningJob::to_json() const {
    json j;
    j["job_id"] = job_id;
    j["prevhash"] = prev_block_hash.to_string();
    j["coinb1"] = coinbase1;
    j["coinb2"] = coinbase2;

    json merkle_array = json::array();
    for (const auto& hash : merkle_branch) {
        merkle_array.push_back(hash.to_string());
    }
    j["merkle_branch"] = merkle_array;

    j["version"] = version;
    j["nbits"] = bits;
    j["ntime"] = timestamp;
    j["clean_jobs"] = clean_jobs;

    return j;
}

// StratumServer implementation

StratumServer::StratumServer(uint16_t port, StratumVersion version)
    : port_(port), version_(version), running_(false), vardiff_enabled_(true) {
}

StratumServer::~StratumServer() {
    stop();
}

bool StratumServer::start() {
    if (running_) {
        return false;
    }

    running_ = true;

    // Start server thread
    std::thread([this]() {
        accept_connections();
    }).detach();

    std::cout << "Stratum server started on port " << port_ << std::endl;
    return true;
}

void StratumServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    std::cout << "Stratum server stopped" << std::endl;
}

void StratumServer::accept_connections() {
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(server_fd);
        return;
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen on port " << port_ << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Listening for connections on port " << port_ << std::endl;

    // Accept loop
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }

        std::cout << "New client connected from "
                  << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Handle client in separate thread
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }

    close(server_fd);
}

void StratumServer::handle_client(int socket_fd) {
    std::string worker_name;

    try {
        while (running_) {
            json request = receive_stratum_message(socket_fd);
            if (request.is_null()) {
                break;
            }

            // Parse method
            std::string method_str = request.value("method", "");
            StratumMethod method = parse_method(method_str);
            json params = request.value("params", json::array());
            json id = request.value("id", nullptr);

            json response;
            response["id"] = id;
            response["error"] = nullptr;

            switch (method) {
                case StratumMethod::SUBSCRIBE: {
                    response["result"] = handle_subscribe(params);
                    send_stratum_message(socket_fd, response);

                    // Send initial difficulty and job
                    send_set_difficulty(socket_fd, 1.0);
                    send_notify(socket_fd, current_job_);
                    break;
                }

                case StratumMethod::AUTHORIZE: {
                    response["result"] = handle_authorize(params);
                    if (response["result"].get<bool>() && !params.empty()) {
                        worker_name = params[0].get<std::string>();
                    }
                    send_stratum_message(socket_fd, response);
                    break;
                }

                case StratumMethod::SUBMIT: {
                    response["result"] = handle_submit(params);
                    send_stratum_message(socket_fd, response);

                    // Adjust difficulty if vardiff enabled
                    if (vardiff_enabled_ && !worker_name.empty()) {
                        adjust_difficulty(worker_name);
                    }
                    break;
                }

                default:
                    response["error"] = "Unknown method";
                    send_stratum_message(socket_fd, response);
                    break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handler error: " << e.what() << std::endl;
    }

    if (!worker_name.empty()) {
        disconnect_worker(worker_name);
    }
    close(socket_fd);
}

void StratumServer::send_stratum_message(int socket_fd, const json& message) {
    std::string msg_str = message.dump() + "\n";
    send(socket_fd, msg_str.c_str(), msg_str.length(), 0);
}

json StratumServer::receive_stratum_message(int socket_fd) {
    char buffer[4096];
    ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);

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

StratumMethod StratumServer::parse_method(const std::string& method_str) const {
    if (method_str == "mining.subscribe") return StratumMethod::SUBSCRIBE;
    if (method_str == "mining.authorize") return StratumMethod::AUTHORIZE;
    if (method_str == "mining.submit") return StratumMethod::SUBMIT;
    return StratumMethod::UNKNOWN;
}

json StratumServer::handle_subscribe(const json& params) {
    // Return subscription details
    json result = json::array();

    // Subscription ID and extranonce
    result.push_back(json::array({"mining.notify", "subscription_id"}));
    result.push_back("extranonce1_hex");  // 4 bytes
    result.push_back(4);  // extranonce2 size

    return result;
}

json StratumServer::handle_authorize(const json& params) {
    if (params.size() < 2) {
        return false;
    }

    std::string username = params[0].get<std::string>();
    std::string password = params[1].get<std::string>();

    bool authorized = authorize_worker(username, password);
    return authorized;
}

json StratumServer::handle_submit(const json& params) {
    if (params.size() < 5) {
        return false;
    }

    try {
        MiningShare share;
        share.worker_name = params[0].get<std::string>();
        share.job_id = params[1].get<std::string>();
        share.extranonce2 = params[2].get<std::string>();
        share.timestamp = std::stoull(params[3].get<std::string>(), nullptr, 16);
        share.nonce = std::stoull(params[4].get<std::string>(), nullptr, 16);

        bool valid = validate_share(share);
        update_worker_stats(share.worker_name, valid);

        if (valid && share_handler_) {
            share_handler_(share);
        }

        return valid;
    } catch (...) {
        return false;
    }
}

void StratumServer::send_notify(int socket_fd, const MiningJob& job) {
    json notify;
    notify["method"] = "mining.notify";
    notify["params"] = json::array({
        job.job_id,
        job.prev_block_hash.to_string(),
        job.coinbase1,
        job.coinbase2,
        json(job.merkle_branch),
        job.version,
        job.bits,
        job.timestamp,
        job.clean_jobs
    });

    send_stratum_message(socket_fd, notify);
}

void StratumServer::send_set_difficulty(int socket_fd, double difficulty) {
    json set_diff;
    set_diff["method"] = "mining.set_difficulty";
    set_diff["params"] = json::array({difficulty});

    send_stratum_message(socket_fd, set_diff);
}

bool StratumServer::validate_share(const MiningShare& share) const {
    // Basic validation
    if (share.worker_name.empty() || share.job_id.empty()) {
        return false;
    }

    // Check if job ID matches current job
    std::lock_guard<std::mutex> lock(job_mutex_);
    if (share.job_id != current_job_.job_id) {
        return false;  // Stale share
    }

    // TODO: Validate hash meets difficulty
    // This would involve reconstructing the block and checking PoW

    return true;
}

void StratumServer::update_worker_stats(const std::string& worker, bool accepted) {
    std::lock_guard<std::mutex> lock(workers_mutex_);

    auto& stats = worker_stats_[worker];
    if (accepted) {
        stats.shares_accepted++;
    } else {
        stats.shares_rejected++;
    }
    stats.last_share_time = std::chrono::system_clock::now().time_since_epoch().count();
}

void StratumServer::set_new_job(const MiningJob& job) {
    std::lock_guard<std::mutex> lock(job_mutex_);
    current_job_ = job;

    // TODO: Notify all connected miners
}

MiningJob StratumServer::get_current_job() const {
    std::lock_guard<std::mutex> lock(job_mutex_);
    return current_job_;
}

void StratumServer::set_difficulty(const std::string& worker, double difficulty) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    worker_difficulty_[worker] = difficulty;
}

double StratumServer::get_difficulty(const std::string& worker) const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    auto it = worker_difficulty_.find(worker);
    return it != worker_difficulty_.end() ? it->second : 1.0;
}

bool StratumServer::authorize_worker(const std::string& worker, const std::string& password) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    authorized_workers_[worker] = password;

    // Initialize worker stats
    if (worker_stats_.find(worker) == worker_stats_.end()) {
        worker_stats_[worker] = WorkerStats();
        worker_stats_[worker].worker_name = worker;
    }

    return true;
}

void StratumServer::disconnect_worker(const std::string& worker) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    authorized_workers_.erase(worker);
}

std::vector<std::string> StratumServer::get_connected_workers() const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    std::vector<std::string> workers;
    for (const auto& pair : authorized_workers_) {
        workers.push_back(pair.first);
    }
    return workers;
}

WorkerStats StratumServer::get_worker_stats(const std::string& worker) const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    auto it = worker_stats_.find(worker);
    return it != worker_stats_.end() ? it->second : WorkerStats();
}

void StratumServer::set_share_handler(std::function<bool(const MiningShare&)> handler) {
    share_handler_ = handler;
}

void StratumServer::set_block_found_handler(std::function<void(const Block&)> handler) {
    block_found_handler_ = handler;
}

uint64_t StratumServer::get_total_shares() const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    uint64_t total = 0;
    for (const auto& pair : worker_stats_) {
        total += pair.second.shares_accepted;
    }
    return total;
}

uint64_t StratumServer::get_total_blocks() const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    uint64_t total = 0;
    for (const auto& pair : worker_stats_) {
        total += pair.second.blocks_found;
    }
    return total;
}

double StratumServer::get_pool_hashrate() const {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    double total_hashrate = 0.0;
    for (const auto& pair : worker_stats_) {
        total_hashrate += pair.second.hashrate;
    }
    return total_hashrate;
}

void StratumServer::adjust_difficulty(const std::string& worker) {
    std::lock_guard<std::mutex> lock(workers_mutex_);

    auto stats_it = worker_stats_.find(worker);
    if (stats_it == worker_stats_.end()) {
        return;
    }

    double new_diff = calculate_vardiff(stats_it->second);
    worker_difficulty_[worker] = new_diff;

    // TODO: Send new difficulty to worker
}

double StratumServer::calculate_vardiff(const WorkerStats& stats) const {
    // Simple vardiff: aim for one share every 15 seconds
    // This is a placeholder - real vardiff is more complex
    if (stats.shares_accepted < 10) {
        return 1.0;  // Start easy
    }

    // TODO: Implement proper vardiff algorithm
    return 1.0;
}

// DifficultyCalculator implementation

double DifficultyCalculator::calculate_share_difficulty(const Hash256& hash) {
    // Calculate difficulty from hash
    // difficulty = max_target / hash_value
    // This is a simplified implementation
    return 1.0;  // TODO: Implement proper calculation
}

Hash256 DifficultyCalculator::difficulty_to_target(double difficulty) {
    // Convert difficulty to target hash
    // target = max_target / difficulty
    // TODO: Implement proper calculation
    return Hash256();
}

bool DifficultyCalculator::check_difficulty(const Hash256& hash, double difficulty) {
    Hash256 target = difficulty_to_target(difficulty);
    // Check if hash < target
    // TODO: Implement hash comparison
    return true;
}

double DifficultyCalculator::calculate_hashrate(uint64_t shares, uint64_t time_period,
                                                 double avg_difficulty) {
    if (time_period == 0) return 0.0;

    // hashrate = (shares × difficulty × 2^32) / time
    double hashes = shares * avg_difficulty * 4294967296.0;
    return hashes / time_period;  // Hashes per second
}

double DifficultyCalculator::calculate_vardiff(const WorkerStats& stats,
                                               const VardiffConfig& config) {
    // Calculate time since last share
    uint64_t now = std::chrono::system_clock::now().time_since_epoch().count();
    double time_since = (now - stats.last_share_time) / 1e9;  // Convert to seconds

    if (time_since < 1.0) {
        return config.min_difficulty;  // Too fast, use minimum
    }

    // Calculate new difficulty based on target share time
    double ratio = time_since / config.target_share_time;
    double new_diff = stats.hashrate / (config.target_share_time * 4294967296.0);

    // Clamp to configured limits
    new_diff = std::max(config.min_difficulty, std::min(config.max_difficulty, new_diff));

    return new_diff;
}

} // namespace pool
} // namespace intcoin
