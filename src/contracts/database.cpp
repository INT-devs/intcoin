// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include "intcoin/contracts/database.h"
#include "intcoin/storage.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace contracts {

//
// Key Prefixes for RocksDB
//
namespace db_prefix {
    constexpr char CONTRACT_ACCOUNT  = 'C';  // Contract account data
    constexpr char CONTRACT_STORAGE  = 'S';  // Contract storage
    constexpr char TX_RECEIPT        = 'R';  // Transaction receipts
    constexpr char EVENT_LOG         = 'L';  // Event logs
    constexpr char CONTRACT_INDEX    = 'I';  // Contract address index
    [[maybe_unused]] constexpr char LOG_INDEX_BLOCK   = 'B';  // Log index by block
    [[maybe_unused]] constexpr char LOG_INDEX_ADDRESS = 'A';  // Log index by address
}

//
// Helper functions
//

static std::string MakeContractAccountKey(const std::string& address) {
    return std::string(1, db_prefix::CONTRACT_ACCOUNT) + address;
}

static std::string MakeContractStorageKey(const std::string& address, const Word256& key) {
    std::string result(1, db_prefix::CONTRACT_STORAGE);
    result += address;
    result += std::string(reinterpret_cast<const char*>(key.data()), key.size());
    return result;
}

[[maybe_unused]] static std::string MakeReceiptKey(const uint256& txid) {
    return std::string(1, db_prefix::TX_RECEIPT) +
           std::string(reinterpret_cast<const char*>(txid.data()), txid.size());
}

[[maybe_unused]] static std::string MakeEventLogKey(uint64_t block_number, const uint256& tx_hash, uint32_t log_index) {
    std::string result(1, db_prefix::EVENT_LOG);

    // Add block number (8 bytes, big-endian for proper sorting)
    for (int i = 7; i >= 0; --i) {
        result += static_cast<char>((block_number >> (i * 8)) & 0xff);
    }

    // Add transaction hash
    result += std::string(reinterpret_cast<const char*>(tx_hash.data()), tx_hash.size());

    // Add log index (4 bytes, big-endian)
    for (int i = 3; i >= 0; --i) {
        result += static_cast<char>((log_index >> (i * 8)) & 0xff);
    }

    return result;
}

[[maybe_unused]] static std::string MakeContractIndexKey(uint32_t index) {
    std::string result(1, db_prefix::CONTRACT_INDEX);
    for (int i = 3; i >= 0; --i) {
        result += static_cast<char>((index >> (i * 8)) & 0xff);
    }
    return result;
}

//
// Serialization helpers
//

static std::vector<uint8_t> SerializeContractAccount(const ContractAccount& account) {
    std::vector<uint8_t> data;

    // Address (length-prefixed string)
    uint32_t addr_len = account.address.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((addr_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), account.address.begin(), account.address.end());

    // Balance (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((account.balance >> (i * 8)) & 0xff);
    }

    // Nonce (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((account.nonce >> (i * 8)) & 0xff);
    }

    // Bytecode (length-prefixed)
    uint32_t code_len = account.bytecode.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((code_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), account.bytecode.begin(), account.bytecode.end());

    // Code hash (32 bytes)
    data.insert(data.end(), account.code_hash.begin(), account.code_hash.end());

    // Storage root (32 bytes)
    data.insert(data.end(), account.storage_root.begin(), account.storage_root.end());

    // Creator (length-prefixed)
    uint32_t creator_len = account.creator.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((creator_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), account.creator.begin(), account.creator.end());

    // Creation tx (32 bytes)
    data.insert(data.end(), account.creation_tx.begin(), account.creation_tx.end());

    // Block created (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((account.block_created >> (i * 8)) & 0xff);
    }

    // Block updated (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((account.block_updated >> (i * 8)) & 0xff);
    }

    return data;
}

static Result<ContractAccount> DeserializeContractAccount(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return Result<ContractAccount>::Error("Invalid data size");
    }

    ContractAccount account;
    size_t offset = 0;

    // Address
    uint32_t addr_len = 0;
    for (int i = 0; i < 4; ++i) {
        addr_len = (addr_len << 8) | data[offset++];
    }
    if (offset + addr_len > data.size()) {
        return Result<ContractAccount>::Error("Invalid address length");
    }
    account.address = std::string(data.begin() + offset, data.begin() + offset + addr_len);
    offset += addr_len;

    // Balance
    account.balance = 0;
    for (int i = 0; i < 8; ++i) {
        account.balance = (account.balance << 8) | data[offset++];
    }

    // Nonce
    account.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        account.nonce = (account.nonce << 8) | data[offset++];
    }

    // Bytecode
    uint32_t code_len = 0;
    for (int i = 0; i < 4; ++i) {
        code_len = (code_len << 8) | data[offset++];
    }
    if (offset + code_len > data.size()) {
        return Result<ContractAccount>::Error("Invalid bytecode length");
    }
    account.bytecode = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + code_len);
    offset += code_len;

    // Code hash
    if (offset + 32 > data.size()) {
        return Result<ContractAccount>::Error("Invalid code hash");
    }
    std::copy(data.begin() + offset, data.begin() + offset + 32, account.code_hash.begin());
    offset += 32;

    // Storage root
    if (offset + 32 > data.size()) {
        return Result<ContractAccount>::Error("Invalid storage root");
    }
    std::copy(data.begin() + offset, data.begin() + offset + 32, account.storage_root.begin());
    offset += 32;

    // Creator
    uint32_t creator_len = 0;
    for (int i = 0; i < 4; ++i) {
        creator_len = (creator_len << 8) | data[offset++];
    }
    if (offset + creator_len > data.size()) {
        return Result<ContractAccount>::Error("Invalid creator length");
    }
    account.creator = std::string(data.begin() + offset, data.begin() + offset + creator_len);
    offset += creator_len;

    // Creation tx
    if (offset + 32 > data.size()) {
        return Result<ContractAccount>::Error("Invalid creation tx");
    }
    std::copy(data.begin() + offset, data.begin() + offset + 32, account.creation_tx.begin());
    offset += 32;

    // Block created
    account.block_created = 0;
    for (int i = 0; i < 8; ++i) {
        account.block_created = (account.block_created << 8) | data[offset++];
    }

    // Block updated
    account.block_updated = 0;
    for (int i = 0; i < 8; ++i) {
        account.block_updated = (account.block_updated << 8) | data[offset++];
    }

    return Result<ContractAccount>::Ok(std::move(account));
}

//
// ContractDatabase::Impl
//

class ContractDatabase::Impl {
public:
    rocksdb::DB* db = nullptr;
    rocksdb::WriteBatch* batch = nullptr;
    bool batch_active = false;

    ~Impl() {
        if (batch) {
            delete batch;
            batch = nullptr;
        }
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

//
// ContractDatabase Implementation
//

ContractDatabase::ContractDatabase()
    : impl_(std::make_unique<Impl>()) {}

ContractDatabase::ContractDatabase(const std::string& db_path)
    : impl_(std::make_unique<Impl>()) {
    Open(db_path);
}

ContractDatabase::~ContractDatabase() {
    Close();
}

ContractDatabase::ContractDatabase(ContractDatabase&&) noexcept = default;
ContractDatabase& ContractDatabase::operator=(ContractDatabase&&) noexcept = default;

Result<void> ContractDatabase::Open(const std::string& db_path) {
    if (impl_->db) {
        return Result<void>::Error("Database already open");
    }

    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &impl_->db);
    if (!status.ok()) {
        return Result<void>::Error("Failed to open database: " + status.ToString());
    }

    return Result<void>::Ok();
}

void ContractDatabase::Close() {
    if (impl_->batch) {
        delete impl_->batch;
        impl_->batch = nullptr;
        impl_->batch_active = false;
    }

    if (impl_->db) {
        delete impl_->db;
        impl_->db = nullptr;
    }
}

bool ContractDatabase::IsOpen() const {
    return impl_->db != nullptr;
}

Result<void> ContractDatabase::PutContractAccount(const ContractAccount& account) {
    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string key = MakeContractAccountKey(account.address);
    std::vector<uint8_t> value = SerializeContractAccount(account);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Put(key, rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));
        return Result<void>::Ok();
    } else {
        status = impl_->db->Put(rocksdb::WriteOptions(), key,
            rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));

        if (!status.ok()) {
            return Result<void>::Error("Failed to write contract account: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<ContractAccount> ContractDatabase::GetContractAccount(const std::string& address) {
    if (!impl_->db) {
        return Result<ContractAccount>::Error("Database not open");
    }

    std::string key = MakeContractAccountKey(address);
    std::string value_str;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value_str);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            return Result<ContractAccount>::Error("Contract not found");
        }
        return Result<ContractAccount>::Error("Database read error: " + status.ToString());
    }

    std::vector<uint8_t> value(value_str.begin(), value_str.end());
    return DeserializeContractAccount(value);
}

bool ContractDatabase::ContractExists(const std::string& address) {
    if (!impl_->db) {
        return false;
    }

    std::string key = MakeContractAccountKey(address);
    std::string value;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);
    return status.ok();
}

Result<void> ContractDatabase::DeleteContractAccount(const std::string& address) {
    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string key = MakeContractAccountKey(address);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Delete(key);
        return Result<void>::Ok();
    } else {
        status = impl_->db->Delete(rocksdb::WriteOptions(), key);

        if (!status.ok()) {
            return Result<void>::Error("Failed to delete contract: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<std::vector<std::string>> ContractDatabase::ListContractAddresses(uint32_t limit, uint32_t offset) {
    if (!impl_->db) {
        return Result<std::vector<std::string>>::Error("Database not open");
    }

    std::vector<std::string> addresses;
    std::string prefix(1, db_prefix::CONTRACT_ACCOUNT);

    rocksdb::Iterator* it = impl_->db->NewIterator(rocksdb::ReadOptions());
    uint32_t count = 0;
    uint32_t skipped = 0;

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        if (skipped < offset) {
            skipped++;
            continue;
        }

        if (count >= limit) {
            break;
        }

        std::string key = it->key().ToString();
        std::string address = key.substr(1); // Skip prefix
        addresses.push_back(address);
        count++;
    }

    delete it;

    return Result<std::vector<std::string>>::Ok(std::move(addresses));
}

Result<void> ContractDatabase::PutContractStorage(
    const std::string& contract_address,
    const Word256& key,
    const Word256& value) {

    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string db_key = MakeContractStorageKey(contract_address, key);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Put(db_key, rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));
        return Result<void>::Ok();
    } else {
        status = impl_->db->Put(rocksdb::WriteOptions(), db_key,
            rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));

        if (!status.ok()) {
            return Result<void>::Error("Failed to write storage: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<Word256> ContractDatabase::GetContractStorage(
    const std::string& contract_address,
    const Word256& key) {

    if (!impl_->db) {
        return Result<Word256>::Error("Database not open");
    }

    std::string db_key = MakeContractStorageKey(contract_address, key);
    std::string value_str;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), db_key, &value_str);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            // Return zero for non-existent keys
            Word256 zero{};
            return Result<Word256>::Ok(zero);
        }
        return Result<Word256>::Error("Database read error: " + status.ToString());
    }

    if (value_str.size() != 32) {
        return Result<Word256>::Error("Invalid storage value size");
    }

    Word256 value;
    std::copy(value_str.begin(), value_str.end(), value.begin());

    return Result<Word256>::Ok(value);
}

bool ContractDatabase::StorageKeyExists(
    const std::string& contract_address,
    const Word256& key) {

    if (!impl_->db) {
        return false;
    }

    std::string db_key = MakeContractStorageKey(contract_address, key);
    std::string value;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), db_key, &value);
    return status.ok();
}

Result<void> ContractDatabase::DeleteContractStorage(
    const std::string& contract_address,
    const Word256& key) {

    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string db_key = MakeContractStorageKey(contract_address, key);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Delete(db_key);
        return Result<void>::Ok();
    } else {
        status = impl_->db->Delete(rocksdb::WriteOptions(), db_key);

        if (!status.ok()) {
            return Result<void>::Error("Failed to delete storage: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<void> ContractDatabase::PutReceipt(const TransactionReceipt& receipt) {
    // TODO: Implement receipt serialization and storage
    (void)receipt;
    return Result<void>::Error("PutReceipt not yet implemented");
}

Result<TransactionReceipt> ContractDatabase::GetReceipt(const uint256& txid) {
    // TODO: Implement receipt deserialization
    (void)txid;
    return Result<TransactionReceipt>::Error("GetReceipt not yet implemented");
}

bool ContractDatabase::ReceiptExists(const uint256& txid) {
    (void)txid;
    return false;
}

Result<void> ContractDatabase::PutEventLog(const EventLogEntry& log) {
    // TODO: Implement event log storage
    (void)log;
    return Result<void>::Error("PutEventLog not yet implemented");
}

Result<std::vector<EventLogEntry>> ContractDatabase::QueryEventLogs(
    const std::string& contract_address,
    uint64_t from_block,
    uint64_t to_block,
    const std::vector<uint256>& topics) {

    // TODO: Implement event log querying
    (void)contract_address;
    (void)from_block;
    (void)to_block;
    (void)topics;

    return Result<std::vector<EventLogEntry>>::Ok(std::vector<EventLogEntry>{});
}

void ContractDatabase::BeginBatch() {
    if (impl_->batch) {
        delete impl_->batch;
    }
    impl_->batch = new rocksdb::WriteBatch();
    impl_->batch_active = true;
}

Result<void> ContractDatabase::CommitBatch() {
    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    if (!impl_->batch_active || !impl_->batch) {
        return Result<void>::Error("No active batch");
    }

    rocksdb::Status status = impl_->db->Write(rocksdb::WriteOptions(), impl_->batch);

    delete impl_->batch;
    impl_->batch = nullptr;
    impl_->batch_active = false;

    if (!status.ok()) {
        return Result<void>::Error("Failed to commit batch: " + status.ToString());
    }

    return Result<void>::Ok();
}

void ContractDatabase::DiscardBatch() {
    if (impl_->batch) {
        delete impl_->batch;
        impl_->batch = nullptr;
    }
    impl_->batch_active = false;
}

Result<ContractDatabase::Stats> ContractDatabase::GetStats() {
    if (!impl_->db) {
        return Result<Stats>::Error("Database not open");
    }

    Stats stats{};

    // Count contracts
    std::string prefix(1, db_prefix::CONTRACT_ACCOUNT);
    rocksdb::Iterator* it = impl_->db->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        stats.total_contracts++;
    }
    delete it;

    // TODO: Count receipts and logs
    stats.total_receipts = 0;
    stats.total_logs = 0;
    stats.db_size_bytes = 0;

    return Result<Stats>::Ok(stats);
}

} // namespace contracts
} // namespace intcoin
