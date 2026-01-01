// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/blockchain_monitor.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <curl/curl.h>
#include <json/json.h>

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace blockchain_monitor {

// Helper: Convert bytes to hex string
static std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// Helper: Convert hex string to bytes
static std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

// Bitcoin RPC Client
class BitcoinRPCClient {
public:
    BitcoinRPCClient(const std::string& url, const std::string& user, const std::string& password)
        : url_(url), user_(user), password_(password) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~BitcoinRPCClient() {
        curl_global_cleanup();
    }

    // Execute RPC call
    Result<Json::Value> Call(const std::string& method, const Json::Value& params) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return Result<Json::Value>::Error("Failed to initialize CURL");
        }

        // Build JSON-RPC request
        Json::Value request;
        request["jsonrpc"] = "1.0";
        request["id"] = "intcoin";
        request["method"] = method;
        request["params"] = params;

        Json::StreamWriterBuilder writer;
        std::string request_str = Json::writeString(writer, request);

        // Set up CURL
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string response_str;
        curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERPWD, (user_ + ":" + password_).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return Result<Json::Value>::Error("CURL error: " + std::string(curl_easy_strerror(res)));
        }

        // Parse JSON response
        Json::CharReaderBuilder reader;
        Json::Value response;
        std::string errors;
        std::istringstream iss(response_str);
        if (!Json::parseFromStream(reader, iss, &response, &errors)) {
            return Result<Json::Value>::Error("JSON parse error: " + errors);
        }

        if (response.isMember("error") && !response["error"].isNull()) {
            return Result<Json::Value>::Error("RPC error: " + response["error"]["message"].asString());
        }

        return Result<Json::Value>::Ok(response["result"]);
    }

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string url_;
    std::string user_;
    std::string password_;
};

// BitcoinMonitor implementation
struct BitcoinMonitor::Impl {
    std::string rpc_url;
    std::string rpc_user;
    std::string rpc_password;
    bool testnet;
    BlockchainType blockchain_type;

    std::unique_ptr<BitcoinRPCClient> rpc_client;
    std::atomic<bool> is_active;
    std::thread monitor_thread;

    // Watched HTLCs
    struct WatchedHTLC {
        std::vector<uint8_t> payment_hash;
        std::vector<uint8_t> recipient_pubkey;
        std::vector<uint8_t> refund_pubkey;
        uint64_t locktime;
    };
    std::vector<WatchedHTLC> watched_htlcs;
    std::mutex watched_htlcs_mutex;

    // Callbacks
    HTLCDetectedCallback htlc_detected_callback;
    HTLCConfirmedCallback htlc_confirmed_callback;
    HTLCClaimedCallback htlc_claimed_callback;
    HTLCRefundedCallback htlc_refunded_callback;
    std::mutex callbacks_mutex;

    Impl(const std::string& url, const std::string& user, const std::string& password, bool testnet_flag)
        : rpc_url(url), rpc_user(user), rpc_password(password), testnet(testnet_flag),
          blockchain_type(testnet_flag ? BlockchainType::TESTNET_BTC : BlockchainType::BITCOIN),
          is_active(false) {
        rpc_client = std::make_unique<BitcoinRPCClient>(url, user, password);
    }

    void MonitorLoop() {
        while (is_active) {
            // Monitor for new blocks and HTLC transactions
            try {
                auto height_result = GetCurrentBlockHeight();
                if (height_result.IsOk()) {
                    uint64_t current_height = height_result.GetValue();
                    // Scan recent blocks for HTLC transactions
                    ScanRecentBlocks(current_height);
                }
            } catch (const std::exception& e) {
                LogF(LogLevel::ERROR, "Bitcoin monitor error: %s", e.what());
            }

            // Sleep for 10 seconds between scans
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    void ScanRecentBlocks(uint64_t current_height) {
        // Scan last 6 blocks for HTLC transactions
        uint64_t start_height = current_height > 6 ? current_height - 6 : 0;

        for (uint64_t height = start_height; height <= current_height; ++height) {
            auto block_hash_result = GetBlockHash(height);
            if (block_hash_result.IsError()) continue;

            auto block_result = GetBlock(block_hash_result.GetValue());
            if (block_result.IsError()) continue;

            const Json::Value& block = block_result.GetValue();
            if (block.isMember("tx")) {
                for (const auto& txid : block["tx"]) {
                    ScanTransaction(txid.asString(), height);
                }
            }
        }
    }

    void ScanTransaction(const std::string& txid, uint64_t block_height) {
        auto tx_result = GetRawTransaction(txid);
        if (tx_result.IsError()) return;

        const Json::Value& tx = tx_result.GetValue();

        // Check each output for HTLC scripts
        if (tx.isMember("vout")) {
            for (uint32_t vout_index = 0; vout_index < tx["vout"].size(); ++vout_index) {
                const auto& vout = tx["vout"][vout_index];
                if (vout.isMember("scriptPubKey") && vout["scriptPubKey"].isMember("hex")) {
                    std::string script_hex = vout["scriptPubKey"]["hex"].asString();
                    CheckForHTLC(txid, vout_index, script_hex, vout["value"].asDouble() * 100000000, block_height);
                }
            }
        }
    }

    void CheckForHTLC(const std::string& txid, uint32_t output_index,
                     const std::string& script_hex, uint64_t amount, uint64_t block_height) {
        std::vector<uint8_t> script = HexToBytes(script_hex);

        std::lock_guard<std::mutex> lock(watched_htlcs_mutex);
        for (const auto& watched : watched_htlcs) {
            // Simple pattern matching - look for payment hash in script
            std::string hash_hex = BytesToHex(watched.payment_hash);
            if (script_hex.find(hash_hex) != std::string::npos) {
                // Found matching HTLC!
                NotifyHTLCDetected(txid, output_index, script, watched.payment_hash,
                                  watched.locktime, amount, block_height);
            }
        }
    }

    void NotifyHTLCDetected(const std::string& txid, uint32_t output_index,
                           const std::vector<uint8_t>& htlc_script,
                           const std::vector<uint8_t>& payment_hash,
                           uint64_t locktime, uint64_t amount, uint64_t block_height) {
        HTLCTransaction htlc_tx;

        // Convert txid to uint256
        std::vector<uint8_t> txid_bytes = HexToBytes(txid);
        std::copy(txid_bytes.begin(), txid_bytes.end(), htlc_tx.tx_hash.begin());

        htlc_tx.output_index = output_index;
        htlc_tx.amount = amount;
        htlc_tx.confirmations = 1; // At least 1 if in block
        htlc_tx.block_height = block_height;
        htlc_tx.status = TxStatus::CONFIRMING;
        htlc_tx.htlc_script = htlc_script;
        htlc_tx.payment_hash = payment_hash;
        htlc_tx.locktime = locktime;
        htlc_tx.claimed = false;
        htlc_tx.refunded = false;

        std::lock_guard<std::mutex> lock(callbacks_mutex);
        if (htlc_detected_callback) {
            htlc_detected_callback(htlc_tx);
        }
    }

    Result<uint64_t> GetCurrentBlockHeight() {
        Json::Value params(Json::arrayValue);
        auto result = rpc_client->Call("getblockcount", params);
        if (result.IsError()) {
            return Result<uint64_t>::Error(result.error);
        }
        return Result<uint64_t>::Ok(result.GetValue().asUInt64());
    }

    Result<std::string> GetBlockHash(uint64_t height) {
        Json::Value params(Json::arrayValue);
        params.append(Json::Value::UInt64(height));
        auto result = rpc_client->Call("getblockhash", params);
        if (result.IsError()) {
            return Result<std::string>::Error(result.error);
        }
        return Result<std::string>::Ok(result.GetValue().asString());
    }

    Result<Json::Value> GetBlock(const std::string& block_hash) {
        Json::Value params(Json::arrayValue);
        params.append(block_hash);
        params.append(2); // Verbosity 2 for full transaction data
        return rpc_client->Call("getblock", params);
    }

    Result<Json::Value> GetRawTransaction(const std::string& txid) {
        Json::Value params(Json::arrayValue);
        params.append(txid);
        params.append(true); // Verbose output
        return rpc_client->Call("getrawtransaction", params);
    }
};

BitcoinMonitor::BitcoinMonitor(const std::string& rpc_url,
                               const std::string& rpc_user,
                               const std::string& rpc_password,
                               bool testnet)
    : impl_(std::make_unique<Impl>(rpc_url, rpc_user, rpc_password, testnet)) {
}

BitcoinMonitor::~BitcoinMonitor() {
    Stop();
}

Result<void> BitcoinMonitor::Start() {
    if (impl_->is_active) {
        return Result<void>::Error("Monitor already active");
    }

    impl_->is_active = true;
    impl_->monitor_thread = std::thread([this]() {
        impl_->MonitorLoop();
    });

    LogF(LogLevel::INFO, "Bitcoin monitor started");
    return Result<void>::Ok();
}

Result<void> BitcoinMonitor::Stop() {
    if (!impl_->is_active) {
        return Result<void>::Ok();
    }

    impl_->is_active = false;
    if (impl_->monitor_thread.joinable()) {
        impl_->monitor_thread.join();
    }

    LogF(LogLevel::INFO, "Bitcoin monitor stopped");
    return Result<void>::Ok();
}

bool BitcoinMonitor::IsActive() const {
    return impl_->is_active;
}

BlockchainType BitcoinMonitor::GetBlockchainType() const {
    return impl_->blockchain_type;
}

Result<uint64_t> BitcoinMonitor::GetCurrentBlockHeight() {
    return impl_->GetCurrentBlockHeight();
}

Result<uint256> BitcoinMonitor::GetCurrentBlockHash() {
    auto height_result = impl_->GetCurrentBlockHeight();
    if (height_result.IsError()) {
        return Result<uint256>::Error(height_result.error);
    }

    auto hash_result = impl_->GetBlockHash(height_result.GetValue());
    if (hash_result.IsError()) {
        return Result<uint256>::Error(hash_result.error);
    }

    uint256 hash;
    std::vector<uint8_t> hash_bytes = HexToBytes(hash_result.GetValue());
    std::copy(hash_bytes.begin(), hash_bytes.end(), hash.begin());
    return Result<uint256>::Ok(hash);
}

Result<void> BitcoinMonitor::WatchForHTLC(
    const std::vector<uint8_t>& payment_hash,
    const std::vector<uint8_t>& recipient_pubkey,
    const std::vector<uint8_t>& refund_pubkey,
    uint64_t locktime) {

    std::lock_guard<std::mutex> lock(impl_->watched_htlcs_mutex);

    Impl::WatchedHTLC watched;
    watched.payment_hash = payment_hash;
    watched.recipient_pubkey = recipient_pubkey;
    watched.refund_pubkey = refund_pubkey;
    watched.locktime = locktime;

    impl_->watched_htlcs.push_back(watched);

    LogF(LogLevel::INFO, "Now watching for Bitcoin HTLC with payment hash: %s",
         BytesToHex(payment_hash).c_str());

    return Result<void>::Ok();
}

Result<void> BitcoinMonitor::StopWatchingHTLC(const std::vector<uint8_t>& payment_hash) {
    std::lock_guard<std::mutex> lock(impl_->watched_htlcs_mutex);

    auto it = std::remove_if(impl_->watched_htlcs.begin(), impl_->watched_htlcs.end(),
        [&payment_hash](const Impl::WatchedHTLC& watched) {
            return watched.payment_hash == payment_hash;
        });

    if (it != impl_->watched_htlcs.end()) {
        impl_->watched_htlcs.erase(it, impl_->watched_htlcs.end());
        return Result<void>::Ok();
    }

    return Result<void>::Error("HTLC not being watched");
}

Result<HTLCTransaction> BitcoinMonitor::GetHTLCTransaction(
    const uint256& tx_hash,
    uint32_t output_index) {

    std::string txid = BytesToHex(std::vector<uint8_t>(tx_hash.begin(), tx_hash.end()));

    auto tx_result = impl_->GetRawTransaction(txid);
    if (tx_result.IsError()) {
        return Result<HTLCTransaction>::Error(tx_result.error);
    }

    const Json::Value& tx = tx_result.GetValue();

    HTLCTransaction htlc_tx;
    htlc_tx.tx_hash = tx_hash;
    htlc_tx.output_index = output_index;

    if (tx.isMember("confirmations")) {
        htlc_tx.confirmations = tx["confirmations"].asUInt();
        htlc_tx.status = htlc_tx.confirmations >= 6 ? TxStatus::CONFIRMED : TxStatus::CONFIRMING;
    } else {
        htlc_tx.confirmations = 0;
        htlc_tx.status = TxStatus::PENDING;
    }

    if (tx.isMember("vout") && output_index < tx["vout"].size()) {
        const auto& vout = tx["vout"][output_index];
        htlc_tx.amount = vout["value"].asDouble() * 100000000;

        if (vout.isMember("scriptPubKey") && vout["scriptPubKey"].isMember("hex")) {
            htlc_tx.htlc_script = HexToBytes(vout["scriptPubKey"]["hex"].asString());
        }
    }

    return Result<HTLCTransaction>::Ok(htlc_tx);
}

Result<uint32_t> BitcoinMonitor::GetConfirmations(const uint256& tx_hash) {
    std::string txid = BytesToHex(std::vector<uint8_t>(tx_hash.begin(), tx_hash.end()));

    auto tx_result = impl_->GetRawTransaction(txid);
    if (tx_result.IsError()) {
        return Result<uint32_t>::Error(tx_result.error);
    }

    const Json::Value& tx = tx_result.GetValue();
    if (tx.isMember("confirmations")) {
        return Result<uint32_t>::Ok(tx["confirmations"].asUInt());
    }

    return Result<uint32_t>::Ok(0);
}

Result<std::vector<uint8_t>> BitcoinMonitor::WatchForPreimage(
    const uint256& htlc_tx_hash,
    uint32_t htlc_output_index) {

    // Query for spending transaction
    // This would need to scan the mempool and recent blocks for transactions
    // that spend the HTLC output and extract the preimage from witness data

    // TODO: Implement full preimage extraction from witness data
    return Result<std::vector<uint8_t>>::Error("Preimage watching not yet implemented");
}

Result<bool> BitcoinMonitor::IsHTLCSpent(
    const uint256& htlc_tx_hash,
    uint32_t htlc_output_index) {

    std::string txid = BytesToHex(std::vector<uint8_t>(htlc_tx_hash.begin(), htlc_tx_hash.end()));

    Json::Value params(Json::arrayValue);
    params.append(txid);
    params.append(static_cast<int>(htlc_output_index));

    auto result = impl_->rpc_client->Call("gettxout", params);
    if (result.IsError()) {
        return Result<bool>::Error(result.error);
    }

    // If gettxout returns null, the output has been spent
    bool is_spent = result.GetValue().isNull();
    return Result<bool>::Ok(is_spent);
}

Result<uint256> BitcoinMonitor::BroadcastTransaction(const std::string& raw_tx_hex) {
    Json::Value params(Json::arrayValue);
    params.append(raw_tx_hex);

    auto result = impl_->rpc_client->Call("sendrawtransaction", params);
    if (result.IsError()) {
        return Result<uint256>::Error(result.error);
    }

    std::string txid = result.GetValue().asString();
    uint256 tx_hash;
    std::vector<uint8_t> txid_bytes = HexToBytes(txid);
    std::copy(txid_bytes.begin(), txid_bytes.end(), tx_hash.begin());

    return Result<uint256>::Ok(tx_hash);
}

void BitcoinMonitor::OnHTLCDetected(HTLCDetectedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    impl_->htlc_detected_callback = callback;
}

void BitcoinMonitor::OnHTLCConfirmed(HTLCConfirmedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    impl_->htlc_confirmed_callback = callback;
}

void BitcoinMonitor::OnHTLCClaimed(HTLCClaimedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    impl_->htlc_claimed_callback = callback;
}

void BitcoinMonitor::OnHTLCRefunded(HTLCRefundedCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    impl_->htlc_refunded_callback = callback;
}

}  // namespace blockchain_monitor
}  // namespace intcoin
