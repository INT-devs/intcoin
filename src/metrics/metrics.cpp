// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/metrics.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace intcoin {

// ============================================================================
// Counter Implementation
// ============================================================================

Counter::Counter(const std::string& name, const std::string& help)
    : name_(name), help_(help), value_(0.0) {}

void Counter::Inc() {
    value_.fetch_add(1.0);
}

void Counter::Add(double value) {
    if (value < 0) {
        return;  // Counters can only increase
    }
    value_.fetch_add(value);
}

double Counter::Value() const {
    return value_.load();
}

std::string Counter::ExportPrometheus() const {
    std::ostringstream oss;
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " counter\n";
    oss << name_ << " " << std::fixed << std::setprecision(2) << Value() << "\n";
    return oss.str();
}

// ============================================================================
// Gauge Implementation
// ============================================================================

Gauge::Gauge(const std::string& name, const std::string& help)
    : name_(name), help_(help), value_(0.0) {}

void Gauge::Set(double value) {
    value_.store(value);
}

void Gauge::Inc() {
    value_.fetch_add(1.0);
}

void Gauge::Dec() {
    value_.fetch_sub(1.0);
}

void Gauge::Add(double value) {
    value_.fetch_add(value);
}

void Gauge::Sub(double value) {
    value_.fetch_sub(value);
}

double Gauge::Value() const {
    return value_.load();
}

std::string Gauge::ExportPrometheus() const {
    std::ostringstream oss;
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " gauge\n";
    oss << name_ << " " << std::fixed << std::setprecision(2) << Value() << "\n";
    return oss.str();
}

// ============================================================================
// Histogram Implementation
// ============================================================================

Histogram::Histogram(const std::string& name, const std::string& help,
                     const std::vector<double>& buckets)
    : name_(name), help_(help), buckets_(buckets), count_(0), sum_(0.0) {

    // Sort buckets
    std::sort(buckets_.begin(), buckets_.end());

    // Initialize bucket counts (including +Inf bucket)
    bucket_counts_.resize(buckets_.size() + 1, 0);
}

void Histogram::Observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);

    count_++;
    sum_ += value;

    // Find bucket and increment
    for (size_t i = 0; i < buckets_.size(); ++i) {
        if (value <= buckets_[i]) {
            bucket_counts_[i]++;
        }
    }

    // +Inf bucket always gets incremented
    bucket_counts_[buckets_.size()]++;
}

uint64_t Histogram::Count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
}

double Histogram::Sum() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sum_;
}

std::string Histogram::ExportPrometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " histogram\n";

    // Export buckets
    for (size_t i = 0; i < buckets_.size(); ++i) {
        oss << name_ << "_bucket{le=\"" << std::fixed << std::setprecision(2)
            << buckets_[i] << "\"} " << bucket_counts_[i] << "\n";
    }

    // +Inf bucket
    oss << name_ << "_bucket{le=\"+Inf\"} " << bucket_counts_[buckets_.size()] << "\n";

    // Sum and count
    oss << name_ << "_sum " << std::fixed << std::setprecision(2) << sum_ << "\n";
    oss << name_ << "_count " << count_ << "\n";

    return oss.str();
}

// ============================================================================
// Timer Implementation
// ============================================================================

Timer::Timer(Histogram& histogram)
    : histogram_(histogram), start_(std::chrono::steady_clock::now()), stopped_(false) {}

Timer::~Timer() {
    if (!stopped_) {
        Stop();
    }
}

void Timer::Stop() {
    if (stopped_) {
        return;
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    histogram_.Observe(duration.count());
    stopped_ = true;
}

// ============================================================================
// MetricsRegistry Implementation
// ============================================================================

MetricsRegistry& MetricsRegistry::Instance() {
    static MetricsRegistry instance;
    return instance;
}

Counter& MetricsRegistry::RegisterCounter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return *it->second;
    }

    auto counter = std::make_unique<Counter>(name, help);
    Counter& ref = *counter;
    counters_[name] = std::move(counter);
    return ref;
}

Gauge& MetricsRegistry::RegisterGauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return *it->second;
    }

    auto gauge = std::make_unique<Gauge>(name, help);
    Gauge& ref = *gauge;
    gauges_[name] = std::move(gauge);
    return ref;
}

Histogram& MetricsRegistry::RegisterHistogram(const std::string& name, const std::string& help,
                                              const std::vector<double>& buckets) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = histograms_.find(name);
    if (it != histograms_.end()) {
        return *it->second;
    }

    auto histogram = std::make_unique<Histogram>(name, help, buckets);
    Histogram& ref = *histogram;
    histograms_[name] = std::move(histogram);
    return ref;
}

Counter* MetricsRegistry::GetCounter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    return it != counters_.end() ? it->second.get() : nullptr;
}

Gauge* MetricsRegistry::GetGauge(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    return it != gauges_.end() ? it->second.get() : nullptr;
}

Histogram* MetricsRegistry::GetHistogram(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = histograms_.find(name);
    return it != histograms_.end() ? it->second.get() : nullptr;
}

std::string MetricsRegistry::ExportPrometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;

    // Export all counters
    for (const auto& pair : counters_) {
        oss << pair.second->ExportPrometheus();
    }

    // Export all gauges
    for (const auto& pair : gauges_) {
        oss << pair.second->ExportPrometheus();
    }

    // Export all histograms
    for (const auto& pair : histograms_) {
        oss << pair.second->ExportPrometheus();
    }

    return oss.str();
}

void MetricsRegistry::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

// ============================================================================
// Standard Metrics Initialization
// ============================================================================

namespace metrics {

// Define standard buckets for different metric types
static const std::vector<double> SIZE_BUCKETS = {
    100, 500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000
};

static const std::vector<double> DURATION_BUCKETS = {
    1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000
};

static const std::vector<double> FEE_BUCKETS = {
    100, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000
};

// Blockchain metrics
Counter& blocks_processed = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_blocks_processed_total",
    "Total number of blocks processed"
);

Counter& transactions_processed = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_transactions_processed_total",
    "Total number of transactions processed"
);

Gauge& blockchain_height = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_blockchain_height",
    "Current blockchain height"
);

Gauge& blockchain_difficulty = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_blockchain_difficulty",
    "Current mining difficulty"
);

Histogram& block_processing_duration = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_block_processing_duration_ms",
    "Block processing duration in milliseconds",
    DURATION_BUCKETS
);

Histogram& block_size = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_block_size_bytes",
    "Block size in bytes",
    SIZE_BUCKETS
);

// Mempool metrics
Gauge& mempool_size = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_mempool_size",
    "Current number of transactions in mempool"
);

Gauge& mempool_bytes = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_mempool_bytes",
    "Current mempool size in bytes"
);

Counter& mempool_accepted = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_mempool_accepted_total",
    "Total number of transactions accepted to mempool"
);

Counter& mempool_rejected = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_mempool_rejected_total",
    "Total number of transactions rejected from mempool"
);

Histogram& mempool_tx_fee = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_mempool_tx_fee_satoshis",
    "Transaction fees in mempool (satoshis)",
    FEE_BUCKETS
);

// Network metrics
Gauge& peer_count = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_peer_count",
    "Current number of connected peers"
);

Counter& bytes_sent = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_bytes_sent_total",
    "Total bytes sent to network"
);

Counter& bytes_received = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_bytes_received_total",
    "Total bytes received from network"
);

Counter& messages_sent = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_messages_sent_total",
    "Total messages sent to network"
);

Counter& messages_received = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_messages_received_total",
    "Total messages received from network"
);

Histogram& message_processing_duration = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_message_processing_duration_ms",
    "Message processing duration in milliseconds",
    DURATION_BUCKETS
);

// Mining metrics
Counter& blocks_mined = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_blocks_mined_total",
    "Total number of blocks mined"
);

Counter& hashes_computed = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_hashes_computed_total",
    "Total number of hashes computed"
);

Gauge& hashrate = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_hashrate",
    "Current hashrate (hashes per second)"
);

Histogram& mining_duration = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_mining_duration_ms",
    "Mining duration per block in milliseconds",
    {100, 500, 1000, 5000, 10000, 30000, 60000, 120000, 300000, 600000}
);

// Wallet metrics
Gauge& wallet_balance = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_wallet_balance_ints",
    "Current wallet balance in INTS"
);

Counter& wallet_transactions = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_wallet_transactions_total",
    "Total number of wallet transactions"
);

Gauge& wallet_utxo_count = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_wallet_utxo_count",
    "Current number of UTXOs in wallet"
);

// P2P metrics
Gauge& spv_best_height = MetricsRegistry::Instance().RegisterGauge(
    "intcoin_spv_best_height",
    "SPV client best header height"
);

Counter& bloom_filters_loaded = MetricsRegistry::Instance().RegisterCounter(
    "intcoin_bloom_filters_loaded_total",
    "Total number of bloom filters loaded"
);

Histogram& header_sync_duration = MetricsRegistry::Instance().RegisterHistogram(
    "intcoin_header_sync_duration_ms",
    "Header sync duration in milliseconds",
    DURATION_BUCKETS
);

void InitializeMetrics() {
    // Metrics are initialized on first access via RegisterX() calls above
    // This function exists for explicit initialization if needed
}

} // namespace metrics

} // namespace intcoin
