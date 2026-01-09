// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_STORAGE_H
#define INTCOIN_CONTRACTS_STORAGE_H

#include "intcoin/contracts/vm.h"
#include <map>
#include <string>
#include <memory>

namespace intcoin {
namespace contracts {

/**
 * In-Memory Storage Implementation
 *
 * Simple key-value storage for testing and development
 */
class MemoryStorage : public IStorage {
public:
    MemoryStorage() = default;
    ~MemoryStorage() override = default;

    /**
     * Load value from storage
     */
    Word256 Load(const std::string& address, const Word256& key) override;

    /**
     * Store value to storage
     */
    void Store(const std::string& address, const Word256& key, const Word256& value) override;

    /**
     * Check if key exists
     */
    bool Exists(const std::string& address, const Word256& key) override;

    /**
     * Clear all storage
     */
    void Clear();

    /**
     * Get storage size
     */
    size_t Size() const;

    /**
     * Get storage for address
     */
    std::map<Word256, Word256> GetStorage(const std::string& address) const;

private:
    std::map<std::string, std::map<Word256, Word256>> storage_;
};

/**
 * Persistent Storage Implementation
 *
 * Storage backed by RocksDB or similar database
 */
class PersistentStorage : public IStorage {
public:
    struct Config {
        std::string db_path;
        bool create_if_missing{true};
        bool enable_cache{true};
        size_t cache_size{100 * 1024 * 1024};  // 100 MB
    };

    explicit PersistentStorage(const Config& config);
    ~PersistentStorage() override;

    /**
     * Load value from storage
     */
    Word256 Load(const std::string& address, const Word256& key) override;

    /**
     * Store value to storage
     */
    void Store(const std::string& address, const Word256& key, const Word256& value) override;

    /**
     * Check if key exists
     */
    bool Exists(const std::string& address, const Word256& key) override;

    /**
     * Flush pending writes
     */
    void Flush();

    /**
     * Compact database
     */
    void Compact();

    /**
     * Get statistics
     */
    struct Statistics {
        uint64_t total_keys{0};
        uint64_t total_size{0};
        uint64_t cache_hits{0};
        uint64_t cache_misses{0};
    };

    Statistics GetStatistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Storage Trie
 *
 * Merkle Patricia Trie for storage with efficient proofs
 */
class StorageTrie {
public:
    StorageTrie();
    ~StorageTrie();

    /**
     * Insert key-value pair
     */
    void Insert(const Word256& key, const Word256& value);

    /**
     * Get value by key
     */
    Word256 Get(const Word256& key) const;

    /**
     * Delete key
     */
    void Delete(const Word256& key);

    /**
     * Check if key exists
     */
    bool Contains(const Word256& key) const;

    /**
     * Get root hash
     */
    Word256 GetRootHash() const;

    /**
     * Generate proof for key
     */
    std::vector<Word256> GenerateProof(const Word256& key) const;

    /**
     * Verify proof
     */
    static bool VerifyProof(
        const Word256& root_hash,
        const Word256& key,
        const Word256& value,
        const std::vector<Word256>& proof
    );

    /**
     * Clear trie
     */
    void Clear();

    /**
     * Get size
     */
    size_t Size() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Helper functions
 */

/**
 * Create storage key from address and key
 */
std::string MakeStorageKey(const std::string& address, const Word256& key);

/**
 * Parse storage key
 */
bool ParseStorageKey(const std::string& storage_key, std::string& address, Word256& key);

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_STORAGE_H
