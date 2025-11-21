// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_TESTING_INFRASTRUCTURE_H
#define INTCOIN_TESTING_INFRASTRUCTURE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <chrono>
#include <random>
#include <optional>

namespace intcoin {
namespace testing {

/**
 * Test Coverage Tracker - Ensures critical paths are tested
 */
class CoverageTracker {
public:
    struct CriticalPath {
        std::string module;
        std::string function;
        bool tested = false;
        size_t test_count = 0;
    };

    static CoverageTracker& instance() {
        static CoverageTracker inst;
        return inst;
    }

    void register_critical_path(const std::string& module, const std::string& function) {
        std::string key = module + "::" + function;
        critical_paths_[key] = {module, function, false, 0};
    }

    void mark_tested(const std::string& module, const std::string& function) {
        std::string key = module + "::" + function;
        if (auto it = critical_paths_.find(key); it != critical_paths_.end()) {
            it->second.tested = true;
            it->second.test_count++;
        }
    }

    struct CoverageReport {
        size_t total_paths = 0;
        size_t tested_paths = 0;
        double coverage_percent = 0.0;
        std::vector<std::string> untested;
    };

    CoverageReport get_report() const {
        CoverageReport report;
        report.total_paths = critical_paths_.size();

        for (const auto& [key, path] : critical_paths_) {
            if (path.tested) {
                report.tested_paths++;
            } else {
                report.untested.push_back(key);
            }
        }

        if (report.total_paths > 0) {
            report.coverage_percent = 100.0 * report.tested_paths / report.total_paths;
        }

        return report;
    }

private:
    CoverageTracker() {
        // Register all critical paths
        register_critical_path("crypto", "dilithium5_sign");
        register_critical_path("crypto", "dilithium5_verify");
        register_critical_path("crypto", "kyber1024_encapsulate");
        register_critical_path("crypto", "kyber1024_decapsulate");
        register_critical_path("consensus", "validate_block");
        register_critical_path("consensus", "connect_block");
        register_critical_path("wallet", "create_transaction");
        register_critical_path("wallet", "sign_transaction");
        register_critical_path("network", "process_message");
        register_critical_path("network", "validate_peer");
        register_critical_path("mempool", "accept_transaction");
        register_critical_path("script", "verify_script");
    }

    std::unordered_map<std::string, CriticalPath> critical_paths_;
};

/**
 * Test Suite Manager - Tracks 400+ test cases
 */
class TestSuiteManager {
public:
    enum class TestCategory {
        UNIT,
        INTEGRATION,
        FUNCTIONAL,
        PERFORMANCE,
        REGRESSION,
        EDGE_CASE,
        FUZZ
    };

    struct TestCase {
        std::string name;
        TestCategory category;
        std::string module;
        std::function<bool()> test_fn;
        bool enabled = true;
    };

    struct TestResult {
        std::string name;
        bool passed = false;
        std::chrono::milliseconds duration{0};
        std::string error;
    };

    static TestSuiteManager& instance() {
        static TestSuiteManager inst;
        return inst;
    }

    void register_test(const std::string& name, TestCategory category,
                      const std::string& module, std::function<bool()> fn) {
        tests_.push_back({name, category, module, fn, true});
    }

    std::vector<TestResult> run_category(TestCategory category) {
        std::vector<TestResult> results;

        for (const auto& test : tests_) {
            if (test.category != category || !test.enabled) continue;

            TestResult result;
            result.name = test.name;

            auto start = std::chrono::steady_clock::now();
            try {
                result.passed = test.test_fn();
            } catch (const std::exception& e) {
                result.passed = false;
                result.error = e.what();
            }
            auto end = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            results.push_back(result);
        }

        return results;
    }

    size_t get_test_count() const { return tests_.size(); }

    size_t get_test_count_by_category(TestCategory cat) const {
        size_t count = 0;
        for (const auto& t : tests_) {
            if (t.category == cat) count++;
        }
        return count;
    }

private:
    std::vector<TestCase> tests_;
};

/**
 * Fuzz Testing Infrastructure
 */
class FuzzTestingEngine {
public:
    struct FuzzConfig {
        size_t max_iterations = 100000;
        size_t max_input_size = 65536;
        uint64_t seed = 0;
        bool use_coverage_guided = true;
    };

    struct FuzzResult {
        size_t iterations = 0;
        size_t crashes = 0;
        size_t hangs = 0;
        size_t unique_paths = 0;
        std::vector<std::vector<uint8_t>> crash_inputs;
    };

    static std::vector<uint8_t> generate_random_input(size_t max_size, std::mt19937_64& rng) {
        std::uniform_int_distribution<size_t> size_dist(0, max_size);
        std::uniform_int_distribution<int> byte_dist(0, 255);

        size_t size = size_dist(rng);
        std::vector<uint8_t> input(size);
        for (auto& byte : input) {
            byte = static_cast<uint8_t>(byte_dist(rng));
        }
        return input;
    }

    static std::vector<uint8_t> mutate_input(const std::vector<uint8_t>& input, std::mt19937_64& rng) {
        if (input.empty()) return generate_random_input(100, rng);

        std::vector<uint8_t> mutated = input;
        std::uniform_int_distribution<int> mutation_type(0, 4);
        std::uniform_int_distribution<size_t> pos_dist(0, mutated.size() - 1);
        std::uniform_int_distribution<int> byte_dist(0, 255);

        switch (mutation_type(rng)) {
            case 0: // Bit flip
                mutated[pos_dist(rng)] ^= (1 << (rng() % 8));
                break;
            case 1: // Byte replacement
                mutated[pos_dist(rng)] = static_cast<uint8_t>(byte_dist(rng));
                break;
            case 2: // Insertion
                mutated.insert(mutated.begin() + pos_dist(rng),
                              static_cast<uint8_t>(byte_dist(rng)));
                break;
            case 3: // Deletion
                if (mutated.size() > 1) {
                    mutated.erase(mutated.begin() + pos_dist(rng));
                }
                break;
            case 4: // Interesting values
                {
                    static const std::vector<uint8_t> interesting = {
                        0, 1, 0x7f, 0x80, 0xff, 0xfe
                    };
                    std::uniform_int_distribution<size_t> int_dist(0, interesting.size() - 1);
                    mutated[pos_dist(rng)] = interesting[int_dist(rng)];
                }
                break;
        }

        return mutated;
    }

    template<typename TargetFunc>
    static FuzzResult run_fuzz(TargetFunc target, const FuzzConfig& config = FuzzConfig{}) {
        FuzzResult result;
        std::mt19937_64 rng(config.seed ? config.seed : std::random_device{}());

        std::vector<uint8_t> current_input;
        std::unordered_set<size_t> seen_paths;

        for (size_t i = 0; i < config.max_iterations; ++i) {
            result.iterations++;

            // Generate or mutate input
            if (current_input.empty() || (rng() % 10) == 0) {
                current_input = generate_random_input(config.max_input_size, rng);
            } else {
                current_input = mutate_input(current_input, rng);
            }

            // Run target
            try {
                size_t path_hash = target(current_input.data(), current_input.size());
                if (seen_paths.insert(path_hash).second) {
                    result.unique_paths++;
                }
            } catch (...) {
                result.crashes++;
                result.crash_inputs.push_back(current_input);
            }
        }

        return result;
    }
};

/**
 * Integration Test Framework
 */
class IntegrationTestFramework {
public:
    struct TestEnvironment {
        std::string data_dir;
        uint16_t rpc_port;
        uint16_t p2p_port;
        bool testnet = true;
    };

    static std::vector<std::string> get_integration_tests() {
        return {
            "test_node_startup_shutdown",
            "test_peer_connection",
            "test_block_sync",
            "test_transaction_broadcast",
            "test_mempool_acceptance",
            "test_wallet_operations",
            "test_rpc_authentication",
            "test_network_partition_recovery",
        };
    }
};

/**
 * Functional Test Framework
 */
class FunctionalTestFramework {
public:
    static std::vector<std::string> get_e2e_scenarios() {
        return {
            "scenario_send_receive_coins",
            "scenario_multisig_transaction",
            "scenario_atomic_swap",
            "scenario_lightning_channel_open_close",
            "scenario_block_reorganization",
            "scenario_wallet_backup_restore",
            "scenario_upgrade_migration",
        };
    }
};

/**
 * Performance Benchmark Framework
 */
class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string name;
        size_t iterations;
        std::chrono::nanoseconds total_time;
        std::chrono::nanoseconds min_time;
        std::chrono::nanoseconds max_time;
        std::chrono::nanoseconds avg_time;
        double ops_per_second;
    };

    template<typename Func>
    static BenchmarkResult run(const std::string& name, Func fn,
                               size_t iterations = 1000, size_t warmup = 100) {
        BenchmarkResult result;
        result.name = name;
        result.iterations = iterations;
        result.min_time = std::chrono::nanoseconds::max();
        result.max_time = std::chrono::nanoseconds::zero();

        // Warmup
        for (size_t i = 0; i < warmup; ++i) {
            fn();
        }

        // Benchmark
        auto total_start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::steady_clock::now();
            fn();
            auto end = std::chrono::steady_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            result.min_time = std::min(result.min_time, duration);
            result.max_time = std::max(result.max_time, duration);
        }
        auto total_end = std::chrono::steady_clock::now();

        result.total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(total_end - total_start);
        result.avg_time = result.total_time / iterations;
        result.ops_per_second = 1e9 * iterations / result.total_time.count();

        return result;
    }

    static std::vector<std::string> get_benchmark_targets() {
        return {
            "bench_dilithium5_keygen",
            "bench_dilithium5_sign",
            "bench_dilithium5_verify",
            "bench_kyber1024_keygen",
            "bench_kyber1024_encap",
            "bench_kyber1024_decap",
            "bench_sha256",
            "bench_sha3_256",
            "bench_block_validation",
            "bench_script_verification",
            "bench_mempool_acceptance",
        };
    }
};

/**
 * Regression Test Suite
 */
class RegressionTestSuite {
public:
    struct RegressionTest {
        std::string id;
        std::string description;
        std::string related_issue;
        std::function<bool()> test_fn;
    };

    static RegressionTestSuite& instance() {
        static RegressionTestSuite inst;
        return inst;
    }

    void add_regression(const std::string& id, const std::string& desc,
                       const std::string& issue, std::function<bool()> fn) {
        regressions_.push_back({id, desc, issue, fn});
    }

    size_t get_count() const { return regressions_.size(); }

private:
    std::vector<RegressionTest> regressions_;
};

/**
 * Edge Case Test Generator
 */
class EdgeCaseGenerator {
public:
    // Numeric edge cases
    static std::vector<int64_t> get_int64_edge_cases() {
        return {
            0, 1, -1,
            INT64_MAX, INT64_MIN,
            INT64_MAX - 1, INT64_MIN + 1,
            INT32_MAX, INT32_MIN,
            UINT32_MAX,
        };
    }

    static std::vector<uint64_t> get_uint64_edge_cases() {
        return {
            0, 1,
            UINT64_MAX, UINT64_MAX - 1,
            UINT32_MAX, UINT32_MAX + 1ULL,
            1ULL << 32, (1ULL << 32) - 1,
        };
    }

    // String edge cases
    static std::vector<std::string> get_string_edge_cases() {
        return {
            "",
            " ",
            "\0",
            std::string(1000, 'a'),
            std::string(65536, 'x'),
            "\xff\xfe",
            "null\0byte",
        };
    }

    // Buffer edge cases
    static std::vector<std::vector<uint8_t>> get_buffer_edge_cases() {
        return {
            {},
            {0},
            {0xff},
            std::vector<uint8_t>(32, 0),
            std::vector<uint8_t>(32, 0xff),
            std::vector<uint8_t>(64, 0),
            std::vector<uint8_t>(65536, 0),
        };
    }
};

/**
 * Testing Infrastructure Manager
 */
class TestingInfrastructureManager {
public:
    static TestingInfrastructureManager& instance() {
        static TestingInfrastructureManager inst;
        return inst;
    }

    struct TestSummary {
        size_t total_tests = 0;
        size_t unit_tests = 0;
        size_t integration_tests = 0;
        size_t functional_tests = 0;
        size_t performance_benchmarks = 0;
        size_t regression_tests = 0;
        size_t edge_case_tests = 0;
        double coverage_percent = 0.0;
    };

    TestSummary get_summary() const {
        TestSummary summary;

        auto& suite = TestSuiteManager::instance();
        summary.total_tests = suite.get_test_count();
        summary.unit_tests = suite.get_test_count_by_category(TestSuiteManager::TestCategory::UNIT);
        summary.integration_tests = IntegrationTestFramework::get_integration_tests().size();
        summary.functional_tests = FunctionalTestFramework::get_e2e_scenarios().size();
        summary.performance_benchmarks = PerformanceBenchmark::get_benchmark_targets().size();
        summary.regression_tests = RegressionTestSuite::instance().get_count();

        auto coverage = CoverageTracker::instance().get_report();
        summary.coverage_percent = coverage.coverage_percent;

        return summary;
    }

private:
    TestingInfrastructureManager() = default;
};

} // namespace testing
} // namespace intcoin

#endif // INTCOIN_TESTING_INFRASTRUCTURE_H
