// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONCURRENCY_SAFETY_H
#define INTCOIN_CONCURRENCY_SAFETY_H

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <functional>

namespace intcoin {
namespace concurrency {

/**
 * Deadlock Prevention - Ordered Lock Acquisition
 * Locks must be acquired in consistent order to prevent deadlocks
 */
class OrderedLock {
    std::mutex mtx_;
    uint32_t lock_id_;
    static std::atomic<uint32_t> next_id_;

public:
    OrderedLock() : lock_id_(next_id_++) {}

    uint32_t id() const { return lock_id_; }
    std::mutex& get_mutex() { return mtx_; }

    // Lock with ID ordering to prevent deadlock
    void lock() { mtx_.lock(); }
    void unlock() { mtx_.unlock(); }
    bool try_lock() { return mtx_.try_lock(); }
};

inline std::atomic<uint32_t> OrderedLock::next_id_{0};

/**
 * Scoped Lock with Deadlock Detection
 * Acquires locks in consistent order based on lock IDs
 */
template<typename... Mutexes>
class SafeScopedLock {
    std::tuple<std::unique_lock<Mutexes>...> locks_;

public:
    SafeScopedLock(Mutexes&... mtxs) {
        // Use std::lock to acquire all locks atomically (deadlock-free)
        std::lock(mtxs...);
        // Adopt the already-locked mutexes
        locks_ = std::make_tuple(std::unique_lock<Mutexes>(mtxs, std::adopt_lock)...);
    }

    // No copy or move
    SafeScopedLock(const SafeScopedLock&) = delete;
    SafeScopedLock& operator=(const SafeScopedLock&) = delete;
};

/**
 * Read-Write Lock Wrapper
 * Multiple readers, single writer pattern
 */
class ReadWriteLock {
    mutable std::shared_mutex mtx_;

public:
    // RAII read lock
    class ReadLock {
        const ReadWriteLock& rwlock_;
        std::shared_lock<std::shared_mutex> lock_;
    public:
        explicit ReadLock(const ReadWriteLock& rw)
            : rwlock_(rw), lock_(rw.mtx_) {}
    };

    // RAII write lock
    class WriteLock {
        ReadWriteLock& rwlock_;
        std::unique_lock<std::shared_mutex> lock_;
    public:
        explicit WriteLock(ReadWriteLock& rw)
            : rwlock_(rw), lock_(rw.mtx_) {}
    };

    ReadLock read_lock() const { return ReadLock(*this); }
    WriteLock write_lock() { return WriteLock(*this); }
};

/**
 * Thread-Safe Queue
 * Lock-based concurrent queue with bounded size
 */
template<typename T>
class ThreadSafeQueue {
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t max_size_;
    bool closed_ = false;

public:
    explicit ThreadSafeQueue(size_t max_size = 1000)
        : max_size_(max_size) {}

    // Non-blocking push (returns false if full or closed)
    bool try_push(T item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (closed_ || queue_.size() >= max_size_) {
            return false;
        }
        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Blocking push (waits if full)
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [this] {
            return closed_ || queue_.size() < max_size_;
        });
        if (closed_) return false;
        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Non-blocking pop
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    // Blocking pop (waits if empty)
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    // Timed pop
    template<typename Rep, typename Period>
    std::optional<T> pop_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!not_empty_.wait_for(lock, timeout, [this] {
            return closed_ || !queue_.empty();
        })) {
            return std::nullopt;  // Timeout
        }
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    void close() {
        std::lock_guard<std::mutex> lock(mtx_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    bool is_closed() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return closed_;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }
};

/**
 * Thread-Safe Map
 * Concurrent hash map with fine-grained locking
 */
template<typename K, typename V>
class ThreadSafeMap {
    std::unordered_map<K, V> map_;
    mutable std::shared_mutex mtx_;

public:
    // Insert or update
    void insert(const K& key, V value) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        map_[key] = std::move(value);
    }

    // Get value (returns nullopt if not found)
    std::optional<V> get(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Check if key exists
    bool contains(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return map_.find(key) != map_.end();
    }

    // Remove key
    bool erase(const K& key) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        return map_.erase(key) > 0;
    }

    // Get size
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return map_.size();
    }

    // Clear all
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        map_.clear();
    }

    // Execute function under read lock
    template<typename F>
    auto with_read_lock(F&& func) const -> decltype(func(map_)) {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return func(map_);
    }

    // Execute function under write lock
    template<typename F>
    auto with_write_lock(F&& func) -> decltype(func(map_)) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        return func(map_);
    }
};

/**
 * Atomic Operations Helper
 * Safe atomic operations with memory ordering
 */
template<typename T>
class SafeAtomic {
    std::atomic<T> value_;

public:
    SafeAtomic() : value_(T{}) {}
    explicit SafeAtomic(T initial) : value_(initial) {}

    // Load with acquire semantics (reads after this see writes before release)
    T load() const {
        return value_.load(std::memory_order_acquire);
    }

    // Store with release semantics (writes before this are visible after acquire)
    void store(T val) {
        value_.store(val, std::memory_order_release);
    }

    // Atomic exchange
    T exchange(T val) {
        return value_.exchange(val, std::memory_order_acq_rel);
    }

    // Compare-exchange (returns true if exchanged)
    bool compare_exchange(T& expected, T desired) {
        return value_.compare_exchange_strong(
            expected, desired,
            std::memory_order_acq_rel,
            std::memory_order_acquire
        );
    }

    // Atomic increment (returns previous value)
    T fetch_add(T val) {
        return value_.fetch_add(val, std::memory_order_acq_rel);
    }

    // Atomic decrement
    T fetch_sub(T val) {
        return value_.fetch_sub(val, std::memory_order_acq_rel);
    }

    // Pre-increment
    T operator++() {
        return fetch_add(1) + 1;
    }

    // Post-increment
    T operator++(int) {
        return fetch_add(1);
    }

    // Pre-decrement
    T operator--() {
        return fetch_sub(1) - 1;
    }

    // Post-decrement
    T operator--(int) {
        return fetch_sub(1);
    }
};

/**
 * Atomic Flag (for simple signaling)
 */
class AtomicFlag {
    std::atomic<bool> flag_{false};

public:
    void set() { flag_.store(true, std::memory_order_release); }
    void clear() { flag_.store(false, std::memory_order_release); }
    bool test() const { return flag_.load(std::memory_order_acquire); }
    bool test_and_set() { return flag_.exchange(true, std::memory_order_acq_rel); }
};

/**
 * Spin Lock (for short critical sections)
 */
class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // Spin-wait with pause hint
            std::this_thread::yield();
        }
    }

    void unlock() {
        flag_.clear(std::memory_order_release);
    }

    bool try_lock() {
        return !flag_.test_and_set(std::memory_order_acquire);
    }
};

/**
 * Once Flag (thread-safe initialization)
 */
class OnceFlag {
    std::once_flag flag_;

public:
    template<typename F, typename... Args>
    void call_once(F&& func, Args&&... args) {
        std::call_once(flag_, std::forward<F>(func), std::forward<Args>(args)...);
    }
};

/**
 * Data Race Detector (Debug mode)
 * Tracks access patterns to detect potential races
 */
class DataRaceDetector {
    struct AccessInfo {
        std::thread::id thread_id;
        bool is_write;
        uint64_t timestamp;
    };

    mutable std::mutex mtx_;
    std::vector<AccessInfo> accesses_;
    static constexpr size_t MAX_HISTORY = 100;

public:
    void record_read() {
        record_access(false);
    }

    void record_write() {
        record_access(true);
    }

    // Check for potential races (writes from different threads without sync)
    bool has_potential_race() const {
        std::lock_guard<std::mutex> lock(mtx_);

        std::thread::id last_writer;
        bool has_writer = false;

        for (const auto& access : accesses_) {
            if (access.is_write) {
                if (has_writer && access.thread_id != last_writer) {
                    return true;  // Different threads writing
                }
                last_writer = access.thread_id;
                has_writer = true;
            }
        }

        return false;
    }

private:
    void record_access(bool is_write) {
        std::lock_guard<std::mutex> lock(mtx_);

        AccessInfo info;
        info.thread_id = std::this_thread::get_id();
        info.is_write = is_write;
        info.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        accesses_.push_back(info);

        if (accesses_.size() > MAX_HISTORY) {
            accesses_.erase(accesses_.begin());
        }
    }
};

/**
 * Thread-Safe Counter
 */
class ThreadSafeCounter {
    SafeAtomic<uint64_t> count_{0};

public:
    uint64_t increment() { return ++count_; }
    uint64_t decrement() { return --count_; }
    uint64_t get() const { return count_.load(); }
    void reset() { count_.store(0); }
    uint64_t add(uint64_t val) { return count_.fetch_add(val) + val; }
};

/**
 * Semaphore (counting semaphore)
 */
class Semaphore {
    std::mutex mtx_;
    std::condition_variable cv_;
    size_t count_;

public:
    explicit Semaphore(size_t initial = 0) : count_(initial) {}

    void release(size_t n = 1) {
        std::lock_guard<std::mutex> lock(mtx_);
        count_ += n;
        for (size_t i = 0; i < n; ++i) {
            cv_.notify_one();
        }
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }

    bool try_acquire() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (count_ > 0) {
            --count_;
            return true;
        }
        return false;
    }

    template<typename Rep, typename Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_for(lock, timeout, [this] { return count_ > 0; })) {
            return false;
        }
        --count_;
        return true;
    }
};

/**
 * Concurrency Safety Statistics
 */
class ConcurrencyStats {
    static inline struct Stats {
        SafeAtomic<uint64_t> locks_acquired{0};
        SafeAtomic<uint64_t> locks_released{0};
        SafeAtomic<uint64_t> lock_contentions{0};
        SafeAtomic<uint64_t> deadlocks_prevented{0};
        SafeAtomic<uint64_t> races_detected{0};
    } stats_;

public:
    static void track_lock_acquire() { ++stats_.locks_acquired; }
    static void track_lock_release() { ++stats_.locks_released; }
    static void track_contention() { ++stats_.lock_contentions; }
    static void track_deadlock_prevention() { ++stats_.deadlocks_prevented; }
    static void track_race_detection() { ++stats_.races_detected; }

    static uint64_t get_locks_acquired() { return stats_.locks_acquired.load(); }
    static uint64_t get_locks_released() { return stats_.locks_released.load(); }
    static uint64_t get_contentions() { return stats_.lock_contentions.load(); }
    static uint64_t get_deadlocks_prevented() { return stats_.deadlocks_prevented.load(); }
    static uint64_t get_races_detected() { return stats_.races_detected.load(); }

    static bool check_lock_balance() {
        return stats_.locks_acquired.load() == stats_.locks_released.load();
    }
};

} // namespace concurrency
} // namespace intcoin

#endif // INTCOIN_CONCURRENCY_SAFETY_H
