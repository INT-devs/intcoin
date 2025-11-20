// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Machine Learning Integration for INTcoin
// Predictive analytics, anomaly detection, and optimization

#ifndef INTCOIN_ML_H
#define INTCOIN_ML_H

#include "primitives.h"
#include "transaction.h"
#include "blockchain.h"
#include <vector>
#include <string>
#include <map>
#include <optional>
#include <memory>
#include <functional>

namespace intcoin {
namespace ml {

/**
 * Transaction Fee Prediction
 * Predict optimal transaction fees based on mempool state and historical data
 */
class FeePredictionModel {
public:
    FeePredictionModel();

    // Training
    void train_on_historical_data(const std::vector<Transaction>& txs,
                                   const std::vector<uint32_t>& confirmation_times);
    void update_model(const Transaction& tx, uint32_t confirmation_time);

    // Prediction
    struct FeePrediction {
        uint64_t recommended_fee;      // Satoshis
        uint64_t low_fee;              // Budget option (slower)
        uint64_t high_fee;             // Priority option (faster)
        double confidence_score;       // 0.0-1.0
        uint32_t estimated_blocks;     // Blocks until confirmation
        uint32_t estimated_seconds;    // Time until confirmation
    };
    FeePrediction predict_fee(size_t tx_size_bytes, uint32_t target_blocks = 6) const;

    // Analysis
    double get_mempool_congestion() const;
    std::vector<double> get_fee_distribution() const;

    // Model persistence
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);

private:
    struct TrainingData {
        size_t tx_size;
        uint64_t fee;
        uint32_t confirmation_blocks;
        uint64_t timestamp;
        uint32_t mempool_size;
    };
    std::vector<TrainingData> training_data_;

    // Model parameters (simple linear regression + exponential moving average)
    double fee_per_byte_avg_;
    double fee_per_byte_std_;
    std::map<uint32_t, double> target_block_multipliers_;  // 1 block, 3 blocks, 6 blocks, etc.

    // Time-series smoothing
    struct ExponentialMovingAverage {
        double value;
        double alpha;  // Smoothing factor (0-1)
        void update(double new_value) { value = alpha * new_value + (1 - alpha) * value; }
    };
    ExponentialMovingAverage current_congestion_;

    // Internal calculations
    double calculate_base_fee_rate() const;
    double calculate_congestion_multiplier() const;
    void update_statistics();
};

/**
 * Network Traffic Prediction
 * Predict network congestion and optimal connection strategies
 */
class NetworkTrafficPredictor {
public:
    NetworkTrafficPredictor();

    // Training
    void record_network_event(uint64_t timestamp, uint32_t peer_count,
                               uint64_t bandwidth_in, uint64_t bandwidth_out);

    // Prediction
    struct TrafficForecast {
        uint32_t predicted_peer_count;
        uint64_t predicted_bandwidth_in;
        uint64_t predicted_bandwidth_out;
        double network_health_score;  // 0.0-1.0
        bool congestion_warning;
    };
    TrafficForecast predict_traffic(uint32_t minutes_ahead) const;

    // Recommendations
    uint32_t recommend_connection_limit() const;
    bool should_throttle_connections() const;
    std::vector<std::string> get_optimization_suggestions() const;

private:
    struct NetworkSnapshot {
        uint64_t timestamp;
        uint32_t peer_count;
        uint64_t bandwidth_in;
        uint64_t bandwidth_out;
    };
    std::vector<NetworkSnapshot> history_;
    static constexpr size_t MAX_HISTORY = 10080;  // 1 week at 1-minute intervals

    // Time-series analysis
    double calculate_trend(const std::vector<double>& values) const;
    double calculate_seasonality(uint64_t timestamp) const;
};

/**
 * Anomaly Detection
 * Detect unusual patterns that may indicate attacks or bugs
 */
class AnomalyDetector {
public:
    enum class AnomalyType {
        NONE,
        UNUSUAL_TRANSACTION_PATTERN,
        POTENTIAL_DOUBLE_SPEND,
        MEMPOOL_SPAM,
        PEER_MISBEHAVIOR,
        MINING_ANOMALY,
        FORK_DETECTION,
        UNUSUAL_FEE_SPIKE,
        NETWORK_PARTITION
    };

    struct Anomaly {
        AnomalyType type;
        double severity;  // 0.0-1.0 (1.0 = critical)
        std::string description;
        uint64_t timestamp;
        std::map<std::string, std::string> details;
    };

    AnomalyDetector();

    // Real-time detection
    std::optional<Anomaly> check_transaction(const Transaction& tx);
    std::optional<Anomaly> check_block(const Block& block);
    std::optional<Anomaly> check_mempool_state(size_t mempool_size, double avg_fee);
    std::optional<Anomaly> check_peer_behavior(const std::string& peer_id,
                                                 const std::vector<std::string>& recent_messages);

    // Batch analysis
    std::vector<Anomaly> analyze_recent_activity(uint32_t blocks_back = 100);

    // Configuration
    void set_sensitivity(double sensitivity);  // 0.0-1.0 (higher = more sensitive)
    void whitelist_address(const std::string& address);
    void blacklist_address(const std::string& address);

    // Training (learn normal patterns)
    void train_on_historical_data(const Blockchain& blockchain);
    void update_baseline();

private:
    double sensitivity_;
    std::set<std::string> whitelisted_addresses_;
    std::set<std::string> blacklisted_addresses_;

    // Statistical baselines
    struct NormalPattern {
        double avg_tx_size;
        double std_tx_size;
        double avg_tx_fee;
        double std_tx_fee;
        double avg_block_time;
        double std_block_time;
        uint32_t avg_mempool_size;
    };
    NormalPattern baseline_;

    // Detection algorithms
    bool is_statistical_outlier(double value, double mean, double std_dev, double threshold) const;
    double calculate_z_score(double value, double mean, double std_dev) const;
    bool check_for_double_spend_attempt(const Transaction& tx);
};

/**
 * Mining Difficulty Predictor
 * Predict future mining difficulty adjustments
 */
class DifficultyPredictor {
public:
    DifficultyPredictor();

    // Training
    void record_difficulty_change(uint32_t height, const Hash256& difficulty, uint64_t timestamp);

    // Prediction
    struct DifficultyForecast {
        Hash256 predicted_difficulty;
        double confidence;
        uint32_t blocks_until_adjustment;
        int32_t percent_change;  // +10 = 10% increase, -5 = 5% decrease
    };
    DifficultyForecast predict_next_difficulty() const;
    std::vector<DifficultyForecast> predict_difficulty_trend(uint32_t adjustments_ahead) const;

    // Analysis
    double get_current_hashrate_estimate() const;
    double get_hashrate_trend() const;  // Positive = increasing, negative = decreasing

private:
    struct DifficultyRecord {
        uint32_t height;
        Hash256 difficulty;
        uint64_t timestamp;
        double hashrate_estimate;
    };
    std::vector<DifficultyRecord> history_;

    double calculate_hashrate(const Hash256& difficulty, uint64_t block_time) const;
};

/**
 * Lightning Network Route Optimization
 * ML-based route finding for Lightning payments
 */
class LightningRouteOptimizer {
public:
    LightningRouteOptimizer();

    // Route scoring
    struct RouteScore {
        std::vector<std::string> node_path;
        double success_probability;
        uint64_t total_fees;
        uint32_t total_time_locks;
        double reliability_score;
        double overall_score;  // Weighted combination
    };

    // Route prediction
    std::vector<RouteScore> find_optimal_routes(const std::string& source,
                                                  const std::string& destination,
                                                  uint64_t amount,
                                                  size_t max_routes = 5);

    // Learning
    void record_payment_result(const std::vector<std::string>& route,
                                bool success,
                                uint64_t actual_fees,
                                uint32_t completion_time_ms);

    // Node reputation
    double get_node_reliability(const std::string& node_id) const;
    void update_node_reputation(const std::string& node_id, bool payment_success);

private:
    // Node statistics
    struct NodeStats {
        uint32_t payment_attempts;
        uint32_t payment_successes;
        uint64_t total_fees_charged;
        uint32_t avg_response_time_ms;
        double reliability_score;
    };
    std::map<std::string, NodeStats> node_stats_;

    // Route history
    struct RouteAttempt {
        std::vector<std::string> path;
        bool success;
        uint64_t amount;
        uint64_t fees;
        uint64_t timestamp;
    };
    std::vector<RouteAttempt> route_history_;

    // Scoring algorithms
    double calculate_success_probability(const std::vector<std::string>& route, uint64_t amount) const;
    double calculate_reliability_score(const std::vector<std::string>& route) const;
};

/**
 * Smart Mempool Management
 * ML-based transaction prioritization and eviction
 */
class SmartMempoolManager {
public:
    SmartMempoolManager();

    // Transaction scoring
    struct TransactionScore {
        double priority_score;      // Higher = more important
        double fee_per_byte;
        double time_in_mempool;
        double spam_likelihood;     // 0.0-1.0
        bool should_keep;
        bool should_relay;
    };
    TransactionScore score_transaction(const Transaction& tx) const;

    // Eviction strategy
    std::vector<Hash256> select_transactions_to_evict(size_t target_count);

    // Learning
    void record_transaction_outcome(const Hash256& tx_hash, bool was_mined, uint32_t blocks_waited);

    // Configuration
    void set_mempool_size_target(size_t bytes);

private:
    size_t mempool_size_target_;

    struct TxHistory {
        uint64_t first_seen;
        uint64_t fee;
        size_t size;
        bool was_mined;
        uint32_t blocks_until_mined;
    };
    std::map<Hash256, TxHistory> tx_history_;

    double calculate_spam_likelihood(const Transaction& tx) const;
};

/**
 * Peer Quality Scoring
 * Evaluate peer reliability and usefulness
 */
class PeerQualityScorer {
public:
    PeerQualityScorer();

    struct PeerScore {
        double overall_quality;     // 0.0-1.0
        double response_time;       // Lower = better
        double uptime_ratio;
        double data_quality;        // Valid vs invalid messages
        double bandwidth_efficiency;
        bool should_maintain_connection;
    };

    // Scoring
    PeerScore score_peer(const std::string& peer_id) const;
    std::vector<std::string> get_best_peers(size_t count) const;
    std::vector<std::string> get_worst_peers(size_t count) const;

    // Updates
    void record_peer_message(const std::string& peer_id, bool valid, uint64_t size);
    void record_peer_response_time(const std::string& peer_id, uint32_t ms);
    void record_peer_disconnect(const std::string& peer_id, const std::string& reason);

private:
    struct PeerStats {
        uint32_t valid_messages;
        uint32_t invalid_messages;
        uint64_t bytes_received;
        uint64_t bytes_sent;
        std::vector<uint32_t> response_times_ms;
        uint64_t connection_start;
        uint32_t disconnection_count;
    };
    std::map<std::string, PeerStats> peer_stats_;

    double calculate_quality_score(const PeerStats& stats) const;
};

/**
 * Machine Learning Model Manager
 * Centralized ML model training and inference
 */
class MLModelManager {
public:
    static MLModelManager& instance();

    // Initialize all ML models
    void initialize(const Blockchain& blockchain);
    void shutdown();

    // Access models
    FeePredictionModel& fee_predictor() { return *fee_predictor_; }
    NetworkTrafficPredictor& traffic_predictor() { return *traffic_predictor_; }
    AnomalyDetector& anomaly_detector() { return *anomaly_detector_; }
    DifficultyPredictor& difficulty_predictor() { return *difficulty_predictor_; }
    LightningRouteOptimizer& route_optimizer() { return *route_optimizer_; }
    SmartMempoolManager& mempool_manager() { return *mempool_manager_; }
    PeerQualityScorer& peer_scorer() { return *peer_scorer_; }

    // Batch training
    void train_all_models(const Blockchain& blockchain);
    void save_all_models(const std::string& directory) const;
    void load_all_models(const std::string& directory);

    // Statistics
    struct MLStats {
        uint32_t models_loaded;
        uint64_t total_predictions;
        uint64_t training_samples;
        double avg_model_accuracy;
    };
    MLStats get_statistics() const;

private:
    MLModelManager() = default;
    ~MLModelManager() = default;
    MLModelManager(const MLModelManager&) = delete;
    MLModelManager& operator=(const MLModelManager&) = delete;

    std::unique_ptr<FeePredictionModel> fee_predictor_;
    std::unique_ptr<NetworkTrafficPredictor> traffic_predictor_;
    std::unique_ptr<AnomalyDetector> anomaly_detector_;
    std::unique_ptr<DifficultyPredictor> difficulty_predictor_;
    std::unique_ptr<LightningRouteOptimizer> route_optimizer_;
    std::unique_ptr<SmartMempoolManager> mempool_manager_;
    std::unique_ptr<PeerQualityScorer> peer_scorer_;

    bool initialized_;
};

/**
 * ML Configuration
 */
struct MLConfig {
    bool enable_fee_prediction;
    bool enable_traffic_prediction;
    bool enable_anomaly_detection;
    bool enable_route_optimization;
    bool enable_smart_mempool;
    bool enable_peer_scoring;

    double anomaly_sensitivity;  // 0.0-1.0
    uint32_t training_interval_blocks;
    std::string model_storage_path;

    MLConfig()
        : enable_fee_prediction(true)
        , enable_traffic_prediction(true)
        , enable_anomaly_detection(true)
        , enable_route_optimization(true)
        , enable_smart_mempool(true)
        , enable_peer_scoring(true)
        , anomaly_sensitivity(0.7)
        , training_interval_blocks(2016)  // ~2 weeks
        , model_storage_path("data/ml_models/")
    {}
};

} // namespace ml
} // namespace intcoin

#endif // INTCOIN_ML_H
