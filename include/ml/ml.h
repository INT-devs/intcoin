// INTcoin Machine Learning Module
// Provides ML-based analysis and prediction for blockchain operations
// Copyright (c) 2025 INTcoin Team

#ifndef INTCOIN_ML_H
#define INTCOIN_ML_H

#include "../intcoin/types.h"
#include "../intcoin/transaction.h"
#include "../intcoin/block.h"
#include "../intcoin/network.h"
#include <vector>
#include <memory>
#include <map>
#include <chrono>

namespace intcoin {
namespace ml {

// ============================================================================
// ML Configuration
// ============================================================================

struct MLConfig {
    // Anomaly detection thresholds
    double anomaly_threshold = 0.85;  // Probability threshold for flagging
    size_t training_window_size = 1000;  // Number of samples for training

    // Fee estimation
    size_t fee_history_size = 500;  // Transactions to consider for fee estimation
    double fee_confidence_interval = 0.95;  // 95% confidence

    // Network analysis
    size_t peer_history_size = 100;  // Peer interactions to track
    double malicious_peer_threshold = 0.7;  // Probability threshold

    // Model update frequency
    std::chrono::minutes model_update_interval{60};  // Update models every hour
};

// ============================================================================
// Transaction Anomaly Detection
// ============================================================================

// Transaction features for ML analysis
struct TransactionFeatures {
    // Amount features
    double total_input_amount = 0.0;
    double total_output_amount = 0.0;
    double fee_rate = 0.0;  // Fee per byte

    // Structural features
    size_t num_inputs = 0;
    size_t num_outputs = 0;
    size_t transaction_size = 0;  // In bytes

    // Temporal features
    uint64_t timestamp = 0;
    uint32_t block_height = 0;

    // Behavioral features
    bool has_multiple_outputs = false;  // Possible mixing
    bool has_round_amounts = false;  // Suspicious round numbers
    double output_distribution_entropy = 0.0;  // Output amount distribution

    // Extract features from a transaction
    static TransactionFeatures Extract(const Transaction& tx, uint32_t height = 0);
};

// Anomaly detection result
struct AnomalyScore {
    double score = 0.0;  // 0.0 (normal) to 1.0 (highly anomalous)
    bool is_anomalous = false;
    std::string reason;
    std::map<std::string, double> feature_contributions;  // Which features contributed
};

// Transaction anomaly detector using statistical ML
class TransactionAnomalyDetector {
public:
    TransactionAnomalyDetector(const MLConfig& config = MLConfig());
    ~TransactionAnomalyDetector();

    // Train the model on historical transactions
    Result<void> Train(const std::vector<Transaction>& transactions);

    // Detect if a transaction is anomalous
    Result<AnomalyScore> Detect(const Transaction& tx);

    // Update model with new transaction (online learning)
    Result<void> Update(const Transaction& tx, bool is_valid);

    // Get model statistics
    struct ModelStats {
        size_t training_samples = 0;
        double accuracy = 0.0;
        double false_positive_rate = 0.0;
        std::chrono::system_clock::time_point last_updated;
    };
    ModelStats GetStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Network Behavior Analysis
// ============================================================================

// Peer behavior features
struct PeerBehaviorFeatures {
    // Message patterns
    size_t total_messages_sent = 0;
    size_t total_messages_received = 0;
    double message_rate = 0.0;  // Messages per minute

    // Block/transaction propagation
    size_t blocks_relayed = 0;
    size_t transactions_relayed = 0;
    size_t invalid_blocks_sent = 0;
    size_t invalid_transactions_sent = 0;

    // Response times
    double average_response_time_ms = 0.0;
    double response_time_variance = 0.0;

    // Connection patterns
    std::chrono::seconds connection_duration{0};
    size_t disconnection_count = 0;

    // Ban score history
    int current_ban_score = 0;
    std::vector<int> ban_score_history;

    // Extract features from peer
    static PeerBehaviorFeatures Extract(const Peer& peer);
};

// Peer reputation score
struct PeerReputationScore {
    double trust_score = 0.5;  // 0.0 (untrusted) to 1.0 (highly trusted)
    double malicious_probability = 0.0;  // Probability peer is malicious
    bool is_trusted = false;
    bool should_ban = false;
    std::string assessment;
};

// Network behavior analyzer
class NetworkBehaviorAnalyzer {
public:
    NetworkBehaviorAnalyzer(const MLConfig& config = MLConfig());
    ~NetworkBehaviorAnalyzer();

    // Train on known good/bad peer behavior
    Result<void> Train(const std::vector<Peer>& good_peers,
                      const std::vector<Peer>& bad_peers);

    // Analyze peer behavior
    Result<PeerReputationScore> AnalyzePeer(const Peer& peer);

    // Update model with peer feedback
    Result<void> UpdatePeerReputation(uint64_t peer_id, bool is_good);

    // Get recommended peers to connect to
    std::vector<uint64_t> GetRecommendedPeers(
        const std::vector<std::shared_ptr<Peer>>& available_peers,
        size_t count = 8);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Fee Estimation
// ============================================================================

// Fee recommendation
struct FeeRecommendation {
    uint64_t low_priority_fee = 0;      // Fee for low priority (slow confirmation)
    uint64_t medium_priority_fee = 0;   // Fee for medium priority (medium confirmation)
    uint64_t high_priority_fee = 0;     // Fee for high priority (fast confirmation)

    double confidence = 0.0;  // Confidence in the estimate (0.0 to 1.0)
    uint32_t estimated_blocks_low = 0;     // Estimated blocks for low priority
    uint32_t estimated_blocks_medium = 0;  // Estimated blocks for medium priority
    uint32_t estimated_blocks_high = 0;    // Estimated blocks for high priority
};

// Fee estimator using ML
class FeeEstimator {
public:
    FeeEstimator(const MLConfig& config = MLConfig());
    ~FeeEstimator();

    // Train on historical transaction confirmations
    struct ConfirmationData {
        Transaction tx;
        uint32_t confirmation_block_height;
        uint32_t blocks_to_confirm;  // How many blocks it took
    };
    Result<void> Train(const std::vector<ConfirmationData>& history);

    // Get fee recommendation for a transaction
    Result<FeeRecommendation> EstimateFee(size_t tx_size_bytes,
                                         uint32_t target_blocks = 6);

    // Update with confirmed transaction
    Result<void> UpdateWithConfirmation(const Transaction& tx,
                                       uint32_t blocks_to_confirm);

    // Get current mempool-based estimates
    Result<FeeRecommendation> EstimateFromMempool(
        const std::vector<Transaction>& mempool_txs,
        size_t tx_size_bytes);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Mining Difficulty Prediction
// ============================================================================

// Hashrate estimate
struct HashrateEstimate {
    double network_hashrate = 0.0;  // Hashes per second
    double difficulty_next_block = 0.0;
    uint32_t confidence_percent = 0;
    std::chrono::seconds estimated_block_time{0};
};

// Difficulty predictor
class DifficultyPredictor {
public:
    DifficultyPredictor();
    ~DifficultyPredictor();

    // Train on historical difficulty adjustments
    struct DifficultyHistory {
        uint32_t block_height;
        uint32_t difficulty_bits;
        uint64_t timestamp;
        uint64_t actual_hashrate;  // If known
    };
    Result<void> Train(const std::vector<DifficultyHistory>& history);

    // Predict next difficulty
    Result<HashrateEstimate> PredictNextDifficulty(
        uint32_t current_height,
        uint32_t current_bits,
        const std::vector<uint64_t>& recent_block_times);

    // Estimate current network hashrate
    Result<double> EstimateNetworkHashrate(
        const std::vector<uint64_t>& recent_block_times);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Integrated ML Manager
// ============================================================================

// Main ML manager that coordinates all ML components
class MLManager {
public:
    MLManager(const MLConfig& config = MLConfig());
    ~MLManager();

    // Initialize all ML models
    Result<void> Initialize();

    // Component accessors
    TransactionAnomalyDetector& GetAnomalyDetector();
    NetworkBehaviorAnalyzer& GetNetworkAnalyzer();
    FeeEstimator& GetFeeEstimator();
    DifficultyPredictor& GetDifficultyPredictor();

    // Train all models on blockchain data
    Result<void> TrainOnBlockchain(const std::vector<Block>& blocks);

    // Update models (should be called periodically)
    Result<void> UpdateModels();

    // Get overall system health
    struct SystemHealth {
        bool models_trained = false;
        size_t total_training_samples = 0;
        double overall_accuracy = 0.0;
        std::chrono::system_clock::time_point last_update;
    };
    SystemHealth GetSystemHealth() const;

    // Save/load models to disk
    Result<void> SaveModels(const std::string& directory);
    Result<void> LoadModels(const std::string& directory);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Utility Functions
// ============================================================================

// Statistical utilities for ML
namespace stats {
    // Calculate mean of a dataset
    double Mean(const std::vector<double>& data);

    // Calculate standard deviation
    double StdDev(const std::vector<double>& data);

    // Calculate entropy
    double Entropy(const std::vector<double>& probabilities);

    // Normalize data to [0, 1]
    std::vector<double> Normalize(const std::vector<double>& data);

    // Calculate Z-score
    double ZScore(double value, double mean, double std_dev);

    // Calculate percentile
    double Percentile(const std::vector<double>& data, double percentile);
}

// Simple neural network for classification
class SimpleNeuralNetwork {
public:
    SimpleNeuralNetwork(size_t input_size, size_t hidden_size, size_t output_size);
    ~SimpleNeuralNetwork();

    // Forward pass
    std::vector<double> Forward(const std::vector<double>& input);

    // Train with backpropagation
    Result<void> Train(const std::vector<std::vector<double>>& inputs,
                      const std::vector<std::vector<double>>& targets,
                      size_t epochs = 100,
                      double learning_rate = 0.01);

    // Predict
    std::vector<double> Predict(const std::vector<double>& input);

    // Save/load weights
    Result<void> SaveWeights(const std::string& filepath);
    Result<void> LoadWeights(const std::string& filepath);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ml
} // namespace intcoin

#endif // INTCOIN_ML_H
