// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_MEMPOOL_ANALYTICS_FEE_ESTIMATOR_H
#define INTCOIN_MEMPOOL_ANALYTICS_FEE_ESTIMATOR_H

#include <cstdint>
#include <vector>
#include <memory>

namespace intcoin {
namespace mempool_analytics {

/**
 * Fee estimation result
 */
struct FeeEstimate {
    double fee_rate{0.0};                // Recommended fee rate (sat/byte)
    double confidence{0.0};              // Confidence level (0.0-1.0)
    uint32_t target_blocks{0};           // Target confirmation blocks
    uint64_t timestamp{0};               // Estimate timestamp
};

/**
 * Fee range with confidence interval
 */
struct FeeRange {
    double min_fee_rate{0.0};            // Minimum fee rate
    double max_fee_rate{0.0};            // Maximum fee rate
    double optimal_fee_rate{0.0};        // Optimal fee rate
    double confidence{0.0};              // Confidence level
};

/**
 * Block data for fee estimation
 */
struct BlockData {
    uint32_t height{0};
    uint64_t timestamp{0};
    std::vector<double> fee_rates;       // Fee rates of included transactions
    uint64_t total_size{0};              // Total block size
};

/**
 * Feature vector for ML model
 */
struct FeatureVector {
    uint32_t hour_of_day{0};            // 0-23
    uint32_t day_of_week{0};            // 0-6 (Sunday = 0)
    uint64_t mempool_size{0};           // Current mempool size
    double avg_fee_rate{0.0};           // Average fee rate
    double median_fee_rate{0.0};        // Median fee rate
    uint32_t recent_blocks{0};          // Blocks mined in last hour
};

/**
 * Machine Learning-based Fee Estimator
 *
 * Uses gradient boosting (XGBoost) to predict optimal fees
 * based on historical data and current mempool state.
 */
class FeeEstimator {
public:
    FeeEstimator();
    ~FeeEstimator();

    /**
     * Estimate fee for target confirmation time
     *
     * @param target_blocks Number of blocks for confirmation
     * @return Fee estimation
     */
    FeeEstimate EstimateFee(uint32_t target_blocks) const;

    /**
     * Get fee range with confidence interval
     *
     * @param target_blocks Number of blocks for confirmation
     * @param confidence Confidence level (e.g., 0.95 for 95%)
     * @return Fee range
     */
    FeeRange GetFeeRange(uint32_t target_blocks, double confidence) const;

    /**
     * Update model with new block data
     *
     * @param block Block data to train on
     */
    void UpdateModel(const BlockData& block);

    /**
     * Train model on historical data
     *
     * @param blocks Historical block data
     * @return True if training succeeded
     */
    bool TrainModel(const std::vector<BlockData>& blocks);

    /**
     * Load pre-trained model from file
     *
     * @param model_path Path to model file
     * @return True if loaded successfully
     */
    bool LoadModel(const std::string& model_path);

    /**
     * Save trained model to file
     *
     * @param model_path Path to save model
     * @return True if saved successfully
     */
    bool SaveModel(const std::string& model_path) const;

    /**
     * Get model accuracy metrics
     *
     * @return Map of metric name to value
     */
    std::map<std::string, double> GetModelMetrics() const;

private:
    /**
     * Extract features from current state
     *
     * @param target_blocks Target confirmation blocks
     * @return Feature vector
     */
    FeatureVector ExtractFeatures(uint32_t target_blocks) const;

    /**
     * Predict fee using ML model
     *
     * @param features Feature vector
     * @return Predicted fee rate
     */
    double PredictFeeRate(const FeatureVector& features) const;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace mempool_analytics
} // namespace intcoin

#endif // INTCOIN_MEMPOOL_ANALYTICS_FEE_ESTIMATOR_H
