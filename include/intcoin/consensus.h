/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Consensus Rules (RandomX PoW + Digishield V3)
 */

#ifndef INTCOIN_CONSENSUS_H
#define INTCOIN_CONSENSUS_H

#include "types.h"
#include "block.h"
#include <cstdint>
#include <vector>
#include <map>

namespace intcoin {

// ============================================================================
// Consensus Parameters
// ============================================================================

namespace consensus {

/// Block time target (2 minutes)
constexpr uint64_t TARGET_BLOCK_TIME = 120; // seconds

/// Maximum future block time (2 hours)
constexpr uint64_t MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60; // 2 hours in seconds

/// Median time past span (BIP 113)
constexpr uint64_t MEDIAN_TIME_SPAN = 11; // blocks

/// Halving interval (~4 years at 2 min blocks)
constexpr uint64_t HALVING_INTERVAL = 1051200; // blocks

/// Initial block reward (105,113,636 INT)
constexpr uint64_t INITIAL_BLOCK_REWARD = 105113636ULL * INTS_PER_INT;

/// Maximum number of halvings (64)
constexpr uint64_t MAX_HALVINGS = 64;

/// Total supply (221 Trillion INT)
constexpr uint64_t MAX_SUPPLY = 221000000000000ULL * INTS_PER_INT;

/// Maximum block size (8 MB)
constexpr size_t MAX_BLOCK_SIZE = 8 * 1024 * 1024;

/// Coinbase maturity (100 blocks)
constexpr uint64_t COINBASE_MATURITY = 100;

/// Max transaction size (1 MB)
constexpr size_t MAX_TX_SIZE = 1024 * 1024;

/// Min transaction fee (0.0001 INT)
constexpr uint64_t MIN_TX_FEE = 100; // 100 INTS = 0.0001 INT

/// Difficulty adjustment interval (every block - Digishield V3)
constexpr uint64_t DIFFICULTY_ADJUSTMENT_INTERVAL = 1;

/// Difficulty averaging window (Digishield V3)
constexpr uint64_t DIFFICULTY_AVERAGING_WINDOW = 60;

/// Difficulty damping factor (max adjustment per block)
constexpr double DIFFICULTY_DAMPING_FACTOR = 4.0;

/// Minimum difficulty (initial)
/// Minimum difficulty - very easy for testnet (~50% of hashes pass)
constexpr uint32_t MIN_DIFFICULTY_BITS = 0x207fffff;

/// Maximum difficulty (hardest possible)
constexpr uint32_t MAX_DIFFICULTY_BITS = 0x03010000;

/// Maximum nonce value
constexpr uint64_t MAX_NONCE = 0xFFFFFFFFFFFFFFFFULL;

/// ========================================================================
/// 51% Attack Protection
/// ========================================================================

/// Maximum reorganization depth (protect against deep reorgs)
constexpr uint64_t MAX_REORG_DEPTH = 100; // blocks

/// Deep reorganization warning threshold
constexpr uint64_t DEEP_REORG_WARNING_THRESHOLD = 6; // blocks

/// Checkpoint interval (every ~1 week: 5040 blocks at 2 min/block)
constexpr uint64_t CHECKPOINT_INTERVAL = 5040;

/// Minimum confirmations for finality
constexpr uint64_t FINALITY_DEPTH = 100; // blocks

} // namespace consensus

// ============================================================================
// Block Reward Calculation
// ============================================================================

/// Calculate block reward for given height
uint64_t GetBlockReward(uint64_t height);

/// Calculate total supply at given height
uint64_t GetSupplyAtHeight(uint64_t height);

/// Get number of halvings that have occurred
uint64_t GetHalvingCount(uint64_t height);

/// Get next halving height
uint64_t GetNextHalvingHeight(uint64_t height);

// ============================================================================
// Difficulty Adjustment (Digishield V3)
// ============================================================================

class DifficultyCalculator {
public:
    /// Calculate next difficulty target (Digishield V3)
    static uint32_t GetNextWorkRequired(const BlockHeader& last_block,
                                       const class Blockchain& chain);

    /// Convert compact bits to target
    static uint256 CompactToTarget(uint32_t compact);

    /// Convert target to compact bits
    static uint32_t TargetToCompact(const uint256& target);

    /// Check if hash meets difficulty target
    static bool CheckProofOfWork(const uint256& hash, uint32_t bits);

    /// Get difficulty from bits
    static double GetDifficulty(uint32_t bits);

    /// Get network hash rate estimate
    static double GetNetworkHashRate(const class Blockchain& chain,
                                    uint64_t num_blocks = 120);
};

// ============================================================================
// RandomX Proof-of-Work
// ============================================================================

class RandomXValidator {
public:
    /// Initialize RandomX (call once at startup)
    static Result<void> Initialize();

    /// Shutdown RandomX (call at cleanup)
    static void Shutdown();

    /// Validate block's RandomX hash
    static Result<void> ValidateBlockHash(const BlockHeader& header);

    /// Calculate RandomX hash for header
    static Result<uint256> CalculateHash(const BlockHeader& header);

    /// Get RandomX key for block height
    static uint256 GetRandomXKey(uint64_t height);

    /// Check if RandomX dataset needs update
    static bool NeedsDatasetUpdate(uint64_t height);

    /// Update RandomX dataset for new epoch
    static Result<void> UpdateDataset(uint64_t height);

private:
    static constexpr uint64_t RANDOMX_EPOCH_BLOCKS = 2048; // ~2.8 days
};

// ============================================================================
// Block Validation
// ============================================================================

class ConsensusValidator {
public:
    /// Validate block header (PoW, timestamp, difficulty)
    static Result<void> ValidateBlockHeader(const BlockHeader& header,
                                           const class Blockchain& chain);

    /// Validate block size
    static Result<void> ValidateBlockSize(const Block& block);

    /// Validate coinbase transaction
    static Result<void> ValidateCoinbase(const Transaction& coinbase,
                                        uint64_t height,
                                        uint64_t total_fees);

    /// Validate block timestamp
    static Result<void> ValidateTimestamp(uint64_t timestamp,
                                         uint64_t median_time_past);

    /// Validate block transactions
    static Result<void> ValidateBlockTransactions(const Block& block,
                                                 const class UTXOSet& utxo_set);

    /// Check if block is valid
    static Result<void> ValidateBlock(const Block& block,
                                     const class Blockchain& chain);

    /// Calculate median time past
    static uint64_t GetMedianTimePast(const class Blockchain& chain,
                                     uint64_t height,
                                     size_t num_blocks = 11);
};

// ============================================================================
// Transaction Validation
// ============================================================================

class TransactionValidator {
public:
    /// Validate transaction structure
    static Result<void> ValidateStructure(const Transaction& tx);

    /// Validate transaction inputs
    static Result<void> ValidateInputs(const Transaction& tx,
                                      const class UTXOSet& utxo_set);

    /// Validate transaction outputs
    static Result<void> ValidateOutputs(const Transaction& tx);

    /// Validate transaction signature
    static Result<void> ValidateSignature(const Transaction& tx,
                                         const PublicKey& pubkey);

    /// Validate transaction fees
    static Result<void> ValidateFees(const Transaction& tx,
                                    const class UTXOSet& utxo_set);

    /// Check for double spend
    static Result<void> CheckDoubleSpend(const Transaction& tx,
                                        const class UTXOSet& utxo_set,
                                        const class Mempool& mempool);

    /// Validate transaction locktime
    static Result<void> ValidateLocktime(const Transaction& tx,
                                        uint64_t block_height,
                                        uint64_t block_time);

    /// Check if transaction is final
    static bool IsFinal(const Transaction& tx,
                       uint64_t block_height,
                       uint64_t block_time);
};

// ============================================================================
// Script Validation
// ============================================================================

class ScriptValidator {
public:
    /// Execute and validate script
    static Result<void> ValidateScript(const Script& script_sig,
                                      const Script& script_pubkey,
                                      const Transaction& tx,
                                      size_t input_index);

    /// Verify signature in script
    static Result<void> VerifyScriptSignature(const Signature& sig,
                                             const uint256& message_hash,
                                             const PublicKey& pubkey);

    /// Check standard script types
    static bool IsStandard(const Script& script);

    /// Get script type
    static ScriptType GetScriptType(const Script& script);
};

// ============================================================================
// Chain Validation
// ============================================================================

class ChainValidator {
public:
    /// Validate chain from genesis
    static Result<void> ValidateChain(const class Blockchain& chain);

    /// Validate chain segment
    static Result<void> ValidateChainSegment(const class Blockchain& chain,
                                            uint64_t start_height,
                                            uint64_t end_height);

    /// Check if chain is valid
    static bool IsValidChain(const class Blockchain& chain);

    /// Get cumulative chain work
    static uint256 GetChainWork(const class Blockchain& chain);

    /// Compare chain work (for reorg)
    static bool HasMoreWork(const class Blockchain& chain1,
                           const class Blockchain& chain2);

    // 51% Attack Protection
    /// Check if block is at a checkpoint
    static bool IsCheckpoint(uint64_t height, const uint256& hash);

    /// Check if reorganization depth is allowed
    static Result<void> ValidateReorgDepth(uint64_t current_height,
                                          uint64_t fork_height);

    /// Get checkpoints map
    static const std::map<uint64_t, uint256>& GetCheckpoints();
};

// ============================================================================
// Mempool Validation
// ============================================================================

class MempoolValidator {
public:
    /// Validate transaction for mempool acceptance
    static Result<void> ValidateForMempool(const Transaction& tx,
                                          const class UTXOSet& utxo_set,
                                          const class Mempool& mempool);

    /// Check minimum fee
    static Result<void> ValidateMinimumFee(const Transaction& tx,
                                          const class UTXOSet& utxo_set);

    /// Check if transaction conflicts
    static bool HasConflict(const Transaction& tx,
                           const class Mempool& mempool);

    /// Validate RBF (Replace-by-Fee)
    static Result<void> ValidateReplacement(const Transaction& new_tx,
                                           const Transaction& old_tx,
                                           const class UTXOSet& utxo_set);
};

// ============================================================================
// Consensus Constants Access
// ============================================================================

/// Get consensus parameters for network
struct ConsensusParams {
    uint64_t target_block_time;
    uint64_t halving_interval;
    uint64_t initial_block_reward;
    uint64_t max_halvings;
    uint64_t max_supply;
    size_t max_block_size;
    uint64_t coinbase_maturity;
    size_t max_tx_size;
    uint64_t min_tx_fee;
};

/// Get mainnet consensus parameters
ConsensusParams GetMainnetParams();

/// Get testnet consensus parameters
ConsensusParams GetTestnetParams();

/// Get regtest consensus parameters
ConsensusParams GetRegtestParams();

} // namespace intcoin

#endif // INTCOIN_CONSENSUS_H
