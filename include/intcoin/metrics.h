// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_METRICS_H
#define INTCOIN_METRICS_H

#include <intcoin/types.h>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace intcoin {

/// Metric types (Prometheus compatible)
enum class MetricType {
    COUNTER,    // Monotonically increasing counter
    GAUGE,      // Value that can go up or down
    HISTOGRAM,  // Distribution of values (with buckets)
    SUMMARY     // Distribution with quantiles
};

/// Base metric interface
class Metric {
public:
    virtual ~Metric() = default;

    /// Get metric name
    virtual std::string GetName() const = 0;

    /// Get metric type
    virtual MetricType GetType() const = 0;

    /// Get metric help text
    virtual std::string GetHelp() const = 0;

    /// Export metric in Prometheus format
    virtual std::string ExportPrometheus() const = 0;
};

/// Counter metric - monotonically increasing value
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help);

    /// Increment counter by 1
    void Inc();

    /// Increment counter by n
    void Add(double value);

    /// Get current value
    double Value() const;

    std::string GetName() const override { return name_; }
    MetricType GetType() const override { return MetricType::COUNTER; }
    std::string GetHelp() const override { return help_; }
    std::string ExportPrometheus() const override;

private:
    std::string name_;
    std::string help_;
    std::atomic<double> value_;
};

/// Gauge metric - value that can go up or down
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help);

    /// Set gauge value
    void Set(double value);

    /// Increment gauge
    void Inc();

    /// Decrement gauge
    void Dec();

    /// Add to gauge
    void Add(double value);

    /// Subtract from gauge
    void Sub(double value);

    /// Get current value
    double Value() const;

    std::string GetName() const override { return name_; }
    MetricType GetType() const override { return MetricType::GAUGE; }
    std::string GetHelp() const override { return help_; }
    std::string ExportPrometheus() const override;

private:
    std::string name_;
    std::string help_;
    std::atomic<double> value_;
};

/// Histogram metric - distribution with buckets
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& help,
             const std::vector<double>& buckets);

    /// Observe a value
    void Observe(double value);

    /// Get observation count
    uint64_t Count() const;

    /// Get sum of observations
    double Sum() const;

    std::string GetName() const override { return name_; }
    MetricType GetType() const override { return MetricType::HISTOGRAM; }
    std::string GetHelp() const override { return help_; }
    std::string ExportPrometheus() const override;

private:
    std::string name_;
    std::string help_;
    std::vector<double> buckets_;
    std::vector<uint64_t> bucket_counts_;  // Protected by mutex
    uint64_t count_;  // Protected by mutex
    double sum_;      // Protected by mutex
    mutable std::mutex mutex_;
};

/// Timer for measuring durations
class Timer {
public:
    Timer(Histogram& histogram);
    ~Timer();

    /// Stop timer manually
    void Stop();

private:
    Histogram& histogram_;
    std::chrono::steady_clock::time_point start_;
    bool stopped_;
};

/// Metrics registry - central collection point for all metrics
class MetricsRegistry {
public:
    static MetricsRegistry& Instance();

    /// Register a counter
    Counter& RegisterCounter(const std::string& name, const std::string& help);

    /// Register a gauge
    Gauge& RegisterGauge(const std::string& name, const std::string& help);

    /// Register a histogram
    Histogram& RegisterHistogram(const std::string& name, const std::string& help,
                                 const std::vector<double>& buckets);

    /// Get counter by name
    Counter* GetCounter(const std::string& name);

    /// Get gauge by name
    Gauge* GetGauge(const std::string& name);

    /// Get histogram by name
    Histogram* GetHistogram(const std::string& name);

    /// Export all metrics in Prometheus format
    std::string ExportPrometheus() const;

    /// Clear all metrics
    void Clear();

private:
    MetricsRegistry() = default;
    ~MetricsRegistry() = default;
    MetricsRegistry(const MetricsRegistry&) = delete;
    MetricsRegistry& operator=(const MetricsRegistry&) = delete;

    std::map<std::string, std::unique_ptr<Counter>> counters_;
    std::map<std::string, std::unique_ptr<Gauge>> gauges_;
    std::map<std::string, std::unique_ptr<Histogram>> histograms_;
    mutable std::mutex mutex_;
};

/// Blockchain metrics
namespace metrics {
    // Blockchain metrics
    extern Counter& blocks_processed;
    extern Counter& transactions_processed;
    extern Gauge& blockchain_height;
    extern Gauge& blockchain_difficulty;
    extern Histogram& block_processing_duration;
    extern Histogram& block_size;

    // Mempool metrics
    extern Gauge& mempool_size;
    extern Gauge& mempool_bytes;
    extern Counter& mempool_accepted;
    extern Counter& mempool_rejected;
    extern Histogram& mempool_tx_fee;

    // Network metrics
    extern Gauge& peer_count;
    extern Counter& bytes_sent;
    extern Counter& bytes_received;
    extern Counter& messages_sent;
    extern Counter& messages_received;
    extern Histogram& message_processing_duration;

    // Mining metrics
    extern Counter& blocks_mined;
    extern Counter& hashes_computed;
    extern Gauge& hashrate;
    extern Histogram& mining_duration;

    // Wallet metrics
    extern Gauge& wallet_balance;
    extern Counter& wallet_transactions;
    extern Gauge& wallet_utxo_count;

    // P2P metrics
    extern Gauge& spv_best_height;
    extern Counter& bloom_filters_loaded;
    extern Histogram& header_sync_duration;

    /// Initialize all metrics
    void InitializeMetrics();
}

} // namespace intcoin

#endif // INTCOIN_METRICS_H
