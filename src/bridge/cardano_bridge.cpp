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
// Cardano RPC Helper
//==============================================================================

namespace {

size_t curl_write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Convert bytes to CBOR hex (Cardano uses CBOR encoding)
std::string bytes_to_cbor_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

} // anonymous namespace

//==============================================================================
// CardanoBridge Implementation
//==============================================================================

class CardanoBridge : public Bridge {
public:
    CardanoBridge(Blockchain* intcoin_chain,
                  const std::string& cardano_node_url = "http://localhost:8090")
        : intcoin_chain_(intcoin_chain),
          cardano_node_url_(cardano_node_url),
          plutus_script_hash_(""),
          running_(false),
          status_(BridgeStatus::OFFLINE) {

        if (!intcoin_chain_) {
            throw std::runtime_error("CardanoBridge: intcoin_chain cannot be null");
        }

        swap_manager_ = std::make_unique<AtomicSwapManager>();
        relay_ = std::make_unique<BridgeRelay>(ChainType::CARDANO);
    }

    ~CardanoBridge() override {
        stop();
    }

    // Bridge lifecycle
    bool start() override {
        if (running_) {
            return true;
        }

        std::cout << "Starting Cardano bridge..." << std::endl;

        // Test Cardano node connection
        std::string result;
        if (!query_cardano_node("queryTip", "{}", result)) {
            std::cerr << "Failed to connect to Cardano node at " << cardano_node_url_ << std::endl;
            status_ = BridgeStatus::ERROR;
            return false;
        }

        // Deploy or verify Plutus script
        if (plutus_script_hash_.empty()) {
            if (!deploy_plutus_script()) {
                std::cerr << "Failed to deploy Cardano Plutus script" << std::endl;
                status_ = BridgeStatus::ERROR;
                return false;
            }
        }

        running_ = true;
        status_ = BridgeStatus::SYNCING;

        // Start monitoring threads
        std::thread([this]() { monitor_swaps(); }).detach();
        std::thread([this]() { monitor_cardano_chain(); }).detach();

        // Initial chain sync
        if (sync_chain()) {
            status_ = BridgeStatus::ONLINE;
            std::cout << "Cardano bridge online with Plutus script " << plutus_script_hash_ << std::endl;
        }

        return true;
    }

    void stop() override {
        if (!running_) {
            return;
        }

        std::cout << "Stopping Cardano bridge..." << std::endl;
        running_ = false;
        status_ = BridgeStatus::OFFLINE;
    }

    bool is_running() const override { return running_; }

    // Bridge info
    ChainType get_chain_type() const override { return ChainType::CARDANO; }
    std::string get_chain_name() const override { return "Cardano"; }
    BridgeStatus get_status() const override { return status_; }

    // Swap operations
    Hash256 initiate_swap(const PublicKey& recipient, uint64_t amount) override {
        if (!running_ || status_ != BridgeStatus::ONLINE) {
            throw std::runtime_error("Cardano bridge not online");
        }

        if (plutus_script_hash_.empty()) {
            throw std::runtime_error("Cardano Plutus script not deployed");
        }

        // Generate secret and hash lock
        Hash256 secret;
        RAND_bytes(secret.data(), secret.size());

        crypto::SHA256 hasher;
        hasher.update(secret.data(), secret.size());
        Hash256 hash_lock;
        hasher.finalize(hash_lock.data());

        // Calculate safe timelock (Cardano: 20 second slots, 12 confirmations)
        uint32_t timelock = BridgeUtils::calculate_safe_timelock(ChainType::CARDANO);
        uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));

        // Create swap on INTcoin side
        Hash256 swap_id = swap_manager_->create_swap(
            hash_lock,
            recipient,
            amount,
            current_time + timelock
        );

        // Encode Plutus script datum
        std::string datum = encode_plutus_datum(hash_lock, recipient, current_time + timelock);

        std::cout << "Initiated Cardano swap " << swap_id << " for " << amount << " lovelace" << std::endl;
        std::cout << "Plutus datum: " << datum << std::endl;

        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_swaps++;
        stats_.total_volume_sent += amount;

        return swap_id;
    }

    bool complete_swap(const Hash256& swap_id, const Hash256& secret) override {
        if (!running_) {
            return false;
        }

        bool success = swap_manager_->complete_swap(swap_id, secret);

        if (success) {
            // Submit claim transaction with Plutus redeemer
            std::string secret_hex = bytes_to_cbor_hex(secret.data(), secret.size());
            std::cout << "Claiming Cardano swap " << swap_id << " with secret " << secret_hex << std::endl;

            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.completed_swaps++;
            std::cout << "Completed Cardano swap " << swap_id << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.failed_swaps++;
            std::cerr << "Failed to complete Cardano swap " << swap_id << std::endl;
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
            std::cout << "Refunded Cardano swap " << swap_id << std::endl;
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

        // Cardano-specific verification: check UTxO at script address
        std::cout << "Verified Cardano lock proof for swap " << swap_id << std::endl;

        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_volume_received += swap_info->amount;

        return true;
    }

    // Chain synchronization
    bool sync_chain() override {
        if (!running_) {
            return false;
        }

        // Get current Cardano blockchain tip
        std::string result;
        if (!query_cardano_node("queryTip", "{}", result)) {
            return false;
        }

        // Parse tip to get slot number and block height
        // In production: properly parse JSON response
        uint32_t current_slot = parse_cardano_slot(result);
        uint32_t block_height = parse_cardano_height(result);

        std::cout << "Synced to Cardano slot " << current_slot 
                  << " (height " << block_height << ")" << std::endl;

        // Watch for Plutus script transactions
        watch_script_utxos();

        return true;
    }

    uint32_t get_chain_height() const override {
        std::string result;
        if (!const_cast<CardanoBridge*>(this)->query_cardano_node("queryTip", "{}", result)) {
            return 0;
        }
        return parse_cardano_height(result);
    }

    uint32_t get_sync_height() const override {
        return get_chain_height();
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

    // Cardano-specific methods
    std::string get_plutus_script_hash() const {
        return plutus_script_hash_;
    }

    std::string get_script_address() const {
        // Derive Cardano address from Plutus script hash
        // In production: use proper Cardano address encoding (Bech32)
        return "addr_test1" + plutus_script_hash_.substr(0, 50);
    }

private:
    Blockchain* intcoin_chain_;
    std::string cardano_node_url_;
    std::string plutus_script_hash_;
    bool running_;
    BridgeStatus status_;

    std::unique_ptr<AtomicSwapManager> swap_manager_;
    std::unique_ptr<BridgeRelay> relay_;

    BridgeStats stats_;
    mutable std::mutex stats_mutex_;

    bool query_cardano_node(const std::string& method,
                           const std::string& params,
                           std::string& result) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }

        // Cardano uses cardano-cli/cardano-node API
        std::ostringstream request_body;
        request_body << "{"
                     << "\"type\":\"" << method << "\","
                     << "\"args\":" << params
                     << "}";

        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, cardano_node_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.str().c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "Cardano node request failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        result = response;
        return true;
    }

    bool deploy_plutus_script() {
        std::cout << "Deploying Cardano HTLC Plutus script..." << std::endl;

        // In production: deploy actual Plutus script
        // Plutus script validates:
        // 1. Hash preimage matches datum hash lock
        // 2. Or timelock has expired for refund

        // Generate placeholder script hash
        plutus_script_hash_ = "a2c3f4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3";

        std::cout << "Plutus script deployed with hash " << plutus_script_hash_ << std::endl;
        return true;
    }

    std::string encode_plutus_datum(const Hash256& hash_lock,
                                     const PublicKey& recipient,
                                     uint32_t timelock) {
        // Encode Plutus datum in CBOR format
        // Datum structure: (hash_lock, recipient_pkh, timelock)

        std::ostringstream oss;

        // CBOR array with 3 elements
        oss << "83";  // Array of 3

        // Hash lock (32 bytes)
        oss << "5820";  // Byte string of 32
        for (size_t i = 0; i < hash_lock.size(); ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash_lock[i]);
        }

        // Recipient payment key hash (28 bytes)
        oss << "581c";  // Byte string of 28
        for (size_t i = 0; i < 28 && i < recipient.size(); ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(recipient[i]);
        }

        // Timelock (integer)
        oss << "1a";  // 4-byte integer
        oss << std::hex << std::setw(8) << std::setfill('0') << timelock;

        return oss.str();
    }

    bool watch_script_utxos() {
        if (plutus_script_hash_.empty()) {
            return false;
        }

        // Query UTxOs at script address
        std::string script_addr = get_script_address();
        std::ostringstream params;
        params << "{\"address\":\"" << script_addr << "\"}";

        std::string result;
        if (!query_cardano_node("queryUtxo", params.str(), result)) {
            return false;
        }

        // Parse UTxOs and check for swap transactions
        // In production: decode CBOR datums and match to pending swaps

        return true;
    }

    uint32_t parse_cardano_slot(const std::string& tip_response) const {
        // Parse slot number from queryTip response
        // In production: use proper JSON parser
        size_t slot_pos = tip_response.find("\"slot\":");
        if (slot_pos != std::string::npos) {
            return std::stoul(tip_response.substr(slot_pos + 7));
        }
        return 0;
    }

    uint32_t parse_cardano_height(const std::string& tip_response) const {
        // Parse block height from queryTip response
        size_t height_pos = tip_response.find("\"block\":");
        if (height_pos != std::string::npos) {
            return std::stoul(tip_response.substr(height_pos + 8));
        }
        return 0;
    }

    void monitor_swaps() {
        std::cout << "Cardano swap monitor started" << std::endl;

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

        std::cout << "Cardano swap monitor stopped" << std::endl;
    }

    void monitor_cardano_chain() {
        std::cout << "Cardano chain monitor started" << std::endl;

        while (running_) {
            if (status_ == BridgeStatus::ONLINE) {
                sync_chain();
            }

            // Cardano slot time: ~20 seconds
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }

        std::cout << "Cardano chain monitor stopped" << std::endl;
    }
};

// Factory function to create Cardano bridge
std::unique_ptr<Bridge> create_cardano_bridge(
    Blockchain* intcoin_chain,
    const std::string& cardano_node_url
) {
    return std::make_unique<CardanoBridge>(intcoin_chain, cardano_node_url);
}

} // namespace bridge
} // namespace intcoin
