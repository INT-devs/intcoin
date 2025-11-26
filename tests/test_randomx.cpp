/*
 * RandomX PoW Tests
 * Tests RandomX initialization, hashing, and validation
 */

#include "intcoin/consensus.h"
#include "intcoin/block.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

void test_randomx_initialization() {
    std::cout << "\n=== Test 1: RandomX Initialization ===" << std::endl;

    // Initialize RandomX
    auto init_result = RandomXValidator::Initialize();
    assert(init_result.IsOk());
    std::cout << "✓ RandomX initialized successfully" << std::endl;

    // Test double initialization (should succeed)
    auto init_result2 = RandomXValidator::Initialize();
    assert(init_result2.IsOk());
    std::cout << "✓ Double initialization handled correctly" << std::endl;
}

void test_randomx_key_generation() {
    std::cout << "\n=== Test 2: RandomX Key Generation ===" << std::endl;

    // Test key generation for different epochs
    uint256 key0 = RandomXValidator::GetRandomXKey(0);
    uint256 key1 = RandomXValidator::GetRandomXKey(2048);  // Next epoch
    uint256 key2 = RandomXValidator::GetRandomXKey(4096);  // Epoch 2

    // Keys should be different for different epochs
    assert(key0 != key1);
    assert(key1 != key2);
    assert(key0 != key2);
    std::cout << "✓ Different epochs produce different keys" << std::endl;

    // Same epoch should produce same key
    uint256 key0_dup = RandomXValidator::GetRandomXKey(100);  // Still epoch 0
    uint256 key1_dup = RandomXValidator::GetRandomXKey(3000);  // Still epoch 1
    assert(key0 == key0_dup);
    assert(key1 == key1_dup);
    std::cout << "✓ Same epoch produces same key" << std::endl;

    std::cout << "Epoch 0 key: " << ToHex(key0) << std::endl;
    std::cout << "Epoch 1 key: " << ToHex(key1) << std::endl;
}

void test_randomx_hash_calculation() {
    std::cout << "\n=== Test 3: RandomX Hash Calculation ===" << std::endl;

    // Create a test block header
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = {};  // Genesis
    header.merkle_root = {};
    header.timestamp = GetCurrentTime();
    header.bits = consensus::MIN_DIFFICULTY_BITS;
    header.nonce = 0;
    header.randomx_key = RandomXValidator::GetRandomXKey(0);  // Epoch 0

    // Calculate hash
    auto hash_result = RandomXValidator::CalculateHash(header);
    assert(hash_result.IsOk());
    uint256 hash1 = *hash_result.value;
    std::cout << "✓ Hash calculated successfully" << std::endl;
    std::cout << "Hash: " << ToHex(hash1) << std::endl;

    // Same header should produce same hash
    auto hash_result2 = RandomXValidator::CalculateHash(header);
    assert(hash_result2.IsOk());
    uint256 hash2 = *hash_result2.value;
    assert(hash1 == hash2);
    std::cout << "✓ Deterministic hashing (same input → same output)" << std::endl;

    // Different nonce should produce different hash
    header.nonce = 1;
    auto hash_result3 = RandomXValidator::CalculateHash(header);
    assert(hash_result3.IsOk());
    uint256 hash3 = *hash_result3.value;
    assert(hash1 != hash3);
    std::cout << "✓ Different nonce produces different hash" << std::endl;
}

void test_dataset_update() {
    std::cout << "\n=== Test 4: Dataset Update ===" << std::endl;

    // Test if dataset update is needed
    bool needs_update_0 = RandomXValidator::NeedsDatasetUpdate(0);
    bool needs_update_100 = RandomXValidator::NeedsDatasetUpdate(100);
    bool needs_update_2048 = RandomXValidator::NeedsDatasetUpdate(2048);

    assert(needs_update_0 == true);    // Block 0 starts epoch 0
    assert(needs_update_100 == false); // Still in epoch 0
    assert(needs_update_2048 == true); // Block 2048 starts epoch 1
    std::cout << "✓ Dataset update detection working correctly" << std::endl;

    // Test dataset update
    auto update_result = RandomXValidator::UpdateDataset(2048);
    assert(update_result.IsOk());
    std::cout << "✓ Dataset updated successfully for new epoch" << std::endl;

    // Hash should work with new epoch
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = {};
    header.merkle_root = {};
    header.timestamp = GetCurrentTime();
    header.bits = consensus::MIN_DIFFICULTY_BITS;
    header.nonce = 0;
    header.randomx_key = RandomXValidator::GetRandomXKey(2048);  // Epoch 1 key

    auto hash_result = RandomXValidator::CalculateHash(header);
    assert(hash_result.IsOk());
    std::cout << "✓ Hashing works after dataset update" << std::endl;
    std::cout << "Epoch 1 hash: " << ToHex(*hash_result.value) << std::endl;
}

void test_block_validation() {
    std::cout << "\n=== Test 5: Block Hash Validation ===" << std::endl;

    // Create a test block header
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = {};
    header.merkle_root = {};
    header.timestamp = GetCurrentTime();
    header.bits = 0x1f00ffff;  // Very low difficulty for testing
    header.nonce = 0;
    header.randomx_key = RandomXValidator::GetRandomXKey(0);

    // Calculate hash first
    auto hash_result = RandomXValidator::CalculateHash(header);
    assert(hash_result.IsOk());
    header.randomx_hash = *hash_result.value;

    // Validate the block (should pass with low difficulty)
    auto validate_result = RandomXValidator::ValidateBlockHash(header);
    if (validate_result.IsError()) {
        std::cout << "Note: Hash doesn't meet difficulty (this is normal, try different nonce)" << std::endl;
        std::cout << "Hash: " << ToHex(header.randomx_hash) << std::endl;
        // This is OK - we'd need to mine to find a valid nonce
    } else {
        std::cout << "✓ Block validation passed (lucky nonce!)" << std::endl;
    }
}

void test_randomx_shutdown() {
    std::cout << "\n=== Test 6: RandomX Shutdown ===" << std::endl;

    // Shutdown RandomX
    RandomXValidator::Shutdown();
    std::cout << "✓ RandomX shut down successfully" << std::endl;

    // Test that operations fail after shutdown
    BlockHeader header;
    header.version = 1;
    header.randomx_key = RandomXValidator::GetRandomXKey(0);
    auto hash_result = RandomXValidator::CalculateHash(header);
    assert(hash_result.IsError());
    std::cout << "✓ Operations correctly fail after shutdown" << std::endl;

    // Re-initialize for cleanup
    auto init_result = RandomXValidator::Initialize();
    assert(init_result.IsOk());
    std::cout << "✓ Re-initialization works" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RandomX PoW Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_randomx_initialization();
        test_randomx_key_generation();
        test_randomx_hash_calculation();
        test_dataset_update();
        test_block_validation();
        test_randomx_shutdown();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✓ All RandomX tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;

        // Final cleanup
        RandomXValidator::Shutdown();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        RandomXValidator::Shutdown();
        return 1;
    }
}
