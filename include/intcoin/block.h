// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Block and block header definitions.

#ifndef INTCOIN_BLOCK_H
#define INTCOIN_BLOCK_H

#include "primitives.h"
#include "transaction.h"
#include <vector>
#include <memory>
#include <chrono>

namespace intcoin {

/**
 * Block header structure
 * Contains all metadata for a block
 */
struct BlockHeader {
    uint32_t version;                    // Block version
    Hash256 previous_block_hash;         // Hash of previous block
    Hash256 merkle_root;                 // Merkle root of transactions
    uint64_t timestamp;                  // Unix timestamp
    uint32_t bits;                       // Difficulty target (compact form)
    uint64_t nonce;                      // RandomX nonce

    BlockHeader()
        : version(1)
        , previous_block_hash{}
        , merkle_root{}
        , timestamp(0)
        , bits(0)
        , nonce(0)
    {}

    // Serialize the header for hashing
    std::vector<uint8_t> serialize() const;

    // Deserialize from bytes
    static BlockHeader deserialize(const std::vector<uint8_t>& data);

    // Calculate hash of this header
    Hash256 get_hash() const;

    // Check if header satisfies difficulty target
    bool check_proof_of_work() const;
};

/**
 * Full block structure
 * Contains header and all transactions
 */
class Block {
public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    Block() = default;

    Block(const BlockHeader& hdr, const std::vector<Transaction>& txs)
        : header(hdr), transactions(txs) {}

    // Serialize the entire block
    std::vector<uint8_t> serialize() const;

    // Deserialize from bytes
    static Block deserialize(const std::vector<uint8_t>& data);

    // Get block hash (hash of header)
    Hash256 get_hash() const {
        return header.get_hash();
    }

    // Calculate merkle root from transactions
    Hash256 calculate_merkle_root() const;

    // Validate block structure and transactions
    bool validate() const;

    // Get block size in bytes
    size_t get_size() const;

    // Get block weight (for block size limit calculation)
    size_t get_weight() const;

    // Get total fees in block
    uint64_t get_total_fees() const;

    // Get coinbase transaction
    const Transaction* get_coinbase() const {
        return transactions.empty() ? nullptr : &transactions[0];
    }

    // Check if this is a genesis block
    bool is_genesis() const {
        return header.previous_block_hash == Hash256{};
    }

    // Get block reward for given height
    static uint64_t get_block_reward(uint32_t height);

    // Verify block reward is correct
    bool verify_block_reward(uint32_t height) const;
};

/**
 * Block index entry
 * Stores block metadata in the chain database
 */
struct BlockIndex {
    Hash256 hash;
    Hash256 previous_hash;
    uint32_t height;
    uint64_t chain_work;  // Cumulative proof of work
    uint64_t timestamp;
    uint32_t bits;
    uint32_t tx_count;
    size_t file_pos;      // Position in block file

    BlockIndex()
        : hash{}
        , previous_hash{}
        , height(0)
        , chain_work(0)
        , timestamp(0)
        , bits(0)
        , tx_count(0)
        , file_pos(0)
    {}

    // Check if this is genesis block
    bool is_genesis() const {
        return height == 0;
    }
};

/**
 * Genesis block creation
 */
class GenesisBlock {
public:
    // Create mainnet genesis block
    static Block create_mainnet();

    // Create testnet genesis block
    static Block create_testnet();

private:
    static Block create_genesis(
        const std::string& message,
        uint64_t timestamp,
        uint64_t nonce,
        uint32_t bits
    );
};

} // namespace intcoin

#endif // INTCOIN_BLOCK_H
