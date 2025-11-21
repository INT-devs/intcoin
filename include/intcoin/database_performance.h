// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_DATABASE_PERFORMANCE_H
#define INTCOIN_DATABASE_PERFORMANCE_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <optional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace intcoin {
namespace database {

/**
 * B-Tree Index for efficient queries
 */
template<typename K, typename V>
class BTreeIndex {
    std::map<K, V> index_;  // std::map provides O(log n) operations
    mutable std::mutex mtx_;
    std::string name_;

    struct Statistics {
        std::atomic<uint64_t> lookups{0};
        std::atomic<uint64_t> inserts{0};
        std::atomic<uint64_t> deletes{0};
        std::atomic<uint64_t> range_queries{0};
    } stats_;

public:
    explicit BTreeIndex(const std::string& name) : name_(name) {}

    // Insert or update
    void insert(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        index_[key] = value;
        ++stats_.inserts;
    }

    // Lookup
    std::optional<V> find(const K& key) const {
        std::lock_guard<std::mutex> lock(mtx_);
        ++stats_.lookups;
        auto it = index_.find(key);
        if (it != index_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Delete
    bool erase(const K& key) {
        std::lock_guard<std::mutex> lock(mtx_);
        ++stats_.deletes;
        return index_.erase(key) > 0;
    }

    // Range query [start, end)
    std::vector<std::pair<K, V>> range(const K& start, const K& end) const {
        std::lock_guard<std::mutex> lock(mtx_);
        ++stats_.range_queries;
        std::vector<std::pair<K, V>> result;
        auto it_start = index_.lower_bound(start);
        auto it_end = index_.lower_bound(end);
        for (auto it = it_start; it != it_end; ++it) {
            result.push_back(*it);
        }
        return result;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return index_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        index_.clear();
    }

    const std::string& name() const { return name_; }

    uint64_t get_lookups() const { return stats_.lookups.load(); }
    uint64_t get_inserts() const { return stats_.inserts.load(); }
};

/**
 * Hash Index for O(1) point lookups
 */
template<typename K, typename V>
class HashIndex {
    std::unordered_map<K, V> index_;
    mutable std::mutex mtx_;
    std::string name_;
    size_t bucket_count_threshold_ = 1000000;  // Rehash threshold

    struct Statistics {
        std::atomic<uint64_t> lookups{0};
        std::atomic<uint64_t> inserts{0};
        std::atomic<uint64_t> collisions{0};
        std::atomic<uint64_t> rehashes{0};
    } stats_;

public:
    explicit HashIndex(const std::string& name) : name_(name) {}

    void insert(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mtx_);

        // Check load factor before insert
        if (index_.size() > 0 && index_.bucket_count() > 0) {
            float load = static_cast<float>(index_.size()) / index_.bucket_count();
            if (load > 0.75f) {
                index_.rehash(index_.bucket_count() * 2);
                ++stats_.rehashes;
            }
        }

        index_[key] = value;
        ++stats_.inserts;
    }

    std::optional<V> find(const K& key) const {
        std::lock_guard<std::mutex> lock(mtx_);
        ++stats_.lookups;
        auto it = index_.find(key);
        if (it != index_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool erase(const K& key) {
        std::lock_guard<std::mutex> lock(mtx_);
        return index_.erase(key) > 0;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return index_.size();
    }

    float load_factor() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return index_.load_factor();
    }

    const std::string& name() const { return name_; }
    uint64_t get_lookups() const { return stats_.lookups.load(); }
};

/**
 * Performance Degradation Monitor
 */
class DegradationMonitor {
public:
    struct PerformanceMetrics {
        double avg_read_latency_us;
        double avg_write_latency_us;
        double p99_read_latency_us;
        double p99_write_latency_us;
        uint64_t total_reads;
        uint64_t total_writes;
        double read_throughput_ops;  // ops/sec
        double write_throughput_ops;
    };

    struct DegradationAlert {
        bool is_degraded;
        std::string reason;
        double severity;  // 0.0-1.0
    };

private:
    std::vector<double> read_latencies_;
    std::vector<double> write_latencies_;
    mutable std::mutex mtx_;

    double baseline_read_latency_ = 0;
    double baseline_write_latency_ = 0;
    bool baseline_set_ = false;

    size_t max_samples_ = 10000;
    double degradation_threshold_ = 2.0;  // 2x baseline = degraded

public:
    // Record operation latency
    void record_read(double latency_us) {
        std::lock_guard<std::mutex> lock(mtx_);
        read_latencies_.push_back(latency_us);
        if (read_latencies_.size() > max_samples_) {
            read_latencies_.erase(read_latencies_.begin());
        }
    }

    void record_write(double latency_us) {
        std::lock_guard<std::mutex> lock(mtx_);
        write_latencies_.push_back(latency_us);
        if (write_latencies_.size() > max_samples_) {
            write_latencies_.erase(write_latencies_.begin());
        }
    }

    // Set baseline from current metrics
    void set_baseline() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!read_latencies_.empty()) {
            baseline_read_latency_ = calculate_average(read_latencies_);
        }
        if (!write_latencies_.empty()) {
            baseline_write_latency_ = calculate_average(write_latencies_);
        }
        baseline_set_ = true;
    }

    // Get current metrics
    PerformanceMetrics get_metrics() const {
        std::lock_guard<std::mutex> lock(mtx_);
        PerformanceMetrics metrics = {};

        if (!read_latencies_.empty()) {
            metrics.avg_read_latency_us = calculate_average(read_latencies_);
            metrics.p99_read_latency_us = calculate_percentile(read_latencies_, 99);
            metrics.total_reads = read_latencies_.size();
        }

        if (!write_latencies_.empty()) {
            metrics.avg_write_latency_us = calculate_average(write_latencies_);
            metrics.p99_write_latency_us = calculate_percentile(write_latencies_, 99);
            metrics.total_writes = write_latencies_.size();
        }

        return metrics;
    }

    // Check for degradation
    DegradationAlert check_degradation() const {
        std::lock_guard<std::mutex> lock(mtx_);
        DegradationAlert alert = {false, "", 0.0};

        if (!baseline_set_ || read_latencies_.empty()) {
            return alert;
        }

        double current_read = calculate_average(read_latencies_);
        double current_write = write_latencies_.empty() ? 0 : calculate_average(write_latencies_);

        double read_ratio = baseline_read_latency_ > 0 ? current_read / baseline_read_latency_ : 1.0;
        double write_ratio = baseline_write_latency_ > 0 ? current_write / baseline_write_latency_ : 1.0;

        if (read_ratio > degradation_threshold_) {
            alert.is_degraded = true;
            alert.reason = "Read latency " + std::to_string(read_ratio) + "x baseline";
            alert.severity = std::min(1.0, (read_ratio - 1.0) / 4.0);
        }

        if (write_ratio > degradation_threshold_) {
            alert.is_degraded = true;
            alert.reason += (alert.reason.empty() ? "" : "; ") +
                           std::string("Write latency ") + std::to_string(write_ratio) + "x baseline";
            alert.severity = std::max(alert.severity, std::min(1.0, (write_ratio - 1.0) / 4.0));
        }

        return alert;
    }

private:
    static double calculate_average(const std::vector<double>& values) {
        if (values.empty()) return 0;
        double sum = 0;
        for (double v : values) sum += v;
        return sum / values.size();
    }

    static double calculate_percentile(const std::vector<double>& values, int percentile) {
        if (values.empty()) return 0;
        std::vector<double> sorted = values;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = (sorted.size() * percentile) / 100;
        return sorted[std::min(idx, sorted.size() - 1)];
    }
};

/**
 * Database Compactor
 */
class Compactor {
public:
    struct CompactionResult {
        bool success;
        std::string error;
        uint64_t entries_before;
        uint64_t entries_after;
        uint64_t bytes_reclaimed;
        uint64_t duration_ms;
    };

    struct CompactionStats {
        uint64_t compactions_run = 0;
        uint64_t total_bytes_reclaimed = 0;
        uint64_t total_entries_removed = 0;
    };

private:
    CompactionStats stats_;
    mutable std::mutex mtx_;

    double fragmentation_threshold_ = 0.3;  // Compact when 30% fragmented
    size_t min_entries_for_compaction_ = 1000;

public:
    // Check if compaction is needed
    bool needs_compaction(size_t total_entries, size_t deleted_entries) const {
        if (total_entries < min_entries_for_compaction_) return false;
        double fragmentation = static_cast<double>(deleted_entries) / total_entries;
        return fragmentation > fragmentation_threshold_;
    }

    // Run compaction (removes deleted entries, reorganizes data)
    template<typename Entry>
    CompactionResult compact(
        std::vector<Entry>& entries,
        std::function<bool(const Entry&)> is_deleted
    ) {
        std::lock_guard<std::mutex> lock(mtx_);

        auto start = std::chrono::steady_clock::now();
        CompactionResult result = {};
        result.entries_before = entries.size();

        // Calculate size before
        size_t bytes_before = entries.size() * sizeof(Entry);

        // Remove deleted entries
        entries.erase(
            std::remove_if(entries.begin(), entries.end(), is_deleted),
            entries.end()
        );

        // Shrink to fit
        entries.shrink_to_fit();

        result.entries_after = entries.size();
        size_t bytes_after = entries.size() * sizeof(Entry);
        result.bytes_reclaimed = bytes_before - bytes_after;

        auto end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        result.success = true;

        // Update stats
        stats_.compactions_run++;
        stats_.total_bytes_reclaimed += result.bytes_reclaimed;
        stats_.total_entries_removed += (result.entries_before - result.entries_after);

        return result;
    }

    void set_fragmentation_threshold(double threshold) {
        fragmentation_threshold_ = threshold;
    }

    const CompactionStats& get_stats() const { return stats_; }
};

/**
 * Memory Usage Manager - Bounds memory consumption
 */
class MemoryManager {
public:
    struct MemoryUsage {
        size_t current_bytes;
        size_t peak_bytes;
        size_t limit_bytes;
        double usage_percent;
    };

    struct EvictionResult {
        bool success;
        size_t entries_evicted;
        size_t bytes_freed;
    };

private:
    std::atomic<size_t> current_usage_{0};
    std::atomic<size_t> peak_usage_{0};
    size_t memory_limit_;
    mutable std::mutex mtx_;

    double eviction_threshold_ = 0.9;  // Start evicting at 90%
    double target_usage_ = 0.7;        // Evict down to 70%

    struct Statistics {
        std::atomic<uint64_t> allocations{0};
        std::atomic<uint64_t> deallocations{0};
        std::atomic<uint64_t> evictions{0};
        std::atomic<uint64_t> allocation_failures{0};
    } stats_;

public:
    explicit MemoryManager(size_t limit_bytes)
        : memory_limit_(limit_bytes) {}

    // Try to allocate memory
    bool try_allocate(size_t bytes) {
        size_t current = current_usage_.load();
        while (current + bytes <= memory_limit_) {
            if (current_usage_.compare_exchange_weak(current, current + bytes)) {
                ++stats_.allocations;

                // Update peak
                size_t peak = peak_usage_.load();
                while (current + bytes > peak) {
                    peak_usage_.compare_exchange_weak(peak, current + bytes);
                }

                return true;
            }
        }
        ++stats_.allocation_failures;
        return false;
    }

    // Release memory
    void release(size_t bytes) {
        current_usage_.fetch_sub(bytes);
        ++stats_.deallocations;
    }

    // Check if eviction needed
    bool needs_eviction() const {
        return static_cast<double>(current_usage_.load()) / memory_limit_ > eviction_threshold_;
    }

    // Calculate how much to evict
    size_t eviction_target() const {
        size_t current = current_usage_.load();
        size_t target = static_cast<size_t>(memory_limit_ * target_usage_);
        return current > target ? current - target : 0;
    }

    // Get current usage
    MemoryUsage get_usage() const {
        MemoryUsage usage;
        usage.current_bytes = current_usage_.load();
        usage.peak_bytes = peak_usage_.load();
        usage.limit_bytes = memory_limit_;
        usage.usage_percent = (static_cast<double>(usage.current_bytes) / memory_limit_) * 100.0;
        return usage;
    }

    // LRU eviction helper
    template<typename K, typename V>
    EvictionResult evict_lru(
        std::map<K, V>& cache,
        std::map<K, uint64_t>& access_times,
        std::function<size_t(const V&)> size_fn
    ) {
        std::lock_guard<std::mutex> lock(mtx_);

        EvictionResult result = {true, 0, 0};
        size_t target = eviction_target();

        while (result.bytes_freed < target && !cache.empty()) {
            // Find LRU entry
            K lru_key;
            uint64_t min_time = UINT64_MAX;

            for (const auto& [key, time] : access_times) {
                if (time < min_time) {
                    min_time = time;
                    lru_key = key;
                }
            }

            // Evict
            auto it = cache.find(lru_key);
            if (it != cache.end()) {
                size_t entry_size = size_fn(it->second);
                cache.erase(it);
                access_times.erase(lru_key);
                release(entry_size);
                result.bytes_freed += entry_size;
                result.entries_evicted++;
                ++stats_.evictions;
            } else {
                break;
            }
        }

        return result;
    }

    void set_limit(size_t limit) { memory_limit_ = limit; }
    size_t get_limit() const { return memory_limit_; }

    uint64_t get_evictions() const { return stats_.evictions.load(); }
    uint64_t get_allocation_failures() const { return stats_.allocation_failures.load(); }
};

/**
 * Database Performance Manager
 */
class DatabasePerformanceManager {
    std::unordered_map<std::string, std::unique_ptr<BTreeIndex<std::string, uint64_t>>> btree_indexes_;
    std::unordered_map<std::string, std::unique_ptr<HashIndex<std::string, uint64_t>>> hash_indexes_;
    DegradationMonitor degradation_monitor_;
    Compactor compactor_;
    std::unique_ptr<MemoryManager> memory_manager_;
    mutable std::mutex mtx_;

    DatabasePerformanceManager()
        : memory_manager_(std::make_unique<MemoryManager>(1024 * 1024 * 1024)) {}  // 1GB default

public:
    static DatabasePerformanceManager& instance() {
        static DatabasePerformanceManager inst;
        return inst;
    }

    // Create B-Tree index
    void create_btree_index(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        btree_indexes_[name] = std::make_unique<BTreeIndex<std::string, uint64_t>>(name);
    }

    // Create Hash index
    void create_hash_index(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        hash_indexes_[name] = std::make_unique<HashIndex<std::string, uint64_t>>(name);
    }

    // Get B-Tree index
    BTreeIndex<std::string, uint64_t>* get_btree_index(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = btree_indexes_.find(name);
        return it != btree_indexes_.end() ? it->second.get() : nullptr;
    }

    // Get Hash index
    HashIndex<std::string, uint64_t>* get_hash_index(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = hash_indexes_.find(name);
        return it != hash_indexes_.end() ? it->second.get() : nullptr;
    }

    DegradationMonitor& degradation_monitor() { return degradation_monitor_; }
    Compactor& compactor() { return compactor_; }
    MemoryManager& memory_manager() { return *memory_manager_; }
};

} // namespace database
} // namespace intcoin

#endif // INTCOIN_DATABASE_PERFORMANCE_H
