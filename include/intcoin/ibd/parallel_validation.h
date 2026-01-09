// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_IBD_PARALLEL_VALIDATION_H
#define INTCOIN_IBD_PARALLEL_VALIDATION_H

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <intcoin/types.h>

namespace intcoin {

// Forward declarations
class Block;
class CBlockIndex;

namespace ibd {

/**
 * Block validation result
 */
struct ValidationResult {
    bool valid{false};
    uint256 block_hash{};  // Default-initialized (will be set by test code)
    std::string error_message;
    uint64_t validation_time_ms{0};
};

/**
 * Validation future handle
 */
using ValidationFuture = std::shared_future<ValidationResult>;

/**
 * Validation statistics
 */
struct ValidationStats {
    uint64_t blocks_submitted{0};
    uint64_t blocks_validated{0};
    uint64_t blocks_failed{0};
    uint64_t total_validation_time_ms{0};
    uint32_t active_threads{0};
    uint32_t queue_size{0};

    double GetAverageValidationTime() const {
        if (blocks_validated == 0) return 0.0;
        return static_cast<double>(total_validation_time_ms) / blocks_validated;
    }

    double GetValidationRate() const {
        if (total_validation_time_ms == 0) return 0.0;
        return (blocks_validated * 1000.0) / total_validation_time_ms;
    }
};

/**
 * Thread pool for parallel tasks
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    /**
     * Submit task to thread pool
     *
     * @param task Task function to execute
     * @return Future for task result
     */
    template<typename F, typename... Args>
    auto Submit(F&& task, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * Get number of active threads
     */
    size_t GetThreadCount() const;

    /**
     * Get number of pending tasks
     */
    size_t GetQueueSize() const;

    /**
     * Submit task to thread pool
     *
     * @param task Task function to execute
     */
    void SubmitTask(std::function<void()> task);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Parallel Block Processor
 *
 * Validates blocks in parallel using a thread pool while maintaining
 * consensus ordering for block acceptance.
 */
class ParallelBlockProcessor {
public:
    /**
     * Configuration for parallel processing
     */
    struct Config {
        uint32_t num_threads = 0;        // 0 = auto-detect
        uint32_t max_queue_size = 1000;  // Maximum pending blocks
        bool enable_out_of_order = true; // Allow out-of-order validation
    };

    ParallelBlockProcessor();
    explicit ParallelBlockProcessor(const Config& config);
    ~ParallelBlockProcessor();

    /**
     * Submit block for validation
     *
     * @param block Block to validate
     * @param index Block index
     * @return Future for validation result
     */
    ValidationFuture SubmitBlock(
        const Block& block,
        CBlockIndex* index
    );

    /**
     * Process validated blocks in consensus order
     *
     * Accepts validated blocks that can be added to the chain.
     * Must be called periodically to process results.
     *
     * @return Number of blocks processed
     */
    uint32_t ProcessValidatedBlocks();

    /**
     * Wait for all pending validations to complete
     */
    void WaitForCompletion();

    /**
     * Get validation statistics
     */
    ValidationStats GetStats() const;

    /**
     * Set number of worker threads
     *
     * @param threads Number of threads (0 = auto-detect)
     */
    void SetThreadCount(size_t threads);

    /**
     * Enable or disable parallel validation
     *
     * @param enabled True to enable parallel validation
     */
    void SetEnabled(bool enabled);

    /**
     * Check if parallel validation is enabled
     */
    bool IsEnabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ibd
} // namespace intcoin

#endif // INTCOIN_IBD_PARALLEL_VALIDATION_H
