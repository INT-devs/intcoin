/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Consensus Implementation (RandomX + Digishield V3)
 */

#include "intcoin/consensus.h"
#include "intcoin/blockchain.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <randomx.h>
#include <mutex>
#include <memory>

namespace intcoin {

// ============================================================================
// RandomX Global State
// ============================================================================

namespace {
    // Global RandomX resources
    randomx_cache* g_randomx_cache = nullptr;
    randomx_vm* g_randomx_vm = nullptr;
    uint256 g_current_key{};
    uint64_t g_current_epoch = 0;
    std::mutex g_randomx_mutex;
    bool g_randomx_initialized = false;
}

// ============================================================================
// Block Reward Calculation
// ============================================================================

uint64_t GetBlockReward(uint64_t height) {
    uint64_t halvings = height / consensus::HALVING_INTERVAL;
    if (halvings >= consensus::MAX_HALVINGS) {
        return 0; // No more block rewards after 64 halvings
    }

    uint64_t reward = consensus::INITIAL_BLOCK_REWARD;
    reward >>= halvings; // Divide by 2^halvings

    return reward;
}

uint64_t GetSupplyAtHeight(uint64_t height) {
    uint64_t supply = 0;
    uint64_t current_height = 0;

    while (current_height < height) {
        uint64_t next_halving = ((current_height / consensus::HALVING_INTERVAL) + 1)
                               * consensus::HALVING_INTERVAL;
        uint64_t blocks_this_period = std::min(next_halving, height) - current_height;

        supply += blocks_this_period * GetBlockReward(current_height);
        current_height += blocks_this_period;
    }

    return supply;
}

uint64_t GetHalvingCount(uint64_t height) {
    return height / consensus::HALVING_INTERVAL;
}

uint64_t GetNextHalvingHeight(uint64_t height) {
    uint64_t halving_count = GetHalvingCount(height);
    return (halving_count + 1) * consensus::HALVING_INTERVAL;
}

// ============================================================================
// Difficulty Adjustment (Digishield V3)
// ============================================================================

uint256 DifficultyCalculator::CompactToTarget(uint32_t compact) {
    // Compact format: 0xNNSSSSS
    // NN = exponent (number of bytes)
    // SSSSSS = mantissa (3-byte coefficient)

    uint32_t exponent = compact >> 24;
    uint32_t mantissa = compact & 0x00FFFFFF;

    uint256 target{};

    // Check for valid exponent (must be 1-32)
    if (exponent <= 3) {
        // For exponent ≤ 3, shift mantissa right
        mantissa >>= (8 * (3 - exponent));
        if (mantissa > 0) {
            target[0] = mantissa & 0xFF;
            target[1] = (mantissa >> 8) & 0xFF;
            target[2] = (mantissa >> 16) & 0xFF;
        }
    } else if (exponent <= 32) {
        // For exponent > 3, place mantissa at the appropriate position
        int start_byte = exponent - 3;
        if (start_byte + 2 < 32) {
            target[start_byte] = mantissa & 0xFF;
            target[start_byte + 1] = (mantissa >> 8) & 0xFF;
            target[start_byte + 2] = (mantissa >> 16) & 0xFF;
        }
    }

    // Check for negative (should not happen in valid blocks)
    if (mantissa & 0x00800000) {
        // Sign bit set - invalid
        return uint256{};
    }

    return target;
}

uint32_t DifficultyCalculator::TargetToCompact(const uint256& target) {
    // Find the most significant non-zero byte
    int size = 32;
    while (size > 0 && target[size - 1] == 0) {
        size--;
    }

    if (size == 0) {
        return 0;
    }

    // Extract 3-byte mantissa
    uint32_t mantissa = 0;
    if (size >= 3) {
        mantissa = target[size - 1];
        mantissa = (mantissa << 8) | target[size - 2];
        mantissa = (mantissa << 8) | target[size - 3];
    } else if (size == 2) {
        mantissa = target[size - 1];
        mantissa = (mantissa << 8) | target[size - 2];
    } else {
        mantissa = target[0];
    }

    // Check if we need to adjust for sign bit
    if (mantissa & 0x00800000) {
        mantissa >>= 8;
        size++;
    }

    uint32_t compact = (size << 24) | mantissa;
    return compact;
}

bool DifficultyCalculator::CheckProofOfWork(const uint256& hash, uint32_t bits) {
    uint256 target = CompactToTarget(bits);

    // Check target is non-zero
    bool is_zero = true;
    for (size_t i = 0; i < 32; i++) {
        if (target[i] != 0) {
            is_zero = false;
            break;
        }
    }
    if (is_zero) {
        return false;
    }

    // Check hash < target (little-endian comparison)
    for (int i = 31; i >= 0; i--) {
        if (hash[i] < target[i]) {
            return true;
        } else if (hash[i] > target[i]) {
            return false;
        }
    }

    // hash == target (should still pass)
    return true;
}

double DifficultyCalculator::GetDifficulty(uint32_t bits) {
    // Difficulty = max_target / current_target
    // max_target is the genesis block target (minimum difficulty)

    uint256 max_target = CompactToTarget(consensus::MIN_DIFFICULTY_BITS);
    uint256 current_target = CompactToTarget(bits);

    // Calculate difficulty as a double
    // For simplicity, we use the first 8 bytes of the target
    uint64_t max_val = 0;
    uint64_t current_val = 0;

    for (int i = 0; i < 8 && i < 32; i++) {
        max_val = (max_val << 8) | max_target[31 - i];
        current_val = (current_val << 8) | current_target[31 - i];
    }

    if (current_val == 0) {
        return 0.0;
    }

    return static_cast<double>(max_val) / static_cast<double>(current_val);
}

uint32_t DifficultyCalculator::GetNextWorkRequired(const BlockHeader& last_block,
                                                  const Blockchain& chain) {
    // Digishield V3 difficulty adjustment algorithm
    constexpr int AVERAGING_WINDOW = 60;    // Last 60 blocks
    constexpr int DAMPING_FACTOR = 4;        // ±25% max adjustment

    // Genesis block or first few blocks use minimum difficulty
    uint64_t current_height = chain.GetBestHeight();
    if (current_height < AVERAGING_WINDOW) {
        return consensus::MIN_DIFFICULTY_BITS;
    }

    // Get last N blocks
    std::vector<BlockHeader> blocks;
    blocks.reserve(AVERAGING_WINDOW);

    for (int i = 0; i < AVERAGING_WINDOW; i++) {
        auto block_result = chain.GetBlockHeaderByHeight(current_height - i);
        if (block_result.IsError()) {
            // If we can't get block, use minimum difficulty
            return consensus::MIN_DIFFICULTY_BITS;
        }
        blocks.push_back(*block_result.value);
    }

    // Calculate actual timespan (time between first and last block in window)
    uint64_t actual_timespan = blocks[0].timestamp - blocks[AVERAGING_WINDOW - 1].timestamp;

    // Calculate expected timespan
    uint64_t expected_timespan = (AVERAGING_WINDOW - 1) * consensus::TARGET_BLOCK_TIME;

    // Apply damping factor to prevent large swings
    uint64_t min_timespan = expected_timespan / DAMPING_FACTOR;
    uint64_t max_timespan = expected_timespan * DAMPING_FACTOR;
    actual_timespan = std::max(min_timespan, std::min(actual_timespan, max_timespan));

    // Calculate average target from last N blocks
    uint256 total_target{};
    for (const auto& block : blocks) {
        uint256 block_target = CompactToTarget(block.bits);

        // Add to total (with overflow protection)
        bool overflow = false;
        for (int i = 0; i < 32; i++) {
            uint16_t sum = total_target[i] + block_target[i];
            if (i > 0 && overflow) {
                sum++;
            }
            total_target[i] = sum & 0xFF;
            overflow = (sum > 0xFF);
        }
    }

    // Divide by AVERAGING_WINDOW to get average
    uint256 avg_target{};
    uint16_t remainder = 0;
    for (int i = 31; i >= 0; i--) {
        uint16_t value = (remainder << 8) | total_target[i];
        avg_target[i] = value / AVERAGING_WINDOW;
        remainder = value % AVERAGING_WINDOW;
    }

    // Adjust target based on actual vs expected timespan
    // new_target = avg_target * actual_timespan / expected_timespan
    uint256 new_target{};
    uint64_t carry = 0;
    for (int i = 0; i < 32; i++) {
        uint64_t product = (uint64_t)avg_target[i] * actual_timespan + carry;
        carry = product / expected_timespan;
        new_target[i] = product % expected_timespan;
    }

    // If there's still a carry, we're at maximum difficulty
    if (carry > 0) {
        // Saturate at maximum value
        for (int i = 0; i < 32; i++) {
            new_target[i] = 0xFF;
        }
    }

    // Enforce minimum and maximum difficulty bounds
    uint256 min_target = CompactToTarget(consensus::MAX_DIFFICULTY_BITS);
    uint256 max_target = CompactToTarget(consensus::MIN_DIFFICULTY_BITS);

    // Check if new_target < min_target (too difficult)
    bool below_min = false;
    for (int i = 31; i >= 0; i--) {
        if (new_target[i] < min_target[i]) {
            below_min = true;
            break;
        } else if (new_target[i] > min_target[i]) {
            break;
        }
    }
    if (below_min) {
        new_target = min_target;
    }

    // Check if new_target > max_target (too easy)
    bool above_max = false;
    for (int i = 31; i >= 0; i--) {
        if (new_target[i] > max_target[i]) {
            above_max = true;
            break;
        } else if (new_target[i] < max_target[i]) {
            break;
        }
    }
    if (above_max) {
        new_target = max_target;
    }

    return TargetToCompact(new_target);
}

// ============================================================================
// RandomX Proof-of-Work
// ============================================================================

Result<void> RandomXValidator::Initialize() {
    std::lock_guard<std::mutex> lock(g_randomx_mutex);

    if (g_randomx_initialized) {
        return Result<void>::Ok(); // Already initialized
    }

    // Allocate RandomX cache
    randomx_flags flags = RANDOMX_FLAG_DEFAULT;
    #if defined(__x86_64__) || defined(_M_X64)
        flags = static_cast<randomx_flags>(flags | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES);
    #elif defined(__aarch64__) || defined(_M_ARM64)
        flags = static_cast<randomx_flags>(flags | RANDOMX_FLAG_HARD_AES);
    #endif

    g_randomx_cache = randomx_alloc_cache(flags);
    if (!g_randomx_cache) {
        return Result<void>::Error("Failed to allocate RandomX cache");
    }

    // Initialize with genesis key (height 0)
    g_current_key = GetRandomXKey(0);
    randomx_init_cache(g_randomx_cache, g_current_key.data(), g_current_key.size());
    g_current_epoch = 0;

    // Create VM
    g_randomx_vm = randomx_create_vm(flags, g_randomx_cache, nullptr);
    if (!g_randomx_vm) {
        randomx_release_cache(g_randomx_cache);
        g_randomx_cache = nullptr;
        return Result<void>::Error("Failed to create RandomX VM");
    }

    g_randomx_initialized = true;
    return Result<void>::Ok();
}

void RandomXValidator::Shutdown() {
    std::lock_guard<std::mutex> lock(g_randomx_mutex);

    if (!g_randomx_initialized) {
        return;
    }

    if (g_randomx_vm) {
        randomx_destroy_vm(g_randomx_vm);
        g_randomx_vm = nullptr;
    }

    if (g_randomx_cache) {
        randomx_release_cache(g_randomx_cache);
        g_randomx_cache = nullptr;
    }

    g_randomx_initialized = false;
}

Result<void> RandomXValidator::ValidateBlockHash(const BlockHeader& header) {
    // Calculate RandomX hash for the header
    auto hash_result = CalculateHash(header);
    if (hash_result.IsError()) {
        return Result<void>::Error("Failed to calculate RandomX hash: " + hash_result.error);
    }

    uint256 calculated_hash = *hash_result.value;

    // Verify the hash meets the difficulty target
    if (!DifficultyCalculator::CheckProofOfWork(calculated_hash, header.bits)) {
        return Result<void>::Error("Block hash does not meet difficulty target");
    }

    return Result<void>::Ok();
}

Result<uint256> RandomXValidator::CalculateHash(const BlockHeader& header) {
    std::lock_guard<std::mutex> lock(g_randomx_mutex);

    if (!g_randomx_initialized) {
        return Result<uint256>::Error("RandomX not initialized");
    }

    // Check if we need to update the cache with the header's RandomX key
    if (header.randomx_key != g_current_key) {
        // Reinitialize cache with the new key
        randomx_init_cache(g_randomx_cache, header.randomx_key.data(), header.randomx_key.size());
        g_current_key = header.randomx_key;
    }

    // Serialize block header for hashing (excluding the randomx_hash field itself)
    std::vector<uint8_t> header_data = header.Serialize();

    // Calculate RandomX hash
    uint256 hash{};
    randomx_calculate_hash(g_randomx_vm, header_data.data(), header_data.size(), hash.data());

    return Result<uint256>::Ok(std::move(hash));
}

uint256 RandomXValidator::GetRandomXKey(uint64_t height) {
    // Calculate epoch number
    uint64_t epoch = height / RANDOMX_EPOCH_BLOCKS;

    // Create key string: "INTcoin-RandomX-Epoch-{epoch_number}"
    std::string key_string = "INTcoin-RandomX-Epoch-" + std::to_string(epoch);

    // Hash the key string with SHA3-256 to get the RandomX key
    std::vector<uint8_t> key_bytes(key_string.begin(), key_string.end());
    return SHA3::Hash(key_bytes);
}

bool RandomXValidator::NeedsDatasetUpdate(uint64_t height) {
    return (height % RANDOMX_EPOCH_BLOCKS) == 0;
}

Result<void> RandomXValidator::UpdateDataset(uint64_t height) {
    std::lock_guard<std::mutex> lock(g_randomx_mutex);

    if (!g_randomx_initialized) {
        return Result<void>::Error("RandomX not initialized");
    }

    uint64_t new_epoch = height / RANDOMX_EPOCH_BLOCKS;

    // Check if update is needed
    if (new_epoch == g_current_epoch) {
        return Result<void>::Ok(); // Already on correct epoch
    }

    // Get new key
    uint256 new_key = GetRandomXKey(height);

    // Reinitialize cache with new key
    randomx_init_cache(g_randomx_cache, new_key.data(), new_key.size());

    // Note: VM automatically uses the updated cache
    g_current_key = new_key;
    g_current_epoch = new_epoch;

    return Result<void>::Ok();
}

// ============================================================================
// Consensus Validation
// ============================================================================

Result<void> ConsensusValidator::ValidateBlockHeader(const BlockHeader& header,
                                                    const Blockchain& chain) {
    // TODO: Implement comprehensive header validation
    return Result<void>::Error("Not implemented");
}

Result<void> ConsensusValidator::ValidateBlockSize(const Block& block) {
    if (block.GetSerializedSize() > consensus::MAX_BLOCK_SIZE) {
        return Result<void>::Error("Block size exceeds maximum");
    }
    return Result<void>::Ok();
}

Result<void> ConsensusValidator::ValidateCoinbase(const Transaction& coinbase,
                                                 uint64_t height,
                                                 uint64_t total_fees) {
    // TODO: Validate coinbase transaction
    // Check:
    // 1. Is coinbase transaction
    // 2. Reward is correct (subsidy + fees)
    // 3. Output scripts are valid

    return Result<void>::Error("Not implemented");
}

Result<void> ConsensusValidator::ValidateTimestamp(uint64_t timestamp,
                                                  uint64_t median_time_past) {
    // Timestamp must be greater than median time past
    if (timestamp <= median_time_past) {
        return Result<void>::Error("Timestamp too old");
    }

    // Timestamp must not be too far in the future (2 hours)
    uint64_t max_timestamp = GetCurrentTime() + 2 * 60 * 60;
    if (timestamp > max_timestamp) {
        return Result<void>::Error("Timestamp too far in future");
    }

    return Result<void>::Ok();
}

// ============================================================================
// Consensus Parameters
// ============================================================================

ConsensusParams GetMainnetParams() {
    return {
        consensus::TARGET_BLOCK_TIME,
        consensus::HALVING_INTERVAL,
        consensus::INITIAL_BLOCK_REWARD,
        consensus::MAX_HALVINGS,
        consensus::MAX_SUPPLY,
        consensus::MAX_BLOCK_SIZE,
        consensus::COINBASE_MATURITY,
        consensus::MAX_TX_SIZE,
        consensus::MIN_TX_FEE
    };
}

ConsensusParams GetTestnetParams() {
    auto params = GetMainnetParams();
    // Testnet has faster block times for testing
    params.target_block_time = 30; // 30 seconds
    return params;
}

ConsensusParams GetRegtestParams() {
    auto params = GetMainnetParams();
    // Regtest has instant block times
    params.target_block_time = 1; // 1 second
    return params;
}

} // namespace intcoin
