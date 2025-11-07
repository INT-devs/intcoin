// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BLOOM_H
#define INTCOIN_BLOOM_H

#include "primitives.h"
#include "transaction.h"
#include "block.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace intcoin {

/**
 * Bloom Filter for SPV (Simplified Payment Verification) clients
 *
 * Implements BIP 37 bloom filtering for privacy-preserving lightweight clients.
 * Bloom filters allow SPV clients to request only relevant transactions without
 * revealing exactly which addresses they're interested in.
 */
class BloomFilter {
public:
    /**
     * Update types for matched elements
     */
    enum class UpdateType : uint8_t {
        NONE = 0,                 // Never update filter
        ALL = 1,                  // Update on all matches
        PUBKEY_ONLY = 2           // Only update on pubkey matches
    };

private:
    std::vector<uint8_t> filter_;     // Bit array
    uint32_t hash_funcs_;             // Number of hash functions
    uint32_t tweak_;                  // Random tweak for hash functions
    UpdateType update_type_;          // Update policy
    bool is_full_;                    // Filter is saturated
    bool is_empty_;                   // Filter has no elements

    // Bloom filter parameters
    static constexpr double MAX_BLOOM_FILTER_SIZE = 36000;  // bytes
    static constexpr uint32_t MAX_HASH_FUNCS = 50;
    static constexpr double LN2_SQUARED = 0.4804530139182014246671025263266649717305529515945455;
    static constexpr double LN2 = 0.6931471805599453094172321214581765680755001343602552;

public:
    /**
     * Create empty bloom filter
     */
    BloomFilter();

    /**
     * Create bloom filter with specified parameters
     *
     * @param elements Expected number of elements
     * @param fp_rate Desired false positive rate (0.0 to 1.0)
     * @param tweak Random tweak for hash functions
     * @param update_type Update policy
     */
    BloomFilter(uint32_t elements, double fp_rate, uint32_t tweak = 0,
                UpdateType update_type = UpdateType::ALL);

    /**
     * Insert data into bloom filter
     */
    void insert(const std::vector<uint8_t>& data);

    /**
     * Insert hash into bloom filter
     */
    void insert(const Hash256& hash);

    /**
     * Check if data might be in filter
     * @return true if possibly in filter, false if definitely not
     */
    bool contains(const std::vector<uint8_t>& data) const;

    /**
     * Check if hash might be in filter
     */
    bool contains(const Hash256& hash) const;

    /**
     * Check if transaction matches filter
     *
     * @param tx Transaction to check
     * @return true if transaction matches any filter criteria
     */
    bool matches_transaction(const Transaction& tx) const;

    /**
     * Check if block matches filter
     * Returns true if any transaction in block matches
     */
    bool matches_block(const Block& block) const;

    /**
     * Update filter with matched transaction outputs
     * Called when a transaction matches to track its outputs
     */
    void update_with_transaction(const Transaction& tx);

    /**
     * Clear the filter
     */
    void clear();

    /**
     * Check if filter is empty
     */
    bool is_empty() const {
        return is_empty_;
    }

    /**
     * Check if filter is full (all bits set)
     */
    bool is_full() const {
        return is_full_;
    }

    /**
     * Get filter size in bytes
     */
    size_t size() const {
        return filter_.size();
    }

    /**
     * Get number of hash functions
     */
    uint32_t get_hash_funcs() const {
        return hash_funcs_;
    }

    /**
     * Serialize bloom filter for network transmission
     */
    std::vector<uint8_t> serialize() const;

    /**
     * Deserialize bloom filter from network data
     */
    static std::unique_ptr<BloomFilter> deserialize(const std::vector<uint8_t>& data);

    /**
     * Check if parameters are valid
     */
    bool is_valid() const;

    /**
     * Get estimated false positive rate
     */
    double get_fp_rate() const;

private:
    /**
     * Hash function for bloom filter
     * Uses MurmurHash3 as specified in BIP 37
     */
    uint32_t hash(uint32_t hash_num, const std::vector<uint8_t>& data) const;

    /**
     * MurmurHash3 implementation
     */
    static uint32_t murmur_hash3(uint32_t seed, const std::vector<uint8_t>& data);

    /**
     * Check if filter is within size limits
     */
    static bool is_size_valid(size_t size) {
        return size > 0 && size <= MAX_BLOOM_FILTER_SIZE;
    }

    /**
     * Check if hash func count is valid
     */
    static bool is_hash_func_count_valid(uint32_t count) {
        return count > 0 && count <= MAX_HASH_FUNCS;
    }

    /**
     * Update filter saturation status
     */
    void update_empty_full_status();
};

/**
 * Merkle block - filtered block for SPV clients
 * Contains block header and merkle branch proving included transactions
 */
class MerkleBlock {
public:
    BlockHeader header;
    std::vector<Hash256> txn_hashes;        // Hashes of matched transactions
    std::vector<bool> flags;                 // Flags for merkle tree traversal
    std::vector<uint8_t> match_flags;       // Which transactions matched

    MerkleBlock() = default;

    /**
     * Create merkle block from full block and bloom filter
     *
     * @param block Full block
     * @param filter Bloom filter to match against
     */
    MerkleBlock(const Block& block, const BloomFilter& filter);

    /**
     * Verify merkle proof
     * @return true if proof is valid
     */
    bool verify_merkle_proof() const;

    /**
     * Get matched transaction indices
     */
    std::vector<size_t> get_matched_indices() const;

    /**
     * Serialize merkle block
     */
    std::vector<uint8_t> serialize() const;

    /**
     * Deserialize merkle block
     */
    static std::unique_ptr<MerkleBlock> deserialize(const std::vector<uint8_t>& data);

private:
    /**
     * Traverse merkle tree and build proof
     */
    void traverse_and_build(
        const std::vector<Hash256>& tx_hashes,
        const std::vector<bool>& matches,
        uint32_t height,
        uint32_t pos,
        std::vector<Hash256>& merkle_branch,
        std::vector<bool>& match_flags
    );

    /**
     * Traverse merkle tree and extract matches
     */
    Hash256 traverse_and_extract(
        uint32_t height,
        uint32_t pos,
        size_t& bits_used,
        size_t& hashes_used,
        std::vector<Hash256>& matched
    ) const;
};

/**
 * SPV Client
 * Lightweight client using bloom filters
 */
class SPVClient {
private:
    std::unique_ptr<BloomFilter> bloom_filter_;
    std::vector<Transaction> matched_transactions_;
    std::vector<MerkleBlock> merkle_blocks_;

    // Addresses to watch
    std::vector<std::vector<uint8_t>> watch_addresses_;

    static constexpr uint32_t DEFAULT_BLOOM_ELEMENTS = 1000;
    static constexpr double DEFAULT_FP_RATE = 0.0001;  // 0.01%

public:
    SPVClient();

    /**
     * Add address to watch list
     */
    void add_address(const std::vector<uint8_t>& address);

    /**
     * Add transaction output to watch (for receiving payments)
     */
    void add_outpoint(const Hash256& tx_hash, uint32_t index);

    /**
     * Rebuild bloom filter with current watch list
     */
    void rebuild_filter();

    /**
     * Get bloom filter for network transmission
     */
    const BloomFilter& get_filter() const {
        return *bloom_filter_;
    }

    /**
     * Process merkle block from network
     */
    bool process_merkle_block(const MerkleBlock& merkle_block);

    /**
     * Process matched transaction
     */
    void process_transaction(const Transaction& tx);

    /**
     * Get all matched transactions
     */
    const std::vector<Transaction>& get_matched_transactions() const {
        return matched_transactions_;
    }

    /**
     * Get balance for watched addresses
     * Note: This is an estimate based on matched transactions
     */
    uint64_t get_estimated_balance() const;

    /**
     * Clear all data
     */
    void clear();
};

} // namespace intcoin

#endif // INTCOIN_BLOOM_H
