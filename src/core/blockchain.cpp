// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/blockchain.h"
#include "intcoin/crypto.h"
#include <stdexcept>
#include <ctime>

namespace intcoin {

Blockchain::Blockchain() : use_database_(false), chain_height_(0), total_work_(0) {
    // Create and add genesis block
    Block genesis = create_genesis_block();
    Hash256 genesis_hash = genesis.get_hash();

    blocks_[genesis_hash] = genesis;
    block_index_[0] = genesis_hash;
    best_block_ = genesis_hash;
    chain_height_ = 0;
}

Blockchain::Blockchain(const std::string& datadir)
    : use_database_(false), chain_height_(0), total_work_(0) {

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
            add_block(genesis);
        }
    } else {
        // Fall back to in-memory mode
        Block genesis = create_genesis_block();
        Hash256 genesis_hash = genesis.get_hash();

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

    // Add block to chain
    blocks_[block_hash] = block;

    // Find height of previous block
    uint32_t prev_height = 0;
    for (const auto& [height, hash] : block_index_) {
        if (hash == block.header.previous_block_hash) {
            prev_height = height;
            break;
        }
    }

    uint32_t new_height = prev_height + 1;
    block_index_[new_height] = block_hash;

    // Update chain if this is the best block
    if (new_height > chain_height_) {
        best_block_ = block_hash;
        chain_height_ = new_height;
        update_utxo_set(block, true);
        update_address_index(block, true);

        // Persist to database if enabled
        if (use_database_) {
            // Serialize block (simple approach: we'd need proper serialization)
            // For now, just store a placeholder
            // TODO: Implement proper block serialization
            std::vector<uint8_t> block_data;  // Placeholder

            block_db_.write_block(block_hash, new_height, block_data);
            block_db_.set_best_block(block_hash, new_height);
        }
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

    // For non-coinbase: verify inputs exist in UTXO set
    for (const auto& input : tx.inputs) {
        auto utxo = get_utxo(input.previous_output.tx_hash, input.previous_output.index);
        if (!utxo) {
            return false;
        }
        // TODO: Verify signature against UTXO public key
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
        // TODO: Add back spent outputs (would need to store them)
    }
}

uint64_t Blockchain::calculate_block_reward(uint32_t height) {
    return Block::get_block_reward(height);
}

uint32_t Blockchain::calculate_next_difficulty(const Hash256& prev_block_hash) const {
    // Difficulty adjustment every 2016 blocks
    const uint32_t difficulty_adjustment_interval = 2016;

    // Find height of previous block
    uint32_t prev_height = 0;
    for (const auto& [height, hash] : block_index_) {
        if (hash == prev_block_hash) {
            prev_height = height;
            break;
        }
    }

    uint32_t current_height = prev_height + 1;

    // Don't adjust on first block or before interval
    if (current_height % difficulty_adjustment_interval != 0) {
        return get_block(prev_block_hash).header.bits;
    }

    // TODO: Implement full difficulty adjustment calculation
    return get_block(prev_block_hash).header.bits;
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

} // namespace intcoin
