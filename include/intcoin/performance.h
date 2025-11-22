// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_PERFORMANCE_H
#define INTCOIN_PERFORMANCE_H

#include "primitives.h"
#include "block.h"
#include "transaction.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <optional>
#include <functional>

namespace intcoin {
namespace performance {

/**
 * LRU Cache for frequently accessed data
 * Thread-safe least-recently-used cache
 */
template<typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            cache_misses_++;
            return std::nullopt;
        }

        // Move to front (most recently used)
        access_order_.splice(access_order_.begin(), access_order_, it->second.second);
        cache_hits_++;
        return it->second.first;
    }

    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);

        if (it != cache_.end()) {
            // Update existing entry
            it->second.first = value;
            access_order_.splice(access_order_.begin(), access_order_, it->second.second);
            return;
        }

        // Add new entry
        if (cache_.size() >= capacity_) {
            // Remove least recently used
            auto lru_key = access_order_.back();
            access_order_.pop_back();
            cache_.erase(lru_key);
        }

        access_order_.push_front(key);
        cache_[key] = {value, access_order_.begin()};
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        access_order_.clear();
        cache_hits_ = 0;
        cache_misses_ = 0;
    }

    struct Stats {
        size_t size;
        size_t capacity;
        uint64_t hits;
        uint64_t misses;
        double hit_rate;
    };

    Stats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats;
        stats.size = cache_.size();
        stats.capacity = capacity_;
        stats.hits = cache_hits_;
        stats.misses = cache_misses_;
        stats.hit_rate = (cache_hits_ + cache_misses_) > 0
            ? static_cast<double>(cache_hits_) / (cache_hits_ + cache_misses_)
            : 0.0;
        return stats;
    }

private:
    size_t capacity_;
    mutable std::mutex mutex_;
    std::list<K> access_order_;
    std::unordered_map<K, std::pair<V, typename std::list<K>::iterator>> cache_;
    uint64_t cache_hits_ = 0;
    uint64_t cache_misses_ = 0;
};

/**
 * Block cache for fast block access
 */
class BlockCache {
public:
    BlockCache(size_t capacity = 1000);
    ~BlockCache();

    std::optional<Block> get_block(const Hash256& hash);
    void put_block(const Hash256& hash, const Block& block);
    void clear();

    struct CacheStats {
        size_t cached_blocks;
        uint64_t hits;
        uint64_t misses;
        double hit_rate;
        uint64_t memory_usage_bytes;
    };

    CacheStats get_stats() const;

private:
    std::unique_ptr<LRUCache<Hash256, Block>> cache_;
};

/**
 * Transaction cache for mempool and validation
 */
class TransactionCache {
public:
    TransactionCache(size_t capacity = 10000);
    ~TransactionCache();

    std::optional<Transaction> get_transaction(const Hash256& txid);
    void put_transaction(const Hash256& txid, const Transaction& tx);
    void clear();

    LRUCache<Hash256, Transaction>::Stats get_stats() const;

private:
    std::unique_ptr<LRUCache<Hash256, Transaction>> cache_;
};

/**
 * Memory pool for object reuse
 * Reduces allocations/deallocations for frequently created objects
 */
template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t initial_size = 100) {
        for (size_t i = 0; i < initial_size; ++i) {
            pool_.push_back(std::make_unique<T>());
        }
    }

    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            allocations_++;
            return std::make_unique<T>();
        }

        auto obj = std::move(pool_.back());
        pool_.pop_back();
        pool_hits_++;
        return obj;
    }

    void release(std::unique_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < max_pool_size_) {
            pool_.push_back(std::move(obj));
        }
    }

    struct PoolStats {
        size_t pool_size;
        size_t max_pool_size;
        uint64_t pool_hits;
        uint64_t allocations;
    };

    PoolStats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {pool_.size(), max_pool_size_, pool_hits_, allocations_};
    }

private:
    std::vector<std::unique_ptr<T>> pool_;
    size_t max_pool_size_ = 1000;
    uint64_t pool_hits_ = 0;
    uint64_t allocations_ = 0;
    mutable std::mutex mutex_;
};

/**
 * Batch processor for efficient transaction validation
 */
class BatchProcessor {
public:
    BatchProcessor(size_t batch_size = 100, size_t num_threads = 0);
    ~BatchProcessor();

    /**
     * Process transactions in parallel batches
     */
    using TransactionValidator = std::function<bool(const Transaction&)>;

    struct ValidationResult {
        Hash256 txid;
        bool valid;
        std::string error_message;
    };

    std::vector<ValidationResult> validate_batch(
        const std::vector<Transaction>& transactions,
        TransactionValidator validator
    );

    /**
     * Process blocks in parallel
     */
    using BlockValidator = std::function<bool(const Block&)>;

    std::vector<std::pair<Hash256, bool>> validate_blocks(
        const std::vector<Block>& blocks,
        BlockValidator validator
    );

    struct ProcessorStats {
        uint64_t total_batches_processed;
        uint64_t total_items_processed;
        double avg_batch_time_ms;
        size_t active_threads;
    };

    ProcessorStats get_stats() const;

private:
    size_t batch_size_;
    size_t num_threads_;
    std::atomic<uint64_t> batches_processed_;
    std::atomic<uint64_t> items_processed_;
    std::atomic<uint64_t> total_processing_time_ms_;
};

/**
 * Signature verification batch processor
 * Batch verify multiple signatures in parallel
 */
class SignatureVerificationBatch {
public:
    SignatureVerificationBatch();
    ~SignatureVerificationBatch();

    /**
     * Add signature to batch
     */
    void add(const Hash256& message,
             const DilithiumSignature& signature,
             const PublicKey& pubkey);

    /**
     * Verify all signatures in batch
     */
    struct VerificationResult {
        size_t index;
        Hash256 message;
        bool valid;
    };

    std::vector<VerificationResult> verify_all();

    /**
     * Clear batch
     */
    void clear();

    size_t size() const { return batch_.size(); }

private:
    struct SignatureEntry {
        Hash256 message;
        DilithiumSignature signature;
        PublicKey pubkey;
    };

    std::vector<SignatureEntry> batch_;
    static constexpr size_t MAX_BATCH_SIZE = 1000;
};

/**
 * Performance profiler
 */
class Profiler {
public:
    /**
     * Profile a code block
     */
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name, Profiler& profiler);
        ~ScopedTimer();

    private:
        std::string name_;
        Profiler& profiler_;
        std::chrono::high_resolution_clock::time_point start_;
    };

    void record_timing(const std::string& name, uint64_t duration_ns);

    struct TimingStats {
        uint64_t call_count;
        uint64_t total_time_ns;
        uint64_t min_time_ns;
        uint64_t max_time_ns;
        uint64_t avg_time_ns;
    };

    TimingStats get_timing_stats(const std::string& name) const;
    std::unordered_map<std::string, TimingStats> get_all_stats() const;

    void reset();
    void print_report() const;

private:
    struct TimingEntry {
        uint64_t call_count = 0;
        uint64_t total_time = 0;
        uint64_t min_time = UINT64_MAX;
        uint64_t max_time = 0;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, TimingEntry> timings_;
};

/**
 * Parallel transaction validator
 */
class ParallelTransactionValidator {
public:
    ParallelTransactionValidator(size_t num_threads = 0);
    ~ParallelTransactionValidator();

    /**
     * Validate multiple transactions in parallel
     */
    struct ValidationContext {
        const Transaction* tx;
        bool valid;
        std::string error;
        uint64_t validation_time_ns;
    };

    std::vector<ValidationContext> validate(
        const std::vector<Transaction>& transactions,
        std::function<bool(const Transaction&, std::string&)> validator
    );

    struct ValidatorStats {
        uint64_t total_validated;
        uint64_t total_valid;
        uint64_t total_invalid;
        double avg_validation_time_ms;
        size_t active_threads;
    };

    ValidatorStats get_stats() const;

private:
    size_t num_threads_;
    std::atomic<uint64_t> total_validated_;
    std::atomic<uint64_t> total_valid_;
    std::atomic<uint64_t> total_invalid_;
    std::atomic<uint64_t> total_validation_time_ns_;
};

/**
 * Database query optimizer
 */
class QueryOptimizer {
public:
    /**
     * Batch get blocks by hashes
     */
    static std::vector<std::optional<Block>> batch_get_blocks(
        const std::vector<Hash256>& hashes
    );

    /**
     * Batch get transactions by IDs
     */
    static std::vector<std::optional<Transaction>> batch_get_transactions(
        const std::vector<Hash256>& txids
    );

    /**
     * Prefetch blocks likely to be needed soon
     */
    static void prefetch_blocks(const std::vector<Hash256>& hashes);

    /**
     * Optimize database indices
     */
    static void optimize_indices();

    struct QueryStats {
        uint64_t total_queries;
        uint64_t batch_queries;
        uint64_t single_queries;
        double avg_query_time_ms;
        double batch_speedup_factor;
    };

    static QueryStats get_stats();
};

/**
 * Network message compression optimizer
 */
class MessageOptimizer {
public:
    /**
     * Compress message if beneficial
     */
    static std::vector<uint8_t> optimize_message(
        const std::vector<uint8_t>& message,
        bool& compressed
    );

    /**
     * Decompress message
     */
    static std::optional<std::vector<uint8_t>> decompress_message(
        const std::vector<uint8_t>& compressed_message
    );

    struct CompressionStats {
        uint64_t messages_compressed;
        uint64_t total_original_bytes;
        uint64_t total_compressed_bytes;
        double avg_compression_ratio;
        uint64_t bandwidth_saved_bytes;
    };

    static CompressionStats get_stats();
};

/**
 * Block validation optimizer
 */
class BlockValidationOptimizer {
public:
    /**
     * Validate block with optimizations
     * - Parallel transaction validation
     * - Signature batch verification
     * - Merkle root calculation optimization
     */
    struct OptimizedValidationResult {
        bool valid;
        std::string error;
        uint64_t validation_time_ns;
        struct Breakdown {
            uint64_t header_validation_ns;
            uint64_t tx_validation_ns;
            uint64_t signature_verification_ns;
            uint64_t merkle_verification_ns;
        } breakdown;
    };

    static OptimizedValidationResult validate_block_optimized(
        const Block& block,
        bool parallel = true
    );

    /**
     * Pre-validate block header (fast check)
     */
    static bool quick_validate_header(const Block& block);
};

/**
 * Memory usage tracker
 */
class MemoryTracker {
public:
    static MemoryTracker& instance();

    void track_allocation(const std::string& category, size_t bytes);
    void track_deallocation(const std::string& category, size_t bytes);

    struct MemoryStats {
        size_t total_allocated;
        size_t total_deallocated;
        size_t current_usage;
        std::unordered_map<std::string, size_t> category_usage;
    };

    MemoryStats get_stats() const;
    void reset();

private:
    MemoryTracker() = default;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> total_deallocated_{0};
    std::unordered_map<std::string, std::atomic<size_t>> category_usage_;
    mutable std::mutex mutex_;
};

/**
 * Performance benchmarking suite
 */
class Benchmark {
public:
    /**
     * Benchmark transaction validation
     */
    struct TxValidationBenchmark {
        uint64_t transactions_per_second;
        uint64_t avg_validation_time_ns;
        uint64_t min_validation_time_ns;
        uint64_t max_validation_time_ns;
    };

    static TxValidationBenchmark benchmark_tx_validation(size_t num_transactions = 1000);

    /**
     * Benchmark block validation
     */
    struct BlockValidationBenchmark {
        uint64_t blocks_per_second;
        uint64_t avg_validation_time_ns;
        uint64_t transactions_per_second;
    };

    static BlockValidationBenchmark benchmark_block_validation(size_t num_blocks = 100);

    /**
     * Benchmark signature verification
     */
    struct SignatureVerificationBenchmark {
        uint64_t signatures_per_second;
        uint64_t avg_verification_time_ns;
        uint64_t batch_speedup_factor;  // How much faster batch verification is
    };

    static SignatureVerificationBenchmark benchmark_signature_verification(size_t num_sigs = 1000);

    /**
     * Benchmark database operations
     */
    struct DatabaseBenchmark {
        uint64_t reads_per_second;
        uint64_t writes_per_second;
        uint64_t avg_read_time_ns;
        uint64_t avg_write_time_ns;
    };

    static DatabaseBenchmark benchmark_database(size_t num_operations = 1000);

    /**
     * Run full benchmark suite
     */
    static void run_full_benchmark();
};

/**
 * Performance configuration
 */
struct PerformanceConfig {
    // Caching
    size_t block_cache_size;
    size_t tx_cache_size;
    bool enable_caching;

    // Parallel processing
    size_t num_validation_threads;
    size_t batch_size;
    bool enable_parallel_validation;

    // Memory pooling
    bool enable_memory_pooling;
    size_t memory_pool_size;

    // Compression
    bool enable_message_compression;
    size_t min_compression_size;

    // Profiling
    bool enable_profiling;

    PerformanceConfig()
        : block_cache_size(1000)
        , tx_cache_size(10000)
        , enable_caching(true)
        , num_validation_threads(std::thread::hardware_concurrency())
        , batch_size(100)
        , enable_parallel_validation(true)
        , enable_memory_pooling(true)
        , memory_pool_size(100)
        , enable_message_compression(true)
        , min_compression_size(1024)
        , enable_profiling(false)
    {}
};

/**
 * Global performance manager
 */
class PerformanceManager {
public:
    static PerformanceManager& instance();

    void set_config(const PerformanceConfig& config);
    PerformanceConfig get_config() const;

    BlockCache& get_block_cache();
    TransactionCache& get_tx_cache();
    Profiler& get_profiler();

    /**
     * Get overall performance statistics
     */
    struct OverallStats {
        typename LRUCache<Hash256, Block>::Stats block_cache;
        typename LRUCache<Hash256, Transaction>::Stats tx_cache;
        MemoryTracker::MemoryStats memory;
        size_t active_threads;
    };

    OverallStats get_overall_stats() const;

    void print_performance_report() const;

private:
    PerformanceManager();
    PerformanceConfig config_;
    std::unique_ptr<BlockCache> block_cache_;
    std::unique_ptr<TransactionCache> tx_cache_;
    std::unique_ptr<Profiler> profiler_;
    mutable std::mutex mutex_;
};

// Convenience macros for profiling
#define PROFILE_SCOPE(name) \
    performance::Profiler::ScopedTimer _profile_timer_##__LINE__(name, \
        performance::PerformanceManager::instance().get_profiler())

#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace performance
} // namespace intcoin

#endif // INTCOIN_PERFORMANCE_H
