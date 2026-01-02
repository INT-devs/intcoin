// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_IBD_ASSUME_UTXO_H
#define INTCOIN_IBD_ASSUME_UTXO_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace intcoin {

// Forward declarations
class uint256;

namespace ibd {

/**
 * UTXO entry in snapshot
 */
struct UTXOEntry {
    uint256 txid;
    uint32_t vout{0};
    uint64_t amount{0};
    std::vector<uint8_t> script_pubkey;
    uint32_t height{0};
    bool is_coinbase{false};
};

/**
 * UTXO snapshot metadata
 */
struct SnapshotMetadata {
    uint32_t block_height{0};
    uint256 block_hash;
    uint256 utxo_set_hash;
    uint64_t total_amount{0};
    uint64_t num_utxos{0};
    uint64_t timestamp{0};
    std::string source_url;
    std::vector<uint8_t> signature;
};

/**
 * UTXO snapshot
 */
struct UTXOSnapshot {
    SnapshotMetadata metadata;
    std::vector<UTXOEntry> utxos;
};

/**
 * Snapshot verification result
 */
struct VerificationResult {
    bool valid{false};
    std::string error_message;
    uint256 computed_hash;
    uint64_t verification_time_ms{0};
};

/**
 * Background validation progress
 */
struct BackgroundProgress {
    uint32_t validated_height{0};
    uint32_t target_height{0};
    double progress_percent{0.0};
    uint64_t estimated_time_remaining_sec{0};
    bool completed{false};
};

/**
 * AssumeUTXO Manager
 *
 * Manages UTXO snapshots for fast initial block download.
 * Allows nodes to assume a UTXO set at a known height and
 * validate recent blocks first, with background validation
 * of historical blocks.
 */
class AssumeUTXOManager {
public:
    /**
     * Hardcoded trusted snapshot hashes
     */
    struct TrustedSnapshot {
        uint32_t height;
        uint256 block_hash;
        uint256 utxo_hash;
    };

    AssumeUTXOManager();
    ~AssumeUTXOManager();

    /**
     * Download UTXO snapshot from URL
     *
     * @param url Snapshot source URL
     * @param verify_signature Verify cryptographic signature
     * @return True if download succeeded
     */
    bool DownloadSnapshot(const std::string& url, bool verify_signature = true);

    /**
     * Load UTXO snapshot from file
     *
     * @param snapshot_path Path to snapshot file
     * @return True if loaded successfully
     */
    bool LoadSnapshot(const std::string& snapshot_path);

    /**
     * Verify UTXO snapshot integrity
     *
     * @param snapshot Snapshot to verify
     * @return Verification result
     */
    VerificationResult VerifySnapshot(const UTXOSnapshot& snapshot) const;

    /**
     * Apply UTXO snapshot to chainstate
     *
     * Activates the snapshot, allowing the node to sync from this point.
     *
     * @return True if applied successfully
     */
    bool ApplySnapshot();

    /**
     * Start background validation of historical blocks
     *
     * Validates blocks before the snapshot height in the background.
     */
    void StartBackgroundValidation();

    /**
     * Get background validation progress
     */
    BackgroundProgress GetBackgroundProgress() const;

    /**
     * Check if using AssumeUTXO mode
     */
    bool IsAssumeUTXOActive() const;

    /**
     * Get list of hardcoded trusted snapshots
     */
    static std::vector<TrustedSnapshot> GetTrustedSnapshots();

    /**
     * Create UTXO snapshot at current height
     *
     * Used for creating snapshots for distribution.
     *
     * @param output_path Path to save snapshot
     * @return True if created successfully
     */
    bool CreateSnapshot(const std::string& output_path) const;

    /**
     * Export snapshot metadata to JSON
     */
    std::string ExportMetadataJSON() const;

private:
    /**
     * Compute UTXO set hash
     */
    uint256 ComputeUTXOHash(const std::vector<UTXOEntry>& utxos) const;

    /**
     * Verify snapshot signature
     */
    bool VerifySignature(const SnapshotMetadata& metadata) const;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ibd
} // namespace intcoin

#endif // INTCOIN_IBD_ASSUME_UTXO_H
