// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_MINING_H
#define INTCOIN_MINING_H

#include "types.h"
#include "block.h"
#include "blockchain.h"
#include "consensus.h"
#include <randomx.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <memory>

namespace intcoin {
namespace mining {

// Mining statistics
struct MiningStats {
    uint64_t hashes_computed = 0;      // Total hashes computed
    uint64_t blocks_found = 0;          // Blocks successfully mined
    uint64_t shares_submitted = 0;      // Shares submitted to pool
    uint64_t shares_accepted = 0;       // Shares accepted by pool
    uint64_t shares_rejected = 0;       // Shares rejected by pool
    double hashrate = 0.0;              // Current hashrate (H/s)
    double average_hashrate = 0.0;      // Average hashrate (H/s)
    uint64_t uptime = 0;                // Miner uptime (seconds)
    uint32_t thread_count = 0;          // Number of mining threads
};

// Mining configuration
struct MiningConfig {
    uint32_t thread_count = 0;          // Number of threads (0 = auto-detect)
    std::string mining_address;         // Address to receive block rewards
    bool testnet = false;               // Mine on testnet
    uint32_t update_interval = 5;      // Stats update interval (seconds)

    // Pool configuration
    bool pool_mining = false;           // Enable pool mining
    std::string pool_host;              // Pool hostname
    uint16_t pool_port = 0;             // Pool port
    std::string pool_username;          // Pool username/worker name
    std::string pool_password;          // Pool password

    // Performance
    uint32_t batch_size = 100;          // Nonces to try per batch
    bool affinity_enabled = false;      // CPU affinity for threads
};

// Mining job (work unit)
struct MiningJob {
    BlockHeader header;                 // Block header template
    uint256 target;                     // Difficulty target
    uint64_t height = 0;                // Block height
    std::string job_id;                 // Job identifier (for pool)
    uint64_t extra_nonce = 0;           // Extra nonce (for pool)
    std::vector<uint8_t> coinbase;      // Coinbase transaction
    std::vector<uint256> merkle_branch; // Merkle branch for coinbase
};

// Mining result
struct MiningResult {
    bool found = false;                 // Block/share found
    BlockHeader header;                 // Solved header
    uint32_t nonce = 0;                 // Winning nonce
    uint256 hash;                       // Block hash
    uint64_t hashes_done = 0;           // Hashes computed
    double time_elapsed = 0.0;          // Time taken (seconds)
};

// Forward declarations
class MinerThread;
class MiningManager;
class StratumClient;

// ============================================================================
// Miner Thread - Individual mining thread
// ============================================================================

class MinerThread {
public:
    MinerThread(uint32_t thread_id, MiningManager* manager);
    ~MinerThread();

    // Start/stop mining
    void Start();
    void Stop();

    // Check if running
    bool IsRunning() const { return running_.load(); }

    // Get statistics
    uint64_t GetHashCount() const { return hash_count_.load(); }
    double GetHashrate() const;

    // Set mining job
    void SetJob(const MiningJob& job);

private:
    void MiningLoop();
    bool TrySolveBlock(const MiningJob& job, uint32_t nonce_start, uint32_t nonce_end);

    uint32_t thread_id_;
    MiningManager* manager_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> has_new_job_{false};
    std::atomic<uint64_t> hash_count_{0};

    MiningJob current_job_;
    std::mutex job_mutex_;

    // RandomX
    randomx_vm* vm_ = nullptr;
};

// ============================================================================
// Mining Manager - Coordinates multiple mining threads
// ============================================================================

class MiningManager {
public:
    MiningManager(const MiningConfig& config);
    ~MiningManager();

    // Start/stop mining
    Result<void> Start(Blockchain& blockchain);
    void Stop();

    // Check if mining
    bool IsMining() const { return mining_.load(); }

    // Get statistics
    MiningStats GetStats() const;

    // Update configuration
    void UpdateConfig(const MiningConfig& config);

    // Callbacks
    using BlockFoundCallback = std::function<void(const Block&)>;
    using ShareFoundCallback = std::function<void(const MiningResult&)>;

    void SetBlockFoundCallback(BlockFoundCallback callback) {
        block_found_callback_ = callback;
    }

    void SetShareFoundCallback(ShareFoundCallback callback) {
        share_found_callback_ = callback;
    }

    // Called by worker threads
    void OnBlockFound(const MiningResult& result);
    void OnShareFound(const MiningResult& result);

private:
    friend class MinerThread;

    void UpdateJob();
    void StatsUpdateLoop();
    Block BuildBlock(const MiningResult& result);

    MiningConfig config_;
    Blockchain* blockchain_ = nullptr;

    std::vector<std::unique_ptr<MinerThread>> threads_;
    std::atomic<bool> mining_{false};
    std::atomic<bool> stop_requested_{false};

    MiningJob current_job_;
    std::mutex job_mutex_;

    MiningStats stats_;
    std::mutex stats_mutex_;

    std::unique_ptr<std::thread> stats_thread_;
    std::unique_ptr<std::thread> job_update_thread_;

    BlockFoundCallback block_found_callback_;
    ShareFoundCallback share_found_callback_;

    // RandomX
    randomx_cache* cache_ = nullptr;
    randomx_dataset* dataset_ = nullptr;
};

// ============================================================================
// Stratum Client - Mining pool protocol
// ============================================================================

class StratumClient {
public:
    StratumClient(const MiningConfig& config);
    ~StratumClient();

    // Connect/disconnect
    Result<void> Connect();
    void Disconnect();

    // Check connection
    bool IsConnected() const { return connected_.load(); }

    // Subscribe and authorize
    Result<void> Subscribe();
    Result<void> Authorize();

    // Submit share
    Result<void> SubmitShare(const MiningResult& result, const std::string& job_id);

    // Get current job
    MiningJob GetCurrentJob() const;

    // Callbacks
    using JobCallback = std::function<void(const MiningJob&)>;
    using AcceptCallback = std::function<void(bool accepted, const std::string& reason)>;

    void SetJobCallback(JobCallback callback) {
        job_callback_ = callback;
    }

    void SetAcceptCallback(AcceptCallback callback) {
        accept_callback_ = callback;
    }

private:
    void ReceiveLoop();
    void SendMessage(const std::string& message);
    std::string ReadLine();

    void HandleMessage(const std::string& message);
    void HandleJobNotification(const std::string& message);
    void HandleResponse(const std::string& message);

    MiningConfig config_;
    int socket_ = -1;
    std::atomic<bool> connected_{false};
    std::atomic<bool> subscribed_{false};
    std::atomic<bool> authorized_{false};

    std::unique_ptr<std::thread> receive_thread_;
    std::mutex socket_mutex_;

    MiningJob current_job_;
    std::mutex job_mutex_;

    JobCallback job_callback_;
    AcceptCallback accept_callback_;

    std::string session_id_;
    uint64_t extra_nonce1_ = 0;
    uint32_t extra_nonce2_size_ = 0;
    uint32_t message_id_ = 1;
};

// ============================================================================
// Mining Utilities
// ============================================================================

// Detect optimal thread count based on CPU
uint32_t DetectOptimalThreadCount();

// Calculate hashrate from stats
double CalculateHashrate(uint64_t hashes, double time_seconds);

// Check if hash meets target
bool CheckHash(const uint256& hash, const uint256& target);

// Format hashrate for display (e.g., "1.23 MH/s")
std::string FormatHashrate(double hashrate);

// Build coinbase transaction
Transaction BuildCoinbaseTransaction(
    const std::string& mining_address,
    uint64_t block_reward,
    uint32_t height,
    const std::string& message = ""
);

} // namespace mining
} // namespace intcoin

#endif // INTCOIN_MINING_H
