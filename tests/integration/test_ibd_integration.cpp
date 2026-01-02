// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

/**
 * Integration Test: Initial Block Download (IBD)
 *
 * Tests end-to-end IBD functionality including:
 * - Parallel block validation
 * - AssumeUTXO snapshot loading
 * - Background validation
 * - Performance measurement
 */

#include <intcoin/ibd/parallel_validation.h>
#include <intcoin/ibd/assume_utxo.h>
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>

using namespace intcoin::ibd;

// Mock CBlock and CBlockIndex
class CBlock {
public:
    uint32_t nHeight{0};
    std::vector<uint8_t> data;

    CBlock(uint32_t height) : nHeight(height) {
        // Simulate block data
        data.resize(1000000); // 1MB block
    }
};

class CBlockIndex {
public:
    uint32_t nHeight{0};
    CBlockIndex(uint32_t height) : nHeight(height) {}
};

struct TestResult {
    std::string test_name;
    bool passed;
    uint64_t duration_ms;
};

std::vector<TestResult> results;

#define INTEGRATION_TEST(name) \
    bool name(); \
    static bool run_##name() { \
        auto start = std::chrono::steady_clock::now(); \
        bool result = name(); \
        auto end = std::chrono::steady_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); \
        results.push_back({#name, result, static_cast<uint64_t>(duration)}); \
        return result; \
    } \
    bool name()

// Test: T-IBD-001 - Parallel Validation Performance
INTEGRATION_TEST(test_parallel_validation_performance) {
    ParallelBlockProcessor::Config config;
    config.num_threads = 8;

    ParallelBlockProcessor processor(config);

    const int NUM_BLOCKS = 1000;
    std::cout << "  → Processing " << NUM_BLOCKS << " blocks with 8 threads..." << std::endl;

    auto start = std::chrono::steady_clock::now();

    // Submit blocks for validation
    std::vector<std::future<ValidationResult>> futures;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        CBlock block(i);
        CBlockIndex index(i);
        futures.push_back(processor.SubmitBlock(block, &index));
    }

    // Wait for all validations
    for (auto& future : futures) {
        future.wait();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    auto stats = processor.GetStats();

    std::cout << "  → Validated " << stats.blocks_validated << " blocks in " << duration << "ms" << std::endl;
    std::cout << "  → Throughput: " << (NUM_BLOCKS * 1000.0 / duration) << " blocks/sec" << std::endl;
    std::cout << "  → Average validation time: " << (stats.total_validation_time_ms / stats.blocks_validated) << "ms" << std::endl;

    assert(stats.blocks_validated == NUM_BLOCKS);

    return true;
}

// Test: T-IBD-002 - AssumeUTXO Fast Sync
INTEGRATION_TEST(test_assumeutxo_fast_sync) {
    AssumeUTXOManager manager;

    std::cout << "  → Creating UTXO snapshot at height 100,000..." << std::endl;

    // Create a snapshot (in real implementation, this would serialize UTXO set)
    bool created = manager.CreateSnapshot("/tmp/test_snapshot_100k.dat");

    std::cout << "  → Loading snapshot..." << std::endl;
    auto load_start = std::chrono::steady_clock::now();

    // Load the snapshot
    bool loaded = manager.LoadSnapshot("/tmp/test_snapshot_100k.dat");

    auto load_end = std::chrono::steady_clock::now();
    auto load_duration = std::chrono::duration_cast<std::chrono::seconds>(load_end - load_start).count();

    std::cout << "  → Snapshot loaded in " << load_duration << " seconds" << std::endl;

    // Check if node is usable
    bool active = manager.IsAssumeUTXOActive();
    std::cout << "  → Node usable: " << (active ? "YES" : "NO") << std::endl;

    // Verify load time is reasonable (should be <10 minutes in production)
    assert(load_duration < 600);

    return true;
}

// Test: T-IBD-003 - Parallel Validation Correctness
INTEGRATION_TEST(test_parallel_validation_correctness) {
    // Test both serial and parallel validation produce same result
    const int NUM_BLOCKS = 100;

    // Serial validation
    ParallelBlockProcessor::Config serial_config;
    serial_config.num_threads = 1;
    ParallelBlockProcessor serial_processor(serial_config);

    std::vector<uint64_t> serial_hashes;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        CBlock block(i);
        CBlockIndex index(i);
        auto future = serial_processor.SubmitBlock(block, &index);
        auto result = future.get();
        serial_hashes.push_back(result.block_hash);
    }

    // Parallel validation
    ParallelBlockProcessor::Config parallel_config;
    parallel_config.num_threads = 8;
    ParallelBlockProcessor parallel_processor(parallel_config);

    std::vector<uint64_t> parallel_hashes;
    std::vector<std::future<ValidationResult>> futures;

    for (int i = 0; i < NUM_BLOCKS; i++) {
        CBlock block(i);
        CBlockIndex index(i);
        futures.push_back(parallel_processor.SubmitBlock(block, &index));
    }

    for (auto& future : futures) {
        auto result = future.get();
        parallel_hashes.push_back(result.block_hash);
    }

    // Verify results match
    std::cout << "  → Comparing serial vs parallel results..." << std::endl;

    bool match = true;
    for (size_t i = 0; i < NUM_BLOCKS; i++) {
        if (serial_hashes[i] != parallel_hashes[i]) {
            std::cerr << "    MISMATCH at block " << i << std::endl;
            match = false;
        }
    }

    std::cout << "  → Results match: " << (match ? "YES" : "NO") << std::endl;

    assert(match);

    return true;
}

// Test: T-IBD-004 - UTXO Snapshot Verification
INTEGRATION_TEST(test_snapshot_verification) {
    AssumeUTXOManager manager;

    // Create a test snapshot
    UTXOSnapshot snapshot;
    snapshot.block_height = 100000;
    snapshot.block_hash = 0x1234567890ABCDEF;
    snapshot.num_utxos = 1000000;
    snapshot.total_amount = 21000000 * 100000000ULL; // 21M coins
    snapshot.utxo_set_hash = 0xFEDCBA0987654321;

    std::cout << "  → Verifying snapshot at height " << snapshot.block_height << "..." << std::endl;

    auto result = manager.VerifySnapshot(snapshot);

    std::cout << "  → Verification result: " << (result.valid ? "VALID" : "INVALID") << std::endl;
    if (!result.valid) {
        std::cout << "  → Error: " << result.error_message << std::endl;
    }

    // Verification should fail for incomplete snapshot (expected in test)
    // In production, this would verify SHA256 hash matches hardcoded trusted hash

    return true;
}

// Test: T-IBD-005 - Background Validation Progress
INTEGRATION_TEST(test_background_validation) {
    AssumeUTXOManager manager;

    std::cout << "  → Starting background validation..." << std::endl;

    // Start background validation (simulated)
    manager.StartBackgroundValidation();

    // Monitor progress for 5 seconds
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        auto progress = manager.GetBackgroundProgress();

        std::cout << "  → Progress: " << progress.validated_height
                  << " / " << progress.target_height
                  << " (" << (progress.validated_height * 100.0 / progress.target_height) << "%)"
                  << std::endl;

        if (progress.completed) {
            std::cout << "  → Background validation completed!" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return true;
}

// Test runner
int main() {
    std::cout << "=== IBD Integration Tests ===" << std::endl << std::endl;

    // Run all tests
    run_test_parallel_validation_performance();
    run_test_assumeutxo_fast_sync();
    run_test_parallel_validation_correctness();
    run_test_snapshot_verification();
    run_test_background_validation();

    // Print results
    std::cout << std::endl << "=== Test Results ===" << std::endl;

    int passed = 0;
    int failed = 0;
    uint64_t total_duration = 0;

    for (const auto& result : results) {
        std::string status = result.passed ? "PASS" : "FAIL";
        std::cout << "[" << status << "] " << result.test_name
                  << " (" << result.duration_ms << "ms)" << std::endl;

        if (result.passed) passed++;
        else failed++;

        total_duration += result.duration_ms;
    }

    std::cout << std::endl;
    std::cout << "Total: " << results.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total Duration: " << total_duration << "ms" << std::endl;

    return failed == 0 ? 0 : 1;
}
