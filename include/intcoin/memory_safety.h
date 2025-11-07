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

} // namespace memory_safety
} // namespace intcoin

#endif // INTCOIN_MEMORY_SAFETY_H
