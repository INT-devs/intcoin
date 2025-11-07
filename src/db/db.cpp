// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/db.h"
#include "intcoin/primitives.h"
#include <filesystem>
#include <iostream>

namespace intcoin {

// Database implementation

bool Database::open(const std::string& path, bool create_if_missing) {
    if (db_) {
        return false; // Already open
    }

    db_path_ = path;

    // Create directory if it doesn't exist
    if (create_if_missing) {
        std::filesystem::create_directories(path);
    }

    // Configure RocksDB options
    rocksdb::Options options;
    options.create_if_missing = create_if_missing;
    options.compression = rocksdb::kZSTD; // Use Zstandard compression
    options.max_open_files = 256;
    options.write_buffer_size = 64 * 1024 * 1024; // 64 MB
    options.max_write_buffer_number = 3;
    options.target_file_size_base = 64 * 1024 * 1024; // 64 MB

    // Performance optimizations
    options.level0_file_num_compaction_trigger = 4;
    options.level0_slowdown_writes_trigger = 20;
    options.level0_stop_writes_trigger = 30;
    options.max_background_jobs = 4;

    rocksdb::DB* db_ptr;
    rocksdb::Status status = rocksdb::DB::Open(options, path, &db_ptr);

    if (!status.ok()) {
        std::cerr << "Failed to open database at " << path << ": "
                  << status.ToString() << std::endl;
        return false;
    }

    db_.reset(db_ptr);
    return true;
}

void Database::close() {
    if (db_) {
        db_->Close();
        db_.reset();
    }
}

bool Database::write(const std::string& key, const std::string& value) {
    if (!db_) return false;

    rocksdb::WriteOptions options;
    options.sync = false; // Async write for performance

    rocksdb::Status status = db_->Put(options, key, value);
    return status.ok();
}

bool Database::write(const std::string& key, const std::vector<uint8_t>& value) {
    if (!db_) return false;

    rocksdb::WriteOptions options;
    options.sync = false;

    rocksdb::Slice value_slice(reinterpret_cast<const char*>(value.data()), value.size());
    rocksdb::Status status = db_->Put(options, key, value_slice);
    return status.ok();
}

std::optional<std::string> Database::read(const std::string& key) const {
    if (!db_) return std::nullopt;

    std::string value;
    rocksdb::ReadOptions options;
    rocksdb::Status status = db_->Get(options, key, &value);

    if (status.ok()) {
        return value;
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> Database::read_bytes(const std::string& key) const {
    if (!db_) return std::nullopt;

    std::string value;
    rocksdb::ReadOptions options;
    rocksdb::Status status = db_->Get(options, key, &value);

    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    return std::nullopt;
}

bool Database::erase(const std::string& key) {
    if (!db_) return false;

    rocksdb::WriteOptions options;
    rocksdb::Status status = db_->Delete(options, key);
    return status.ok();
}

bool Database::exists(const std::string& key) const {
    if (!db_) return false;

    std::string value;
    rocksdb::ReadOptions options;
    rocksdb::Status status = db_->Get(options, key, &value);
    return status.ok();
}

void Database::Batch::write(const std::string& key, const std::string& value) {
    batch_.Put(key, value);
}

void Database::Batch::write(const std::string& key, const std::vector<uint8_t>& value) {
    rocksdb::Slice value_slice(reinterpret_cast<const char*>(value.data()), value.size());
    batch_.Put(key, value_slice);
}

void Database::Batch::erase(const std::string& key) {
    batch_.Delete(key);
}

void Database::Batch::clear() {
    batch_.Clear();
}

bool Database::write_batch(const Batch& batch) {
    if (!db_) return false;

    rocksdb::WriteOptions options;
    options.sync = true; // Sync batch writes for consistency

    // Need to cast away const to pass to RocksDB Write()
    rocksdb::WriteBatch* batch_ptr = const_cast<rocksdb::WriteBatch*>(&batch.batch_);
    rocksdb::Status status = db_->Write(options, batch_ptr);
    return status.ok();
}

void Database::compact() {
    if (!db_) return;

    rocksdb::CompactRangeOptions options;
    db_->CompactRange(options, nullptr, nullptr);
}

Database::Stats Database::get_stats() const {
    Stats stats;
    stats.db_path = db_path_;
    stats.num_keys = 0;
    stats.total_size = 0;

    if (!db_) return stats;

    // Get approximate key count
    std::string num_keys_str;
    db_->GetProperty("rocksdb.estimate-num-keys", &num_keys_str);
    if (!num_keys_str.empty()) {
        stats.num_keys = std::stoull(num_keys_str);
    }

    // Get total size
    std::string total_size_str;
    db_->GetProperty("rocksdb.total-sst-files-size", &total_size_str);
    if (!total_size_str.empty()) {
        stats.total_size = std::stoull(total_size_str);
    }

    return stats;
}

// BlockIndexDB implementation

bool BlockIndexDB::open(const std::string& data_dir) {
    std::string db_path = data_dir + "/blocks";
    return db_.open(db_path, true);
}

void BlockIndexDB::close() {
    db_.close();
}

bool BlockIndexDB::write_block(const Hash256& hash, uint32_t height,
                               const std::vector<uint8_t>& block_data) {
    Database::Batch batch;

    // Write block hash -> data
    std::string hash_key = "b" + std::string(hash.begin(), hash.end());
    batch.write(hash_key, block_data);

    // Write height -> hash
    std::string height_key = "h" + std::to_string(height);
    batch.write(height_key, std::vector<uint8_t>(hash.begin(), hash.end()));

    // Write hash -> height
    std::string hash_height_key = "bh" + std::string(hash.begin(), hash.end());
    std::vector<uint8_t> height_bytes(4);
    height_bytes[0] = (height >> 24) & 0xFF;
    height_bytes[1] = (height >> 16) & 0xFF;
    height_bytes[2] = (height >> 8) & 0xFF;
    height_bytes[3] = height & 0xFF;
    batch.write(hash_height_key, height_bytes);

    return db_.write_batch(batch);
}

std::optional<std::vector<uint8_t>> BlockIndexDB::read_block(const Hash256& hash) const {
    std::string key = "b" + std::string(hash.begin(), hash.end());
    return db_.read_bytes(key);
}

std::optional<Hash256> BlockIndexDB::get_block_hash(uint32_t height) const {
    std::string key = "h" + std::to_string(height);
    auto hash_bytes = db_.read_bytes(key);

    if (hash_bytes && hash_bytes->size() == 32) {
        Hash256 hash;
        std::copy(hash_bytes->begin(), hash_bytes->end(), hash.begin());
        return hash;
    }
    return std::nullopt;
}

std::optional<uint32_t> BlockIndexDB::get_block_height(const Hash256& hash) const {
    std::string key = "bh" + std::string(hash.begin(), hash.end());
    auto height_bytes = db_.read_bytes(key);

    if (height_bytes && height_bytes->size() == 4) {
        uint32_t height = (static_cast<uint32_t>((*height_bytes)[0]) << 24) |
                         (static_cast<uint32_t>((*height_bytes)[1]) << 16) |
                         (static_cast<uint32_t>((*height_bytes)[2]) << 8) |
                         static_cast<uint32_t>((*height_bytes)[3]);
        return height;
    }
    return std::nullopt;
}

bool BlockIndexDB::has_block(const Hash256& hash) const {
    std::string key = "b" + std::string(hash.begin(), hash.end());
    return db_.exists(key);
}

std::optional<uint32_t> BlockIndexDB::get_best_height() const {
    auto height_str = db_.read("best_height");
    if (height_str) {
        return std::stoul(*height_str);
    }
    return std::nullopt;
}

bool BlockIndexDB::set_best_block(const Hash256& hash, uint32_t height) {
    Database::Batch batch;
    batch.write("best_height", std::to_string(height));
    batch.write("best_hash", std::vector<uint8_t>(hash.begin(), hash.end()));
    return db_.write_batch(batch);
}

// UTXODatabase implementation

bool UTXODatabase::open(const std::string& data_dir) {
    std::string db_path = data_dir + "/utxos";
    return db_.open(db_path, true);
}

void UTXODatabase::close() {
    db_.close();
}

bool UTXODatabase::write_utxo(const OutPoint& outpoint, const TxOutput& output, uint32_t height) {
    // Serialize OutPoint as key
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());

    // Serialize TxOutput + height as value
    std::vector<uint8_t> value_data;

    // Height (4 bytes)
    value_data.push_back((height >> 24) & 0xFF);
    value_data.push_back((height >> 16) & 0xFF);
    value_data.push_back((height >> 8) & 0xFF);
    value_data.push_back(height & 0xFF);

    // Value (8 bytes)
    for (int i = 7; i >= 0; --i) {
        value_data.push_back((output.value >> (i * 8)) & 0xFF);
    }

    // Script length (4 bytes)
    uint32_t script_len = output.script_pubkey.size();
    value_data.push_back((script_len >> 24) & 0xFF);
    value_data.push_back((script_len >> 16) & 0xFF);
    value_data.push_back((script_len >> 8) & 0xFF);
    value_data.push_back(script_len & 0xFF);

    // Script data
    value_data.insert(value_data.end(), output.script_pubkey.begin(), output.script_pubkey.end());

    return db_.write(key, value_data);
}

std::optional<std::pair<TxOutput, uint32_t>> UTXODatabase::read_utxo(const OutPoint& outpoint) const {
    // Serialize OutPoint as key
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());

    auto value_data = db_.read_bytes(key);
    if (!value_data || value_data->size() < 16) {
        return std::nullopt;
    }

    const auto& data = *value_data;
    size_t offset = 0;

    // Read height
    uint32_t height = (static_cast<uint32_t>(data[offset]) << 24) |
                     (static_cast<uint32_t>(data[offset + 1]) << 16) |
                     (static_cast<uint32_t>(data[offset + 2]) << 8) |
                     static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    // Read value
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | data[offset++];
    }

    // Read script length
    uint32_t script_len = (static_cast<uint32_t>(data[offset]) << 24) |
                         (static_cast<uint32_t>(data[offset + 1]) << 16) |
                         (static_cast<uint32_t>(data[offset + 2]) << 8) |
                         static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + script_len > data.size()) {
        return std::nullopt;
    }

    // Read script
    std::vector<uint8_t> script(data.begin() + offset, data.begin() + offset + script_len);

    TxOutput output;
    output.value = value;
    output.script_pubkey = script;

    return std::make_pair(output, height);
}

bool UTXODatabase::erase_utxo(const OutPoint& outpoint) {
    // Serialize OutPoint as key
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());
    return db_.erase(key);
}

bool UTXODatabase::has_utxo(const OutPoint& outpoint) const {
    // Serialize OutPoint as key
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());
    return db_.exists(key);
}

void UTXODatabase::Batch::add_utxo(const OutPoint& outpoint, const TxOutput& output, uint32_t height) {
    // Serialize OutPoint as key
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());

    // Serialize TxOutput + height as value
    std::vector<uint8_t> value_data;

    value_data.push_back((height >> 24) & 0xFF);
    value_data.push_back((height >> 16) & 0xFF);
    value_data.push_back((height >> 8) & 0xFF);
    value_data.push_back(height & 0xFF);

    for (int i = 7; i >= 0; --i) {
        value_data.push_back((output.value >> (i * 8)) & 0xFF);
    }

    uint32_t script_len = output.script_pubkey.size();
    value_data.push_back((script_len >> 24) & 0xFF);
    value_data.push_back((script_len >> 16) & 0xFF);
    value_data.push_back((script_len >> 8) & 0xFF);
    value_data.push_back(script_len & 0xFF);

    value_data.insert(value_data.end(), output.script_pubkey.begin(), output.script_pubkey.end());

    batch_.write(key, value_data);
}

void UTXODatabase::Batch::spend_utxo(const OutPoint& outpoint) {
    std::vector<uint8_t> key_data;
    key_data.insert(key_data.end(), outpoint.tx_hash.begin(), outpoint.tx_hash.end());
    key_data.push_back((outpoint.index >> 24) & 0xFF);
    key_data.push_back((outpoint.index >> 16) & 0xFF);
    key_data.push_back((outpoint.index >> 8) & 0xFF);
    key_data.push_back(outpoint.index & 0xFF);

    std::string key(key_data.begin(), key_data.end());
    batch_.erase(key);
}

bool UTXODatabase::apply_batch(const Batch& batch) {
    return db_.write_batch(batch.batch_);
}

size_t UTXODatabase::get_utxo_count() const {
    auto stats = db_.get_stats();
    return stats.num_keys;
}

// TransactionIndexDB implementation

bool TransactionIndexDB::open(const std::string& data_dir) {
    std::string db_path = data_dir + "/txindex";
    return db_.open(db_path, true);
}

void TransactionIndexDB::close() {
    db_.close();
}

bool TransactionIndexDB::write_transaction(const Hash256& tx_hash, const Hash256& block_hash,
                                          uint32_t height, const std::vector<uint8_t>& tx_data) {
    Database::Batch batch;

    // Write tx_hash -> tx_data
    std::string tx_key = "t" + std::string(tx_hash.begin(), tx_hash.end());
    batch.write(tx_key, tx_data);

    // Write tx_hash -> block_hash
    std::string block_key = "tb" + std::string(tx_hash.begin(), tx_hash.end());
    batch.write(block_key, std::vector<uint8_t>(block_hash.begin(), block_hash.end()));

    // Write tx_hash -> height
    std::string height_key = "th" + std::string(tx_hash.begin(), tx_hash.end());
    std::vector<uint8_t> height_bytes(4);
    height_bytes[0] = (height >> 24) & 0xFF;
    height_bytes[1] = (height >> 16) & 0xFF;
    height_bytes[2] = (height >> 8) & 0xFF;
    height_bytes[3] = height & 0xFF;
    batch.write(height_key, height_bytes);

    return db_.write_batch(batch);
}

std::optional<std::vector<uint8_t>> TransactionIndexDB::read_transaction(const Hash256& tx_hash) const {
    std::string key = "t" + std::string(tx_hash.begin(), tx_hash.end());
    return db_.read_bytes(key);
}

std::optional<Hash256> TransactionIndexDB::get_transaction_block(const Hash256& tx_hash) const {
    std::string key = "tb" + std::string(tx_hash.begin(), tx_hash.end());
    auto hash_bytes = db_.read_bytes(key);

    if (hash_bytes && hash_bytes->size() == 32) {
        Hash256 hash;
        std::copy(hash_bytes->begin(), hash_bytes->end(), hash.begin());
        return hash;
    }
    return std::nullopt;
}

std::optional<uint32_t> TransactionIndexDB::get_transaction_height(const Hash256& tx_hash) const {
    std::string key = "th" + std::string(tx_hash.begin(), tx_hash.end());
    auto height_bytes = db_.read_bytes(key);

    if (height_bytes && height_bytes->size() == 4) {
        uint32_t height = (static_cast<uint32_t>((*height_bytes)[0]) << 24) |
                         (static_cast<uint32_t>((*height_bytes)[1]) << 16) |
                         (static_cast<uint32_t>((*height_bytes)[2]) << 8) |
                         static_cast<uint32_t>((*height_bytes)[3]);
        return height;
    }
    return std::nullopt;
}

bool TransactionIndexDB::has_transaction(const Hash256& tx_hash) const {
    std::string key = "t" + std::string(tx_hash.begin(), tx_hash.end());
    return db_.exists(key);
}

} // namespace intcoin
