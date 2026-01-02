// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/ibd/parallel_validation.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>

namespace intcoin {
namespace ibd {

// ThreadPool implementation
class ThreadPool::Impl {
public:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

    explicit Impl(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });

                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~Impl() {
        stop_.store(true);
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

ThreadPool::ThreadPool(size_t num_threads)
    : pimpl_(std::make_unique<Impl>(num_threads)) {}

ThreadPool::~ThreadPool() = default;

size_t ThreadPool::GetThreadCount() const {
    return pimpl_->workers_.size();
}

size_t ThreadPool::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(pimpl_->queue_mutex_);
    return pimpl_->tasks_.size();
}

// ParallelBlockProcessor implementation
class ParallelBlockProcessor::Impl {
public:
    Config config_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::map<uint256, ValidationFuture> pending_validations_;
    std::mutex mutex_;
    ValidationStats stats_;
    std::atomic<bool> enabled_{true};

    explicit Impl(const Config& config)
        : config_(config) {
        size_t num_threads = config.num_threads;
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }
        thread_pool_ = std::make_unique<ThreadPool>(num_threads);
    }

    ValidationResult ValidateBlock(const CBlock& block) {
        ValidationResult result;
        auto start = std::chrono::steady_clock::now();

        // TODO: Actual block validation logic
        // For now, simulate validation
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        result.valid = true;

        auto end = std::chrono::steady_clock::now();
        result.validation_time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        return result;
    }
};

ParallelBlockProcessor::ParallelBlockProcessor(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

ParallelBlockProcessor::~ParallelBlockProcessor() = default;

ValidationFuture ParallelBlockProcessor::SubmitBlock(
    const CBlock& block,
    CBlockIndex* index
) {
    if (!pimpl_->enabled_.load()) {
        // Synchronous validation
        std::promise<ValidationResult> promise;
        ValidationResult result = pimpl_->ValidateBlock(block);
        promise.set_value(result);
        return promise.get_future().share();
    }

    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    // Create validation task
    auto promise = std::make_shared<std::promise<ValidationResult>>();
    auto future = promise->get_future().share();

    // TODO: Submit to thread pool
    // For now, validate synchronously
    ValidationResult result = pimpl_->ValidateBlock(block);
    promise->set_value(result);

    pimpl_->stats_.blocks_submitted++;

    return future;
}

uint32_t ParallelBlockProcessor::ProcessValidatedBlocks() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    uint32_t processed = 0;

    // TODO: Process validated blocks in consensus order

    return processed;
}

void ParallelBlockProcessor::WaitForCompletion() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    // TODO: Wait for all pending validations
}

ValidationStats ParallelBlockProcessor::GetStats() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    auto stats = pimpl_->stats_;
    stats.active_threads = static_cast<uint32_t>(pimpl_->thread_pool_->GetThreadCount());
    stats.queue_size = static_cast<uint32_t>(pimpl_->thread_pool_->GetQueueSize());
    return stats;
}

void ParallelBlockProcessor::SetThreadCount(size_t threads) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;
    }
    pimpl_->thread_pool_ = std::make_unique<ThreadPool>(threads);
}

void ParallelBlockProcessor::SetEnabled(bool enabled) {
    pimpl_->enabled_.store(enabled);
}

bool ParallelBlockProcessor::IsEnabled() const {
    return pimpl_->enabled_.load();
}

} // namespace ibd
} // namespace intcoin
