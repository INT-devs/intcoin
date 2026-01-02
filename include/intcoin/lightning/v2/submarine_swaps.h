// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_SUBMARINE_SWAPS_H
#define INTCOIN_LIGHTNING_V2_SUBMARINE_SWAPS_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Swap type
 */
enum class SwapType {
    SWAP_IN,                         // On-chain → Lightning
    SWAP_OUT,                        // Lightning → On-chain
    LOOP_IN,                         // Alternative name for SWAP_IN
    LOOP_OUT                         // Alternative name for SWAP_OUT
};

/**
 * Swap status
 */
enum class SwapStatus {
    PENDING,                         // Swap initiated
    INVOICE_GENERATED,               // Invoice created (swap-in)
    LOCKUP_TX_BROADCAST,             // Lockup tx broadcast (swap-out)
    LOCKUP_TX_CONFIRMED,             // Lockup tx confirmed
    CLAIM_TX_BROADCAST,              // Claim tx broadcast
    CLAIM_TX_CONFIRMED,              // Claim confirmed
    COMPLETED,                       // Swap completed
    REFUNDED,                        // Swap refunded (timeout)
    FAILED                           // Swap failed
};

/**
 * Swap parameters
 */
struct SwapParams {
    uint64_t amount{0};              // Amount in satoshis
    uint64_t fee{0};                 // Service fee
    uint64_t onchain_fee{0};         // On-chain miner fee
    uint32_t timeout_blocks{144};    // Timeout (blocks)
    uint32_t confirmation_target{6}; // Target confirmations
    std::string server_pubkey;       // Swap service pubkey
};

/**
 * Submarine swap
 */
struct SubmarineSwap {
    std::string swap_id;
    SwapType type{SwapType::SWAP_IN};
    SwapStatus status{SwapStatus::PENDING};
    uint64_t amount{0};
    uint64_t fee{0};
    std::string payment_hash;
    std::string preimage;
    std::string refund_address;
    std::string claim_address;

    // On-chain details
    std::string lockup_address;
    std::string lockup_txid;
    uint32_t lockup_vout{0};
    std::string claim_txid;
    std::string refund_txid;

    // Lightning details
    std::string invoice;             // Lightning invoice
    std::string payment_request;

    // Timing
    uint64_t created_at{0};
    uint64_t expires_at{0};
    uint64_t completed_at{0};
    uint32_t timeout_height{0};

    std::string error_message;
};

/**
 * Swap quote
 */
struct SwapQuote {
    SwapType type{SwapType::SWAP_IN};
    uint64_t amount{0};
    uint64_t service_fee{0};          // Service fee
    uint64_t onchain_fee{0};          // Miner fee estimate
    uint64_t total_fee{0};            // Total fees
    double fee_percentage{0.0};       // Fee as percentage
    uint32_t timeout_blocks{144};
    uint64_t min_amount{0};
    uint64_t max_amount{0};
    uint64_t valid_until{0};          // Quote expiration timestamp
};

/**
 * Submarine Swap Manager
 *
 * Manages atomic swaps between on-chain and Lightning Network.
 * Implements submarine swaps (loop in/out) for channel liquidity management.
 */
class SubmarineSwapManager {
public:
    struct Config {
        std::string server_url{"https://swap.intcoin.org"};
        std::string server_pubkey;
        uint64_t min_swap_amount{10000};      // Minimum 10k sats
        uint64_t max_swap_amount{100000000};  // Maximum 1 BTC
        uint32_t default_timeout{144};        // ~24 hours
        uint32_t confirmation_target{6};
    };

    SubmarineSwapManager();
    explicit SubmarineSwapManager(const Config& config);
    ~SubmarineSwapManager();

    /**
     * Get swap quote
     *
     * @param type Swap type (in/out)
     * @param amount Amount in satoshis
     * @return Swap quote with fees
     */
    SwapQuote GetQuote(SwapType type, uint64_t amount) const;

    /**
     * Create swap-in (on-chain → Lightning)
     *
     * User sends on-chain funds, receives Lightning payment
     *
     * @param amount Amount in satoshis
     * @param refund_address Refund address for timeout
     * @return Submarine swap details
     */
    SubmarineSwap CreateSwapIn(
        uint64_t amount,
        const std::string& refund_address
    );

    /**
     * Create swap-out (Lightning → on-chain)
     *
     * User sends Lightning payment, receives on-chain funds
     *
     * @param amount Amount in satoshis
     * @param claim_address On-chain address to receive funds
     * @return Submarine swap details
     */
    SubmarineSwap CreateSwapOut(
        uint64_t amount,
        const std::string& claim_address
    );

    /**
     * Complete swap-in
     *
     * Called after on-chain lockup tx is confirmed
     *
     * @param swap_id Swap ID
     * @param lockup_txid On-chain transaction ID
     * @return True if completed successfully
     */
    bool CompleteSwapIn(
        const std::string& swap_id,
        const std::string& lockup_txid
    );

    /**
     * Complete swap-out
     *
     * Pay Lightning invoice to claim on-chain funds
     *
     * @param swap_id Swap ID
     * @return Payment preimage
     */
    std::string CompleteSwapOut(const std::string& swap_id);

    /**
     * Refund expired swap
     *
     * Claim refund after timeout expires
     *
     * @param swap_id Swap ID
     * @return Refund transaction ID
     */
    std::string RefundSwap(const std::string& swap_id);

    /**
     * Get swap
     *
     * @param swap_id Swap ID
     * @return Submarine swap
     */
    SubmarineSwap GetSwap(const std::string& swap_id) const;

    /**
     * Get active swaps
     */
    std::vector<SubmarineSwap> GetActiveSwaps() const;

    /**
     * Get swap history
     *
     * @param limit Maximum number of swaps
     * @return Vector of completed swaps
     */
    std::vector<SubmarineSwap> GetSwapHistory(uint32_t limit = 100) const;

    /**
     * Monitor swap progress
     *
     * Check on-chain confirmations and update status
     *
     * @param swap_id Swap ID
     * @return Updated swap status
     */
    SwapStatus MonitorSwap(const std::string& swap_id);

    /**
     * Cancel pending swap
     *
     * @param swap_id Swap ID
     * @return True if cancelled successfully
     */
    bool CancelSwap(const std::string& swap_id);

    /**
     * Get swap limits
     *
     * @return Min/max swap amounts
     */
    struct SwapLimits {
        uint64_t min_amount{0};
        uint64_t max_amount{0};
    };

    SwapLimits GetSwapLimits(SwapType type) const;

    /**
     * Estimate swap fees
     *
     * @param type Swap type
     * @param amount Amount
     * @return Total estimated fees
     */
    uint64_t EstimateFees(SwapType type, uint64_t amount) const;

    /**
     * Set configuration
     *
     * @param config Swap configuration
     */
    void SetConfig(const Config& config);

    /**
     * Get configuration
     */
    Config GetConfig() const;

    /**
     * Get statistics
     */
    struct Statistics {
        uint64_t total_swaps{0};
        uint64_t completed_swaps{0};
        uint64_t failed_swaps{0};
        uint64_t refunded_swaps{0};
        uint64_t total_swapped_in{0};
        uint64_t total_swapped_out{0};
        uint64_t total_fees_paid{0};
    };

    Statistics GetStatistics() const;

    /**
     * Enable/disable submarine swaps
     *
     * @param enabled Enable flag
     */
    void SetEnabled(bool enabled);

    /**
     * Check if submarine swaps are enabled
     */
    bool IsEnabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get swap type name
 */
std::string GetSwapTypeName(SwapType type);

/**
 * Parse swap type from string
 */
SwapType ParseSwapType(const std::string& name);

/**
 * Get swap status name
 */
std::string GetSwapStatusName(SwapStatus status);

/**
 * Parse swap status from string
 */
SwapStatus ParseSwapStatus(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_SUBMARINE_SWAPS_H
