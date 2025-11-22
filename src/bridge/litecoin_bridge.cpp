// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/bridge/bridge.h"
#include "../../include/intcoin/crypto/hash.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <curl/curl.h>

namespace intcoin {
namespace bridge {

//==============================================================================
// Litecoin RPC Helper
//==============================================================================

namespace {

size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

} // anonymous namespace

//==============================================================================
// LitecoinBridge Implementation
//==============================================================================

class LitecoinBridge : public Bridge {
public:
    LitecoinBridge(Blockchain* intcoin_chain,
                   const std::string& litecoin_rpc_url = "http://localhost:9332")
        : intcoin_chain_(intcoin_chain),
          litecoin_rpc_url_(litecoin_rpc_url),
          running_(false),
          status_(BridgeStatus::OFFLINE) {

        if (!intcoin_chain_) {
            throw std::runtime_error("LitecoinBridge: intcoin_chain cannot be null");
        }

        swap_manager_ = std::make_unique<AtomicSwapManager>();
        relay_ = std::make_unique<BridgeRelay>(ChainType::LITECOIN);
        ltc_verifier_ = std::make_unique<SPVChainVerifier>(ChainType::LITECOIN);
    }

    ~LitecoinBridge() override {
        stop();
    }

    // Bridge lifecycle
    bool start() override {
        if (running_) {
            return true;
        }

        std::cout << "Starting Litecoin bridge..." << std::endl;

        // Test Litecoin RPC connection
        std::string result;
        if (!query_litecoin_rpc("getblockchaininfo", "{}", result)) {
            std::cerr << "Failed to connect to Litecoin RPC at " << litecoin_rpc_url_ << std::endl;
            status_ = BridgeStatus::ERROR;
            return false;
        }

        running_ = true;
        status_ = BridgeStatus::SYNCING;

        // Start monitoring threads
        std::thread([this]() { monitor_swaps(); }).detach();
        std::thread([this]() { monitor_litecoin_chain(); }).detach();

        // Initial chain sync
        if (sync_chain()) {
            status_ = BridgeStatus::ONLINE;
            std::cout << "Litecoin bridge online" << std::endl;
        }

        return true;
    }

    void stop() override {
        if (!running_) {
            return;
        }

        std::cout << "Stopping Litecoin bridge..." << std::endl;
        running_ = false;
        status_ = BridgeStatus::OFFLINE;
    }

    bool is_running() const override { return running_; }

    // Bridge info
    ChainType get_chain_type() const override { return ChainType::LITECOIN; }
    std::string get_chain_name() const override { return "Litecoin"; }
    BridgeStatus get_status() const override { return status_; }

    // Swap operations
    Hash256 initiate_swap(const PublicKey& recipient, uint64_t amount) override {
        if (!running_ || status_ != BridgeStatus::ONLINE) {
            throw std::runtime_error("Litecoin bridge not online");
        }

        // Generate secret and hash lock
        Hash256 secret;
        RAND_bytes(secret.data(), secret.size());

        crypto::SHA256 hasher;
        hasher.update(secret.data(), secret.size());
        Hash256 hash_lock;
        hasher.finalize(hash_lock.data());

        // Calculate safe timelock (Litecoin: 2.5 min blocks, 12 confirmations)
        uint32_t timelock = BridgeUtils::calculate_safe_timelock(ChainType::LITECOIN);
        uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));

        // Create swap on INTcoin side
        Hash256 swap_id = swap_manager_->create_swap(
            hash_lock,
            recipient,
            amount,
            current_time + timelock
        );

        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_swaps++;
        stats_.total_volume_sent += amount;

        std::cout << "Initiated Litecoin swap " << swap_id << " for " << amount << " litoshis" << std::endl;

        return swap_id;
    }

    bool complete_swap(const Hash256& swap_id, const Hash256& secret) override {
        if (!running_) {
            return false;
        }

        bool success = swap_manager_->complete_swap(swap_id, secret);

        if (success) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.completed_swaps++;
            std::cout << "Completed Litecoin swap " << swap_id << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.failed_swaps++;
            std::cerr << "Failed to complete Litecoin swap " << swap_id << std::endl;
        }

        return success;
    }

    bool refund_swap(const Hash256& swap_id) override {
        if (!running_) {
            return false;
        }

        bool success = swap_manager_->refund_swap(swap_id);

        if (success) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.failed_swaps++;
            std::cout << "Refunded Litecoin swap " << swap_id << std::endl;
        }

        return success;
    }

    // Proof verification
    bool verify_lock_proof(const Hash256& swap_id, const CrossChainProof& proof) override {
        if (!running_) {
            return false;
        }

        if (!relay_->verify_proof(proof)) {
            return false;
        }

        auto swap_info = swap_manager_->get_swap(swap_id);
        if (!swap_info.has_value()) {
            return false;
        }

        std::cout << "Verified Litecoin lock proof for swap " << swap_id << std::endl;

        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_volume_received += swap_info->amount;

        return true;
    }

    // Chain synchronization
    bool sync_chain() override {
        if (!running_) {
            return false;
        }

        // Get current Litecoin blockchain height
        std::string result;
        if (!query_litecoin_rpc("getblockcount", "{}", result)) {
            return false;
        }

        uint32_t ltc_height = std::stoul(result);
        uint32_t current_height = ltc_verifier_->get_height();

        // Sync headers from current position to ltc_height
        for (uint32_t h = current_height + 1; h <= ltc_height && running_; ++h) {
            SPVBlockHeader header;
            if (!get_litecoin_block_header(h, header)) {
                std::cerr << "Failed to get Litecoin block header at height " << h << std::endl;
                return false;
            }

            if (!ltc_verifier_->add_header(header)) {
                std::cerr << "Failed to verify Litecoin block header at height " << h << std::endl;
                return false;
            }

            relay_->add_header(h, header);
        }

        std::cout << "Synced to Litecoin height " << ltc_height << std::endl;
        return true;
    }

    uint32_t get_chain_height() const override {
        std::string result;
        if (!const_cast<LitecoinBridge*>(this)->query_litecoin_rpc("getblockcount", "{}", result)) {
            return 0;
        }
        return std::stoul(result);
    }

    uint32_t get_sync_height() const override {
        return ltc_verifier_->get_height();
    }

    // Statistics
    BridgeStats get_stats() const override {
        std::lock_guard<std::mutex> lock(stats_mutex_);

        BridgeStats stats = stats_;
        if (stats.total_swaps > 0) {
            stats.success_rate = static_cast<double>(stats.completed_swaps) /
                                static_cast<double>(stats.total_swaps);
        }

        return stats;
    }

private:
    Blockchain* intcoin_chain_;
    std::string litecoin_rpc_url_;
    bool running_;
    BridgeStatus status_;

    std::unique_ptr<AtomicSwapManager> swap_manager_;
    std::unique_ptr<BridgeRelay> relay_;
    std::unique_ptr<SPVChainVerifier> ltc_verifier_;

    BridgeStats stats_;
    mutable std::mutex stats_mutex_;

    bool query_litecoin_rpc(const std::string& method,
                           const std::string& params,
                           std::string& result) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }

        std::ostringstream request_body;
        request_body << "{"
                     << "\"jsonrpc\":\"2.0\","
                     << "\"id\":\"intcoin-bridge\","
                     << "\"method\":\"" << method << "\","
                     << "\"params\":" << params
                     << "}";

        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, litecoin_rpc_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.str().c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "Litecoin RPC request failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        size_t result_pos = response.find("\"result\":");
        if (result_pos == std::string::npos) {
            return false;
        }

        result = response;
        return true;
    }

    bool get_litecoin_block_header(uint32_t height, SPVBlockHeader& header) {
        std::ostringstream params;
        params << "{\"height\":" << height << "}";

        std::string hash_result;
        if (!query_litecoin_rpc("getblockhash", params.str(), hash_result)) {
            return false;
        }

        std::ostringstream header_params;
        header_params << "{\"blockhash\":\"" << hash_result << "\",\"verbose\":false}";

        std::string header_result;
        if (!query_litecoin_rpc("getblockheader", header_params.str(), header_result)) {
            return false;
        }

        // Parse header (simplified)
        header.version = 1;
        header.timestamp = static_cast<uint32_t>(std::time(nullptr));
        header.bits = 0x1d00ffff;
        header.nonce = 0;

        return true;
    }

    void monitor_swaps() {
        std::cout << "Litecoin swap monitor started" << std::endl;

        while (running_) {
            auto pending_swaps = swap_manager_->get_pending_swaps();

            for (const auto& swap_id : pending_swaps) {
                auto swap_info = swap_manager_->get_swap(swap_id);
                if (!swap_info.has_value()) {
                    continue;
                }

                uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));
                if (current_time > swap_info->timelock) {
                    refund_swap(swap_id);
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(30));
        }

        std::cout << "Litecoin swap monitor stopped" << std::endl;
    }

    void monitor_litecoin_chain() {
        std::cout << "Litecoin chain monitor started" << std::endl;

        while (running_) {
            if (status_ == BridgeStatus::ONLINE) {
                sync_chain();
            }

            // Litecoin block time: ~2.5 minutes
            std::this_thread::sleep_for(std::chrono::seconds(150));
        }

        std::cout << "Litecoin chain monitor stopped" << std::endl;
    }
};

// Factory function to create Litecoin bridge
std::unique_ptr<Bridge> create_litecoin_bridge(
    Blockchain* intcoin_chain,
    const std::string& litecoin_rpc_url
) {
    return std::make_unique<LitecoinBridge>(intcoin_chain, litecoin_rpc_url);
}

} // namespace bridge
} // namespace intcoin
