// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_TEST_FRAMEWORK_H
#define INTCOIN_TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>

namespace intcoin {
namespace test {

/**
 * Simple test framework for unit testing
 * Provides assertions, test registration, and reporting
 */

// Test result
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    double duration_ms;

    TestResult(const std::string& name, bool pass, const std::string& error = "", double duration = 0.0)
        : test_name(name), passed(pass), error_message(error), duration_ms(duration) {}
};

// Test case function
using TestFunction = std::function<void()>;

// Test suite
class TestSuite {
private:
    std::string suite_name_;
    std::vector<std::pair<std::string, TestFunction>> tests_;
    std::vector<TestResult> results_;

public:
    explicit TestSuite(const std::string& name) : suite_name_(name) {}

    /**
     * Register a test
     */
    void add_test(const std::string& name, TestFunction func) {
        tests_.push_back({name, func});
    }

    /**
     * Run all tests in suite
     */
    bool run() {
        std::cout << "\n=== Running Test Suite: " << suite_name_ << " ===\n" << std::endl;

        int passed = 0;
        int failed = 0;

        for (const auto& [name, func] : tests_) {
            auto start = std::chrono::high_resolution_clock::now();

            try {
                func();
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();

                results_.push_back(TestResult(name, true, "", duration));
                std::cout << "  ✓ " << name << " (" << duration << "ms)" << std::endl;
                passed++;
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();

                results_.push_back(TestResult(name, false, e.what(), duration));
                std::cout << "  ✗ " << name << " - " << e.what() << std::endl;
                failed++;
            }
        }

        std::cout << "\n" << suite_name_ << " Results: "
                  << passed << " passed, " << failed << " failed\n" << std::endl;

        return failed == 0;
    }

    /**
     * Get results
     */
    const std::vector<TestResult>& get_results() const {
        return results_;
    }
};

// Assertion exception
class AssertionError : public std::runtime_error {
public:
    explicit AssertionError(const std::string& msg) : std::runtime_error(msg) {}
};

// Assertion macros
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #condition \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (val_a != val_b) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " == " << #b \
                << " (" << val_a << " != " << val_b << ")" \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (val_a == val_b) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " != " << #b \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

#define ASSERT_GT(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (!(val_a > val_b)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " > " << #b \
                << " (" << val_a << " <= " << val_b << ")" \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

#define ASSERT_LT(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (!(val_a < val_b)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #a << " < " << #b \
                << " (" << val_a << " >= " << val_b << ")" \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

#define ASSERT_THROWS(statement, exception_type) \
    do { \
        bool threw = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            threw = true; \
        } catch (...) { \
        } \
        if (!threw) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #statement \
                << " did not throw " << #exception_type \
                << " at " << __FILE__ << ":" << __LINE__; \
            throw intcoin::test::AssertionError(oss.str()); \
        } \
    } while(0)

/**
 * Test runner - runs all registered test suites
 */
class TestRunner {
private:
    std::vector<TestSuite*> suites_;

public:
    /**
     * Register a test suite
     */
    void add_suite(TestSuite* suite) {
        suites_.push_back(suite);
    }

    /**
     * Run all test suites
     */
    bool run_all() {
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║   INTcoin Test Suite                  ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;

        int total_passed = 0;
        int total_failed = 0;
        int suites_passed = 0;
        int suites_failed = 0;

        for (auto* suite : suites_) {
            bool passed = suite->run();

            if (passed) {
                suites_passed++;
            } else {
                suites_failed++;
            }

            // Count individual tests
            for (const auto& result : suite->get_results()) {
                if (result.passed) {
                    total_passed++;
                } else {
                    total_failed++;
                }
            }
        }

        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║   Test Summary                         ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "Test Suites: " << suites_passed << " passed, "
                  << suites_failed << " failed" << std::endl;
        std::cout << "Tests:       " << total_passed << " passed, "
                  << total_failed << " failed" << std::endl;

        return suites_failed == 0;
    }
};

/**
 * Mock blockchain for testing
 */
class MockBlockchain {
private:
    std::vector<class Block> blocks_;
    std::unordered_map<Hash256, class Block> block_map_;

public:
    void add_block(const class Block& block);
    class Block* get_block(const Hash256& hash);
    size_t get_height() const { return blocks_.size(); }
    void clear() { blocks_.clear(); block_map_.clear(); }
};

/**
 * Mock network for testing P2P
 */
class MockNetwork {
private:
    std::vector<std::pair<std::string, std::vector<uint8_t>>> sent_messages_;

public:
    void send_message(const std::string& peer, const std::vector<uint8_t>& message) {
        sent_messages_.push_back({peer, message});
    }

    size_t get_message_count() const {
        return sent_messages_.size();
    }

    void clear() {
        sent_messages_.clear();
    }
};

/**
 * Test utilities
 */
class TestUtils {
public:
    /**
     * Generate random bytes
     */
    static std::vector<uint8_t> random_bytes(size_t count);

    /**
     * Generate random hash
     */
    static Hash256 random_hash();

    /**
     * Create dummy block
     */
    static class Block create_dummy_block(uint32_t height = 0);

    /**
     * Create dummy transaction
     */
    static class Transaction create_dummy_transaction();

    /**
     * Measure execution time
     */
    template<typename Func>
    static double measure_time_ms(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

} // namespace test
} // namespace intcoin

#endif // INTCOIN_TEST_FRAMEWORK_H
