// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BENCHMARK_H
#define INTCOIN_BENCHMARK_H

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace intcoin {
namespace benchmark {

/**
 * Benchmark result
 */
struct BenchmarkResult {
    std::string name;
    size_t iterations;
    double total_time_ms;
    double mean_time_ms;
    double median_time_ms;
    double min_time_ms;
    double max_time_ms;
    double stddev_ms;
    double ops_per_second;

    void print() const {
        std::cout << "\nBenchmark: " << name << std::endl;
        std::cout << "  Iterations:  " << iterations << std::endl;
        std::cout << "  Total time:  " << std::fixed << std::setprecision(2)
                  << total_time_ms << " ms" << std::endl;
        std::cout << "  Mean:        " << mean_time_ms << " ms" << std::endl;
        std::cout << "  Median:      " << median_time_ms << " ms" << std::endl;
        std::cout << "  Min:         " << min_time_ms << " ms" << std::endl;
        std::cout << "  Max:         " << max_time_ms << " ms" << std::endl;
        std::cout << "  Std Dev:     " << stddev_ms << " ms" << std::endl;
        std::cout << "  Throughput:  " << std::setprecision(0)
                  << ops_per_second << " ops/sec" << std::endl;
    }
};

/**
 * Benchmark runner
 */
class Benchmark {
private:
    std::string name_;
    size_t iterations_;
    size_t warmup_iterations_;
    std::vector<double> timings_;

public:
    Benchmark(const std::string& name, size_t iterations = 1000, size_t warmup = 100)
        : name_(name), iterations_(iterations), warmup_iterations_(warmup) {
        timings_.reserve(iterations);
    }

    /**
     * Run benchmark
     */
    template<typename Func>
    BenchmarkResult run(Func func) {
        // Warmup
        for (size_t i = 0; i < warmup_iterations_; ++i) {
            func();
        }

        // Actual benchmark
        timings_.clear();
        for (size_t i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();

            double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            timings_.push_back(duration_ms);
        }

        return calculate_statistics();
    }

    /**
     * Run benchmark with parameter
     */
    template<typename Func, typename Param>
    BenchmarkResult run_with_param(Func func, Param param) {
        return run([&]() { func(param); });
    }

private:
    BenchmarkResult calculate_statistics() {
        BenchmarkResult result;
        result.name = name_;
        result.iterations = iterations_;

        // Total time
        result.total_time_ms = std::accumulate(timings_.begin(), timings_.end(), 0.0);

        // Mean
        result.mean_time_ms = result.total_time_ms / iterations_;

        // Median
        std::vector<double> sorted_timings = timings_;
        std::sort(sorted_timings.begin(), sorted_timings.end());
        result.median_time_ms = sorted_timings[iterations_ / 2];

        // Min/Max
        result.min_time_ms = *std::min_element(timings_.begin(), timings_.end());
        result.max_time_ms = *std::max_element(timings_.begin(), timings_.end());

        // Standard deviation
        double variance = 0.0;
        for (double time : timings_) {
            variance += (time - result.mean_time_ms) * (time - result.mean_time_ms);
        }
        result.stddev_ms = std::sqrt(variance / iterations_);

        // Operations per second
        result.ops_per_second = 1000.0 / result.mean_time_ms;

        return result;
    }
};

/**
 * Benchmark suite
 */
class BenchmarkSuite {
private:
    std::string suite_name_;
    std::vector<BenchmarkResult> results_;

public:
    explicit BenchmarkSuite(const std::string& name) : suite_name_(name) {}

    /**
     * Add benchmark result
     */
    void add_result(const BenchmarkResult& result) {
        results_.push_back(result);
    }

    /**
     * Print all results
     */
    void print_results() {
        std::cout << "\n╔════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Benchmark Suite: " << std::setw(31) << std::left
                  << suite_name_ << "║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;

        for (const auto& result : results_) {
            result.print();
        }

        std::cout << "\nSummary: " << results_.size() << " benchmarks completed" << std::endl;
    }

    /**
     * Export results to CSV
     */
    void export_csv(const std::string& filename) {
        std::ofstream file(filename);
        file << "Name,Iterations,TotalTime(ms),Mean(ms),Median(ms),Min(ms),Max(ms),StdDev(ms),Throughput(ops/s)\n";

        for (const auto& result : results_) {
            file << result.name << ","
                 << result.iterations << ","
                 << result.total_time_ms << ","
                 << result.mean_time_ms << ","
                 << result.median_time_ms << ","
                 << result.min_time_ms << ","
                 << result.max_time_ms << ","
                 << result.stddev_ms << ","
                 << result.ops_per_second << "\n";
        }
    }
};

/**
 * Memory usage tracker
 */
class MemoryTracker {
private:
    size_t initial_memory_;

public:
    MemoryTracker();

    /**
     * Get current memory usage
     */
    size_t get_current_memory() const;

    /**
     * Get memory delta since construction
     */
    size_t get_memory_delta() const {
        return get_current_memory() - initial_memory_;
    }

    /**
     * Print memory usage
     */
    void print() const {
        size_t current = get_current_memory();
        size_t delta = current - initial_memory_;

        std::cout << "Memory Usage:" << std::endl;
        std::cout << "  Current: " << (current / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  Delta:   " << (delta / 1024 / 1024) << " MB" << std::endl;
    }
};

/**
 * Throughput benchmark
 * Measures operations per second
 */
class ThroughputBenchmark {
private:
    std::string name_;
    size_t target_duration_ms_;

public:
    ThroughputBenchmark(const std::string& name, size_t duration_ms = 5000)
        : name_(name), target_duration_ms_(duration_ms) {}

    /**
     * Run throughput test
     */
    template<typename Func>
    double measure(Func func) {
        size_t iterations = 0;
        auto start = std::chrono::high_resolution_clock::now();
        auto end_time = start + std::chrono::milliseconds(target_duration_ms_);

        while (std::chrono::high_resolution_clock::now() < end_time) {
            func();
            iterations++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        double throughput = (iterations * 1000.0) / duration_ms;

        std::cout << name_ << ": " << std::fixed << std::setprecision(0)
                  << throughput << " ops/sec (" << iterations
                  << " iterations in " << std::setprecision(2)
                  << duration_ms << " ms)" << std::endl;

        return throughput;
    }
};

/**
 * Scalability benchmark
 * Tests performance with increasing workload
 */
class ScalabilityBenchmark {
private:
    std::string name_;
    std::vector<size_t> workload_sizes_;

public:
    ScalabilityBenchmark(const std::string& name, const std::vector<size_t>& sizes)
        : name_(name), workload_sizes_(sizes) {}

    /**
     * Run scalability test
     */
    template<typename Func>
    void measure(Func func) {
        std::cout << "\nScalability Benchmark: " << name_ << std::endl;
        std::cout << std::setw(15) << "Workload"
                  << std::setw(15) << "Time(ms)"
                  << std::setw(15) << "Throughput" << std::endl;
        std::cout << std::string(45, '-') << std::endl;

        for (size_t size : workload_sizes_) {
            auto start = std::chrono::high_resolution_clock::now();
            func(size);
            auto end = std::chrono::high_resolution_clock::now();

            double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            double throughput = (size * 1000.0) / duration_ms;

            std::cout << std::setw(15) << size
                      << std::setw(15) << std::fixed << std::setprecision(2) << duration_ms
                      << std::setw(15) << std::setprecision(0) << throughput << std::endl;
        }
    }
};

} // namespace benchmark
} // namespace intcoin

#endif // INTCOIN_BENCHMARK_H
