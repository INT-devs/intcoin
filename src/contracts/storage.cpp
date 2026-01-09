// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/contracts/storage.h"
#include <algorithm>

namespace intcoin {
namespace contracts {

//
// MemoryStorage Implementation
//

Word256 MemoryStorage::Load(const std::string& address, const Word256& key) {
    auto addr_it = storage_.find(address);
    if (addr_it == storage_.end()) {
        return Word256{};  // Return zero if address not found
    }

    auto key_it = addr_it->second.find(key);
    if (key_it == addr_it->second.end()) {
        return Word256{};  // Return zero if key not found
    }

    return key_it->second;
}

void MemoryStorage::Store(const std::string& address, const Word256& key, const Word256& value) {
    // Check if value is zero - if so, delete the entry
    if (IsZeroWord(value)) {
        auto addr_it = storage_.find(address);
        if (addr_it != storage_.end()) {
            addr_it->second.erase(key);
            // Clean up empty address entries
            if (addr_it->second.empty()) {
                storage_.erase(addr_it);
            }
        }
    } else {
        storage_[address][key] = value;
    }
}

bool MemoryStorage::Exists(const std::string& address, const Word256& key) {
    auto addr_it = storage_.find(address);
    if (addr_it == storage_.end()) {
        return false;
    }
    return addr_it->second.find(key) != addr_it->second.end();
}

void MemoryStorage::Clear() {
    storage_.clear();
}

size_t MemoryStorage::Size() const {
    size_t total = 0;
    for (const auto& [address, keys] : storage_) {
        total += keys.size();
    }
    return total;
}

std::map<Word256, Word256> MemoryStorage::GetStorage(const std::string& address) const {
    auto it = storage_.find(address);
    if (it == storage_.end()) {
        return {};
    }
    return it->second;
}

//
// PersistentStorage Implementation (Stub)
//

class PersistentStorage::Impl {
public:
    Config config_;
    std::map<std::string, std::map<Word256, Word256>> storage_;  // Temporary in-memory
    Statistics stats_;

    explicit Impl(const Config& config) : config_(config) {
        // TODO: Initialize RocksDB
    }

    ~Impl() {
        // TODO: Close RocksDB
    }
};

PersistentStorage::PersistentStorage(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

PersistentStorage::~PersistentStorage() = default;

Word256 PersistentStorage::Load(const std::string& address, const Word256& key) {
    // TODO: Load from RocksDB
    pimpl_->stats_.cache_misses++;

    auto addr_it = pimpl_->storage_.find(address);
    if (addr_it == pimpl_->storage_.end()) {
        return Word256{};
    }

    auto key_it = addr_it->second.find(key);
    if (key_it == addr_it->second.end()) {
        return Word256{};
    }

    pimpl_->stats_.cache_hits++;
    return key_it->second;
}

void PersistentStorage::Store(const std::string& address, const Word256& key, const Word256& value) {
    // TODO: Store to RocksDB
    if (IsZeroWord(value)) {
        auto addr_it = pimpl_->storage_.find(address);
        if (addr_it != pimpl_->storage_.end()) {
            addr_it->second.erase(key);
            if (addr_it->second.empty()) {
                pimpl_->storage_.erase(addr_it);
            }
        }
    } else {
        pimpl_->storage_[address][key] = value;
        pimpl_->stats_.total_keys++;
    }
}

bool PersistentStorage::Exists(const std::string& address, const Word256& key) {
    // TODO: Check in RocksDB
    auto addr_it = pimpl_->storage_.find(address);
    if (addr_it == pimpl_->storage_.end()) {
        return false;
    }
    return addr_it->second.find(key) != addr_it->second.end();
}

void PersistentStorage::Flush() {
    // TODO: Flush RocksDB write buffer
}

void PersistentStorage::Compact() {
    // TODO: Compact RocksDB
}

PersistentStorage::Statistics PersistentStorage::GetStatistics() const {
    return pimpl_->stats_;
}

//
// StorageTrie Implementation (Stub)
//

class StorageTrie::Impl {
public:
    std::map<Word256, Word256> data_;
    Word256 root_hash_{};

    void UpdateRootHash() {
        // TODO: Calculate Merkle root
        root_hash_ = Word256{};
    }
};

StorageTrie::StorageTrie() : pimpl_(std::make_unique<Impl>()) {}

StorageTrie::~StorageTrie() = default;

void StorageTrie::Insert(const Word256& key, const Word256& value) {
    pimpl_->data_[key] = value;
    pimpl_->UpdateRootHash();
}

Word256 StorageTrie::Get(const Word256& key) const {
    auto it = pimpl_->data_.find(key);
    if (it == pimpl_->data_.end()) {
        return Word256{};
    }
    return it->second;
}

void StorageTrie::Delete(const Word256& key) {
    pimpl_->data_.erase(key);
    pimpl_->UpdateRootHash();
}

bool StorageTrie::Contains(const Word256& key) const {
    return pimpl_->data_.find(key) != pimpl_->data_.end();
}

Word256 StorageTrie::GetRootHash() const {
    return pimpl_->root_hash_;
}

std::vector<Word256> StorageTrie::GenerateProof(const Word256& key) const {
    // TODO: Generate Merkle proof
    return {};
}

bool StorageTrie::VerifyProof(
    const Word256& root_hash,
    const Word256& key,
    const Word256& value,
    const std::vector<Word256>& proof
) {
    // TODO: Verify Merkle proof
    return false;
}

void StorageTrie::Clear() {
    pimpl_->data_.clear();
    pimpl_->UpdateRootHash();
}

size_t StorageTrie::Size() const {
    return pimpl_->data_.size();
}

//
// Helper Functions
//

std::string MakeStorageKey(const std::string& address, const Word256& key) {
    std::string storage_key;
    storage_key.reserve(address.size() + 1 + 64);
    storage_key += address;
    storage_key += ":";
    storage_key += Word256ToHex(key);
    return storage_key;
}

bool ParseStorageKey(const std::string& storage_key, std::string& address, Word256& key) {
    size_t pos = storage_key.find(':');
    if (pos == std::string::npos) {
        return false;
    }

    address = storage_key.substr(0, pos);
    std::string key_hex = storage_key.substr(pos + 1);

    try {
        key = HexToWord256(key_hex);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace contracts
} // namespace intcoin
