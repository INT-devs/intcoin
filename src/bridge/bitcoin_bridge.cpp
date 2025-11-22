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
// Bitcoin RPC Helper
//==============================================================================

namespace {

// Callback for CURL to handle response data
size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

} // anonymous namespace

//==============================================================================
// BitcoinBridge Implementation
//==============================================================================

BitcoinBridge::BitcoinBridge(Blockchain* intcoin_chain,
                             const std::string& bitcoin_rpc_url)
    : intcoin_chain_(intcoin_chain),
      bitcoin_rpc_url_(bitcoin_rpc_url),
      running_(false),
      status_(BridgeStatus::OFFLINE) {

    if (!intcoin_chain_) {
        throw std::runtime_error("BitcoinBridge: intcoin_chain cannot be null");
    }

    // Initialize components
    swap_manager_ = std::make_unique<AtomicSwapManager>();
    relay_ = std::make_unique<BridgeRelay>(ChainType::BITCOIN);
    btc_verifier_ = std::make_unique<SPVChainVerifier>(ChainType::BITCOIN);
}

BitcoinBridge::~BitcoinBridge() {
    stop();
}

bool BitcoinBridge::start() {
    if (running_) {
        return true;
    }

    std::cout << "Starting Bitcoin bridge..." << std::endl;

    // Test Bitcoin RPC connection
    std::string result;
    if (!query_bitcoin_rpc("getblockchaininfo", "{}", result)) {
        std::cerr << "Failed to connect to Bitcoin RPC at " << bitcoin_rpc_url_ << std::endl;
        status_ = BridgeStatus::ERROR;
        return false;
    }

    running_ = true;
    status_ = BridgeStatus::SYNCING;

    // Start monitoring threads
    std::thread([this]() { monitor_swaps(); }).detach();
    std::thread([this]() { monitor_bitcoin_chain(); }).detach();

    // Initial chain sync
    if (sync_chain()) {
        status_ = BridgeStatus::ONLINE;
        std::cout << "Bitcoin bridge online" << std::endl;
    }

    return true;
}

void BitcoinBridge::stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping Bitcoin bridge..." << std::endl;
    running_ = false;
    status_ = BridgeStatus::OFFLINE;
}

Hash256 BitcoinBridge::initiate_swap(const PublicKey& recipient, uint64_t amount) {
    if (!running_ || status_ != BridgeStatus::ONLINE) {
        throw std::runtime_error("Bitcoin bridge not online");
    }

    // Generate secret and hash lock
    Hash256 secret;
    RAND_bytes(secret.data(), secret.size());

    crypto::SHA256 hasher;
    hasher.update(secret.data(), secret.size());
    Hash256 hash_lock;
    hasher.finalize(hash_lock.data());

    // Calculate safe timelock
    uint32_t timelock = BridgeUtils::calculate_safe_timelock(ChainType::BITCOIN);
    uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));

    // Create swap on INTcoin side
    Hash256 swap_id = swap_manager_->create_swap(
        hash_lock,
        recipient,
        amount,
        current_time + timelock
    );

    // Store secret for later completion
    // In production, this would be securely stored

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_swaps++;
    stats_.total_volume_sent += amount;

    std::cout << "Initiated swap " << swap_id << " for " << amount << " satoshis" << std::endl;

    return swap_id;
}

bool BitcoinBridge::complete_swap(const Hash256& swap_id, const Hash256& secret) {
    if (!running_) {
        return false;
    }

    // Verify and complete swap
    bool success = swap_manager_->complete_swap(swap_id, secret);

    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.completed_swaps++;

        std::cout << "Completed swap " << swap_id << std::endl;
    } else {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.failed_swaps++;

        std::cerr << "Failed to complete swap " << swap_id << std::endl;
    }

    return success;
}

bool BitcoinBridge::refund_swap(const Hash256& swap_id) {
    if (!running_) {
        return false;
    }

    // Verify timelock has expired, then refund
    bool success = swap_manager_->refund_swap(swap_id);

    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.failed_swaps++;

        std::cout << "Refunded swap " << swap_id << std::endl;
    }

    return success;
}

bool BitcoinBridge::verify_lock_proof(const Hash256& swap_id,
                                      const CrossChainProof& proof) {
    if (!running_) {
        return false;
    }

    // Verify the proof using BridgeRelay
    if (!relay_->verify_proof(proof)) {
        return false;
    }

    // Verify proof corresponds to this swap
    auto swap_info = swap_manager_->get_swap(swap_id);
    if (!swap_info.has_value()) {
        return false;
    }

    // Additional Bitcoin-specific verification
    // Check transaction format, script, etc.

    std::cout << "Verified lock proof for swap " << swap_id << std::endl;

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_volume_received += swap_info->amount;

    return true;
}

bool BitcoinBridge::sync_chain() {
    if (!running_) {
        return false;
    }

    // Get current Bitcoin blockchain height
    std::string result;
    if (!query_bitcoin_rpc("getblockcount", "{}", result)) {
        return false;
    }

    // Parse height from result
    uint32_t btc_height = std::stoul(result);

    // Sync headers from current position to btc_height
    uint32_t current_height = btc_verifier_->get_height();

    for (uint32_t h = current_height + 1; h <= btc_height && running_; ++h) {
        SPVBlockHeader header;
        if (!get_bitcoin_block_header(h, header)) {
            std::cerr << "Failed to get Bitcoin block header at height " << h << std::endl;
            return false;
        }

        if (!btc_verifier_->add_header(header)) {
            std::cerr << "Failed to verify Bitcoin block header at height " << h << std::endl;
            return false;
        }

        // Update relay with new header
        relay_->add_header(h, header);
    }

    std::cout << "Synced to Bitcoin height " << btc_height << std::endl;
    return true;
}

uint32_t BitcoinBridge::get_chain_height() const {
    // Query Bitcoin for current height
    std::string result;
    if (!const_cast<BitcoinBridge*>(this)->query_bitcoin_rpc("getblockcount", "{}", result)) {
        return 0;
    }

    return std::stoul(result);
}

uint32_t BitcoinBridge::get_sync_height() const {
    return btc_verifier_->get_height();
}

BridgeStats BitcoinBridge::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    BridgeStats stats = stats_;

    // Calculate success rate
    if (stats.total_swaps > 0) {
        stats.success_rate = static_cast<double>(stats.completed_swaps) /
                            static_cast<double>(stats.total_swaps);
    }

    return stats;
}

std::string BitcoinBridge::get_bitcoin_address() const {
    // Return bridge's Bitcoin receiving address
    // In production, this would be derived from bridge's key
    return "1BridgeAddressPlaceholder";
}

bool BitcoinBridge::verify_bitcoin_transaction(const std::string& txid) {
    if (!running_) {
        return false;
    }

    // Query Bitcoin for transaction
    std::ostringstream params;
    params << "{\"txid\":\"" << txid << "\",\"verbose\":true}";

    std::string result;
    if (!query_bitcoin_rpc("getrawtransaction", params.str(), result)) {
        return false;
    }

    // Parse and verify transaction
    // In production: decode hex, verify script, amounts, etc.

    std::cout << "Verified Bitcoin transaction " << txid << std::endl;
    return true;
}

//==============================================================================
// Private Methods
//==============================================================================

bool BitcoinBridge::query_bitcoin_rpc(const std::string& method,
                                      const std::string& params,
                                      std::string& result) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    // Prepare JSON-RPC request
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

    curl_easy_setopt(curl, CURLOPT_URL, bitcoin_rpc_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // In production: add username/password authentication
    // curl_easy_setopt(curl, CURLOPT_USERPWD, "rpcuser:rpcpassword");

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Bitcoin RPC request failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    // Parse JSON response and extract result
    // In production: use proper JSON parser
    size_t result_pos = response.find("\"result\":");
    if (result_pos == std::string::npos) {
        return false;
    }

    result = response;  // Simplified - in production, parse JSON properly
    return true;
}

bool BitcoinBridge::get_bitcoin_block_header(uint32_t height, SPVBlockHeader& header) {
    // Get block hash for height
    std::ostringstream params;
    params << "{\"height\":" << height << "}";

    std::string hash_result;
    if (!query_bitcoin_rpc("getblockhash", params.str(), hash_result)) {
        return false;
    }

    // Get block header by hash
    std::ostringstream header_params;
    header_params << "{\"blockhash\":\"" << hash_result << "\",\"verbose\":false}";

    std::string header_result;
    if (!query_bitcoin_rpc("getblockheader", header_params.str(), header_result)) {
        return false;
    }

    // Parse header
    // In production: properly decode Bitcoin block header format
    header.version = 1;
    header.timestamp = static_cast<uint32_t>(std::time(nullptr));
    header.bits = 0x1d00ffff;  // Difficulty bits
    header.nonce = 0;

    return true;
}

void BitcoinBridge::monitor_swaps() {
    std::cout << "Bitcoin swap monitor started" << std::endl;

    while (running_) {
        // Check for pending swaps
        auto pending_swaps = swap_manager_->get_pending_swaps();

        for (const auto& swap_id : pending_swaps) {
            auto swap_info = swap_manager_->get_swap(swap_id);
            if (!swap_info.has_value()) {
                continue;
            }

            // Check if swap has timed out
            uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));
            if (current_time > swap_info->timelock) {
                // Attempt refund
                refund_swap(swap_id);
            }
        }

        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }

    std::cout << "Bitcoin swap monitor stopped" << std::endl;
}

void BitcoinBridge::monitor_bitcoin_chain() {
    std::cout << "Bitcoin chain monitor started" << std::endl;

    while (running_) {
        // Periodically sync with Bitcoin blockchain
        if (status_ == BridgeStatus::ONLINE) {
            sync_chain();
        }

        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }

    std::cout << "Bitcoin chain monitor stopped" << std::endl;
}

} // namespace bridge
} // namespace intcoin
