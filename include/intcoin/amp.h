// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Atomic Multi-Path Payments (AMP) Implementation
// Split large payments across multiple routes for improved reliability
//
// AMP allows splitting a single payment into multiple HTLCs that can
// be routed through different paths, improving success rates and privacy.

#ifndef INTCOIN_AMP_H
#define INTCOIN_AMP_H

#include "lightning.h"
#include "primitives.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <map>
#include <optional>

namespace intcoin {
namespace amp {

/**
 * AMP protocol version
 */
static constexpr uint32_t AMP_VERSION = 1;

/**
 * Maximum number of paths for a single AMP payment
 */
static constexpr size_t MAX_AMP_PATHS = 16;

/**
 * Minimum amount per path (in satoshis)
 */
static constexpr uint64_t MIN_PATH_AMOUNT = 1000;

/**
 * AMP payment state
 */
enum class AMPPaymentState {
    PENDING,        // Payment initiated, paths being created
    IN_FLIGHT,      // HTLCs sent on all paths
    SUCCEEDED,      // All paths succeeded
    FAILED,         // At least one path failed
    CANCELLED       // Payment cancelled by user
};

/**
 * Individual AMP path
 * Represents one of multiple paths that an AMP payment is split across
 */
struct AMPPath {
    Hash256 path_id;                    // Unique path identifier
    Hash256 payment_hash;               // Derived payment hash for this path
    Hash256 preimage;                   // Derived preimage for this path

    uint64_t amount_sat;                // Amount sent on this path
    uint32_t timeout_height;            // HTLC timeout height

    std::vector<lightning::RouteHop> route;  // Route through network

    bool sent;                          // HTLC sent?
    bool completed;                     // Path succeeded?

    std::optional<Hash256> htlc_id;     // HTLC ID if sent
    std::optional<std::string> error;   // Error message if failed

    AMPPath() : amount_sat(0), timeout_height(0), sent(false), completed(false) {}

    std::vector<uint8_t> serialize() const;
    static AMPPath deserialize(const std::vector<uint8_t>& data);
};

/**
 * AMP payment
 * Represents a single payment split across multiple paths
 */
struct AMPPayment {
    Hash256 payment_id;                 // Unique payment identifier
    Hash256 root_secret;                // Root secret for deriving path secrets

    uint64_t total_amount_sat;          // Total payment amount
    uint64_t total_fee_sat;             // Total fees across all paths

    AMPPaymentState state;              // Current payment state

    std::vector<AMPPath> paths;         // All paths for this payment

    uint32_t created_at;                // Block height when created
    uint32_t completed_at;              // Block height when completed

    AMPPayment() : total_amount_sat(0), total_fee_sat(0),
                  state(AMPPaymentState::PENDING),
                  created_at(0), completed_at(0) {}

    std::vector<uint8_t> serialize() const;
    static AMPPayment deserialize(const std::vector<uint8_t>& data);

    // Check if all paths completed successfully
    bool all_paths_succeeded() const;

    // Check if any path failed
    bool any_path_failed() const;

    // Get number of completed paths
    size_t num_completed_paths() const;
};

/**
 * Path splitting strategy
 */
enum class SplitStrategy {
    EQUAL,          // Split equally across all paths
    WEIGHTED,       // Split based on path capacity/reliability
    RANDOM          // Random split (better privacy)
};

/**
 * AMP payment parameters
 */
struct AMPPaymentParams {
    uint64_t total_amount_sat;          // Total amount to send
    size_t num_paths;                   // Number of paths to use (0 = auto)
    SplitStrategy strategy;             // Path splitting strategy
    uint32_t timeout_blocks;            // HTLC timeout (relative)
    double max_fee_percent;             // Maximum fee percentage (0.0 - 1.0)

    AMPPaymentParams()
        : total_amount_sat(0),
          num_paths(0),
          strategy(SplitStrategy::WEIGHTED),
          timeout_blocks(144),
          max_fee_percent(0.01) {}
};

/**
 * AMP payment manager
 * Coordinates atomic multi-path payments
 */
class AMPPaymentManager {
public:
    AMPPaymentManager();
    ~AMPPaymentManager() = default;

    // ========================================================================
    // Payment Initiation
    // ========================================================================

    /**
     * Create an AMP payment
     *
     * This will:
     * 1. Derive path secrets from root secret
     * 2. Find multiple routes to destination
     * 3. Split payment amount across routes
     * 4. Create HTLCs for each path
     *
     * @param destination Destination node pubkey
     * @param params Payment parameters
     * @return Payment ID if successful
     */
    std::optional<Hash256> create_amp_payment(
        const DilithiumPubKey& destination,
        const AMPPaymentParams& params
    );

    /**
     * Send AMP payment
     * Sends HTLCs on all paths
     *
     * @param payment_id Payment ID
     * @return True if all HTLCs sent successfully
     */
    bool send_amp_payment(const Hash256& payment_id);

    /**
     * Cancel AMP payment
     * Cancels all pending paths
     *
     * @param payment_id Payment ID
     * @return True if cancelled successfully
     */
    bool cancel_amp_payment(const Hash256& payment_id);

    // ========================================================================
    // Payment Monitoring
    // ========================================================================

    /**
     * Handle HTLC success on a path
     * Called when a path HTLC is settled
     *
     * @param payment_id Payment ID
     * @param path_id Path ID
     * @param preimage Preimage revealed by destination
     * @return True if handled successfully
     */
    bool handle_path_success(
        const Hash256& payment_id,
        const Hash256& path_id,
        const Hash256& preimage
    );

    /**
     * Handle HTLC failure on a path
     * Called when a path HTLC fails
     *
     * @param payment_id Payment ID
     * @param path_id Path ID
     * @param error Error message
     * @return True if handled successfully
     */
    bool handle_path_failure(
        const Hash256& payment_id,
        const Hash256& path_id,
        const std::string& error
    );

    /**
     * Check if payment is complete
     * A payment is complete when all paths have either succeeded or failed
     *
     * @param payment_id Payment ID
     * @return True if payment is complete
     */
    bool is_payment_complete(const Hash256& payment_id) const;

    /**
     * Wait for payment completion
     * Blocks until payment completes or times out
     *
     * @param payment_id Payment ID
     * @param timeout_seconds Timeout in seconds
     * @return True if payment succeeded, false if failed or timed out
     */
    bool wait_for_completion(
        const Hash256& payment_id,
        uint32_t timeout_seconds
    );

    // ========================================================================
    // Payment Management
    // ========================================================================

    /**
     * Get payment details
     */
    std::optional<AMPPayment> get_payment(const Hash256& payment_id) const;

    /**
     * List all payments
     */
    std::vector<AMPPayment> list_payments() const;

    /**
     * List payments by state
     */
    std::vector<AMPPayment> list_payments_by_state(AMPPaymentState state) const;

    /**
     * Remove completed payment from history
     */
    bool remove_payment(const Hash256& payment_id);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct AMPStats {
        size_t total_payments;
        size_t successful_payments;
        size_t failed_payments;
        size_t pending_payments;
        uint64_t total_volume_sat;
        uint64_t total_fees_sat;
        double average_paths_per_payment;
        double success_rate;
    };

    AMPStats get_stats() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Set default split strategy
     */
    void set_default_strategy(SplitStrategy strategy);

    /**
     * Set maximum paths per payment
     */
    void set_max_paths(size_t max_paths);

    /**
     * Set minimum amount per path
     */
    void set_min_path_amount(uint64_t min_amount_sat);

private:
    // Payment storage
    std::map<Hash256, AMPPayment> payments_;

    // Configuration
    SplitStrategy default_strategy_;
    size_t max_paths_;
    uint64_t min_path_amount_;

    // Current block height
    uint32_t current_height_;

    // Thread safety
    mutable std::mutex mutex_;
    std::condition_variable completion_cv_;

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    /**
     * Generate unique payment ID
     */
    Hash256 generate_payment_id() const;

    /**
     * Generate root secret for payment
     */
    Hash256 generate_root_secret() const;

    /**
     * Derive path secret from root secret and path index
     * Uses SHA3-256(root_secret || path_index)
     */
    Hash256 derive_path_secret(const Hash256& root_secret, size_t path_index) const;

    /**
     * Derive path preimage from path secret
     */
    Hash256 derive_path_preimage(const Hash256& path_secret) const;

    /**
     * Compute payment hash from preimage
     */
    Hash256 compute_payment_hash(const Hash256& preimage) const;

    /**
     * Find multiple routes to destination
     * Returns up to max_paths different routes
     */
    std::vector<std::vector<lightning::RouteHop>> find_multiple_routes(
        const DilithiumPubKey& destination,
        size_t max_paths,
        uint64_t total_amount_sat
    ) const;

    /**
     * Split payment amount across paths
     * Uses the configured strategy to determine amounts
     */
    std::vector<uint64_t> split_payment_amount(
        uint64_t total_amount_sat,
        const std::vector<std::vector<lightning::RouteHop>>& routes,
        SplitStrategy strategy
    ) const;

    /**
     * Verify all path preimages are valid
     */
    bool verify_all_preimages(const AMPPayment& payment) const;

    /**
     * Mark payment as complete
     */
    void finalize_payment(const Hash256& payment_id);

    /**
     * Cleanup failed paths
     * Attempts to reclaim HTLCs from failed paths
     */
    void cleanup_failed_paths(const Hash256& payment_id);
};

/**
 * AMP-aware invoice
 * Invoice that supports atomic multi-path payments
 */
struct AMPInvoice {
    Hash256 payment_id;                 // Payment identifier
    DilithiumPubKey destination;        // Destination node
    uint64_t amount_sat;                // Total amount
    std::string description;            // Payment description
    uint64_t expiry_timestamp;          // Expiry time

    // AMP-specific fields
    bool amp_required;                  // Must use AMP?
    size_t min_paths;                   // Minimum number of paths
    size_t max_paths;                   // Maximum number of paths

    std::vector<uint8_t> serialize() const;
    static AMPInvoice deserialize(const std::vector<uint8_t>& data);

    // Encode invoice to string (Bech32)
    std::string encode() const;

    // Decode invoice from string
    static std::optional<AMPInvoice> decode(const std::string& encoded);
};

} // namespace amp
} // namespace intcoin

#endif // INTCOIN_AMP_H
