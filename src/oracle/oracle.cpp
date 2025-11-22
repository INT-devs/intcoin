// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/oracle/oracle.h"
#include "../../include/intcoin/crypto/hash.h"
#include <algorithm>
#include <numeric>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace intcoin {
namespace oracle {

//==============================================================================
// PriceFeedOracle Implementation
//==============================================================================

PriceFeedOracle::PriceFeedOracle() {
    // Add default price pairs
    add_price_pair("BTC/USD");
    add_price_pair("ETH/USD");
    add_price_pair("INT/USD");
    add_price_pair("LTC/USD");
}

PriceFeedOracle::~PriceFeedOracle() = default;

bool PriceFeedOracle::update_price(const std::string& pair, const OracleDataPoint& data_point) {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    auto it = price_feeds_.find(pair);
    if (it == price_feeds_.end()) {
        return false;
    }

    // Parse price from data point
    double price = utils::parse_price(data_point.value);
    if (price <= 0.0) {
        return false;
    }

    PriceFeed& feed = it->second;
    
    // Store old price for change calculation
    double old_price = feed.price;
    
    // Update feed
    feed.pair = pair;
    feed.price = price;
    feed.timestamp = data_point.timestamp;
    feed.sources.push_back(data_point);

    // Keep only recent sources (last 10)
    if (feed.sources.size() > 10) {
        feed.sources.erase(feed.sources.begin());
    }

    // Calculate statistics
    update_statistics(feed);

    // Add to history
    price_history_[pair].push_back(feed);
    
    // Keep history limited (last 1000 entries)
    if (price_history_[pair].size() > 1000) {
        price_history_[pair].erase(price_history_[pair].begin());
    }

    return true;
}

std::optional<PriceFeed> PriceFeedOracle::get_price(const std::string& pair) const {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    auto it = price_feeds_.find(pair);
    if (it == price_feeds_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<std::string> PriceFeedOracle::get_supported_pairs() const {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    std::vector<std::string> pairs;
    pairs.reserve(price_feeds_.size());

    for (const auto& [pair, feed] : price_feeds_) {
        pairs.push_back(pair);
    }

    return pairs;
}

std::vector<PriceFeed> PriceFeedOracle::get_price_history(
    const std::string& pair,
    uint32_t limit
) const {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    auto it = price_history_.find(pair);
    if (it == price_history_.end()) {
        return {};
    }

    const auto& history = it->second;
    
    // Return last 'limit' entries
    size_t start = history.size() > limit ? history.size() - limit : 0;
    return std::vector<PriceFeed>(history.begin() + start, history.end());
}

double PriceFeedOracle::get_average_price(
    const std::string& pair,
    uint32_t duration_seconds
) const {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    auto it = price_history_.find(pair);
    if (it == price_history_.end() || it->second.empty()) {
        return 0.0;
    }

    const auto& history = it->second;
    uint64_t cutoff_time = utils::get_current_timestamp() - duration_seconds;

    double sum = 0.0;
    uint32_t count = 0;

    for (auto rit = history.rbegin(); rit != history.rend(); ++rit) {
        if (rit->timestamp < cutoff_time) {
            break;
        }
        sum += rit->price;
        count++;
    }

    return count > 0 ? sum / count : 0.0;
}

void PriceFeedOracle::add_price_pair(const std::string& pair) {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    if (price_feeds_.find(pair) == price_feeds_.end()) {
        PriceFeed feed;
        feed.pair = pair;
        price_feeds_[pair] = feed;
    }
}

void PriceFeedOracle::remove_price_pair(const std::string& pair) {
    std::lock_guard<std::mutex> lock(feeds_mutex_);

    price_feeds_.erase(pair);
    price_history_.erase(pair);
}

void PriceFeedOracle::update_statistics(PriceFeed& feed) {
    // Calculate 24h change from history
    auto hist_it = price_history_.find(feed.pair);
    if (hist_it != price_history_.end() && !hist_it->second.empty()) {
        uint64_t time_24h_ago = utils::get_current_timestamp() - 86400;
        
        // Find price 24h ago
        for (auto rit = hist_it->second.rbegin(); rit != hist_it->second.rend(); ++rit) {
            if (rit->timestamp <= time_24h_ago) {
                double old_price = rit->price;
                if (old_price > 0.0) {
                    feed.change_24h = ((feed.price - old_price) / old_price) * 100.0;
                }
                break;
            }
        }
    }
}

//==============================================================================
// OracleAggregator Implementation
//==============================================================================

OracleAggregator::OracleAggregator() = default;

OracleAggregator::~OracleAggregator() = default;

bool OracleAggregator::register_provider(const OracleProvider& provider) {
    std::lock_guard<std::mutex> lock(providers_mutex_);

    // Check if already registered
    if (providers_.find(provider.public_key) != providers_.end()) {
        return false;
    }

    providers_[provider.public_key] = provider;
    return true;
}

bool OracleAggregator::unregister_provider(const PublicKey& provider_key) {
    std::lock_guard<std::mutex> lock(providers_mutex_);

    return providers_.erase(provider_key) > 0;
}

std::vector<OracleProvider> OracleAggregator::get_providers() const {
    std::lock_guard<std::mutex> lock(providers_mutex_);

    std::vector<OracleProvider> providers;
    providers.reserve(providers_.size());

    for (const auto& [key, provider] : providers_) {
        providers.push_back(provider);
    }

    return providers;
}

std::optional<OracleProvider> OracleAggregator::get_provider(const PublicKey& key) const {
    std::lock_guard<std::mutex> lock(providers_mutex_);

    auto it = providers_.find(key);
    if (it == providers_.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool OracleAggregator::submit_data(const OracleDataPoint& data_point) {
    // Verify signature
    if (!verify_data_point(data_point)) {
        return false;
    }

    // Check provider is registered
    {
        std::lock_guard<std::mutex> lock(providers_mutex_);
        auto it = providers_.find(data_point.provider);
        if (it == providers_.end() || !it->second.is_active) {
            return false;
        }

        // Update provider stats
        it->second.last_update = utils::get_current_timestamp();
        it->second.total_submissions++;
    }

    // Store data point
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        data_points_[data_point.key].push_back(data_point);
    }

    return true;
}

std::optional<AggregatedData> OracleAggregator::get_aggregated_data(
    const std::string& key
) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_points_.find(key);
    if (it == data_points_.end() || it->second.empty()) {
        return std::nullopt;
    }

    return aggregate(key, it->second);
}

std::vector<OracleDataPoint> OracleAggregator::get_data_points(
    const std::string& key
) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = data_points_.find(key);
    if (it == data_points_.end()) {
        return {};
    }

    return it->second;
}

bool OracleAggregator::verify_data_point(const OracleDataPoint& data_point) const {
    // Verify quantum-resistant signature
    return verify_quantum_signature(data_point);
}

void OracleAggregator::update_provider_reputation(
    const PublicKey& provider_key,
    bool positive
) {
    std::lock_guard<std::mutex> lock(providers_mutex_);

    auto it = providers_.find(provider_key);
    if (it == providers_.end()) {
        return;
    }

    if (positive) {
        it->second.reputation = std::min(100u, it->second.reputation + 1);
    } else {
        it->second.reputation = it->second.reputation > 0 ? it->second.reputation - 1 : 0;
    }

    // Deactivate if reputation too low
    if (it->second.reputation < 20) {
        it->second.is_active = false;
    }
}

void OracleAggregator::remove_stale_data(uint64_t max_age_seconds) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    uint64_t cutoff_time = utils::get_current_timestamp() - max_age_seconds;

    for (auto& [key, points] : data_points_) {
        points.erase(
            std::remove_if(points.begin(), points.end(),
                [cutoff_time](const OracleDataPoint& p) {
                    return p.timestamp < cutoff_time;
                }),
            points.end()
        );
    }
}

void OracleAggregator::clear_data(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    data_points_.erase(key);
}

AggregatedData OracleAggregator::aggregate(
    const std::string& key,
    const std::vector<OracleDataPoint>& points
) const {
    AggregatedData result;
    result.key = key;
    result.num_sources = points.size();
    result.timestamp = utils::get_current_timestamp();
    result.sources = points;

    if (points.empty()) {
        return result;
    }

    // Extract values
    std::vector<std::string> values;
    values.reserve(points.size());
    for (const auto& point : points) {
        values.push_back(point.value);
    }

    // Calculate aggregates
    result.median_value = utils::calculate_median(values);
    result.mean_value = utils::calculate_mean(values);
    result.confidence = utils::calculate_confidence(points);

    return result;
}

bool OracleAggregator::verify_quantum_signature(const OracleDataPoint& data_point) const {
    // In production: verify using quantum-resistant signature scheme (Dilithium/SPHINCS+)
    // For now, simplified verification
    return !data_point.signature.empty();
}

//==============================================================================
// OracleNetwork Implementation
//==============================================================================

OracleNetwork::OracleNetwork() : running_(false) {}

OracleNetwork::~OracleNetwork() {
    stop();
}

bool OracleNetwork::start() {
    if (running_) {
        return true;
    }

    std::lock_guard<std::mutex> lock(network_mutex_);
    running_ = true;

    // Start monitoring thread
    std::thread([this]() { monitor_network(); }).detach();

    return true;
}

void OracleNetwork::stop() {
    if (!running_) {
        return;
    }

    std::lock_guard<std::mutex> lock(network_mutex_);
    running_ = false;
}

bool OracleNetwork::submit_data(const OracleDataPoint& data_point) {
    if (!running_) {
        return false;
    }

    // Submit to aggregator
    if (!aggregator_.submit_data(data_point)) {
        return false;
    }

    // If it's a price feed, update price oracle
    if (data_point.type == OracleDataType::PRICE) {
        price_feed_.update_price(data_point.key, data_point);
    }

    return true;
}

std::optional<AggregatedData> OracleNetwork::query_data(const std::string& key) const {
    if (!running_) {
        return std::nullopt;
    }

    return aggregator_.get_aggregated_data(key);
}

std::optional<PriceFeed> OracleNetwork::query_price(const std::string& pair) const {
    if (!running_) {
        return std::nullopt;
    }

    return price_feed_.get_price(pair);
}

bool OracleNetwork::add_provider(const OracleProvider& provider) {
    if (!running_) {
        return false;
    }

    return aggregator_.register_provider(provider);
}

std::vector<OracleProvider> OracleNetwork::list_providers() const {
    return aggregator_.get_providers();
}

OracleNetwork::NetworkStats OracleNetwork::get_stats() const {
    NetworkStats stats;

    auto providers = aggregator_.get_providers();
    stats.total_providers = providers.size();

    for (const auto& provider : providers) {
        if (provider.is_active) {
            stats.active_providers++;
        }
        stats.total_data_points += provider.total_submissions;
        if (provider.last_update > stats.last_update) {
            stats.last_update = provider.last_update;
        }
    }

    stats.price_pairs_count = price_feed_.get_supported_pairs().size();

    return stats;
}

void OracleNetwork::monitor_network() {
    while (running_) {
        // Remove stale data (older than 1 hour)
        aggregator_.remove_stale_data(3600);

        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

//==============================================================================
// Oracle Utilities
//==============================================================================

namespace utils {

double parse_price(const std::string& value) {
    try {
        return std::stod(value);
    } catch (...) {
        return 0.0;
    }
}

std::string format_price(double price) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(8) << price;
    return oss.str();
}

uint64_t get_current_timestamp() {
    return static_cast<uint64_t>(std::time(nullptr));
}

bool is_timestamp_recent(uint64_t timestamp, uint64_t max_age_seconds) {
    uint64_t current = get_current_timestamp();
    return (current - timestamp) <= max_age_seconds;
}

Signature sign_data_point(const OracleDataPoint& data, const PrivateKey& key) {
    // In production: use quantum-resistant signature (Dilithium/SPHINCS+)
    // For now, use simplified signature
    
    crypto::SHA256 hasher;
    hasher.update(reinterpret_cast<const uint8_t*>(data.key.data()), data.key.size());
    hasher.update(reinterpret_cast<const uint8_t*>(data.value.data()), data.value.size());
    
    Hash256 hash;
    hasher.finalize(hash.data());

    // Placeholder signature
    Signature signature;
    signature.resize(64);
    std::copy(hash.begin(), hash.end(), signature.begin());
    
    return signature;
}

bool verify_data_signature(const OracleDataPoint& data) {
    // In production: verify quantum-resistant signature
    return !data.signature.empty();
}

std::string calculate_median(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "0";
    }

    // Convert to doubles
    std::vector<double> nums;
    nums.reserve(values.size());
    
    for (const auto& val : values) {
        nums.push_back(parse_price(val));
    }

    // Sort
    std::sort(nums.begin(), nums.end());

    // Calculate median
    double median;
    size_t size = nums.size();
    if (size % 2 == 0) {
        median = (nums[size/2 - 1] + nums[size/2]) / 2.0;
    } else {
        median = nums[size/2];
    }

    return format_price(median);
}

std::string calculate_mean(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "0";
    }

    double sum = 0.0;
    for (const auto& val : values) {
        sum += parse_price(val);
    }

    return format_price(sum / values.size());
}

uint32_t calculate_confidence(const std::vector<OracleDataPoint>& points) {
    if (points.empty()) {
        return 0;
    }

    // Calculate based on:
    // 1. Number of sources (more is better)
    // 2. Agreement between sources (low variance is better)
    // 3. Recency of data (recent is better)

    uint32_t num_sources = points.size();
    uint32_t source_score = std::min(num_sources * 20, 60u);  // Max 60 for 3+ sources

    // Calculate variance
    std::vector<double> values;
    for (const auto& point : points) {
        values.push_back(parse_price(point.value));
    }

    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0;
    for (double val : values) {
        variance += (val - mean) * (val - mean);
    }
    variance /= values.size();

    // Low variance = high agreement
    double cv = mean > 0.0 ? std::sqrt(variance) / mean : 1.0;  // Coefficient of variation
    uint32_t agreement_score = cv < 0.01 ? 30 : (cv < 0.05 ? 20 : 10);

    // Recency score
    uint64_t current = get_current_timestamp();
    uint32_t recency_score = 0;
    for (const auto& point : points) {
        uint64_t age = current - point.timestamp;
        if (age < 60) recency_score = 10;  // Less than 1 minute
        else if (age < 300) recency_score = 5;  // Less than 5 minutes
    }

    return std::min(source_score + agreement_score + recency_score, 100u);
}

} // namespace utils

} // namespace oracle
} // namespace intcoin
