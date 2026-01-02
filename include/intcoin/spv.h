// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_SPV_H
#define INTCOIN_SPV_H

#include <intcoin/block.h>
#include <intcoin/bloom.h>
#include <intcoin/storage.h>
#include <intcoin/network.h>
#include <intcoin/transaction.h>
#include <intcoin/types.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace intcoin {

/// SPV (Simplified Payment Verification) client for lightweight wallet operations
/// Implements Bitcoin-style SPV as described in Satoshi's whitepaper Section 8
class SPVClient {
public:
    /// Constructor
    /// @param db Storage backend for headers and merkle proofs
    SPVClient(std::shared_ptr<BlockchainDB> db);

    /// Destructor
    ~SPVClient();

    /// Start SPV sync from genesis or last known header
    /// @return Success/failure result
    Result<void> StartSync();

    /// Stop SPV sync
    void StopSync();

    /// Check if sync is in progress
    /// @return True if syncing
    bool IsSyncing() const;

    /// Get current sync progress (0.0 to 1.0)
    /// @return Sync progress as fraction
    double GetSyncProgress() const;

    /// Get best header height
    /// @return Block height of best known header
    uint64_t GetBestHeight() const;

    /// Get best header hash
    /// @return Hash of best known header
    uint256 GetBestHash() const;

    /// Request merkle proof for transaction
    /// @param tx_hash Transaction hash
    /// @param block_hash Block hash containing transaction
    /// @return Merkle proof (branch + index)
    Result<std::pair<std::vector<uint256>, size_t>> RequestMerkleProof(
        const uint256& tx_hash, const uint256& block_hash);

    /// Verify transaction is in block using merkle proof
    /// @param tx_hash Transaction hash
    /// @param block_hash Block hash
    /// @param merkle_branch Merkle branch (sibling hashes)
    /// @param tx_index Transaction index in block
    /// @return True if proof valid
    bool VerifyTransaction(const uint256& tx_hash, const uint256& block_hash,
                          const std::vector<uint256>& merkle_branch, size_t tx_index);

    /// Add wallet address to monitor for transactions
    /// @param address Address to monitor
    void AddWatchAddress(const std::string& address);

    /// Remove wallet address from monitoring
    /// @param address Address to remove
    void RemoveWatchAddress(const std::string& address);

    /// Get all monitored addresses
    /// @return Set of monitored addresses
    std::set<std::string> GetWatchAddresses() const;

    /// Set bloom filter for transaction filtering
    /// @param filter Bloom filter to use
    void SetBloomFilter(const BloomFilter& filter);

    /// Get current bloom filter
    /// @return Current bloom filter (if set)
    Result<BloomFilter> GetBloomFilter() const;

    /// Clear bloom filter (stop filtering)
    void ClearBloomFilter();

    /// Check if bloom filter is active
    /// @return True if bloom filter is set
    bool HasBloomFilter() const;

    /// Get header by hash
    /// @param hash Block hash
    /// @return Block header
    Result<BlockHeader> GetHeader(const uint256& hash);

    /// Get header by height
    /// @param height Block height
    /// @return Block header
    Result<BlockHeader> GetHeaderByHeight(uint64_t height);

    /// Get headers in range
    /// @param start_height Start height (inclusive)
    /// @param end_height End height (inclusive)
    /// @return Vector of headers
    std::vector<BlockHeader> GetHeadersInRange(uint64_t start_height, uint64_t end_height);

    /// Check if we have header for hash
    /// @param hash Block hash
    /// @return True if header exists
    bool HasHeader(const uint256& hash) const;

    /// Get total number of headers stored
    /// @return Header count
    uint64_t GetHeaderCount() const;

    /// Estimate bandwidth usage (bytes)
    /// @return Estimated bytes sent/received
    struct BandwidthStats {
        uint64_t headers_downloaded;  // Bytes downloading headers
        uint64_t proofs_downloaded;    // Bytes downloading merkle proofs
        uint64_t total_sent;           // Total bytes sent
        uint64_t total_received;       // Total bytes received
    };
    BandwidthStats GetBandwidthStats() const;

private:
    /// Network message handlers
    void HandleHeaders(const std::vector<BlockHeader>& headers);
    void HandleMerkleBlock(const uint256& block_hash, const std::vector<uint256>& tx_hashes,
                          const std::vector<uint256>& merkle_branch);

    /// Request headers from peers
    void RequestHeaders();

    /// Validate header chain (PoW, timestamps, difficulty)
    Result<void> ValidateHeaderChain(const std::vector<BlockHeader>& headers);

    /// Store header to database
    Result<void> StoreHeader(const BlockHeader& header);

    /// Load headers from database on startup
    Result<void> LoadHeaders();

    /// Sync worker thread
    void SyncWorker();

    /// Database backend
    std::shared_ptr<BlockchainDB> db_;

    /// Header chain (hash -> header)
    std::unordered_map<uint256, BlockHeader, uint256_hash> headers_;

    /// Height index (height -> hash)
    std::unordered_map<uint64_t, uint256> height_index_;

    /// Best header hash
    uint256 best_hash_;

    /// Best header height
    uint64_t best_height_;

    /// Watched addresses for transaction monitoring
    std::set<std::string> watch_addresses_;

    /// Bloom filter for transaction filtering (optional)
    std::unique_ptr<BloomFilter> bloom_filter_;

    /// Pending merkle proof requests (tx_hash -> block_hash)
    std::unordered_map<uint256, uint256, uint256_hash> pending_proof_requests_;

    /// Cached merkle proofs (tx_hash -> (branch, index))
    std::unordered_map<uint256, std::pair<std::vector<uint256>, size_t>, uint256_hash> merkle_proofs_;

    /// Sync state
    bool is_syncing_;
    std::atomic<double> sync_progress_;

    /// Bandwidth statistics
    BandwidthStats bandwidth_stats_;

    /// Mutex for thread safety
    mutable std::mutex mutex_;

    /// Sync thread
    std::unique_ptr<std::thread> sync_thread_;
    std::atomic<bool> stop_sync_;
};

/// SPV-specific storage keys
namespace spv_storage {
    /// Prefix for header storage: "h" + block_hash -> BlockHeader
    constexpr uint8_t PREFIX_HEADER = 0x68;  // 'h'

    /// Prefix for height index: "H" + height -> block_hash
    constexpr uint8_t PREFIX_HEIGHT = 0x48;  // 'H'

    /// Prefix for merkle proof storage: "m" + tx_hash -> (branch, index)
    constexpr uint8_t PREFIX_MERKLE_PROOF = 0x6D;  // 'm'

    /// Best header hash key: "best_header"
    const std::string KEY_BEST_HEADER = "best_header";

    /// Best height key: "best_height"
    const std::string KEY_BEST_HEIGHT = "best_height";
}

}  // namespace intcoin

#endif  // INTCOIN_SPV_H
