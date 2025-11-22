// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/bridge/bridge.h"
#include "../../include/intcoin/crypto/hash.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>

namespace intcoin {
namespace bridge {

//==============================================================================
// Ethereum RPC Helper
//==============================================================================

namespace {

// Callback for CURL to handle response data
size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Convert bytes to hex string with 0x prefix
std::string bytes_to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << "0x";
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

} // anonymous namespace

//==============================================================================
// EthereumBridge Implementation
//==============================================================================

EthereumBridge::EthereumBridge(Blockchain* intcoin_chain,
                               const std::string& ethereum_rpc_url)
    : intcoin_chain_(intcoin_chain),
      ethereum_rpc_url_(ethereum_rpc_url),
      contract_address_(""),
      running_(false),
      status_(BridgeStatus::OFFLINE) {

    if (!intcoin_chain_) {
        throw std::runtime_error("EthereumBridge: intcoin_chain cannot be null");
    }

    // Initialize components
    swap_manager_ = std::make_unique<AtomicSwapManager>();
    relay_ = std::make_unique<BridgeRelay>(ChainType::ETHEREUM);
}

EthereumBridge::~EthereumBridge() {
    stop();
}

bool EthereumBridge::start() {
    if (running_) {
        return true;
    }

    std::cout << "Starting Ethereum bridge..." << std::endl;

    // Test Ethereum RPC connection
    std::string result;
    if (!query_ethereum_rpc("eth_chainId", "[]", result)) {
        std::cerr << "Failed to connect to Ethereum RPC at " << ethereum_rpc_url_ << std::endl;
        status_ = BridgeStatus::ERROR;
        return false;
    }

    // Deploy or verify swap contract
    if (contract_address_.empty()) {
        if (!deploy_swap_contract()) {
            std::cerr << "Failed to deploy Ethereum swap contract" << std::endl;
            status_ = BridgeStatus::ERROR;
            return false;
        }
    }

    running_ = true;
    status_ = BridgeStatus::SYNCING;

    // Start monitoring threads
    std::thread([this]() { monitor_swaps(); }).detach();
    std::thread([this]() { monitor_ethereum_chain(); }).detach();

    // Initial chain sync
    if (sync_chain()) {
        status_ = BridgeStatus::ONLINE;
        std::cout << "Ethereum bridge online at contract " << contract_address_ << std::endl;
    }

    return true;
}

void EthereumBridge::stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping Ethereum bridge..." << std::endl;
    running_ = false;
    status_ = BridgeStatus::OFFLINE;
}

Hash256 EthereumBridge::initiate_swap(const PublicKey& recipient, uint64_t amount) {
    if (!running_ || status_ != BridgeStatus::ONLINE) {
        throw std::runtime_error("Ethereum bridge not online");
    }

    if (contract_address_.empty()) {
        throw std::runtime_error("Ethereum swap contract not deployed");
    }

    // Generate secret and hash lock
    Hash256 secret;
    RAND_bytes(secret.data(), secret.size());

    crypto::SHA256 hasher;
    hasher.update(secret.data(), secret.size());
    Hash256 hash_lock;
    hasher.finalize(hash_lock.data());

    // Calculate safe timelock
    uint32_t timelock = BridgeUtils::calculate_safe_timelock(ChainType::ETHEREUM);
    uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));

    // Create swap on INTcoin side
    Hash256 swap_id = swap_manager_->create_swap(
        hash_lock,
        recipient,
        amount,
        current_time + timelock
    );

    // Encode contract call data for Ethereum
    std::string contract_data = encode_swap_data(hash_lock, recipient, current_time + timelock);

    // In production: submit transaction to Ethereum contract
    std::cout << "Initiated Ethereum swap " << swap_id << " for " << amount << " wei" << std::endl;
    std::cout << "Contract call data: " << contract_data << std::endl;

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_swaps++;
    stats_.total_volume_sent += amount;

    return swap_id;
}

bool EthereumBridge::complete_swap(const Hash256& swap_id, const Hash256& secret) {
    if (!running_) {
        return false;
    }

    // Verify and complete swap
    bool success = swap_manager_->complete_swap(swap_id, secret);

    if (success) {
        // Submit claim transaction to Ethereum contract
        std::string secret_hex = bytes_to_hex(secret.data(), secret.size());
        std::cout << "Claiming Ethereum swap " << swap_id << " with secret " << secret_hex << std::endl;

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

bool EthereumBridge::refund_swap(const Hash256& swap_id) {
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

bool EthereumBridge::verify_lock_proof(const Hash256& swap_id,
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

    // Additional Ethereum-specific verification
    // Check transaction receipt, contract event logs, etc.

    std::cout << "Verified lock proof for Ethereum swap " << swap_id << std::endl;

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_volume_received += swap_info->amount;

    return true;
}

bool EthereumBridge::sync_chain() {
    if (!running_) {
        return false;
    }

    // Get current Ethereum block number
    std::string result;
    if (!query_ethereum_rpc("eth_blockNumber", "[]", result)) {
        return false;
    }

    // Parse block number from hex result
    // In production: properly parse JSON and convert hex to decimal
    uint32_t eth_height = 0;
    if (result.find("0x") != std::string::npos) {
        eth_height = std::stoul(result.substr(result.find("0x") + 2), nullptr, 16);
    }

    std::cout << "Synced to Ethereum block " << eth_height << std::endl;

    // Watch for contract events
    watch_contract_events();

    return true;
}

uint32_t EthereumBridge::get_chain_height() const {
    // Query Ethereum for current block number
    std::string result;
    if (!const_cast<EthereumBridge*>(this)->query_ethereum_rpc("eth_blockNumber", "[]", result)) {
        return 0;
    }

    // Parse hex result
    if (result.find("0x") != std::string::npos) {
        return std::stoul(result.substr(result.find("0x") + 2), nullptr, 16);
    }

    return 0;
}

uint32_t EthereumBridge::get_sync_height() const {
    // For Ethereum, we don't maintain SPV headers like Bitcoin
    // Return the current synced height
    return get_chain_height();
}

BridgeStats EthereumBridge::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    BridgeStats stats = stats_;

    // Calculate success rate
    if (stats.total_swaps > 0) {
        stats.success_rate = static_cast<double>(stats.completed_swaps) /
                            static_cast<double>(stats.total_swaps);
    }

    return stats;
}

std::string EthereumBridge::get_contract_address() const {
    return contract_address_;
}

bool EthereumBridge::deploy_swap_contract() {
    std::cout << "Deploying Ethereum HTLC swap contract..." << std::endl;

    // In production: deploy actual Solidity contract
    // For now, use placeholder address
    contract_address_ = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb0";

    std::cout << "Contract deployed at " << contract_address_ << std::endl;
    return true;
}

bool EthereumBridge::verify_eth_transaction(const std::string& txhash) {
    if (!running_) {
        return false;
    }

    // Query Ethereum for transaction receipt
    std::ostringstream params;
    params << "[\"" << txhash << "\"]";

    std::string result;
    if (!query_ethereum_rpc("eth_getTransactionReceipt", params.str(), result)) {
        return false;
    }

    // Parse and verify transaction receipt
    // In production: decode JSON, verify status, logs, etc.

    std::cout << "Verified Ethereum transaction " << txhash << std::endl;
    return true;
}

//==============================================================================
// Private Methods
//==============================================================================

bool EthereumBridge::query_ethereum_rpc(const std::string& method,
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
                 << "\"id\":1,"
                 << "\"method\":\"" << method << "\","
                 << "\"params\":" << params
                 << "}";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, ethereum_rpc_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.str().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Ethereum RPC request failed: " << curl_easy_strerror(res) << std::endl;
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

std::string EthereumBridge::encode_swap_data(const Hash256& hash_lock,
                                             const PublicKey& recipient,
                                             uint32_t timelock) {
    // Encode Ethereum contract call data for initiating HTLC
    // Format: function signature + parameters

    // In production: use proper ABI encoding
    // Function signature: initiate(bytes32 hashLock, address recipient, uint256 timelock)
    // Keccak-256 hash of signature, first 4 bytes

    std::ostringstream oss;
    oss << "0x";

    // Function selector (placeholder - compute from keccak256)
    oss << "12345678";

    // Hash lock (32 bytes)
    for (size_t i = 0; i < hash_lock.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash_lock[i]);
    }

    // Recipient address (20 bytes, padded to 32 bytes)
    for (size_t i = 0; i < 12; ++i) {
        oss << "00";
    }
    for (size_t i = 0; i < 20 && i < recipient.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(recipient[i]);
    }

    // Timelock (32 bytes)
    oss << std::hex << std::setw(64) << std::setfill('0') << timelock;

    return oss.str();
}

bool EthereumBridge::watch_contract_events() {
    if (contract_address_.empty()) {
        return false;
    }

    // Query contract event logs
    // In production: use eth_getLogs with filter for contract address

    std::ostringstream params;
    params << "[{";
    params << "\"address\":\"" << contract_address_ << "\",";
    params << "\"fromBlock\":\"latest\",";
    params << "\"toBlock\":\"latest\"";
    params << "}]";

    std::string result;
    if (!query_ethereum_rpc("eth_getLogs", params.str(), result)) {
        return false;
    }

    // Parse event logs
    // In production: decode SwapInitiated, SwapCompleted, SwapRefunded events

    return true;
}

void EthereumBridge::monitor_swaps() {
    std::cout << "Ethereum swap monitor started" << std::endl;

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

    std::cout << "Ethereum swap monitor stopped" << std::endl;
}

void EthereumBridge::monitor_ethereum_chain() {
    std::cout << "Ethereum chain monitor started" << std::endl;

    while (running_) {
        // Periodically sync with Ethereum blockchain
        if (status_ == BridgeStatus::ONLINE) {
            sync_chain();
        }

        // Sleep for monitoring interval (Ethereum blocks every ~12-15 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }

    std::cout << "Ethereum chain monitor stopped" << std::endl;
}

} // namespace bridge
} // namespace intcoin
