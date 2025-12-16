// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/mining.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include "intcoin/consensus.h"
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace intcoin {
namespace mining {

// ============================================================================
// Mining Utilities
// ============================================================================

uint32_t DetectOptimalThreadCount() {
    uint32_t hardware_threads = std::thread::hardware_concurrency();
    if (hardware_threads == 0) {
        return 4; // Fallback
    }
    // Use all available threads for mining
    return hardware_threads;
}

double CalculateHashrate(uint64_t hashes, double time_seconds) {
    if (time_seconds <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(hashes) / time_seconds;
}

bool CheckHash(const uint256& hash, const uint256& target) {
    // Compare hash with target (lower hash value = more difficult)
    for (size_t i = 0; i < 32; ++i) {
        if (hash[i] < target[i]) {
            return true; // Hash meets target
        }
        if (hash[i] > target[i]) {
            return false; // Hash doesn't meet target
        }
    }
    return true; // Hashes are equal
}

std::string FormatHashrate(double hashrate) {
    const char* suffixes[] = {"H/s", "KH/s", "MH/s", "GH/s", "TH/s", "PH/s"};
    int suffix_index = 0;

    while (hashrate >= 1000.0 && suffix_index < 5) {
        hashrate /= 1000.0;
        suffix_index++;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", hashrate, suffixes[suffix_index]);
    return std::string(buffer);
}

Transaction BuildCoinbaseTransaction(
    const std::string& mining_address,
    uint64_t block_reward,
    uint32_t height,
    const std::string& message
) {
    Transaction tx;
    tx.version = 1;

    // Coinbase input
    TxIn coinbase_in;
    coinbase_in.prev_tx_hash = uint256{}; // Null hash
    coinbase_in.prev_tx_index = 0xFFFFFFFF; // Special index for coinbase

    // Build coinbase script: height + message
    std::vector<uint8_t> script_data;
    // Add height (BIP34)
    script_data.push_back(static_cast<uint8_t>(height & 0xFF));
    script_data.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
    script_data.push_back(static_cast<uint8_t>((height >> 16) & 0xFF));
    script_data.push_back(static_cast<uint8_t>((height >> 24) & 0xFF));

    // Add message if provided
    if (!message.empty()) {
        script_data.insert(script_data.end(), message.begin(), message.end());
    }

    coinbase_in.script_sig = Script(script_data);
    coinbase_in.sequence = 0xFFFFFFFF;

    tx.inputs.push_back(coinbase_in);

    // Coinbase output
    TxOut coinbase_out;
    coinbase_out.value = block_reward;

    // Decode mining address and create script
    auto decode_result = AddressEncoder::DecodeAddress(mining_address);
    if (decode_result.IsOk()) {
        uint256 pubkey_hash = decode_result.GetValue();

        // P2PKH script: OP_DUP OP_HASH160 <pubkeyhash> OP_EQUALVERIFY OP_CHECKSIG
        std::vector<uint8_t> script_bytes;
        script_bytes.push_back(0x76); // OP_DUP
        script_bytes.push_back(0xA9); // OP_HASH160
        script_bytes.push_back(32); // pubkey_hash is 32 bytes
        script_bytes.insert(script_bytes.end(), pubkey_hash.begin(), pubkey_hash.end());
        script_bytes.push_back(0x88); // OP_EQUALVERIFY
        script_bytes.push_back(0xAC); // OP_CHECKSIG

        coinbase_out.script_pubkey = Script(script_bytes);
    }

    tx.outputs.push_back(coinbase_out);

    return tx;
}

// ============================================================================
// Miner Thread Implementation
// ============================================================================

MinerThread::MinerThread(uint32_t thread_id, MiningManager* manager)
    : thread_id_(thread_id), manager_(manager) {
}

MinerThread::~MinerThread() {
    Stop();
    if (vm_) {
        randomx_destroy_vm(vm_);
    }
}

void MinerThread::Start() {
    if (running_.load()) {
        return;
    }

    running_.store(true);
    thread_ = std::make_unique<std::thread>(&MinerThread::MiningLoop, this);
}

void MinerThread::Stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

double MinerThread::GetHashrate() const {
    // Simple hashrate calculation based on recent performance
    // In a real implementation, this would track time windows
    return 0.0; // Placeholder
}

void MinerThread::SetJob(const MiningJob& job) {
    std::lock_guard<std::mutex> lock(job_mutex_);
    current_job_ = job;
    has_new_job_.store(true);
}

void MinerThread::MiningLoop() {
    // Initialize RandomX VM
    if (!manager_->cache_) {
        return;
    }

    vm_ = randomx_create_vm(RANDOMX_FLAG_DEFAULT, manager_->cache_, nullptr);
    if (!vm_) {
        return;
    }

    uint32_t nonce = thread_id_ * 1000000; // Offset for each thread

    while (running_.load()) {
        // Check for new job
        if (has_new_job_.load()) {
            std::lock_guard<std::mutex> lock(job_mutex_);
            has_new_job_.store(false);
            nonce = thread_id_ * 1000000; // Reset nonce
        }

        // Get current job
        MiningJob job;
        {
            std::lock_guard<std::mutex> lock(job_mutex_);
            job = current_job_;
        }

        // Try solving block
        const uint32_t batch_size = 100;
        if (TrySolveBlock(job, nonce, nonce + batch_size)) {
            // Block found!
        }

        nonce += batch_size;
        hash_count_.fetch_add(batch_size);

        // Wrap around at 32-bit boundary
        if (nonce == 0) {
            nonce = thread_id_ * 1000000;
        }
    }
}

bool MinerThread::TrySolveBlock(const MiningJob& job, uint32_t nonce_start, uint32_t nonce_end) {
    BlockHeader header = job.header;

    for (uint32_t nonce = nonce_start; nonce < nonce_end; ++nonce) {
        if (!running_.load()) {
            return false;
        }

        header.nonce = nonce;

        // Serialize header (up to nonce: 4+32+32+8+4+8 = 88 bytes)
        std::vector<uint8_t> header_data;
        header_data.resize(88);
        // Simplified serialization (real implementation would use proper serialization)
        std::memcpy(header_data.data(), &header.version, 4);
        std::memcpy(header_data.data() + 4, header.prev_block_hash.data(), 32);
        std::memcpy(header_data.data() + 36, header.merkle_root.data(), 32);
        std::memcpy(header_data.data() + 68, &header.timestamp, 8);
        std::memcpy(header_data.data() + 76, &header.bits, 4);
        std::memcpy(header_data.data() + 80, &header.nonce, 8);

        // Calculate RandomX hash
        uint256 hash;
        randomx_calculate_hash(vm_, header_data.data(), header_data.size(), hash.data());

        // Check if hash meets target
        if (CheckHash(hash, job.target)) {
            MiningResult result;
            result.found = true;
            result.header = header;
            result.nonce = nonce;
            result.hash = hash;
            result.hashes_done = nonce - nonce_start + 1;

            // Notify manager
            manager_->OnBlockFound(result);
            return true;
        }
    }

    return false;
}

// ============================================================================
// Mining Manager Implementation
// ============================================================================

MiningManager::MiningManager(const MiningConfig& config)
    : config_(config) {

    if (config_.thread_count == 0) {
        config_.thread_count = DetectOptimalThreadCount();
    }
}

MiningManager::~MiningManager() {
    Stop();

    if (cache_) {
        randomx_release_cache(cache_);
    }
    if (dataset_) {
        randomx_release_dataset(dataset_);
    }
}

Result<void> MiningManager::Start(Blockchain& blockchain) {
    if (mining_.load()) {
        return Result<void>::Error("Mining already started");
    }

    blockchain_ = &blockchain;

    // Initialize RandomX
    // Get key from genesis block (use height 0)
    auto genesis_result = blockchain_->GetBlockByHeight(0);
    if (!genesis_result.IsOk()) {
        return Result<void>::Error("Failed to get genesis block");
    }

    Block genesis = genesis_result.GetValue();
    uint256 key = genesis.GetHash();

    cache_ = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
    if (!cache_) {
        return Result<void>::Error("Failed to allocate RandomX cache");
    }

    randomx_init_cache(cache_, key.data(), key.size());

    // Create mining threads
    for (uint32_t i = 0; i < config_.thread_count; ++i) {
        threads_.push_back(std::make_unique<MinerThread>(i, this));
    }

    // Start threads
    for (auto& thread : threads_) {
        thread->Start();
    }

    mining_.store(true);
    stats_.thread_count = config_.thread_count;

    // Start stats update thread
    stats_thread_ = std::make_unique<std::thread>(&MiningManager::StatsUpdateLoop, this);

    // Initial job update
    UpdateJob();

    return Result<void>::Ok();
}

void MiningManager::Stop() {
    if (!mining_.load()) {
        return;
    }

    mining_.store(false);
    stop_requested_.store(true);

    // Stop all threads
    for (auto& thread : threads_) {
        thread->Stop();
    }

    if (stats_thread_ && stats_thread_->joinable()) {
        stats_thread_->join();
    }

    threads_.clear();
}

MiningStats MiningManager::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void MiningManager::UpdateConfig(const MiningConfig& config) {
    config_ = config;
}

void MiningManager::OnBlockFound(const MiningResult& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.blocks_found++;

    // Build complete block
    Block block = BuildBlock(result);

    // Callback
    if (block_found_callback_) {
        block_found_callback_(block);
    }

    // Update job (new block found, need new template)
    UpdateJob();
}

void MiningManager::OnShareFound(const MiningResult& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.shares_submitted++;

    // Callback
    if (share_found_callback_) {
        share_found_callback_(result);
    }
}

void MiningManager::UpdateJob() {
    if (!blockchain_) {
        return;
    }

    // Get current blockchain state
    uint32_t height = blockchain_->GetBestHeight() + 1;
    uint256 prev_hash = blockchain_->GetBestBlockHash();

    // Build coinbase transaction
    uint64_t block_reward = GetBlockReward(height);
    Transaction coinbase = BuildCoinbaseTransaction(
        config_.mining_address,
        block_reward,
        height,
        "Mined with INTcoin CPU Miner"
    );

    // Create block template
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = prev_hash;
    header.timestamp = std::time(nullptr);

    // Get difficulty target from last block
    auto last_block_result = blockchain_->GetBlockByHeight(blockchain_->GetBestHeight());
    if (last_block_result.IsOk()) {
        BlockHeader last_header = last_block_result.GetValue().header;
        header.bits = DifficultyCalculator::GetNextWorkRequired(last_header, *blockchain_);
    } else {
        header.bits = consensus::MIN_DIFFICULTY_BITS; // Fallback
    }

    header.nonce = 0;

    // Build merkle root (just coinbase for now)
    std::vector<uint256> tx_hashes;
    tx_hashes.push_back(coinbase.GetHash());
    header.merkle_root = CalculateMerkleRoot(tx_hashes);

    // Create mining job
    MiningJob job;
    job.header = header;
    job.target = DifficultyCalculator::CompactToTarget(header.bits);
    job.height = height;
    job.job_id = std::to_string(height);

    // Update all threads
    {
        std::lock_guard<std::mutex> lock(job_mutex_);
        current_job_ = job;
    }

    for (auto& thread : threads_) {
        thread->SetJob(job);
    }
}

void MiningManager::StatsUpdateLoop() {
    auto start_time = std::chrono::steady_clock::now();

    while (mining_.load() && !stop_requested_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.update_interval));

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

        // Collect stats from all threads
        uint64_t total_hashes = 0;
        for (const auto& thread : threads_) {
            total_hashes += thread->GetHashCount();
        }

        // Update stats
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.hashes_computed = total_hashes;
            stats_.uptime = elapsed;

            if (elapsed > 0) {
                stats_.hashrate = CalculateHashrate(total_hashes, static_cast<double>(elapsed));
                stats_.average_hashrate = stats_.hashrate; // Simplified
            }
        }

        // Print stats
        auto stats = GetStats();
        std::cout << "[Mining] Hashrate: " << FormatHashrate(stats.hashrate)
                  << " | Blocks: " << stats.blocks_found
                  << " | Uptime: " << stats.uptime << "s\n";
    }
}

Block MiningManager::BuildBlock(const MiningResult& result) {
    Block block;
    block.header = result.header;

    // Add coinbase transaction
    // In a real implementation, would add transactions from mempool
    uint64_t block_reward = GetBlockReward(current_job_.height);
    Transaction coinbase = BuildCoinbaseTransaction(
        config_.mining_address,
        block_reward,
        current_job_.height,
        "Mined with INTcoin CPU Miner"
    );

    block.transactions.push_back(coinbase);

    return block;
}

// ============================================================================
// Stratum Client Implementation (Simplified)
// ============================================================================

StratumClient::StratumClient(const MiningConfig& config)
    : config_(config) {
}

StratumClient::~StratumClient() {
    Disconnect();
}

Result<void> StratumClient::Connect() {
    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        return Result<void>::Error("Failed to create socket");
    }

    // Resolve hostname
    struct hostent* host = gethostbyname(config_.pool_host.c_str());
    if (!host) {
        close(socket_);
        return Result<void>::Error("Failed to resolve hostname");
    }

    // Connect
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.pool_port);
    std::memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_);
        return Result<void>::Error("Failed to connect to pool");
    }

    connected_.store(true);

    // Start receive thread
    receive_thread_ = std::make_unique<std::thread>(&StratumClient::ReceiveLoop, this);

    return Result<void>::Ok();
}

void StratumClient::Disconnect() {
    if (!connected_.load()) {
        return;
    }

    connected_.store(false);

    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }

    if (receive_thread_ && receive_thread_->joinable()) {
        receive_thread_->join();
    }
}

Result<void> StratumClient::Subscribe() {
    if (!connected_.load()) {
        return Result<void>::Error("Not connected");
    }

    // Send subscribe message (simplified JSON-RPC)
    std::string message = "{\"id\":" + std::to_string(message_id_++) +
                          ",\"method\":\"mining.subscribe\",\"params\":[\"INTcoin Miner/1.0\"]}\n";

    SendMessage(message);
    subscribed_.store(true);

    return Result<void>::Ok();
}

Result<void> StratumClient::Authorize() {
    if (!subscribed_.load()) {
        return Result<void>::Error("Not subscribed");
    }

    // Send authorize message
    std::string message = "{\"id\":" + std::to_string(message_id_++) +
                          ",\"method\":\"mining.authorize\",\"params\":[\"" +
                          config_.pool_username + "\",\"" +
                          config_.pool_password + "\"]}\n";

    SendMessage(message);
    authorized_.store(true);

    return Result<void>::Ok();
}

Result<void> StratumClient::SubmitShare(const MiningResult& result, const std::string& job_id) {
    if (!authorized_.load()) {
        return Result<void>::Error("Not authorized");
    }

    // Build submit message (simplified)
    char nonce_hex[17];
    snprintf(nonce_hex, sizeof(nonce_hex), "%08x", result.nonce);

    std::string message = "{\"id\":" + std::to_string(message_id_++) +
                          ",\"method\":\"mining.submit\",\"params\":[\"" +
                          config_.pool_username + "\",\"" +
                          job_id + "\",\"" +
                          nonce_hex + "\"]}\n";

    SendMessage(message);

    return Result<void>::Ok();
}

MiningJob StratumClient::GetCurrentJob() const {
    std::lock_guard<std::mutex> lock(job_mutex_);
    return current_job_;
}

void StratumClient::ReceiveLoop() {
    while (connected_.load()) {
        std::string line = ReadLine();
        if (!line.empty()) {
            HandleMessage(line);
        }
    }
}

void StratumClient::SendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (socket_ >= 0) {
        send(socket_, message.c_str(), message.length(), 0);
    }
}

std::string StratumClient::ReadLine() {
    std::string line;
    char ch;

    while (connected_.load()) {
        ssize_t result = recv(socket_, &ch, 1, 0);
        if (result <= 0) {
            break;
        }

        if (ch == '\n') {
            break;
        }

        line += ch;
    }

    return line;
}

void StratumClient::HandleMessage(const std::string& message) {
    // Simplified message handling
    // Real implementation would parse JSON properly

    if (message.find("mining.notify") != std::string::npos) {
        HandleJobNotification(message);
    } else if (message.find("\"result\"") != std::string::npos) {
        HandleResponse(message);
    }
}

void StratumClient::HandleJobNotification(const std::string& message) {
    // Parse job notification and update current job
    // Simplified - real implementation would parse JSON

    if (job_callback_) {
        job_callback_(current_job_);
    }
}

void StratumClient::HandleResponse(const std::string& message) {
    // Handle response (subscribe, authorize, submit)
    // Simplified - real implementation would parse JSON

    bool accepted = (message.find("\"result\":true") != std::string::npos);

    if (accept_callback_) {
        accept_callback_(accepted, accepted ? "Accepted" : "Rejected");
    }
}

} // namespace mining
} // namespace intcoin
