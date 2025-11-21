// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Block Explorer Backend Implementation

#include "explorer.h"
#include "intcoin/blockchain.h"
#include "intcoin/mempool.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <cmath>

namespace intcoin {
namespace explorer {

// ========== Explorer Implementation ==========

class Explorer::Impl {
public:
    std::string datadir;
    std::unique_ptr<Blockchain> blockchain;
    Mempool* mempool = nullptr;
    mutable std::mutex mutex;

    // Caches
    mutable std::vector<RichListEntry> rich_list_cache;
    mutable uint64_t rich_list_cache_height = 0;
    mutable NetworkStats network_stats_cache;
    mutable uint64_t network_stats_cache_time = 0;

    // Known addresses database
    std::map<std::string, std::string> known_addresses;

    bool initialized = false;
};

Explorer::Explorer() : impl_(std::make_unique<Impl>()) {}
Explorer::~Explorer() = default;

bool Explorer::initialize(const std::string& datadir) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    impl_->datadir = datadir;
    impl_->blockchain = std::make_unique<Blockchain>();

    if (!impl_->blockchain->load(datadir)) {
        return false;
    }

    // Load known addresses from file
    // impl_->load_known_addresses();

    impl_->initialized = true;
    return true;
}

std::vector<RichListEntry> Explorer::get_rich_list(uint32_t limit,
                                                    uint32_t offset) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->initialized) {
        return {};
    }

    // Check cache validity
    uint32_t current_height = impl_->blockchain->get_height();
    if (impl_->rich_list_cache_height != current_height ||
        impl_->rich_list_cache.empty()) {
        // Rebuild rich list
        impl_->rich_list_cache.clear();

        // Get all address balances from blockchain
        auto balances = impl_->blockchain->get_all_address_balances();

        // Convert to rich list entries
        for (const auto& [address, balance] : balances) {
            if (balance == 0) continue;

            RichListEntry entry;
            entry.address = address;
            entry.balance = balance;
            entry.percentage = 0.0;  // Calculate after sorting
            entry.rank = 0;
            entry.tx_count = impl_->blockchain->get_address_tx_count(address);
            entry.first_seen = impl_->blockchain->get_address_first_seen(address);
            entry.last_seen = impl_->blockchain->get_address_last_seen(address);
            entry.is_contract = false;

            auto label = get_address_label(address);
            if (label) entry.label = *label;

            impl_->rich_list_cache.push_back(entry);
        }

        // Sort by balance descending
        std::sort(impl_->rich_list_cache.begin(), impl_->rich_list_cache.end(),
                  [](const RichListEntry& a, const RichListEntry& b) {
                      return a.balance > b.balance;
                  });

        // Calculate ranks and percentages
        uint64_t total_supply = impl_->blockchain->get_circulating_supply();
        for (size_t i = 0; i < impl_->rich_list_cache.size(); ++i) {
            impl_->rich_list_cache[i].rank = static_cast<uint32_t>(i + 1);
            impl_->rich_list_cache[i].percentage =
                (static_cast<double>(impl_->rich_list_cache[i].balance) /
                 static_cast<double>(total_supply)) * 100.0;
        }

        impl_->rich_list_cache_height = current_height;
    }

    // Return requested slice
    std::vector<RichListEntry> result;
    size_t start = std::min(static_cast<size_t>(offset),
                           impl_->rich_list_cache.size());
    size_t end = std::min(static_cast<size_t>(offset + limit),
                         impl_->rich_list_cache.size());

    for (size_t i = start; i < end; ++i) {
        result.push_back(impl_->rich_list_cache[i]);
    }

    return result;
}

std::optional<uint32_t> Explorer::get_address_rank(
    const std::string& address) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Ensure rich list is built
    get_rich_list(1, 0);

    for (const auto& entry : impl_->rich_list_cache) {
        if (entry.address == address) {
            return entry.rank;
        }
    }

    return std::nullopt;
}

uint64_t Explorer::get_total_addresses() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->initialized) return 0;

    return impl_->blockchain->get_total_addresses();
}

uint64_t Explorer::get_addresses_above(uint64_t min_balance) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Ensure rich list is built
    get_rich_list(1, 0);

    uint64_t count = 0;
    for (const auto& entry : impl_->rich_list_cache) {
        if (entry.balance >= min_balance) {
            ++count;
        } else {
            break;  // List is sorted, no more above threshold
        }
    }

    return count;
}

NetworkStats Explorer::get_network_stats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->initialized) {
        return {};
    }

    // Check cache (valid for 60 seconds)
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (impl_->network_stats_cache_time > 0 &&
        now - impl_->network_stats_cache_time < 60) {
        return impl_->network_stats_cache;
    }

    NetworkStats stats;

    // Chain stats
    stats.height = impl_->blockchain->get_height();
    stats.best_block_hash = impl_->blockchain->get_best_block_hash_hex();
    stats.total_transactions = impl_->blockchain->get_total_transactions();
    stats.total_addresses = impl_->blockchain->get_total_addresses();
    stats.active_addresses_24h = impl_->blockchain->get_active_addresses_24h();

    // Supply stats
    stats.circulating_supply = impl_->blockchain->get_circulating_supply();
    stats.total_supply = stats.circulating_supply;
    stats.max_supply = 221000000000000ULL;  // 221 trillion INT
    stats.inflation_rate = impl_->blockchain->get_inflation_rate();

    // Mining stats
    stats.difficulty = impl_->blockchain->get_difficulty();
    stats.hashrate = impl_->blockchain->estimate_network_hashrate();
    stats.avg_block_time = 120;  // 2 minutes target
    stats.blocks_24h = impl_->blockchain->get_blocks_in_period(86400);

    // Transaction stats
    stats.tx_24h = impl_->blockchain->get_transactions_in_period(86400);
    stats.tx_7d = impl_->blockchain->get_transactions_in_period(604800);
    stats.avg_tx_value = impl_->blockchain->get_avg_transaction_value();
    stats.median_tx_value = impl_->blockchain->get_median_transaction_value();
    stats.total_fees_24h = impl_->blockchain->get_total_fees_in_period(86400);

    // Network stats
    stats.node_count = 0;  // Would come from peer manager
    stats.peer_count = 0;
    stats.protocol_version = "1.2.0";

    // Mempool stats
    if (impl_->mempool) {
        stats.mempool_size = impl_->mempool->size();
        stats.mempool_bytes = impl_->mempool->total_size_bytes();
        stats.mempool_min_fee = impl_->mempool->get_min_fee_rate();
        stats.mempool_avg_fee = impl_->mempool->get_avg_fee_rate();
    }

    stats.timestamp = now;

    // Update cache
    impl_->network_stats_cache = stats;
    impl_->network_stats_cache_time = now;

    return stats;
}

MempoolSummary Explorer::get_mempool_summary() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    MempoolSummary summary;

    if (!impl_->mempool) {
        return summary;
    }

    summary.size = impl_->mempool->size();
    summary.bytes = impl_->mempool->total_size_bytes();
    summary.usage = impl_->mempool->memory_usage();
    summary.max_mempool = impl_->mempool->max_size();
    summary.min_fee_rate = impl_->mempool->get_min_fee_rate();
    summary.median_fee_rate = impl_->mempool->get_median_fee_rate();
    summary.avg_fee_rate = impl_->mempool->get_avg_fee_rate();
    summary.total_fee = impl_->mempool->total_fees();

    // Build fee histogram
    std::vector<double> boundaries = {1, 2, 3, 5, 10, 20, 50, 100, 200, 500, 1000};
    for (size_t i = 0; i < boundaries.size(); ++i) {
        FeeBucket bucket;
        bucket.min_fee_rate = (i == 0) ? 0 : boundaries[i - 1];
        bucket.max_fee_rate = boundaries[i];
        bucket.tx_count = impl_->mempool->count_in_fee_range(
            bucket.min_fee_rate, bucket.max_fee_rate);
        bucket.total_size = impl_->mempool->size_in_fee_range(
            bucket.min_fee_rate, bucket.max_fee_rate);
        summary.fee_histogram.push_back(bucket);
    }

    summary.last_update = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return summary;
}

std::vector<MempoolTx> Explorer::get_mempool_transactions(
    uint32_t limit, uint32_t offset) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<MempoolTx> result;

    if (!impl_->mempool) {
        return result;
    }

    auto txs = impl_->mempool->get_sorted_transactions(limit, offset);

    for (const auto& tx : txs) {
        MempoolTx mtx;
        mtx.tx_hash = tx.hash;
        mtx.size = tx.size;
        mtx.vsize = tx.vsize;
        mtx.fee = tx.fee;
        mtx.fee_rate = static_cast<double>(tx.fee) / static_cast<double>(tx.vsize);
        mtx.time = tx.time;
        mtx.descendant_count = tx.descendant_count;
        mtx.descendant_size = tx.descendant_size;
        mtx.descendant_fees = tx.descendant_fees;
        mtx.ancestor_count = tx.ancestor_count;
        mtx.ancestor_size = tx.ancestor_size;
        mtx.ancestor_fees = tx.ancestor_fees;
        mtx.depends = tx.depends;
        mtx.rbf = tx.rbf;
        result.push_back(mtx);
    }

    return result;
}

double Explorer::estimate_fee(uint32_t target_blocks) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->mempool) {
        return 1.0;  // Default 1 sat/vbyte
    }

    return impl_->mempool->estimate_fee(target_blocks);
}

Explorer::FeeRecommendation Explorer::get_fee_recommendation() const {
    FeeRecommendation rec;

    rec.fastest = estimate_fee(1);
    rec.fast = estimate_fee(3);
    rec.medium = estimate_fee(6);
    rec.slow = estimate_fee(12);
    rec.economy = estimate_fee(24);

    // Ensure minimum fees
    rec.fastest = std::max(rec.fastest, 1.0);
    rec.fast = std::max(rec.fast, 1.0);
    rec.medium = std::max(rec.medium, 1.0);
    rec.slow = std::max(rec.slow, 1.0);
    rec.economy = std::max(rec.economy, 1.0);

    return rec;
}

std::optional<AddressStats> Explorer::get_address_stats(
    const std::string& address) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->initialized) {
        return std::nullopt;
    }

    auto stats_opt = impl_->blockchain->get_address_stats(address);
    if (!stats_opt) {
        return std::nullopt;
    }

    AddressStats stats;
    stats.address = address;
    stats.balance = stats_opt->balance;
    stats.total_received = stats_opt->total_received;
    stats.total_sent = stats_opt->total_sent;
    stats.tx_count = stats_opt->tx_count;
    stats.unspent_count = stats_opt->unspent_count;
    stats.first_seen = stats_opt->first_seen;
    stats.last_seen = stats_opt->last_seen;

    return stats;
}

std::vector<UTXO> Explorer::get_address_utxos(const std::string& address,
                                               uint32_t limit) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<UTXO> result;

    if (!impl_->initialized) {
        return result;
    }

    auto utxos = impl_->blockchain->get_utxos_for_address(address);

    for (size_t i = 0; i < std::min(static_cast<size_t>(limit), utxos.size()); ++i) {
        UTXO utxo;
        utxo.tx_hash = utxos[i].tx_hash;
        utxo.output_index = utxos[i].output_index;
        utxo.value = utxos[i].value;
        utxo.script_pubkey = utxos[i].script_pubkey_hex;
        utxo.address = address;
        utxo.confirmations = utxos[i].confirmations;
        utxo.coinbase = utxos[i].coinbase;
        result.push_back(utxo);
    }

    return result;
}

Explorer::SearchResult Explorer::search(const std::string& query) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    SearchResult result;
    result.type = SearchResult::UNKNOWN;
    result.value = query;

    if (!impl_->initialized) {
        return result;
    }

    // Try as block height
    try {
        uint32_t height = std::stoul(query);
        if (impl_->blockchain->has_block_at_height(height)) {
            result.type = SearchResult::BLOCK;
            return result;
        }
    } catch (...) {}

    // Try as block hash (64 hex chars)
    if (query.length() == 64 && query.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
        if (impl_->blockchain->has_block(query)) {
            result.type = SearchResult::BLOCK;
            return result;
        }
        if (impl_->blockchain->has_transaction(query)) {
            result.type = SearchResult::TRANSACTION;
            return result;
        }
    }

    // Try as address
    if (query.length() >= 26 && query.length() <= 42 && query[0] == 'i') {
        if (impl_->blockchain->has_address(query)) {
            result.type = SearchResult::ADDRESS;
            return result;
        }
    }

    return result;
}

void Explorer::add_known_address(const std::string& address,
                                  const std::string& label) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->known_addresses[address] = label;
}

std::optional<std::string> Explorer::get_address_label(
    const std::string& address) const {
    auto it = impl_->known_addresses.find(address);
    if (it != impl_->known_addresses.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::map<std::string, std::string> Explorer::get_known_addresses() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->known_addresses;
}

std::vector<std::pair<uint64_t, uint64_t>> Explorer::get_tx_count_chart(
    uint32_t days) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<std::pair<uint64_t, uint64_t>> result;

    if (!impl_->initialized) {
        return result;
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (uint32_t i = 0; i < days; ++i) {
        uint64_t day_start = now - ((days - i) * 86400);
        uint64_t day_end = day_start + 86400;
        uint64_t tx_count = impl_->blockchain->get_transactions_in_range(
            day_start, day_end);
        result.emplace_back(day_start, tx_count);
    }

    return result;
}

std::vector<std::pair<uint64_t, double>> Explorer::get_difficulty_chart(
    uint32_t days) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<std::pair<uint64_t, double>> result;

    if (!impl_->initialized) {
        return result;
    }

    auto history = impl_->blockchain->get_difficulty_history(days);
    for (const auto& [timestamp, difficulty] : history) {
        result.emplace_back(timestamp, difficulty);
    }

    return result;
}

// ========== Explorer Server Implementation ==========

class ExplorerServer::Impl {
public:
    Explorer& explorer;
    uint16_t port;
    bool running = false;

    Impl(Explorer& exp, uint16_t p) : explorer(exp), port(p) {}
};

ExplorerServer::ExplorerServer(Explorer& explorer, uint16_t port)
    : impl_(std::make_unique<Impl>(explorer, port)) {}

ExplorerServer::~ExplorerServer() {
    stop();
}

bool ExplorerServer::start() {
    // HTTP server implementation would go here
    // Using a library like cpp-httplib or Boost.Beast
    impl_->running = true;
    return true;
}

void ExplorerServer::stop() {
    impl_->running = false;
}

bool ExplorerServer::is_running() const {
    return impl_->running;
}

std::string ExplorerServer::get_url() const {
    return "http://localhost:" + std::to_string(impl_->port);
}

} // namespace explorer
} // namespace intcoin
