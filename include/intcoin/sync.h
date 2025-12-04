/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Blockchain Synchronization Manager
 */

#ifndef INTCOIN_SYNC_H
#define INTCOIN_SYNC_H

#include "types.h"
#include "block.h"
#include "blockchain.h"
#include "network.h"
#include <memory>
#include <vector>
#include <chrono>
#include <optional>
#include <atomic>
#include <mutex>

namespace intcoin {

// ============================================================================
// Sync State
// ============================================================================

enum class SyncState {
    NOT_STARTED,      // Sync not started yet
    CONNECTING,       // Connecting to peers
    HEADERS_SYNC,     // Downloading headers
    BLOCKS_SYNC,      // Downloading blocks
    CATCHING_UP,      // Catching up with chain tip
    SYNCED,           // Fully synced
    STALLED,          // Sync stalled (no progress)
    ERROR             // Error during sync
};

// ============================================================================
// Block Download Status
// ============================================================================

enum class BlockDownloadStatus {
    REQUESTED,        // Requested from peer
    DOWNLOADING,      // Currently downloading
    RECEIVED,         // Received but not validated
    VALIDATED,        // Validated and ready to add
    FAILED            // Download failed
};

struct BlockDownload {
    uint256 hash;
    uint64_t height;
    uint64_t peer_id;
    BlockDownloadStatus status;
    std::chrono::system_clock::time_point request_time;
    size_t retry_count;
};

// ============================================================================
// Sync Progress
// ============================================================================

struct SyncProgress {
    SyncState state;
    uint64_t current_height;
    uint64_t target_height;
    uint64_t headers_count;
    uint64_t blocks_downloaded;
    uint64_t blocks_validated;
    double progress_percent;
    double download_speed_bps;  // bytes per second
    std::chrono::seconds estimated_time_remaining;
    uint64_t connected_peers;
    uint64_t sync_peers;
};

// ============================================================================
// Sync Statistics
// ============================================================================

struct SyncStatistics {
    uint64_t total_headers_downloaded;
    uint64_t total_blocks_downloaded;
    uint64_t total_blocks_validated;
    uint64_t total_bytes_downloaded;
    uint64_t total_bytes_validated;
    std::chrono::milliseconds total_sync_time;
    std::chrono::milliseconds average_block_download_time;
    std::chrono::milliseconds average_block_validation_time;
    uint64_t failed_downloads;
    uint64_t retries;
};

// ============================================================================
// Blockchain Sync Manager
// ============================================================================

class BlockchainSyncManager {
public:
    /// Constructor
    BlockchainSyncManager(std::shared_ptr<Blockchain> blockchain,
                         std::shared_ptr<P2PNode> p2p);

    /// Destructor
    ~BlockchainSyncManager();

    // ------------------------------------------------------------------------
    // Sync Control
    // ------------------------------------------------------------------------

    /// Start synchronization
    Result<void> StartSync();

    /// Stop synchronization
    void StopSync();

    /// Pause synchronization
    void PauseSync();

    /// Resume synchronization
    void ResumeSync();

    /// Check if syncing
    bool IsSyncing() const;

    /// Check if synced
    bool IsSynced() const;

    // ------------------------------------------------------------------------
    // Sync Progress
    // ------------------------------------------------------------------------

    /// Get sync progress
    SyncProgress GetProgress() const;

    /// Get sync statistics
    SyncStatistics GetStatistics() const;

    /// Get sync state
    SyncState GetState() const;

    /// Estimate time remaining
    std::chrono::seconds EstimateTimeRemaining() const;

    // ------------------------------------------------------------------------
    // Headers Sync
    // ------------------------------------------------------------------------

    /// Request headers from peers
    Result<void> RequestHeaders(uint64_t peer_id, const uint256& start_hash);

    /// Process received headers
    Result<void> ProcessHeaders(uint64_t peer_id,
                               const std::vector<BlockHeader>& headers);

    /// Get best header height
    uint64_t GetBestHeaderHeight() const;

    /// Get best header hash
    uint256 GetBestHeaderHash() const;

    // ------------------------------------------------------------------------
    // Block Download
    // ------------------------------------------------------------------------

    /// Request block from peer
    Result<void> RequestBlock(const uint256& block_hash, uint64_t peer_id);

    /// Request blocks batch
    Result<void> RequestBlocks(const std::vector<uint256>& block_hashes,
                              uint64_t peer_id);

    /// Process received block
    Result<void> ProcessBlock(uint64_t peer_id, const Block& block);

    /// Mark block download as failed
    void MarkBlockFailed(const uint256& block_hash, uint64_t peer_id);

    /// Retry failed blocks
    void RetryFailedBlocks();

    /// Get pending blocks
    std::vector<BlockDownload> GetPendingBlocks() const;

    /// Get blocks in flight
    size_t GetBlocksInFlight() const;

    // ------------------------------------------------------------------------
    // Peer Management
    // ------------------------------------------------------------------------

    /// Select best sync peer
    std::optional<uint64_t> SelectSyncPeer() const;

    /// Get sync peers
    std::vector<uint64_t> GetSyncPeers() const;

    /// Update peer sync status
    void UpdatePeerSyncStatus(uint64_t peer_id, uint64_t height);

    /// Mark peer as stalled
    void MarkPeerStalled(uint64_t peer_id);

    // ------------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------------

    struct Config {
        size_t max_blocks_in_flight;        // Maximum concurrent block downloads
        size_t max_blocks_per_peer;         // Maximum blocks per peer
        std::chrono::seconds block_timeout; // Timeout for block download
        std::chrono::seconds stall_timeout; // Timeout for stall detection
        size_t max_retries;                 // Maximum retry attempts
        bool headers_first;                 // Use headers-first sync
        size_t header_batch_size;           // Headers per batch
    };

    /// Get configuration
    const Config& GetConfig() const;

    /// Set configuration
    void SetConfig(const Config& config);

    // ------------------------------------------------------------------------
    // Callbacks
    // ------------------------------------------------------------------------

    /// Callback for sync state change
    using SyncStateCallback = std::function<void(SyncState)>;

    /// Callback for sync progress
    using SyncProgressCallback = std::function<void(const SyncProgress&)>;

    /// Register sync state callback
    void RegisterSyncStateCallback(SyncStateCallback callback);

    /// Register sync progress callback
    void RegisterSyncProgressCallback(SyncProgressCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Headers Sync Manager
// ============================================================================

class HeadersSyncManager {
public:
    /// Constructor
    HeadersSyncManager(std::shared_ptr<Blockchain> blockchain);

    /// Add header
    Result<void> AddHeader(const BlockHeader& header);

    /// Add headers batch
    Result<void> AddHeaders(const std::vector<BlockHeader>& headers);

    /// Validate header chain
    Result<void> ValidateHeaderChain() const;

    /// Get headers from height
    std::vector<BlockHeader> GetHeadersFromHeight(uint64_t height,
                                                  size_t count) const;

    /// Get best header
    std::optional<BlockHeader> GetBestHeader() const;

    /// Get best header height
    uint64_t GetBestHeaderHeight() const;

    /// Check if header exists
    bool HasHeader(const uint256& hash) const;

    /// Clear headers
    void Clear();

private:
    std::shared_ptr<Blockchain> blockchain_;
    std::map<uint256, BlockHeader> headers_;
    std::map<uint64_t, uint256> height_to_hash_;
    uint256 best_header_hash_;
    uint64_t best_header_height_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Block Download Manager
// ============================================================================

class BlockDownloadManager {
public:
    /// Constructor
    BlockDownloadManager();

    /// Add block to download queue
    void AddBlock(const uint256& hash, uint64_t height);

    /// Add blocks batch
    void AddBlocks(const std::vector<std::pair<uint256, uint64_t>>& blocks);

    /// Mark block as requested
    void MarkRequested(const uint256& hash, uint64_t peer_id);

    /// Mark block as received
    void MarkReceived(const uint256& hash);

    /// Mark block as validated
    void MarkValidated(const uint256& hash);

    /// Mark block as failed
    void MarkFailed(const uint256& hash);

    /// Get next blocks to download
    std::vector<uint256> GetNextBlocks(size_t count, uint64_t peer_id) const;

    /// Get pending blocks
    std::vector<BlockDownload> GetPendingBlocks() const;

    /// Get blocks in flight
    size_t GetBlocksInFlight() const;

    /// Check for stalled downloads
    std::vector<uint256> CheckStalledDownloads(std::chrono::seconds timeout);

    /// Clear completed blocks
    void ClearCompleted();

    /// Clear all
    void Clear();

private:
    std::map<uint256, BlockDownload> blocks_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Sync Scheduler
// ============================================================================

class SyncScheduler {
public:
    /// Constructor
    SyncScheduler();

    /// Schedule header sync
    void ScheduleHeaderSync();

    /// Schedule block sync
    void ScheduleBlockSync();

    /// Schedule peer update
    void SchedulePeerUpdate();

    /// Schedule cleanup
    void ScheduleCleanup();

    /// Process scheduled tasks
    void ProcessTasks();

    /// Check if header sync due
    bool IsHeaderSyncDue() const;

    /// Check if block sync due
    bool IsBlockSyncDue() const;

    /// Check if peer update due
    bool IsPeerUpdateDue() const;

    /// Check if cleanup due
    bool IsCleanupDue() const;

private:
    std::chrono::system_clock::time_point last_header_sync_;
    std::chrono::system_clock::time_point last_block_sync_;
    std::chrono::system_clock::time_point last_peer_update_;
    std::chrono::system_clock::time_point last_cleanup_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Utility Functions
// ============================================================================

/// Convert sync state to string
std::string ToString(SyncState state);

/// Convert block download status to string
std::string ToString(BlockDownloadStatus status);

/// Calculate sync progress percentage
double CalculateSyncProgress(uint64_t current_height, uint64_t target_height);

/// Estimate remaining sync time
std::chrono::seconds EstimateSyncTime(uint64_t blocks_remaining,
                                      double download_speed_bps,
                                      size_t avg_block_size);

} // namespace intcoin

#endif // INTCOIN_SYNC_H
