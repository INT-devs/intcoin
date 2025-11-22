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

    // Access traditional models
    FeePredictionModel& fee_predictor() { return *fee_predictor_; }
    NetworkTrafficPredictor& traffic_predictor() { return *traffic_predictor_; }
    AnomalyDetector& anomaly_detector() { return *anomaly_detector_; }
    DifficultyPredictor& difficulty_predictor() { return *difficulty_predictor_; }
    LightningRouteOptimizer& route_optimizer() { return *route_optimizer_; }
    SmartMempoolManager& mempool_manager() { return *mempool_manager_; }
    PeerQualityScorer& peer_scorer() { return *peer_scorer_; }

    // Access advanced models (v1.3.0+)
    BridgeFraudDetector& fraud_detector() { return *fraud_detector_; }
    DeFiMarketPredictor& defi_predictor() { return *defi_predictor_; }
    ContractVulnerabilityDetector& contract_scanner() { return *contract_scanner_; }
    FederatedLearningCoordinator& federated_coordinator() { return *federated_coordinator_; }
    ModelEncryption& model_encryption() { return *model_encryption_; }

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
        uint32_t fraud_alerts_total;
        uint32_t contracts_scanned;
        uint32_t federated_participants;
    };
    MLStats get_statistics() const;

private:
    MLModelManager() = default;
    ~MLModelManager() = default;
    MLModelManager(const MLModelManager&) = delete;
    MLModelManager& operator=(const MLModelManager&) = delete;

    // Traditional models
    std::unique_ptr<FeePredictionModel> fee_predictor_;
    std::unique_ptr<NetworkTrafficPredictor> traffic_predictor_;
    std::unique_ptr<AnomalyDetector> anomaly_detector_;
    std::unique_ptr<DifficultyPredictor> difficulty_predictor_;
    std::unique_ptr<LightningRouteOptimizer> route_optimizer_;
    std::unique_ptr<SmartMempoolManager> mempool_manager_;
    std::unique_ptr<PeerQualityScorer> peer_scorer_;

    // Advanced models (v1.3.0+)
    std::unique_ptr<BridgeFraudDetector> fraud_detector_;
    std::unique_ptr<DeFiMarketPredictor> defi_predictor_;
    std::unique_ptr<ContractVulnerabilityDetector> contract_scanner_;
    std::unique_ptr<FederatedLearningCoordinator> federated_coordinator_;
    std::unique_ptr<ModelEncryption> model_encryption_;

    bool initialized_;
};

/**
 * Deep Learning Neural Network
 * Multi-layer perceptron for complex pattern recognition
 */
class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<size_t>& layer_sizes);
    ~NeuralNetwork();

    // Training
    void train(const std::vector<std::vector<double>>& inputs,
               const std::vector<std::vector<double>>& outputs,
               uint32_t epochs, double learning_rate);

    double get_training_loss() const { return training_loss_; }
    double get_validation_accuracy() const { return validation_accuracy_; }

    // Inference
    std::vector<double> predict(const std::vector<double>& input) const;
    std::vector<std::vector<double>> predict_batch(
        const std::vector<std::vector<double>>& inputs) const;

    // Model persistence
    bool save(const std::string& filepath) const;
    bool load(const std::string& filepath);

    // Architecture info
    std::vector<size_t> get_layer_sizes() const { return layer_sizes_; }
    size_t get_parameter_count() const;

private:
    std::vector<size_t> layer_sizes_;

    // Network weights and biases
    struct Layer {
        std::vector<std::vector<double>> weights;  // weights[neuron][input]
        std::vector<double> biases;
        std::vector<double> activations;
        std::vector<double> gradients;
    };
    std::vector<Layer> layers_;

    double training_loss_;
    double validation_accuracy_;

    // Activation functions
    double sigmoid(double x) const;
    double relu(double x) const;
    double tanh_activation(double x) const;

    // Forward and backward propagation
    void forward_pass(const std::vector<double>& input);
    void backward_pass(const std::vector<double>& target, double learning_rate);

    // Initialization
    void initialize_weights();
    double random_weight() const;
};

/**
 * Cross-Chain Bridge Fraud Detection
 * Detect fraudulent atomic swaps and bridge exploits using ML
 */
class BridgeFraudDetector {
public:
    BridgeFraudDetector();

    enum class FraudType {
        NONE,
        SUSPICIOUS_SWAP_PATTERN,
        AMOUNT_MANIPULATION,
        REPLAY_ATTACK,
        PRICE_ORACLE_MANIPULATION,
        LIQUIDITY_DRAIN_ATTACK,
        FLASH_LOAN_EXPLOIT,
        SANDWICH_ATTACK,
        FRONT_RUNNING
    };

    struct FraudAlert {
        FraudType type;
        double confidence;  // 0.0-1.0
        std::string swap_id;
        std::string description;
        std::vector<std::string> evidence;
        uint64_t timestamp;
        bool requires_immediate_action;
    };

    // Real-time monitoring
    std::optional<FraudAlert> analyze_swap(
        const std::string& swap_id,
        const std::string& chain_a,
        const std::string& chain_b,
        uint64_t amount_a,
        uint64_t amount_b,
        const std::string& initiator
    );

    std::optional<FraudAlert> analyze_liquidity_change(
        const std::string& pool_id,
        int64_t reserve_change_a,
        int64_t reserve_change_b,
        uint64_t timestamp
    );

    // Pattern learning
    void train_on_known_frauds(
        const std::vector<std::string>& fraud_swap_ids,
        const std::vector<FraudType>& fraud_types
    );

    void train_on_legitimate_swaps(
        const std::vector<std::string>& legit_swap_ids
    );

    // Reporting
    std::vector<FraudAlert> get_recent_alerts(uint32_t hours_back = 24) const;
    void mark_alert_as_false_positive(const std::string& swap_id);

private:
    std::unique_ptr<NeuralNetwork> fraud_model_;
    std::vector<FraudAlert> alert_history_;

    struct SwapPattern {
        double amount_ratio;
        double time_of_day;
        double tx_count_recent;
        uint32_t chain_pair_hash;
        bool user_has_history;
    };

    std::vector<double> extract_features(
        const std::string& chain_a,
        const std::string& chain_b,
        uint64_t amount_a,
        uint64_t amount_b,
        const std::string& initiator
    ) const;

    bool check_price_manipulation(uint64_t amount_a, uint64_t amount_b,
                                   const std::string& chain_a,
                                   const std::string& chain_b) const;
};

/**
 * DeFi Market Prediction
 * Predict liquidity pool prices and yield farming APY
 */
class DeFiMarketPredictor {
public:
    DeFiMarketPredictor();

    struct PricePrediction {
        double predicted_price;      // Token A / Token B ratio
        double confidence_interval_low;
        double confidence_interval_high;
        double volatility_estimate;
        uint32_t minutes_ahead;
    };

    struct YieldPrediction {
        double predicted_apy;
        double risk_score;  // 0.0-1.0 (1.0 = very risky)
        uint32_t days_ahead;
        std::string risk_factors;
    };

    // Price predictions
    PricePrediction predict_pool_price(
        const std::string& pool_id,
        uint32_t minutes_ahead = 60
    ) const;

    std::vector<PricePrediction> predict_price_trend(
        const std::string& pool_id,
        uint32_t hours_ahead = 24
    ) const;

    // Yield predictions
    YieldPrediction predict_yield(
        const std::string& pool_id,
        uint32_t lock_period_days
    ) const;

    // Opportunity detection
    struct ArbitrageOpportunity {
        std::string pool_a;
        std::string pool_b;
        double profit_percentage;
        uint64_t optimal_amount;
        uint32_t urgency;  // Minutes until opportunity expires
    };

    std::vector<ArbitrageOpportunity> find_arbitrage_opportunities() const;

    // Learning
    void record_pool_state(
        const std::string& pool_id,
        uint64_t reserve_a,
        uint64_t reserve_b,
        uint64_t volume_24h,
        uint64_t timestamp
    );

private:
    std::unique_ptr<NeuralNetwork> price_model_;
    std::unique_ptr<NeuralNetwork> yield_model_;

    struct PoolHistory {
        std::string pool_id;
        std::vector<uint64_t> timestamps;
        std::vector<double> prices;
        std::vector<uint64_t> volumes;
        std::vector<uint64_t> reserves_a;
        std::vector<uint64_t> reserves_b;
    };
    std::map<std::string, PoolHistory> pool_histories_;

    std::vector<double> extract_price_features(const std::string& pool_id) const;
    double calculate_volatility(const std::vector<double>& prices) const;
};

/**
 * Smart Contract Vulnerability Detection
 * Detect potential vulnerabilities in contract bytecode and behavior
 */
class ContractVulnerabilityDetector {
public:
    ContractVulnerabilityDetector();

    enum class VulnerabilityType {
        NONE,
        REENTRANCY,
        INTEGER_OVERFLOW,
        UNCHECKED_CALL,
        DELEGATECALL_INJECTION,
        TIMESTAMP_DEPENDENCE,
        TX_ORIGIN_AUTHENTICATION,
        UNPROTECTED_SELFDESTRUCT,
        UNCHECKED_RETURN_VALUE,
        DOS_GAS_LIMIT,
        FRONT_RUNNING_VULNERABLE
    };

    struct Vulnerability {
        VulnerabilityType type;
        double severity;  // 0.0-1.0 (1.0 = critical)
        std::string description;
        std::vector<uint32_t> bytecode_offsets;  // Where vulnerability detected
        std::string mitigation_advice;
        bool exploitable;
    };

    // Static analysis
    std::vector<Vulnerability> analyze_bytecode(
        const std::vector<uint8_t>& bytecode
    );

    std::vector<Vulnerability> analyze_source_code(
        const std::string& solidity_source
    );

    // Dynamic analysis
    std::vector<Vulnerability> analyze_execution_trace(
        const std::vector<uint8_t>& bytecode,
        const std::vector<std::vector<uint8_t>>& call_data_samples
    );

    // Pattern database
    void add_vulnerability_pattern(
        VulnerabilityType type,
        const std::vector<uint8_t>& bytecode_pattern,
        const std::string& description
    );

    // Scoring
    double calculate_contract_safety_score(
        const std::vector<uint8_t>& bytecode
    ) const;

private:
    std::unique_ptr<NeuralNetwork> vulnerability_model_;

    struct VulnerabilityPattern {
        VulnerabilityType type;
        std::vector<uint8_t> bytecode_signature;
        std::string description;
        double severity;
    };
    std::vector<VulnerabilityPattern> known_patterns_;

    std::vector<double> extract_bytecode_features(
        const std::vector<uint8_t>& bytecode
    ) const;

    bool matches_pattern(
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& pattern
    ) const;
};

/**
 * Federated Learning Coordinator
 * Privacy-preserving distributed ML across nodes
 */
class FederatedLearningCoordinator {
public:
    FederatedLearningCoordinator();

    struct ModelUpdate {
        std::string node_id;
        std::vector<std::vector<double>> gradient_updates;
        uint32_t training_samples;
        double local_loss;
        uint64_t timestamp;
    };

    // Coordination
    void register_participant(const std::string& node_id);
    void submit_model_update(const ModelUpdate& update);

    // Aggregation
    void aggregate_updates();
    std::vector<std::vector<double>> get_global_model() const;

    // Privacy protection
    void enable_differential_privacy(double epsilon, double delta);
    void add_noise_to_gradients(std::vector<std::vector<double>>& gradients);

    // Byzantine resilience
    void enable_byzantine_detection(double outlier_threshold = 3.0);
    std::vector<std::string> detect_malicious_participants(
        const std::vector<ModelUpdate>& updates
    );

    // Statistics
    struct FederatedStats {
        uint32_t participating_nodes;
        uint32_t training_rounds;
        double global_loss;
        double convergence_rate;
        std::vector<std::string> malicious_nodes_detected;
    };
    FederatedStats get_statistics() const;

private:
    std::vector<std::string> participants_;
    std::vector<ModelUpdate> pending_updates_;
    std::vector<std::vector<double>> global_model_;

    // Privacy settings
    bool differential_privacy_enabled_;
    double epsilon_;  // Privacy budget
    double delta_;    // Privacy parameter

    // Byzantine detection
    bool byzantine_detection_enabled_;
    double outlier_threshold_;
    std::set<std::string> flagged_nodes_;

    // Aggregation algorithms
    std::vector<std::vector<double>> federated_averaging(
        const std::vector<ModelUpdate>& updates
    );

    std::vector<std::vector<double>> median_aggregation(
        const std::vector<ModelUpdate>& updates
    );

    double calculate_update_distance(
        const std::vector<std::vector<double>>& a,
        const std::vector<std::vector<double>>& b
    ) const;

    std::vector<std::vector<double>> add_laplace_noise(
        const std::vector<std::vector<double>>& gradients,
        double scale
    ) const;
};

/**
 * Quantum-Resistant Model Protection
 * Encrypt and protect ML models using post-quantum cryptography
 */
class ModelEncryption {
public:
    ModelEncryption();

    // Encryption
    std::vector<uint8_t> encrypt_model(
        const std::vector<uint8_t>& model_data,
        const std::string& password
    );

    std::vector<uint8_t> decrypt_model(
        const std::vector<uint8_t>& encrypted_data,
        const std::string& password
    );

    // Digital signatures
    std::vector<uint8_t> sign_model(
        const std::vector<uint8_t>& model_data,
        const DilithiumPrivKey& private_key
    );

    bool verify_model_signature(
        const std::vector<uint8_t>& model_data,
        const std::vector<uint8_t>& signature,
        const DilithiumPubKey& public_key
    );

    // Key encapsulation for secure model transfer
    struct EncapsulatedModel {
        std::vector<uint8_t> encrypted_model;
        std::vector<uint8_t> encapsulated_key;  // Kyber-encrypted symmetric key
        std::vector<uint8_t> signature;
    };

    EncapsulatedModel encapsulate_model(
        const std::vector<uint8_t>& model_data,
        const KyberPubKey& recipient_public_key,
        const DilithiumPrivKey& sender_private_key
    );

    std::vector<uint8_t> decapsulate_model(
        const EncapsulatedModel& encapsulated,
        const KyberPrivKey& recipient_private_key,
        const DilithiumPubKey& sender_public_key
    );

private:
    // AES-256-GCM for symmetric encryption
    std::vector<uint8_t> aes_gcm_encrypt(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    std::vector<uint8_t> aes_gcm_decrypt(
        const std::vector<uint8_t>& encrypted_data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    // Key derivation
    std::vector<uint8_t> derive_key_from_password(
        const std::string& password,
        const std::vector<uint8_t>& salt
    );
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
    bool enable_deep_learning;
    bool enable_fraud_detection;
    bool enable_defi_prediction;
    bool enable_contract_scanning;
    bool enable_federated_learning;
    bool enable_model_encryption;

    double anomaly_sensitivity;  // 0.0-1.0
    uint32_t training_interval_blocks;
    std::string model_storage_path;

    // Neural network settings
    uint32_t nn_hidden_layers;
    uint32_t nn_neurons_per_layer;
    double nn_learning_rate;
    uint32_t nn_training_epochs;

    // Federated learning settings
    double federated_privacy_epsilon;
    double federated_privacy_delta;
    bool federated_byzantine_detection;

    MLConfig()
        : enable_fee_prediction(true)
        , enable_traffic_prediction(true)
        , enable_anomaly_detection(true)
        , enable_route_optimization(true)
        , enable_smart_mempool(true)
        , enable_peer_scoring(true)
        , enable_deep_learning(true)
        , enable_fraud_detection(true)
        , enable_defi_prediction(true)
        , enable_contract_scanning(true)
        , enable_federated_learning(false)  // Opt-in
        , enable_model_encryption(true)
        , anomaly_sensitivity(0.7)
        , training_interval_blocks(2016)  // ~2 weeks
        , model_storage_path("data/ml_models/")
        , nn_hidden_layers(3)
        , nn_neurons_per_layer(128)
        , nn_learning_rate(0.001)
        , nn_training_epochs(100)
        , federated_privacy_epsilon(1.0)
        , federated_privacy_delta(1e-5)
        , federated_byzantine_detection(true)
    {}
};

} // namespace ml
} // namespace intcoin

#endif // INTCOIN_ML_H
