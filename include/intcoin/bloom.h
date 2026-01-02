// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_BLOOM_H
#define INTCOIN_BLOOM_H

#include <intcoin/types.h>

#include <cstdint>
#include <vector>

namespace intcoin {

// Forward declarations
struct OutPoint;
class Transaction;

/// Bloom filter flags (BIP37)
enum BloomFlags : uint8_t {
    BLOOM_UPDATE_NONE = 0,          // Never update the filter
    BLOOM_UPDATE_ALL = 1,           // Always update filter on match
    BLOOM_UPDATE_P2PUBKEY_ONLY = 2  // Only update on pay-to-pubkey matches
};

/// Bloom filter for SPV transaction filtering (BIP37)
/// Allows lightweight clients to request filtered transactions without revealing exact addresses
class BloomFilter {
public:
    /// Constructor
    /// @param elements Expected number of elements
    /// @param fp_rate Desired false positive rate (0.0 - 1.0)
    /// @param tweak Random value to randomize hash functions
    /// @param flags Update flags for matched transactions
    BloomFilter(uint32_t elements, double fp_rate, uint32_t tweak = 0,
                BloomFlags flags = BLOOM_UPDATE_ALL);

    /// Default constructor for deserialization
    BloomFilter();

    /// Add element to the filter
    /// @param data Element to add
    void Add(const std::vector<uint8_t>& data);

    /// Add element to the filter (convenience overload)
    /// @param data Pointer to data
    /// @param len Length of data
    void Add(const uint8_t* data, size_t len);

    /// Add outpoint to the filter
    /// @param outpoint Transaction outpoint to add
    void AddOutPoint(const OutPoint& outpoint);

    /// Check if element matches the filter
    /// @param data Element to check
    /// @return True if element matches (may be false positive)
    bool Contains(const std::vector<uint8_t>& data) const;

    /// Check if element matches the filter (convenience overload)
    /// @param data Pointer to data
    /// @param len Length of data
    /// @return True if element matches (may be false positive)
    bool Contains(const uint8_t* data, size_t len) const;

    /// Check if outpoint matches the filter
    /// @param outpoint Transaction outpoint to check
    /// @return True if outpoint matches
    bool ContainsOutPoint(const OutPoint& outpoint) const;

    /// Check if transaction matches the filter
    /// @param tx Transaction to check
    /// @return True if any element of the transaction matches
    bool MatchesTransaction(const Transaction& tx) const;

    /// Clear the filter
    void Clear();

    /// Check if filter is empty (matches nothing)
    /// @return True if filter is empty
    bool IsEmpty() const;

    /// Check if filter is full (matches everything)
    /// @return True if filter is full
    bool IsFull() const;

    /// Check if filter is within size limits
    /// @return True if filter size is valid
    bool IsValid() const;

    /// Get filter size in bytes
    /// @return Size of the filter data
    size_t GetSize() const { return filter_.size(); }

    /// Get number of hash functions
    /// @return Number of hash functions used
    uint32_t GetNumHashFuncs() const { return hash_funcs_; }

    /// Get tweak value
    /// @return Tweak for hash randomization
    uint32_t GetTweak() const { return tweak_; }

    /// Get update flags
    /// @return Filter update flags
    BloomFlags GetFlags() const { return flags_; }

    /// Serialize filter for network transmission
    /// @return Serialized filter data
    std::vector<uint8_t> Serialize() const;

    /// Deserialize filter from network data
    /// @param data Serialized filter data
    /// @return Result with deserialized filter
    static Result<BloomFilter> Deserialize(const std::vector<uint8_t>& data);

    /// Maximum filter size (36,000 bytes per BIP37)
    static constexpr size_t MAX_BLOOM_FILTER_SIZE = 36000;

    /// Maximum number of hash functions (50 per BIP37)
    static constexpr uint32_t MAX_HASH_FUNCS = 50;

private:
    /// Calculate hash for element
    /// @param hash_num Hash function index (0 to hash_funcs-1)
    /// @param data Data to hash
    /// @return Hash value (bit index in filter)
    uint32_t Hash(uint32_t hash_num, const std::vector<uint8_t>& data) const;

    /// MurmurHash3 implementation for bloom filters
    /// @param seed Hash seed
    /// @param data Data to hash
    /// @return 32-bit hash value
    static uint32_t MurmurHash3(uint32_t seed, const std::vector<uint8_t>& data);

    /// Filter data (bit array stored as bytes)
    std::vector<uint8_t> filter_;

    /// Number of hash functions
    uint32_t hash_funcs_;

    /// Random tweak for hash functions
    uint32_t tweak_;

    /// Update flags
    BloomFlags flags_;

    /// Empty flag (all bits zero)
    bool is_empty_;

    /// Full flag (all bits one)
    bool is_full_;
};

}  // namespace intcoin

#endif  // INTCOIN_BLOOM_H
