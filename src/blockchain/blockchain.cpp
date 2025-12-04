/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Blockchain Implementation
 */

#include "intcoin/blockchain.h"
#include "intcoin/consensus.h"
#include "intcoin/util.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
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

        // Restore spent outputs
        for (const auto& tx : block.transactions) {
            if (tx.IsCoinbase()) continue;

            for (const auto& input : tx.inputs) {
                OutPoint outpoint;
                outpoint.tx_hash = input.prev_tx_hash;
                outpoint.index = input.prev_tx_index;

                // TODO: Need to restore the original output
                // This requires storing spent outputs or being able to look them up
                // For now, this is a stub
            }
        }

        return Result<void>::Ok();
    }

    // Calculate chain work for a block
    uint256 CalculateChainWork(uint32_t bits) const {
        // Chain work = difficulty = max_target / current_target
        // For simplicity, we'll use a basic calculation
        uint256 work{};

        // Extract exponent from compact format
        uint32_t exponent = bits >> 24;
        (void)exponent;  // Suppress unused warning for now

        // Calculate work (inverse of target)
        // TODO: Implement proper chain work calculation
        work[0] = 1;

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
    // TODO: Implement network hashrate calculation
    return 0.0;
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
    // TODO: Implement chain reorganization
    return Result<void>::Error("Not implemented");
}

Result<uint256> Blockchain::FindForkPoint(const uint256& hash1, const uint256& hash2) const {
    // TODO: Implement fork point finding
    return Result<uint256>::Error("Not implemented");
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
    // TODO: Implement block template generation
    return Result<Block>::Error("Not implemented");
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
    info.verification_progress = 1.0;  // TODO: Calculate actual progress
    info.pruned = false;

    return info;
}

Result<Blockchain::BlockStats> Blockchain::GetBlockStats(const uint256& block_hash) const {
    // TODO: Implement block stats
    return Result<BlockStats>::Error("Not implemented");
}

Result<Blockchain::BlockStats> Blockchain::GetBlockStatsByHeight(uint64_t height) const {
    // TODO: Implement block stats by height
    return Result<BlockStats>::Error("Not implemented");
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
