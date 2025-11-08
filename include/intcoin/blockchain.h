// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Blockchain state management and validation.

#ifndef INTCOIN_BLOCKCHAIN_H
#define INTCOIN_BLOCKCHAIN_H

#include "block.h"
#include "transaction.h"
#include "primitives.h"
#include "merkle.h"
#include "db.h"
#include "consensus.h"
#include <map>
#include <unordered_map>
#include <optional>
#include <set>

namespace intcoin {

// Note: OutPoint and UTXO are defined in transaction.h

/**
 * Blockchain - manages the blockchain state
 */
class Blockchain {
public:
    Blockchain();
    Blockchain(const std::string& datadir);  // Constructor with data directory

    // Add a new block to the chain
    bool add_block(const Block& block);

    // Get a block by hash
    Block get_block(const Hash256& hash) const;

    // Get a block by height
    Block get_block_by_height(uint32_t height) const;

    // Get current blockchain height
    uint32_t get_height() const;

    // Get hash of best block
    Hash256 get_best_block_hash() const;

    // Check if a block exists
    bool has_block(const Hash256& hash) const;

    // Get UTXO for a specific output
    std::optional<UTXO> get_utxo(const Hash256& tx_hash, uint32_t index) const;

    // Get all UTXOs for an address
    std::vector<UTXO> get_utxos_for_address(const std::string& address) const;

    // Get transaction by hash
    std::optional<Transaction> get_transaction(const Hash256& tx_hash) const;

    // Get block height containing a transaction (returns 0 if not found)
    uint32_t get_transaction_block_height(const Hash256& tx_hash) const;

    // Scan blockchain for transactions involving addresses
    std::vector<Transaction> scan_for_addresses(const std::vector<std::string>& addresses) const;

    // Verify all transactions in a block
    bool verify_transactions(const Block& block) const;

    // Verify a single transaction
    bool verify_transaction(const Transaction& tx) const;

    // Calculate block reward for given height
    static uint64_t calculate_block_reward(uint32_t height);

    // Calculate next difficulty target
    uint32_t calculate_next_difficulty(const Hash256& prev_block_hash) const;

    // Create genesis block
    static Block create_genesis_block();

private:
    // Update UTXO set when connecting/disconnecting a block
    void update_utxo_set(const Block& block, bool connect);

    // Update address index when connecting/disconnecting a block
    void update_address_index(const Block& block, bool connect);

    // Extract address from transaction output
    std::optional<std::string> extract_address(const TxOutput& output) const;

    // Database initialization
    bool init_databases(const std::string& datadir);

    // In-memory storage (cache)
    std::unordered_map<Hash256, Block> blocks_;  // All blocks indexed by hash
    std::map<uint32_t, Hash256> block_index_;    // Height to hash mapping
    std::unordered_map<OutPoint, UTXO> utxo_set_;  // All unspent outputs

    // Address indexing (in-memory for now)
    std::unordered_map<std::string, std::vector<OutPoint>> address_index_;  // Address to UTXOs
    std::unordered_map<Hash256, Transaction> transactions_;  // All transactions by hash

    // Persistent storage
    BlockIndexDB block_db_;
    UTXODatabase utxo_db_;
    TransactionIndexDB tx_db_;
    bool use_database_;  // Whether to use persistent storage

    // Consensus
    consensus::ConsensusParams consensus_params_;
    consensus::DifficultyCalculator difficulty_calc_;
    consensus::ForkDetector fork_detector_;
    consensus::CheckpointSystem checkpoint_system_;

    // Chain state
    Hash256 best_block_;
    uint32_t chain_height_;
    uint64_t total_work_ [[maybe_unused]];  // Reserved for future difficulty tracking

    // Fork handling
    bool handle_potential_fork(const Block& new_block, uint32_t new_height);
    bool perform_reorganization(const consensus::ReorgHandler::ReorgInfo& reorg_info);
};

} // namespace intcoin

#endif // INTCOIN_BLOCKCHAIN_H
