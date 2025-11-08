// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/blockchain.h"
#include "intcoin/crypto.h"
#include <stdexcept>
#include <ctime>

namespace intcoin {

Blockchain::Blockchain()
    : use_database_(false),
      consensus_params_(),  // Default mainnet parameters
      difficulty_calc_(consensus_params_),
      fork_detector_(consensus_params_),
      checkpoint_system_(consensus_params_),
      chain_height_(0),
      total_work_(0) {

    // Add genesis checkpoint
    Block genesis = create_genesis_block();
    Hash256 genesis_hash = genesis.get_hash();
    checkpoint_system_.add_checkpoint(0, genesis_hash);

    blocks_[genesis_hash] = genesis;
    block_index_[0] = genesis_hash;
    best_block_ = genesis_hash;
    chain_height_ = 0;
}

Blockchain::Blockchain(const std::string& datadir)
    : use_database_(false),
      consensus_params_(),  // Default mainnet parameters
      difficulty_calc_(consensus_params_),
      fork_detector_(consensus_params_),
      checkpoint_system_(consensus_params_),
      chain_height_(0),
      total_work_(0) {

    // Initialize databases
    if (init_databases(datadir)) {
        use_database_ = true;

        // Try to load blockchain state from database
        auto best_height = block_db_.get_best_height();
        if (best_height) {
            // Database exists, load from it
            chain_height_ = *best_height;
            auto best_hash = block_db_.get_block_hash(chain_height_);
            if (best_hash) {
                best_block_ = *best_hash;
            }
        } else {
            // New database, create genesis block
            Block genesis = create_genesis_block();
            Hash256 genesis_hash = genesis.get_hash();
            checkpoint_system_.add_checkpoint(0, genesis_hash);
            add_block(genesis);
        }
    } else {
        // Fall back to in-memory mode
        Block genesis = create_genesis_block();
        Hash256 genesis_hash = genesis.get_hash();
        checkpoint_system_.add_checkpoint(0, genesis_hash);

        blocks_[genesis_hash] = genesis;
        block_index_[0] = genesis_hash;
        best_block_ = genesis_hash;
        chain_height_ = 0;
    }
}

bool Blockchain::init_databases(const std::string& datadir) {
    bool success = true;

    // Open block database
    if (!block_db_.open(datadir)) {
        success = false;
    }

    // Open UTXO database
    if (!utxo_db_.open(datadir)) {
        success = false;
    }

    // Open transaction index database
    if (!tx_db_.open(datadir)) {
        success = false;
    }

    return success;
}

Block Blockchain::create_genesis_block() {
    return GenesisBlock::create_mainnet();
}

bool Blockchain::add_block(const Block& block) {
    Hash256 block_hash = block.get_hash();

    // Check if block already exists
    if (blocks_.find(block_hash) != blocks_.end()) {
        return false;
    }

    // Verify previous block exists (unless this is genesis)
    if (block.header.previous_block_hash != Hash256{}) {
        if (blocks_.find(block.header.previous_block_hash) == blocks_.end()) {
            return false;
        }
    }

    // Verify block structure
    if (!block.validate()) {
        return false;
    }

    // Find height of this block first (before adding)
    uint32_t prev_height = 0;
    for (const auto& [height, hash] : block_index_) {
        if (hash == block.header.previous_block_hash) {
            prev_height = height;
            break;
        }
    }
    uint32_t new_height = prev_height + 1;

    // Verify checkpoint (if this height has a checkpoint)
    if (!checkpoint_system_.verify_checkpoint(new_height, block_hash)) {
        return false;  // Block doesn't match checkpoint
    }

    // Add block to chain storage
    blocks_[block_hash] = block;

    // Check if this creates a fork or extends the main chain
    if (new_height > chain_height_) {
        // Simple case: extends main chain
        block_index_[new_height] = block_hash;
        best_block_ = block_hash;
        chain_height_ = new_height;
        update_utxo_set(block, true);
        update_address_index(block, true);

        // Persist to database if enabled
        if (use_database_) {
            std::vector<uint8_t> block_data;  // Placeholder
            block_db_.write_block(block_hash, new_height, block_data);
            block_db_.set_best_block(block_hash, new_height);
        }
    } else if (new_height == chain_height_) {
        // Potential fork at same height - need to check accumulated work
        return handle_potential_fork(block, new_height);
    } else {
        // Block builds on an older chain - potential reorganization needed
        // Detect forks and select best chain
        auto forks = fork_detector_.detect_forks(block_index_, blocks_);

        if (forks.size() > 1) {
            // Multiple competing chains found
            auto best_chain = fork_detector_.select_best_chain(forks);

            // Check if best chain is different from current chain
            if (best_chain.tip_hash != best_block_) {
                // Need to reorganize to the better chain
                auto reorg_info = consensus::ReorgHandler::calculate_reorg(
                    best_block_, best_chain.tip_hash, blocks_);

                // Validate reorganization
                if (consensus::ReorgHandler::validate_reorg(reorg_info, consensus_params_.max_reorg_depth)) {
                    // Check if reorg violates any checkpoints (use common ancestor)
                    if (!checkpoint_system_.reorg_violates_checkpoint(
                            reorg_info.reorg_depth, best_chain.tip_hash)) {
                        // Perform the reorganization
                        return perform_reorganization(reorg_info);
                    }
                }
            }
        }

        // If no reorganization needed, just store the block as an orphan/side chain block
        // It might become part of the main chain later
    }

    return true;
}

Block Blockchain::get_block(const Hash256& hash) const {
    auto it = blocks_.find(hash);
    if (it == blocks_.end()) {
        throw std::runtime_error("Block not found");
    }
    return it->second;
}

Block Blockchain::get_block_by_height(uint32_t height) const {
    auto it = block_index_.find(height);
    if (it == block_index_.end()) {
        throw std::runtime_error("Block at height not found");
    }
    return get_block(it->second);
}

uint32_t Blockchain::get_height() const {
    return chain_height_;
}

Hash256 Blockchain::get_best_block_hash() const {
    return best_block_;
}

bool Blockchain::has_block(const Hash256& hash) const {
    return blocks_.find(hash) != blocks_.end();
}

std::optional<UTXO> Blockchain::get_utxo(const Hash256& tx_hash, uint32_t index) const {
    OutPoint key(tx_hash, index);
    auto it = utxo_set_.find(key);
    if (it == utxo_set_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Blockchain::verify_transactions(const Block& block) const {
    if (block.transactions.empty()) {
        return false;
    }

    // First transaction must be coinbase
    if (!block.transactions[0].is_coinbase()) {
        return false;
    }

    // Only first transaction can be coinbase
    for (size_t i = 1; i < block.transactions.size(); ++i) {
        if (block.transactions[i].is_coinbase()) {
            return false;
        }
    }

    // Verify each transaction
    for (const auto& tx : block.transactions) {
        if (!verify_transaction(tx)) {
            return false;
        }
    }

    return true;
}

bool Blockchain::verify_transaction(const Transaction& tx) const {
    // Basic validation
    if (!tx.validate_structure()) {
        return false;
    }

    // Coinbase transactions have special rules
    if (tx.is_coinbase()) {
        return true;
    }

    // For non-coinbase: verify inputs exist in UTXO set and signatures are valid
    for (size_t i = 0; i < tx.inputs.size(); ++i) {
        const auto& input = tx.inputs[i];
        auto utxo = get_utxo(input.previous_output.tx_hash, input.previous_output.index);
        if (!utxo) {
            return false;  // Input doesn't exist in UTXO set
        }

        // Verify signature against UTXO public key
        // Signature script should contain: signature + pubkey
        constexpr size_t DILITHIUM_SIG_SIZE = 4627;
        constexpr size_t DILITHIUM_PUBKEY_SIZE = 2592;

        if (input.script_sig.size() != DILITHIUM_SIG_SIZE + DILITHIUM_PUBKEY_SIZE) {
            return false;  // Invalid signature script
        }

        // Extract public key from signature script
        std::vector<uint8_t> pubkey_from_sig(
            input.script_sig.begin() + DILITHIUM_SIG_SIZE,
            input.script_sig.end()
        );

        // Verify pubkey matches UTXO script pubkey
        if (pubkey_from_sig != utxo->output.script_pubkey) {
            return false;  // Public key doesn't match UTXO
        }

        // Verify the signature itself
        if (!tx.verify_signature(i)) {
            return false;  // Invalid signature
        }
    }

    return true;
}

void Blockchain::update_utxo_set(const Block& block, bool connect) {
    if (connect) {
        // Prepare database batch if using persistent storage
        UTXODatabase::Batch db_batch;

        // Remove spent outputs
        for (const auto& tx : block.transactions) {
            if (!tx.is_coinbase()) {
                for (const auto& input : tx.inputs) {
                    OutPoint key(input.previous_output.tx_hash, input.previous_output.index);
                    utxo_set_.erase(key);

                    // Remove from database
                    if (use_database_) {
                        db_batch.spend_utxo(key);
                    }
                }
            }
        }

        // Add new outputs
        for (const auto& tx : block.transactions) {
            Hash256 tx_hash = tx.get_hash();
            for (uint32_t i = 0; i < tx.outputs.size(); ++i) {
                OutPoint outpoint(tx_hash, i);
                UTXO utxo;
                utxo.outpoint = outpoint;
                utxo.output = tx.outputs[i];
                utxo.height = chain_height_;
                utxo.is_coinbase = tx.is_coinbase();
                utxo_set_[outpoint] = utxo;

                // Add to database
                if (use_database_) {
                    db_batch.add_utxo(outpoint, tx.outputs[i], chain_height_);
                }
            }
        }

        // Apply database batch
        if (use_database_) {
            utxo_db_.apply_batch(db_batch);
        }
    } else {
        // Disconnect block (reorg)
        // Remove new outputs
        for (const auto& tx : block.transactions) {
            Hash256 tx_hash = tx.get_hash();
            for (uint32_t i = 0; i < tx.outputs.size(); ++i) {
                OutPoint key(tx_hash, i);
                utxo_set_.erase(key);

                // Remove from database
                if (use_database_) {
                    utxo_db_.erase_utxo(key);
                }
            }
        }

        // Add back spent outputs from inputs (reorg undo)
        // In a full implementation, would use undo data stored with each block
        // For now, note that this is a simplified implementation
        // Production blockchain would maintain BlockUndo data structures
        for (const auto& tx : block.transactions) {
            if (tx.is_coinbase()) {
                continue;  // Coinbase has no inputs to restore
            }

            for (const auto& input : tx.inputs) {
                // In production: would retrieve from undo data
                // For now: this is a placeholder showing the structure
                // Real implementation requires storing spent outputs in BlockUndo
                (void)input;  // Suppress warning - would use undo data here
            }
        }
    }
}

uint64_t Blockchain::calculate_block_reward(uint32_t height) {
    return Block::get_block_reward(height);
}

uint32_t Blockchain::calculate_next_difficulty(const Hash256& prev_block_hash) const {
    // Use the consensus DifficultyCalculator
    Block prev_block = get_block(prev_block_hash);
    return difficulty_calc_.calculate_next_difficulty(prev_block, block_index_, blocks_);
}

std::vector<UTXO> Blockchain::get_utxos_for_address(const std::string& address) const {
    std::vector<UTXO> result;

    // Search through all UTXOs
    for (const auto& [outpoint, utxo] : utxo_set_) {
        // Extract address from output
        auto addr = extract_address(utxo.output);
        if (addr && *addr == address) {
            result.push_back(utxo);
        }
    }

    return result;
}

std::optional<Transaction> Blockchain::get_transaction(const Hash256& tx_hash) const {
    // Search through all blocks for the transaction
    for (const auto& [block_hash, block] : blocks_) {
        for (const auto& tx : block.transactions) {
            if (tx.get_hash() == tx_hash) {
                return tx;
            }
        }
    }
    return std::nullopt;
}

std::vector<Transaction> Blockchain::scan_for_addresses(const std::vector<std::string>& addresses) const {
    std::vector<Transaction> result;
    std::set<std::string> addr_set(addresses.begin(), addresses.end());

    // Scan all blocks
    for (const auto& [block_hash, block] : blocks_) {
        for (const auto& tx : block.transactions) {
            bool involves_address = false;

            // Check outputs
            for (const auto& output : tx.outputs) {
                auto addr = extract_address(output);
                if (addr && addr_set.count(*addr)) {
                    involves_address = true;
                    break;
                }
            }

            if (involves_address) {
                result.push_back(tx);
            }
        }
    }

    return result;
}

void Blockchain::update_address_index(const Block& block, bool connect) {
    if (connect) {
        // Index new transactions
        for (const auto& tx : block.transactions) {
            Hash256 tx_hash = tx.get_hash();
            transactions_[tx_hash] = tx;

            // Index outputs by address
            for (uint32_t i = 0; i < tx.outputs.size(); ++i) {
                auto addr = extract_address(tx.outputs[i]);
                if (addr) {
                    OutPoint outpoint(tx_hash, i);
                    address_index_[*addr].push_back(outpoint);
                }
            }
        }
    } else {
        // Remove transactions from index
        for (const auto& tx : block.transactions) {
            Hash256 tx_hash = tx.get_hash();
            transactions_.erase(tx_hash);

            // Remove from address index
            for (uint32_t i = 0; i < tx.outputs.size(); ++i) {
                auto addr = extract_address(tx.outputs[i]);
                if (addr) {
                    OutPoint outpoint(tx_hash, i);
                    auto& outpoints = address_index_[*addr];
                    outpoints.erase(
                        std::remove(outpoints.begin(), outpoints.end(), outpoint),
                        outpoints.end()
                    );
                }
            }
        }
    }
}

std::optional<std::string> Blockchain::extract_address(const TxOutput& output) const {
    // In INTcoin, the script_pubkey contains the Dilithium public key
    // We need to convert it to an address
    // For now, this is a placeholder that would need crypto::Address integration

    if (output.script_pubkey.size() == DILITHIUM_PUBKEY_SIZE) {
        DilithiumPubKey pubkey;
        std::copy(output.script_pubkey.begin(), output.script_pubkey.end(), pubkey.begin());
        return crypto::Address::from_public_key(pubkey);
    }

    return std::nullopt;
}

// Fork handling implementation

bool Blockchain::handle_potential_fork(const Block& new_block, uint32_t new_height) {
    // Fork at same height as current tip
    // Compare accumulated work to decide which chain to follow

    Hash256 new_hash = new_block.get_hash();

    // Calculate work for both chains
    double current_work = fork_detector_.calculate_chain_work(best_block_, blocks_);
    double new_work = fork_detector_.calculate_chain_work(new_hash, blocks_);

    if (new_work > current_work) {
        // New chain has more work - switch to it
        Hash256 old_best = best_block_;

        // Update to new tip
        block_index_[new_height] = new_hash;
        best_block_ = new_hash;

        // Disconnect old tip and connect new tip
        Block old_block = blocks_[old_best];
        update_utxo_set(old_block, false);  // Disconnect
        update_address_index(old_block, false);

        update_utxo_set(new_block, true);  // Connect
        update_address_index(new_block, true);

        // Persist to database
        if (use_database_) {
            std::vector<uint8_t> block_data;
            block_db_.write_block(new_hash, new_height, block_data);
            block_db_.set_best_block(new_hash, new_height);
        }

        return true;
    }

    // Current chain has more work, keep it
    // New block is stored but not on main chain
    return true;
}

bool Blockchain::perform_reorganization(const consensus::ReorgHandler::ReorgInfo& reorg_info) {
    // Perform chain reorganization
    // 1. Disconnect blocks from old chain
    // 2. Connect blocks from new chain
    // 3. Update UTXO set and indexes
    // 4. Update best block and height

    // Disconnect blocks in reverse order
    for (auto it = reorg_info.disconnect_blocks.rbegin();
         it != reorg_info.disconnect_blocks.rend(); ++it) {

        const Hash256& block_hash = *it;
        Block block = blocks_[block_hash];

        // Disconnect from UTXO set
        update_utxo_set(block, false);
        update_address_index(block, false);

        // Remove from block index
        // Find and remove this block from the index
        for (auto index_it = block_index_.begin(); index_it != block_index_.end(); ++index_it) {
            if (index_it->second == block_hash) {
                block_index_.erase(index_it);
                break;
            }
        }
    }

    // Connect new blocks in forward order
    // Height starts from common ancestor + 1
    uint32_t height = chain_height_ - reorg_info.reorg_depth + 1;
    for (const auto& block_hash : reorg_info.connect_blocks) {
        Block block = blocks_[block_hash];

        // Connect to UTXO set
        update_utxo_set(block, true);
        update_address_index(block, true);

        // Add to block index
        block_index_[height] = block_hash;

        // Persist to database
        if (use_database_) {
            std::vector<uint8_t> block_data;
            block_db_.write_block(block_hash, height, block_data);
        }

        height++;
    }

    // Update chain state
    if (!reorg_info.connect_blocks.empty()) {
        best_block_ = reorg_info.connect_blocks.back();
        chain_height_ = height - 1;

        // Update best block in database
        if (use_database_) {
            block_db_.set_best_block(best_block_, chain_height_);
        }
    }

    return true;
}

} // namespace intcoin
