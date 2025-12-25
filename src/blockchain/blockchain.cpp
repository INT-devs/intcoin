/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Blockchain Implementation
 */

#include "intcoin/blockchain.h"
#include "intcoin/consensus.h"
#include "intcoin/util.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace intcoin {

// ============================================================================
// Blockchain::Impl (Private Implementation)
// ============================================================================

class Blockchain::Impl {
public:
    // Database
    std::shared_ptr<BlockchainDB> db_;

    // UTXO set (in-memory for fast access)
    std::unordered_map<OutPoint, TxOut, OutPointHash> utxo_set_;

    // Chain state cache
    ChainState chain_state_;
    bool chain_state_loaded_ = false;

    // Mempool
    std::unique_ptr<Mempool> mempool_;

    // Callbacks
    std::vector<BlockCallback> block_callbacks_;
    std::vector<TransactionCallback> tx_callbacks_;

    // Mutex for thread safety
    mutable std::mutex mutex_;

    Impl(std::shared_ptr<BlockchainDB> db)
        : db_(std::move(db)) {}

    // Load chain state from database
    Result<void> LoadChainState() {
        auto result = db_->GetChainState();
        if (result.IsOk()) {
            chain_state_ = *result.value;
            chain_state_loaded_ = true;
            return Result<void>::Ok();
        }

        // If no chain state exists, initialize with defaults
        chain_state_.best_block_hash = uint256{};
        chain_state_.best_height = 0;
        chain_state_.chain_work = uint256{};
        chain_state_.total_transactions = 0;
        chain_state_.utxo_count = 0;
        chain_state_.total_supply = 0;
        chain_state_loaded_ = true;

        return Result<void>::Ok();
    }

    // Save chain state to database
    Result<void> SaveChainState() {
        return db_->StoreChainState(chain_state_);
    }

    // Load UTXO set from database (expensive operation)
    Result<void> LoadUTXOSet() {
        // TODO: Implement UTXO set loading
        // For now, UTXO set will be built incrementally as blocks are processed
        return Result<void>::Ok();
    }

    // Apply block to UTXO set
    Result<void> ApplyBlockToUTXO(const Block& block) {
        // Track spent outputs for reorganization support
        std::vector<SpentOutput> spent_outputs;

        // Remove spent outputs
        for (const auto& tx : block.transactions) {
            if (tx.IsCoinbase()) continue;

            for (const auto& input : tx.inputs) {
                OutPoint outpoint;
                outpoint.tx_hash = input.prev_tx_hash;
                outpoint.index = input.prev_tx_index;

                auto it = utxo_set_.find(outpoint);
                if (it == utxo_set_.end()) {
                    return Result<void>::Error("UTXO not found for input: " + ToHex(outpoint.tx_hash));
                }

                // Store spent output for potential reorg
                SpentOutput spent;
                spent.outpoint = outpoint;
                spent.output = it->second;
                spent_outputs.push_back(spent);

                // Remove from in-memory set
                utxo_set_.erase(it);

                // Remove from database
                auto delete_result = db_->DeleteUTXO(outpoint);
                if (delete_result.IsError()) {
                    return delete_result;
                }

                chain_state_.utxo_count--;
            }
        }

        // TODO: Store spent outputs in database for potential reorganization
        // Temporarily disabled pending StoreSpentOutputs implementation
        // if (!spent_outputs.empty()) {
        //     uint256 block_hash = block.GetHash();
        //     auto store_spent_result = db_->StoreSpentOutputs(block_hash, spent_outputs);
        //     if (store_spent_result.IsError()) {
        //         return store_spent_result;
        //     }
        // }

        // Add new outputs
        for (const auto& tx : block.transactions) {
            uint256 tx_hash = tx.GetHash();

            for (uint32_t i = 0; i < tx.outputs.size(); i++) {
                OutPoint outpoint;
                outpoint.tx_hash = tx_hash;
                outpoint.index = i;

                // Add to in-memory set
                utxo_set_[outpoint] = tx.outputs[i];

                // Add to database
                auto store_result = db_->StoreUTXO(outpoint, tx.outputs[i]);
                if (store_result.IsError()) {
                    return store_result;
                }

                chain_state_.utxo_count++;
            }
        }

        return Result<void>::Ok();
    }

    // Revert block from UTXO set (for reorganization)
    Result<void> RevertBlockFromUTXO(const Block& block) {
        // Reverse of ApplyBlockToUTXO
        // Remove outputs that were added
        for (const auto& tx : block.transactions) {
            uint256 tx_hash = tx.GetHash();

            for (uint32_t i = 0; i < tx.outputs.size(); i++) {
                OutPoint outpoint;
                outpoint.tx_hash = tx_hash;
                outpoint.index = i;

                utxo_set_.erase(outpoint);
                db_->DeleteUTXO(outpoint);
                chain_state_.utxo_count--;
            }
        }

        // Restore spent outputs from database
        uint256 block_hash = block.GetHash();
        auto spent_outputs_result = db_->GetSpentOutputs(block_hash);

        if (spent_outputs_result.IsOk()) {
            auto spent_outputs = spent_outputs_result.GetValue();

            for (const auto& spent : spent_outputs) {
                // Restore to in-memory set
                utxo_set_[spent.outpoint] = spent.output;

                // Restore to database
                auto restore_result = db_->StoreUTXO(spent.outpoint, spent.output);
                if (restore_result.IsError()) {
                    return restore_result;
                }

                chain_state_.utxo_count++;
            }

            // Clean up spent outputs record
            db_->DeleteSpentOutputs(block_hash);
        }

        return Result<void>::Ok();
    }

    // Calculate chain work for a block
    uint256 CalculateChainWork(uint32_t bits) const {
        // Chain work = difficulty = max_target / current_target
        // Compact format: bits = exponent || mantissa
        // Target = mantissa * 256^(exponent - 3)

        uint256 work{};

        if (bits == 0) {
            return work;  // Zero work for invalid bits
        }

        // Extract components from compact representation
        uint32_t exponent = bits >> 24;
        uint32_t mantissa = bits & 0x00FFFFFF;

        // Sanity checks
        if (exponent > 32 || mantissa == 0) {
            work[0] = 1;  // Minimal work for invalid encoding
            return work;
        }

        // For chain work calculation, we approximate:
        // work ≈ 2^256 / target
        // Simplified: work[0] = difficulty approximation
        // A proper implementation would use big integer division

        // Calculate approximate difficulty based on how hard the target is
        // Lower target (higher difficulty) = more work
        // This is a simplified calculation suitable for alpha
        uint64_t difficulty_approx = 1;
        if (exponent < 32) {
            // Scale difficulty based on exponent and mantissa
            difficulty_approx = (uint64_t(1) << (32 - exponent)) / (mantissa + 1);
        }

        // Store approximation in work (first 8 bytes)
        work[0] = difficulty_approx & 0xFF;
        work[1] = (difficulty_approx >> 8) & 0xFF;
        work[2] = (difficulty_approx >> 16) & 0xFF;
        work[3] = (difficulty_approx >> 24) & 0xFF;
        work[4] = (difficulty_approx >> 32) & 0xFF;
        work[5] = (difficulty_approx >> 40) & 0xFF;
        work[6] = (difficulty_approx >> 48) & 0xFF;
        work[7] = (difficulty_approx >> 56) & 0xFF;

        return work;
    }

    // Add work to cumulative chain work
    void AddChainWork(const uint256& work) {
        // Simple addition (with overflow handling)
        bool carry = false;
        for (int i = 0; i < 32; i++) {
            uint16_t sum = chain_state_.chain_work[i] + work[i];
            if (carry) sum++;
            chain_state_.chain_work[i] = sum & 0xFF;
            carry = (sum > 0xFF);
        }
    }
};

// ============================================================================
// Blockchain Public Interface
// ============================================================================

Blockchain::Blockchain(std::shared_ptr<BlockchainDB> db)
    : impl_(std::make_unique<Impl>(db)) {}

Blockchain::~Blockchain() = default;

Result<void> Blockchain::Initialize() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Load chain state
    auto state_result = impl_->LoadChainState();
    if (state_result.IsError()) {
        return state_result;
    }

    // Check if we have genesis block
    if (impl_->chain_state_.best_height == 0 &&
        std::all_of(impl_->chain_state_.best_block_hash.begin(),
                   impl_->chain_state_.best_block_hash.end(),
                   [](uint8_t b) { return b == 0; })) {
        // Create and store genesis block
        Block genesis = CreateGenesisBlock();
        auto add_result = AddBlock(genesis);
        if (add_result.IsError()) {
            return Result<void>::Error("Failed to create genesis block: " + add_result.error);
        }
    }

    // Load UTXO set (TODO: optimize this for large chains)
    auto utxo_result = impl_->LoadUTXOSet();
    if (utxo_result.IsError()) {
        return utxo_result;
    }

    // Initialize mempool
    impl_->mempool_ = std::make_unique<Mempool>();

    return Result<void>::Ok();
}

// ------------------------------------------------------------------------
// Block Operations
// ------------------------------------------------------------------------

Result<void> Blockchain::AddBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    uint256 block_hash = block.GetHash();

    // Check if block already exists
    if (impl_->db_->HasBlock(block_hash)) {
        return Result<void>::Error("Block already exists");
    }

    // Validate block
    auto validate_result = ValidateBlock(block);
    if (validate_result.IsError()) {
        return validate_result;
    }

    // Begin database batch for atomic update
    impl_->db_->BeginBatch();

    // Store block
    auto store_result = impl_->db_->StoreBlock(block);
    if (store_result.IsError()) {
        impl_->db_->AbortBatch();
        return store_result;
    }

    // Calculate block index metadata
    uint64_t height = impl_->chain_state_.best_height + 1;

    // For genesis block, height is 0
    if (std::all_of(block.header.prev_block_hash.begin(),
                   block.header.prev_block_hash.end(),
                   [](uint8_t b) { return b == 0; })) {
        height = 0;
    }

    BlockIndex index;
    index.hash = block_hash;
    index.height = height;
    index.prev_hash = block.header.prev_block_hash;
    index.timestamp = block.header.timestamp;
    index.bits = block.header.bits;
    index.chain_work = impl_->CalculateChainWork(block.header.bits);
    index.tx_count = block.transactions.size();
    index.size = block.GetSerializedSize();
    index.file_pos = 0;

    // Store block index
    auto index_result = impl_->db_->StoreBlockIndex(index);
    if (index_result.IsError()) {
        impl_->db_->AbortBatch();
        return index_result;
    }

    // Store height mapping
    auto height_result = impl_->db_->StoreBlockHeight(height, block_hash);
    if (height_result.IsError()) {
        impl_->db_->AbortBatch();
        return height_result;
    }

    // Store transactions
    for (const auto& tx : block.transactions) {
        auto tx_result = impl_->db_->StoreTransaction(tx);
        if (tx_result.IsError()) {
            impl_->db_->AbortBatch();
            return tx_result;
        }
    }

    // Apply block to UTXO set
    auto utxo_result = impl_->ApplyBlockToUTXO(block);
    if (utxo_result.IsError()) {
        impl_->db_->AbortBatch();
        return utxo_result;
    }

    // Update chain state
    impl_->chain_state_.best_block_hash = block_hash;
    impl_->chain_state_.best_height = height;
    impl_->AddChainWork(index.chain_work);
    impl_->chain_state_.total_transactions += block.transactions.size();

    // Update supply with block reward
    for (const auto& tx : block.transactions) {
        if (tx.IsCoinbase()) {
            impl_->chain_state_.total_supply += tx.GetTotalOutputValue();
        }
    }

    // Save chain state
    auto save_result = impl_->SaveChainState();
    if (save_result.IsError()) {
        impl_->db_->AbortBatch();
        return save_result;
    }

    // Update best block in database
    auto update_result = impl_->db_->UpdateBestBlock(block_hash, height);
    if (update_result.IsError()) {
        impl_->db_->AbortBatch();
        return update_result;
    }

    // Commit batch
    auto commit_result = impl_->db_->CommitBatch();
    if (commit_result.IsError()) {
        return commit_result;
    }

    // Remove confirmed transactions from mempool
    if (impl_->mempool_) {
        impl_->mempool_->RemoveBlockTransactions(block);
    }

    // Notify callbacks
    for (const auto& callback : impl_->block_callbacks_) {
        callback(block);
    }

    // Notify transaction callbacks for each transaction in the block
    for (const auto& tx : block.transactions) {
        for (const auto& callback : impl_->tx_callbacks_) {
            callback(tx);
        }
    }

    return Result<void>::Ok();
}

Result<Block> Blockchain::GetBlock(const uint256& hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->GetBlock(hash);
}

Result<Block> Blockchain::GetBlockByHeight(uint64_t height) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->GetBlockByHeight(height);
}

Result<BlockHeader> Blockchain::GetBlockHeader(const uint256& hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto block_result = impl_->db_->GetBlock(hash);
    if (block_result.IsError()) {
        return Result<BlockHeader>::Error(block_result.error);
    }

    return Result<BlockHeader>::Ok(block_result.value->header);
}

Result<BlockHeader> Blockchain::GetBlockHeaderByHeight(uint64_t height) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto block_result = impl_->db_->GetBlockByHeight(height);
    if (block_result.IsError()) {
        return Result<BlockHeader>::Error(block_result.error);
    }

    return Result<BlockHeader>::Ok(block_result.value->header);
}

bool Blockchain::HasBlock(const uint256& hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->HasBlock(hash);
}

Result<Block> Blockchain::GetBestBlock() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->GetBlock(impl_->chain_state_.best_block_hash);
}

uint256 Blockchain::GetBestBlockHash() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->chain_state_.best_block_hash;
}

uint64_t Blockchain::GetBestHeight() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->chain_state_.best_height;
}

// ------------------------------------------------------------------------
// Chain State
// ------------------------------------------------------------------------

uint256 Blockchain::GetChainWork() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->chain_state_.chain_work;
}

uint64_t Blockchain::GetTotalTransactions() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->chain_state_.total_transactions;
}

uint64_t Blockchain::GetTotalSupply() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->chain_state_.total_supply;
}

double Blockchain::GetDifficulty() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto block_result = GetBestBlock();
    if (block_result.IsError()) {
        return 0.0;
    }

    return DifficultyCalculator::GetDifficulty(block_result.value->header.bits);
}

double Blockchain::GetNetworkHashRate() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    uint64_t current_height = impl_->chain_state_.best_height;
    if (current_height < 2) {
        return 0.0; // Not enough blocks to calculate hashrate
    }

    // Use the last 120 blocks (approximately 4 hours at 2-minute blocks) for estimation
    const uint64_t sample_blocks = std::min(static_cast<uint64_t>(120), current_height);
    const uint64_t start_height = current_height - sample_blocks;

    auto start_block_result = impl_->db_->GetBlockByHeight(start_height);
    auto end_block_result = impl_->db_->GetBlockByHeight(current_height);

    if (start_block_result.IsError() || end_block_result.IsError()) {
        return 0.0;
    }

    // Calculate time difference
    uint64_t time_diff = end_block_result.value->header.timestamp -
                         start_block_result.value->header.timestamp;

    if (time_diff == 0) {
        return 0.0;
    }

    // Get current difficulty
    double difficulty = DifficultyCalculator::GetDifficulty(end_block_result.value->header.bits);

    // Network hashrate (hashes/second) = (difficulty × blocks) / time
    // For RandomX, difficulty represents the hash complexity
    double hashrate = (difficulty * sample_blocks) / static_cast<double>(time_diff);

    return hashrate;
}

// ------------------------------------------------------------------------
// Block Validation
// ------------------------------------------------------------------------

Result<void> Blockchain::ValidateBlock(const Block& block) const {
    // Basic structural validation
    if (block.transactions.empty()) {
        return Result<void>::Error("Block has no transactions");
    }

    // First transaction must be coinbase
    if (!block.transactions[0].IsCoinbase()) {
        return Result<void>::Error("First transaction is not coinbase");
    }

    // Only first transaction can be coinbase
    for (size_t i = 1; i < block.transactions.size(); i++) {
        if (block.transactions[i].IsCoinbase()) {
            return Result<void>::Error("Non-first transaction is coinbase");
        }
    }

    // Verify merkle root
    uint256 calculated_merkle = block.CalculateMerkleRoot();
    if (calculated_merkle != block.header.merkle_root) {
        return Result<void>::Error("Invalid merkle root");
    }

    // TODO: Add more validation:
    // - Proof of work
    // - Timestamp validity
    // - Difficulty check
    // - Transaction validation
    // - Script validation

    return Result<void>::Ok();
}

bool Blockchain::IsOnMainChain(const uint256& block_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Get block index
    auto index_result = impl_->db_->GetBlockIndex(block_hash);
    if (index_result.IsError()) {
        return false;
    }

    // Check if this block is at the same height as the main chain
    auto main_hash_result = impl_->db_->GetBlockHash(index_result.value->height);
    if (main_hash_result.IsError()) {
        return false;
    }

    return *main_hash_result.value == block_hash;
}

uint64_t Blockchain::GetBlockConfirmations(const uint256& block_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    if (!IsOnMainChain(block_hash)) {
        return 0;
    }

    auto index_result = impl_->db_->GetBlockIndex(block_hash);
    if (index_result.IsError()) {
        return 0;
    }

    uint64_t block_height = index_result.value->height;
    uint64_t best_height = impl_->chain_state_.best_height;

    if (best_height < block_height) {
        return 0;
    }

    return (best_height - block_height) + 1;
}

// ------------------------------------------------------------------------
// Chain Reorganization (Stubs)
// ------------------------------------------------------------------------

Result<void> Blockchain::Reorganize(const std::vector<Block>& new_chain) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    if (new_chain.empty()) {
        return Result<void>::Error("Cannot reorganize to empty chain");
    }

    // Find fork point
    uint64_t fork_height = 0;
    for (size_t i = 0; i < new_chain.size(); ++i) {
        const auto& block = new_chain[i];
        auto existing_block = GetBlockByHeight(i);

        if (!existing_block.IsOk() || existing_block.GetValue().GetHash() != block.GetHash()) {
            fork_height = i;
            break;
        }
    }

    uint64_t current_height = GetBestHeight();

    // **51% Attack Protection: Validate reorganization depth**
    auto reorg_check = ChainValidator::ValidateReorgDepth(current_height, fork_height);
    if (!reorg_check.IsOk()) {
        return reorg_check; // Reject deep reorganizations
    }

    // **51% Attack Protection: Check checkpoints**
    for (size_t i = 0; i < new_chain.size(); ++i) {
        const auto& block = new_chain[i];
        uint64_t height = i; // Height is the index in the chain

        if (ChainValidator::IsCheckpoint(height, block.GetHash())) {
            // Checkpoint validation passed
            continue;
        }
        // If there's a checkpoint at this height but hash doesn't match, reject
        const auto& checkpoints = ChainValidator::GetCheckpoints();
        if (checkpoints.find(height) != checkpoints.end()) {
            return Result<void>::Error(
                "Block at checkpoint height " + std::to_string(height) +
                " has incorrect hash. Expected checkpoint hash."
            );
        }
    }

    // Log deep reorganization warning
    uint64_t reorg_depth = current_height - fork_height;
    if (reorg_depth >= consensus::DEEP_REORG_WARNING_THRESHOLD) {
        // TODO: Add logging system
        // LOG_WARNING("Deep reorganization: " + std::to_string(reorg_depth) +
        //             " blocks from height " + std::to_string(fork_height));
    }

    // Disconnect blocks from current chain back to fork point
    for (uint64_t h = current_height; h > fork_height; --h) {
        auto block_result = GetBlockByHeight(h);
        if (!block_result.IsOk()) {
            return Result<void>::Error("Failed to get block at height " + std::to_string(h));
        }
        const auto& block = block_result.GetValue();

        // Reverse UTXO changes (add spent outputs, remove new outputs)
        for (const auto& tx : block.transactions) {
            // Remove outputs created by this transaction
            for (size_t i = 0; i < tx.outputs.size(); ++i) {
                OutPoint outpoint{tx.GetHash(), static_cast<uint32_t>(i)};
                impl_->utxo_set_.erase(outpoint);
            }

            // Restore spent outputs (except coinbase)
            if (!tx.IsCoinbase()) {
                // TODO: Need to restore spent UTXOs from database
                // This requires storing spent outputs for reorganization support
            }
        }
    }

    // Connect blocks from new chain
    for (size_t i = fork_height; i < new_chain.size(); ++i) {
        const auto& block = new_chain[i];

        // Validate block before adding
        auto validation = ValidateBlock(block);
        if (!validation.IsOk()) {
            return validation;
        }

        // Add block to chain
        auto add_result = AddBlock(block);
        if (!add_result.IsOk()) {
            return add_result;
        }
    }

    return Result<void>::Ok();
}

Result<uint256> Blockchain::FindForkPoint(const uint256& hash1, const uint256& hash2) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Simple approach: Collect all ancestors of hash1 into a set
    std::unordered_set<uint256, uint256_hash> chain1_ancestors;
    uint256 current_hash = hash1;

    // Walk chain1 backwards, collecting all hashes
    while (true) {
        chain1_ancestors.insert(current_hash);

        auto block_result = impl_->db_->GetBlock(current_hash);
        if (block_result.IsError()) {
            break;  // Reached end of chain
        }

        auto block = block_result.GetValue();
        current_hash = block.header.prev_block_hash;

        // Stop if we reach genesis (prev_block_hash is all zeros)
        uint256 zero_hash;
        zero_hash.fill(0);
        if (current_hash == zero_hash) {
            chain1_ancestors.insert(current_hash);
            break;
        }
    }

    // Now walk chain2 backwards until we find a hash in chain1's ancestors
    current_hash = hash2;
    while (true) {
        // Check if this hash is in chain1
        if (chain1_ancestors.count(current_hash) > 0) {
            // Found the fork point!
            return Result<uint256>::Ok(current_hash);
        }

        auto block_result = impl_->db_->GetBlock(current_hash);
        if (block_result.IsError()) {
            return Result<uint256>::Error("Cannot find fork point - chains do not intersect");
        }

        auto block = block_result.GetValue();
        current_hash = block.header.prev_block_hash;

        // Stop if we reach genesis
        uint256 zero_hash;
        zero_hash.fill(0);
        if (current_hash == zero_hash) {
            // Genesis is the fork point
            return Result<uint256>::Ok(current_hash);
        }
    }
}

Result<std::vector<Block>> Blockchain::GetBlocksFromHeight(uint64_t start_height, size_t count) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::vector<Block> blocks;
    blocks.reserve(count);

    for (size_t i = 0; i < count; i++) {
        auto result = impl_->db_->GetBlockByHeight(start_height + i);
        if (result.IsError()) {
            break;
        }
        blocks.push_back(*result.value);
    }

    return Result<std::vector<Block>>::Ok(std::move(blocks));
}

// ------------------------------------------------------------------------
// Transaction Queries
// ------------------------------------------------------------------------

Result<Transaction> Blockchain::GetTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->GetTransaction(tx_hash);
}

bool Blockchain::HasTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->db_->HasTransaction(tx_hash);
}

uint64_t Blockchain::GetTransactionConfirmations(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Get transaction from database
    auto tx_result = impl_->db_->GetTransaction(tx_hash);
    if (tx_result.IsError()) {
        return 0; // Transaction not found = 0 confirmations
    }

    // Find which block contains this transaction
    // We need to look up the transaction's block hash from the database
    // For now, we'll search through recent blocks (this is inefficient but works)
    uint64_t current_height = impl_->chain_state_.best_height;

    // Search backwards from current height
    for (uint64_t height = current_height; height > 0 && (current_height - height) < 1000; --height) {
        auto block_result = impl_->db_->GetBlockByHeight(height);
        if (block_result.IsOk()) {
            const auto& block = block_result.value;
            // Check if transaction is in this block
            for (const auto& tx : block->transactions) {
                if (tx.GetHash() == tx_hash) {
                    // Found the transaction in this block
                    return current_height - height + 1;
                }
            }
        }
    }

    // Transaction exists but not in any block (in mempool)
    return 0;
}

Result<Block> Blockchain::GetTransactionBlock(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Get transaction from database
    auto tx_result = impl_->db_->GetTransaction(tx_hash);
    if (tx_result.IsError()) {
        return Result<Block>::Error("Transaction not found");
    }

    // Search for the block containing this transaction
    uint64_t current_height = impl_->chain_state_.best_height;

    // Search backwards from current height
    for (uint64_t height = current_height; height > 0 && (current_height - height) < 1000; --height) {
        auto block_result = impl_->db_->GetBlockByHeight(height);
        if (block_result.IsOk()) {
            const auto& block = block_result.value;
            // Check if transaction is in this block
            for (const auto& tx : block->transactions) {
                if (tx.GetHash() == tx_hash) {
                    return Result<Block>::Ok(*block);
                }
            }
        }
    }

    return Result<Block>::Error("Transaction block not found");
}

// ------------------------------------------------------------------------
// UTXO Queries
// ------------------------------------------------------------------------

std::optional<TxOut> Blockchain::GetUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->utxo_set_.find(outpoint);
    if (it != impl_->utxo_set_.end()) {
        return it->second;
    }

    // Try database
    auto result = impl_->db_->GetUTXO(outpoint);
    if (result.IsOk()) {
        return *result.value;
    }

    return std::nullopt;
}

bool Blockchain::HasUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    if (impl_->utxo_set_.find(outpoint) != impl_->utxo_set_.end()) {
        return true;
    }

    return impl_->db_->HasUTXO(outpoint);
}

const UTXOSet& Blockchain::GetUTXOSet() const {
    // TODO: Implement UTXOSet properly
    // For now, throw an exception as this isn't implemented yet
    throw std::runtime_error("UTXOSet not yet implemented");
}

std::vector<std::pair<OutPoint, TxOut>> Blockchain::GetUTXOsForAddress(const std::string& address) const {
    // TODO: Implement address UTXO lookup
    return {};
}

uint64_t Blockchain::GetAddressBalance(const std::string& address) const {
    auto utxos = GetUTXOsForAddress(address);
    uint64_t balance = 0;
    for (const auto& [outpoint, output] : utxos) {
        balance += output.value;
    }
    return balance;
}

// ------------------------------------------------------------------------
// Block Mining Support (Stubs)
// ------------------------------------------------------------------------

Result<Block> Blockchain::GetBlockTemplate(const PublicKey& miner_pubkey) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    Block template_block;

    // Calculate the height for this new block
    uint64_t block_height = impl_->chain_state_.best_height + 1;

    // 1. Set header fields
    template_block.header.version = 1;
    template_block.header.prev_block_hash = impl_->chain_state_.best_block_hash;
    template_block.header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    template_block.header.nonce = 0;  // Miner will modify this

    // Calculate next difficulty target
    if (block_height > 0) {
        auto best_block_result = GetBestBlock();
        if (best_block_result.IsOk()) {
            template_block.header.bits = DifficultyCalculator::GetNextWorkRequired(
                best_block_result.GetValue().header, *this);
        } else {
            // Fallback to previous difficulty if we can't get best block
            template_block.header.bits = 0x1e0fffff;  // Initial difficulty
        }
    } else {
        // Genesis block
        template_block.header.bits = 0x1e0fffff;  // Initial difficulty
    }

    // 2. Create coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    // Coinbase input (no previous transaction)
    TxIn coinbase_input;
    coinbase_input.prev_tx_hash.fill(0);  // All zeros for coinbase
    coinbase_input.prev_tx_index = 0xFFFFFFFF;  // Max value for coinbase
    coinbase_input.sequence = 0xFFFFFFFF;

    // Coinbase unlocking script: block height + extra nonce
    std::vector<uint8_t> coinbase_script;

    // Push block height (BIP34)
    uint64_t height = block_height;
    if (height <= 16) {
        coinbase_script.push_back(static_cast<uint8_t>(height));
    } else {
        // Encode height as little-endian
        std::vector<uint8_t> height_bytes;
        while (height > 0) {
            height_bytes.push_back(static_cast<uint8_t>(height & 0xFF));
            height >>= 8;
        }
        coinbase_script.push_back(static_cast<uint8_t>(height_bytes.size()));
        coinbase_script.insert(coinbase_script.end(), height_bytes.begin(), height_bytes.end());
    }

    // Add extra nonce space (8 bytes)
    for (int i = 0; i < 8; i++) {
        coinbase_script.push_back(0);
    }

    coinbase_input.script_sig = Script(coinbase_script);
    coinbase.inputs.push_back(coinbase_input);

    // Coinbase output to miner (will set value after calculating fees)
    TxOut coinbase_output;
    coinbase_output.value = 0;  // Will be set after adding mempool transactions
    coinbase_output.script_pubkey = Script::CreateP2PK(miner_pubkey);
    coinbase.outputs.push_back(coinbase_output);

    template_block.transactions.push_back(coinbase);

    // 3. Add transactions from mempool
    constexpr size_t MAX_BLOCK_SIZE = 8 * 1024 * 1024;  // 8 MB
    size_t current_size = template_block.GetSerializedSize();
    uint64_t total_fees = 0;

    if (impl_->mempool_) {
        // Get transactions sorted by fee (descending)
        auto mempool_txs = impl_->mempool_->GetTransactionsForMining(10000);

        for (const auto& tx : mempool_txs) {
            // Skip coinbase transactions (shouldn't be in mempool anyway)
            if (tx.IsCoinbase()) {
                continue;
            }

            size_t tx_size = tx.GetSerializedSize();

            // Check if adding this transaction would exceed block size
            if (current_size + tx_size > MAX_BLOCK_SIZE) {
                break;  // Block is full
            }

            // Calculate transaction fee
            uint64_t input_value = 0;
            bool can_calculate_fee = true;

            for (const auto& input : tx.inputs) {
                OutPoint outpoint;
                outpoint.tx_hash = input.prev_tx_hash;
                outpoint.index = input.prev_tx_index;

                auto utxo_opt = GetUTXO(outpoint);
                if (utxo_opt.has_value()) {
                    input_value += utxo_opt.value().value;
                } else {
                    can_calculate_fee = false;
                    break;
                }
            }

            if (!can_calculate_fee) {
                continue;  // Skip transactions with missing inputs
            }

            uint64_t output_value = tx.GetTotalOutputValue();
            if (input_value < output_value) {
                continue;  // Invalid transaction (outputs exceed inputs)
            }

            uint64_t fee = input_value - output_value;
            total_fees += fee;

            // Add transaction to block
            template_block.transactions.push_back(tx);
            current_size += tx_size;
        }
    }

    // 4. Set coinbase output value (block reward + fees)
    uint64_t block_reward = GetBlockReward(block_height);
    template_block.transactions[0].outputs[0].value = block_reward + total_fees;

    // 5. Calculate merkle root
    template_block.header.merkle_root = template_block.CalculateMerkleRoot();

    return Result<Block>::Ok(template_block);
}

Result<void> Blockchain::SubmitBlock(const Block& block) {
    return AddBlock(block);
}

// ------------------------------------------------------------------------
// Statistics
// ------------------------------------------------------------------------

Blockchain::BlockchainInfo Blockchain::GetInfo() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    BlockchainInfo info;
    info.height = impl_->chain_state_.best_height;
    info.best_block_hash = impl_->chain_state_.best_block_hash;
    info.chain_work = impl_->chain_state_.chain_work;
    info.difficulty = GetDifficulty();
    info.total_transactions = impl_->chain_state_.total_transactions;
    info.total_supply = impl_->chain_state_.total_supply;
    info.utxo_count = impl_->chain_state_.utxo_count;

    // Calculate verification progress based on block timestamp vs current time
    // Progress = min(1.0, (best_block_timestamp / current_time))
    if (impl_->chain_state_.best_height > 0) {
        auto best_block_result = GetBlockByHeight(impl_->chain_state_.best_height);
        if (best_block_result.IsOk()) {
            uint64_t best_timestamp = best_block_result.GetValue().header.timestamp;
            uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));

            if (current_time > 0 && best_timestamp > 0) {
                double progress = static_cast<double>(best_timestamp) / static_cast<double>(current_time);
                info.verification_progress = std::min(1.0, progress);
            } else {
                info.verification_progress = 1.0;
            }
        } else {
            info.verification_progress = 1.0;
        }
    } else {
        info.verification_progress = 0.0;  // No blocks yet
    }

    info.pruned = false;

    return info;
}

Result<Blockchain::BlockStats> Blockchain::GetBlockStats(const uint256& block_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Get the block
    auto block_result = GetBlock(block_hash);
    if (!block_result.IsOk()) {
        return Result<BlockStats>::Error("Block not found: " + block_result.error);
    }

    auto block = block_result.GetValue();

    // Get block index to retrieve height
    auto index_result = impl_->db_->GetBlockIndex(block_hash);
    if (!index_result.IsOk()) {
        return Result<BlockStats>::Error("Block index not found: " + index_result.error);
    }

    auto index = index_result.GetValue();

    BlockStats stats;
    stats.height = index.height;
    stats.hash = block.GetHash();
    stats.timestamp = block.header.timestamp;
    stats.tx_count = static_cast<uint32_t>(block.transactions.size());

    // Calculate total fees (sum of all transaction fees)
    stats.total_fees = 0;
    for (size_t i = 1; i < block.transactions.size(); i++) {  // Skip coinbase
        const auto& tx = block.transactions[i];

        // Calculate input value
        uint64_t input_value = 0;
        for (const auto& input : tx.inputs) {
            OutPoint outpoint{input.prev_tx_hash, input.prev_tx_index};
            auto utxo_result = impl_->db_->GetUTXO(outpoint);
            if (utxo_result.IsOk()) {
                input_value += utxo_result.GetValue().value;
            }
        }

        // Calculate output value
        uint64_t output_value = 0;
        for (const auto& output : tx.outputs) {
            output_value += output.value;
        }

        // Fee is difference
        if (input_value >= output_value) {
            stats.total_fees += (input_value - output_value);
        }
    }

    // Get block reward from consensus
    stats.block_reward = GetBlockReward(index.height);

    // Calculate block size and weight
    auto serialized = block.Serialize();
    stats.size = static_cast<uint32_t>(serialized.size());
    stats.weight = static_cast<uint32_t>(serialized.size() * 4);  // Weight = size * 4 (no SegWit)

    // Calculate difficulty from bits
    stats.difficulty = DifficultyCalculator::GetDifficulty(block.header.bits);

    return Result<BlockStats>::Ok(stats);
}

Result<Blockchain::BlockStats> Blockchain::GetBlockStatsByHeight(uint64_t height) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Get block by height first
    auto block_result = GetBlockByHeight(height);
    if (!block_result.IsOk()) {
        return Result<BlockStats>::Error("Block at height " + std::to_string(height) + " not found");
    }

    // Use GetBlockStats with the hash
    auto block = block_result.GetValue();
    return GetBlockStats(block.GetHash());
}

// ------------------------------------------------------------------------
// Mempool Access (Stubs)
// ------------------------------------------------------------------------

Mempool& Blockchain::GetMempool() {
    if (!impl_->mempool_) {
        throw std::runtime_error("Mempool not initialized - call Initialize() first");
    }
    return *impl_->mempool_;
}

const Mempool& Blockchain::GetMempool() const {
    if (!impl_->mempool_) {
        throw std::runtime_error("Mempool not initialized - call Initialize() first");
    }
    return *impl_->mempool_;
}

Result<void> Blockchain::AddToMempool(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    if (!impl_->mempool_) {
        return Result<void>::Error("Mempool not initialized");
    }

    // Add transaction to mempool
    auto add_result = impl_->mempool_->AddTransaction(tx);
    if (add_result.IsError()) {
        return add_result;
    }

    // Notify transaction callbacks
    for (const auto& callback : impl_->tx_callbacks_) {
        callback(tx);
    }

    return Result<void>::Ok();
}

// ------------------------------------------------------------------------
// Callbacks
// ------------------------------------------------------------------------

void Blockchain::RegisterBlockCallback(BlockCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->block_callbacks_.push_back(std::move(callback));
}

void Blockchain::RegisterTransactionCallback(TransactionCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->tx_callbacks_.push_back(std::move(callback));
}

} // namespace intcoin
