#ifndef INTCOIN_MEMORY_SAFETY_H
#define INTCOIN_MEMORY_SAFETY_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <string>
#include <optional>
#include <memory>
#include <algorithm>

namespace intcoin {
namespace memory_safety {

/**
 * Memory Safety Framework
 * Provides buffer overflow protection, bounds checking, and safe memory operations.
 */

/**
 * Bounds-Checked Buffer
 * Wrapper around byte arrays with automatic bounds checking
 */
class SafeBuffer {
    std::vector<uint8_t> data_;
    size_t capacity_;

public:
    // Constructor with maximum capacity
    explicit SafeBuffer(size_t capacity)
        : data_(), capacity_(capacity) {
        data_.reserve(capacity);
    }

    // Get current size
    size_t size() const { return data_.size(); }

    // Get capacity
    size_t capacity() const { return capacity_; }

    // Get remaining space
    size_t available() const { return capacity_ - data_.size(); }

    // Check if buffer is full
    bool is_full() const { return data_.size() >= capacity_; }

    // Check if buffer is empty
    bool is_empty() const { return data_.empty(); }

    // Safe append - returns false if would overflow
    bool append(const uint8_t* src, size_t len) {
        if (!src || len == 0) return true;  // No-op

        // Check for overflow
        if (len > available()) {
            return false;  // Would overflow
        }

        data_.insert(data_.end(), src, src + len);
        return true;
    }

    // Safe append from vector
    bool append(const std::vector<uint8_t>& src) {
        return append(src.data(), src.size());
    }

    // Safe append single byte
    bool append_byte(uint8_t byte) {
        return append(&byte, 1);
    }

    // Safe read - returns false if out of bounds
    bool read(size_t offset, uint8_t* dest, size_t len) const {
        if (!dest || len == 0) return true;  // No-op

        // Bounds check
        if (offset + len > data_.size() || offset + len < offset) {
            return false;  // Out of bounds or overflow
        }

        std::memcpy(dest, data_.data() + offset, len);
        return true;
    }

    // Safe read to vector
    std::optional<std::vector<uint8_t>> read(size_t offset, size_t len) const {
        if (offset + len > data_.size() || offset + len < offset) {
            return std::nullopt;  // Out of bounds
        }

        return std::vector<uint8_t>(
            data_.begin() + offset,
            data_.begin() + offset + len
        );
    }

    // Safe indexed access
    std::optional<uint8_t> at(size_t index) const {
        if (index >= data_.size()) {
            return std::nullopt;
        }
        return data_[index];
    }

    // Get raw data (const only - no modification)
    const uint8_t* data() const { return data_.data(); }

    // Get as vector (copy)
    std::vector<uint8_t> to_vector() const { return data_; }

    // Clear buffer
    void clear() { data_.clear(); }

    // Reset to new capacity (clears data)
    void reset(size_t new_capacity) {
        data_.clear();
        capacity_ = new_capacity;
        data_.reserve(capacity_);
    }
};

/**
 * Safe String Operations
 * Bounds-checked string operations to prevent buffer overflows
 */
class SafeString {
public:
    // Safe string copy with bounds checking
    // Returns false if dest buffer too small
    static bool safe_strcpy(char* dest, size_t dest_size, const char* src) {
        if (!dest || !src || dest_size == 0) {
            return false;
        }

        size_t src_len = std::strlen(src);
        if (src_len >= dest_size) {
            return false;  // Won't fit (need room for null terminator)
        }

        std::strncpy(dest, src, dest_size);
        dest[dest_size - 1] = '\0';  // Ensure null termination
        return true;
    }

    // Safe string concatenation with bounds checking
    static bool safe_strcat(char* dest, size_t dest_size, const char* src) {
        if (!dest || !src || dest_size == 0) {
            return false;
        }

        size_t dest_len = std::strlen(dest);
        size_t src_len = std::strlen(src);

        if (dest_len + src_len >= dest_size) {
            return false;  // Won't fit
        }

        std::strncat(dest, src, dest_size - dest_len - 1);
        dest[dest_size - 1] = '\0';  // Ensure null termination
        return true;
    }

    // Safe string formatting (like snprintf but returns success)
    template<typename... Args>
    static bool safe_format(char* dest, size_t dest_size,
                           const char* format, Args... args) {
        if (!dest || !format || dest_size == 0) {
            return false;
        }

        int result = std::snprintf(dest, dest_size, format, args...);

        // Check for errors or truncation
        if (result < 0 || static_cast<size_t>(result) >= dest_size) {
            dest[0] = '\0';  // Clear on error
            return false;
        }

        return true;
    }

    // Convert to std::string safely (with length limit)
    static std::optional<std::string> to_string(
        const char* src,
        size_t max_length = 1024 * 1024  // 1MB default limit
    ) {
        if (!src) {
            return std::nullopt;
        }

        // Use strnlen to safely find string length
        size_t len = std::strnlen(src, max_length);

        if (len >= max_length) {
            return std::nullopt;  // String too long or not null-terminated
        }

        return std::string(src, len);
    }
};

/**
 * Safe Array Operations
 * Template functions for safe array access
 */
template<typename T, size_t N>
class SafeArray {
    std::array<T, N> data_;
    size_t size_;  // Current number of elements

public:
    SafeArray() : data_(), size_(0) {}

    // Get current size
    size_t size() const { return size_; }

    // Get capacity
    constexpr size_t capacity() const { return N; }

    // Check if full
    bool is_full() const { return size_ >= N; }

    // Check if empty
    bool is_empty() const { return size_ == 0; }

    // Safe push (returns false if full)
    bool push(const T& value) {
        if (size_ >= N) {
            return false;  // Array is full
        }
        data_[size_++] = value;
        return true;
    }

    // Safe pop (returns nullopt if empty)
    std::optional<T> pop() {
        if (size_ == 0) {
            return std::nullopt;
        }
        return data_[--size_];
    }

    // Safe indexed access (returns nullopt if out of bounds)
    std::optional<T> at(size_t index) const {
        if (index >= size_) {
            return std::nullopt;
        }
        return data_[index];
    }

    // Safe indexed write (returns false if out of bounds)
    bool set(size_t index, const T& value) {
        if (index >= size_) {
            return false;
        }
        data_[index] = value;
        return true;
    }

    // Clear array
    void clear() { size_ = 0; }

    // Get raw data pointer (const)
    const T* data() const { return data_.data(); }

    // Begin/end iterators for range-based for
    typename std::array<T, N>::iterator begin() { return data_.begin(); }
    typename std::array<T, N>::iterator end() { return data_.begin() + size_; }
    typename std::array<T, N>::const_iterator begin() const { return data_.begin(); }
    typename std::array<T, N>::const_iterator end() const { return data_.begin() + size_; }
};

/**
 * Safe Memory Copy Operations
 * Bounds-checked memory operations
 */
class SafeMemory {
public:
    // Safe memory copy with bounds checking
    static bool copy(void* dest, size_t dest_size,
                    const void* src, size_t src_size) {
        if (!dest || !src) {
            return false;
        }

        if (src_size > dest_size) {
            return false;  // Source too large for destination
        }

        std::memcpy(dest, src, src_size);
        return true;
    }

    // Safe memory copy from vector to buffer
    static bool copy_from_vector(uint8_t* dest, size_t dest_size,
                                const std::vector<uint8_t>& src) {
        return copy(dest, dest_size, src.data(), src.size());
    }

    // Safe memory copy to vector
    static std::optional<std::vector<uint8_t>> copy_to_vector(
        const uint8_t* src,
        size_t size,
        size_t max_size = 10 * 1024 * 1024  // 10MB default limit
    ) {
        if (!src || size == 0) {
            return std::vector<uint8_t>();  // Empty vector
        }

        if (size > max_size) {
            return std::nullopt;  // Too large
        }

        return std::vector<uint8_t>(src, src + size);
    }

    // Secure memory clear (prevents compiler optimization)
    static void secure_clear(void* ptr, size_t size) {
        if (!ptr || size == 0) return;

        // Use volatile to prevent compiler from optimizing away
        volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
        for (size_t i = 0; i < size; ++i) {
            p[i] = 0;
        }
    }

    // Secure memory compare (constant-time to prevent timing attacks)
    static bool secure_compare(const void* a, const void* b, size_t size) {
        if (!a || !b) return false;

        const volatile uint8_t* pa = static_cast<const volatile uint8_t*>(a);
        const volatile uint8_t* pb = static_cast<const volatile uint8_t*>(b);

        volatile uint8_t result = 0;
        for (size_t i = 0; i < size; ++i) {
            result |= pa[i] ^ pb[i];
        }

        return result == 0;
    }
};

/**
 * RAII Secure Memory
 * Automatically cleared on destruction
 */
template<typename T>
class SecureMemory {
    T* data_;
    size_t size_;

public:
    SecureMemory(size_t size)
        : data_(new T[size]()), size_(size) {}

    ~SecureMemory() {
        if (data_) {
            SafeMemory::secure_clear(data_, size_ * sizeof(T));
            delete[] data_;
        }
    }

    // Disable copy
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;

    // Enable move
    SecureMemory(SecureMemory&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    SecureMemory& operator=(SecureMemory&& other) noexcept {
        if (this != &other) {
            if (data_) {
                SafeMemory::secure_clear(data_, size_ * sizeof(T));
                delete[] data_;
            }
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // Access
    T* data() { return data_; }
    const T* data() const { return data_; }
    size_t size() const { return size_; }

    // Safe indexed access
    std::optional<T> at(size_t index) const {
        if (index >= size_) {
            return std::nullopt;
        }
        return data_[index];
    }

    // Safe indexed write
    bool set(size_t index, const T& value) {
        if (index >= size_) {
            return false;
        }
        data_[index] = value;
        return true;
    }
};

/**
 * Bounded Vector
 * Vector with maximum size limit to prevent unbounded growth
 */
template<typename T>
class BoundedVector {
    std::vector<T> data_;
    size_t max_size_;

public:
    explicit BoundedVector(size_t max_size)
        : data_(), max_size_(max_size) {
        data_.reserve(std::min(max_size, size_t(1024)));  // Reserve reasonable initial size
    }

    // Get current size
    size_t size() const { return data_.size(); }

    // Get max size
    size_t max_size() const { return max_size_; }

    // Check if full
    bool is_full() const { return data_.size() >= max_size_; }

    // Safe push (returns false if would exceed limit)
    bool push_back(const T& value) {
        if (data_.size() >= max_size_) {
            return false;
        }
        data_.push_back(value);
        return true;
    }

    bool push_back(T&& value) {
        if (data_.size() >= max_size_) {
            return false;
        }
        data_.push_back(std::move(value));
        return true;
    }

    // Safe emplace (returns false if would exceed limit)
    template<typename... Args>
    bool emplace_back(Args&&... args) {
        if (data_.size() >= max_size_) {
            return false;
        }
        data_.emplace_back(std::forward<Args>(args)...);
        return true;
    }

    // Standard vector operations
    void clear() { data_.clear(); }
    bool empty() const { return data_.empty(); }

    // Safe indexed access
    std::optional<T> at(size_t index) const {
        if (index >= data_.size()) {
            return std::nullopt;
        }
        return data_[index];
    }

    // Safe indexed access (reference)
    T* at_ptr(size_t index) {
        if (index >= data_.size()) {
            return nullptr;
        }
        return &data_[index];
    }

    const T* at_ptr(size_t index) const {
        if (index >= data_.size()) {
            return nullptr;
        }
        return &data_[index];
    }

    // Iterators
    typename std::vector<T>::iterator begin() { return data_.begin(); }
    typename std::vector<T>::iterator end() { return data_.end(); }
    typename std::vector<T>::const_iterator begin() const { return data_.begin(); }
    typename std::vector<T>::const_iterator end() const { return data_.end(); }

    // Get underlying vector (const)
    const std::vector<T>& get() const { return data_; }
};

/**
 * Stack Bounds Checker
 * Detect potential stack overflows
 */
class StackGuard {
    static constexpr size_t MAX_RECURSION_DEPTH = 1000;
    static thread_local size_t recursion_depth_;

public:
    StackGuard() {
        ++recursion_depth_;
    }

    ~StackGuard() {
        --recursion_depth_;
    }

    static bool check_depth() {
        return recursion_depth_ < MAX_RECURSION_DEPTH;
    }

    static size_t depth() {
        return recursion_depth_;
    }
};

// Thread-local storage for recursion depth
template<typename T = void>
struct StackGuardStorage {
    static thread_local size_t value;
};

template<typename T>
thread_local size_t StackGuardStorage<T>::value = 0;

#define STACK_GUARD() \
    intcoin::memory_safety::StackGuard _stack_guard; \
    if (!intcoin::memory_safety::StackGuard::check_depth()) { \
        throw std::runtime_error("Stack overflow detected"); \
    }

/**
 * Alignment Helpers
 * Ensure proper memory alignment
 */
class Alignment {
public:
    // Check if pointer is aligned
    template<typename T>
    static bool is_aligned(const T* ptr, size_t alignment = alignof(T)) {
        return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
    }

    // Align size up to alignment boundary
    static size_t align_up(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    // Align size down to alignment boundary
    static size_t align_down(size_t size, size_t alignment) {
        return size & ~(alignment - 1);
    }
};

/**
 * RAII Resource Manager
 * Generic RAII wrapper for any resource with custom deleter
 */
template<typename T, typename Deleter>
class RAIIResource {
    T resource_;
    Deleter deleter_;
    bool owns_ = true;

public:
    explicit RAIIResource(T resource, Deleter deleter)
        : resource_(resource), deleter_(deleter) {}

    // No copy (single ownership)
    RAIIResource(const RAIIResource&) = delete;
    RAIIResource& operator=(const RAIIResource&) = delete;

    // Move allowed
    RAIIResource(RAIIResource&& other) noexcept
        : resource_(other.resource_), deleter_(std::move(other.deleter_)), owns_(other.owns_) {
        other.owns_ = false;
    }

    RAIIResource& operator=(RAIIResource&& other) noexcept {
        if (this != &other) {
            release();
            resource_ = other.resource_;
            deleter_ = std::move(other.deleter_);
            owns_ = other.owns_;
            other.owns_ = false;
        }
        return *this;
    }

    ~RAIIResource() { release(); }

    T get() const { return resource_; }
    T operator*() const { return resource_; }

    void release() {
        if (owns_) {
            deleter_(resource_);
            owns_ = false;
        }
    }

    T detach() {
        owns_ = false;
        return resource_;
    }
};

/**
 * RAII File Handle
 * Automatic file closing on scope exit
 */
class FileHandle {
    FILE* file_ = nullptr;
    std::string path_;

public:
    FileHandle() = default;

    explicit FileHandle(const std::string& path, const char* mode = "r")
        : path_(path) {
        file_ = std::fopen(path.c_str(), mode);
    }

    // No copy
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // Move allowed
    FileHandle(FileHandle&& other) noexcept
        : file_(other.file_), path_(std::move(other.path_)) {
        other.file_ = nullptr;
    }

    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            close();
            file_ = other.file_;
            path_ = std::move(other.path_);
            other.file_ = nullptr;
        }
        return *this;
    }

    ~FileHandle() { close(); }

    bool is_open() const { return file_ != nullptr; }
    FILE* get() const { return file_; }
    const std::string& path() const { return path_; }

    void close() {
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }
};

/**
 * RAII Scope Guard
 * Execute cleanup function on scope exit
 */
class ScopeGuard {
    std::function<void()> cleanup_;
    bool dismissed_ = false;

public:
    explicit ScopeGuard(std::function<void()> cleanup)
        : cleanup_(std::move(cleanup)) {}

    // No copy or move
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

    ~ScopeGuard() {
        if (!dismissed_ && cleanup_) {
            cleanup_();
        }
    }

    void dismiss() { dismissed_ = true; }
};

/**
 * Smart Pointer Guidelines Enforcer
 * Factory methods that encourage proper smart pointer usage
 */
class SmartPointerFactory {
public:
    // Create unique_ptr (single ownership - preferred for most cases)
    template<typename T, typename... Args>
    static std::unique_ptr<T> make_unique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Create shared_ptr (shared ownership - use only when necessary)
    template<typename T, typename... Args>
    static std::shared_ptr<T> make_shared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Create weak_ptr from shared_ptr (non-owning observer)
    template<typename T>
    static std::weak_ptr<T> make_weak(const std::shared_ptr<T>& shared) {
        return std::weak_ptr<T>(shared);
    }

    // Safe shared_ptr access (checks if expired)
    template<typename T>
    static std::shared_ptr<T> lock_weak(const std::weak_ptr<T>& weak) {
        return weak.lock();  // Returns nullptr if expired
    }
};

/**
 * Undefined Behavior Prevention
 * Safe arithmetic and operations to prevent UB
 */
class UBPrevention {
public:
    // Safe signed integer addition (prevents overflow UB)
    static std::optional<int64_t> safe_add(int64_t a, int64_t b) {
        if (b > 0 && a > INT64_MAX - b) return std::nullopt;
        if (b < 0 && a < INT64_MIN - b) return std::nullopt;
        return a + b;
    }

    // Safe signed integer subtraction
    static std::optional<int64_t> safe_sub(int64_t a, int64_t b) {
        if (b < 0 && a > INT64_MAX + b) return std::nullopt;
        if (b > 0 && a < INT64_MIN + b) return std::nullopt;
        return a - b;
    }

    // Safe signed integer multiplication
    static std::optional<int64_t> safe_mul(int64_t a, int64_t b) {
        if (a == 0 || b == 0) return 0;
        if (a > 0 && b > 0 && a > INT64_MAX / b) return std::nullopt;
        if (a > 0 && b < 0 && b < INT64_MIN / a) return std::nullopt;
        if (a < 0 && b > 0 && a < INT64_MIN / b) return std::nullopt;
        if (a < 0 && b < 0 && a < INT64_MAX / b) return std::nullopt;
        return a * b;
    }

    // Safe signed integer division (prevents divide by zero and INT_MIN/-1)
    static std::optional<int64_t> safe_div(int64_t a, int64_t b) {
        if (b == 0) return std::nullopt;
        if (a == INT64_MIN && b == -1) return std::nullopt;
        return a / b;
    }

    // Safe left shift (prevents UB from oversized shifts)
    static std::optional<uint64_t> safe_shl(uint64_t value, unsigned shift) {
        if (shift >= 64) return std::nullopt;
        if (value > (UINT64_MAX >> shift)) return std::nullopt;
        return value << shift;
    }

    // Safe right shift
    static uint64_t safe_shr(uint64_t value, unsigned shift) {
        if (shift >= 64) return 0;
        return value >> shift;
    }

    // Safe pointer dereference (checks null)
    template<typename T>
    static T& safe_deref(T* ptr) {
        if (!ptr) {
            throw std::runtime_error("Null pointer dereference");
        }
        return *ptr;
    }

    // Safe array access (bounds checking)
    template<typename Container>
    static auto safe_at(Container& c, size_t index) -> decltype(c.at(index)) {
        return c.at(index);
    }

    // Safe modulo (prevents divide by zero)
    static std::optional<int64_t> safe_mod(int64_t a, int64_t b) {
        if (b == 0) return std::nullopt;
        return a % b;
    }
};

/**
 * Non-Null Pointer Wrapper
 * Compile-time guarantee of non-null pointer
 */
template<typename T>
class NonNull {
    T* ptr_;

public:
    explicit NonNull(T* ptr) : ptr_(ptr) {
        if (!ptr_) {
            throw std::invalid_argument("NonNull constructed with null");
        }
    }

    explicit NonNull(T& ref) : ptr_(&ref) {}

    NonNull() = delete;

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }

    bool operator==(const NonNull& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const NonNull& o) const { return ptr_ != o.ptr_; }
};

/**
 * Optional Reference
 * Safe alternative to nullable pointers
 */
template<typename T>
class OptionalRef {
    T* ptr_ = nullptr;

public:
    OptionalRef() = default;
    OptionalRef(T& ref) : ptr_(&ref) {}
    OptionalRef(std::nullopt_t) : ptr_(nullptr) {}

    bool has_value() const noexcept { return ptr_ != nullptr; }
    explicit operator bool() const noexcept { return has_value(); }

    T& value() {
        if (!ptr_) throw std::bad_optional_access();
        return *ptr_;
    }

    const T& value() const {
        if (!ptr_) throw std::bad_optional_access();
        return *ptr_;
    }

    T& value_or(T& def) noexcept { return ptr_ ? *ptr_ : def; }
    const T& value_or(const T& def) const noexcept { return ptr_ ? *ptr_ : def; }
};

/**
 * Memory Safety Statistics
 */
class MemorySafetyStats {
    static inline struct Stats {
        std::atomic<uint64_t> raii_created{0};
        std::atomic<uint64_t> raii_destroyed{0};
        std::atomic<uint64_t> bounds_checks{0};
        std::atomic<uint64_t> overflow_prevented{0};
        std::atomic<uint64_t> null_checks{0};
    } stats_;

public:
    static void track_raii_create() { ++stats_.raii_created; }
    static void track_raii_destroy() { ++stats_.raii_destroyed; }
    static void track_bounds_check() { ++stats_.bounds_checks; }
    static void track_overflow_prevented() { ++stats_.overflow_prevented; }
    static void track_null_check() { ++stats_.null_checks; }

    static bool check_balance() {
        return stats_.raii_created.load() == stats_.raii_destroyed.load();
    }

    static uint64_t get_raii_created() { return stats_.raii_created.load(); }
    static uint64_t get_raii_destroyed() { return stats_.raii_destroyed.load(); }
    static uint64_t get_bounds_checks() { return stats_.bounds_checks.load(); }
    static uint64_t get_overflow_prevented() { return stats_.overflow_prevented.load(); }
};

} // namespace memory_safety
} // namespace intcoin

#endif // INTCOIN_MEMORY_SAFETY_H
