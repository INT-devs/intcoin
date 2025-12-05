/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Blockchain Synchronization Manager Implementation
 */

#include "../../include/intcoin/sync.h"
#include "../../include/intcoin/util.h"
#include <algorithm>
#include <thread>
#include <iostream>

namespace intcoin {

// ============================================================================
// BlockchainSyncManager::Impl
// ============================================================================

class BlockchainSyncManager::Impl {
public:
    Impl(std::shared_ptr<Blockchain> blockchain, std::shared_ptr<P2PNode> p2p)
        : blockchain_(blockchain)
        , p2p_(p2p)
        , state_(SyncState::NOT_STARTED)
        , is_syncing_(false)
        , is_paused_(false)
        , current_height_(0)
        , target_height_(0)
        , sync_start_time_(std::chrono::system_clock::now())
        , headers_manager_(blockchain)
        , config_{
            .max_blocks_in_flight = 128,
            .max_blocks_per_peer = 16,
            .block_timeout = std::chrono::seconds(60),
            .stall_timeout = std::chrono::seconds(120),
            .max_retries = 3,
            .headers_first = true,
            .header_batch_size = 2000
        }
    {
        stats_.total_headers_downloaded = 0;
        stats_.total_blocks_downloaded = 0;
        stats_.total_blocks_validated = 0;
        stats_.total_bytes_downloaded = 0;
        stats_.total_bytes_validated = 0;
        stats_.total_sync_time = std::chrono::milliseconds(0);
        stats_.average_block_download_time = std::chrono::milliseconds(0);
        stats_.average_block_validation_time = std::chrono::milliseconds(0);
        stats_.failed_downloads = 0;
        stats_.retries = 0;
    }

    Result<void> StartSync() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (is_syncing_) {
            return Err("Sync already in progress");
        }

        is_syncing_ = true;
        is_paused_ = false;
        state_ = SyncState::CONNECTING;
        sync_start_time_ = std::chrono::system_clock::now();

        // Start sync thread
        sync_thread_ = std::thread([this]() { SyncLoop(); });

        return Ok();
    }

    void StopSync() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_syncing_ = false;
            is_paused_ = false;
        }

        if (sync_thread_.joinable()) {
            sync_thread_.join();
        }
    }

    void PauseSync() {
        std::lock_guard<std::mutex> lock(mutex_);
        is_paused_ = true;
    }

    void ResumeSync() {
        std::lock_guard<std::mutex> lock(mutex_);
        is_paused_ = false;
    }

    bool IsSyncing() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_syncing_;
    }

    bool IsSynced() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == SyncState::SYNCED;
    }

    SyncProgress GetProgress() const {
        std::lock_guard<std::mutex> lock(mutex_);

        SyncProgress progress;
        progress.state = state_;
        progress.current_height = current_height_;
        progress.target_height = target_height_;
        progress.headers_count = headers_manager_.GetBestHeaderHeight();
        progress.blocks_downloaded = stats_.total_blocks_downloaded;
        progress.blocks_validated = stats_.total_blocks_validated;
        progress.progress_percent = CalculateSyncProgress(current_height_, target_height_);
        progress.download_speed_bps = CalculateDownloadSpeed();
        progress.estimated_time_remaining = EstimateTimeRemaining();
        progress.connected_peers = p2p_->GetPeerCount();
        progress.sync_peers = GetSyncPeers().size();

        return progress;
    }

    SyncStatistics GetStatistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    SyncState GetState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    std::chrono::seconds EstimateTimeRemaining() const {
        if (current_height_ >= target_height_) {
            return std::chrono::seconds(0);
        }

        uint64_t blocks_remaining = target_height_ - current_height_;
        double speed_bps = CalculateDownloadSpeed();

        if (speed_bps <= 0.0) {
            return std::chrono::seconds(0);
        }

        // Estimate average block size (100 KB)
        const size_t avg_block_size = 100 * 1024;
        return EstimateSyncTime(blocks_remaining, speed_bps, avg_block_size);
    }

    Result<void> RequestHeaders(uint64_t peer_id, const uint256& start_hash) {
        // Create GETHEADERS message
        // This would be sent via P2P network
        // For now, return success
        return Ok();
    }

    Result<void> ProcessHeaders(uint64_t peer_id, const std::vector<BlockHeader>& headers) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (headers.empty()) {
            return Ok();
        }

        // Add headers to headers manager
        auto result = headers_manager_.AddHeaders(headers);
        if (!result.IsOk()) {
            return result;
        }

        stats_.total_headers_downloaded += headers.size();

        // Update target height
        uint64_t best_header_height = headers_manager_.GetBestHeaderHeight();
        if (best_header_height > target_height_) {
            target_height_ = best_header_height;
        }

        // Queue blocks for download
        for (const auto& header : headers) {
            uint256 block_hash = header.GetHash();
            uint64_t height = header.height;
            download_manager_.AddBlock(block_hash, height);
        }

        return Ok();
    }

    uint64_t GetBestHeaderHeight() const {
        return headers_manager_.GetBestHeaderHeight();
    }

    uint256 GetBestHeaderHash() const {
        auto best_header = headers_manager_.GetBestHeader();
        if (best_header.has_value()) {
            return best_header->GetHash();
        }
        return uint256{};
    }

    Result<void> RequestBlock(const uint256& block_hash, uint64_t peer_id) {
        download_manager_.MarkRequested(block_hash, peer_id);
        // Send GETDATA message via P2P
        return Ok();
    }

    Result<void> RequestBlocks(const std::vector<uint256>& block_hashes, uint64_t peer_id) {
        for (const auto& hash : block_hashes) {
            download_manager_.MarkRequested(hash, peer_id);
        }
        // Send GETDATA messages via P2P
        return Ok();
    }

    Result<void> ProcessBlock(uint64_t peer_id, const Block& block) {
        std::lock_guard<std::mutex> lock(mutex_);

        uint256 block_hash = block.GetHash();

        // Mark as received
        download_manager_.MarkReceived(block_hash);

        // Validate block
        auto validate_start = std::chrono::steady_clock::now();
        auto result = blockchain_->AddBlock(block);
        auto validate_end = std::chrono::steady_clock::now();

        if (!result.IsOk()) {
            download_manager_.MarkFailed(block_hash);
            stats_.failed_downloads++;
            return result;
        }

        // Mark as validated
        download_manager_.MarkValidated(block_hash);

        // Update statistics
        stats_.total_blocks_downloaded++;
        stats_.total_blocks_validated++;
        stats_.total_bytes_downloaded += block.Serialize().size();
        stats_.total_bytes_validated += block.Serialize().size();

        auto validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            validate_end - validate_start);
        UpdateAverageValidationTime(validation_time);

        // Update current height
        current_height_ = blockchain_->GetHeight();

        // Call progress callback
        if (progress_callback_) {
            progress_callback_(GetProgress());
        }

        return Ok();
    }

    void MarkBlockFailed(const uint256& block_hash, uint64_t peer_id) {
        download_manager_.MarkFailed(block_hash);
        stats_.failed_downloads++;
    }

    void RetryFailedBlocks() {
        // Get stalled downloads and retry them
        auto stalled = download_manager_.CheckStalledDownloads(config_.block_timeout);
        for (const auto& hash : stalled) {
            stats_.retries++;
        }
    }

    std::vector<BlockDownload> GetPendingBlocks() const {
        return download_manager_.GetPendingBlocks();
    }

    size_t GetBlocksInFlight() const {
        return download_manager_.GetBlocksInFlight();
    }

    std::optional<uint64_t> SelectSyncPeer() const {
        auto peers = GetSyncPeers();
        if (peers.empty()) {
            return std::nullopt;
        }

        // Select peer with highest reputation
        uint64_t best_peer = peers[0];
        int best_reputation = p2p_->GetPeerReputation(best_peer);

        for (uint64_t peer_id : peers) {
            int reputation = p2p_->GetPeerReputation(peer_id);
            if (reputation > best_reputation) {
                best_reputation = reputation;
                best_peer = peer_id;
            }
        }

        return best_peer;
    }

    std::vector<uint64_t> GetSyncPeers() const {
        // Return all connected peers that are suitable for sync
        // For now, return trusted peers
        return p2p_->GetTrustedPeers();
    }

    void UpdatePeerSyncStatus(uint64_t peer_id, uint64_t height) {
        std::lock_guard<std::mutex> lock(mutex_);
        peer_heights_[peer_id] = height;

        // Update target height if this peer reports higher
        if (height > target_height_) {
            target_height_ = height;
        }
    }

    void MarkPeerStalled(uint64_t peer_id) {
        // Decrease peer reputation
        p2p_->UpdatePeerReputation(peer_id, -10);
    }

    const Config& GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    void SetConfig(const Config& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    void RegisterSyncStateCallback(SyncStateCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_callback_ = callback;
    }

    void RegisterSyncProgressCallback(SyncProgressCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        progress_callback_ = callback;
    }

private:
    void SyncLoop() {
        while (is_syncing_) {
            if (is_paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Update state
            UpdateSyncState();

            // Process sync based on state
            switch (state_) {
                case SyncState::CONNECTING:
                    ProcessConnecting();
                    break;
                case SyncState::HEADERS_SYNC:
                    ProcessHeadersSync();
                    break;
                case SyncState::BLOCKS_SYNC:
                    ProcessBlocksSync();
                    break;
                case SyncState::CATCHING_UP:
                    ProcessCatchingUp();
                    break;
                case SyncState::SYNCED:
                    ProcessSynced();
                    break;
                case SyncState::STALLED:
                    ProcessStalled();
                    break;
                default:
                    break;
            }

            // Small delay between iterations
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void UpdateSyncState() {
        std::lock_guard<std::mutex> lock(mutex_);

        SyncState old_state = state_;

        // Determine new state
        if (p2p_->GetPeerCount() == 0) {
            state_ = SyncState::CONNECTING;
        } else if (config_.headers_first && headers_manager_.GetBestHeaderHeight() < target_height_) {
            state_ = SyncState::HEADERS_SYNC;
        } else if (current_height_ < target_height_) {
            uint64_t blocks_behind = target_height_ - current_height_;
            if (blocks_behind > 100) {
                state_ = SyncState::BLOCKS_SYNC;
            } else {
                state_ = SyncState::CATCHING_UP;
            }
        } else {
            state_ = SyncState::SYNCED;
        }

        // Check for stall
        if (GetBlocksInFlight() == 0 && current_height_ < target_height_) {
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - last_block_time_);
            if (elapsed > config_.stall_timeout) {
                state_ = SyncState::STALLED;
            }
        }

        // Call state callback if changed
        if (state_ != old_state && state_callback_) {
            state_callback_(state_);
        }
    }

    void ProcessConnecting() {
        // Wait for peers to connect
        // P2P node handles peer discovery
    }

    void ProcessHeadersSync() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Request headers from sync peer
        auto peer_id = SelectSyncPeer();
        if (!peer_id.has_value()) {
            return;
        }

        // Request next batch of headers
        uint256 start_hash = headers_manager_.GetBestHeaderHash();
        RequestHeaders(peer_id.value(), start_hash);
    }

    void ProcessBlocksSync() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Download blocks in parallel from multiple peers
        size_t in_flight = GetBlocksInFlight();
        if (in_flight >= config_.max_blocks_in_flight) {
            return;
        }

        // Get next blocks to download
        auto peers = GetSyncPeers();
        if (peers.empty()) {
            return;
        }

        // Distribute block downloads across peers
        size_t blocks_to_request = config_.max_blocks_in_flight - in_flight;
        size_t per_peer = blocks_to_request / peers.size();
        if (per_peer == 0) per_peer = 1;

        for (uint64_t peer_id : peers) {
            auto next_blocks = download_manager_.GetNextBlocks(per_peer, peer_id);
            if (!next_blocks.empty()) {
                RequestBlocks(next_blocks, peer_id);
            }
        }
    }

    void ProcessCatchingUp() {
        // Similar to blocks sync but with lower parallelism
        ProcessBlocksSync();
    }

    void ProcessSynced() {
        // Monitor for new blocks
        // Check if target height increased
    }

    void ProcessStalled() {
        // Retry failed blocks
        RetryFailedBlocks();

        // Ban stalled peers
        p2p_->AutoBanSuspiciousPeers();

        // Reset last block time
        last_block_time_ = std::chrono::system_clock::now();
    }

    double CalculateDownloadSpeed() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - sync_start_time_);

        if (elapsed.count() == 0) {
            return 0.0;
        }

        return static_cast<double>(stats_.total_bytes_downloaded) / elapsed.count();
    }

    void UpdateAverageValidationTime(std::chrono::milliseconds time) {
        if (stats_.total_blocks_validated == 0) {
            stats_.average_block_validation_time = time;
        } else {
            auto total_ms = stats_.average_block_validation_time.count() *
                          (stats_.total_blocks_validated - 1) + time.count();
            stats_.average_block_validation_time =
                std::chrono::milliseconds(total_ms / stats_.total_blocks_validated);
        }
    }

    std::shared_ptr<Blockchain> blockchain_;
    std::shared_ptr<P2PNode> p2p_;
    SyncState state_;
    bool is_syncing_;
    bool is_paused_;
    uint64_t current_height_;
    uint64_t target_height_;
    std::chrono::system_clock::time_point sync_start_time_;
    std::chrono::system_clock::time_point last_block_time_;
    SyncStatistics stats_;
    Config config_;
    HeadersSyncManager headers_manager_;
    BlockDownloadManager download_manager_;
    std::map<uint64_t, uint64_t> peer_heights_;
    SyncStateCallback state_callback_;
    SyncProgressCallback progress_callback_;
    std::thread sync_thread_;
    mutable std::mutex mutex_;
};

// ============================================================================
// BlockchainSyncManager
// ============================================================================

BlockchainSyncManager::BlockchainSyncManager(std::shared_ptr<Blockchain> blockchain,
                                             std::shared_ptr<P2PNode> p2p)
    : impl_(std::make_unique<Impl>(blockchain, p2p))
{
}

BlockchainSyncManager::~BlockchainSyncManager() {
    impl_->StopSync();
}

Result<void> BlockchainSyncManager::StartSync() {
    return impl_->StartSync();
}

void BlockchainSyncManager::StopSync() {
    impl_->StopSync();
}

void BlockchainSyncManager::PauseSync() {
    impl_->PauseSync();
}

void BlockchainSyncManager::ResumeSync() {
    impl_->ResumeSync();
}

bool BlockchainSyncManager::IsSyncing() const {
    return impl_->IsSyncing();
}

bool BlockchainSyncManager::IsSynced() const {
    return impl_->IsSynced();
}

SyncProgress BlockchainSyncManager::GetProgress() const {
    return impl_->GetProgress();
}

SyncStatistics BlockchainSyncManager::GetStatistics() const {
    return impl_->GetStatistics();
}

SyncState BlockchainSyncManager::GetState() const {
    return impl_->GetState();
}

std::chrono::seconds BlockchainSyncManager::EstimateTimeRemaining() const {
    return impl_->EstimateTimeRemaining();
}

Result<void> BlockchainSyncManager::RequestHeaders(uint64_t peer_id, const uint256& start_hash) {
    return impl_->RequestHeaders(peer_id, start_hash);
}

Result<void> BlockchainSyncManager::ProcessHeaders(uint64_t peer_id,
                                                   const std::vector<BlockHeader>& headers) {
    return impl_->ProcessHeaders(peer_id, headers);
}

uint64_t BlockchainSyncManager::GetBestHeaderHeight() const {
    return impl_->GetBestHeaderHeight();
}

uint256 BlockchainSyncManager::GetBestHeaderHash() const {
    return impl_->GetBestHeaderHash();
}

Result<void> BlockchainSyncManager::RequestBlock(const uint256& block_hash, uint64_t peer_id) {
    return impl_->RequestBlock(block_hash, peer_id);
}

Result<void> BlockchainSyncManager::RequestBlocks(const std::vector<uint256>& block_hashes,
                                                  uint64_t peer_id) {
    return impl_->RequestBlocks(block_hashes, peer_id);
}

Result<void> BlockchainSyncManager::ProcessBlock(uint64_t peer_id, const Block& block) {
    return impl_->ProcessBlock(peer_id, block);
}

void BlockchainSyncManager::MarkBlockFailed(const uint256& block_hash, uint64_t peer_id) {
    impl_->MarkBlockFailed(block_hash, peer_id);
}

void BlockchainSyncManager::RetryFailedBlocks() {
    impl_->RetryFailedBlocks();
}

std::vector<BlockDownload> BlockchainSyncManager::GetPendingBlocks() const {
    return impl_->GetPendingBlocks();
}

size_t BlockchainSyncManager::GetBlocksInFlight() const {
    return impl_->GetBlocksInFlight();
}

std::optional<uint64_t> BlockchainSyncManager::SelectSyncPeer() const {
    return impl_->SelectSyncPeer();
}

std::vector<uint64_t> BlockchainSyncManager::GetSyncPeers() const {
    return impl_->GetSyncPeers();
}

void BlockchainSyncManager::UpdatePeerSyncStatus(uint64_t peer_id, uint64_t height) {
    impl_->UpdatePeerSyncStatus(peer_id, height);
}

void BlockchainSyncManager::MarkPeerStalled(uint64_t peer_id) {
    impl_->MarkPeerStalled(peer_id);
}

const BlockchainSyncManager::Config& BlockchainSyncManager::GetConfig() const {
    return impl_->GetConfig();
}

void BlockchainSyncManager::SetConfig(const Config& config) {
    impl_->SetConfig(config);
}

void BlockchainSyncManager::RegisterSyncStateCallback(SyncStateCallback callback) {
    impl_->RegisterSyncStateCallback(callback);
}

void BlockchainSyncManager::RegisterSyncProgressCallback(SyncProgressCallback callback) {
    impl_->RegisterSyncProgressCallback(callback);
}

// ============================================================================
// HeadersSyncManager
// ============================================================================

HeadersSyncManager::HeadersSyncManager(std::shared_ptr<Blockchain> blockchain)
    : blockchain_(blockchain)
    , best_header_height_(0)
{
}

Result<void> HeadersSyncManager::AddHeader(const BlockHeader& header) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint256 hash = header.GetHash();

    // Check if already have this header
    if (headers_.count(hash) > 0) {
        return Ok();
    }

    // Add to maps
    headers_[hash] = header;
    height_to_hash_[header.height] = hash;

    // Update best header
    if (header.height > best_header_height_) {
        best_header_height_ = header.height;
        best_header_hash_ = hash;
    }

    return Ok();
}

Result<void> HeadersSyncManager::AddHeaders(const std::vector<BlockHeader>& headers) {
    for (const auto& header : headers) {
        auto result = AddHeader(header);
        if (!result.IsOk()) {
            return result;
        }
    }
    return Ok();
}

Result<void> HeadersSyncManager::ValidateHeaderChain() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate chain of headers
    // Check that each header points to previous
    // For now, return success
    return Ok();
}

std::vector<BlockHeader> HeadersSyncManager::GetHeadersFromHeight(uint64_t height, size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BlockHeader> result;
    for (uint64_t h = height; h < height + count && h <= best_header_height_; h++) {
        auto it = height_to_hash_.find(h);
        if (it != height_to_hash_.end()) {
            auto header_it = headers_.find(it->second);
            if (header_it != headers_.end()) {
                result.push_back(header_it->second);
            }
        }
    }

    return result;
}

std::optional<BlockHeader> HeadersSyncManager::GetBestHeader() const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = headers_.find(best_header_hash_);
    if (it != headers_.end()) {
        return it->second;
    }

    return std::nullopt;
}

uint64_t HeadersSyncManager::GetBestHeaderHeight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return best_header_height_;
}

bool HeadersSyncManager::HasHeader(const uint256& hash) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return headers_.count(hash) > 0;
}

void HeadersSyncManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    headers_.clear();
    height_to_hash_.clear();
    best_header_height_ = 0;
    best_header_hash_ = uint256{};
}

// ============================================================================
// BlockDownloadManager
// ============================================================================

BlockDownloadManager::BlockDownloadManager() {
}

void BlockDownloadManager::AddBlock(const uint256& hash, uint64_t height) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (blocks_.count(hash) > 0) {
        return;
    }

    BlockDownload download;
    download.hash = hash;
    download.height = height;
    download.peer_id = 0;
    download.status = BlockDownloadStatus::REQUESTED;
    download.request_time = std::chrono::system_clock::now();
    download.retry_count = 0;

    blocks_[hash] = download;
}

void BlockDownloadManager::AddBlocks(const std::vector<std::pair<uint256, uint64_t>>& blocks) {
    for (const auto& [hash, height] : blocks) {
        AddBlock(hash, height);
    }
}

void BlockDownloadManager::MarkRequested(const uint256& hash, uint64_t peer_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = blocks_.find(hash);
    if (it != blocks_.end()) {
        it->second.status = BlockDownloadStatus::DOWNLOADING;
        it->second.peer_id = peer_id;
        it->second.request_time = std::chrono::system_clock::now();
    }
}

void BlockDownloadManager::MarkReceived(const uint256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = blocks_.find(hash);
    if (it != blocks_.end()) {
        it->second.status = BlockDownloadStatus::RECEIVED;
    }
}

void BlockDownloadManager::MarkValidated(const uint256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = blocks_.find(hash);
    if (it != blocks_.end()) {
        it->second.status = BlockDownloadStatus::VALIDATED;
    }
}

void BlockDownloadManager::MarkFailed(const uint256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = blocks_.find(hash);
    if (it != blocks_.end()) {
        it->second.status = BlockDownloadStatus::FAILED;
        it->second.retry_count++;
    }
}

std::vector<uint256> BlockDownloadManager::GetNextBlocks(size_t count, uint64_t peer_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<uint256> result;

    for (const auto& [hash, download] : blocks_) {
        if (download.status == BlockDownloadStatus::REQUESTED ||
            download.status == BlockDownloadStatus::FAILED) {
            result.push_back(hash);
            if (result.size() >= count) {
                break;
            }
        }
    }

    return result;
}

std::vector<BlockDownload> BlockDownloadManager::GetPendingBlocks() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BlockDownload> result;

    for (const auto& [hash, download] : blocks_) {
        if (download.status != BlockDownloadStatus::VALIDATED) {
            result.push_back(download);
        }
    }

    return result;
}

size_t BlockDownloadManager::GetBlocksInFlight() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& [hash, download] : blocks_) {
        if (download.status == BlockDownloadStatus::DOWNLOADING ||
            download.status == BlockDownloadStatus::RECEIVED) {
            count++;
        }
    }

    return count;
}

std::vector<uint256> BlockDownloadManager::CheckStalledDownloads(std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<uint256> stalled;
    auto now = std::chrono::system_clock::now();

    for (auto& [hash, download] : blocks_) {
        if (download.status == BlockDownloadStatus::DOWNLOADING) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - download.request_time);

            if (elapsed > timeout) {
                stalled.push_back(hash);
                download.status = BlockDownloadStatus::FAILED;
                download.retry_count++;
            }
        }
    }

    return stalled;
}

void BlockDownloadManager::ClearCompleted() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = blocks_.begin(); it != blocks_.end(); ) {
        if (it->second.status == BlockDownloadStatus::VALIDATED) {
            it = blocks_.erase(it);
        } else {
            ++it;
        }
    }
}

void BlockDownloadManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    blocks_.clear();
}

// ============================================================================
// SyncScheduler
// ============================================================================

SyncScheduler::SyncScheduler() {
    auto now = std::chrono::system_clock::now();
    last_header_sync_ = now;
    last_block_sync_ = now;
    last_peer_update_ = now;
    last_cleanup_ = now;
}

void SyncScheduler::ScheduleHeaderSync() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_header_sync_ = std::chrono::system_clock::now();
}

void SyncScheduler::ScheduleBlockSync() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_block_sync_ = std::chrono::system_clock::now();
}

void SyncScheduler::SchedulePeerUpdate() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_peer_update_ = std::chrono::system_clock::now();
}

void SyncScheduler::ScheduleCleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_cleanup_ = std::chrono::system_clock::now();
}

void SyncScheduler::ProcessTasks() {
    // Process scheduled tasks
}

bool SyncScheduler::IsHeaderSyncDue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_header_sync_);
    return elapsed > std::chrono::seconds(60);
}

bool SyncScheduler::IsBlockSyncDue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_block_sync_);
    return elapsed > std::chrono::seconds(10);
}

bool SyncScheduler::IsPeerUpdateDue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_peer_update_);
    return elapsed > std::chrono::seconds(30);
}

bool SyncScheduler::IsCleanupDue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup_);
    return elapsed > std::chrono::seconds(300);
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string ToString(SyncState state) {
    switch (state) {
        case SyncState::NOT_STARTED: return "NOT_STARTED";
        case SyncState::CONNECTING: return "CONNECTING";
        case SyncState::HEADERS_SYNC: return "HEADERS_SYNC";
        case SyncState::BLOCKS_SYNC: return "BLOCKS_SYNC";
        case SyncState::CATCHING_UP: return "CATCHING_UP";
        case SyncState::SYNCED: return "SYNCED";
        case SyncState::STALLED: return "STALLED";
        case SyncState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string ToString(BlockDownloadStatus status) {
    switch (status) {
        case BlockDownloadStatus::REQUESTED: return "REQUESTED";
        case BlockDownloadStatus::DOWNLOADING: return "DOWNLOADING";
        case BlockDownloadStatus::RECEIVED: return "RECEIVED";
        case BlockDownloadStatus::VALIDATED: return "VALIDATED";
        case BlockDownloadStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

double CalculateSyncProgress(uint64_t current_height, uint64_t target_height) {
    if (target_height == 0) {
        return 0.0;
    }

    if (current_height >= target_height) {
        return 100.0;
    }

    return (static_cast<double>(current_height) / target_height) * 100.0;
}

std::chrono::seconds EstimateSyncTime(uint64_t blocks_remaining,
                                      double download_speed_bps,
                                      size_t avg_block_size) {
    if (download_speed_bps <= 0.0) {
        return std::chrono::seconds(0);
    }

    uint64_t bytes_remaining = blocks_remaining * avg_block_size;
    double seconds = bytes_remaining / download_speed_bps;

    return std::chrono::seconds(static_cast<uint64_t>(seconds));
}

} // namespace intcoin
