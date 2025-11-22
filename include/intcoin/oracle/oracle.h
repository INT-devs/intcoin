// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_ORACLE_H
#define INTCOIN_ORACLE_H

#include "../primitives.h"
#include "../crypto/crypto.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>

namespace intcoin {
namespace oracle {

/**
 * Oracle data types
 */
enum class OracleDataType {
    PRICE,              // Price feed (e.g., BTC/USD)
    WEATHER,            // Weather data
    SPORTS,             // Sports results
    RANDOM,             // Random number
    GENERAL,            // General external data
    TIMESTAMP,          // Verified timestamp
    CUSTOM              // Custom data type
};

/**
 * Oracle data source
 */
struct OracleDataPoint {
    std::string key;                    // Data identifier (e.g., "BTC/USD")
    std::string value;                  // Data value
    OracleDataType type;                // Data type
    uint64_t timestamp;                 // Unix timestamp
    PublicKey provider;                 // Oracle provider public key
    Signature signature;                // Quantum-resistant signature
    uint32_t confidence;                // Confidence level (0-100)

    OracleDataPoint() : type(OracleDataType::GENERAL), timestamp(0), confidence(0) {}
};

/**
 * Aggregated oracle data
 */
struct AggregatedData {
    std::string key;                    // Data identifier
    std::string median_value;           // Median of all values
    std::string mean_value;             // Mean of all values
    uint32_t num_sources;               // Number of data sources
    uint64_t timestamp;                 // Aggregation timestamp
    uint32_t confidence;                // Overall confidence
    std::vector<OracleDataPoint> sources;  // Individual data points

    AggregatedData() : num_sources(0), timestamp(0), confidence(0) {}
};

/**
 * Oracle provider information
 */
struct OracleProvider {
    PublicKey public_key;               // Provider's public key
    std::string name;                   // Provider name
    std::string endpoint;               // API endpoint
    uint32_t reputation;                // Reputation score (0-100)
    bool is_active;                     // Active status
    uint64_t last_update;               // Last data submission
    uint64_t total_submissions;         // Total data points submitted

    OracleProvider() : reputation(50), is_active(true), 
                      last_update(0), total_submissions(0) {}
};

/**
 * Price feed data
 */
struct PriceFeed {
    std::string pair;                   // Trading pair (e.g., "BTC/USD")
    double price;                       // Current price
    double volume_24h;                  // 24-hour volume
    double change_24h;                  // 24-hour price change %
    uint64_t timestamp;                 // Price timestamp
    std::vector<OracleDataPoint> sources;  // Price sources

    PriceFeed() : price(0.0), volume_24h(0.0), change_24h(0.0), timestamp(0) {}
};

/**
 * Oracle data provider
 *
 * Base class for oracle data sources
 */
class OracleDataProvider {
public:
    virtual ~OracleDataProvider() = default;

    // Provider lifecycle
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;

    // Data submission
    virtual bool submit_data(const OracleDataPoint& data_point) = 0;
    virtual std::optional<OracleDataPoint> fetch_data(const std::string& key) = 0;

    // Provider info
    virtual OracleProvider get_provider_info() const = 0;
    virtual bool verify_signature(const OracleDataPoint& data_point) const = 0;
};

/**
 * Price feed oracle
 *
 * Specialized oracle for cryptocurrency price feeds
 */
class PriceFeedOracle {
public:
    PriceFeedOracle();
    ~PriceFeedOracle();

    // Price feed operations
    bool update_price(const std::string& pair, const OracleDataPoint& data_point);
    std::optional<PriceFeed> get_price(const std::string& pair) const;
    std::vector<std::string> get_supported_pairs() const;

    // Price history
    std::vector<PriceFeed> get_price_history(const std::string& pair, uint32_t limit = 100) const;
    double get_average_price(const std::string& pair, uint32_t duration_seconds) const;

    // Management
    void add_price_pair(const std::string& pair);
    void remove_price_pair(const std::string& pair);

private:
    std::map<std::string, PriceFeed> price_feeds_;
    std::map<std::string, std::vector<PriceFeed>> price_history_;
    mutable std::mutex feeds_mutex_;

    void update_statistics(PriceFeed& feed);
};

/**
 * Oracle aggregator
 *
 * Aggregates data from multiple oracle providers with quantum-resistant verification
 */
class OracleAggregator {
public:
    OracleAggregator();
    ~OracleAggregator();

    // Provider management
    bool register_provider(const OracleProvider& provider);
    bool unregister_provider(const PublicKey& provider_key);
    std::vector<OracleProvider> get_providers() const;
    std::optional<OracleProvider> get_provider(const PublicKey& key) const;

    // Data aggregation
    bool submit_data(const OracleDataPoint& data_point);
    std::optional<AggregatedData> get_aggregated_data(const std::string& key) const;
    std::vector<OracleDataPoint> get_data_points(const std::string& key) const;

    // Data verification
    bool verify_data_point(const OracleDataPoint& data_point) const;
    void update_provider_reputation(const PublicKey& provider_key, bool positive);

    // Cleanup
    void remove_stale_data(uint64_t max_age_seconds);
    void clear_data(const std::string& key);

private:
    std::map<PublicKey, OracleProvider> providers_;
    std::map<std::string, std::vector<OracleDataPoint>> data_points_;
    mutable std::mutex providers_mutex_;
    mutable std::mutex data_mutex_;

    AggregatedData aggregate(const std::string& key, const std::vector<OracleDataPoint>& points) const;
    bool verify_quantum_signature(const OracleDataPoint& data_point) const;
};

/**
 * Oracle network
 *
 * Manages oracle network connectivity and data distribution
 */
class OracleNetwork {
public:
    OracleNetwork();
    ~OracleNetwork();

    // Network lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Components
    OracleAggregator& get_aggregator() { return aggregator_; }
    PriceFeedOracle& get_price_feed() { return price_feed_; }

    // Data operations
    bool submit_data(const OracleDataPoint& data_point);
    std::optional<AggregatedData> query_data(const std::string& key) const;
    std::optional<PriceFeed> query_price(const std::string& pair) const;

    // Provider management
    bool add_provider(const OracleProvider& provider);
    std::vector<OracleProvider> list_providers() const;

    // Statistics
    struct NetworkStats {
        uint32_t total_providers;
        uint32_t active_providers;
        uint64_t total_data_points;
        uint32_t price_pairs_count;
        uint64_t last_update;

        NetworkStats() : total_providers(0), active_providers(0),
                        total_data_points(0), price_pairs_count(0), last_update(0) {}
    };

    NetworkStats get_stats() const;

private:
    bool running_;
    OracleAggregator aggregator_;
    PriceFeedOracle price_feed_;
    mutable std::mutex network_mutex_;

    void monitor_network();
};

/**
 * Oracle utilities
 */
namespace utils {

// Data parsing
double parse_price(const std::string& value);
std::string format_price(double price);

// Timestamp utilities
uint64_t get_current_timestamp();
bool is_timestamp_recent(uint64_t timestamp, uint64_t max_age_seconds);

// Signature utilities
Signature sign_data_point(const OracleDataPoint& data, const PrivateKey& key);
bool verify_data_signature(const OracleDataPoint& data);

// Aggregation utilities
std::string calculate_median(const std::vector<std::string>& values);
std::string calculate_mean(const std::vector<std::string>& values);
uint32_t calculate_confidence(const std::vector<OracleDataPoint>& points);

} // namespace utils

} // namespace oracle
} // namespace intcoin

#endif // INTCOIN_ORACLE_H
