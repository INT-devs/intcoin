// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/explorer.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace intcoin {
namespace explorer {

// ============================================================================
// Utility Functions Implementation
// ============================================================================

uint64_t CountActiveAddresses(const Blockchain& blockchain) {
    // Count addresses with non-zero balance
    // This is an expensive operation, should be cached
    uint64_t count = 0;

    // In a real implementation, we would iterate through UTXO set
    // and count unique addresses
    // For now, return estimated value
    count = blockchain.GetTotalTransactions() / 10; // Rough estimate

    return count;
}

double CalculateAverageBlockTime(const Blockchain& blockchain, uint32_t num_blocks) {
    uint64_t current_height = blockchain.GetBestHeight();
    if (current_height < num_blocks) {
        num_blocks = current_height;
    }

    if (num_blocks < 2) {
        return 120.0; // Default 2 minutes
    }

    auto latest_result = blockchain.GetBlockByHeight(current_height);
    auto oldest_result = blockchain.GetBlockByHeight(current_height - num_blocks);

    if (!latest_result.IsOk() || !oldest_result.IsOk()) {
        return 120.0;
    }

    Block latest = latest_result.GetValue();
    Block oldest = oldest_result.GetValue();

    uint64_t time_diff = latest.header.timestamp - oldest.header.timestamp;
    return static_cast<double>(time_diff) / static_cast<double>(num_blocks);
}

std::vector<std::string> ExtractAddresses(const Transaction& tx) {
    std::vector<std::string> addresses;

    // Extract from outputs
    for (const auto& output : tx.outputs) {
        // Parse script_pubkey to extract address
        // For now, simplified implementation
        if (output.script_pubkey.bytes.size() > 0) {
            // Would need proper script parsing here
            addresses.push_back("int1q..."); // Placeholder
        }
    }

    return addresses;
}

std::string FormatAmount(uint64_t ints) {
    double int_value = IntsToInt(ints);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);

    // Add comma separators
    std::string num_str = std::to_string(static_cast<uint64_t>(int_value));
    int insert_pos = num_str.length() - 3;
    while (insert_pos > 0) {
        num_str.insert(insert_pos, ",");
        insert_pos -= 3;
    }

    oss << num_str;

    // Add decimal part
    double decimal = int_value - static_cast<uint64_t>(int_value);
    if (decimal > 0.000001) {
        oss << std::setprecision(6) << decimal;
    }

    oss << " INT";
    return oss.str();
}

std::string FormatTimestamp(uint64_t unix_timestamp) {
    std::time_t time = static_cast<std::time_t>(unix_timestamp);
    std::tm* tm_info = std::gmtime(&time);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return std::string(buffer);
}

// ============================================================================
// BlockExplorer Implementation
// ============================================================================

BlockExplorer::BlockExplorer(const ExplorerConfig& config)
    : config_(config) {
}

BlockExplorer::~BlockExplorer() {
    Stop();
}

Result<void> BlockExplorer::Start(Blockchain& blockchain) {
    if (running_.load()) {
        return Result<void>::Error("Explorer already running");
    }

    blockchain_ = &blockchain;
    running_.store(true);

    std::cout << "Starting Block Explorer on " << config_.host << ":"
              << config_.port << "\n";

    // Start HTTP server
    http_thread_ = std::make_unique<std::thread>(&BlockExplorer::HTTPServerLoop, this);

    // Start WebSocket server (if enabled)
    if (config_.enable_websocket) {
        websocket_thread_ = std::make_unique<std::thread>(
            &BlockExplorer::WebSocketServerLoop, this);
    }

    // Start rich list updater
    rich_list_thread_ = std::make_unique<std::thread>(
        &BlockExplorer::RichListUpdateLoop, this);

    // Start stats cache updater
    stats_cache_thread_ = std::make_unique<std::thread>(
        &BlockExplorer::StatsCacheUpdateLoop, this);

    std::cout << "✓ Block Explorer started successfully\n";

    return Result<void>::Ok();
}

void BlockExplorer::Stop() {
    if (!running_.load()) {
        return;
    }

    std::cout << "Stopping Block Explorer...\n";

    running_.store(false);
    stop_requested_.store(true);

    // Stop HTTP server
    if (http_socket_ >= 0) {
        close(http_socket_);
        http_socket_ = -1;
    }

    if (http_thread_ && http_thread_->joinable()) {
        http_thread_->join();
    }

    // Stop WebSocket server
    if (websocket_socket_ >= 0) {
        close(websocket_socket_);
        websocket_socket_ = -1;
    }

    if (websocket_thread_ && websocket_thread_->joinable()) {
        websocket_thread_->join();
    }

    // Close all WebSocket clients
    {
        std::lock_guard<std::mutex> lock(websocket_clients_mutex_);
        for (int client : websocket_clients_) {
            close(client);
        }
        websocket_clients_.clear();
    }

    // Stop background threads
    if (rich_list_thread_ && rich_list_thread_->joinable()) {
        rich_list_thread_->join();
    }

    if (stats_cache_thread_ && stats_cache_thread_->joinable()) {
        stats_cache_thread_->join();
    }

    std::cout << "✓ Block Explorer stopped\n";
}

// ============================================================================
// Block Queries
// ============================================================================

Result<BlockSummary> BlockExplorer::GetBlockSummary(const uint256& hash) {
    auto block_result = blockchain_->GetBlock(hash);
    if (!block_result.IsOk()) {
        return Result<BlockSummary>::Error("Block not found");
    }

    Block block = block_result.GetValue();

    BlockSummary summary;
    summary.hash = hash;
    summary.height = blockchain_->GetBlockConfirmations(hash);
    summary.timestamp = block.header.timestamp;
    summary.tx_count = block.transactions.size();

    // Calculate total amount
    uint64_t total = 0;
    for (const auto& tx : block.transactions) {
        total += tx.GetTotalOutputValue();
    }
    summary.total_amount = total;

    // Block reward (first tx is coinbase)
    if (!block.transactions.empty()) {
        summary.block_reward = block.transactions[0].GetTotalOutputValue();
    }

    summary.size = block.GetSerializedSize();
    summary.difficulty = blockchain_->GetDifficulty();

    // Extract miner address from coinbase
    if (!block.transactions.empty()) {
        auto addresses = ExtractAddresses(block.transactions[0]);
        if (!addresses.empty()) {
            summary.miner = addresses[0];
        }
    }

    return Result<BlockSummary>::Ok(summary);
}

Result<BlockSummary> BlockExplorer::GetBlockSummaryByHeight(uint64_t height) {
    auto block_result = blockchain_->GetBlockByHeight(height);
    if (!block_result.IsOk()) {
        return Result<BlockSummary>::Error("Block not found at height");
    }

    Block block = block_result.GetValue();
    return GetBlockSummary(block.GetHash());
}

Result<std::vector<BlockSummary>> BlockExplorer::GetRecentBlocks(uint32_t page) {
    std::vector<BlockSummary> blocks;

    uint64_t best_height = blockchain_->GetBestHeight();
    uint32_t start = page * config_.blocks_per_page;

    if (start > best_height) {
        return Result<std::vector<BlockSummary>>::Ok(blocks);
    }

    uint64_t end_height = best_height - start;
    uint64_t start_height = (end_height > config_.blocks_per_page)
                           ? end_height - config_.blocks_per_page
                           : 0;

    for (uint64_t h = end_height; h >= start_height && h <= end_height; --h) {
        auto summary_result = GetBlockSummaryByHeight(h);
        if (summary_result.IsOk()) {
            blocks.push_back(summary_result.GetValue());
        }

        if (h == 0) break; // Prevent underflow
    }

    return Result<std::vector<BlockSummary>>::Ok(blocks);
}

Result<Block> BlockExplorer::GetBlockDetails(const uint256& hash) {
    return blockchain_->GetBlock(hash);
}

// ============================================================================
// Transaction Queries
// ============================================================================

Result<TransactionSummary> BlockExplorer::GetTransactionSummary(const uint256& tx_hash) {
    auto tx_result = blockchain_->GetTransaction(tx_hash);
    if (!tx_result.IsOk()) {
        return Result<TransactionSummary>::Error("Transaction not found");
    }

    Transaction tx = tx_result.GetValue();

    TransactionSummary summary;
    summary.hash = tx_hash;
    summary.block_height = 0; // Would need to query transaction index
    summary.timestamp = 0;    // Would need block timestamp
    summary.total_input = 0;  // Would need UTXO set
    summary.total_output = tx.GetTotalOutputValue();
    summary.fee = 0;          // total_input - total_output
    summary.size = tx.GetSerializedSize();
    summary.is_coinbase = tx.IsCoinbase();

    // Extract addresses
    summary.to_addresses = ExtractAddresses(tx);

    return Result<TransactionSummary>::Ok(summary);
}

Result<Transaction> BlockExplorer::GetTransactionDetails(const uint256& tx_hash) {
    return blockchain_->GetTransaction(tx_hash);
}

Result<std::vector<TransactionSummary>> BlockExplorer::GetRecentTransactions(uint32_t page) {
    std::vector<TransactionSummary> txs;

    // Get recent blocks and extract transactions
    auto blocks_result = GetRecentBlocks(page / 10); // Approximate
    if (!blocks_result.IsOk()) {
        return Result<std::vector<TransactionSummary>>::Ok(txs);
    }

    for (const auto& block_summary : blocks_result.GetValue()) {
        auto block_result = GetBlockDetails(block_summary.hash);
        if (block_result.IsOk()) {
            Block block = block_result.GetValue();
            for (const auto& tx : block.transactions) {
                auto tx_summary_result = GetTransactionSummary(tx.GetHash());
                if (tx_summary_result.IsOk()) {
                    txs.push_back(tx_summary_result.GetValue());
                }

                if (txs.size() >= config_.txs_per_page) {
                    return Result<std::vector<TransactionSummary>>::Ok(txs);
                }
            }
        }
    }

    return Result<std::vector<TransactionSummary>>::Ok(txs);
}

Result<std::vector<TransactionSummary>> BlockExplorer::GetAddressTransactions(
    const std::string& address, uint32_t page) {

    std::vector<TransactionSummary> txs;

    // Would need address index to efficiently query this
    // For now, return empty list

    return Result<std::vector<TransactionSummary>>::Ok(txs);
}

// ============================================================================
// Address Queries
// ============================================================================

Result<AddressStats> BlockExplorer::GetAddressStats(const std::string& address) {
    AddressStats stats;
    stats.address = address;

    // Get balance
    stats.balance = blockchain_->GetAddressBalance(address);

    // Get transaction count and amounts
    // Would need address index for efficient implementation
    stats.received_total = stats.balance;
    stats.sent_total = 0;
    stats.tx_count = 0;

    // Get rank
    auto rank_result = GetAddressRank(address);
    if (rank_result.IsOk()) {
        stats.rank = rank_result.GetValue();
    } else {
        stats.rank = 0; // Not in top
    }

    return Result<AddressStats>::Ok(stats);
}

Result<uint64_t> BlockExplorer::GetAddressBalance(const std::string& address) {
    return Result<uint64_t>::Ok(blockchain_->GetAddressBalance(address));
}

Result<uint32_t> BlockExplorer::GetAddressRank(const std::string& address) {
    std::lock_guard<std::mutex> lock(rich_list_mutex_);

    for (size_t i = 0; i < rich_list_.size(); ++i) {
        if (rich_list_[i].address == address) {
            return Result<uint32_t>::Ok(i + 1);
        }
    }

    return Result<uint32_t>::Error("Address not in rich list");
}

// Continued in next part...

// ============================================================================
// Rich List Implementation
// ============================================================================

Result<std::vector<RichListEntry>> BlockExplorer::GetRichList(uint32_t limit) {
    std::lock_guard<std::mutex> lock(rich_list_mutex_);

    std::vector<RichListEntry> result;
    uint32_t count = std::min(limit, static_cast<uint32_t>(rich_list_.size()));

    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(rich_list_[i]);
    }

    return Result<std::vector<RichListEntry>>::Ok(result);
}

Result<void> BlockExplorer::UpdateRichList() {
    std::cout << "[Explorer] Updating rich list...\n";

    // Get all addresses with balances
    std::map<std::string, uint64_t> address_balances;

    // Scan UTXO set to get all addresses and balances
    // const auto& utxo_set = blockchain_->GetUTXOSet();

    // Iterate through all UTXOs
    // In a real implementation, we would have an efficient way to iterate
    // For now, we'll create a simplified version

    // Calculate total supply for percentage
    uint64_t total_supply = blockchain_->GetTotalSupply();

    // Build rich list
    std::vector<RichListEntry> new_rich_list;

    // This is a placeholder - in reality we'd scan all UTXOs
    // For demonstration, add some sample entries
    for (uint64_t h = 0; h < blockchain_->GetBestHeight() && h < 1000; ++h) {
        auto block_result = blockchain_->GetBlockByHeight(h);
        if (block_result.IsOk()) {
            Block block = block_result.GetValue();
            for (const auto& tx : block.transactions) {
                auto addresses = ExtractAddresses(tx);
                for (const auto& addr : addresses) {
                    if (address_balances.find(addr) == address_balances.end()) {
                        uint64_t balance = blockchain_->GetAddressBalance(addr);
                        if (balance > 0) {
                            address_balances[addr] = balance;
                        }
                    }
                }
            }
        }
    }

    // Convert to rich list entries
    for (const auto& [addr, balance] : address_balances) {
        RichListEntry entry;
        entry.address = addr;
        entry.balance = balance;
        entry.tx_count = 0; // Would need transaction index
        entry.percentage = (static_cast<double>(balance) / static_cast<double>(total_supply)) * 100.0;

        new_rich_list.push_back(entry);
    }

    // Sort by balance (descending)
    std::sort(new_rich_list.begin(), new_rich_list.end());

    // Keep only top N
    if (new_rich_list.size() > config_.rich_list_size) {
        new_rich_list.resize(config_.rich_list_size);
    }

    // Update rich list
    {
        std::lock_guard<std::mutex> lock(rich_list_mutex_);
        rich_list_ = std::move(new_rich_list);
        rich_list_last_update_.store(std::time(nullptr));
    }

    std::cout << "[Explorer] Rich list updated (" << rich_list_.size() << " addresses)\n";

    return Result<void>::Ok();
}

uint64_t BlockExplorer::GetRichListLastUpdate() const {
    return rich_list_last_update_.load();
}

// ============================================================================
// Network Statistics
// ============================================================================

Result<NetworkStats> BlockExplorer::GetNetworkStats() {
    // Return cached stats
    std::lock_guard<std::mutex> lock(stats_cache_mutex_);
    return Result<NetworkStats>::Ok(cached_stats_);
}

Result<ChartData> BlockExplorer::GetHashrateChart(uint32_t days) {
    ChartData chart;
    chart.title = "Network Hashrate";
    chart.y_axis_label = "Hashrate (H/s)";

    uint64_t current_height = blockchain_->GetBestHeight();
    uint32_t blocks_per_day = (24 * 60 * 60) / 120; // 720 blocks per day

    for (uint32_t day = 0; day < days; ++day) {
        uint64_t height = current_height - (day * blocks_per_day);
        if (height > current_height) break;

        auto block_result = blockchain_->GetBlockByHeight(height);
        if (block_result.IsOk()) {
            Block block = block_result.GetValue();

            ChartDataPoint point;
            point.timestamp = block.header.timestamp;
            point.value = blockchain_->GetNetworkHashRate();
            point.label = FormatTimestamp(block.header.timestamp);

            chart.points.push_back(point);
        }
    }

    std::reverse(chart.points.begin(), chart.points.end());

    return Result<ChartData>::Ok(chart);
}

Result<ChartData> BlockExplorer::GetDifficultyChart(uint32_t days) {
    ChartData chart;
    chart.title = "Network Difficulty";
    chart.y_axis_label = "Difficulty";

    uint64_t current_height = blockchain_->GetBestHeight();
    uint32_t blocks_per_day = (24 * 60 * 60) / 120;

    for (uint32_t day = 0; day < days; ++day) {
        uint64_t height = current_height - (day * blocks_per_day);
        if (height > current_height) break;

        auto block_result = blockchain_->GetBlockByHeight(height);
        if (block_result.IsOk()) {
            Block block = block_result.GetValue();

            ChartDataPoint point;
            point.timestamp = block.header.timestamp;
            point.value = blockchain_->GetDifficulty();
            point.label = FormatTimestamp(block.header.timestamp);

            chart.points.push_back(point);
        }
    }

    std::reverse(chart.points.begin(), chart.points.end());

    return Result<ChartData>::Ok(chart);
}

Result<ChartData> BlockExplorer::GetTxVolumeChart(uint32_t days) {
    ChartData chart;
    chart.title = "Transaction Volume";
    chart.y_axis_label = "Transactions";

    uint64_t current_height = blockchain_->GetBestHeight();
    uint32_t blocks_per_day = (24 * 60 * 60) / 120;

    for (uint32_t day = 0; day < days; ++day) {
        uint64_t start_height = current_height - ((day + 1) * blocks_per_day);
        uint64_t end_height = current_height - (day * blocks_per_day);

        uint32_t tx_count = 0;
        uint64_t timestamp = 0;

        for (uint64_t h = start_height; h <= end_height; ++h) {
            auto block_result = blockchain_->GetBlockByHeight(h);
            if (block_result.IsOk()) {
                Block block = block_result.GetValue();
                tx_count += block.transactions.size();
                timestamp = block.header.timestamp;
            }
        }

        ChartDataPoint point;
        point.timestamp = timestamp;
        point.value = tx_count;
        point.label = FormatTimestamp(timestamp);

        chart.points.push_back(point);
    }

    std::reverse(chart.points.begin(), chart.points.end());

    return Result<ChartData>::Ok(chart);
}

// ============================================================================
// Search Implementation
// ============================================================================

Result<SearchResult> BlockExplorer::Search(const std::string& query) {
    SearchResult result;
    result.type = SearchResultType::NOT_FOUND;

    // Try as block height (numeric)
    if (std::all_of(query.begin(), query.end(), ::isdigit)) {
        uint64_t height = std::stoull(query);
        if (height <= blockchain_->GetBestHeight()) {
            result.type = SearchResultType::BLOCK_HEIGHT;
            result.value = query;
            result.display_value = "Block #" + query;
            return Result<SearchResult>::Ok(result);
        }
    }

    // Try as block hash (64 hex chars)
    if (query.length() == 64 && std::all_of(query.begin(), query.end(), ::isxdigit)) {
        auto hash_opt = FromHex(query);
        if (hash_opt.has_value()) {
            uint256 hash = hash_opt.value();
            if (blockchain_->HasBlock(hash)) {
                result.type = SearchResultType::BLOCK_HASH;
                result.value = query;
                result.display_value = "Block " + query.substr(0, 16) + "...";
                return Result<SearchResult>::Ok(result);
            }

            // Try as transaction
            if (blockchain_->HasTransaction(hash)) {
                result.type = SearchResultType::TRANSACTION;
                result.value = query;
                result.display_value = "TX " + query.substr(0, 16) + "...";
                return Result<SearchResult>::Ok(result);
            }
        }
    }

    // Try as address (starts with "int1")
    if (query.substr(0, 4) == "int1" && query.length() > 10) {
        if (AddressEncoder::ValidateAddress(query)) {
            result.type = SearchResultType::ADDRESS;
            result.value = query;
            result.display_value = "Address " + query.substr(0, 16) + "...";
            return Result<SearchResult>::Ok(result);
        }
    }

    return Result<SearchResult>::Ok(result);
}

// ============================================================================
// WebSocket Callbacks
// ============================================================================

void BlockExplorer::RegisterBlockCallback(BlockCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    block_callbacks_.push_back(callback);
}

void BlockExplorer::RegisterTransactionCallback(TransactionCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    tx_callbacks_.push_back(callback);
}

void BlockExplorer::BroadcastNewBlock(const BlockSummary& block) {
    std::string json = SerializeJSON(block);
    std::string message = "{\"type\":\"block\",\"data\":" + json + "}";

    std::lock_guard<std::mutex> lock(websocket_clients_mutex_);
    for (int client : websocket_clients_) {
        send(client, message.c_str(), message.length(), 0);
    }
}

void BlockExplorer::BroadcastNewTransaction(const TransactionSummary& tx) {
    std::string json = SerializeJSON(tx);
    std::string message = "{\"type\":\"transaction\",\"data\":" + json + "}";

    std::lock_guard<std::mutex> lock(websocket_clients_mutex_);
    for (int client : websocket_clients_) {
        send(client, message.c_str(), message.length(), 0);
    }
}

// Continued in next message...

// ============================================================================
// JSON Serialization
// ============================================================================

std::string BlockExplorer::SerializeJSON(const BlockSummary& block) {
    std::ostringstream json;
    json << "{"
         << "\"hash\":\"" << ToHex(block.hash) << "\","
         << "\"height\":" << block.height << ","
         << "\"timestamp\":" << block.timestamp << ","
         << "\"tx_count\":" << block.tx_count << ","
         << "\"total_amount\":" << block.total_amount << ","
         << "\"block_reward\":" << block.block_reward << ","
         << "\"size\":" << block.size << ","
         << "\"difficulty\":" << block.difficulty << ","
         << "\"miner\":\"" << block.miner << "\""
         << "}";
    return json.str();
}

std::string BlockExplorer::SerializeJSON(const TransactionSummary& tx) {
    std::ostringstream json;
    json << "{"
         << "\"hash\":\"" << ToHex(tx.hash) << "\","
         << "\"block_height\":" << tx.block_height << ","
         << "\"timestamp\":" << tx.timestamp << ","
         << "\"total_input\":" << tx.total_input << ","
         << "\"total_output\":" << tx.total_output << ","
         << "\"fee\":" << tx.fee << ","
         << "\"size\":" << tx.size << ","
         << "\"is_coinbase\":" << (tx.is_coinbase ? "true" : "false")
         << "}";
    return json.str();
}

std::string BlockExplorer::SerializeJSON(const AddressStats& stats) {
    std::ostringstream json;
    json << "{"
         << "\"address\":\"" << stats.address << "\","
         << "\"balance\":" << stats.balance << ","
         << "\"received_total\":" << stats.received_total << ","
         << "\"sent_total\":" << stats.sent_total << ","
         << "\"tx_count\":" << stats.tx_count << ","
         << "\"rank\":" << stats.rank
         << "}";
    return json.str();
}

std::string BlockExplorer::SerializeJSON(const RichListEntry& entry) {
    std::ostringstream json;
    json << "{"
         << "\"address\":\"" << entry.address << "\","
         << "\"balance\":" << entry.balance << ","
         << "\"tx_count\":" << entry.tx_count << ","
         << "\"percentage\":" << std::fixed << std::setprecision(6) << entry.percentage
         << "}";
    return json.str();
}

std::string BlockExplorer::SerializeJSON(const NetworkStats& stats) {
    std::ostringstream json;
    json << "{"
         << "\"height\":" << stats.height << ","
         << "\"best_block_hash\":\"" << ToHex(stats.best_block_hash) << "\","
         << "\"difficulty\":" << stats.difficulty << ","
         << "\"hashrate\":" << stats.hashrate << ","
         << "\"total_supply\":" << stats.total_supply << ","
         << "\"total_transactions\":" << stats.total_transactions << ","
         << "\"mempool_size\":" << stats.mempool_size << ","
         << "\"mempool_bytes\":" << stats.mempool_bytes << ","
         << "\"avg_block_time\":" << stats.avg_block_time << ","
         << "\"total_addresses\":" << stats.total_addresses
         << "}";
    return json.str();
}

std::string BlockExplorer::SerializeJSON(const ChartData& chart) {
    std::ostringstream json;
    json << "{"
         << "\"title\":\"" << chart.title << "\","
         << "\"y_axis_label\":\"" << chart.y_axis_label << "\","
         << "\"points\":[";

    for (size_t i = 0; i < chart.points.size(); ++i) {
        const auto& point = chart.points[i];
        json << "{"
             << "\"timestamp\":" << point.timestamp << ","
             << "\"value\":" << point.value << ","
             << "\"label\":\"" << point.label << "\""
             << "}";
        if (i < chart.points.size() - 1) {
            json << ",";
        }
    }

    json << "]}";
    return json.str();
}

// ============================================================================
// HTTP Server Loop
// ============================================================================

void BlockExplorer::HTTPServerLoop() {
    // Create socket
    http_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (http_socket_ < 0) {
        std::cerr << "[Explorer] Failed to create HTTP socket\n";
        return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(http_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config_.port);

    if (bind(http_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Explorer] Failed to bind HTTP socket\n";
        close(http_socket_);
        http_socket_ = -1;
        return;
    }

    // Listen
    if (listen(http_socket_, 10) < 0) {
        std::cerr << "[Explorer] Failed to listen on HTTP socket\n";
        close(http_socket_);
        http_socket_ = -1;
        return;
    }

    std::cout << "[Explorer] HTTP server listening on port " << config_.port << "\n";

    // Accept loop
    while (running_.load() && !stop_requested_.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(http_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            continue;
        }

        // Read request (simplified)
        char buffer[4096];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);

            // Parse request line
            std::istringstream iss(request);
            std::string method, path, protocol;
            iss >> method >> path >> protocol;

            // Handle request
            std::string response = HandleRequest(method, path, "");

            // Send response
            std::string http_response = "HTTP/1.1 200 OK\r\n";
            http_response += "Content-Type: application/json\r\n";
            if (config_.enable_cors) {
                http_response += "Access-Control-Allow-Origin: *\r\n";
            }
            http_response += "Content-Length: " + std::to_string(response.length()) + "\r\n";
            http_response += "\r\n";
            http_response += response;

            send(client_socket, http_response.c_str(), http_response.length(), 0);
        }

        close(client_socket);
    }
}

std::string BlockExplorer::HandleRequest(const std::string& method,
                                        const std::string& path,
                                        const std::string& body) {
    // API routes
    if (path == "/api/stats") {
        auto result = GetNetworkStats();
        if (result.IsOk()) {
            return SerializeJSON(result.GetValue());
        }
    }
    else if (path == "/api/richlist") {
        auto result = GetRichList(100);
        if (result.IsOk()) {
            std::ostringstream json;
            json << "[";
            const auto& list = result.GetValue();
            for (size_t i = 0; i < list.size(); ++i) {
                json << SerializeJSON(list[i]);
                if (i < list.size() - 1) json << ",";
            }
            json << "]";
            return json.str();
        }
    }
    else if (path.find("/api/block/") == 0) {
        std::string hash_str = path.substr(11);
        auto hash_opt = FromHex(hash_str);
        if (hash_opt.has_value()) {
            auto result = GetBlockSummary(hash_opt.value());
            if (result.IsOk()) {
                return SerializeJSON(result.GetValue());
            }
        }
    }
    else if (path == "/api/blocks/recent") {
        auto result = GetRecentBlocks(0);
        if (result.IsOk()) {
            std::ostringstream json;
            json << "[";
            const auto& blocks = result.GetValue();
            for (size_t i = 0; i < blocks.size(); ++i) {
                json << SerializeJSON(blocks[i]);
                if (i < blocks.size() - 1) json << ",";
            }
            json << "]";
            return json.str();
        }
    }

    return "{\"error\":\"Not found\"}";
}

// ============================================================================
// WebSocket Server Loop (Simplified)
// ============================================================================

void BlockExplorer::WebSocketServerLoop() {
    // Simplified WebSocket implementation
    // In production, would use a proper WebSocket library
    std::cout << "[Explorer] WebSocket server started\n";

    while (running_.load() && !stop_requested_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ============================================================================
// Background Update Loops
// ============================================================================

void BlockExplorer::RichListUpdateLoop() {
    // Initial update
    UpdateRichList();

    while (running_.load() && !stop_requested_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.rich_list_update_interval));

        if (!stop_requested_.load()) {
            UpdateRichList();
        }
    }
}

void BlockExplorer::StatsCacheUpdateLoop() {
    while (running_.load() && !stop_requested_.load()) {
        // Update cached statistics
        NetworkStats stats;
        stats.height = blockchain_->GetBestHeight();
        stats.best_block_hash = blockchain_->GetBestBlockHash();
        stats.difficulty = blockchain_->GetDifficulty();
        stats.hashrate = blockchain_->GetNetworkHashRate();
        stats.total_supply = blockchain_->GetTotalSupply();
        stats.total_transactions = blockchain_->GetTotalTransactions();
        stats.mempool_size = blockchain_->GetMempool().GetSize();
        stats.mempool_bytes = stats.mempool_size * 500; // Estimate ~500 bytes per tx
        stats.avg_block_time = CalculateAverageBlockTime(*blockchain_, 100);
        stats.total_addresses = CountActiveAddresses(*blockchain_);

        {
            std::lock_guard<std::mutex> lock(stats_cache_mutex_);
            cached_stats_ = stats;
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

} // namespace explorer
} // namespace intcoin
