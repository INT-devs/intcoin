/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Storage Layer Implementation (RocksDB Backend)
 */

#include "intcoin/storage.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/slice.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <rocksdb/cache.h>
#include <rocksdb/utilities/backup_engine.h>
#include <unordered_map>
#include <shared_mutex>
#include <iostream>

namespace intcoin {

// ============================================================================
// ChainState Serialization
// ============================================================================

std::vector<uint8_t> ChainState::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint256(result, best_block_hash);
    SerializeUint64(result, best_height);
    SerializeUint256(result, chain_work);
    SerializeUint64(result, total_transactions);
    SerializeUint64(result, utxo_count);
    SerializeUint64(result, total_supply);
    return result;
}

Result<ChainState> ChainState::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    ChainState state;

    // Deserialize best_block_hash (32 bytes)
    auto hash_result = DeserializeUint256(data, pos);
    if (hash_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize best_block_hash: " + hash_result.error);
    }
    state.best_block_hash = *hash_result.value;

    // Deserialize best_height (8 bytes)
    auto height_result = DeserializeUint64(data, pos);
    if (height_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize best_height: " + height_result.error);
    }
    state.best_height = *height_result.value;

    // Deserialize chain_work (32 bytes)
    auto work_result = DeserializeUint256(data, pos);
    if (work_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize chain_work: " + work_result.error);
    }
    state.chain_work = *work_result.value;

    // Deserialize total_transactions (8 bytes)
    auto tx_count_result = DeserializeUint64(data, pos);
    if (tx_count_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize total_transactions: " + tx_count_result.error);
    }
    state.total_transactions = *tx_count_result.value;

    // Deserialize utxo_count (8 bytes)
    auto utxo_result = DeserializeUint64(data, pos);
    if (utxo_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize utxo_count: " + utxo_result.error);
    }
    state.utxo_count = *utxo_result.value;

    // Deserialize total_supply (8 bytes)
    auto supply_result = DeserializeUint64(data, pos);
    if (supply_result.IsError()) {
        return Result<ChainState>::Error("Failed to deserialize total_supply: " + supply_result.error);
    }
    state.total_supply = *supply_result.value;

    return Result<ChainState>::Ok(std::move(state));
}

// ============================================================================
// SpentOutput Serialization
// ============================================================================

std::vector<uint8_t> SpentOutput::Serialize() const {
    std::vector<uint8_t> result;

    // Serialize outpoint
    auto outpoint_data = outpoint.Serialize();
    result.insert(result.end(), outpoint_data.begin(), outpoint_data.end());

    // Serialize output
    auto output_data = output.Serialize();
    result.insert(result.end(), output_data.begin(), output_data.end());

    return result;
}

Result<SpentOutput> SpentOutput::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    SpentOutput spent;

    // Deserialize outpoint
    std::vector<uint8_t> outpoint_data(data.begin(), data.end());
    auto outpoint_result = OutPoint::Deserialize(outpoint_data);
    if (outpoint_result.IsError()) {
        return Result<SpentOutput>::Error("Failed to deserialize outpoint: " +
                                         outpoint_result.error);
    }
    spent.outpoint = *outpoint_result.value;
    pos += outpoint_result.value->Serialize().size();

    // Deserialize output
    std::vector<uint8_t> output_data(data.begin() + pos, data.end());
    auto output_result = TxOut::Deserialize(output_data);
    if (output_result.IsError()) {
        return Result<SpentOutput>::Error("Failed to deserialize output: " +
                                         output_result.error);
    }
    spent.output = *output_result.value;

    return Result<SpentOutput>::Ok(std::move(spent));
}

// ============================================================================
// BlockIndex Serialization
// ============================================================================

std::vector<uint8_t> BlockIndex::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint256(result, hash);
    SerializeUint64(result, height);
    SerializeUint256(result, prev_hash);
    SerializeUint64(result, timestamp);
    SerializeUint32(result, bits);
    SerializeUint256(result, chain_work);
    SerializeUint32(result, tx_count);
    SerializeUint32(result, size);
    SerializeUint64(result, file_pos);
    return result;
}

Result<BlockIndex> BlockIndex::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    BlockIndex index;

    // Deserialize hash (32 bytes)
    auto hash_result = DeserializeUint256(data, pos);
    if (hash_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize hash: " + hash_result.error);
    }
    index.hash = *hash_result.value;

    // Deserialize height (8 bytes)
    auto height_result = DeserializeUint64(data, pos);
    if (height_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize height: " + height_result.error);
    }
    index.height = *height_result.value;

    // Deserialize prev_hash (32 bytes)
    auto prev_result = DeserializeUint256(data, pos);
    if (prev_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize prev_hash: " + prev_result.error);
    }
    index.prev_hash = *prev_result.value;

    // Deserialize timestamp (8 bytes)
    auto time_result = DeserializeUint64(data, pos);
    if (time_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize timestamp: " + time_result.error);
    }
    index.timestamp = *time_result.value;

    // Deserialize bits (4 bytes)
    auto bits_result = DeserializeUint32(data, pos);
    if (bits_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize bits: " + bits_result.error);
    }
    index.bits = *bits_result.value;

    // Deserialize chain_work (32 bytes)
    auto work_result = DeserializeUint256(data, pos);
    if (work_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize chain_work: " + work_result.error);
    }
    index.chain_work = *work_result.value;

    // Deserialize tx_count (4 bytes)
    auto tx_result = DeserializeUint32(data, pos);
    if (tx_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize tx_count: " + tx_result.error);
    }
    index.tx_count = *tx_result.value;

    // Deserialize size (4 bytes)
    auto size_result = DeserializeUint32(data, pos);
    if (size_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize size: " + size_result.error);
    }
    index.size = *size_result.value;

    // Deserialize file_pos (8 bytes)
    auto pos_result = DeserializeUint64(data, pos);
    if (pos_result.IsError()) {
        return Result<BlockIndex>::Error("Failed to deserialize file_pos: " + pos_result.error);
    }
    index.file_pos = *pos_result.value;

    return Result<BlockIndex>::Ok(std::move(index));
}

// ============================================================================
// BlockchainDB Implementation
// ============================================================================

class BlockchainDB::Impl {
public:
    rocksdb::DB* db_;
    rocksdb::WriteBatch* batch_;
    std::string data_dir_;
    bool is_open_;
    bool in_batch_;
    bool pruning_enabled_;
    uint64_t pruning_target_size_;

    Impl(const std::string& data_dir)
        : db_(nullptr)
        , batch_(nullptr)
        , data_dir_(data_dir)
        , is_open_(false)
        , in_batch_(false)
        , pruning_enabled_(false)
        , pruning_target_size_(0)
    {}

    ~Impl() {
        if (batch_) {
            delete batch_;
            batch_ = nullptr;
        }
        if (db_) {
            delete db_;
            db_ = nullptr;
        }
    }

    // Helper: Create database key
    std::string MakeKey(char prefix, const uint256& hash) const {
        std::string key;
        key.push_back(prefix);
        key.append(reinterpret_cast<const char*>(hash.data()), hash.size());
        return key;
    }

    std::string MakeKey(char prefix, uint64_t value) const {
        std::string key;
        key.push_back(prefix);
        std::vector<uint8_t> data;
        SerializeUint64(data, value);
        key.append(reinterpret_cast<const char*>(data.data()), data.size());
        return key;
    }

    std::string MakeKey(char prefix, const OutPoint& outpoint) const {
        std::string key;
        key.push_back(prefix);
        auto serialized = outpoint.Serialize();
        key.append(reinterpret_cast<const char*>(serialized.data()), serialized.size());
        return key;
    }

    std::string MakeKey(char prefix) const {
        return std::string(1, prefix);
    }

    // Helper: Put data
    rocksdb::Status Put(const std::string& key, const std::vector<uint8_t>& value) {
        rocksdb::Slice key_slice(key);
        rocksdb::Slice value_slice(reinterpret_cast<const char*>(value.data()), value.size());

        if (in_batch_ && batch_) {
            batch_->Put(key_slice, value_slice);
            return rocksdb::Status::OK();
        } else {
            rocksdb::WriteOptions options;
            return db_->Put(options, key_slice, value_slice);
        }
    }

    // Helper: Get data
    rocksdb::Status Get(const std::string& key, std::string& value) const {
        rocksdb::ReadOptions options;
        return db_->Get(options, key, &value);
    }

    // Helper: Delete data
    rocksdb::Status Delete(const std::string& key) {
        if (in_batch_ && batch_) {
            batch_->Delete(key);
            return rocksdb::Status::OK();
        } else {
            rocksdb::WriteOptions options;
            return db_->Delete(options, key);
        }
    }

    // Helper: Check if key exists
    bool Exists(const std::string& key) const {
        std::string value;
        rocksdb::Status s = Get(key, value);
        return s.ok();
    }
};

// Constructor
BlockchainDB::BlockchainDB(const std::string& data_dir)
    : impl_(std::make_unique<Impl>(data_dir))
{}

// Destructor
BlockchainDB::~BlockchainDB() {
    Close();
}

// Open database
Result<void> BlockchainDB::Open() {
    if (impl_->is_open_) {
        return Result<void>::Error("Database already open");
    }

    // Configure RocksDB options
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kLZ4Compression;
    options.max_open_files = 512;
    options.write_buffer_size = 64 * 1024 * 1024; // 64 MB
    options.max_write_buffer_number = 3;

    // Configure block-based table options
    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(256 * 1024 * 1024); // 256 MB cache
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    // Open database
    rocksdb::Status status = rocksdb::DB::Open(options, impl_->data_dir_, &impl_->db_);
    if (!status.ok()) {
        return Result<void>::Error("Failed to open database: " + status.ToString());
    }

    impl_->is_open_ = true;
    return Result<void>::Ok();
}

// Close database
void BlockchainDB::Close() {
    if (impl_->is_open_) {
        if (impl_->batch_) {
            delete impl_->batch_;
            impl_->batch_ = nullptr;
        }
        if (impl_->db_) {
            delete impl_->db_;
            impl_->db_ = nullptr;
        }
        impl_->is_open_ = false;
    }
}

// Check if open
bool BlockchainDB::IsOpen() const {
    return impl_->is_open_;
}

// Get data directory
std::string BlockchainDB::GetDataDir() const {
    return impl_->data_dir_;
}

// ============================================================================
// Block Operations
// ============================================================================

Result<void> BlockchainDB::StoreBlock(const Block& block) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Serialize block
    auto serialized = block.Serialize();

    // Store block data
    std::string key = impl_->MakeKey(db::PREFIX_BLOCK, block.GetHash());
    rocksdb::Status status = impl_->Put(key, serialized);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store block: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<Block> BlockchainDB::GetBlock(const uint256& hash) const {
    if (!impl_->is_open_) {
        return Result<Block>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_BLOCK, hash);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        return Result<Block>::Error("Block not found: " + ToHex(hash));
    }

    // Deserialize block
    std::vector<uint8_t> data(value.begin(), value.end());
    return Block::Deserialize(data);
}

Result<Block> BlockchainDB::GetBlockByHeight(uint64_t height) const {
    // First get the hash for this height
    auto hash_result = GetBlockHash(height);
    if (hash_result.IsError()) {
        return Result<Block>::Error(hash_result.error);
    }

    // Then get the block
    return GetBlock(*hash_result.value);
}

bool BlockchainDB::HasBlock(const uint256& hash) const {
    if (!impl_->is_open_) {
        return false;
    }

    std::string key = impl_->MakeKey(db::PREFIX_BLOCK, hash);
    return impl_->Exists(key);
}

Result<void> BlockchainDB::DeleteBlock(const uint256& hash) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Check if block exists first
    std::string key = impl_->MakeKey(db::PREFIX_BLOCK, hash);
    if (!impl_->Exists(key)) {
        return Result<void>::Error("Block not found: " + ToHex(hash));
    }

    rocksdb::Status status = impl_->Delete(key);

    if (!status.ok()) {
        return Result<void>::Error("Failed to delete block: " + status.ToString());
    }

    return Result<void>::Ok();
}

// ============================================================================
// Block Index Operations
// ============================================================================

Result<void> BlockchainDB::StoreBlockIndex(const BlockIndex& index) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    auto serialized = index.Serialize();
    std::string key = impl_->MakeKey(db::PREFIX_BLOCK_INDEX, index.hash);
    rocksdb::Status status = impl_->Put(key, serialized);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store block index: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<BlockIndex> BlockchainDB::GetBlockIndex(const uint256& hash) const {
    if (!impl_->is_open_) {
        return Result<BlockIndex>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_BLOCK_INDEX, hash);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        return Result<BlockIndex>::Error("Block index not found");
    }

    std::vector<uint8_t> data(value.begin(), value.end());
    return BlockIndex::Deserialize(data);
}

Result<uint256> BlockchainDB::GetBlockHash(uint64_t height) const {
    if (!impl_->is_open_) {
        return Result<uint256>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_BLOCK_HEIGHT, height);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        return Result<uint256>::Error("Block hash not found for height " + std::to_string(height));
    }

    // Deserialize uint256
    std::vector<uint8_t> data(value.begin(), value.end());
    size_t pos = 0;
    return DeserializeUint256(data, pos);
}

Result<void> BlockchainDB::StoreBlockHeight(uint64_t height, const uint256& hash) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_BLOCK_HEIGHT, height);
    std::vector<uint8_t> value;
    SerializeUint256(value, hash);
    rocksdb::Status status = impl_->Put(key, value);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store block height: " + status.ToString());
    }

    return Result<void>::Ok();
}

// ============================================================================
// Transaction Operations
// ============================================================================

Result<void> BlockchainDB::StoreTransaction(const Transaction& tx) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    auto serialized = tx.Serialize();
    std::string key = impl_->MakeKey(db::PREFIX_TX, tx.GetHash());
    rocksdb::Status status = impl_->Put(key, serialized);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store transaction: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<Transaction> BlockchainDB::GetTransaction(const uint256& hash) const {
    if (!impl_->is_open_) {
        return Result<Transaction>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_TX, hash);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        return Result<Transaction>::Error("Transaction not found");
    }

    std::vector<uint8_t> data(value.begin(), value.end());
    return Transaction::Deserialize(data);
}

bool BlockchainDB::HasTransaction(const uint256& hash) const {
    if (!impl_->is_open_) {
        return false;
    }

    std::string key = impl_->MakeKey(db::PREFIX_TX, hash);
    return impl_->Exists(key);
}

Result<void> BlockchainDB::DeleteTransaction(const uint256& hash) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Check if transaction exists first
    std::string key = impl_->MakeKey(db::PREFIX_TX, hash);
    if (!impl_->Exists(key)) {
        return Result<void>::Error("Transaction not found: " + ToHex(hash));
    }

    rocksdb::Status status = impl_->Delete(key);

    if (!status.ok()) {
        return Result<void>::Error("Failed to delete transaction: " + status.ToString());
    }

    return Result<void>::Ok();
}

// ============================================================================
// UTXO Operations
// ============================================================================

Result<void> BlockchainDB::StoreUTXO(const OutPoint& outpoint, const TxOut& output) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    auto serialized = output.Serialize();
    std::string key = impl_->MakeKey(db::PREFIX_UTXO, outpoint);
    rocksdb::Status status = impl_->Put(key, serialized);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store UTXO: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<TxOut> BlockchainDB::GetUTXO(const OutPoint& outpoint) const {
    if (!impl_->is_open_) {
        return Result<TxOut>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_UTXO, outpoint);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        return Result<TxOut>::Error("UTXO not found");
    }

    std::vector<uint8_t> data(value.begin(), value.end());
    return TxOut::Deserialize(data);
}

bool BlockchainDB::HasUTXO(const OutPoint& outpoint) const {
    if (!impl_->is_open_) {
        return false;
    }

    std::string key = impl_->MakeKey(db::PREFIX_UTXO, outpoint);
    return impl_->Exists(key);
}

Result<void> BlockchainDB::DeleteUTXO(const OutPoint& outpoint) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Check if UTXO exists first
    std::string key = impl_->MakeKey(db::PREFIX_UTXO, outpoint);
    if (!impl_->Exists(key)) {
        return Result<void>::Error("UTXO not found");
    }

    rocksdb::Status status = impl_->Delete(key);

    if (!status.ok()) {
        return Result<void>::Error("Failed to delete UTXO: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<std::vector<std::pair<OutPoint, TxOut>>> BlockchainDB::GetUTXOsForAddress(
    const std::string& address) const {
    // TODO: Implement address UTXO lookup (requires address index)
    return Result<std::vector<std::pair<OutPoint, TxOut>>>::Error("Not implemented");
}

Result<std::vector<std::pair<OutPoint, TxOut>>> BlockchainDB::GetAllUTXOs(size_t limit) const {
    if (!impl_->is_open_) {
        return Result<std::vector<std::pair<OutPoint, TxOut>>>::Error("Database not open");
    }

    std::vector<std::pair<OutPoint, TxOut>> utxos;

    // Create iterator for UTXO prefix
    std::string prefix_key(1, db::PREFIX_UTXO);
    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(impl_->db_->NewIterator(read_options));

    // Seek to first UTXO entry
    it->Seek(prefix_key);

    size_t count = 0;
    while (it->Valid()) {
        rocksdb::Slice key_slice = it->key();
        std::string key_str = key_slice.ToString();

        // Check if we're still in the UTXO prefix
        if (key_str.empty() || key_str[0] != db::PREFIX_UTXO) {
            break;  // Reached end of UTXO entries
        }

        // Parse OutPoint from key (skip prefix byte)
        std::vector<uint8_t> key_data(key_str.begin() + 1, key_str.end());
        auto outpoint_result = OutPoint::Deserialize(key_data);
        if (outpoint_result.IsError()) {
            it->Next();
            continue;  // Skip invalid entries
        }

        // Parse TxOut from value
        rocksdb::Slice value_slice = it->value();
        std::vector<uint8_t> value_data(value_slice.data(),
                                        value_slice.data() + value_slice.size());
        auto txout_result = TxOut::Deserialize(value_data);
        if (txout_result.IsError()) {
            it->Next();
            continue;  // Skip invalid entries
        }

        utxos.push_back({*outpoint_result.value, *txout_result.value});
        count++;

        // Check limit
        if (limit > 0 && count >= limit) {
            break;
        }

        it->Next();
    }

    if (!it->status().ok()) {
        return Result<std::vector<std::pair<OutPoint, TxOut>>>::Error(
            "Iterator error: " + it->status().ToString());
    }

    return Result<std::vector<std::pair<OutPoint, TxOut>>>::Ok(std::move(utxos));
}

// ============================================================================
// Spent Outputs Operations (for reorganization)
// ============================================================================

Result<void> BlockchainDB::StoreSpentOutputs(const uint256& block_hash,
                                             const std::vector<SpentOutput>& spent_outputs) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Build database key: PREFIX_SPENT_OUTPUTS + block_hash
    std::string key;
    key.push_back(db::PREFIX_SPENT_OUTPUTS);
    key.append(reinterpret_cast<const char*>(block_hash.data()), block_hash.size());

    // Serialize vector of spent outputs
    std::vector<uint8_t> value_data;

    // Serialize count
    SerializeUint64(value_data, static_cast<uint64_t>(spent_outputs.size()));

    // Serialize each spent output
    for (const SpentOutput& spent : spent_outputs) {
        auto spent_data = spent.Serialize();
        value_data.insert(value_data.end(), spent_data.begin(), spent_data.end());
    }

    // Write to database
    rocksdb::WriteOptions write_options;
    std::string value_str(value_data.begin(), value_data.end());
    rocksdb::Status status = impl_->db_->Put(write_options, key, value_str);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store spent outputs: " +
                                  status.ToString());
    }

    return Result<void>::Ok();
}

Result<std::vector<SpentOutput>> BlockchainDB::GetSpentOutputs(const uint256& block_hash) const {
    if (!impl_->is_open_) {
        return Result<std::vector<SpentOutput>>::Error("Database not open");
    }

    // Build database key: PREFIX_SPENT_OUTPUTS + block_hash
    std::string key;
    key.push_back(db::PREFIX_SPENT_OUTPUTS);
    key.append(reinterpret_cast<const char*>(block_hash.data()), block_hash.size());

    // Read from database
    rocksdb::ReadOptions read_options;
    std::string value_str;
    rocksdb::Status status = impl_->db_->Get(read_options, key, &value_str);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            // No spent outputs for this block (coinbase-only block or not found)
            return Result<std::vector<SpentOutput>>::Ok(std::vector<SpentOutput>());
        }
        return Result<std::vector<SpentOutput>>::Error("Failed to read spent outputs: " +
                                                       status.ToString());
    }

    // Deserialize vector of spent outputs
    std::vector<uint8_t> value_data(value_str.begin(), value_str.end());
    size_t pos = 0;

    // Deserialize count
    auto count_result = DeserializeUint64(value_data, pos);
    if (count_result.IsError()) {
        return Result<std::vector<SpentOutput>>::Error("Failed to deserialize count: " +
                                                       count_result.error);
    }
    uint64_t count = *count_result.value;

    // Deserialize each spent output
    std::vector<SpentOutput> spent_outputs;
    spent_outputs.reserve(count);

    for (uint64_t i = 0; i < count; i++) {
        std::vector<uint8_t> spent_data(value_data.begin() + pos, value_data.end());
        auto spent_result = SpentOutput::Deserialize(spent_data);
        if (spent_result.IsError()) {
            return Result<std::vector<SpentOutput>>::Error("Failed to deserialize spent output: " +
                                                           spent_result.error);
        }
        spent_outputs.push_back(*spent_result.value);
        pos += spent_result.value->Serialize().size();
    }

    return Result<std::vector<SpentOutput>>::Ok(std::move(spent_outputs));
}

Result<void> BlockchainDB::DeleteSpentOutputs(const uint256& block_hash) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Build database key: PREFIX_SPENT_OUTPUTS + block_hash
    std::string key;
    key.push_back(db::PREFIX_SPENT_OUTPUTS);
    key.append(reinterpret_cast<const char*>(block_hash.data()), block_hash.size());

    // Delete from database
    rocksdb::WriteOptions write_options;
    rocksdb::Status status = impl_->db_->Delete(write_options, key);

    if (!status.ok() && !status.IsNotFound()) {
        return Result<void>::Error("Failed to delete spent outputs: " +
                                  status.ToString());
    }

    return Result<void>::Ok();
}

// ============================================================================
// Chain State Operations
// ============================================================================

Result<void> BlockchainDB::StoreChainState(const ChainState& state) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    auto serialized = state.Serialize();
    std::string key = impl_->MakeKey(db::PREFIX_CHAINSTATE);
    rocksdb::Status status = impl_->Put(key, serialized);

    if (!status.ok()) {
        return Result<void>::Error("Failed to store chain state: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<ChainState> BlockchainDB::GetChainState() const {
    if (!impl_->is_open_) {
        return Result<ChainState>::Error("Database not open");
    }

    std::string key = impl_->MakeKey(db::PREFIX_CHAINSTATE);
    std::string value;
    rocksdb::Status status = impl_->Get(key, value);

    if (!status.ok()) {
        // Return empty chain state if not found (new database)
        ChainState state;
        state.best_height = 0;
        state.total_transactions = 0;
        state.utxo_count = 0;
        state.total_supply = 0;
        return Result<ChainState>::Ok(state);
    }

    std::vector<uint8_t> data(value.begin(), value.end());
    return ChainState::Deserialize(data);
}

Result<void> BlockchainDB::UpdateBestBlock(const uint256& hash, uint64_t height) {
    auto state_result = GetChainState();
    if (state_result.IsError()) {
        return Result<void>::Error("Failed to get chain state: " + state_result.error);
    }

    ChainState state = *state_result.value;
    state.best_block_hash = hash;
    state.best_height = height;

    return StoreChainState(state);
}

// ============================================================================
// Address Index Operations
// ============================================================================

// Helper: Extract address from script
static std::string ExtractAddressFromScript(const Script& script) {
    // Try P2PKH (most common)
    if (script.IsP2PKH()) {
        auto hash_opt = script.GetP2PKHHash();
        if (hash_opt.has_value()) {
            // Encode pubkey hash as Bech32 address
            auto result = AddressEncoder::EncodeAddress(hash_opt.value());
            if (result.IsOk()) {
                return result.value.value();
            }
        }
    }

    // Try P2PK
    if (script.IsP2PK()) {
        auto pubkey_opt = script.GetP2PKPublicKey();
        if (pubkey_opt.has_value()) {
            // Hash the public key and encode as Bech32 address
            const PublicKey& pubkey = pubkey_opt.value();
            std::vector<uint8_t> pubkey_vec(pubkey.begin(), pubkey.end());
            uint256 pubkey_hash = SHA3::Hash(pubkey_vec);
            auto result = AddressEncoder::EncodeAddress(pubkey_hash);
            if (result.IsOk()) {
                return result.value.value();
            }
        }
    }

    // Unknown script type - return empty string
    return "";
}

// Helper: Serialize vector of uint256 hashes
static std::vector<uint8_t> SerializeTxHashVector(const std::vector<uint256>& hashes) {
    std::vector<uint8_t> result;

    // Serialize count (8 bytes)
    SerializeUint64(result, static_cast<uint64_t>(hashes.size()));

    // Serialize each hash (32 bytes each)
    for (const uint256& hash : hashes) {
        SerializeUint256(result, hash);
    }

    return result;
}

// Helper: Deserialize vector of uint256 hashes
static Result<std::vector<uint256>> DeserializeTxHashVector(
    const std::vector<uint8_t>& data) {
    size_t pos = 0;
    std::vector<uint256> hashes;

    // Deserialize count
    auto count_result = DeserializeUint64(data, pos);
    if (count_result.IsError()) {
        return Result<std::vector<uint256>>::Error("Failed to deserialize count: " +
                                                   count_result.error);
    }
    uint64_t count = *count_result.value;

    // Deserialize each hash
    hashes.reserve(count);
    for (uint64_t i = 0; i < count; i++) {
        auto hash_result = DeserializeUint256(data, pos);
        if (hash_result.IsError()) {
            return Result<std::vector<uint256>>::Error("Failed to deserialize hash: " +
                                                       hash_result.error);
        }
        hashes.push_back(*hash_result.value);
    }

    return Result<std::vector<uint256>>::Ok(std::move(hashes));
}

Result<void> BlockchainDB::IndexTransaction(const Transaction& tx) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Get transaction hash
    uint256 tx_hash = tx.GetHash();

    // Extract addresses from all outputs
    for (const TxOut& output : tx.outputs) {
        std::string address = ExtractAddressFromScript(output.script_pubkey);
        if (address.empty()) {
            continue;  // Skip outputs with unknown script types
        }

        // Build database key: PREFIX_ADDRESS_INDEX + address
        std::string key;
        key.push_back(db::PREFIX_ADDRESS_INDEX);
        key.append(address);

        // Read existing transaction hashes for this address
        std::vector<uint256> tx_hashes;
        rocksdb::ReadOptions read_options;
        std::string value_str;
        rocksdb::Status status = impl_->db_->Get(read_options, key, &value_str);

        if (status.ok()) {
            // Deserialize existing hashes
            std::vector<uint8_t> value_data(value_str.begin(), value_str.end());
            auto result = DeserializeTxHashVector(value_data);
            if (result.IsOk()) {
                tx_hashes = std::move(*result.value);
            }
        }
        // If key doesn't exist, tx_hashes remains empty (new address)

        // Check if transaction already indexed (prevent duplicates)
        bool already_indexed = false;
        for (const uint256& existing_hash : tx_hashes) {
            if (existing_hash == tx_hash) {
                already_indexed = true;
                break;
            }
        }

        if (!already_indexed) {
            // Add new transaction hash
            tx_hashes.push_back(tx_hash);

            // Serialize updated list
            std::vector<uint8_t> value_data = SerializeTxHashVector(tx_hashes);

            // Write back to database
            rocksdb::WriteOptions write_options;
            std::string value_str_new(value_data.begin(), value_data.end());
            status = impl_->db_->Put(write_options, key, value_str_new);
            if (!status.ok()) {
                return Result<void>::Error("Failed to update address index: " +
                                          status.ToString());
            }
        }
    }

    return Result<void>::Ok();
}

Result<std::vector<uint256>> BlockchainDB::GetTransactionsForAddress(
    const std::string& address) const {
    if (!impl_->is_open_) {
        return Result<std::vector<uint256>>::Error("Database not open");
    }

    // Build database key: PREFIX_ADDRESS_INDEX + address
    std::string key;
    key.push_back(db::PREFIX_ADDRESS_INDEX);
    key.append(address);

    // Read transaction hashes
    rocksdb::ReadOptions read_options;
    std::string value_str;
    rocksdb::Status status = impl_->db_->Get(read_options, key, &value_str);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            // Address has no transactions - return empty vector
            return Result<std::vector<uint256>>::Ok(std::vector<uint256>());
        }
        return Result<std::vector<uint256>>::Error("Failed to read address index: " +
                                                   status.ToString());
    }

    // Deserialize transaction hashes
    std::vector<uint8_t> value_data(value_str.begin(), value_str.end());
    auto result = DeserializeTxHashVector(value_data);
    if (result.IsError()) {
        return Result<std::vector<uint256>>::Error("Failed to deserialize tx hashes: " +
                                                   result.error);
    }

    return Result<std::vector<uint256>>::Ok(std::move(*result.value));
}

// ============================================================================
// Transaction-to-Block Mapping
// ============================================================================

Result<void> BlockchainDB::IndexTransactionBlock(const uint256& tx_hash,
                                                 const uint256& block_hash) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Build database key: PREFIX_TX_BLOCK + tx_hash
    std::string key;
    key.push_back(db::PREFIX_TX_BLOCK);
    key.append(reinterpret_cast<const char*>(tx_hash.data()), tx_hash.size());

    // Serialize block hash as value
    std::vector<uint8_t> value_data;
    SerializeUint256(value_data, block_hash);

    // Write to database
    rocksdb::WriteOptions write_options;
    std::string value_str(value_data.begin(), value_data.end());
    rocksdb::Status status = impl_->db_->Put(write_options, key, value_str);

    if (!status.ok()) {
        return Result<void>::Error("Failed to index transaction block: " +
                                  status.ToString());
    }

    return Result<void>::Ok();
}

Result<uint256> BlockchainDB::GetBlockHashForTransaction(const uint256& tx_hash) const {
    if (!impl_->is_open_) {
        return Result<uint256>::Error("Database not open");
    }

    // Build database key: PREFIX_TX_BLOCK + tx_hash
    std::string key;
    key.push_back(db::PREFIX_TX_BLOCK);
    key.append(reinterpret_cast<const char*>(tx_hash.data()), tx_hash.size());

    // Read from database
    rocksdb::ReadOptions read_options;
    std::string value_str;
    rocksdb::Status status = impl_->db_->Get(read_options, key, &value_str);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            return Result<uint256>::Error("Transaction block mapping not found");
        }
        return Result<uint256>::Error("Failed to read transaction block mapping: " +
                                     status.ToString());
    }

    // Deserialize block hash
    std::vector<uint8_t> value_data(value_str.begin(), value_str.end());
    size_t pos = 0;
    auto result = DeserializeUint256(value_data, pos);
    if (result.IsError()) {
        return Result<uint256>::Error("Failed to deserialize block hash: " +
                                     result.error);
    }

    return Result<uint256>::Ok(*result.value);
}

// ============================================================================
// Batch Operations
// ============================================================================

void BlockchainDB::BeginBatch() {
    if (impl_->batch_) {
        delete impl_->batch_;
    }
    impl_->batch_ = new rocksdb::WriteBatch();
    impl_->in_batch_ = true;
}

Result<void> BlockchainDB::CommitBatch() {
    if (!impl_->in_batch_ || !impl_->batch_) {
        return Result<void>::Error("No active batch");
    }

    rocksdb::WriteOptions options;
    rocksdb::Status status = impl_->db_->Write(options, impl_->batch_);

    delete impl_->batch_;
    impl_->batch_ = nullptr;
    impl_->in_batch_ = false;

    if (!status.ok()) {
        return Result<void>::Error("Failed to commit batch: " + status.ToString());
    }

    return Result<void>::Ok();
}

void BlockchainDB::AbortBatch() {
    if (impl_->batch_) {
        delete impl_->batch_;
        impl_->batch_ = nullptr;
    }
    impl_->in_batch_ = false;
}

// ============================================================================
// Pruning
// ============================================================================

void BlockchainDB::EnablePruning(uint64_t target_size_gb) {
    impl_->pruning_enabled_ = true;
    impl_->pruning_target_size_ = target_size_gb * 1024 * 1024 * 1024;
}

Result<void> BlockchainDB::PruneBlocks(uint64_t keep_blocks) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    if (!impl_->pruning_enabled_) {
        return Result<void>::Error("Pruning not enabled");
    }

    // Get current best height
    auto chain_state_result = GetChainState();
    if (chain_state_result.IsError()) {
        return Result<void>::Error("Failed to get chain state: " +
                                  chain_state_result.error);
    }
    uint64_t best_height = chain_state_result.value->best_height;

    // Calculate prune height (don't prune the most recent keep_blocks)
    if (best_height <= keep_blocks) {
        // Chain too short to prune
        return Result<void>::Ok();
    }
    uint64_t prune_height = best_height - keep_blocks;

    // Prune blocks from height 1 up to prune_height
    // Note: We keep genesis block (height 0) forever
    BeginBatch();

    uint64_t blocks_pruned = 0;
    for (uint64_t height = 1; height <= prune_height; height++) {
        // Get block hash for this height
        auto hash_result = GetBlockHash(height);
        if (hash_result.IsError()) {
            continue;  // Skip if block hash not found
        }
        uint256 block_hash = *hash_result.value;

        // Delete block data (but keep block index for headers)
        auto delete_result = DeleteBlock(block_hash);
        if (delete_result.IsError()) {
            AbortBatch();
            return Result<void>::Error("Failed to delete block at height " +
                                      std::to_string(height) + ": " +
                                      delete_result.error);
        }

        // Delete spent outputs for this block (no longer needed for pruned blocks)
        auto delete_spent_result = DeleteSpentOutputs(block_hash);
        if (delete_spent_result.IsError()) {
            // Non-fatal error (spent outputs may not exist for coinbase-only blocks)
            // Continue pruning
        }

        // Delete transactions in this block (they're in the pruned block)
        // Note: We keep the block index for SPV/headers verification
        // UTXO set is maintained separately and not pruned

        blocks_pruned++;
    }

    auto commit_result = CommitBatch();
    if (commit_result.IsError()) {
        return Result<void>::Error("Failed to commit pruning batch: " +
                                  commit_result.error);
    }

    // Log pruning statistics
    LogF(LogLevel::INFO, "Pruned %llu blocks (kept last %llu blocks)",
         blocks_pruned, keep_blocks);

    return Result<void>::Ok();
}

bool BlockchainDB::IsPruningEnabled() const {
    return impl_->pruning_enabled_;
}

// ============================================================================
// Database Stats
// ============================================================================

uint64_t BlockchainDB::GetDatabaseSize() const {
    if (!impl_->is_open_ || !impl_->db_) {
        return 0;
    }

    // Get total SST file size from RocksDB
    uint64_t total_size = 0;
    std::string value;

    // Get SST files size (actual data size on disk)
    if (impl_->db_->GetProperty("rocksdb.total-sst-files-size", &value)) {
        try {
            total_size = std::stoull(value);
        } catch (...) {
            // Failed to parse, use estimate
        }
    }

    // If property not available, try approximate sizes
    if (total_size == 0) {
        if (impl_->db_->GetProperty("rocksdb.estimate-live-data-size", &value)) {
            try {
                total_size = std::stoull(value);
            } catch (...) {
                // Failed to parse
            }
        }
    }

    return total_size;
}

uint64_t BlockchainDB::GetBlockCount() const {
    auto state_result = GetChainState();
    if (state_result.IsError()) {
        return 0;
    }
    return state_result.value->best_height + 1;
}

uint64_t BlockchainDB::GetTransactionCount() const {
    auto state_result = GetChainState();
    if (state_result.IsError()) {
        return 0;
    }
    return state_result.value->total_transactions;
}

uint64_t BlockchainDB::GetUTXOCount() const {
    auto state_result = GetChainState();
    if (state_result.IsError()) {
        return 0;
    }
    return state_result.value->utxo_count;
}

// ============================================================================
// Maintenance
// ============================================================================

Result<void> BlockchainDB::Compact() {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    rocksdb::CompactRangeOptions options;
    rocksdb::Status status = impl_->db_->CompactRange(options, nullptr, nullptr);

    if (!status.ok()) {
        return Result<void>::Error("Failed to compact database: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<void> BlockchainDB::Verify() {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    // Step 1: Verify chain state exists
    auto chain_state_result = GetChainState();
    if (chain_state_result.IsError()) {
        return Result<void>::Error("Chain state verification failed: " +
                                  chain_state_result.error);
    }
    const ChainState& state = *chain_state_result.value;

    // Step 2: Verify genesis block
    auto genesis_result = GetBlockByHeight(0);
    if (genesis_result.IsError()) {
        return Result<void>::Error("Genesis block not found");
    }

    // Step 3: Verify block height mappings
    for (uint64_t height = 0; height <= state.best_height; height++) {
        auto hash_result = GetBlockHash(height);
        if (hash_result.IsError()) {
            return Result<void>::Error("Block hash not found for height " +
                                      std::to_string(height));
        }

        // Verify block index exists
        auto index_result = GetBlockIndex(*hash_result.value);
        if (index_result.IsError()) {
            return Result<void>::Error("Block index not found for height " +
                                      std::to_string(height));
        }

        // Verify height matches
        if (index_result.value->height != height) {
            return Result<void>::Error("Height mismatch at height " +
                                      std::to_string(height) +
                                      " (expected " + std::to_string(height) +
                                      ", got " + std::to_string(index_result.value->height) + ")");
        }
    }

    // Step 4: Verify UTXO count (sample check)
    auto utxos_result = GetAllUTXOs(1000);  // Sample first 1000
    if (utxos_result.IsError()) {
        return Result<void>::Error("UTXO verification failed: " +
                                  utxos_result.error);
    }

    // Step 5: Run RocksDB internal verification
    rocksdb::Status status = impl_->db_->VerifyChecksum();
    if (!status.ok()) {
        return Result<void>::Error("RocksDB checksum verification failed: " +
                                  status.ToString());
    }

    LogF(LogLevel::INFO, "Database verification passed: %llu blocks, %llu transactions, %llu UTXOs",
         state.best_height + 1, state.total_transactions, state.utxo_count);

    return Result<void>::Ok();
}

Result<void> BlockchainDB::Backup(const std::string& backup_dir) {
    if (!impl_->is_open_) {
        return Result<void>::Error("Database not open");
    }

    if (backup_dir.empty()) {
        return Result<void>::Error("Backup directory path is empty");
    }

    // Create backup engine
    rocksdb::BackupEngine* backup_engine;
    rocksdb::BackupEngineOptions backup_options(backup_dir);
    backup_options.share_table_files = true;  // Deduplicate SST files
    backup_options.sync = true;               // Ensure data is flushed to disk

    rocksdb::Status status = rocksdb::BackupEngine::Open(
        rocksdb::Env::Default(),
        backup_options,
        &backup_engine
    );

    if (!status.ok()) {
        return Result<void>::Error("Failed to open backup engine: " +
                                  status.ToString());
    }

    // Create new backup
    status = backup_engine->CreateNewBackup(impl_->db_);

    if (!status.ok()) {
        delete backup_engine;
        return Result<void>::Error("Failed to create backup: " +
                                  status.ToString());
    }

    // Purge old backups (keep last 5)
    status = backup_engine->PurgeOldBackups(5);
    if (!status.ok()) {
        LogF(LogLevel::WARNING, "Failed to purge old backups: %s",
             status.ToString().c_str());
        // Non-fatal error, continue
    }

    // Get backup info
    std::vector<rocksdb::BackupInfo> backup_info;
    backup_engine->GetBackupInfo(&backup_info);

    delete backup_engine;

    LogF(LogLevel::INFO, "Database backup created successfully at %s (%zu backups total)",
         backup_dir.c_str(), backup_info.size());

    return Result<void>::Ok();
}

// ============================================================================
// Mempool Implementation
// ============================================================================

struct MempoolEntry {
    Transaction tx;
    uint256 tx_hash;
    uint64_t fee;
    double fee_rate;
    std::chrono::system_clock::time_point time_added;
    size_t size;

    MempoolEntry() : fee(0), fee_rate(0.0), size(0) {}

    MempoolEntry(Transaction transaction, uint64_t tx_fee)
        : tx(std::move(transaction))
        , fee(tx_fee)
        , time_added(std::chrono::system_clock::now())
    {
        tx_hash = tx.GetHash();
        size = tx.GetSerializedSize();
        fee_rate = static_cast<double>(fee) / static_cast<double>(size);
    }

    bool operator<(const MempoolEntry& other) const {
        // Higher fee rate = higher priority
        return fee_rate > other.fee_rate;
    }
};

class Mempool::Impl {
public:
    std::unordered_map<uint256, MempoolEntry, uint256_hash> transactions;
    std::unordered_map<OutPoint, uint256, OutPointHash> outpoint_to_tx;
    mutable std::mutex mutex;
    size_t total_size = 0;

    static constexpr size_t MAX_MEMPOOL_SIZE = 100 * 1024 * 1024; // 100 MB
};

Mempool::Mempool() : impl_(std::make_unique<Impl>()) {}

Mempool::~Mempool() = default;

Result<void> Mempool::AddTransaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto tx_hash = tx.GetHash();

    // Check if already in mempool
    if (impl_->transactions.find(tx_hash) != impl_->transactions.end()) {
        return Result<void>::Error("Transaction already in mempool");
    }

    // Basic validation
    if (tx.inputs.empty()) {
        return Result<void>::Error("Transaction has no inputs");
    }
    if (tx.outputs.empty()) {
        return Result<void>::Error("Transaction has no outputs");
    }

    // Check for conflicts
    for (const auto& input : tx.inputs) {
        OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
        if (impl_->outpoint_to_tx.find(outpoint) != impl_->outpoint_to_tx.end()) {
            return Result<void>::Error("Transaction conflicts with mempool");
        }
    }

    // Fee estimation placeholder - actual fee should be calculated by the transaction
    // validator/creator before adding to mempool (requires UTXO lookups)
    // For now, use a default fee estimate
    uint64_t fee = 1000; // Default fee estimate in INTS

    // Add to mempool
    MempoolEntry entry(tx, fee);
    impl_->total_size += entry.size;
    impl_->transactions[tx_hash] = entry;

    // Track outpoints
    for (const auto& input : tx.inputs) {
        OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
        impl_->outpoint_to_tx[outpoint] = tx_hash;
    }

    return Result<void>::Ok();
}

void Mempool::RemoveTransaction(const uint256& tx_hash) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->transactions.find(tx_hash);
    if (it == impl_->transactions.end()) {
        return;
    }

    impl_->total_size -= it->second.size;

    for (const auto& input : it->second.tx.inputs) {
        OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
        impl_->outpoint_to_tx.erase(outpoint);
    }

    impl_->transactions.erase(it);
}

std::optional<Transaction> Mempool::GetTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->transactions.find(tx_hash);
    if (it == impl_->transactions.end()) {
        return std::nullopt;
    }

    return it->second.tx;
}

bool Mempool::HasTransaction(const uint256& tx_hash) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->transactions.find(tx_hash) != impl_->transactions.end();
}

std::vector<Transaction> Mempool::GetAllTransactions() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<Transaction> result;
    result.reserve(impl_->transactions.size());

    for (const auto& [hash, entry] : impl_->transactions) {
        result.push_back(entry.tx);
    }

    return result;
}

std::vector<Transaction> Mempool::GetTransactionsForMining(size_t max_count) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    std::vector<MempoolEntry> sorted_entries;
    sorted_entries.reserve(impl_->transactions.size());

    for (const auto& [hash, entry] : impl_->transactions) {
        sorted_entries.push_back(entry);
    }

    // Sort by fee rate (highest first)
    std::sort(sorted_entries.begin(), sorted_entries.end());

    // Extract transactions
    std::vector<Transaction> result;
    size_t count = (max_count > 0) ? std::min(max_count, sorted_entries.size())
                                   : sorted_entries.size();
    result.reserve(count);

    for (size_t i = 0; i < count; i++) {
        result.push_back(sorted_entries[i].tx);
    }

    return result;
}

void Mempool::RemoveBlockTransactions(const Block& block) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    for (const auto& tx : block.transactions) {
        auto tx_hash = tx.GetHash();
        auto it = impl_->transactions.find(tx_hash);
        if (it == impl_->transactions.end()) {
            continue;
        }

        impl_->total_size -= it->second.size;

        for (const auto& input : it->second.tx.inputs) {
            OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
            impl_->outpoint_to_tx.erase(outpoint);
        }

        impl_->transactions.erase(it);
    }
}

size_t Mempool::GetSize() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->transactions.size();
}

uint64_t Mempool::GetTotalFees() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    uint64_t total = 0;
    for (const auto& [hash, entry] : impl_->transactions) {
        total += entry.fee;
    }

    return total;
}

void Mempool::Clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->transactions.clear();
    impl_->outpoint_to_tx.clear();
    impl_->total_size = 0;
}

void Mempool::LimitSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->total_size <= max_size) {
        return;
    }

    // Get transactions sorted by fee rate (lowest first)
    std::vector<std::pair<uint256, MempoolEntry>> sorted_txs;
    sorted_txs.reserve(impl_->transactions.size());

    for (const auto& [hash, entry] : impl_->transactions) {
        sorted_txs.push_back({hash, entry});
    }

    std::sort(sorted_txs.begin(), sorted_txs.end(),
              [](const auto& a, const auto& b) {
                  return a.second.fee_rate < b.second.fee_rate;
              });

    // Remove lowest fee transactions until under limit
    for (const auto& [hash, entry] : sorted_txs) {
        if (impl_->total_size <= max_size) {
            break;
        }

        impl_->total_size -= entry.size;

        for (const auto& input : entry.tx.inputs) {
            OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
            impl_->outpoint_to_tx.erase(outpoint);
        }

        impl_->transactions.erase(hash);
    }
}

// ============================================================================
// UTXOSet Implementation
// ============================================================================

class UTXOSet::Impl {
public:
    std::shared_ptr<BlockchainDB> db;
    std::map<OutPoint, TxOut> cache;  // In-memory cache
    mutable std::mutex mutex;

    explicit Impl(std::shared_ptr<BlockchainDB> database) : db(std::move(database)) {}
};

UTXOSet::UTXOSet(std::shared_ptr<BlockchainDB> db)
    : impl_(std::make_unique<Impl>(std::move(db))) {}

UTXOSet::~UTXOSet() = default;

Result<void> UTXOSet::Load() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Get all UTXOs from database
    auto utxos_result = impl_->db->GetAllUTXOs();
    if (utxos_result.IsError()) {
        return Result<void>::Error("Failed to load UTXOs: " + utxos_result.error);
    }

    // Clear existing cache
    impl_->cache.clear();

    // Load UTXOs into cache
    const auto& utxos = *utxos_result.value;
    for (const auto& [outpoint, txout] : utxos) {
        impl_->cache[outpoint] = txout;
    }

    return Result<void>::Ok();
}

Result<void> UTXOSet::AddUTXO(const OutPoint& outpoint, const TxOut& output) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cache[outpoint] = output;
    return Result<void>::Ok();
}

Result<void> UTXOSet::SpendUTXO(const OutPoint& outpoint) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cache.erase(outpoint);
    return Result<void>::Ok();
}

std::optional<TxOut> UTXOSet::GetUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->cache.find(outpoint);
    if (it != impl_->cache.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool UTXOSet::HasUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->cache.find(outpoint) != impl_->cache.end();
}

uint64_t UTXOSet::GetTotalValue() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    uint64_t total = 0;
    for (const auto& [outpoint, txout] : impl_->cache) {
        total += txout.value;
    }
    return total;
}

size_t UTXOSet::GetCount() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->cache.size();
}

Result<void> UTXOSet::ApplyBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Spend inputs
    for (const auto& tx : block.transactions) {
        if (tx.IsCoinbase()) {
            continue;
        }
        for (const auto& input : tx.inputs) {
            OutPoint outpoint(input.prev_tx_hash, input.prev_tx_index);
            impl_->cache.erase(outpoint);
        }
    }

    // Add outputs
    for (const auto& tx : block.transactions) {
        uint256 tx_hash = tx.GetHash();
        for (uint32_t i = 0; i < tx.outputs.size(); i++) {
            OutPoint outpoint(tx_hash, i);
            impl_->cache[outpoint] = tx.outputs[i];
        }
    }

    return Result<void>::Ok();
}

Result<void> UTXOSet::RevertBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Step 1: Remove outputs that were created by this block
    for (const auto& tx : block.transactions) {
        uint256 tx_hash = tx.GetHash();

        for (uint32_t i = 0; i < tx.outputs.size(); i++) {
            OutPoint outpoint;
            outpoint.tx_hash = tx_hash;
            outpoint.index = i;

            // Remove from cache
            impl_->cache.erase(outpoint);

            // Remove from database
            auto delete_result = impl_->db->DeleteUTXO(outpoint);
            if (delete_result.IsError()) {
                return delete_result;
            }
        }
    }

    // Step 2: Restore outputs that were spent by this block
    uint256 block_hash = block.GetHash();
    auto spent_outputs_result = impl_->db->GetSpentOutputs(block_hash);

    if (spent_outputs_result.IsOk()) {
        const auto& spent_outputs = *spent_outputs_result.value;

        for (const auto& spent : spent_outputs) {
            // Restore to cache
            impl_->cache[spent.outpoint] = spent.output;

            // Restore to database
            auto store_result = impl_->db->StoreUTXO(spent.outpoint, spent.output);
            if (store_result.IsError()) {
                return store_result;
            }
        }

        // Clean up spent outputs record from database
        auto delete_spent_result = impl_->db->DeleteSpentOutputs(block_hash);
        if (delete_spent_result.IsError()) {
            return delete_spent_result;
        }
    }

    return Result<void>::Ok();
}

Result<void> UTXOSet::Flush() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->db) {
        return Result<void>::Error("Database not initialized");
    }

    // Begin batch write for atomic flush
    impl_->db->BeginBatch();

    size_t flushed_count = 0;

    // Write all cached UTXOs to database
    for (const auto& [outpoint, txout] : impl_->cache) {
        auto store_result = impl_->db->StoreUTXO(outpoint, txout);
        if (store_result.IsError()) {
            impl_->db->AbortBatch();
            return Result<void>::Error("Failed to flush UTXO: " +
                                      store_result.error);
        }
        flushed_count++;
    }

    // Commit batch
    auto commit_result = impl_->db->CommitBatch();
    if (commit_result.IsError()) {
        return Result<void>::Error("Failed to commit UTXO flush: " +
                                  commit_result.error);
    }

    LogF(LogLevel::INFO, "Flushed %zu UTXOs to database", flushed_count);

    return Result<void>::Ok();
}

std::vector<std::pair<OutPoint, TxOut>> UTXOSet::GetUTXOsForAddress(
    const std::string& address) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    std::vector<std::pair<OutPoint, TxOut>> result;

    // Decode address to get pubkey hash
    auto address_decode_result = AddressEncoder::DecodeAddress(address);
    if (address_decode_result.IsError()) {
        // Invalid address, return empty result
        return result;
    }
    const uint256& target_pubkey_hash = *address_decode_result.value;

    // Filter UTXOs by matching script_pubkey address
    for (const auto& [outpoint, txout] : impl_->cache) {
        // Check if this is a P2PKH script
        if (txout.script_pubkey.IsP2PKH()) {
            // Extract pubkey hash from script
            auto script_hash_opt = txout.script_pubkey.GetP2PKHHash();
            if (script_hash_opt.has_value()) {
                // Compare with target address hash
                if (*script_hash_opt == target_pubkey_hash) {
                    result.push_back({outpoint, txout});
                }
            }
        }
        // TODO: Add support for P2PK and other script types
    }

    return result;
}

} // namespace intcoin
