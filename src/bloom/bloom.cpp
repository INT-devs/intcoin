// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/bloom.h>
#include <intcoin/transaction.h>
#include <intcoin/util.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace intcoin {

// MurmurHash3 implementation (32-bit) for bloom filters
// Based on Austin Appleby's MurmurHash3 public domain implementation
uint32_t BloomFilter::MurmurHash3(uint32_t seed, const std::vector<uint8_t>& data) {
    uint32_t h = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const size_t nblocks = data.size() / 4;
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data.data());

    // Process 4-byte blocks
    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> 17);  // ROTL32(k, 15)
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> 19);  // ROTL32(h, 13)
        h = h * 5 + 0xe6546b64;
    }

    // Process remaining bytes
    const uint8_t* tail = data.data() + nblocks * 4;
    uint32_t k = 0;

    switch (data.size() & 3) {
        case 3:
            k ^= tail[2] << 16;
            [[fallthrough]];
        case 2:
            k ^= tail[1] << 8;
            [[fallthrough]];
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 15) | (k >> 17);
            k *= c2;
            h ^= k;
    }

    // Finalization
    h ^= data.size();
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

BloomFilter::BloomFilter(uint32_t elements, double fp_rate, uint32_t tweak, BloomFlags flags)
    : tweak_(tweak), flags_(flags), is_empty_(true), is_full_(false) {

    // Calculate optimal filter size: m = -n * ln(p) / (ln(2)^2)
    // where n = elements, p = false positive rate
    double size = -1.0 * elements * std::log(fp_rate) / (std::log(2.0) * std::log(2.0));
    uint32_t filter_size = std::min(static_cast<uint32_t>(size / 8),
                                    static_cast<uint32_t>(MAX_BLOOM_FILTER_SIZE));

    // Ensure minimum size of 1 byte
    if (filter_size < 1) {
        filter_size = 1;
    }

    // Calculate optimal number of hash functions: k = (m/n) * ln(2)
    double hash_count = filter_size * 8.0 / elements * std::log(2.0);
    hash_funcs_ = std::min(static_cast<uint32_t>(hash_count), MAX_HASH_FUNCS);

    // Ensure at least 1 hash function
    if (hash_funcs_ < 1) {
        hash_funcs_ = 1;
    }

    // Initialize filter with all zeros
    filter_.resize(filter_size, 0);
}

BloomFilter::BloomFilter()
    : hash_funcs_(0), tweak_(0), flags_(BLOOM_UPDATE_NONE),
      is_empty_(true), is_full_(false) {
}

void BloomFilter::Add(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    is_empty_ = false;

    for (uint32_t i = 0; i < hash_funcs_; i++) {
        uint32_t index = Hash(i, data);
        filter_[index >> 3] |= (1 << (7 & index));
    }

    // Check if filter is now full
    is_full_ = true;
    for (uint8_t byte : filter_) {
        if (byte != 0xFF) {
            is_full_ = false;
            break;
        }
    }
}

void BloomFilter::Add(const uint8_t* data, size_t len) {
    std::vector<uint8_t> vec(data, data + len);
    Add(vec);
}

void BloomFilter::AddOutPoint(const OutPoint& outpoint) {
    std::vector<uint8_t> data;
    data.insert(data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());

    // Add index in little-endian
    for (int i = 0; i < 4; i++) {
        data.push_back((outpoint.index >> (i * 8)) & 0xFF);
    }

    Add(data);
}

bool BloomFilter::Contains(const std::vector<uint8_t>& data) const {
    if (is_full_) {
        return true;
    }

    if (is_empty_) {
        return false;
    }

    if (data.empty()) {
        return false;
    }

    for (uint32_t i = 0; i < hash_funcs_; i++) {
        uint32_t index = Hash(i, data);
        if (!(filter_[index >> 3] & (1 << (7 & index)))) {
            return false;
        }
    }

    return true;
}

bool BloomFilter::Contains(const uint8_t* data, size_t len) const {
    std::vector<uint8_t> vec(data, data + len);
    return Contains(vec);
}

bool BloomFilter::ContainsOutPoint(const OutPoint& outpoint) const {
    std::vector<uint8_t> data;
    data.insert(data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());

    // Add index in little-endian
    for (int i = 0; i < 4; i++) {
        data.push_back((outpoint.index >> (i * 8)) & 0xFF);
    }

    return Contains(data);
}

bool BloomFilter::MatchesTransaction(const Transaction& tx) const {
    if (is_full_) {
        return true;
    }

    if (is_empty_) {
        return false;
    }

    // Check transaction hash
    uint256 tx_hash = tx.GetHash();
    std::vector<uint8_t> tx_hash_vec(tx_hash.begin(), tx_hash.end());
    if (Contains(tx_hash_vec)) {
        return true;
    }

    // Check each output for matching scriptPubKey data
    for (const auto& output : tx.outputs) {
        // Check scriptPubKey
        if (Contains(output.script_pubkey.bytes)) {
            return true;
        }

        // Extract and check addresses/pubkeys from scriptPubKey
        // TODO: Parse P2PKH, P2SH addresses and check individually
    }

    // Check each input for matching outpoints
    for (const auto& input : tx.inputs) {
        // Create OutPoint from tx input
        OutPoint outpoint;
        outpoint.tx_hash = input.prev_tx_hash;
        outpoint.index = input.prev_tx_index;

        if (ContainsOutPoint(outpoint)) {
            return true;
        }

        // Check scriptSig data
        if (Contains(input.script_sig.bytes)) {
            return true;
        }
    }

    return false;
}

void BloomFilter::Clear() {
    std::fill(filter_.begin(), filter_.end(), 0);
    is_empty_ = true;
    is_full_ = false;
}

bool BloomFilter::IsEmpty() const {
    return is_empty_;
}

bool BloomFilter::IsFull() const {
    return is_full_;
}

bool BloomFilter::IsValid() const {
    if (filter_.size() > MAX_BLOOM_FILTER_SIZE) {
        return false;
    }

    if (hash_funcs_ > MAX_HASH_FUNCS) {
        return false;
    }

    return true;
}

std::vector<uint8_t> BloomFilter::Serialize() const {
    std::vector<uint8_t> result;

    // Filter size (varint)
    uint32_t size = filter_.size();
    if (size < 0xFD) {
        result.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        result.push_back(0xFD);
        result.push_back(size & 0xFF);
        result.push_back((size >> 8) & 0xFF);
    } else {
        result.push_back(0xFE);
        for (int i = 0; i < 4; i++) {
            result.push_back((size >> (i * 8)) & 0xFF);
        }
    }

    // Filter data
    result.insert(result.end(), filter_.begin(), filter_.end());

    // Number of hash functions (4 bytes, little-endian)
    for (int i = 0; i < 4; i++) {
        result.push_back((hash_funcs_ >> (i * 8)) & 0xFF);
    }

    // Tweak (4 bytes, little-endian)
    for (int i = 0; i < 4; i++) {
        result.push_back((tweak_ >> (i * 8)) & 0xFF);
    }

    // Flags (1 byte)
    result.push_back(static_cast<uint8_t>(flags_));

    return result;
}

Result<BloomFilter> BloomFilter::Deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return Result<BloomFilter>::Error("Empty bloom filter data");
    }

    size_t offset = 0;

    // Read filter size (varint)
    uint32_t filter_size = 0;
    if (data[offset] < 0xFD) {
        filter_size = data[offset];
        offset += 1;
    } else if (data[offset] == 0xFD) {
        if (data.size() < offset + 3) {
            return Result<BloomFilter>::Error("Truncated filter size");
        }
        filter_size = data[offset + 1] | (data[offset + 2] << 8);
        offset += 3;
    } else if (data[offset] == 0xFE) {
        if (data.size() < offset + 5) {
            return Result<BloomFilter>::Error("Truncated filter size");
        }
        filter_size = data[offset + 1] | (data[offset + 2] << 8) |
                      (data[offset + 3] << 16) | (data[offset + 4] << 24);
        offset += 5;
    } else {
        return Result<BloomFilter>::Error("Invalid filter size encoding");
    }

    // Validate filter size
    if (filter_size > MAX_BLOOM_FILTER_SIZE) {
        return Result<BloomFilter>::Error("Filter size exceeds maximum");
    }

    // Read filter data
    if (data.size() < offset + filter_size) {
        return Result<BloomFilter>::Error("Truncated filter data");
    }

    BloomFilter filter;
    filter.filter_.assign(data.begin() + offset, data.begin() + offset + filter_size);
    offset += filter_size;

    // Read hash functions (4 bytes)
    if (data.size() < offset + 4) {
        return Result<BloomFilter>::Error("Truncated hash functions");
    }
    filter.hash_funcs_ = data[offset] | (data[offset + 1] << 8) |
                        (data[offset + 2] << 16) | (data[offset + 3] << 24);
    offset += 4;

    // Validate hash functions
    if (filter.hash_funcs_ > MAX_HASH_FUNCS) {
        return Result<BloomFilter>::Error("Too many hash functions");
    }

    // Read tweak (4 bytes)
    if (data.size() < offset + 4) {
        return Result<BloomFilter>::Error("Truncated tweak");
    }
    filter.tweak_ = data[offset] | (data[offset + 1] << 8) |
                   (data[offset + 2] << 16) | (data[offset + 3] << 24);
    offset += 4;

    // Read flags (1 byte)
    if (data.size() < offset + 1) {
        return Result<BloomFilter>::Error("Truncated flags");
    }
    filter.flags_ = static_cast<BloomFlags>(data[offset]);

    // Check if filter is empty or full
    filter.is_empty_ = true;
    filter.is_full_ = true;
    for (uint8_t byte : filter.filter_) {
        if (byte != 0) {
            filter.is_empty_ = false;
        }
        if (byte != 0xFF) {
            filter.is_full_ = false;
        }
    }

    return Result<BloomFilter>::Ok(filter);
}

uint32_t BloomFilter::Hash(uint32_t hash_num, const std::vector<uint8_t>& data) const {
    uint32_t seed = (hash_num * 0xFBA4C795) + tweak_;
    uint32_t hash = MurmurHash3(seed, data);
    return hash % (filter_.size() * 8);
}

}  // namespace intcoin
