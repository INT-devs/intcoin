// Copyright (c) 2025 INTcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_EXPLORER_H
#define INTCOIN_EXPLORER_H

#include "types.h"
#include "block.h"
#include "transaction.h"
#include "blockchain.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

namespace intcoin {
namespace explorer {

// ============================================================================
// Explorer Configuration
// ============================================================================

struct ExplorerConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    std::string data_dir = "./explorer-data";
    uint32_t rich_list_size = 100;          // Top N addresses
    uint32_t rich_list_update_interval = 300; // 5 minutes
    uint32_t blocks_per_page = 20;
    uint32_t txs_per_page = 50;
    bool enable_websocket = true;
    bool enable_cors = true;
};

// ============================================================================
// Rich List Entry
// ============================================================================

struct RichListEntry {
    std::string address;
    uint64_t balance;
    uint32_t tx_count;
    double percentage;  // % of total supply

    bool operator<(const RichListEntry& other) const {
        return balance > other.balance; // Descending order
    }
};

// ============================================================================
// Address Statistics
// ============================================================================

struct AddressStats {
    std::string address;
    uint64_t balance;
    uint64_t received_total;
    uint64_t sent_total;
    uint32_t tx_count;
    uint32_t rank; // Position in rich list (0 if not in top)
    std::vector<uint256> recent_txs; // Recent transaction hashes
};

// ============================================================================
// Network Statistics
// ============================================================================

struct NetworkStats {
    uint64_t height;
    uint256 best_block_hash;
    double difficulty;
    double hashrate;
    uint64_t total_supply;
    uint64_t total_transactions;
    uint32_t mempool_size;
    uint64_t mempool_bytes;
    double avg_block_time;
    uint64_t total_addresses;
};

// ============================================================================
// Block Summary (for lists)
// ============================================================================

struct BlockSummary {
    uint256 hash;
    uint64_t height;
    uint64_t timestamp;
    uint32_t tx_count;
    uint64_t total_amount;
    uint64_t block_reward;
    uint32_t size;
    double difficulty;
    std::string miner; // Mining address
};

// ============================================================================
// Transaction Summary (for lists)
// ============================================================================

struct TransactionSummary {
    uint256 hash;
    uint64_t block_height;
    uint64_t timestamp;
    uint64_t total_input;
    uint64_t total_output;
    uint64_t fee;
    uint32_t size;
    bool is_coinbase;
    std::vector<std::string> from_addresses;
    std::vector<std::string> to_addresses;
};

// ============================================================================
// Search Result
// ============================================================================

enum class SearchResultType {
    BLOCK_HASH,
    BLOCK_HEIGHT,
    TRANSACTION,
    ADDRESS,
    NOT_FOUND
};

struct SearchResult {
    SearchResultType type;
    std::string value; // Hash, height, or address
    std::string display_value; // Human-readable
};

// ============================================================================
// Chart Data
// ============================================================================

struct ChartDataPoint {
    uint64_t timestamp;
    double value;
    std::string label;
};

struct ChartData {
    std::string title;
    std::vector<ChartDataPoint> points;
    std::string y_axis_label;
};

// ============================================================================
// Block Explorer - Main Class
// ============================================================================

class BlockExplorer {
public:
    explicit BlockExplorer(const ExplorerConfig& config);
    ~BlockExplorer();

    // Start/stop explorer
    Result<void> Start(Blockchain& blockchain);
    void Stop();
    bool IsRunning() const { return running_.load(); }

    // -------------------------------------------------------------------------
    // Block Queries
    // -------------------------------------------------------------------------

    /// Get block summary by hash
    Result<BlockSummary> GetBlockSummary(const uint256& hash);

    /// Get block summary by height
    Result<BlockSummary> GetBlockSummaryByHeight(uint64_t height);

    /// Get recent blocks (paginated)
    Result<std::vector<BlockSummary>> GetRecentBlocks(uint32_t page = 0);

    /// Get full block details
    Result<Block> GetBlockDetails(const uint256& hash);

    // -------------------------------------------------------------------------
    // Transaction Queries
    // -------------------------------------------------------------------------

    /// Get transaction summary
    Result<TransactionSummary> GetTransactionSummary(const uint256& tx_hash);

    /// Get full transaction details
    Result<Transaction> GetTransactionDetails(const uint256& tx_hash);

    /// Get recent transactions (paginated)
    Result<std::vector<TransactionSummary>> GetRecentTransactions(uint32_t page = 0);

    /// Get transactions for address (paginated)
    Result<std::vector<TransactionSummary>> GetAddressTransactions(
        const std::string& address, uint32_t page = 0);

    // -------------------------------------------------------------------------
    // Address Queries
    // -------------------------------------------------------------------------

    /// Get address statistics
    Result<AddressStats> GetAddressStats(const std::string& address);

    /// Get address balance
    Result<uint64_t> GetAddressBalance(const std::string& address);

    /// Get address rank in rich list
    Result<uint32_t> GetAddressRank(const std::string& address);

    // -------------------------------------------------------------------------
    // Rich List
    // -------------------------------------------------------------------------

    /// Get rich list (top N addresses)
    Result<std::vector<RichListEntry>> GetRichList(uint32_t limit = 100);

    /// Update rich list (manual trigger)
    Result<void> UpdateRichList();

    /// Get rich list last update time
    uint64_t GetRichListLastUpdate() const;

    // -------------------------------------------------------------------------
    // Network Statistics
    // -------------------------------------------------------------------------

    /// Get current network stats
    Result<NetworkStats> GetNetworkStats();

    /// Get hashrate chart data
    Result<ChartData> GetHashrateChart(uint32_t days = 7);

    /// Get difficulty chart data
    Result<ChartData> GetDifficultyChart(uint32_t days = 7);

    /// Get transaction volume chart
    Result<ChartData> GetTxVolumeChart(uint32_t days = 7);

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    /// Search for block, transaction, or address
    Result<SearchResult> Search(const std::string& query);

    // -------------------------------------------------------------------------
    // WebSocket Notifications
    // -------------------------------------------------------------------------

    using BlockCallback = std::function<void(const BlockSummary&)>;
    using TransactionCallback = std::function<void(const TransactionSummary&)>;

    void RegisterBlockCallback(BlockCallback callback);
    void RegisterTransactionCallback(TransactionCallback callback);

private:
    void HTTPServerLoop();
    void WebSocketServerLoop();
    void RichListUpdateLoop();
    void StatsCacheUpdateLoop();

    std::string HandleRequest(const std::string& method, const std::string& path,
                             const std::string& body);

    std::string SerializeJSON(const BlockSummary& block);
    std::string SerializeJSON(const TransactionSummary& tx);
    std::string SerializeJSON(const AddressStats& stats);
    std::string SerializeJSON(const RichListEntry& entry);
    std::string SerializeJSON(const NetworkStats& stats);
    std::string SerializeJSON(const ChartData& chart);

    void BroadcastNewBlock(const BlockSummary& block);
    void BroadcastNewTransaction(const TransactionSummary& tx);

    ExplorerConfig config_;
    Blockchain* blockchain_ = nullptr;

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    // HTTP server
    std::unique_ptr<std::thread> http_thread_;
    int http_socket_ = -1;

    // WebSocket server
    std::unique_ptr<std::thread> websocket_thread_;
    int websocket_socket_ = -1;
    std::vector<int> websocket_clients_;
    std::mutex websocket_clients_mutex_;

    // Rich list
    std::vector<RichListEntry> rich_list_;
    std::mutex rich_list_mutex_;
    std::unique_ptr<std::thread> rich_list_thread_;
    std::atomic<uint64_t> rich_list_last_update_{0};

    // Statistics cache
    NetworkStats cached_stats_;
    std::mutex stats_cache_mutex_;
    std::unique_ptr<std::thread> stats_cache_thread_;

    // Callbacks
    std::vector<BlockCallback> block_callbacks_;
    std::vector<TransactionCallback> tx_callbacks_;
    std::mutex callbacks_mutex_;
};

// ============================================================================
// Utility Functions
// ============================================================================

/// Calculate total addresses with non-zero balance
uint64_t CountActiveAddresses(const Blockchain& blockchain);

/// Calculate average block time over N blocks
double CalculateAverageBlockTime(const Blockchain& blockchain, uint32_t num_blocks = 100);

/// Extract addresses from transaction
std::vector<std::string> ExtractAddresses(const Transaction& tx);

/// Format amount with commas (e.g., 1,234,567.890000 INT)
std::string FormatAmount(uint64_t ints);

/// Format timestamp as ISO 8601
std::string FormatTimestamp(uint64_t unix_timestamp);

} // namespace explorer
} // namespace intcoin

#endif // INTCOIN_EXPLORER_H
