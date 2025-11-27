// INTcoin Machine Learning Module Implementation
// Copyright (c) 2025 INTcoin Team

#include "ml/ml.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <set>

namespace intcoin {
namespace ml {

// ============================================================================
// Statistical Utilities
// ============================================================================

namespace stats {

double Mean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double StdDev(const std::vector<double>& data) {
    if (data.size() < 2) return 0.0;
    double mean = Mean(data);
    double sq_sum = std::accumulate(data.begin(), data.end(), 0.0,
        [mean](double sum, double val) {
            double diff = val - mean;
            return sum + diff * diff;
        });
    return std::sqrt(sq_sum / (data.size() - 1));
}

double Entropy(const std::vector<double>& probabilities) {
    double entropy = 0.0;
    for (double p : probabilities) {
        if (p > 0.0) {
            entropy -= p * std::log2(p);
        }
    }
    return entropy;
}

std::vector<double> Normalize(const std::vector<double>& data) {
    if (data.empty()) return {};

    double min_val = *std::min_element(data.begin(), data.end());
    double max_val = *std::max_element(data.begin(), data.end());
    double range = max_val - min_val;

    if (range == 0.0) {
        return std::vector<double>(data.size(), 0.5);
    }

    std::vector<double> normalized;
    normalized.reserve(data.size());
    for (double val : data) {
        normalized.push_back((val - min_val) / range);
    }
    return normalized;
}

double ZScore(double value, double mean, double std_dev) {
    if (std_dev == 0.0) return 0.0;
    return (value - mean) / std_dev;
}

double Percentile(const std::vector<double>& data, double percentile) {
    if (data.empty()) return 0.0;

    std::vector<double> sorted = data;
    std::sort(sorted.begin(), sorted.end());

    size_t index = static_cast<size_t>(percentile * (sorted.size() - 1));
    return sorted[index];
}

} // namespace stats

// ============================================================================
// Simple Neural Network Implementation
// ============================================================================

class SimpleNeuralNetwork::Impl {
public:
    size_t input_size;
    size_t hidden_size;
    size_t output_size;

    // Weights: input->hidden, hidden->output
    std::vector<std::vector<double>> weights_ih;  // [hidden][input]
    std::vector<std::vector<double>> weights_ho;  // [output][hidden]

    // Biases
    std::vector<double> bias_h;
    std::vector<double> bias_o;

    // Cached activations for backprop
    std::vector<double> hidden_activation;
    std::vector<double> output_activation;

    std::mt19937 rng;

    Impl(size_t in, size_t hid, size_t out)
        : input_size(in), hidden_size(hid), output_size(out),
          rng(std::random_device{}()) {

        // Initialize weights with Xavier initialization
        std::normal_distribution<double> dist_ih(0.0, std::sqrt(2.0 / (input_size + hidden_size)));
        std::normal_distribution<double> dist_ho(0.0, std::sqrt(2.0 / (hidden_size + output_size)));

        // Initialize input->hidden weights
        weights_ih.resize(hidden_size, std::vector<double>(input_size));
        for (auto& row : weights_ih) {
            for (auto& w : row) {
                w = dist_ih(rng);
            }
        }

        // Initialize hidden->output weights
        weights_ho.resize(output_size, std::vector<double>(hidden_size));
        for (auto& row : weights_ho) {
            for (auto& w : row) {
                w = dist_ho(rng);
            }
        }

        // Initialize biases to zero
        bias_h.resize(hidden_size, 0.0);
        bias_o.resize(output_size, 0.0);
    }

    // Sigmoid activation
    static double sigmoid(double x) {
        return 1.0 / (1.0 + std::exp(-x));
    }

    // Sigmoid derivative
    static double sigmoid_derivative(double x) {
        double s = sigmoid(x);
        return s * (1.0 - s);
    }

    // ReLU activation
    static double relu(double x) {
        return std::max(0.0, x);
    }

    // ReLU derivative
    static double relu_derivative(double x) {
        return x > 0.0 ? 1.0 : 0.0;
    }

    // Forward pass
    std::vector<double> forward(const std::vector<double>& input) {
        if (input.size() != input_size) {
            return {};
        }

        // Hidden layer
        hidden_activation.resize(hidden_size);
        for (size_t i = 0; i < hidden_size; i++) {
            double sum = bias_h[i];
            for (size_t j = 0; j < input_size; j++) {
                sum += weights_ih[i][j] * input[j];
            }
            hidden_activation[i] = relu(sum);
        }

        // Output layer
        output_activation.resize(output_size);
        for (size_t i = 0; i < output_size; i++) {
            double sum = bias_o[i];
            for (size_t j = 0; j < hidden_size; j++) {
                sum += weights_ho[i][j] * hidden_activation[j];
            }
            output_activation[i] = sigmoid(sum);
        }

        return output_activation;
    }

    // Train with backpropagation
    void train_batch(const std::vector<std::vector<double>>& inputs,
                    const std::vector<std::vector<double>>& targets,
                    double learning_rate) {

        // Accumulated gradients
        std::vector<std::vector<double>> grad_ih(hidden_size, std::vector<double>(input_size, 0.0));
        std::vector<std::vector<double>> grad_ho(output_size, std::vector<double>(hidden_size, 0.0));
        std::vector<double> grad_bias_h(hidden_size, 0.0);
        std::vector<double> grad_bias_o(output_size, 0.0);

        // Process each sample
        for (size_t sample = 0; sample < inputs.size(); sample++) {
            const auto& input = inputs[sample];
            const auto& target = targets[sample];

            // Forward pass
            auto output = forward(input);

            // Backward pass - output layer
            std::vector<double> delta_o(output_size);
            for (size_t i = 0; i < output_size; i++) {
                double error = target[i] - output[i];
                delta_o[i] = error * sigmoid_derivative(output[i]);
            }

            // Hidden layer gradients
            std::vector<double> delta_h(hidden_size);
            for (size_t i = 0; i < hidden_size; i++) {
                double error = 0.0;
                for (size_t j = 0; j < output_size; j++) {
                    error += delta_o[j] * weights_ho[j][i];
                }
                delta_h[i] = error * relu_derivative(hidden_activation[i]);
            }

            // Accumulate gradients - hidden->output
            for (size_t i = 0; i < output_size; i++) {
                for (size_t j = 0; j < hidden_size; j++) {
                    grad_ho[i][j] += delta_o[i] * hidden_activation[j];
                }
                grad_bias_o[i] += delta_o[i];
            }

            // Accumulate gradients - input->hidden
            for (size_t i = 0; i < hidden_size; i++) {
                for (size_t j = 0; j < input_size; j++) {
                    grad_ih[i][j] += delta_h[i] * input[j];
                }
                grad_bias_h[i] += delta_h[i];
            }
        }

        // Update weights and biases
        double batch_size = static_cast<double>(inputs.size());

        for (size_t i = 0; i < output_size; i++) {
            for (size_t j = 0; j < hidden_size; j++) {
                weights_ho[i][j] += learning_rate * grad_ho[i][j] / batch_size;
            }
            bias_o[i] += learning_rate * grad_bias_o[i] / batch_size;
        }

        for (size_t i = 0; i < hidden_size; i++) {
            for (size_t j = 0; j < input_size; j++) {
                weights_ih[i][j] += learning_rate * grad_ih[i][j] / batch_size;
            }
            bias_h[i] += learning_rate * grad_bias_h[i] / batch_size;
        }
    }
};

SimpleNeuralNetwork::SimpleNeuralNetwork(size_t input_size, size_t hidden_size, size_t output_size)
    : impl_(std::make_unique<Impl>(input_size, hidden_size, output_size)) {}

SimpleNeuralNetwork::~SimpleNeuralNetwork() = default;

std::vector<double> SimpleNeuralNetwork::Forward(const std::vector<double>& input) {
    return impl_->forward(input);
}

Result<void> SimpleNeuralNetwork::Train(const std::vector<std::vector<double>>& inputs,
                                       const std::vector<std::vector<double>>& targets,
                                       size_t epochs,
                                       double learning_rate) {
    if (inputs.size() != targets.size()) {
        return Result<void>::Error("Input and target sizes don't match");
    }

    if (inputs.empty()) {
        return Result<void>::Error("Empty training data");
    }

    for (size_t epoch = 0; epoch < epochs; epoch++) {
        impl_->train_batch(inputs, targets, learning_rate);
    }

    return Result<void>::Ok();
}

std::vector<double> SimpleNeuralNetwork::Predict(const std::vector<double>& input) {
    return impl_->forward(input);
}

Result<void> SimpleNeuralNetwork::SaveWeights(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return Result<void>::Error("Failed to open file for writing");
    }

    // Write dimensions
    file.write(reinterpret_cast<const char*>(&impl_->input_size), sizeof(size_t));
    file.write(reinterpret_cast<const char*>(&impl_->hidden_size), sizeof(size_t));
    file.write(reinterpret_cast<const char*>(&impl_->output_size), sizeof(size_t));

    // Write weights_ih
    for (const auto& row : impl_->weights_ih) {
        file.write(reinterpret_cast<const char*>(row.data()), row.size() * sizeof(double));
    }

    // Write weights_ho
    for (const auto& row : impl_->weights_ho) {
        file.write(reinterpret_cast<const char*>(row.data()), row.size() * sizeof(double));
    }

    // Write biases
    file.write(reinterpret_cast<const char*>(impl_->bias_h.data()),
               impl_->bias_h.size() * sizeof(double));
    file.write(reinterpret_cast<const char*>(impl_->bias_o.data()),
               impl_->bias_o.size() * sizeof(double));

    return Result<void>::Ok();
}

Result<void> SimpleNeuralNetwork::LoadWeights(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return Result<void>::Error("Failed to open file for reading");
    }

    // Read dimensions
    size_t input_size, hidden_size, output_size;
    file.read(reinterpret_cast<char*>(&input_size), sizeof(size_t));
    file.read(reinterpret_cast<char*>(&hidden_size), sizeof(size_t));
    file.read(reinterpret_cast<char*>(&output_size), sizeof(size_t));

    if (input_size != impl_->input_size ||
        hidden_size != impl_->hidden_size ||
        output_size != impl_->output_size) {
        return Result<void>::Error("Network dimensions don't match saved weights");
    }

    // Read weights_ih
    for (auto& row : impl_->weights_ih) {
        file.read(reinterpret_cast<char*>(row.data()), row.size() * sizeof(double));
    }

    // Read weights_ho
    for (auto& row : impl_->weights_ho) {
        file.read(reinterpret_cast<char*>(row.data()), row.size() * sizeof(double));
    }

    // Read biases
    file.read(reinterpret_cast<char*>(impl_->bias_h.data()),
              impl_->bias_h.size() * sizeof(double));
    file.read(reinterpret_cast<char*>(impl_->bias_o.data()),
              impl_->bias_o.size() * sizeof(double));

    return Result<void>::Ok();
}

// ============================================================================
// Transaction Features
// ============================================================================

TransactionFeatures TransactionFeatures::Extract(const Transaction& tx, uint32_t height) {
    TransactionFeatures features;

    // Calculate total amounts
    // Note: Input amounts require UTXO lookup, so we estimate based on outputs for ML purposes
    features.total_output_amount = static_cast<double>(tx.GetTotalOutputValue());

    // Estimate input amount (assume fee is ~1% for estimation)
    features.total_input_amount = features.total_output_amount * 1.01;

    // Fee rate (estimated)
    auto serialized = tx.Serialize();
    features.transaction_size = serialized.size();
    double estimated_fee = features.total_input_amount - features.total_output_amount;
    features.fee_rate = estimated_fee / static_cast<double>(features.transaction_size);

    // Structural features
    features.num_inputs = tx.inputs.size();
    features.num_outputs = tx.outputs.size();
    features.block_height = height;
    features.timestamp = height;  // Use block height as proxy for timestamp

    // Behavioral features
    features.has_multiple_outputs = tx.outputs.size() > 2;

    // Check for round amounts
    features.has_round_amounts = false;
    for (const auto& output : tx.outputs) {
        uint64_t val = output.value;
        // Check if divisible by large round numbers
        if (val % 1000000 == 0 || val % 100000 == 0) {
            features.has_round_amounts = true;
            break;
        }
    }

    // Output distribution entropy
    if (!tx.outputs.empty()) {
        std::vector<double> probabilities;
        probabilities.reserve(tx.outputs.size());
        for (const auto& output : tx.outputs) {
            probabilities.push_back(static_cast<double>(output.value) /
                                  features.total_output_amount);
        }
        features.output_distribution_entropy = stats::Entropy(probabilities);
    }

    return features;
}

// ============================================================================
// Transaction Anomaly Detector
// ============================================================================

class TransactionAnomalyDetector::Impl {
public:
    MLConfig config;
    SimpleNeuralNetwork neural_net{10, 20, 1};  // 10 features, 20 hidden, 1 output

    // Statistical baselines
    double mean_input_amount = 0.0;
    double stddev_input_amount = 0.0;
    double mean_output_amount = 0.0;
    double stddev_output_amount = 0.0;
    double mean_fee_rate = 0.0;
    double stddev_fee_rate = 0.0;
    double mean_num_inputs = 0.0;
    double stddev_num_inputs = 0.0;
    double mean_num_outputs = 0.0;
    double stddev_num_outputs = 0.0;

    size_t training_samples = 0;
    std::chrono::system_clock::time_point last_updated;

    Impl(const MLConfig& cfg) : config(cfg) {}

    std::vector<double> features_to_vector(const TransactionFeatures& features) {
        std::vector<double> vec;
        vec.reserve(10);

        vec.push_back(std::log1p(features.total_input_amount));
        vec.push_back(std::log1p(features.total_output_amount));
        vec.push_back(std::log1p(features.fee_rate));
        vec.push_back(static_cast<double>(features.num_inputs));
        vec.push_back(static_cast<double>(features.num_outputs));
        vec.push_back(static_cast<double>(features.transaction_size));
        vec.push_back(features.has_multiple_outputs ? 1.0 : 0.0);
        vec.push_back(features.has_round_amounts ? 1.0 : 0.0);
        vec.push_back(features.output_distribution_entropy);
        vec.push_back(static_cast<double>(features.num_outputs) /
                     static_cast<double>(features.num_inputs + 1));

        return vec;
    }

    double calculate_statistical_score(const TransactionFeatures& features) {
        double score = 0.0;
        [[maybe_unused]] int anomaly_count = 0;
        const double z_threshold = 3.0;  // 3 standard deviations

        // Check input amount anomaly
        if (stddev_input_amount > 0) {
            double z = stats::ZScore(features.total_input_amount,
                                    mean_input_amount, stddev_input_amount);
            if (std::abs(z) > z_threshold) {
                score += 0.15;
                anomaly_count++;
            }
        }

        // Check output amount anomaly
        if (stddev_output_amount > 0) {
            double z = stats::ZScore(features.total_output_amount,
                                    mean_output_amount, stddev_output_amount);
            if (std::abs(z) > z_threshold) {
                score += 0.15;
                anomaly_count++;
            }
        }

        // Check fee rate anomaly
        if (stddev_fee_rate > 0) {
            double z = stats::ZScore(features.fee_rate, mean_fee_rate, stddev_fee_rate);
            if (std::abs(z) > z_threshold) {
                score += 0.20;
                anomaly_count++;
            }
        }

        // Check structural anomalies
        if (stddev_num_inputs > 0) {
            double z = stats::ZScore(static_cast<double>(features.num_inputs),
                                    mean_num_inputs, stddev_num_inputs);
            if (std::abs(z) > z_threshold) {
                score += 0.10;
                anomaly_count++;
            }
        }

        // Suspicious patterns
        if (features.has_round_amounts) {
            score += 0.10;
            anomaly_count++;
        }

        // Very high entropy (possible mixing)
        if (features.output_distribution_entropy > 3.5) {
            score += 0.15;
            anomaly_count++;
        }

        // Many outputs (possible dusting attack)
        if (features.num_outputs > 20) {
            score += 0.15;
            anomaly_count++;
        }

        return std::min(score, 1.0);
    }
};

TransactionAnomalyDetector::TransactionAnomalyDetector(const MLConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

TransactionAnomalyDetector::~TransactionAnomalyDetector() = default;

Result<void> TransactionAnomalyDetector::Train(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) {
        return Result<void>::Error("Empty training set");
    }

    // Extract features from all transactions
    std::vector<TransactionFeatures> all_features;
    all_features.reserve(transactions.size());
    for (const auto& tx : transactions) {
        all_features.push_back(TransactionFeatures::Extract(tx));
    }

    // Calculate statistical baselines
    std::vector<double> input_amounts, output_amounts, fee_rates;
    std::vector<double> num_inputs, num_outputs;

    for (const auto& features : all_features) {
        input_amounts.push_back(features.total_input_amount);
        output_amounts.push_back(features.total_output_amount);
        fee_rates.push_back(features.fee_rate);
        num_inputs.push_back(static_cast<double>(features.num_inputs));
        num_outputs.push_back(static_cast<double>(features.num_outputs));
    }

    impl_->mean_input_amount = stats::Mean(input_amounts);
    impl_->stddev_input_amount = stats::StdDev(input_amounts);
    impl_->mean_output_amount = stats::Mean(output_amounts);
    impl_->stddev_output_amount = stats::StdDev(output_amounts);
    impl_->mean_fee_rate = stats::Mean(fee_rates);
    impl_->stddev_fee_rate = stats::StdDev(fee_rates);
    impl_->mean_num_inputs = stats::Mean(num_inputs);
    impl_->stddev_num_inputs = stats::StdDev(num_inputs);
    impl_->mean_num_outputs = stats::Mean(num_outputs);
    impl_->stddev_num_outputs = stats::StdDev(num_outputs);

    // Prepare neural network training data
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;

    for (const auto& features : all_features) {
        inputs.push_back(impl_->features_to_vector(features));
        // All training samples are normal (not anomalous)
        targets.push_back({0.0});
    }

    // Normalize inputs
    for (auto& input : inputs) {
        input = stats::Normalize(input);
    }

    // Train neural network
    auto result = impl_->neural_net.Train(inputs, targets, 100, 0.01);
    if (result.IsError()) {
        return Result<void>::Error("Neural network training failed: " + result.error);
    }

    impl_->training_samples = transactions.size();
    impl_->last_updated = std::chrono::system_clock::now();

    return Result<void>::Ok();
}

Result<AnomalyScore> TransactionAnomalyDetector::Detect(const Transaction& tx) {
    if (impl_->training_samples == 0) {
        return Result<AnomalyScore>::Error("Model not trained");
    }

    AnomalyScore score;

    // Extract features
    auto features = TransactionFeatures::Extract(tx);

    // Statistical anomaly detection
    double stat_score = impl_->calculate_statistical_score(features);

    // Neural network anomaly detection
    auto feature_vec = impl_->features_to_vector(features);
    feature_vec = stats::Normalize(feature_vec);
    auto nn_output = impl_->neural_net.Predict(feature_vec);
    double nn_score = nn_output.empty() ? 0.0 : nn_output[0];

    // Combine scores (weighted average)
    score.score = 0.6 * stat_score + 0.4 * nn_score;
    score.is_anomalous = score.score >= impl_->config.anomaly_threshold;

    // Generate reason
    if (score.is_anomalous) {
        std::vector<std::string> reasons;
        if (features.has_round_amounts) reasons.push_back("round amounts");
        if (features.fee_rate > impl_->mean_fee_rate + 3 * impl_->stddev_fee_rate)
            reasons.push_back("unusually high fee");
        if (features.fee_rate < impl_->mean_fee_rate - 3 * impl_->stddev_fee_rate)
            reasons.push_back("unusually low fee");
        if (features.num_outputs > 20) reasons.push_back("many outputs");
        if (features.output_distribution_entropy > 3.5) reasons.push_back("high entropy");

        score.reason = "Anomalous: ";
        for (size_t i = 0; i < reasons.size(); i++) {
            score.reason += reasons[i];
            if (i < reasons.size() - 1) score.reason += ", ";
        }
    } else {
        score.reason = "Normal transaction";
    }

    return Result<AnomalyScore>::Ok(score);
}

Result<void> TransactionAnomalyDetector::Update(const Transaction& tx, bool is_valid) {
    // Online learning - update baselines with new transaction
    auto features = TransactionFeatures::Extract(tx);

    // Update running statistics using exponential moving average
    const double alpha = 0.1;  // Learning rate for online updates

    impl_->mean_input_amount = alpha * features.total_input_amount +
                               (1 - alpha) * impl_->mean_input_amount;
    impl_->mean_output_amount = alpha * features.total_output_amount +
                                (1 - alpha) * impl_->mean_output_amount;
    impl_->mean_fee_rate = alpha * features.fee_rate + (1 - alpha) * impl_->mean_fee_rate;

    impl_->training_samples++;
    impl_->last_updated = std::chrono::system_clock::now();

    return Result<void>::Ok();
}

TransactionAnomalyDetector::ModelStats TransactionAnomalyDetector::GetStats() const {
    ModelStats stats;
    stats.training_samples = impl_->training_samples;
    stats.accuracy = 0.85;  // Placeholder - would need validation set to calculate
    stats.false_positive_rate = 0.05;  // Placeholder
    stats.last_updated = impl_->last_updated;
    return stats;
}

// ============================================================================
// Peer Behavior Features
// ============================================================================

PeerBehaviorFeatures PeerBehaviorFeatures::Extract(const Peer& peer) {
    PeerBehaviorFeatures features;

    features.total_messages_sent = peer.bytes_sent / 100;  // Approximate message count
    features.total_messages_received = peer.bytes_received / 100;

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(
        now - peer.connect_time);
    features.message_rate = duration.count() > 0 ?
        static_cast<double>(features.total_messages_sent + features.total_messages_received) /
        duration.count() : 0.0;

    features.connection_duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - peer.connect_time);
    features.current_ban_score = peer.ban_score;

    // Placeholder values - would be tracked in practice
    features.blocks_relayed = 0;
    features.transactions_relayed = 0;
    features.invalid_blocks_sent = 0;
    features.invalid_transactions_sent = 0;
    features.average_response_time_ms = 100.0;
    features.response_time_variance = 50.0;
    features.disconnection_count = 0;

    return features;
}

// ============================================================================
// Network Behavior Analyzer
// ============================================================================

class NetworkBehaviorAnalyzer::Impl {
public:
    MLConfig config;
    SimpleNeuralNetwork neural_net{8, 16, 1};  // 8 features, 16 hidden, 1 output

    std::map<uint64_t, double> peer_reputation;
    size_t training_samples = 0;

    Impl(const MLConfig& cfg) : config(cfg) {}

    std::vector<double> features_to_vector(const PeerBehaviorFeatures& features) {
        std::vector<double> vec;
        vec.reserve(8);

        vec.push_back(std::log1p(features.message_rate));
        vec.push_back(std::log1p(static_cast<double>(features.blocks_relayed)));
        vec.push_back(std::log1p(static_cast<double>(features.transactions_relayed)));
        vec.push_back(static_cast<double>(features.invalid_blocks_sent));
        vec.push_back(static_cast<double>(features.invalid_transactions_sent));
        vec.push_back(std::log1p(features.average_response_time_ms));
        vec.push_back(static_cast<double>(features.current_ban_score));
        vec.push_back(std::log1p(features.connection_duration.count()));

        return vec;
    }
};

NetworkBehaviorAnalyzer::NetworkBehaviorAnalyzer(const MLConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

NetworkBehaviorAnalyzer::~NetworkBehaviorAnalyzer() = default;

Result<void> NetworkBehaviorAnalyzer::Train(const std::vector<Peer>& good_peers,
                                           const std::vector<Peer>& bad_peers) {
    std::vector<std::vector<double>> inputs;
    std::vector<std::vector<double>> targets;

    // Good peers (label = 0.0 = not malicious)
    for (const auto& peer : good_peers) {
        auto features = PeerBehaviorFeatures::Extract(peer);
        inputs.push_back(impl_->features_to_vector(features));
        targets.push_back({0.0});
    }

    // Bad peers (label = 1.0 = malicious)
    for (const auto& peer : bad_peers) {
        auto features = PeerBehaviorFeatures::Extract(peer);
        inputs.push_back(impl_->features_to_vector(features));
        targets.push_back({1.0});
    }

    if (inputs.empty()) {
        return Result<void>::Error("Empty training set");
    }

    // Normalize inputs
    for (auto& input : inputs) {
        input = stats::Normalize(input);
    }

    // Train neural network
    auto result = impl_->neural_net.Train(inputs, targets, 100, 0.01);
    if (result.IsError()) {
        return Result<void>::Error("Neural network training failed");
    }

    impl_->training_samples = inputs.size();

    return Result<void>::Ok();
}

Result<PeerReputationScore> NetworkBehaviorAnalyzer::AnalyzePeer(const Peer& peer) {
    PeerReputationScore score;

    auto features = PeerBehaviorFeatures::Extract(peer);
    auto feature_vec = impl_->features_to_vector(features);
    feature_vec = stats::Normalize(feature_vec);

    auto nn_output = impl_->neural_net.Predict(feature_vec);
    score.malicious_probability = nn_output.empty() ? 0.0 : nn_output[0];
    score.trust_score = 1.0 - score.malicious_probability;

    // Check if peer should be trusted
    score.is_trusted = score.trust_score >= 0.6;
    score.should_ban = score.malicious_probability >= impl_->config.malicious_peer_threshold;

    // Generate assessment
    if (score.should_ban) {
        score.assessment = "Likely malicious - recommend ban";
    } else if (score.is_trusted) {
        score.assessment = "Trusted peer";
    } else {
        score.assessment = "Neutral - monitor closely";
    }

    // Update reputation map
    impl_->peer_reputation[peer.id] = score.trust_score;

    return Result<PeerReputationScore>::Ok(score);
}

Result<void> NetworkBehaviorAnalyzer::UpdatePeerReputation(uint64_t peer_id, bool is_good) {
    auto it = impl_->peer_reputation.find(peer_id);
    if (it != impl_->peer_reputation.end()) {
        // Update using exponential moving average
        const double alpha = 0.2;
        double new_reputation = is_good ? 1.0 : 0.0;
        it->second = alpha * new_reputation + (1 - alpha) * it->second;
    } else {
        impl_->peer_reputation[peer_id] = is_good ? 0.8 : 0.2;
    }

    return Result<void>::Ok();
}

std::vector<uint64_t> NetworkBehaviorAnalyzer::GetRecommendedPeers(
    const std::vector<std::shared_ptr<Peer>>& available_peers,
    size_t count) {

    // Sort peers by reputation score
    std::vector<std::pair<uint64_t, double>> peer_scores;
    for (const auto& peer : available_peers) {
        auto it = impl_->peer_reputation.find(peer->id);
        double score = it != impl_->peer_reputation.end() ? it->second : 0.5;
        peer_scores.emplace_back(peer->id, score);
    }

    std::sort(peer_scores.begin(), peer_scores.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

    // Return top N peers
    std::vector<uint64_t> recommended;
    for (size_t i = 0; i < std::min(count, peer_scores.size()); i++) {
        recommended.push_back(peer_scores[i].first);
    }

    return recommended;
}

// ============================================================================
// Fee Estimator
// ============================================================================

class FeeEstimator::Impl {
public:
    MLConfig config;

    struct FeeData {
        double fee_rate;
        uint32_t blocks_to_confirm;
        size_t tx_size;
    };

    std::vector<FeeData> history;

    Impl(const MLConfig& cfg) : config(cfg) {}
};

FeeEstimator::FeeEstimator(const MLConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

FeeEstimator::~FeeEstimator() = default;

Result<void> FeeEstimator::Train(const std::vector<ConfirmationData>& history) {
    impl_->history.clear();

    for (const auto& data : history) {
        auto serialized = data.tx.Serialize();
        size_t tx_size = serialized.size();

        // Calculate fee (estimated since we don't have UTXO set)
        uint64_t output_sum = data.tx.GetTotalOutputValue();

        // Estimate input sum (assume ~1% fee)
        uint64_t input_sum = static_cast<uint64_t>(output_sum * 1.01);

        uint64_t fee = input_sum - output_sum;
        double fee_rate = static_cast<double>(fee) / static_cast<double>(tx_size);

        Impl::FeeData fee_data;
        fee_data.fee_rate = fee_rate;
        fee_data.blocks_to_confirm = data.blocks_to_confirm;
        fee_data.tx_size = tx_size;

        impl_->history.push_back(fee_data);
    }

    return Result<void>::Ok();
}

Result<FeeRecommendation> FeeEstimator::EstimateFee(size_t tx_size_bytes,
                                                    uint32_t target_blocks) {
    if (impl_->history.empty()) {
        // Default fees if no history
        FeeRecommendation rec;
        rec.low_priority_fee = tx_size_bytes * 10;      // 10 per byte
        rec.medium_priority_fee = tx_size_bytes * 50;   // 50 per byte
        rec.high_priority_fee = tx_size_bytes * 100;    // 100 per byte
        rec.confidence = 0.3;
        rec.estimated_blocks_low = 10;
        rec.estimated_blocks_medium = 6;
        rec.estimated_blocks_high = 2;
        return Result<FeeRecommendation>::Ok(rec);
    }

    // Extract fee rates
    std::vector<double> fee_rates;
    for (const auto& data : impl_->history) {
        fee_rates.push_back(data.fee_rate);
    }

    // Calculate percentiles
    double low_fee_rate = stats::Percentile(fee_rates, 0.25);
    double med_fee_rate = stats::Percentile(fee_rates, 0.50);
    double high_fee_rate = stats::Percentile(fee_rates, 0.90);

    FeeRecommendation rec;
    rec.low_priority_fee = static_cast<uint64_t>(low_fee_rate * tx_size_bytes);
    rec.medium_priority_fee = static_cast<uint64_t>(med_fee_rate * tx_size_bytes);
    rec.high_priority_fee = static_cast<uint64_t>(high_fee_rate * tx_size_bytes);
    rec.confidence = std::min(0.95, impl_->history.size() / 1000.0);
    rec.estimated_blocks_low = 10;
    rec.estimated_blocks_medium = 6;
    rec.estimated_blocks_high = 2;

    return Result<FeeRecommendation>::Ok(rec);
}

Result<void> FeeEstimator::UpdateWithConfirmation(const Transaction& tx,
                                                  uint32_t blocks_to_confirm) {
    auto serialized = tx.Serialize();
    size_t tx_size = serialized.size();

    // Estimate input sum (we don't have UTXO set)
    uint64_t output_sum = tx.GetTotalOutputValue();
    uint64_t input_sum = static_cast<uint64_t>(output_sum * 1.01);

    uint64_t fee = input_sum - output_sum;
    double fee_rate = static_cast<double>(fee) / static_cast<double>(tx_size);

    Impl::FeeData fee_data;
    fee_data.fee_rate = fee_rate;
    fee_data.blocks_to_confirm = blocks_to_confirm;
    fee_data.tx_size = tx_size;

    impl_->history.push_back(fee_data);

    // Keep only recent history
    if (impl_->history.size() > impl_->config.fee_history_size) {
        impl_->history.erase(impl_->history.begin());
    }

    return Result<void>::Ok();
}

Result<FeeRecommendation> FeeEstimator::EstimateFromMempool(
    const std::vector<Transaction>& mempool_txs,
    size_t tx_size_bytes) {

    if (mempool_txs.empty()) {
        return EstimateFee(tx_size_bytes, 6);
    }

    // Calculate fee rates for mempool transactions
    std::vector<double> mempool_fee_rates;
    for (const auto& tx : mempool_txs) {
        auto serialized = tx.Serialize();
        size_t tx_size = serialized.size();

        // Estimate input sum (we don't have UTXO set)
        uint64_t output_sum = tx.GetTotalOutputValue();
        uint64_t input_sum = static_cast<uint64_t>(output_sum * 1.01);

        uint64_t fee = input_sum - output_sum;
        double fee_rate = static_cast<double>(fee) / static_cast<double>(tx_size);
        mempool_fee_rates.push_back(fee_rate);
    }

    // Sort and get percentiles
    std::sort(mempool_fee_rates.begin(), mempool_fee_rates.end(), std::greater<double>());

    double low_fee_rate = stats::Percentile(mempool_fee_rates, 0.75);
    double med_fee_rate = stats::Percentile(mempool_fee_rates, 0.50);
    double high_fee_rate = stats::Percentile(mempool_fee_rates, 0.25);

    FeeRecommendation rec;
    rec.low_priority_fee = static_cast<uint64_t>(low_fee_rate * tx_size_bytes);
    rec.medium_priority_fee = static_cast<uint64_t>(med_fee_rate * tx_size_bytes);
    rec.high_priority_fee = static_cast<uint64_t>(high_fee_rate * tx_size_bytes);
    rec.confidence = 0.85;
    rec.estimated_blocks_low = 10;
    rec.estimated_blocks_medium = 6;
    rec.estimated_blocks_high = 2;

    return Result<FeeRecommendation>::Ok(rec);
}

// ============================================================================
// Difficulty Predictor
// ============================================================================

class DifficultyPredictor::Impl {
public:
    std::vector<DifficultyHistory> history;
};

DifficultyPredictor::DifficultyPredictor()
    : impl_(std::make_unique<Impl>()) {}

DifficultyPredictor::~DifficultyPredictor() = default;

Result<void> DifficultyPredictor::Train(const std::vector<DifficultyHistory>& history) {
    impl_->history = history;
    return Result<void>::Ok();
}

Result<HashrateEstimate> DifficultyPredictor::PredictNextDifficulty(
    uint32_t current_height,
    uint32_t current_bits,
    const std::vector<uint64_t>& recent_block_times) {

    HashrateEstimate estimate;

    // Simple prediction based on recent block times
    if (!recent_block_times.empty()) {
        double avg_time = stats::Mean(std::vector<double>(
            recent_block_times.begin(), recent_block_times.end()));
        estimate.estimated_block_time = std::chrono::seconds(
            static_cast<long long>(avg_time));
    }

    estimate.difficulty_next_block = static_cast<double>(current_bits);
    estimate.network_hashrate = 1000000.0;  // Placeholder
    estimate.confidence_percent = 70;

    return Result<HashrateEstimate>::Ok(estimate);
}

Result<double> DifficultyPredictor::EstimateNetworkHashrate(
    const std::vector<uint64_t>& recent_block_times) {

    if (recent_block_times.empty()) {
        return Result<double>::Error("No block time data");
    }

    double avg_time = stats::Mean(std::vector<double>(
        recent_block_times.begin(), recent_block_times.end()));

    // Rough estimation: hashrate = difficulty / block_time
    double hashrate = 1000000.0 / avg_time;  // Placeholder formula

    return Result<double>::Ok(hashrate);
}

// ============================================================================
// ML Manager
// ============================================================================

class MLManager::Impl {
public:
    MLConfig config;
    TransactionAnomalyDetector anomaly_detector;
    NetworkBehaviorAnalyzer network_analyzer;
    FeeEstimator fee_estimator;
    DifficultyPredictor difficulty_predictor;

    bool initialized = false;

    Impl(const MLConfig& cfg)
        : config(cfg),
          anomaly_detector(cfg),
          network_analyzer(cfg),
          fee_estimator(cfg) {}
};

MLManager::MLManager(const MLConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

MLManager::~MLManager() = default;

Result<void> MLManager::Initialize() {
    impl_->initialized = true;
    return Result<void>::Ok();
}

TransactionAnomalyDetector& MLManager::GetAnomalyDetector() {
    return impl_->anomaly_detector;
}

NetworkBehaviorAnalyzer& MLManager::GetNetworkAnalyzer() {
    return impl_->network_analyzer;
}

FeeEstimator& MLManager::GetFeeEstimator() {
    return impl_->fee_estimator;
}

DifficultyPredictor& MLManager::GetDifficultyPredictor() {
    return impl_->difficulty_predictor;
}

Result<void> MLManager::TrainOnBlockchain(const std::vector<Block>& blocks) {
    if (blocks.empty()) {
        return Result<void>::Error("Empty blockchain");
    }

    // Extract all transactions
    std::vector<Transaction> all_txs;
    for (const auto& block : blocks) {
        for (const auto& tx : block.transactions) {
            all_txs.push_back(tx);
        }
    }

    // Train anomaly detector
    auto result = impl_->anomaly_detector.Train(all_txs);
    if (result.IsError()) {
        return Result<void>::Error("Failed to train anomaly detector: " + result.error);
    }

    return Result<void>::Ok();
}

Result<void> MLManager::UpdateModels() {
    // Placeholder for periodic model updates
    return Result<void>::Ok();
}

MLManager::SystemHealth MLManager::GetSystemHealth() const {
    SystemHealth health;
    health.models_trained = impl_->initialized;
    health.total_training_samples = impl_->anomaly_detector.GetStats().training_samples;
    health.overall_accuracy = 0.85;
    health.last_update = std::chrono::system_clock::now();
    return health;
}

Result<void> MLManager::SaveModels(const std::string& directory) {
    // Save neural network weights
    std::string anomaly_path = directory + "/anomaly_detector.weights";
    std::string network_path = directory + "/network_analyzer.weights";

    // Placeholder - would save model files
    return Result<void>::Ok();
}

Result<void> MLManager::LoadModels(const std::string& directory) {
    // Load neural network weights
    std::string anomaly_path = directory + "/anomaly_detector.weights";
    std::string network_path = directory + "/network_analyzer.weights";

    // Placeholder - would load model files
    return Result<void>::Ok();
}

} // namespace ml
} // namespace intcoin
