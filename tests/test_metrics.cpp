/**
 * @file test_metrics.cpp
 * @brief Metrics and monitoring test suite for INTcoin
 * @author INTcoin Core Developers
 * @date 2026-01-01
 * @version 1.2.0-beta
 */

#include "intcoin/metrics.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace intcoin;

// Test 1: Counter basic operations
void TestCounter() {
    std::cout << "Test 1: Counter Operations..." << std::endl;

    Counter counter("test_counter", "Test counter metric");

    assert(counter.Value() == 0.0);

    counter.Inc();
    assert(counter.Value() == 1.0);

    counter.Add(5.5);
    assert(counter.Value() == 6.5);

    // Test that negative values don't decrease counter
    counter.Add(-3.0);
    assert(counter.Value() == 6.5);

    std::cout << "✓ Counter operations working correctly" << std::endl;
}

// Test 2: Gauge basic operations
void TestGauge() {
    std::cout << "\nTest 2: Gauge Operations..." << std::endl;

    Gauge gauge("test_gauge", "Test gauge metric");

    assert(gauge.Value() == 0.0);

    gauge.Set(42.5);
    assert(gauge.Value() == 42.5);

    gauge.Inc();
    assert(gauge.Value() == 43.5);

    gauge.Dec();
    assert(gauge.Value() == 42.5);

    gauge.Add(10.0);
    assert(gauge.Value() == 52.5);

    gauge.Sub(20.0);
    assert(gauge.Value() == 32.5);

    std::cout << "✓ Gauge operations working correctly" << std::endl;
}

// Test 3: Histogram basic operations
void TestHistogram() {
    std::cout << "\nTest 3: Histogram Operations..." << std::endl;

    std::vector<double> buckets = {1, 5, 10, 50, 100};
    Histogram histogram("test_histogram", "Test histogram metric", buckets);

    assert(histogram.Count() == 0);
    assert(histogram.Sum() == 0.0);

    histogram.Observe(3.0);
    assert(histogram.Count() == 1);
    assert(histogram.Sum() == 3.0);

    histogram.Observe(25.0);
    assert(histogram.Count() == 2);
    assert(histogram.Sum() == 28.0);

    histogram.Observe(150.0);
    assert(histogram.Count() == 3);
    assert(histogram.Sum() == 178.0);

    std::cout << "✓ Histogram operations working correctly" << std::endl;
}

// Test 4: Timer functionality
void TestTimer() {
    std::cout << "\nTest 4: Timer Functionality..." << std::endl;

    std::vector<double> buckets = {10, 50, 100, 500, 1000};
    Histogram histogram("test_timer_histogram", "Test timer histogram", buckets);

    {
        Timer timer(histogram);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    assert(histogram.Count() == 1);
    assert(histogram.Sum() >= 10.0);  // Should be at least 10ms

    std::cout << "✓ Timer functionality working correctly" << std::endl;
}

// Test 5: Prometheus export format
void TestPrometheusExport() {
    std::cout << "\nTest 5: Prometheus Export..." << std::endl;

    Counter counter("export_test_counter", "Export test counter");
    counter.Add(123.45);

    std::string export_str = counter.ExportPrometheus();
    assert(export_str.find("# HELP export_test_counter") != std::string::npos);
    assert(export_str.find("# TYPE export_test_counter counter") != std::string::npos);
    assert(export_str.find("export_test_counter 123.45") != std::string::npos);

    Gauge gauge("export_test_gauge", "Export test gauge");
    gauge.Set(98.76);

    export_str = gauge.ExportPrometheus();
    assert(export_str.find("# HELP export_test_gauge") != std::string::npos);
    assert(export_str.find("# TYPE export_test_gauge gauge") != std::string::npos);
    assert(export_str.find("export_test_gauge 98.76") != std::string::npos);

    std::cout << "✓ Prometheus export format correct" << std::endl;
}

// Test 6: Metrics registry
void TestMetricsRegistry() {
    std::cout << "\nTest 6: Metrics Registry..." << std::endl;

    MetricsRegistry& registry = MetricsRegistry::Instance();
    // Note: Don't clear registry as it would delete standard metrics used by other tests

    Counter& counter = registry.RegisterCounter("reg_counter", "Registry counter");
    Gauge& gauge = registry.RegisterGauge("reg_gauge", "Registry gauge");

    counter.Add(10);
    gauge.Set(20);

    // Test retrieval
    Counter* retrieved_counter = registry.GetCounter("reg_counter");
    assert(retrieved_counter != nullptr);
    assert(retrieved_counter->Value() == 10.0);

    Gauge* retrieved_gauge = registry.GetGauge("reg_gauge");
    assert(retrieved_gauge != nullptr);
    assert(retrieved_gauge->Value() == 20.0);

    // Test export
    std::string export_str = registry.ExportPrometheus();
    assert(export_str.find("reg_counter") != std::string::npos);
    assert(export_str.find("reg_gauge") != std::string::npos);

    std::cout << "✓ Metrics registry working correctly" << std::endl;
}

// Test 7: Standard blockchain metrics
void TestStandardMetrics() {
    std::cout << "\nTest 7: Standard Blockchain Metrics..." << std::endl;

    using namespace metrics;

    // Test blockchain metrics
    blocks_processed.Inc();
    transactions_processed.Add(5);
    blockchain_height.Set(12345);
    blockchain_difficulty.Set(1000000.0);

    assert(blocks_processed.Value() >= 1.0);
    assert(transactions_processed.Value() >= 5.0);
    assert(blockchain_height.Value() == 12345.0);
    assert(blockchain_difficulty.Value() == 1000000.0);

    // Test mempool metrics
    mempool_size.Set(42);
    mempool_bytes.Set(128000);
    mempool_accepted.Inc();

    assert(mempool_size.Value() == 42.0);
    assert(mempool_bytes.Value() == 128000.0);
    assert(mempool_accepted.Value() >= 1.0);

    // Test network metrics
    peer_count.Set(8);
    bytes_sent.Add(1024);
    bytes_received.Add(2048);

    assert(peer_count.Value() == 8.0);
    assert(bytes_sent.Value() >= 1024.0);
    assert(bytes_received.Value() >= 2048.0);

    std::cout << "✓ Standard blockchain metrics working correctly" << std::endl;
}

// Test 8: Histogram bucket distribution
void TestHistogramBuckets() {
    std::cout << "\nTest 8: Histogram Bucket Distribution..." << std::endl;

    std::vector<double> buckets = {10, 20, 30, 40, 50};
    Histogram histogram("bucket_test", "Bucket test", buckets);

    // Observe values in different buckets
    histogram.Observe(5);   // Bucket: 10
    histogram.Observe(15);  // Bucket: 20
    histogram.Observe(25);  // Bucket: 30
    histogram.Observe(35);  // Bucket: 40
    histogram.Observe(45);  // Bucket: 50
    histogram.Observe(100); // Bucket: +Inf

    assert(histogram.Count() == 6);
    assert(histogram.Sum() == 225.0);

    std::string export_str = histogram.ExportPrometheus();

    // Debug: print export to see format
    std::cout << "  Export format:\n" << export_str << std::endl;

    assert(export_str.find("bucket_test_bucket{le=\"10.00\"}") != std::string::npos);
    assert(export_str.find("bucket_test_bucket{le=\"+Inf\"}") != std::string::npos);
    assert(export_str.find("bucket_test_sum 225.00") != std::string::npos);
    assert(export_str.find("bucket_test_count 6") != std::string::npos);

    std::cout << "✓ Histogram bucket distribution correct" << std::endl;
}

// Test 9: Thread safety
void TestThreadSafety() {
    std::cout << "\nTest 9: Thread Safety..." << std::endl;

    Counter counter("thread_test_counter", "Thread safe counter");
    Gauge gauge("thread_test_gauge", "Thread safe gauge");

    std::vector<std::thread> threads;
    const int num_threads = 10;
    const int increments_per_thread = 100;

    // Test counter thread safety
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.Inc();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    assert(counter.Value() == num_threads * increments_per_thread);

    std::cout << "✓ Thread safety verified" << std::endl;
}

// Test 10: Complete export test
void TestCompleteExport() {
    std::cout << "\nTest 10: Complete Prometheus Export..." << std::endl;

    MetricsRegistry& registry = MetricsRegistry::Instance();

    // Export all metrics
    std::string complete_export = registry.ExportPrometheus();

    // Should contain standard metrics
    assert(!complete_export.empty());

    // Should have proper Prometheus format
    assert(complete_export.find("# HELP") != std::string::npos);
    assert(complete_export.find("# TYPE") != std::string::npos);

    std::cout << "✓ Complete Prometheus export successful" << std::endl;
    std::cout << "\n--- Sample Prometheus Export ---\n";
    std::cout << complete_export.substr(0, std::min(500UL, complete_export.size()));
    std::cout << "\n... (truncated) ...\n";
}

// Main test runner
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "INTcoin Metrics & Monitoring Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        TestCounter();
        TestGauge();
        TestHistogram();
        TestTimer();
        TestPrometheusExport();
        TestMetricsRegistry();
        TestStandardMetrics();
        TestHistogramBuckets();
        TestThreadSafety();
        TestCompleteExport();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All metrics tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
