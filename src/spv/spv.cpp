// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/spv.h>
#include <intcoin/consensus.h>
#include <intcoin/crypto.h>
#include <intcoin/util.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace intcoin {

SPVClient::SPVClient(std::shared_ptr<BlockchainDB> db)
    : db_(db),
      best_height_(0),
      is_syncing_(false),
      sync_progress_(0.0),
      stop_sync_(false) {

    // Initialize bandwidth stats
    bandwidth_stats_ = {0, 0, 0, 0};

    // Load existing headers from database
    auto result = LoadHeaders();
    if (result.IsError()) {
        LogF(LogLevel::WARNING, "SPV: Failed to load headers: %s", result.error.c_str());
    }
}

SPVClient::~SPVClient() {
    StopSync();
}

Result<void> SPVClient::StartSync() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_syncing_) {
        return Result<void>::Error("SPV sync already in progress");
    }

    LogF(LogLevel::INFO, "SPV: Starting header sync from height %llu", best_height_);

    is_syncing_ = true;
    stop_sync_ = false;
    sync_progress_ = 0.0;

    // Start sync worker thread
    sync_thread_ = std::make_unique<std::thread>(&SPVClient::SyncWorker, this);

    return Result<void>::Ok();
}

void SPVClient::StopSync() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!is_syncing_) {
            return;
        }

        stop_sync_ = true;
        is_syncing_ = false;
    }

    // Wait for sync thread to finish
    if (sync_thread_ && sync_thread_->joinable()) {
        sync_thread_->join();
    }

    LogF(LogLevel::INFO, "SPV: Sync stopped at height %llu", best_height_);
}

bool SPVClient::IsSyncing() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_syncing_;
}

double SPVClient::GetSyncProgress() const {
    return sync_progress_.load();
}

uint64_t SPVClient::GetBestHeight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return best_height_;
}

uint256 SPVClient::GetBestHash() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return best_hash_;
}

Result<std::pair<std::vector<uint256>, size_t>> SPVClient::RequestMerkleProof(
    const uint256& tx_hash, const uint256& block_hash) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if we already have this proof cached
    auto it = merkle_proofs_.find(tx_hash);
    if (it != merkle_proofs_.end()) {
        std::vector<uint8_t> tx_hash_vec(tx_hash.begin(), tx_hash.end());
        LogF(LogLevel::DEBUG, "SPV: Using cached merkle proof for tx %s",
             BytesToHex(tx_hash_vec).c_str());
        return Result<std::pair<std::vector<uint256>, size_t>>::Ok(it->second);
    }

    // Check if header exists
    if (headers_.find(block_hash) == headers_.end()) {
        return Result<std::pair<std::vector<uint256>, size_t>>::Error(
            "Block header not found");
    }

    // Add to pending requests
    pending_proof_requests_[tx_hash] = block_hash;

    // TODO: Send GETMERKLEPROOF message to peer
    // For now, return error indicating proof must be requested from full node
    return Result<std::pair<std::vector<uint256>, size_t>>::Error(
        "Merkle proof request added to queue - not yet implemented");
}

bool SPVClient::VerifyTransaction(const uint256& tx_hash, const uint256& block_hash,
                                 const std::vector<uint256>& merkle_branch, size_t tx_index) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Get block header
    auto it = headers_.find(block_hash);
    if (it == headers_.end()) {
        LogF(LogLevel::WARNING, "SPV: Cannot verify tx - block header not found");
        return false;
    }

    const BlockHeader& header = it->second;

    // Verify merkle proof using existing function from block.cpp
    bool valid = VerifyMerkleProof(tx_hash, header.merkle_root, merkle_branch, tx_index);

    if (valid) {
        std::vector<uint8_t> tx_hash_vec(tx_hash.begin(), tx_hash.end());
        std::vector<uint8_t> block_hash_vec(block_hash.begin(), block_hash.end());
        LogF(LogLevel::INFO, "SPV: Verified transaction %s in block %s",
             BytesToHex(tx_hash_vec).substr(0, 16).c_str(),
             BytesToHex(block_hash_vec).substr(0, 16).c_str());

        // Cache the proof
        merkle_proofs_[tx_hash] = {merkle_branch, tx_index};

        // Update bandwidth stats (estimate)
        bandwidth_stats_.proofs_downloaded += merkle_branch.size() * 32 + 8;
    } else {
        std::vector<uint8_t> tx_hash_vec(tx_hash.begin(), tx_hash.end());
        LogF(LogLevel::WARNING, "SPV: Invalid merkle proof for tx %s",
             BytesToHex(tx_hash_vec).substr(0, 16).c_str());
    }

    return valid;
}

void SPVClient::AddWatchAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    watch_addresses_.insert(address);
    LogF(LogLevel::INFO, "SPV: Added watch address %s (total: %zu)",
         address.c_str(), watch_addresses_.size());
}

void SPVClient::RemoveWatchAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    watch_addresses_.erase(address);
    LogF(LogLevel::INFO, "SPV: Removed watch address %s (total: %zu)",
         address.c_str(), watch_addresses_.size());
}

std::set<std::string> SPVClient::GetWatchAddresses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watch_addresses_;
}

void SPVClient::SetBloomFilter(const BloomFilter& filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    bloom_filter_ = std::make_unique<BloomFilter>(filter);

    LogF(LogLevel::INFO, "SPV: Set bloom filter (size: %zu bytes, hash funcs: %u)",
         filter.GetSize(), filter.GetNumHashFuncs());

    // TODO: Send FILTERLOAD message to peers
}

Result<BloomFilter> SPVClient::GetBloomFilter() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!bloom_filter_) {
        return Result<BloomFilter>::Error("No bloom filter set");
    }

    return Result<BloomFilter>::Ok(*bloom_filter_);
}

void SPVClient::ClearBloomFilter() {
    std::lock_guard<std::mutex> lock(mutex_);
    bloom_filter_.reset();

    LogF(LogLevel::INFO, "SPV: Cleared bloom filter");

    // TODO: Send FILTERCLEAR message to peers
}

bool SPVClient::HasBloomFilter() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bloom_filter_ != nullptr;
}

Result<BlockHeader> SPVClient::GetHeader(const uint256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = headers_.find(hash);
    if (it == headers_.end()) {
        return Result<BlockHeader>::Error("Header not found");
    }

    return Result<BlockHeader>::Ok(it->second);
}

Result<BlockHeader> SPVClient::GetHeaderByHeight(uint64_t height) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = height_index_.find(height);
    if (it == height_index_.end()) {
        return Result<BlockHeader>::Error("No header at height");
    }

    return GetHeader(it->second);
}

std::vector<BlockHeader> SPVClient::GetHeadersInRange(uint64_t start_height, uint64_t end_height) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BlockHeader> result;
    result.reserve(end_height - start_height + 1);

    for (uint64_t h = start_height; h <= end_height && h <= best_height_; ++h) {
        auto it = height_index_.find(h);
        if (it != height_index_.end()) {
            auto header_it = headers_.find(it->second);
            if (header_it != headers_.end()) {
                result.push_back(header_it->second);
            }
        }
    }

    return result;
}

bool SPVClient::HasHeader(const uint256& hash) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return headers_.find(hash) != headers_.end();
}

uint64_t SPVClient::GetHeaderCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return headers_.size();
}

SPVClient::BandwidthStats SPVClient::GetBandwidthStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bandwidth_stats_;
}

void SPVClient::HandleHeaders(const std::vector<BlockHeader>& headers) {
    if (headers.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    LogF(LogLevel::DEBUG, "SPV: Received %zu headers", headers.size());

    // Validate header chain
    auto validate_result = ValidateHeaderChain(headers);
    if (validate_result.IsError()) {
        LogF(LogLevel::WARNING, "SPV: Header validation failed: %s",
             validate_result.error.c_str());
        return;
    }

    // Store headers
    for (const auto& header : headers) {
        auto store_result = StoreHeader(header);
        if (store_result.IsError()) {
            LogF(LogLevel::WARNING, "SPV: Failed to store header: %s",
                 store_result.error.c_str());
            continue;
        }
    }

    // Update bandwidth stats (BlockHeader is approximately 152 bytes)
    constexpr size_t HEADER_SIZE = 152;
    bandwidth_stats_.headers_downloaded += headers.size() * HEADER_SIZE;
    bandwidth_stats_.total_received += headers.size() * HEADER_SIZE;

    LogF(LogLevel::INFO, "SPV: Synced to height %llu (downloaded %llu KB headers)",
         best_height_, bandwidth_stats_.headers_downloaded / 1024);
}

void SPVClient::HandleMerkleBlock(const uint256& block_hash,
                                  const std::vector<uint256>& tx_hashes,
                                  const std::vector<uint256>& merkle_branch) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find the header
    auto it = headers_.find(block_hash);
    if (it == headers_.end()) {
        LogF(LogLevel::WARNING, "SPV: Received merkle block for unknown header");
        return;
    }

    // TODO: Process merkle block for watched transactions
    LogF(LogLevel::DEBUG, "SPV: Received merkle block with %zu transactions",
         tx_hashes.size());
}

void SPVClient::RequestHeaders() {
    // Build GETHEADERS message starting from our best hash
    // TODO: Implement when network protocol supports GETHEADERS

    LogF(LogLevel::DEBUG, "SPV: Requesting headers from height %llu", best_height_);

    // For now, this is a placeholder
    // Real implementation will send GETHEADERS message to peers
}

Result<void> SPVClient::ValidateHeaderChain(const std::vector<BlockHeader>& headers) {
    if (headers.empty()) {
        return Result<void>::Ok();
    }

    // Check first header connects to our chain
    if (best_height_ > 0) {
        const auto& first = headers[0];
        if (first.prev_block_hash != best_hash_) {
            return Result<void>::Error("First header does not connect to our chain");
        }
    }

    // Validate each header
    for (size_t i = 0; i < headers.size(); ++i) {
        const auto& header = headers[i];

        // Basic validation: check PoW hash is below target
        // Full validation requires blockchain context (difficulty adjustments, etc.)
        // For SPV, we trust the longest chain with most work
        uint256 hash = header.GetHash();
        uint256 target = DifficultyCalculator::CompactToTarget(header.bits);

        if (hash > target) {
            return Result<void>::Error("Invalid PoW at index " + std::to_string(i));
        }

        // Check timestamp (not too far in future)
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::from_time_t(header.timestamp);
        auto max_future = now + std::chrono::hours(2);

        if (timestamp > max_future) {
            return Result<void>::Error("Block timestamp too far in future");
        }

        // Check chain linkage (except for first)
        if (i > 0) {
            uint256 prev_hash = headers[i - 1].GetHash();
            if (header.prev_block_hash != prev_hash) {
                return Result<void>::Error("Headers not properly linked");
            }
        }
    }

    return Result<void>::Ok();
}

Result<void> SPVClient::StoreHeader(const BlockHeader& header) {
    uint256 hash = header.GetHash();

    // Add to memory maps
    headers_[hash] = header;

    // Determine height
    uint64_t height;
    if (header.prev_block_hash == uint256{}) {
        // Genesis block
        height = 0;
    } else {
        // Find parent height
        auto parent_it = headers_.find(header.prev_block_hash);
        if (parent_it == headers_.end()) {
            return Result<void>::Error("Parent header not found");
        }

        // Find parent height from index
        uint64_t parent_height = 0;
        for (const auto& [h, hash_at_h] : height_index_) {
            if (hash_at_h == header.prev_block_hash) {
                parent_height = h;
                break;
            }
        }

        height = parent_height + 1;
    }

    height_index_[height] = hash;

    // Update best if this is the new tip
    if (height > best_height_) {
        best_height_ = height;
        best_hash_ = hash;

        // TODO: Persist to database
        // Requires extending BlockchainDB with raw KV methods or custom SPV database
    }

    // TODO: Persist header and height index to database
    // For now, keep everything in memory

    return Result<void>::Ok();
}

Result<void> SPVClient::LoadHeaders() {
    // TODO: Implement header loading from BlockchainDB
    // For now, start with empty header set
    // Future: Add custom SPV database or extend BlockchainDB with raw KV methods

    LogF(LogLevel::INFO, "SPV: Starting with empty header set (database loading not yet implemented)");
    best_height_ = 0;
    best_hash_ = uint256{};

    return Result<void>::Ok();
}

void SPVClient::SyncWorker() {
    LogF(LogLevel::INFO, "SPV: Sync worker started");

    while (!stop_sync_.load()) {
        // Request headers from peers
        RequestHeaders();

        // Update sync progress
        // TODO: Determine network tip height from peers
        // For now, assume we're synced if we haven't received headers in a while

        // Sleep for a bit before next request
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    LogF(LogLevel::INFO, "SPV: Sync worker stopped");
}

}  // namespace intcoin
