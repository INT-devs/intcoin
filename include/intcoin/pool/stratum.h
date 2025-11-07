// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_POOL_STRATUM_H
#define INTCOIN_POOL_STRATUM_H

#include "../primitives.h"
#include "../block.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace intcoin {
namespace pool {

using json = nlohmann::json;

/**
 * Stratum protocol version
 */
enum class StratumVersion {
    V1,  // Stratum V1 (original)
    V2   // Stratum V2 (more efficient)
};

/**
 * Stratum method types
 */
enum class StratumMethod {
    SUBSCRIBE,
    AUTHORIZE,
    SUBMIT,
    SET_DIFFICULTY,
    NOTIFY,
    SET_EXTRANONCE,
    UNKNOWN
};

/**
 * Mining share submission
 */
struct MiningShare {
    std::string worker_name;
    std::string job_id;
    std::string extranonce2;
    uint64_t nonce;
    uint64_t timestamp;
    Hash256 hash;
    double difficulty;
    bool is_block;

    MiningShare() : nonce(0), timestamp(0), difficulty(0.0), is_block(false) {}
};

/**
 * Mining job for miners
 */
struct MiningJob {
    std::string job_id;
    Hash256 prev_block_hash;
    std::string coinbase1;  // First part of coinbase
    std::string coinbase2;  // Second part of coinbase
    std::vector<Hash256> merkle_branch;
    uint32_t version;
    uint32_t bits;
    uint64_t timestamp;
    bool clean_jobs;
    double difficulty;

    MiningJob() : version(0), bits(0), timestamp(0), clean_jobs(false), difficulty(0.0) {}

    // Serialize to JSON for Stratum
    json to_json() const;
};

/**
 * Worker statistics
 */
struct WorkerStats {
    std::string worker_name;
    uint64_t shares_accepted;
    uint64_t shares_rejected;
    uint64_t shares_stale;
    uint64_t blocks_found;
    double hashrate;  // Estimated hashrate
    uint64_t last_share_time;

    WorkerStats() : shares_accepted(0), shares_rejected(0), shares_stale(0),
                    blocks_found(0), hashrate(0.0), last_share_time(0) {}

    double acceptance_rate() const {
        uint64_t total = shares_accepted + shares_rejected;
        return total > 0 ? (double)shares_accepted / total : 0.0;
    }
};

/**
 * Stratum server for mining pool
 *
 * Implements Stratum V1 and V2 protocols for coordinating miners
 */
class StratumServer {
public:
    StratumServer(uint16_t port, StratumVersion version = StratumVersion::V1);
    ~StratumServer();

    // Server control
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Job management
    void set_new_job(const MiningJob& job);
    MiningJob get_current_job() const;

    // Difficulty management
    void set_difficulty(const std::string& worker, double difficulty);
    double get_difficulty(const std::string& worker) const;
    void enable_vardiff(bool enable) { vardiff_enabled_ = enable; }

    // Worker management
    bool authorize_worker(const std::string& worker, const std::string& password);
    void disconnect_worker(const std::string& worker);
    std::vector<std::string> get_connected_workers() const;
    WorkerStats get_worker_stats(const std::string& worker) const;

    // Share handling
    void set_share_handler(std::function<bool(const MiningShare&)> handler);
    void set_block_found_handler(std::function<void(const Block&)> handler);

    // Statistics
    uint64_t get_total_shares() const;
    uint64_t get_total_blocks() const;
    double get_pool_hashrate() const;

private:
    uint16_t port_;
    StratumVersion version_;
    bool running_;
    bool vardiff_enabled_;

    // Current mining job
    MiningJob current_job_;
    mutable std::mutex job_mutex_;

    // Worker data
    std::unordered_map<std::string, WorkerStats> worker_stats_;
    std::unordered_map<std::string, double> worker_difficulty_;
    std::unordered_map<std::string, std::string> authorized_workers_;
    mutable std::mutex workers_mutex_;

    // Handlers
    std::function<bool(const MiningShare&)> share_handler_;
    std::function<void(const Block&)> block_found_handler_;

    // Network handling
    void accept_connections();
    void handle_client(int socket_fd);
    void send_stratum_message(int socket_fd, const json& message);
    json receive_stratum_message(int socket_fd);

    // Stratum protocol
    StratumMethod parse_method(const std::string& method_str) const;
    json handle_subscribe(const json& params);
    json handle_authorize(const json& params);
    json handle_submit(const json& params);
    void send_notify(int socket_fd, const MiningJob& job);
    void send_set_difficulty(int socket_fd, double difficulty);

    // Share validation
    bool validate_share(const MiningShare& share) const;
    void update_worker_stats(const std::string& worker, bool accepted);

    // Variable difficulty
    void adjust_difficulty(const std::string& worker);
    double calculate_vardiff(const WorkerStats& stats) const;
};

/**
 * Stratum client for solo miners connecting to pools
 */
class StratumClient {
public:
    StratumClient(const std::string& pool_url, uint16_t port,
                  const std::string& username, const std::string& password);
    ~StratumClient();

    // Connection control
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }

    // Mining operations
    MiningJob get_current_job() const;
    bool submit_share(const MiningShare& share);

    // Statistics
    WorkerStats get_stats() const { return stats_; }

    // Callbacks
    void set_job_callback(std::function<void(const MiningJob&)> callback);
    void set_difficulty_callback(std::function<void(double)> callback);

private:
    std::string pool_url_;
    uint16_t port_;
    std::string username_;
    std::string password_;
    bool connected_;
    int socket_fd_;

    // Current state
    MiningJob current_job_;
    double current_difficulty_;
    std::string extranonce1_;
    size_t extranonce2_size_;
    WorkerStats stats_;

    // Callbacks
    std::function<void(const MiningJob&)> job_callback_;
    std::function<void(double)> difficulty_callback_;

    // Network handling
    void receive_loop();
    void send_stratum_message(const json& message);
    json receive_stratum_message();

    // Protocol handling
    void handle_notify(const json& params);
    void handle_set_difficulty(const json& params);
    json subscribe_message() const;
    json authorize_message() const;
    json submit_message(const MiningShare& share) const;

    mutable std::mutex job_mutex_;
    mutable std::mutex stats_mutex_;
};

/**
 * Share difficulty calculator
 */
class DifficultyCalculator {
public:
    // Calculate share difficulty from hash
    static double calculate_share_difficulty(const Hash256& hash);

    // Calculate target from difficulty
    static Hash256 difficulty_to_target(double difficulty);

    // Check if hash meets difficulty
    static bool check_difficulty(const Hash256& hash, double difficulty);

    // Calculate pool hashrate from shares
    static double calculate_hashrate(uint64_t shares, uint64_t time_period,
                                     double avg_difficulty);

    // Variable difficulty parameters
    struct VardiffConfig {
        double target_share_time;  // Target seconds between shares
        double min_difficulty;
        double max_difficulty;
        double retarget_time;      // How often to adjust
        double variance_percent;   // Allowed variance before adjust

        VardiffConfig() : target_share_time(15.0), min_difficulty(1.0),
                         max_difficulty(1000000.0), retarget_time(60.0),
                         variance_percent(10.0) {}
    };

    // Calculate new difficulty based on share submission rate
    static double calculate_vardiff(const WorkerStats& stats,
                                    const VardiffConfig& config);
};

} // namespace pool
} // namespace intcoin

#endif // INTCOIN_POOL_STRATUM_H
