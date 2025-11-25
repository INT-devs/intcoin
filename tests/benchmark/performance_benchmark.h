// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "benchmark.h"
#include <iostream>
#include <fstream>

namespace intcoin {
namespace benchmark {

/**
 * Performance benchmarking for core components
 */
class PerformanceBenchmark {
public:
    /**
     * Run cryptocurrency operation benchmarks
     */
    static void run_crypto_benchmarks();

    /**
     * Run transaction processing benchmarks
     */
    static void run_transaction_benchmarks();

    /**
     * Run mining benchmarks
     */
    static void run_mining_benchmarks();

    /**
     * Run smart contract benchmarks
     */
    static void run_contract_benchmarks();

    /**
     * Run Lightning Network benchmarks
     */
    static void run_lightning_benchmarks();

    /**
     * Run bridge and cross-chain benchmarks
     */
    static void run_bridge_benchmarks();

    /**
     * Run network benchmarks
     */
    static void run_network_benchmarks();

    /**
     * Run memory footprint benchmarks
     */
    static void run_memory_benchmarks();

    /**
     * Run all benchmarks
     */
    static void run_all();
};

// Benchmark Results Storage
struct BenchmarkReport {
    std::string timestamp;
    std::vector<BenchmarkResult> crypto_results;
    std::vector<BenchmarkResult> transaction_results;
    std::vector<BenchmarkResult> mining_results;
    std::vector<BenchmarkResult> contract_results;
    std::vector<BenchmarkResult> lightning_results;
    std::vector<BenchmarkResult> bridge_results;
    std::vector<BenchmarkResult> network_results;
    std::vector<BenchmarkResult> memory_results;

    /**
     * Export report to JSON
     */
    void export_json(const std::string& filename);

    /**
     * Export report to CSV
     */
    void export_csv(const std::string& filename);

    /**
     * Print summary
     */
    void print_summary() const;
};

} // namespace benchmark
} // namespace intcoin
