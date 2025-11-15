// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Submarine Swap Implementation
// Seamless on-chain ↔ off-chain atomic swaps using HTLCs
//
// Submarine swaps allow trustless conversion between on-chain and Lightning
// payments using hash-locked transactions on both layers.

#ifndef INTCOIN_SUBMARINE_SWAP_H
#define INTCOIN_SUBMARINE_SWAP_H

#include "primitives.h"
#include "transaction.h"
#include "lightning.h"
#include "crypto.h"
#include <vector>
#include <memory>
#include <optional>
#include <chrono>

namespace intcoin {
namespace submarine {

/**
 * Submarine swap version
 */
static constexpr uint32_t SUBMARINE_SWAP_VERSION = 1;

/**
 * Default swap timeouts (in blocks)
 */
static constexpr uint32_t DEFAULT_SWAP_TIMEOUT = 144;      // ~12 hours
static constexpr uint32_t MIN_SWAP_TIMEOUT = 24;           // ~2 hours
static constexpr uint32_t MAX_SWAP_TIMEOUT = 576;          // ~48 hours

/**
 * Swap direction
 */
enum class SwapDirection {
    ON_TO_OFF,   // On-chain → Lightning (Regular submarine swap)
    OFF_TO_ON    // Lightning → On-chain (Reverse submarine swap)
};

/**
 * Swap state
 */
enum class SwapState {
    PENDING,        // Swap initiated, waiting for funding
    FUNDED,         // On-chain tx confirmed or Lightning payment pending
    REDEEMED,       // Swap completed successfully
    REFUNDED,       // Swap timed out and refunded
    FAILED          // Swap failed
};

/**
 * Submarine swap details
 */
struct SubmarineSwap {
    Hash256 swap_id;                    // Unique swap identifier
    Hash256 payment_hash;               // SHA3-256 hash of preimage
    Hash256 preimage;                   // Preimage (revealed upon redemption)

    SwapDirection direction;            // Swap direction
    SwapState state;                    // Current swap state

    uint64_t amount_sat;                // Swap amount in satoshis
    uint64_t fee_sat;                   // Swap fee in satoshis

    uint32_t timeout_height;            // Absolute block height for timeout
    uint32_t created_at;                // Block height when created

    // On-chain details
    Transaction funding_tx;              // On-chain funding transaction
    Address claim_address;               // Address to claim funds
    Address refund_address;              // Address for refund

    // Lightning details (for off-chain side)
    std::optional<Hash256> channel_id;   // Lightning channel ID
    std::optional<uint64_t> htlc_id;     // HTLC ID in channel

    SubmarineSwap() : swap_id{}, payment_hash{}, preimage{},
                     direction(SwapDirection::ON_TO_OFF),
                     state(SwapState::PENDING),
                     amount_sat(0), fee_sat(0),
                     timeout_height(0), created_at(0) {}

    std::vector<uint8_t> serialize() const;
    static SubmarineSwap deserialize(const std::vector<uint8_t>& data);
};

/**
 * Submarine swap quote
 * Provided by swap service before initiating swap
 */
struct SwapQuote {
    uint64_t amount_sat;                // Requested amount
    uint64_t service_fee_sat;           // Service fee
    uint64_t network_fee_sat;           // On-chain network fee estimate
    uint64_t total_cost_sat;            // Total cost including fees

    uint32_t timeout_blocks;            // Timeout in blocks
    double exchange_rate;               // Exchange rate (if applicable)

    uint64_t expires_at;                // Quote expiry timestamp

    std::vector<uint8_t> serialize() const;
    static SwapQuote deserialize(const std::vector<uint8_t>& data);
};

/**
 * Submarine swap manager
 * Handles both regular and reverse submarine swaps
 */
class SubmarineSwapManager {
public:
    SubmarineSwapManager();
    ~SubmarineSwapManager() = default;

    // ========================================================================
    // Regular Submarine Swap (On-chain → Lightning)
    // ========================================================================

    /**
     * Initiate on-chain to Lightning swap
     *
     * Process:
     * 1. Generate preimage and payment hash
     * 2. Create on-chain HTLC funding transaction
     * 3. Wait for confirmation
     * 4. Create Lightning payment with same payment hash
     * 5. Claim Lightning payment reveals preimage
     * 6. Use preimage to claim on-chain funds
     *
     * @param amount_sat Amount to swap
     * @param lightning_invoice Lightning invoice to pay
     * @param refund_address Address for refund if swap fails
     * @param timeout_blocks Timeout in blocks (default: 144)
     * @return Swap details
     */
    std::optional<SubmarineSwap> initiate_on_to_off_swap(
        uint64_t amount_sat,
        const std::string& lightning_invoice,
        const Address& refund_address,
        uint32_t timeout_blocks = DEFAULT_SWAP_TIMEOUT
    );

    /**
     * Fund an on-chain → Lightning swap
     * Broadcasts the funding transaction to the network
     */
    bool fund_swap(const Hash256& swap_id,
                   const Transaction& funding_tx);

    /**
     * Claim Lightning payment (reveals preimage)
     * After this, the preimage can be used to claim on-chain funds
     */
    bool claim_lightning_payment(const Hash256& swap_id,
                                  const Hash256& preimage);

    // ========================================================================
    // Reverse Submarine Swap (Lightning → On-chain)
    // ========================================================================

    /**
     * Initiate Lightning to on-chain swap
     *
     * Process:
     * 1. Generate preimage and payment hash
     * 2. Create Lightning payment with payment hash
     * 3. Service creates on-chain HTLC with same hash
     * 4. Claim on-chain HTLC reveals preimage
     * 5. Service uses preimage to claim Lightning payment
     *
     * @param amount_sat Amount to swap
     * @param claim_address On-chain address to receive funds
     * @param timeout_blocks Timeout in blocks
     * @return Swap details
     */
    std::optional<SubmarineSwap> initiate_off_to_on_swap(
        uint64_t amount_sat,
        const Address& claim_address,
        uint32_t timeout_blocks = DEFAULT_SWAP_TIMEOUT
    );

    /**
     * Create Lightning payment for reverse swap
     * Sends Lightning payment to swap service
     */
    bool create_lightning_payment(const Hash256& swap_id,
                                   const Hash256& channel_id,
                                   uint64_t htlc_id);

    /**
     * Claim on-chain funds (reveals preimage)
     * After claiming, service can claim Lightning payment
     */
    bool claim_onchain_funds(const Hash256& swap_id,
                            const Hash256& preimage);

    // ========================================================================
    // Swap Management
    // ========================================================================

    /**
     * Get swap details
     */
    std::optional<SubmarineSwap> get_swap(const Hash256& swap_id) const;

    /**
     * List all swaps
     */
    std::vector<SubmarineSwap> list_swaps() const;

    /**
     * List swaps by state
     */
    std::vector<SubmarineSwap> list_swaps_by_state(SwapState state) const;

    /**
     * Refund a timed-out swap
     * Can only be called after timeout height is reached
     */
    bool refund_swap(const Hash256& swap_id);

    /**
     * Monitor swaps for timeouts and auto-refund
     * Should be called periodically (e.g., on new block)
     */
    void monitor_swaps(uint32_t current_block_height);

    // ========================================================================
    // Fee Estimation
    // ========================================================================

    /**
     * Get quote for a swap
     */
    SwapQuote get_swap_quote(SwapDirection direction,
                            uint64_t amount_sat) const;

    /**
     * Calculate service fee
     */
    uint64_t calculate_service_fee(uint64_t amount_sat) const;

    /**
     * Set fee parameters
     */
    void set_fee_params(double base_fee_pct,
                       uint64_t min_fee_sat,
                       uint64_t max_fee_sat);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct SwapStats {
        size_t total_swaps;
        size_t successful_swaps;
        size_t failed_swaps;
        size_t pending_swaps;
        uint64_t total_volume_sat;
        uint64_t total_fees_sat;
    };

    SwapStats get_stats() const;

private:
    // Swap storage
    std::map<Hash256, SubmarineSwap> swaps_;

    // Fee parameters
    double base_fee_pct_;          // Base fee percentage (e.g., 0.01 = 1%)
    uint64_t min_fee_sat_;         // Minimum fee
    uint64_t max_fee_sat_;         // Maximum fee

    // Current block height (updated externally)
    uint32_t current_height_;

    // Mutex for thread safety
    mutable std::mutex mutex_;

    // Helper methods
    Hash256 generate_swap_id() const;
    Hash256 generate_preimage() const;
    Hash256 compute_payment_hash(const Hash256& preimage) const;

    Transaction create_htlc_funding_tx(const SubmarineSwap& swap) const;
    Transaction create_htlc_claim_tx(const SubmarineSwap& swap,
                                     const Hash256& preimage) const;
    Transaction create_htlc_refund_tx(const SubmarineSwap& swap) const;

    bool verify_preimage(const Hash256& payment_hash,
                        const Hash256& preimage) const;

    void update_swap_state(const Hash256& swap_id, SwapState new_state);
};

/**
 * Submarine swap service
 * For running a swap service that facilitates swaps for users
 */
class SubmarineSwapService {
public:
    SubmarineSwapService(uint16_t listen_port);
    ~SubmarineSwapService() = default;

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Handle swap requests
    std::optional<SwapQuote> handle_quote_request(SwapDirection direction,
                                                   uint64_t amount_sat);

    bool handle_swap_request(const SubmarineSwap& swap);

    // Statistics
    struct ServiceStats {
        size_t active_swaps;
        size_t completed_swaps;
        uint64_t total_volume_sat;
        uint64_t total_fees_earned_sat;
    };

    ServiceStats get_stats() const;

private:
    uint16_t listen_port_;
    bool running_;

    SubmarineSwapManager swap_manager_;

    mutable std::mutex mutex_;
};

} // namespace submarine
} // namespace intcoin

#endif // INTCOIN_SUBMARINE_SWAP_H
