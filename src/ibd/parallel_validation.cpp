// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/ibd/parallel_validation.h>
#include <intcoin/block.h>
#include <intcoin/transaction.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <chrono>

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

void ThreadPool::SubmitTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(pimpl_->queue_mutex_);
    pimpl_->tasks_.push(std::move(task));
    pimpl_->condition_.notify_one();
}

// ParallelBlockProcessor implementation
class ParallelBlockProcessor::Impl {
public:
    Config config_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::mutex mutex_;
    ValidationStats stats_;
    std::atomic<bool> enabled_{true};

    // Pending validation results indexed by block hash
    std::map<uint256, ValidationFuture> pending_validations_;

    // Queue of validated blocks waiting to be processed
    std::map<uint256, std::pair<ValidationResult, CBlockIndex*>> validated_blocks_;

    explicit Impl(const Config& config)
        : config_(config) {
        size_t num_threads = config.num_threads;
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }
        thread_pool_ = std::make_unique<ThreadPool>(num_threads);
    }

    ValidationResult ValidateBlock(const Block& block) {
        ValidationResult result;
        auto start = std::chrono::steady_clock::now();

        try {
            // 1. Validate block header structure
            if (block.header.version == 0) {
                result.valid = false;
                result.error_message = "Invalid block version";
                return result;
            }

            // 2. Validate timestamp (not too far in future)
            auto now = std::chrono::system_clock::now();
            auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();

            if (block.header.timestamp > static_cast<uint64_t>(now_seconds) + 7200) {
                result.valid = false;
                result.error_message = "Block timestamp too far in future";
                return result;
            }

            // 3. Check Proof of Work
            uint256 block_hash = block.GetHash();

            // Verify RandomX hash meets difficulty target
            if (!CheckProofOfWork(block.header.randomx_hash, block.header.bits)) {
                result.valid = false;
                result.error_message = "Invalid proof of work";
                result.block_hash = block_hash;
                return result;
            }

            // 4. Validate block size
            size_t block_size = block.GetSerializedSize();
            if (block_size > 8 * 1024 * 1024) { // 8 MB max
                result.valid = false;
                result.error_message = "Block size exceeds maximum";
                return result;
            }

            // 5. Validate transactions exist
            if (block.transactions.empty()) {
                result.valid = false;
                result.error_message = "Block has no transactions";
                return result;
            }

            // 6. Validate coinbase transaction (first tx must be coinbase)
            if (!block.transactions[0].IsCoinbase()) {
                result.valid = false;
                result.error_message = "First transaction is not coinbase";
                return result;
            }

            // 7. Check no other transactions are coinbase
            for (size_t i = 1; i < block.transactions.size(); ++i) {
                if (block.transactions[i].IsCoinbase()) {
                    result.valid = false;
                    result.error_message = "Multiple coinbase transactions";
                    return result;
                }
            }

            // 8. Verify Merkle root
            uint256 calculated_merkle = block.CalculateMerkleRoot();
            if (calculated_merkle != block.header.merkle_root) {
                result.valid = false;
                result.error_message = "Invalid Merkle root";
                return result;
            }

            // 9. Validate transaction structure
            for (size_t i = 0; i < block.transactions.size(); ++i) {
                const auto& tx = block.transactions[i];

                // Check transaction has inputs and outputs
                if (!tx.IsCoinbase()) {
                    if (tx.inputs.empty()) {
                        result.valid = false;
                        result.error_message = "Transaction has no inputs";
                        return result;
                    }
                }

                if (tx.outputs.empty()) {
                    result.valid = false;
                    result.error_message = "Transaction has no outputs";
                    return result;
                }

                // Check transaction size
                if (tx.GetSerializedSize() > 1024 * 1024) { // 1 MB max
                    result.valid = false;
                    result.error_message = "Transaction size exceeds maximum";
                    return result;
                }
            }

            // Note: Full UTXO validation and double-spend checks
            // are performed later in ProcessValidatedBlocks() where
            // we have access to the full blockchain and UTXO set

            // Validation successful
            result.valid = true;
            result.block_hash = block_hash;

        } catch (const std::exception& e) {
            result.valid = false;
            result.error_message = std::string("Validation exception: ") + e.what();
        }

        auto end = std::chrono::steady_clock::now();
        result.validation_time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        stats_.blocks_validated++;
        if (result.valid) {
            stats_.total_validation_time_ms += result.validation_time_ms;
        } else {
            stats_.blocks_failed++;
        }

        return result;
    }

    // Helper function to check proof of work
    bool CheckProofOfWork(const uint256& hash, uint32_t bits) {
        // Convert compact bits to target (uint256 is std::array<uint8_t, 32>)
        uint256 target{};
        uint32_t exponent = bits >> 24;
        uint32_t mantissa = bits & 0x00FFFFFF;

        // Bitcoin compact format: target = mantissa * 2^(8*(exponent-3))
        if (exponent <= 3) {
            mantissa >>= (8 * (3 - exponent));
            target[31] = (mantissa >> 0) & 0xFF;
            target[30] = (mantissa >> 8) & 0xFF;
            target[29] = (mantissa >> 16) & 0xFF;
        } else {
            size_t shift = exponent - 3;
            if (shift >= 32) return false;

            target[31 - shift] = (mantissa >> 0) & 0xFF;
            if (shift >= 1) target[30 - shift] = (mantissa >> 8) & 0xFF;
            if (shift >= 2) target[29 - shift] = (mantissa >> 16) & 0xFF;
        }

        // Check if hash <= target (big-endian comparison, most significant byte first)
        for (int i = 0; i < 32; ++i) {
            if (hash[i] < target[i]) return true;
            if (hash[i] > target[i]) return false;
        }
        return true; // Equal is valid
    }
};

ParallelBlockProcessor::ParallelBlockProcessor()
    : pimpl_(std::make_unique<Impl>(Config())) {}

ParallelBlockProcessor::ParallelBlockProcessor(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

ParallelBlockProcessor::~ParallelBlockProcessor() = default;

ValidationFuture ParallelBlockProcessor::SubmitBlock(
    const Block& block,
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

    // Submit validation task to thread pool
    // Create a copy of the block for the thread to use
    auto block_copy = std::make_shared<Block>(block);
    auto index_copy = index;

    pimpl_->thread_pool_->SubmitTask([this, block_copy, index_copy, promise]() {
        try {
            // Perform validation
            ValidationResult result = pimpl_->ValidateBlock(*block_copy);

            // Store result for processing
            {
                std::lock_guard<std::mutex> result_lock(pimpl_->mutex_);
                pimpl_->validated_blocks_[result.block_hash] =
                    std::make_pair(result, index_copy);
            }

            // Set promise value
            promise->set_value(result);

        } catch (const std::exception& e) {
            ValidationResult error_result;
            error_result.valid = false;
            error_result.error_message = std::string("Exception during validation: ") + e.what();
            promise->set_value(error_result);
        }
    });

    // Track pending validation
    uint256 block_hash = block.GetHash();
    pimpl_->pending_validations_[block_hash] = future;

    pimpl_->stats_.blocks_submitted++;

    return future;
}

uint32_t ParallelBlockProcessor::ProcessValidatedBlocks() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);

    uint32_t processed = 0;

    // Process validated blocks
    // Note: In a full implementation, this would need to:
    // 1. Sort blocks by height/chain order
    // 2. Check that parent blocks have been processed
    // 3. Perform UTXO validation with blockchain context
    // 4. Add valid blocks to the blockchain
    // 5. Handle reorganizations
    //
    // For now, we just clean up completed validations from the pending map

    auto it = pimpl_->validated_blocks_.begin();
    while (it != pimpl_->validated_blocks_.end()) {
        const uint256& block_hash = it->first;
        const ValidationResult& result = it->second.first;
        [[maybe_unused]] CBlockIndex* index = it->second.second;

        if (result.valid) {
            // Block passed cryptographic validation
            // In full implementation, would add to blockchain here
            processed++;

            // Remove from pending validations
            pimpl_->pending_validations_.erase(block_hash);

            // Remove from validated blocks
            it = pimpl_->validated_blocks_.erase(it);
        } else {
            // Block failed validation, remove it
            pimpl_->pending_validations_.erase(block_hash);
            it = pimpl_->validated_blocks_.erase(it);
        }
    }

    return processed;
}

void ParallelBlockProcessor::WaitForCompletion() {
    // Wait for all pending validations to complete
    std::vector<ValidationFuture> pending_futures;

    {
        std::lock_guard<std::mutex> lock(pimpl_->mutex_);
        for (const auto& pair : pimpl_->pending_validations_) {
            pending_futures.push_back(pair.second);
        }
    }

    // Wait for all futures outside the lock to avoid deadlock
    for (auto& future : pending_futures) {
        try {
            // Wait for validation to complete (with timeout to prevent infinite wait)
            if (future.valid()) {
                auto status = future.wait_for(std::chrono::seconds(60));
                if (status == std::future_status::ready) {
                    future.get(); // Consume the result
                }
            }
        } catch (const std::exception& e) {
            // Ignore exceptions from individual validations
        }
    }

    // Clear pending validations
    {
        std::lock_guard<std::mutex> lock(pimpl_->mutex_);
        pimpl_->pending_validations_.clear();
    }
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
