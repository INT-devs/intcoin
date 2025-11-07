#ifndef INTCOIN_SAFE_MATH_H
#define INTCOIN_SAFE_MATH_H

#include <cstdint>
#include <optional>
#include <limits>
#include <type_traits>

namespace intcoin {
namespace safe_math {

/**
 * Safe Arithmetic Operations
 * Provides overflow-safe arithmetic operations for all integer types.
 * Returns std::optional that is empty (nullopt) on overflow.
 */

/**
 * Safe Addition
 * Detects overflow in addition operations
 */
template<typename T>
std::optional<T> safe_add(T a, T b) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    if constexpr (std::is_unsigned<T>::value) {
        // Unsigned overflow check: a + b < a (wraps around)
        if (a > std::numeric_limits<T>::max() - b) {
            return std::nullopt;  // Overflow detected
        }
        return a + b;
    } else {
        // Signed overflow check
        if (b > 0 && a > std::numeric_limits<T>::max() - b) {
            return std::nullopt;  // Positive overflow
        }
        if (b < 0 && a < std::numeric_limits<T>::min() - b) {
            return std::nullopt;  // Negative overflow
        }
        return a + b;
    }
}

/**
 * Safe Subtraction
 * Detects underflow/overflow in subtraction operations
 */
template<typename T>
std::optional<T> safe_sub(T a, T b) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    if constexpr (std::is_unsigned<T>::value) {
        // Unsigned underflow check: a < b
        if (a < b) {
            return std::nullopt;  // Underflow detected
        }
        return a - b;
    } else {
        // Signed overflow check
        if (b < 0 && a > std::numeric_limits<T>::max() + b) {
            return std::nullopt;  // Positive overflow
        }
        if (b > 0 && a < std::numeric_limits<T>::min() + b) {
            return std::nullopt;  // Negative overflow
        }
        return a - b;
    }
}

/**
 * Safe Multiplication
 * Detects overflow in multiplication operations
 */
template<typename T>
std::optional<T> safe_mul(T a, T b) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    if constexpr (std::is_unsigned<T>::value) {
        // Handle zero cases
        if (a == 0 || b == 0) {
            return 0;
        }

        // Unsigned overflow check: a * b / a != b
        if (a > std::numeric_limits<T>::max() / b) {
            return std::nullopt;  // Overflow detected
        }
        return a * b;
    } else {
        // Handle zero cases
        if (a == 0 || b == 0) {
            return 0;
        }

        // Signed overflow is more complex due to negative numbers
        if (a > 0) {
            if (b > 0) {
                // Both positive
                if (a > std::numeric_limits<T>::max() / b) {
                    return std::nullopt;
                }
            } else {
                // a positive, b negative
                if (b < std::numeric_limits<T>::min() / a) {
                    return std::nullopt;
                }
            }
        } else {
            if (b > 0) {
                // a negative, b positive
                if (a < std::numeric_limits<T>::min() / b) {
                    return std::nullopt;
                }
            } else {
                // Both negative
                if (a != 0 && b < std::numeric_limits<T>::max() / a) {
                    return std::nullopt;
                }
            }
        }
        return a * b;
    }
}

/**
 * Safe Division
 * Detects division by zero and overflow (INT_MIN / -1)
 */
template<typename T>
std::optional<T> safe_div(T a, T b) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    // Division by zero check
    if (b == 0) {
        return std::nullopt;
    }

    if constexpr (std::is_signed<T>::value) {
        // Special case: INT_MIN / -1 overflows in two's complement
        if (a == std::numeric_limits<T>::min() && b == -1) {
            return std::nullopt;
        }
    }

    return a / b;
}

/**
 * Safe Modulo
 * Detects division by zero
 */
template<typename T>
std::optional<T> safe_mod(T a, T b) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    // Division by zero check
    if (b == 0) {
        return std::nullopt;
    }

    if constexpr (std::is_signed<T>::value) {
        // Special case: INT_MIN % -1 can be problematic
        if (a == std::numeric_limits<T>::min() && b == -1) {
            return 0;  // Mathematically correct
        }
    }

    return a % b;
}

/**
 * Safe Negation
 * Detects overflow in negation (e.g., -INT_MIN overflows)
 */
template<typename T>
std::optional<T> safe_negate(T a) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    if constexpr (std::is_unsigned<T>::value) {
        // Unsigned negation only works for zero
        if (a != 0) {
            return std::nullopt;
        }
        return 0;
    } else {
        // Signed: -INT_MIN overflows
        if (a == std::numeric_limits<T>::min()) {
            return std::nullopt;
        }
        return -a;
    }
}

/**
 * Safe Left Shift
 * Detects overflow in left shift operations
 */
template<typename T>
std::optional<T> safe_lshift(T a, unsigned int shift) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    // Shift must be less than bit width
    if (shift >= sizeof(T) * 8) {
        return std::nullopt;
    }

    // For zero, any shift is safe
    if (a == 0) {
        return 0;
    }

    if constexpr (std::is_unsigned<T>::value) {
        // Check if high bits will be lost
        T max_value = std::numeric_limits<T>::max() >> shift;
        if (a > max_value) {
            return std::nullopt;
        }
        return a << shift;
    } else {
        // For signed types, shifting into sign bit is undefined
        if (a < 0) {
            return std::nullopt;  // Don't shift negative values
        }

        T max_value = std::numeric_limits<T>::max() >> shift;
        if (a > max_value) {
            return std::nullopt;
        }
        return a << shift;
    }
}

/**
 * Safe Type Conversion
 * Safely converts between integer types, detecting overflow
 */
template<typename To, typename From>
std::optional<To> safe_cast(From value) {
    static_assert(std::is_integral<To>::value, "To must be an integral type");
    static_assert(std::is_integral<From>::value, "From must be an integral type");

    // Check if value fits in target type
    if (value < std::numeric_limits<To>::min() ||
        value > std::numeric_limits<To>::max()) {
        return std::nullopt;
    }

    return static_cast<To>(value);
}

/**
 * Safe Size Cast
 * Safely converts size_t to smaller types (common operation)
 */
template<typename T>
std::optional<T> safe_size_cast(size_t value) {
    return safe_cast<T, size_t>(value);
}

/**
 * Checked Arithmetic Class
 * Wrapper class that provides checked arithmetic operations
 * Throws exception on overflow (alternative to optional return)
 */
template<typename T>
class Checked {
    T value_;

public:
    explicit Checked(T value) : value_(value) {}

    T value() const { return value_; }

    Checked operator+(const Checked& other) const {
        auto result = safe_add(value_, other.value_);
        if (!result) {
            throw std::overflow_error("Addition overflow");
        }
        return Checked(*result);
    }

    Checked operator-(const Checked& other) const {
        auto result = safe_sub(value_, other.value_);
        if (!result) {
            throw std::overflow_error("Subtraction overflow");
        }
        return Checked(*result);
    }

    Checked operator*(const Checked& other) const {
        auto result = safe_mul(value_, other.value_);
        if (!result) {
            throw std::overflow_error("Multiplication overflow");
        }
        return Checked(*result);
    }

    Checked operator/(const Checked& other) const {
        auto result = safe_div(value_, other.value_);
        if (!result) {
            throw std::overflow_error("Division overflow or division by zero");
        }
        return Checked(*result);
    }

    Checked& operator+=(const Checked& other) {
        *this = *this + other;
        return *this;
    }

    Checked& operator-=(const Checked& other) {
        *this = *this - other;
        return *this;
    }

    Checked& operator*=(const Checked& other) {
        *this = *this * other;
        return *this;
    }

    Checked& operator/=(const Checked& other) {
        *this = *this / other;
        return *this;
    }

    bool operator==(const Checked& other) const {
        return value_ == other.value_;
    }

    bool operator!=(const Checked& other) const {
        return value_ != other.value_;
    }

    bool operator<(const Checked& other) const {
        return value_ < other.value_;
    }

    bool operator>(const Checked& other) const {
        return value_ > other.value_;
    }

    bool operator<=(const Checked& other) const {
        return value_ <= other.value_;
    }

    bool operator>=(const Checked& other) const {
        return value_ >= other.value_;
    }
};

/**
 * Saturation Arithmetic
 * Alternative approach: operations saturate at min/max instead of failing
 */
template<typename T>
T saturating_add(T a, T b) {
    auto result = safe_add(a, b);
    if (!result) {
        // Saturate at maximum value
        return std::numeric_limits<T>::max();
    }
    return *result;
}

template<typename T>
T saturating_sub(T a, T b) {
    auto result = safe_sub(a, b);
    if (!result) {
        // Saturate at minimum value (0 for unsigned)
        return std::numeric_limits<T>::min();
    }
    return *result;
}

template<typename T>
T saturating_mul(T a, T b) {
    auto result = safe_mul(a, b);
    if (!result) {
        // Saturate at maximum value
        return std::numeric_limits<T>::max();
    }
    return *result;
}

/**
 * Common cryptocurrency amount operations
 * Special functions for handling satoshi amounts
 */
namespace amount {
    // Maximum supply: 21 million coins * 100 million satoshis
    constexpr int64_t MAX_AMOUNT = 21000000LL * 100000000LL;
    constexpr int64_t COIN = 100000000LL;  // 1 coin = 100 million satoshis

    // Check if amount is valid
    inline bool is_valid_amount(int64_t amount) {
        return amount >= 0 && amount <= MAX_AMOUNT;
    }

    // Safe amount addition
    inline std::optional<int64_t> add_amounts(int64_t a, int64_t b) {
        if (!is_valid_amount(a) || !is_valid_amount(b)) {
            return std::nullopt;
        }

        auto result = safe_add(a, b);
        if (!result || !is_valid_amount(*result)) {
            return std::nullopt;
        }

        return result;
    }

    // Safe amount subtraction
    inline std::optional<int64_t> sub_amounts(int64_t a, int64_t b) {
        if (!is_valid_amount(a) || !is_valid_amount(b)) {
            return std::nullopt;
        }

        auto result = safe_sub(a, b);
        if (!result || !is_valid_amount(*result)) {
            return std::nullopt;
        }

        return result;
    }

    // Sum a vector of amounts safely
    inline std::optional<int64_t> sum_amounts(const std::vector<int64_t>& amounts) {
        int64_t total = 0;

        for (int64_t amount : amounts) {
            if (!is_valid_amount(amount)) {
                return std::nullopt;
            }

            auto new_total = safe_add(total, amount);
            if (!new_total || !is_valid_amount(*new_total)) {
                return std::nullopt;
            }

            total = *new_total;
        }

        return total;
    }
}

/**
 * Utility macros for common overflow checks
 */
#define SAFE_ADD_OR_RETURN(result, a, b) \
    do { \
        auto _tmp = safe_math::safe_add((a), (b)); \
        if (!_tmp) return std::nullopt; \
        (result) = *_tmp; \
    } while(0)

#define SAFE_MUL_OR_RETURN(result, a, b) \
    do { \
        auto _tmp = safe_math::safe_mul((a), (b)); \
        if (!_tmp) return std::nullopt; \
        (result) = *_tmp; \
    } while(0)

#define SAFE_CAST_OR_RETURN(result, type, value) \
    do { \
        auto _tmp = safe_math::safe_cast<type>((value)); \
        if (!_tmp) return std::nullopt; \
        (result) = *_tmp; \
    } while(0)

} // namespace safe_math
} // namespace intcoin

#endif // INTCOIN_SAFE_MATH_H
