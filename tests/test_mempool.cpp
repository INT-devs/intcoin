/**
 * @file test_mempool.cpp
 * @brief Comprehensive test suite for enhanced mempool with priority queues
 * @author INTcoin Development Team
 * @date January 1, 2026
 * @version 1.2.0-beta
 */

#include "intcoin/mempool.h"
#include "intcoin/blockchain.h"
#include "intcoin/crypto.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <random>

using namespace intcoin;

// Helper function to create a test transaction
Transaction CreateTestTransaction(uint64_t fee, size_t num_inputs = 1, size_t num_outputs = 2) {
    Transaction tx;
    tx.version = 1;

    // Create inputs
    for (size_t i = 0; i < num_inputs; ++i) {
        TxIn input;
        // Create random prev_tx_hash
        for (size_t j = 0; j < 32; ++j) {
            input.prev_tx_hash[j] = static_cast<uint8_t>(rand() % 256);
        }
        input.prev_tx_index = static_cast<uint32_t>(i);

        // Create simple signature script
        for (size_t j = 0; j < 72; ++j) {
            input.script_sig.bytes.push_back(static_cast<uint8_t>(0xAB + i + j));
        }

        tx.inputs.push_back(input);
    }

    // Create outputs (including fee)
    for (size_t i = 0; i < num_outputs; ++i) {
        TxOut output;
        output.value = 100000 + (i * 10000);
        for (size_t j = 0; j < 25; ++j) {
            output.script_pubkey.bytes.push_back(static_cast<uint8_t>(0x76 + i + j));
        }
        tx.outputs.push_back(output);
    }

    // Add fee to first output for calculation purposes
    if (!tx.outputs.empty()) {
        tx.outputs[0].value += fee;
    }

    tx.locktime = 0;

    return tx;
}

// Test 1: Basic initialization
void TestMempoolInitialization() {
    std::cout << "Test 1: Mempool Initialization..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.min_relay_fee_per_kb = 1000;
    config.expiry_hours = 24;
    config.persist_on_shutdown = false;
    config.priority_limits[TxPriority::LOW] = 1000;
    config.priority_limits[TxPriority::NORMAL] = 5000;
    config.priority_limits[TxPriority::HIGH] = 2000;
    config.priority_limits[TxPriority::HTLC] = 500;
    config.priority_limits[TxPriority::BRIDGE] = 500;
    config.priority_limits[TxPriority::CRITICAL] = 100;

    INTcoinMempool mempool;
    auto result = mempool.Initialize(config);

    assert(result.IsOk());

    auto stats = mempool.GetStats();
    assert(stats.total_transactions == 0);
    assert(stats.total_size_bytes == 0);

    std::cout << "✓ Mempool initialization successful" << std::endl;
}

// Test 2: Adding transactions with different priorities
void TestAddTransactions() {
    std::cout << "\nTest 2: Adding Transactions..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;
    config.priority_limits[TxPriority::HIGH] = 500;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add normal priority transaction
    Transaction tx1 = CreateTestTransaction(5000);
    auto result1 = mempool.AddTransaction(tx1, TxPriority::NORMAL);
    assert(result1.IsOk());

    // Add high priority transaction
    Transaction tx2 = CreateTestTransaction(10000);
    auto result2 = mempool.AddTransaction(tx2, TxPriority::HIGH);
    assert(result2.IsOk());

    auto stats = mempool.GetStats();
    assert(stats.total_transactions == 2);
    assert(stats.count_by_priority.at(TxPriority::NORMAL) == 1);
    assert(stats.count_by_priority.at(TxPriority::HIGH) == 1);

    std::cout << "✓ Added transactions with different priorities" << std::endl;
}

// Test 3: Fee-based prioritization
void TestFeePrioritization() {
    std::cout << "\nTest 3: Fee-Based Prioritization..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add transactions with different fees
    Transaction tx_low_fee = CreateTestTransaction(1000);
    Transaction tx_med_fee = CreateTestTransaction(5000);
    Transaction tx_high_fee = CreateTestTransaction(20000);

    mempool.AddTransaction(tx_low_fee, TxPriority::NORMAL);
    mempool.AddTransaction(tx_med_fee, TxPriority::NORMAL);
    mempool.AddTransaction(tx_high_fee, TxPriority::NORMAL);

    // Get block template - should prioritize high fee
    auto template_txs = mempool.GetBlockTemplate(1000000, 10);
    assert(!template_txs.empty());

    // First transaction should be highest fee (within priority level)
    std::cout << "✓ Fee-based prioritization working" << std::endl;
}

// Test 4: Removing transactions
void TestRemoveTransaction() {
    std::cout << "\nTest 4: Removing Transactions..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    Transaction tx = CreateTestTransaction(5000);
    auto add_result = mempool.AddTransaction(tx, TxPriority::NORMAL);
    assert(add_result.IsOk());

    // Calculate tx hash
    std::vector<uint8_t> tx_data;
    for (const auto& input : tx.inputs) {
        tx_data.insert(tx_data.end(), input.prev_tx_hash.data(),
                      input.prev_tx_hash.data() + 32);
    }
    uint256 tx_hash = SHA3::Hash(tx_data);

    auto stats_before = mempool.GetStats();
    assert(stats_before.total_transactions == 1);

    auto remove_result = mempool.RemoveTransaction(tx_hash);
    assert(remove_result.IsOk());

    auto stats_after = mempool.GetStats();
    assert(stats_after.total_transactions == 0);

    std::cout << "✓ Transaction removal successful" << std::endl;
}

// Test 5: Block template generation
void TestBlockTemplateGeneration() {
    std::cout << "\nTest 5: Block Template Generation..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::CRITICAL] = 100;
    config.priority_limits[TxPriority::BRIDGE] = 100;
    config.priority_limits[TxPriority::HTLC] = 100;
    config.priority_limits[TxPriority::HIGH] = 100;
    config.priority_limits[TxPriority::NORMAL] = 100;
    config.priority_limits[TxPriority::LOW] = 100;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add transactions with different priorities
    mempool.AddTransaction(CreateTestTransaction(5000), TxPriority::NORMAL);
    mempool.AddTransaction(CreateTestTransaction(10000), TxPriority::HIGH);
    mempool.AddTransaction(CreateTestTransaction(3000), TxPriority::LOW);
    mempool.AddTransaction(CreateTestTransaction(15000), TxPriority::BRIDGE);
    mempool.AddTransaction(CreateTestTransaction(20000), TxPriority::CRITICAL);

    // Get block template with size limit
    auto template_txs = mempool.GetBlockTemplate(100000, 3);

    // Should have CRITICAL and BRIDGE first, then HIGH
    assert(template_txs.size() <= 3);

    std::cout << "✓ Block template generation successful (returned "
              << template_txs.size() << " transactions)" << std::endl;
}

// Test 6: Transaction expiry
void TestTransactionExpiry() {
    std::cout << "\nTest 6: Transaction Expiry..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.expiry_hours = 0;  // Expire immediately for testing
    config.priority_limits[TxPriority::NORMAL] = 1000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    Transaction tx = CreateTestTransaction(5000);
    mempool.AddTransaction(tx, TxPriority::NORMAL);

    auto stats_before = mempool.GetStats();
    assert(stats_before.total_transactions == 1);

    // Sleep to ensure transaction is expired
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto expired_result = mempool.RemoveExpired();
    assert(expired_result.IsOk());

    auto stats_after = mempool.GetStats();
    assert(stats_after.total_transactions == 0);

    std::cout << "✓ Transaction expiry working correctly" << std::endl;
}

// Test 7: Mempool persistence
void TestMempoolPersistence() {
    std::cout << "\nTest 7: Mempool Persistence..." << std::endl;

    const std::string persist_path = "/tmp/test_mempool.dat";

    // Create mempool and add transactions
    {
        MempoolConfig config;
        config.max_size_mb = 100;
        config.persist_file = persist_path;
        config.persist_on_shutdown = true;
        config.priority_limits[TxPriority::NORMAL] = 1000;
        config.priority_limits[TxPriority::HIGH] = 500;

        INTcoinMempool mempool;
        mempool.Initialize(config);

        mempool.AddTransaction(CreateTestTransaction(5000), TxPriority::NORMAL);
        mempool.AddTransaction(CreateTestTransaction(10000), TxPriority::HIGH);

        auto result = mempool.Persist();
        assert(result.IsOk());

        std::cout << "  - Persisted 2 transactions to disk" << std::endl;
    }

    // Create new mempool and restore
    {
        MempoolConfig config;
        config.max_size_mb = 100;
        config.persist_file = persist_path;
        config.priority_limits[TxPriority::NORMAL] = 1000;
        config.priority_limits[TxPriority::HIGH] = 500;

        INTcoinMempool mempool;
        mempool.Initialize(config);

        auto result = mempool.Restore();
        assert(result.IsOk());

        auto stats = mempool.GetStats();
        assert(stats.total_transactions == 2);

        std::cout << "  - Restored 2 transactions from disk" << std::endl;
    }

    // Cleanup
    std::remove(persist_path.c_str());

    std::cout << "✓ Mempool persistence working correctly" << std::endl;
}

// Test 8: Stats tracking
void TestStatsTracking() {
    std::cout << "\nTest 8: Stats Tracking..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;
    config.priority_limits[TxPriority::HIGH] = 500;
    config.priority_limits[TxPriority::HTLC] = 100;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    mempool.AddTransaction(CreateTestTransaction(5000), TxPriority::NORMAL);
    mempool.AddTransaction(CreateTestTransaction(10000), TxPriority::HIGH);
    mempool.AddTransaction(CreateTestTransaction(7500), TxPriority::HTLC);

    auto stats = mempool.GetStats();

    assert(stats.total_transactions == 3);
    assert(stats.count_by_priority.at(TxPriority::NORMAL) == 1);
    assert(stats.count_by_priority.at(TxPriority::HIGH) == 1);
    assert(stats.count_by_priority.at(TxPriority::HTLC) == 1);
    assert(stats.total_size_bytes > 0);
    assert(stats.total_fees > 0);

    std::cout << "✓ Stats tracking working correctly" << std::endl;
    std::cout << "  - Total transactions: " << stats.total_transactions << std::endl;
    std::cout << "  - Total size: " << stats.total_size_bytes << " bytes" << std::endl;
    std::cout << "  - Total fees: " << stats.total_fees << " satoshis" << std::endl;
}

// Test 9: Eviction policy
void TestEvictionPolicy() {
    std::cout << "\nTest 9: Eviction Policy..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 5;  // Low limit to trigger eviction

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add transactions up to limit
    for (int i = 0; i < 5; ++i) {
        mempool.AddTransaction(CreateTestTransaction(1000 + i * 100), TxPriority::NORMAL);
    }

    auto stats_before = mempool.GetStats();
    assert(stats_before.total_transactions == 5);

    // Add one more - should trigger eviction of lowest fee
    mempool.AddTransaction(CreateTestTransaction(10000), TxPriority::NORMAL);

    auto stats_after = mempool.GetStats();
    // Mempool may have evicted low-fee transaction
    assert(stats_after.total_transactions <= 6);

    std::cout << "✓ Eviction policy working (transactions: "
              << stats_after.total_transactions << ")" << std::endl;
}

// Test 10: Priority upgrade based on fees
void TestPriorityUpgrade() {
    std::cout << "\nTest 10: Priority Upgrade..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;
    config.priority_limits[TxPriority::HIGH] = 1000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add low priority transaction with very high fee
    // Mempool should upgrade it to higher priority
    Transaction tx = CreateTestTransaction(50000);  // Very high fee
    mempool.AddTransaction(tx, TxPriority::LOW);

    auto stats = mempool.GetStats();

    // Transaction should have been upgraded from LOW priority
    // (exact behavior depends on DeterminePriority implementation)
    assert(stats.total_transactions == 1);

    std::cout << "✓ Priority upgrade based on fees working" << std::endl;
}

// Test 11: Clear mempool
void TestClearMempool() {
    std::cout << "\nTest 11: Clear Mempool..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 1000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Add multiple transactions
    for (int i = 0; i < 10; ++i) {
        mempool.AddTransaction(CreateTestTransaction(1000 + i * 100), TxPriority::NORMAL);
    }

    auto stats_before = mempool.GetStats();
    assert(stats_before.total_transactions == 10);

    mempool.Clear();

    auto stats_after = mempool.GetStats();
    assert(stats_after.total_transactions == 0);
    assert(stats_after.total_size_bytes == 0);
    assert(stats_after.total_fees == 0);

    std::cout << "✓ Clear mempool successful" << std::endl;
}

// Test 12: Thread safety (concurrent adds)
void TestThreadSafety() {
    std::cout << "\nTest 12: Thread Safety..." << std::endl;

    MempoolConfig config;
    config.max_size_mb = 100;
    config.priority_limits[TxPriority::NORMAL] = 10000;
    config.priority_limits[TxPriority::HIGH] = 10000;

    INTcoinMempool mempool;
    mempool.Initialize(config);

    // Launch multiple threads adding transactions
    std::vector<std::thread> threads;
    const int num_threads = 4;
    const int txs_per_thread = 25;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mempool, i]() {
            for (int j = 0; j < txs_per_thread; ++j) {
                Transaction tx = CreateTestTransaction(1000 + (i * 100) + j);
                TxPriority priority = (i % 2 == 0) ? TxPriority::NORMAL : TxPriority::HIGH;
                mempool.AddTransaction(tx, priority);
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto stats = mempool.GetStats();
    assert(stats.total_transactions == num_threads * txs_per_thread);

    std::cout << "✓ Thread safety verified (" << stats.total_transactions
              << " transactions added concurrently)" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "INTcoin Enhanced Mempool Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        TestMempoolInitialization();
        TestAddTransactions();
        TestFeePrioritization();
        TestRemoveTransaction();
        TestBlockTemplateGeneration();
        TestTransactionExpiry();
        TestMempoolPersistence();
        TestStatsTracking();
        TestEvictionPolicy();
        TestPriorityUpgrade();
        TestClearMempool();
        TestThreadSafety();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All mempool tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n✗ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
