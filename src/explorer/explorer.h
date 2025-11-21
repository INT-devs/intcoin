// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Block Explorer Backend
// Provides rich list, network statistics, and mempool viewer

#ifndef INTCOIN_EXPLORER_H
#define INTCOIN_EXPLORER_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <map>

namespace intcoin {
namespace explorer {

/**
 * Rich list entry - top addresses by balance
 */
struct RichListEntry {
    std::string address;
    uint64_t balance;           // Total balance in satoshis
    double percentage;          // Percentage of total supply
    uint32_t rank;              // Position in rich list
    uint64_t tx_count;          // Number of transactions
    uint64_t first_seen;        // First transaction timestamp
    uint64_t last_seen;         // Last transaction timestamp
    bool is_contract;           // True if smart contract address
    std::string label;          // Known address label (optional)
};

/**
 * Network statistics
 */
struct NetworkStats {
    // Chain stats
    uint32_t height;
    std::string best_block_hash;
    uint64_t total_transactions;
    uint64_t total_addresses;
    uint64_t active_addresses_24h;

    // Supply stats
    uint64_t circulating_supply;
    uint64_t total_supply;
    uint64_t max_supply;
    double inflation_rate;

    // Mining stats
    double difficulty;
    double hashrate;            // Estimated network hashrate
    uint64_t avg_block_time;    // Seconds
    uint32_t blocks_24h;

    // Transaction stats
    uint64_t tx_24h;
    uint64_t tx_7d;
    double avg_tx_value;
    double median_tx_value;
    uint64_t total_fees_24h;

    // Network stats
    uint32_t node_count;
    uint32_t peer_count;
    std::string protocol_version;

    // Mempool stats (current)
    uint32_t mempool_size;
    uint64_t mempool_bytes;
    double mempool_min_fee;
    double mempool_avg_fee;

    uint64_t timestamp;
};

/**
 * Mempool transaction entry
 */
struct MempoolTx {
    std::string tx_hash;
    uint64_t size;              // Transaction size in bytes
    uint64_t vsize;             // Virtual size (weight/4)
    uint64_t fee;               // Fee in satoshis
    double fee_rate;            // Fee per vbyte
    uint64_t time;              // Time added to mempool
    uint32_t descendant_count;  // Number of descendant txs
    uint64_t descendant_size;   // Total size of descendants
    uint64_t descendant_fees;   // Total fees of descendants
    uint32_t ancestor_count;    // Number of ancestor txs
    uint64_t ancestor_size;     // Total size of ancestors
    uint64_t ancestor_fees;     // Total fees of ancestors
    std::vector<std::string> depends;  // Unconfirmed parent tx hashes
    bool rbf;                   // Replace-by-fee signaled
};

/**
 * Mempool fee histogram bucket
 */
struct FeeBucket {
    double min_fee_rate;
    double max_fee_rate;
    uint32_t tx_count;
    uint64_t total_size;
};

/**
 * Mempool summary
 */
struct MempoolSummary {
    uint32_t size;              // Number of transactions
    uint64_t bytes;             // Total size in bytes
    uint64_t usage;             // Memory usage
    uint64_t max_mempool;       // Maximum mempool size
    double min_fee_rate;        // Minimum fee rate to enter
    double median_fee_rate;     // Median fee rate
    double avg_fee_rate;        // Average fee rate
    uint64_t total_fee;         // Total fees in mempool
    std::vector<FeeBucket> fee_histogram;
    uint64_t last_update;
};

/**
 * Block statistics for charts
 */
struct BlockStats {
    uint32_t height;
    uint64_t timestamp;
    uint32_t tx_count;
    uint64_t size;
    uint64_t weight;
    double difficulty;
    uint64_t total_fees;
    uint64_t subsidy;
    uint64_t total_output;
    double avg_fee_rate;
    uint32_t input_count;
    uint32_t output_count;
};

/**
 * Address statistics
 */
struct AddressStats {
    std::string address;
    uint64_t balance;
    uint64_t total_received;
    uint64_t total_sent;
    uint32_t tx_count;
    uint32_t unspent_count;
    uint64_t first_seen;
    uint64_t last_seen;
};

/**
 * UTXO information
 */
struct UTXO {
    std::string tx_hash;
    uint32_t output_index;
    uint64_t value;
    std::string script_pubkey;
    std::string address;
    uint32_t confirmations;
    bool coinbase;
};

/**
 * Block Explorer Backend
 */
class Explorer {
public:
    Explorer();
    ~Explorer();

    // Initialize with blockchain data directory
    bool initialize(const std::string& datadir);

    // ========== Rich List ==========

    // Get rich list (top N addresses by balance)
    std::vector<RichListEntry> get_rich_list(uint32_t limit = 100,
                                              uint32_t offset = 0) const;

    // Get address rank in rich list
    std::optional<uint32_t> get_address_rank(const std::string& address) const;

    // Get total unique addresses
    uint64_t get_total_addresses() const;

    // Get addresses with balance above threshold
    uint64_t get_addresses_above(uint64_t min_balance) const;

    // ========== Network Statistics ==========

    // Get current network statistics
    NetworkStats get_network_stats() const;

    // Get historical network stats
    std::vector<NetworkStats> get_network_stats_history(
        uint64_t start_time,
        uint64_t end_time,
        uint32_t interval_seconds = 3600) const;

    // Get block statistics for range
    std::vector<BlockStats> get_block_stats(uint32_t start_height,
                                            uint32_t end_height) const;

    // Get difficulty history
    std::vector<std::pair<uint32_t, double>> get_difficulty_history(
        uint32_t count = 100) const;

    // Get hashrate history (estimated)
    std::vector<std::pair<uint64_t, double>> get_hashrate_history(
        uint32_t count = 100) const;

    // ========== Mempool Viewer ==========

    // Get mempool summary
    MempoolSummary get_mempool_summary() const;

    // Get mempool transactions (sorted by fee rate, highest first)
    std::vector<MempoolTx> get_mempool_transactions(
        uint32_t limit = 100,
        uint32_t offset = 0) const;

    // Get specific mempool transaction
    std::optional<MempoolTx> get_mempool_tx(const std::string& tx_hash) const;

    // Get mempool ancestors for transaction
    std::vector<MempoolTx> get_mempool_ancestors(
        const std::string& tx_hash) const;

    // Get mempool descendants for transaction
    std::vector<MempoolTx> get_mempool_descendants(
        const std::string& tx_hash) const;

    // Get fee estimation (blocks to confirm)
    double estimate_fee(uint32_t target_blocks) const;

    // Get recommended fees
    struct FeeRecommendation {
        double fastest;     // Next block
        double fast;        // 2-3 blocks
        double medium;      // 4-6 blocks
        double slow;        // 12+ blocks
        double economy;     // 24+ blocks
    };
    FeeRecommendation get_fee_recommendation() const;

    // ========== Address Queries ==========

    // Get address statistics
    std::optional<AddressStats> get_address_stats(
        const std::string& address) const;

    // Get address UTXOs
    std::vector<UTXO> get_address_utxos(const std::string& address,
                                         uint32_t limit = 100) const;

    // Get address transaction history
    std::vector<std::string> get_address_transactions(
        const std::string& address,
        uint32_t limit = 100,
        uint32_t offset = 0) const;

    // ========== Search ==========

    // Search by hash, address, or block height
    struct SearchResult {
        enum Type { BLOCK, TRANSACTION, ADDRESS, UNKNOWN };
        Type type;
        std::string value;
    };
    SearchResult search(const std::string& query) const;

    // ========== Charts Data ==========

    // Get data for transaction count chart
    std::vector<std::pair<uint64_t, uint64_t>> get_tx_count_chart(
        uint32_t days = 30) const;

    // Get data for difficulty chart
    std::vector<std::pair<uint64_t, double>> get_difficulty_chart(
        uint32_t days = 30) const;

    // Get data for fee chart
    std::vector<std::pair<uint64_t, double>> get_fee_chart(
        uint32_t days = 30) const;

    // Get data for address growth chart
    std::vector<std::pair<uint64_t, uint64_t>> get_address_growth_chart(
        uint32_t days = 30) const;

    // ========== Known Addresses ==========

    // Add known address label
    void add_known_address(const std::string& address,
                           const std::string& label);

    // Get known address label
    std::optional<std::string> get_address_label(
        const std::string& address) const;

    // Get all known addresses
    std::map<std::string, std::string> get_known_addresses() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Explorer web server (REST API)
 */
class ExplorerServer {
public:
    ExplorerServer(Explorer& explorer, uint16_t port = 3000);
    ~ExplorerServer();

    // Start serving HTTP requests
    bool start();

    // Stop server
    void stop();

    // Check if running
    bool is_running() const;

    // Get server URL
    std::string get_url() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace explorer
} // namespace intcoin

#endif // INTCOIN_EXPLORER_H
