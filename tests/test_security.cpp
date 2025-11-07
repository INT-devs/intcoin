#include "intcoin/validation.h"
#include "intcoin/safe_math.h"
#include "intcoin/memory_safety.h"
#include <iostream>
#include <cassert>
#include <limits>

using namespace intcoin;

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Running test: " #name << "..." << std::flush; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << " PASSED" << std::endl; \
        } catch (const std::exception& e) { \
            tests_failed++; \
            std::cout << " FAILED: " << e.what() << std::endl; \
        } \
    } \
    void test_##name()

#define ASSERT(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

// ============================================================================
// Input Validation Tests
// ============================================================================

TEST(string_length_validation) {
    using namespace validation;

    // Valid length
    auto result1 = StringValidator::validate_length("hello", 1, 10, "test");
    ASSERT(result1.valid);

    // Too short
    auto result2 = StringValidator::validate_length("hi", 5, 10, "test");
    ASSERT(!result2.valid);

    // Too long
    auto result3 = StringValidator::validate_length("hello world!", 1, 5, "test");
    ASSERT(!result3.valid);

    // Edge cases
    auto result4 = StringValidator::validate_length("test", 4, 4, "test");
    ASSERT(result4.valid);
}

TEST(hex_validation) {
    using namespace validation;

    // Valid hex
    auto result1 = StringValidator::validate_hex("deadbeef");
    ASSERT(result1.valid);

    // Valid with uppercase
    auto result2 = StringValidator::validate_hex("DEADBEEF");
    ASSERT(result2.valid);

    // Invalid characters
    auto result3 = StringValidator::validate_hex("hello");
    ASSERT(!result3.valid);

    // Empty string
    auto result4 = StringValidator::validate_hex("");
    ASSERT(!result4.valid);

    // With length check
    auto result5 = StringValidator::validate_hex("abcd1234", 8);
    ASSERT(result5.valid);

    auto result6 = StringValidator::validate_hex("abcd1234", 16);
    ASSERT(!result6.valid);
}

TEST(base58_validation) {
    using namespace validation;

    // Valid base58
    auto result1 = StringValidator::validate_base58("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
    ASSERT(result1.valid);

    // Invalid: contains 0
    auto result2 = StringValidator::validate_base58("test0");
    ASSERT(!result2.valid);

    // Invalid: contains O
    auto result3 = StringValidator::validate_base58("testO");
    ASSERT(!result3.valid);

    // Invalid: contains I
    auto result4 = StringValidator::validate_base58("testI");
    ASSERT(!result4.valid);

    // Invalid: contains l
    auto result5 = StringValidator::validate_base58("testl");
    ASSERT(!result5.valid);
}

TEST(hostname_validation) {
    using namespace validation;

    // Valid hostnames
    auto result1 = StringValidator::validate_hostname("example.com");
    ASSERT(result1.valid);

    auto result2 = StringValidator::validate_hostname("sub.domain.example.com");
    ASSERT(result2.valid);

    auto result3 = StringValidator::validate_hostname("localhost");
    ASSERT(result3.valid);

    // Invalid: starts with hyphen
    auto result4 = StringValidator::validate_hostname("-invalid.com");
    ASSERT(!result4.valid);

    // Invalid: double dots
    auto result5 = StringValidator::validate_hostname("invalid..com");
    ASSERT(!result5.valid);
}

TEST(amount_validation) {
    using namespace validation;

    // Valid amounts
    auto result1 = NumericValidator::validate_amount(100000000);  // 1 BTC
    ASSERT(result1.valid);

    auto result2 = NumericValidator::validate_amount(0);  // Zero
    ASSERT(result2.valid);

    // Invalid: negative
    auto result3 = NumericValidator::validate_amount(-1);
    ASSERT(!result3.valid);

    // Invalid: exceeds max supply
    auto result4 = NumericValidator::validate_amount(21000000LL * 100000000LL + 1);
    ASSERT(!result4.valid);
}

TEST(binary_hash_validation) {
    using namespace validation;

    // Valid hash (32 bytes)
    std::vector<uint8_t> valid_hash(32, 0xAB);
    auto result1 = BinaryValidator::validate_hash(valid_hash);
    ASSERT(result1.valid);

    // Invalid: too short
    std::vector<uint8_t> short_hash(31, 0xAB);
    auto result2 = BinaryValidator::validate_hash(short_hash);
    ASSERT(!result2.valid);

    // Invalid: too long
    std::vector<uint8_t> long_hash(33, 0xAB);
    auto result3 = BinaryValidator::validate_hash(long_hash);
    ASSERT(!result3.valid);
}

TEST(public_key_validation) {
    using namespace validation;

    // Valid compressed pubkey (33 bytes, 0x02 prefix)
    std::vector<uint8_t> compressed_key(33);
    compressed_key[0] = 0x02;
    auto result1 = BinaryValidator::validate_pubkey(compressed_key);
    ASSERT(result1.valid);

    // Valid compressed pubkey (33 bytes, 0x03 prefix)
    compressed_key[0] = 0x03;
    auto result2 = BinaryValidator::validate_pubkey(compressed_key);
    ASSERT(result2.valid);

    // Valid uncompressed pubkey (65 bytes, 0x04 prefix)
    std::vector<uint8_t> uncompressed_key(65);
    uncompressed_key[0] = 0x04;
    auto result3 = BinaryValidator::validate_pubkey(uncompressed_key);
    ASSERT(result3.valid);

    // Invalid: wrong prefix for compressed
    compressed_key.resize(33);
    compressed_key[0] = 0x05;
    auto result4 = BinaryValidator::validate_pubkey(compressed_key);
    ASSERT(!result4.valid);

    // Invalid: wrong length
    std::vector<uint8_t> invalid_key(32);
    auto result5 = BinaryValidator::validate_pubkey(invalid_key);
    ASSERT(!result5.valid);
}

TEST(ipv4_validation) {
    using namespace validation;

    // Valid IPv4 addresses
    auto result1 = NetworkValidator::validate_ipv4("192.168.1.1");
    ASSERT(result1.valid);

    auto result2 = NetworkValidator::validate_ipv4("255.255.255.255");
    ASSERT(result2.valid);

    auto result3 = NetworkValidator::validate_ipv4("0.0.0.0");
    ASSERT(result3.valid);

    // Invalid: out of range
    auto result4 = NetworkValidator::validate_ipv4("256.1.1.1");
    ASSERT(!result4.valid);

    // Invalid: wrong format
    auto result5 = NetworkValidator::validate_ipv4("192.168.1");
    ASSERT(!result5.valid);

    auto result6 = NetworkValidator::validate_ipv4("not.an.ip.address");
    ASSERT(!result6.valid);
}

// ============================================================================
// Safe Math Tests
// ============================================================================

TEST(safe_addition) {
    using namespace safe_math;

    // Normal addition
    auto result1 = safe_add<uint32_t>(100, 200);
    ASSERT(result1.has_value() && *result1 == 300);

    // Overflow detection
    auto result2 = safe_add<uint32_t>(
        std::numeric_limits<uint32_t>::max(),
        1
    );
    ASSERT(!result2.has_value());

    // Signed overflow (positive)
    auto result3 = safe_add<int32_t>(
        std::numeric_limits<int32_t>::max(),
        1
    );
    ASSERT(!result3.has_value());

    // Signed overflow (negative)
    auto result4 = safe_add<int32_t>(
        std::numeric_limits<int32_t>::min(),
        -1
    );
    ASSERT(!result4.has_value());
}

TEST(safe_subtraction) {
    using namespace safe_math;

    // Normal subtraction
    auto result1 = safe_sub<uint32_t>(200, 100);
    ASSERT(result1.has_value() && *result1 == 100);

    // Underflow detection
    auto result2 = safe_sub<uint32_t>(100, 200);
    ASSERT(!result2.has_value());

    // Signed underflow
    auto result3 = safe_sub<int32_t>(
        std::numeric_limits<int32_t>::min(),
        1
    );
    ASSERT(!result3.has_value());
}

TEST(safe_multiplication) {
    using namespace safe_math;

    // Normal multiplication
    auto result1 = safe_mul<uint32_t>(100, 200);
    ASSERT(result1.has_value() && *result1 == 20000);

    // Overflow detection
    auto result2 = safe_mul<uint32_t>(
        std::numeric_limits<uint32_t>::max(),
        2
    );
    ASSERT(!result2.has_value());

    // Zero multiplication (always safe)
    auto result3 = safe_mul<uint32_t>(0, 123456);
    ASSERT(result3.has_value() && *result3 == 0);

    // Signed overflow
    auto result4 = safe_mul<int32_t>(
        std::numeric_limits<int32_t>::max(),
        2
    );
    ASSERT(!result4.has_value());
}

TEST(safe_division) {
    using namespace safe_math;

    // Normal division
    auto result1 = safe_div<uint32_t>(200, 2);
    ASSERT(result1.has_value() && *result1 == 100);

    // Division by zero
    auto result2 = safe_div<uint32_t>(100, 0);
    ASSERT(!result2.has_value());

    // Special case: INT_MIN / -1 overflow
    auto result3 = safe_div<int32_t>(
        std::numeric_limits<int32_t>::min(),
        -1
    );
    ASSERT(!result3.has_value());
}

TEST(safe_type_casting) {
    using namespace safe_math;

    // Safe downcast
    auto result1 = safe_cast<uint8_t, uint32_t>(200);
    ASSERT(result1.has_value() && *result1 == 200);

    // Overflow in downcast
    auto result2 = safe_cast<uint8_t, uint32_t>(300);
    ASSERT(!result2.has_value());

    // Safe upcast (always works)
    auto result3 = safe_cast<uint64_t, uint32_t>(12345);
    ASSERT(result3.has_value() && *result3 == 12345);
}

TEST(amount_operations) {
    using namespace safe_math::amount;

    // Valid amount addition
    auto result1 = add_amounts(COIN, COIN);
    ASSERT(result1.has_value() && *result1 == 2 * COIN);

    // Amount overflow
    auto result2 = add_amounts(MAX_AMOUNT, 1);
    ASSERT(!result2.has_value());

    // Invalid input
    auto result3 = add_amounts(-1, COIN);
    ASSERT(!result3.has_value());

    // Sum vector of amounts
    std::vector<int64_t> amounts = {COIN, COIN * 2, COIN * 3};
    auto result4 = sum_amounts(amounts);
    ASSERT(result4.has_value() && *result4 == COIN * 6);
}

TEST(checked_arithmetic_class) {
    using namespace safe_math;

    // Normal operations
    Checked<uint32_t> a(100);
    Checked<uint32_t> b(200);
    auto c = a + b;
    ASSERT(c.value() == 300);

    // Overflow throws exception
    try {
        Checked<uint32_t> max(std::numeric_limits<uint32_t>::max());
        Checked<uint32_t> one(1);
        auto overflow = max + one;  // Should throw
        ASSERT(false);  // Should not reach here
    } catch (const std::overflow_error&) {
        // Expected
    }
}

TEST(saturation_arithmetic) {
    using namespace safe_math;

    // Normal saturating add
    auto result1 = saturating_add<uint32_t>(100, 200);
    ASSERT(result1 == 300);

    // Saturating add at max
    auto result2 = saturating_add<uint32_t>(
        std::numeric_limits<uint32_t>::max(),
        100
    );
    ASSERT(result2 == std::numeric_limits<uint32_t>::max());

    // Saturating sub at min
    auto result3 = saturating_sub<uint32_t>(50, 100);
    ASSERT(result3 == 0);
}

// ============================================================================
// Memory Safety Tests
// ============================================================================

TEST(safe_buffer) {
    using namespace memory_safety;

    SafeBuffer buffer(100);

    // Normal append
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    ASSERT(buffer.append(data));
    ASSERT(buffer.size() == 5);

    // Read back
    auto read_data = buffer.read(0, 5);
    ASSERT(read_data.has_value());
    ASSERT(*read_data == data);

    // Overflow protection
    std::vector<uint8_t> large_data(200, 0xFF);
    ASSERT(!buffer.append(large_data));  // Should fail

    // Bounds checking on read
    auto out_of_bounds = buffer.read(10, 100);
    ASSERT(!out_of_bounds.has_value());
}

TEST(safe_string_operations) {
    using namespace memory_safety;

    // Safe strcpy
    char dest[10];
    ASSERT(SafeString::safe_strcpy(dest, sizeof(dest), "hello"));
    ASSERT(std::string(dest) == "hello");

    // Buffer too small
    ASSERT(!SafeString::safe_strcpy(dest, sizeof(dest), "this is too long"));

    // Safe strcat
    char dest2[20] = "hello";
    ASSERT(SafeString::safe_strcat(dest2, sizeof(dest2), " world"));
    ASSERT(std::string(dest2) == "hello world");

    // Safe format
    char dest3[20];
    ASSERT(SafeString::safe_format(dest3, sizeof(dest3), "num: %d", 42));
    ASSERT(std::string(dest3) == "num: 42");
}

TEST(safe_array) {
    using namespace memory_safety;

    SafeArray<int, 5> arr;

    // Push elements
    ASSERT(arr.push(1));
    ASSERT(arr.push(2));
    ASSERT(arr.push(3));
    ASSERT(arr.size() == 3);

    // Access elements
    auto val = arr.at(1);
    ASSERT(val.has_value() && *val == 2);

    // Out of bounds
    auto out = arr.at(10);
    ASSERT(!out.has_value());

    // Fill to capacity
    ASSERT(arr.push(4));
    ASSERT(arr.push(5));
    ASSERT(arr.is_full());

    // Can't push when full
    ASSERT(!arr.push(6));

    // Pop elements
    auto popped = arr.pop();
    ASSERT(popped.has_value() && *popped == 5);
    ASSERT(!arr.is_full());
}

TEST(bounded_vector) {
    using namespace memory_safety;

    BoundedVector<int> vec(5);

    // Push elements
    ASSERT(vec.push_back(1));
    ASSERT(vec.push_back(2));
    ASSERT(vec.push_back(3));
    ASSERT(vec.size() == 3);

    // Fill to limit
    ASSERT(vec.push_back(4));
    ASSERT(vec.push_back(5));
    ASSERT(vec.is_full());

    // Can't exceed limit
    ASSERT(!vec.push_back(6));

    // Safe access
    auto val = vec.at(2);
    ASSERT(val.has_value() && *val == 3);

    auto out = vec.at(10);
    ASSERT(!out.has_value());
}

TEST(secure_memory_operations) {
    using namespace memory_safety;

    // Secure compare (constant time)
    uint8_t data1[] = {1, 2, 3, 4};
    uint8_t data2[] = {1, 2, 3, 4};
    uint8_t data3[] = {1, 2, 3, 5};

    ASSERT(SafeMemory::secure_compare(data1, data2, 4));
    ASSERT(!SafeMemory::secure_compare(data1, data3, 4));

    // Secure clear
    uint8_t secret[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    SafeMemory::secure_clear(secret, sizeof(secret));
    bool all_zero = true;
    for (size_t i = 0; i < sizeof(secret); ++i) {
        if (secret[i] != 0) all_zero = false;
    }
    ASSERT(all_zero);
}

TEST(secure_memory_raii) {
    using namespace memory_safety;

    {
        SecureMemory<uint8_t> mem(100);
        ASSERT(mem.size() == 100);

        // Use the memory
        mem.set(0, 42);
        auto val = mem.at(0);
        ASSERT(val.has_value() && *val == 42);

        // Memory will be securely cleared on destruction
    }

    // Out of scope - memory should be cleared
}

TEST(alignment_helpers) {
    using namespace memory_safety;

    // Align up
    ASSERT(Alignment::align_up(10, 8) == 16);
    ASSERT(Alignment::align_up(16, 8) == 16);
    ASSERT(Alignment::align_up(17, 8) == 24);

    // Align down
    ASSERT(Alignment::align_down(10, 8) == 8);
    ASSERT(Alignment::align_down(16, 8) == 16);
    ASSERT(Alignment::align_down(17, 8) == 16);

    // Check alignment
    int aligned_var alignas(16);
    ASSERT(Alignment::is_aligned(&aligned_var, 16));
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "===========================================\n";
    std::cout << "INTcoin Security Features Test Suite\n";
    std::cout << "===========================================\n\n";

    // Input validation tests
    std::cout << "--- Input Validation Tests ---\n";
    run_test_string_length_validation();
    run_test_hex_validation();
    run_test_base58_validation();
    run_test_hostname_validation();
    run_test_amount_validation();
    run_test_binary_hash_validation();
    run_test_public_key_validation();
    run_test_ipv4_validation();

    // Safe math tests
    std::cout << "\n--- Safe Math Tests ---\n";
    run_test_safe_addition();
    run_test_safe_subtraction();
    run_test_safe_multiplication();
    run_test_safe_division();
    run_test_safe_type_casting();
    run_test_amount_operations();
    run_test_checked_arithmetic_class();
    run_test_saturation_arithmetic();

    // Memory safety tests
    std::cout << "\n--- Memory Safety Tests ---\n";
    run_test_safe_buffer();
    run_test_safe_string_operations();
    run_test_safe_array();
    run_test_bounded_vector();
    run_test_secure_memory_operations();
    run_test_secure_memory_raii();
    run_test_alignment_helpers();

    // Summary
    std::cout << "\n===========================================\n";
    std::cout << "Test Results:\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";
    std::cout << "  Total:  " << (tests_passed + tests_failed) << "\n";
    std::cout << "===========================================\n";

    return tests_failed == 0 ? 0 : 1;
}
