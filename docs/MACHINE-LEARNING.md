# INTcoin Machine Learning Integration

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

## Overview

INTcoin integrates **Machine Learning** capabilities to provide intelligent network optimization, predictive analytics, and automated anomaly detection. These ML features enhance user experience, improve network efficiency, and strengthen security.

---

## Why Machine Learning in Cryptocurrency?

Traditional cryptocurrency nodes make decisions using fixed algorithms and heuristics. Machine Learning enables:

✅ **Dynamic Fee Optimization** - Predict optimal transaction fees based on real-time network conditions
✅ **Anomaly Detection** - Identify unusual patterns indicating attacks or bugs
✅ **Network Optimization** - Intelligently manage peer connections and bandwidth
✅ **Lightning Route Optimization** - Find the best payment routes using historical success data
✅ **Smart Mempool Management** - Prioritize transactions based on multiple factors
✅ **Difficulty Prediction** - Forecast mining difficulty adjustments

---

## ML Features

### 1. Transaction Fee Prediction

**Purpose**: Help users pay optimal fees for their transactions.

**How It Works**:
- Analyzes historical transaction data (fees, sizes, confirmation times)
- Monitors real-time mempool congestion
- Uses exponential moving averages for smoothing
- Provides three fee tiers: low (budget), medium (standard), high (priority)

**API Usage**:
```cpp
#include "intcoin/ml.h"

using namespace intcoin::ml;

FeePredictionModel fee_predictor;

// Train on historical data
fee_predictor.train_on_historical_data(historical_txs, confirmation_times);

// Predict fee for new transaction
auto prediction = fee_predictor.predict_fee(
    250,  // Transaction size in bytes
    6     // Target confirmation blocks
);

std::cout << "Recommended fee: " << prediction.recommended_fee << " satoshis" << std::endl;
std::cout << "Estimated time: " << prediction.estimated_seconds << " seconds" << std::endl;
std::cout << "Confidence: " << (prediction.confidence_score * 100) << "%" << std::endl;
```

**Output Example**:
```
Recommended fee: 5000 satoshis (20 sat/byte)
Low fee: 3750 satoshis (15 sat/byte)
High fee: 7500 satoshis (30 sat/byte)
Estimated blocks: 6
Estimated time: 720 seconds (12 minutes)
Confidence: 87.3%
```

**Benefits**:
- Users save money by avoiding overpayment
- Transactions confirm faster with optimal fees
- Reduces mempool congestion from incorrect fees

---

### 2. Anomaly Detection

**Purpose**: Detect unusual patterns that may indicate attacks, bugs, or malicious activity.

**Detection Types**:
- **Transaction Anomalies**: Unusual amounts, fees, or patterns
- **Mempool Spam**: Coordinated flooding attacks
- **Double-Spend Attempts**: Conflicting transactions
- **Peer Misbehavior**: Malicious or buggy peers
- **Mining Anomalies**: Unexpected difficulty changes
- **Fork Detection**: Chain splits or deep reorgs
- **Fee Spikes**: Manipulation attempts
- **Network Partitions**: Connectivity issues

**API Usage**:
```cpp
AnomalyDetector detector;
detector.train_on_historical_data(blockchain);
detector.set_sensitivity(0.8);  // 0.0-1.0, higher = more sensitive

// Check individual transaction
auto anomaly = detector.check_transaction(tx);
if (anomaly) {
    std::cout << "Anomaly detected: " << anomaly->description << std::endl;
    std::cout << "Severity: " << (anomaly->severity * 100) << "%" << std::endl;
    std::cout << "Type: " << static_cast<int>(anomaly->type) << std::endl;
}

// Batch analysis
auto anomalies = detector.analyze_recent_activity(100);  // Last 100 blocks
for (const auto& anomaly : anomalies) {
    if (anomaly.severity > 0.7) {
        alert_administrator(anomaly);
    }
}
```

**Output Example**:
```
Anomaly detected: Transaction fee 10x higher than average
Severity: 65%
Type: UNUSUAL_FEE_SPIKE

Anomaly detected: Same inputs seen in mempool 3 times
Severity: 85%
Type: POTENTIAL_DOUBLE_SPEND
```

**Benefits**:
- Early detection of attacks (DOS, spam, double-spend)
- Identify bugs before they cause damage
- Network health monitoring
- Automated incident response

---

### 3. Network Traffic Prediction

**Purpose**: Forecast network congestion and optimize connection strategies.

**Predictions**:
- Peer count trends
- Bandwidth usage (upload/download)
- Network health score
- Congestion warnings

**API Usage**:
```cpp
NetworkTrafficPredictor predictor;

// Record network snapshots every minute
predictor.record_network_event(
    current_timestamp,
    peer_count,
    bytes_in,
    bytes_out
);

// Predict 30 minutes ahead
auto forecast = predictor.predict_traffic(30);

std::cout << "Predicted peers: " << forecast.predicted_peer_count << std::endl;
std::cout << "Predicted bandwidth in: " << forecast.predicted_bandwidth_in << " bytes/s" << std::endl;
std::cout << "Network health: " << (forecast.network_health_score * 100) << "%" << std::endl;

if (forecast.congestion_warning) {
    std::cout << "Warning: Network congestion expected" << std::endl;

    // Get recommendations
    auto suggestions = predictor.get_optimization_suggestions();
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion << std::endl;
    }
}

// Automatic optimization
uint32_t recommended_connections = predictor.recommend_connection_limit();
if (predictor.should_throttle_connections()) {
    reduce_connection_rate();
}
```

**Output Example**:
```
Predicted peers: 72 (current: 68)
Predicted bandwidth in: 125000 bytes/s
Predicted bandwidth out: 98000 bytes/s
Network health: 92%
Warning: Network congestion expected
  - Reduce outgoing connection rate by 20%
  - Defer large block downloads by 5 minutes
  - Increase mempool eviction threshold
```

**Benefits**:
- Proactive resource management
- Avoid network overload
- Better user experience during peak times
- Reduced bandwidth waste

---

### 4. Lightning Route Optimization

**Purpose**: Find the most reliable and cost-effective Lightning payment routes.

**Scoring Factors**:
- Historical success rate of nodes
- Fee competitiveness
- Response time
- Liquidity availability
- Multi-path reliability

**API Usage**:
```cpp
LightningRouteOptimizer optimizer;

// Find optimal routes
auto routes = optimizer.find_optimal_routes(
    "source_node_id",
    "destination_node_id",
    100000,  // Amount in satoshis
    5        // Max routes to return
);

for (const auto& route : routes) {
    std::cout << "Route: ";
    for (const auto& node : route.node_path) {
        std::cout << node << " -> ";
    }
    std::cout << "destination" << std::endl;

    std::cout << "  Success probability: " << (route.success_probability * 100) << "%" << std::endl;
    std::cout << "  Total fees: " << route.total_fees << " satoshis" << std::endl;
    std::cout << "  Reliability score: " << (route.reliability_score * 100) << "%" << std::endl;
    std::cout << "  Overall score: " << route.overall_score << std::endl;
}

// Record payment result to improve future predictions
optimizer.record_payment_result(
    best_route.node_path,
    true,      // Success
    actual_fees,
    response_time_ms
);
```

**Output Example**:
```
Route 1: A -> B -> C -> D -> destination
  Success probability: 94.2%
  Total fees: 450 satoshis
  Reliability score: 91%
  Overall score: 0.93

Route 2: A -> E -> F -> destination
  Success probability: 89.7%
  Total fees: 380 satoshis
  Reliability score: 88%
  Overall score: 0.89
```

**Benefits**:
- Higher payment success rates
- Lower fees through intelligent routing
- Avoid unreliable nodes
- Faster payment completion

---

### 5. Smart Mempool Management

**Purpose**: Intelligently prioritize and evict transactions from the mempool.

**Features**:
- Transaction spam detection
- Priority scoring based on multiple factors
- Optimal eviction strategy when mempool is full
- Learning from mined transaction patterns

**API Usage**:
```cpp
SmartMempoolManager mempool_mgr;
mempool_mgr.set_mempool_size_target(300 * 1024 * 1024);  // 300 MB

// Score transaction
auto score = mempool_mgr.score_transaction(tx);

std::cout << "Priority score: " << score.priority_score << std::endl;
std::cout << "Fee per byte: " << score.fee_per_byte << std::endl;
std::cout << "Time in mempool: " << score.time_in_mempool << " seconds" << std::endl;
std::cout << "Spam likelihood: " << (score.spam_likelihood * 100) << "%" << std::endl;
std::cout << "Should keep: " << (score.should_keep ? "YES" : "NO") << std::endl;
std::cout << "Should relay: " << (score.should_relay ? "YES" : "NO") << std::endl;

// Evict transactions when mempool is full
if (mempool_size > target_size) {
    auto txs_to_evict = mempool_mgr.select_transactions_to_evict(100);
    for (const auto& tx_hash : txs_to_evict) {
        remove_from_mempool(tx_hash);
    }
}

// Learn from outcomes
mempool_mgr.record_transaction_outcome(tx_hash, was_mined, blocks_waited);
```

**Benefits**:
- Efficient mempool space utilization
- Better transaction propagation
- Reduced spam in mempool
- Improved mining profitability

---

### 6. Difficulty Prediction

**Purpose**: Forecast future mining difficulty adjustments.

**Uses**:
- Miner profitability planning
- Network hashrate estimation
- Long-term trend analysis

**API Usage**:
```cpp
DifficultyPredictor diff_predictor;

// Record historical difficulty changes
diff_predictor.record_difficulty_change(height, difficulty, timestamp);

// Predict next adjustment
auto forecast = diff_predictor.predict_next_difficulty();

std::cout << "Current hashrate: " << diff_predictor.get_current_hashrate_estimate() << " H/s" << std::endl;
std::cout << "Hashrate trend: " << diff_predictor.get_hashrate_trend() << "%" << std::endl;
std::cout << "Next difficulty: " << forecast.predicted_difficulty.to_string() << std::endl;
std::cout << "Percent change: " << forecast.percent_change << "%" << std::endl;
std::cout << "Blocks until adjustment: " << forecast.blocks_until_adjustment << std::endl;
std::cout << "Confidence: " << (forecast.confidence * 100) << "%" << std::endl;

// Long-term forecast
auto long_term = diff_predictor.predict_difficulty_trend(10);  // Next 10 adjustments
for (size_t i = 0; i < long_term.size(); i++) {
    std::cout << "Adjustment " << (i + 1) << ": " << long_term[i].percent_change << "%" << std::endl;
}
```

**Output Example**:
```
Current hashrate: 1.234 PH/s
Hashrate trend: +15.7%
Next difficulty: 0x1234567890abcdef
Percent change: +12.3%
Blocks until adjustment: 142
Confidence: 83%
```

---

### 7. Peer Quality Scoring

**Purpose**: Evaluate peer reliability and maintain optimal connections.

**Scoring Factors**:
- Response time
- Uptime ratio
- Message validity
- Bandwidth efficiency
- Historical reliability

**API Usage**:
```cpp
PeerQualityScorer peer_scorer;

// Record peer activity
peer_scorer.record_peer_message(peer_id, valid, message_size);
peer_scorer.record_peer_response_time(peer_id, response_ms);

// Score peer
auto score = peer_scorer.score_peer(peer_id);

std::cout << "Overall quality: " << (score.overall_quality * 100) << "%" << std::endl;
std::cout << "Response time: " << score.response_time << " ms" << std::endl;
std::cout << "Uptime ratio: " << (score.uptime_ratio * 100) << "%" << std::endl;
std::cout << "Data quality: " << (score.data_quality * 100) << "%" << std::endl;
std::cout << "Bandwidth efficiency: " << (score.bandwidth_efficiency * 100) << "%" << std::endl;

if (!score.should_maintain_connection) {
    std::cout << "Recommendation: DISCONNECT" << std::endl;
    disconnect_peer(peer_id);
}

// Get best peers for critical operations
auto best_peers = peer_scorer.get_best_peers(10);
for (const auto& peer : best_peers) {
    std::cout << "Best peer: " << peer << std::endl;
}

// Get worst peers for cleanup
auto worst_peers = peer_scorer.get_worst_peers(5);
for (const auto& peer : worst_peers) {
    disconnect_peer(peer);
}
```

---

## Configuration

### Enable/Disable ML Features

Edit `intcoin.conf`:

```ini
[ml]
# Enable Machine Learning
enabled=1

# Individual feature toggles
enable_fee_prediction=1
enable_traffic_prediction=1
enable_anomaly_detection=1
enable_route_optimization=1
enable_smart_mempool=1
enable_peer_scoring=1

# Anomaly detection sensitivity (0.0-1.0, higher = more sensitive)
anomaly_sensitivity=0.7

# Training interval (blocks between model retraining)
training_interval_blocks=2016  # ~2 weeks

# Model storage directory
model_storage_path=data/ml_models/
```

### Performance vs Accuracy Trade-off

**High Performance** (lower CPU usage):
```ini
enable_fee_prediction=1
enable_anomaly_detection=1
enable_peer_scoring=1
enable_traffic_prediction=0
enable_route_optimization=0
enable_smart_mempool=0
training_interval_blocks=4032  # Monthly
```

**Maximum Accuracy** (higher CPU usage):
```ini
enable_fee_prediction=1
enable_traffic_prediction=1
enable_anomaly_detection=1
enable_route_optimization=1
enable_smart_mempool=1
enable_peer_scoring=1
anomaly_sensitivity=0.9
training_interval_blocks=1008  # Weekly
```

---

## Training and Model Persistence

### Initial Training

```bash
# Train all ML models on historical blockchain data
./intcoin-cli ml-train-all

# Train specific model
./intcoin-cli ml-train-fee-predictor
./intcoin-cli ml-train-anomaly-detector
```

### Save/Load Models

```cpp
MLModelManager& manager = MLModelManager::instance();

// Save all models to disk
manager.save_all_models("data/ml_models/");

// Load models from disk (on startup)
manager.load_all_models("data/ml_models/");

// Access individual models
auto& fee_pred = manager.fee_predictor();
auto& anomaly_det = manager.anomaly_detector();
```

### Model Files

```
data/ml_models/
├── fee_predictor.model
├── traffic_predictor.model
├── anomaly_detector.model
├── difficulty_predictor.model
├── route_optimizer.model
├── mempool_manager.model
└── peer_scorer.model
```

---

## Statistics and Monitoring

### Get ML Statistics

```bash
# Overall ML statistics
./intcoin-cli ml-stats

# Individual model statistics
./intcoin-cli ml-fee-stats
./intcoin-cli ml-anomaly-stats
./intcoin-cli ml-peer-stats
```

**Output Example**:
```json
{
  "models_loaded": 7,
  "total_predictions": 15234,
  "training_samples": 98765,
  "avg_model_accuracy": 0.873,
  "fee_predictor": {
    "predictions": 5421,
    "accuracy": 0.891,
    "last_trained": 1700000000
  },
  "anomaly_detector": {
    "anomalies_detected": 12,
    "false_positives": 1,
    "sensitivity": 0.7
  }
}
```

### Performance Metrics

```cpp
auto stats = MLModelManager::instance().get_statistics();

std::cout << "Models loaded: " << stats.models_loaded << std::endl;
std::cout << "Total predictions: " << stats.total_predictions << std::endl;
std::cout << "Training samples: " << stats.training_samples << std::endl;
std::cout << "Average accuracy: " << (stats.avg_model_accuracy * 100) << "%" << std::endl;
```

---

## Building with ML Support

### Compilation

```bash
cd build
cmake .. -DENABLE_ML=ON
make -j$(nproc)
```

### Dependencies

ML features use standard C++ and minimal external dependencies:
- **STL**: `<vector>`, `<map>`, `<algorithm>`
- **Statistics**: Simple linear regression, exponential moving averages
- **No heavy ML frameworks**: Lightweight, efficient implementations

---

## Algorithm Details

### Fee Prediction Algorithm

**Method**: Linear regression + Exponential Moving Average (EMA)

```
base_fee_rate = EMA(historical_fee_per_byte)
congestion_multiplier = mempool_size / max_mempool_size
target_multiplier = {1, 3, 6} blocks -> {1.5, 1.0, 0.75}

recommended_fee = tx_size * base_fee_rate * congestion_multiplier * target_multiplier
```

### Anomaly Detection Algorithm

**Method**: Z-score (standard deviations from mean)

```
z_score = (value - mean) / std_deviation

if abs(z_score) > threshold:
    anomaly_detected = true
    severity = min(abs(z_score) / 10, 1.0)
```

**Thresholds**:
- Low sensitivity (0.3): `threshold = 4.0`
- Medium sensitivity (0.7): `threshold = 2.5`
- High sensitivity (0.9): `threshold = 1.5`

### Route Optimization Algorithm

**Method**: Weighted scoring

```
success_prob = product(node_reliability for node in route)
reliability_score = average(node_reliability for node in route)

overall_score = (
    0.5 * success_prob +
    0.3 * (1 - normalized_fees) +
    0.2 * reliability_score
)
```

---

## Privacy Considerations

### Data Collection

ML models collect and store:
- ✅ Transaction sizes, fees, confirmation times (public blockchain data)
- ✅ Network statistics (peer count, bandwidth)
- ✅ Mempool state (size, fee distribution)
- ❌ **NO** wallet addresses
- ❌ **NO** IP addresses
- ❌ **NO** personal information

### Model Sharing

Models can be shared between nodes to improve accuracy:
- Models contain **only statistical parameters** (means, standard deviations)
- **No transaction-specific data** is stored in models
- Safe to share publicly

### Opt-Out

Disable all ML features:
```ini
[ml]
enabled=0
```

---

## Future Enhancements

### Planned Features

- **Deep Learning Models**: Neural networks for complex pattern recognition
- **Federated Learning**: Train models across multiple nodes without sharing data
- **Predictive Mining**: Forecast which transactions will be mined next
- **Smart Contract Analysis**: ML-based smart contract vulnerability detection
- **Cross-Chain ML**: Shared learning from multiple blockchains

---

## FAQ

**Q: Does ML increase CPU usage?**
A: Minimal impact. Models use simple algorithms (linear regression, moving averages) that are very lightweight.

**Q: Do I need GPU for ML features?**
A: No, all ML features run efficiently on CPU.

**Q: How much training data is needed?**
A: Models improve over time. Minimum ~1000 transactions for fee prediction, ~100 blocks for anomaly detection.

**Q: Can ML features be disabled?**
A: Yes, set `ml.enabled=0` in `intcoin.conf` or use `./intcoind -noml`.

**Q: Are models stored in RAM?**
A: Yes, but models are small (<10 MB total). They're loaded on startup and saved periodically.

**Q: How often are models retrained?**
A: Default is every 2016 blocks (~2 weeks). Configurable with `training_interval_blocks`.

---

## Resources

- **ML Header**: [include/intcoin/ml.h](../include/intcoin/ml.h)
- **Configuration**: `intcoin.conf` `[ml]` section
- **Statistics**: `./intcoin-cli ml-stats`

---

**Last Updated**: November 20, 2025
**Version**: 1.0
**Status**: Production Ready ✅
