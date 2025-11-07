// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_IBD_H
#define INTCOIN_IBD_H

#include "primitives.h"
#include "block.h"
#include "p2p.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

namespace intcoin {

/**
 * Initial Block Download (IBD) Manager
 *
 * Handles the synchronization of the blockchain from peers during initial startup
 * or when a node is significantly behind the network.
 */
class IBDManager {
public:
    /**
     * IBD States
     */
    enum class State {
        IDLE,              // Not in IBD
        STARTING,          // Initializing IBD
        DOWNLOADING,       // Actively downloading blocks
        VALIDATING,        // Validating downloaded blocks
        COMPLETED,         // IBD finished
        FAILED             // IBD failed
    };

    /**
     * Block request information
     */
    struct BlockRequest {
        Hash256 block_hash;
        p2p::PeerAddress peer;
        uint64_t request_time;
        uint32_t retry_count;
        bool in_flight;

        BlockRequest()
            : block_hash{}, request_time(0), retry_count(0), in_flight(false) {}
    };

    /**
     * IBD statistics
     */
    struct Stats {
        uint32_t current_height;
        uint32_t target_height;
        uint32_t blocks_downloaded;
        uint32_t blocks_validated;
        uint32_t blocks_failed;
        uint64_t start_time;
        uint64_t bytes_received;
        double download_rate;  // bytes/second

        double get_progress() const {
            if (target_height == 0) return 0.0;
            return (static_cast<double>(current_height) / target_height) * 100.0;
        }

        uint64_t get_elapsed_time() const {
            return std::chrono::system_clock::now().time_since_epoch().count() - start_time;
        }
    };

private:
    State state_;
    Stats stats_;
    std::mutex mutex_;

    // Block download queue
    std::vector<Hash256> blocks_to_download_;
    std::unordered_map<Hash256, BlockRequest> in_flight_blocks_;

    // Configuration
    static constexpr uint32_t MAX_BLOCKS_IN_FLIGHT = 128;
    static constexpr uint32_t MAX_RETRY_COUNT = 3;
    static constexpr uint64_t REQUEST_TIMEOUT_MS = 30000;  // 30 seconds
    static constexpr uint32_t BATCH_SIZE = 500;  // Request 500 blocks at a time

    // Callbacks
    std::function<void(const Block&)> on_block_received_;
    std::function<void(double)> on_progress_update_;

public:
    IBDManager();
    ~IBDManager() = default;

    /**
     * Start Initial Block Download
     *
     * @param start_height Current blockchain height
     * @param target_height Target height from network
     * @return true if IBD started successfully
     */
    bool start(uint32_t start_height, uint32_t target_height);

    /**
     * Stop IBD process
     */
    void stop();

    /**
     * Process a received block during IBD
     *
     * @param block The received block
     * @param peer Peer that sent the block
     * @return true if block was expected and processed
     */
    bool process_block(const Block& block, const p2p::PeerAddress& peer);

    /**
     * Request blocks from a peer
     *
     * @param peer Peer to request from
     * @param block_hashes Hashes of blocks to request
     */
    void request_blocks(const p2p::PeerAddress& peer, const std::vector<Hash256>& block_hashes);

    /**
     * Handle timeout for in-flight block requests
     * Call this periodically to retry failed requests
     */
    void handle_timeouts();

    /**
     * Check if currently in IBD
     */
    bool is_in_ibd() const {
        return state_ == State::DOWNLOADING || state_ == State::VALIDATING;
    }

    /**
     * Get current state
     */
    State get_state() const {
        return state_;
    }

    /**
     * Get IBD statistics
     */
    Stats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * Set callback for when block is received
     */
    void set_block_callback(std::function<void(const Block&)> callback) {
        on_block_received_ = callback;
    }

    /**
     * Set callback for progress updates
     */
    void set_progress_callback(std::function<void(double)> callback) {
        on_progress_update_ = callback;
    }

    /**
     * Get number of blocks currently in-flight
     */
    size_t get_in_flight_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return in_flight_blocks_.size();
    }

    /**
     * Determine if we should be in IBD mode
     *
     * @param our_height Our current blockchain height
     * @param peer_height Highest height reported by peers
     * @return true if we should enter IBD
     */
    static bool should_start_ibd(uint32_t our_height, uint32_t peer_height) {
        // Enter IBD if we're more than 24 hours (720 blocks at 2min) behind
        return peer_height > our_height + 720;
    }

private:
    /**
     * Add blocks to download queue
     */
    void queue_blocks(const std::vector<Hash256>& block_hashes);

    /**
     * Get next batch of blocks to request
     */
    std::vector<Hash256> get_next_batch();

    /**
     * Mark block request as in-flight
     */
    void mark_in_flight(const Hash256& block_hash, const p2p::PeerAddress& peer);

    /**
     * Remove block from in-flight tracking
     */
    void clear_in_flight(const Hash256& block_hash);

    /**
     * Retry failed block download
     */
    void retry_block(const Hash256& block_hash);

    /**
     * Update statistics
     */
    void update_stats();

    /**
     * Notify progress callback
     */
    void notify_progress();
};

/**
 * Block synchronization manager
 * Handles keeping the blockchain in sync after IBD is complete
 */
class BlockSyncManager {
public:
    /**
     * Sync strategy
     */
    enum class SyncStrategy {
        HEADERS_FIRST,     // Download headers first, then blocks
        BLOCKS_FIRST       // Download complete blocks
    };

private:
    SyncStrategy strategy_;
    std::vector<BlockHeader> pending_headers_;
    std::unordered_map<Hash256, Block> orphan_blocks_;
    std::mutex mutex_;

    static constexpr size_t MAX_ORPHAN_BLOCKS = 100;
    static constexpr uint32_t MAX_HEADERS_BATCH = 2000;

public:
    BlockSyncManager(SyncStrategy strategy = SyncStrategy::HEADERS_FIRST);

    /**
     * Process received block header
     */
    bool process_header(const BlockHeader& header);

    /**
     * Process received block
     */
    bool process_block(const Block& block);

    /**
     * Request headers from peer
     */
    void request_headers(const p2p::PeerAddress& peer, const Hash256& from_hash);

    /**
     * Request block data from peer
     */
    void request_block(const p2p::PeerAddress& peer, const Hash256& block_hash);

    /**
     * Get orphan blocks that can now be connected
     */
    std::vector<Block> get_connectable_orphans(const Hash256& parent_hash);

    /**
     * Add orphan block
     */
    bool add_orphan(const Block& block);

    /**
     * Remove orphan block
     */
    void remove_orphan(const Hash256& block_hash);

    /**
     * Get number of orphan blocks
     */
    size_t get_orphan_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return orphan_blocks_.size();
    }

    /**
     * Clear orphan blocks
     */
    void clear_orphans();
};

} // namespace intcoin

#endif // INTCOIN_IBD_H
