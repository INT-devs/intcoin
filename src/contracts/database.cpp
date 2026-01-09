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

//
// Receipt serialization
//

static std::vector<uint8_t> SerializeReceipt(const TransactionReceipt& receipt) {
    std::vector<uint8_t> data;

    // Transaction ID (32 bytes)
    data.insert(data.end(), receipt.txid.begin(), receipt.txid.end());

    // Contract address (length-prefixed)
    uint32_t addr_len = receipt.contract_address.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((addr_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), receipt.contract_address.begin(), receipt.contract_address.end());

    // From address (length-prefixed)
    uint32_t from_len = receipt.from.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((from_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), receipt.from.begin(), receipt.from.end());

    // To address (length-prefixed)
    uint32_t to_len = receipt.to.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((to_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), receipt.to.begin(), receipt.to.end());

    // Gas used (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((receipt.gas_used >> (i * 8)) & 0xff);
    }

    // Gas price (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((receipt.gas_price >> (i * 8)) & 0xff);
    }

    // Total fee (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((receipt.total_fee >> (i * 8)) & 0xff);
    }

    // Status (1 byte)
    data.push_back(static_cast<uint8_t>(receipt.status));

    // Return data (length-prefixed)
    uint32_t return_len = receipt.return_data.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((return_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), receipt.return_data.begin(), receipt.return_data.end());

    // Number of logs (4 bytes)
    uint32_t log_count = receipt.logs.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((log_count >> (i * 8)) & 0xff);
    }

    // Block number (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((receipt.block_number >> (i * 8)) & 0xff);
    }

    // Block timestamp (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((receipt.block_timestamp >> (i * 8)) & 0xff);
    }

    // Transaction index (4 bytes)
    for (int i = 3; i >= 0; --i) {
        data.push_back((receipt.tx_index >> (i * 8)) & 0xff);
    }

    return data;
}

static Result<TransactionReceipt> DeserializeReceipt(const std::vector<uint8_t>& data) {
    if (data.size() < 32) {
        return Result<TransactionReceipt>::Error("Invalid data size");
    }

    TransactionReceipt receipt;
    size_t offset = 0;

    // Transaction ID
    std::copy(data.begin(), data.begin() + 32, receipt.txid.begin());
    offset += 32;

    // Contract address
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid contract address");
    uint32_t addr_len = 0;
    for (int i = 0; i < 4; ++i) {
        addr_len = (addr_len << 8) | data[offset++];
    }
    if (offset + addr_len > data.size()) return Result<TransactionReceipt>::Error("Invalid contract address length");
    receipt.contract_address = std::string(data.begin() + offset, data.begin() + offset + addr_len);
    offset += addr_len;

    // From address
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid from address");
    uint32_t from_len = 0;
    for (int i = 0; i < 4; ++i) {
        from_len = (from_len << 8) | data[offset++];
    }
    if (offset + from_len > data.size()) return Result<TransactionReceipt>::Error("Invalid from address length");
    receipt.from = std::string(data.begin() + offset, data.begin() + offset + from_len);
    offset += from_len;

    // To address
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid to address");
    uint32_t to_len = 0;
    for (int i = 0; i < 4; ++i) {
        to_len = (to_len << 8) | data[offset++];
    }
    if (offset + to_len > data.size()) return Result<TransactionReceipt>::Error("Invalid to address length");
    receipt.to = std::string(data.begin() + offset, data.begin() + offset + to_len);
    offset += to_len;

    // Gas used
    if (offset + 8 > data.size()) return Result<TransactionReceipt>::Error("Invalid gas used");
    receipt.gas_used = 0;
    for (int i = 0; i < 8; ++i) {
        receipt.gas_used = (receipt.gas_used << 8) | data[offset++];
    }

    // Gas price
    if (offset + 8 > data.size()) return Result<TransactionReceipt>::Error("Invalid gas price");
    receipt.gas_price = 0;
    for (int i = 0; i < 8; ++i) {
        receipt.gas_price = (receipt.gas_price << 8) | data[offset++];
    }

    // Total fee
    if (offset + 8 > data.size()) return Result<TransactionReceipt>::Error("Invalid total fee");
    receipt.total_fee = 0;
    for (int i = 0; i < 8; ++i) {
        receipt.total_fee = (receipt.total_fee << 8) | data[offset++];
    }

    // Status
    if (offset >= data.size()) return Result<TransactionReceipt>::Error("Invalid status");
    receipt.status = static_cast<ExecutionResult>(data[offset++]);

    // Return data
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid return data");
    uint32_t return_len = 0;
    for (int i = 0; i < 4; ++i) {
        return_len = (return_len << 8) | data[offset++];
    }
    if (offset + return_len > data.size()) return Result<TransactionReceipt>::Error("Invalid return data length");
    receipt.return_data = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + return_len);
    offset += return_len;

    // Log count (we don't deserialize logs here, they're stored separately)
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid log count");
    offset += 4; // Skip log count

    // Block number
    if (offset + 8 > data.size()) return Result<TransactionReceipt>::Error("Invalid block number");
    receipt.block_number = 0;
    for (int i = 0; i < 8; ++i) {
        receipt.block_number = (receipt.block_number << 8) | data[offset++];
    }

    // Block timestamp
    if (offset + 8 > data.size()) return Result<TransactionReceipt>::Error("Invalid block timestamp");
    receipt.block_timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        receipt.block_timestamp = (receipt.block_timestamp << 8) | data[offset++];
    }

    // Transaction index
    if (offset + 4 > data.size()) return Result<TransactionReceipt>::Error("Invalid tx index");
    receipt.tx_index = 0;
    for (int i = 0; i < 4; ++i) {
        receipt.tx_index = (receipt.tx_index << 8) | data[offset++];
    }

    return Result<TransactionReceipt>::Ok(std::move(receipt));
}

//
// Event log serialization
//

static std::vector<uint8_t> SerializeEventLog(const EventLogEntry& log) {
    std::vector<uint8_t> data;

    // Contract address (length-prefixed)
    uint32_t addr_len = log.contract_address.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((addr_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), log.contract_address.begin(), log.contract_address.end());

    // Number of topics (4 bytes)
    uint32_t topic_count = log.topics.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((topic_count >> (i * 8)) & 0xff);
    }

    // Topics (32 bytes each)
    for (const auto& topic : log.topics) {
        data.insert(data.end(), topic.begin(), topic.end());
    }

    // Data (length-prefixed)
    uint32_t data_len = log.data.size();
    for (int i = 3; i >= 0; --i) {
        data.push_back((data_len >> (i * 8)) & 0xff);
    }
    data.insert(data.end(), log.data.begin(), log.data.end());

    // Block number (8 bytes)
    for (int i = 7; i >= 0; --i) {
        data.push_back((log.block_number >> (i * 8)) & 0xff);
    }

    // Transaction hash (32 bytes)
    data.insert(data.end(), log.transaction_hash.begin(), log.transaction_hash.end());

    // Log index (4 bytes)
    for (int i = 3; i >= 0; --i) {
        data.push_back((log.log_index >> (i * 8)) & 0xff);
    }

    return data;
}

static Result<EventLogEntry> DeserializeEventLog(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return Result<EventLogEntry>::Error("Invalid data size");
    }

    EventLogEntry log;
    size_t offset = 0;

    // Contract address
    uint32_t addr_len = 0;
    for (int i = 0; i < 4; ++i) {
        addr_len = (addr_len << 8) | data[offset++];
    }
    if (offset + addr_len > data.size()) return Result<EventLogEntry>::Error("Invalid address length");
    log.contract_address = std::string(data.begin() + offset, data.begin() + offset + addr_len);
    offset += addr_len;

    // Topic count
    if (offset + 4 > data.size()) return Result<EventLogEntry>::Error("Invalid topic count");
    uint32_t topic_count = 0;
    for (int i = 0; i < 4; ++i) {
        topic_count = (topic_count << 8) | data[offset++];
    }

    // Topics
    for (uint32_t i = 0; i < topic_count; ++i) {
        if (offset + 32 > data.size()) return Result<EventLogEntry>::Error("Invalid topic");
        uint256 topic;
        std::copy(data.begin() + offset, data.begin() + offset + 32, topic.begin());
        log.topics.push_back(topic);
        offset += 32;
    }

    // Data
    if (offset + 4 > data.size()) return Result<EventLogEntry>::Error("Invalid data length");
    uint32_t data_len = 0;
    for (int i = 0; i < 4; ++i) {
        data_len = (data_len << 8) | data[offset++];
    }
    if (offset + data_len > data.size()) return Result<EventLogEntry>::Error("Invalid data");
    log.data = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + data_len);
    offset += data_len;

    // Block number
    if (offset + 8 > data.size()) return Result<EventLogEntry>::Error("Invalid block number");
    log.block_number = 0;
    for (int i = 0; i < 8; ++i) {
        log.block_number = (log.block_number << 8) | data[offset++];
    }

    // Transaction hash
    if (offset + 32 > data.size()) return Result<EventLogEntry>::Error("Invalid tx hash");
    std::copy(data.begin() + offset, data.begin() + offset + 32, log.transaction_hash.begin());
    offset += 32;

    // Log index
    if (offset + 4 > data.size()) return Result<EventLogEntry>::Error("Invalid log index");
    log.log_index = 0;
    for (int i = 0; i < 4; ++i) {
        log.log_index = (log.log_index << 8) | data[offset++];
    }

    return Result<EventLogEntry>::Ok(std::move(log));
}

Result<void> ContractDatabase::PutReceipt(const TransactionReceipt& receipt) {
    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string key = MakeReceiptKey(receipt.txid);
    std::vector<uint8_t> value = SerializeReceipt(receipt);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Put(key, rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));
        return Result<void>::Ok();
    } else {
        status = impl_->db->Put(rocksdb::WriteOptions(), key,
            rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));

        if (!status.ok()) {
            return Result<void>::Error("Failed to write receipt: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<TransactionReceipt> ContractDatabase::GetReceipt(const uint256& txid) {
    if (!impl_->db) {
        return Result<TransactionReceipt>::Error("Database not open");
    }

    std::string key = MakeReceiptKey(txid);
    std::string value_str;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value_str);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            return Result<TransactionReceipt>::Error("Receipt not found");
        }
        return Result<TransactionReceipt>::Error("Database read error: " + status.ToString());
    }

    std::vector<uint8_t> value(value_str.begin(), value_str.end());
    return DeserializeReceipt(value);
}

bool ContractDatabase::ReceiptExists(const uint256& txid) {
    if (!impl_->db) {
        return false;
    }

    std::string key = MakeReceiptKey(txid);
    std::string value;

    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);
    return status.ok();
}

Result<void> ContractDatabase::PutEventLog(const EventLogEntry& log) {
    if (!impl_->db) {
        return Result<void>::Error("Database not open");
    }

    std::string key = MakeEventLogKey(log.block_number, log.transaction_hash, log.log_index);
    std::vector<uint8_t> value = SerializeEventLog(log);

    rocksdb::Status status;
    if (impl_->batch_active && impl_->batch) {
        impl_->batch->Put(key, rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));
        return Result<void>::Ok();
    } else {
        status = impl_->db->Put(rocksdb::WriteOptions(), key,
            rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size()));

        if (!status.ok()) {
            return Result<void>::Error("Failed to write event log: " + status.ToString());
        }
        return Result<void>::Ok();
    }
}

Result<std::vector<EventLogEntry>> ContractDatabase::QueryEventLogs(
    const std::string& contract_address,
    uint64_t from_block,
    uint64_t to_block,
    const std::vector<uint256>& topics) {

    if (!impl_->db) {
        return Result<std::vector<EventLogEntry>>::Error("Database not open");
    }

    std::vector<EventLogEntry> results;

    // Construct search range using block numbers
    std::string start_key(1, db_prefix::EVENT_LOG);
    for (int i = 7; i >= 0; --i) {
        start_key += static_cast<char>((from_block >> (i * 8)) & 0xff);
    }

    std::string end_key(1, db_prefix::EVENT_LOG);
    for (int i = 7; i >= 0; --i) {
        end_key += static_cast<char>((to_block >> (i * 8)) & 0xff);
    }
    // Add max values to make this inclusive
    for (int i = 0; i < 32 + 4; ++i) {
        end_key += static_cast<char>(0xff);
    }

    // Iterate through event logs in the block range
    rocksdb::Iterator* it = impl_->db->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(start_key); it->Valid() && it->key().ToString() < end_key; it->Next()) {
        std::string value_str = it->value().ToString();
        std::vector<uint8_t> value(value_str.begin(), value_str.end());

        auto log_result = DeserializeEventLog(value);
        if (!log_result.IsOk()) {
            continue; // Skip invalid logs
        }

        EventLogEntry log = log_result.GetValue();

        // Filter by contract address if specified
        if (!contract_address.empty() && log.contract_address != contract_address) {
            continue;
        }

        // Filter by topics if specified
        bool topic_match = true;
        for (size_t i = 0; i < topics.size() && i < log.topics.size(); ++i) {
            // Empty topic means "match any"
            bool is_empty = true;
            for (uint8_t byte : topics[i]) {
                if (byte != 0) {
                    is_empty = false;
                    break;
                }
            }

            if (!is_empty && topics[i] != log.topics[i]) {
                topic_match = false;
                break;
            }
        }

        if (topic_match) {
            results.push_back(std::move(log));
        }
    }

    delete it;

    return Result<std::vector<EventLogEntry>>::Ok(std::move(results));
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
    std::string contract_prefix(1, db_prefix::CONTRACT_ACCOUNT);
    rocksdb::Iterator* it = impl_->db->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(contract_prefix); it->Valid() && it->key().starts_with(contract_prefix); it->Next()) {
        stats.total_contracts++;
    }
    delete it;

    // Count receipts
    std::string receipt_prefix(1, db_prefix::TX_RECEIPT);
    it = impl_->db->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(receipt_prefix); it->Valid() && it->key().starts_with(receipt_prefix); it->Next()) {
        stats.total_receipts++;
    }
    delete it;

    // Count logs
    std::string log_prefix(1, db_prefix::EVENT_LOG);
    it = impl_->db->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(log_prefix); it->Valid() && it->key().starts_with(log_prefix); it->Next()) {
        stats.total_logs++;
    }
    delete it;

    // Get approximate database size
    std::string size_str;
    impl_->db->GetProperty("rocksdb.total-sst-files-size", &size_str);
    if (!size_str.empty()) {
        stats.db_size_bytes = std::stoull(size_str);
    }

    return Result<Stats>::Ok(stats);
}

} // namespace contracts
} // namespace intcoin
