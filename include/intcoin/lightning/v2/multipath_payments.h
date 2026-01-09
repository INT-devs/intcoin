// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_LIGHTNING_V2_MULTIPATH_PAYMENTS_H
#define INTCOIN_LIGHTNING_V2_MULTIPATH_PAYMENTS_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <array>

namespace intcoin {
namespace lightning {
namespace v2 {

/**
 * Payment split strategy
 */
enum class SplitStrategy {
    EQUAL_SPLIT,                     // Split equally across routes
    BALANCED_LIQUIDITY,              // Balance channel liquidity
    MINIMIZE_FEES,                   // Minimize total fees
    OPTIMIZE_SUCCESS_RATE,           // Maximize success probability
    CUSTOM                           // User-defined split
};

/**
 * Payment status
 */
enum class PaymentStatus {
    PENDING,
    IN_FLIGHT,
    SUCCEEDED,
    FAILED,
    TIMEOUT,
    PARTIALLY_FAILED
};

/**
 * Route information for payment path
 */
struct PaymentRoute {
    std::string route_id;
    std::vector<std::string> hops;   // Node pubkeys in path
    uint64_t amount_msat{0};         // Amount for this route (milli-ints)
    uint64_t fee_msat{0};            // Fee for this route
    uint32_t cltv_delta{0};          // CLTV delta
    double success_probability{0.0}; // Estimated success rate (0.0-1.0)
    PaymentStatus status{PaymentStatus::PENDING};  // Route status
    std::string payment_hash;        // Payment hash for this part (AMP uses unique hashes)
    std::string preimage_secret;     // Payment preimage secret
};

/**
 * Multi-path payment configuration
 */
struct MPPConfig {
    uint32_t max_paths{8};           // Maximum number of parallel paths
    uint64_t min_split_amount{1000}; // Minimum amount per split (msat)
    SplitStrategy strategy{SplitStrategy::OPTIMIZE_SUCCESS_RATE};
    bool enable_amp{true};           // Enable AMP (Atomic Multi-Path)
    uint32_t payment_timeout_seconds{60};
    double min_success_probability{0.5};
};

/**
 * Multi-path payment state
 */
struct MPPayment {
    std::string payment_id;
    std::string payment_hash;
    std::string destination;            // Destination node pubkey
    uint64_t total_amount_msat{0};
    uint64_t total_fee_msat{0};
    std::vector<PaymentRoute> routes;
    PaymentStatus status{PaymentStatus::PENDING};
    uint32_t successful_parts{0};
    uint32_t failed_parts{0};
    uint64_t created_at{0};
    uint64_t completed_at{0};
    std::string error_message;
    bool is_amp{false};                 // True if AMP payment
    std::array<uint8_t, 32> amp_root_secret;  // Root secret for AMP
};

/**
 * Payment split result
 */
struct SplitResult {
    std::vector<PaymentRoute> routes;
    uint64_t total_amount_msat{0};
    uint64_t total_fee_msat{0};
    double estimated_success_rate{0.0};
    std::string error_message;
};

/**
 * Multi-Path Payment Manager
 *
 * Implements MPP (Multi-Path Payments) and AMP (Atomic Multi-Path Payments)
 * for splitting large payments across multiple Lightning routes.
 */
class MultiPathPaymentManager {
public:
    MultiPathPaymentManager();
    explicit MultiPathPaymentManager(const MPPConfig& config);
    ~MultiPathPaymentManager();

    /**
     * Send multi-path payment
     *
     * @param destination Destination node pubkey
     * @param amount_msat Total amount in milli-ints
     * @param payment_hash Payment hash
     * @param max_fee_msat Maximum acceptable fee
     * @return Payment ID
     */
    std::string SendPayment(
        const std::string& destination,
        uint64_t amount_msat,
        const std::string& payment_hash,
        uint64_t max_fee_msat = 0
    );

    /**
     * Send AMP payment
     *
     * AMP allows splitting without pre-shared payment hash
     *
     * @param destination Destination node pubkey
     * @param amount_msat Total amount in milli-ints
     * @param max_fee_msat Maximum acceptable fee
     * @return Payment ID
     */
    std::string SendAMPPayment(
        const std::string& destination,
        uint64_t amount_msat,
        uint64_t max_fee_msat = 0
    );

    /**
     * Get payment status
     *
     * @param payment_id Payment ID
     * @return Payment state
     */
    MPPayment GetPayment(const std::string& payment_id) const;

    /**
     * Cancel pending payment
     *
     * @param payment_id Payment ID
     * @return True if cancelled successfully
     */
    bool CancelPayment(const std::string& payment_id);

    /**
     * Split payment into multiple routes
     *
     * @param destination Destination node pubkey
     * @param amount_msat Total amount
     * @param max_fee_msat Maximum fee
     * @return Split result with routes
     */
    SplitResult SplitPayment(
        const std::string& destination,
        uint64_t amount_msat,
        uint64_t max_fee_msat = 0
    ) const;

    /**
     * Find optimal routes for payment
     *
     * @param destination Destination node pubkey
     * @param amount_msat Amount per route
     * @param num_routes Number of routes to find
     * @return Vector of payment routes
     */
    std::vector<PaymentRoute> FindRoutes(
        const std::string& destination,
        uint64_t amount_msat,
        uint32_t num_routes = 1
    ) const;

    /**
     * Calculate optimal payment split
     *
     * @param total_amount Total amount to send
     * @param available_routes Available routes
     * @return Optimal amount distribution
     */
    std::vector<uint64_t> CalculateOptimalSplit(
        uint64_t total_amount,
        const std::vector<PaymentRoute>& available_routes
    ) const;

    /**
     * Retry failed payment parts
     *
     * @param payment_id Payment ID
     * @return True if retry initiated
     */
    bool RetryFailedParts(const std::string& payment_id);

    /**
     * Get all active payments
     */
    std::vector<MPPayment> GetActivePayments() const;

    /**
     * Get payment history
     *
     * @param limit Maximum number of payments
     * @return Vector of completed payments
     */
    std::vector<MPPayment> GetPaymentHistory(uint32_t limit = 100) const;

    /**
     * Set configuration
     *
     * @param config MPP configuration
     */
    void SetConfig(const MPPConfig& config);

    /**
     * Get configuration
     */
    MPPConfig GetConfig() const;

    /**
     * Get statistics
     */
    struct Statistics {
        uint64_t total_payments{0};
        uint64_t successful_payments{0};
        uint64_t failed_payments{0};
        uint64_t mpp_payments{0};       // MPP payments sent
        uint64_t amp_payments{0};       // AMP payments sent
        uint64_t total_amount_msat{0};
        uint64_t total_fees_msat{0};
        double average_success_rate{0.0};
        double average_parts_per_payment{0.0};
    };

    Statistics GetStatistics() const;

    /**
     * Clear payment history
     */
    void ClearHistory();

    /**
     * Enable/disable MPP
     *
     * @param enabled Enable flag
     */
    void SetEnabled(bool enabled);

    /**
     * Check if MPP is enabled
     */
    bool IsEnabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get payment status name
 */
std::string GetPaymentStatusName(PaymentStatus status);

/**
 * Parse payment status from string
 */
PaymentStatus ParsePaymentStatus(const std::string& name);

/**
 * Get split strategy name
 */
std::string GetSplitStrategyName(SplitStrategy strategy);

/**
 * Parse split strategy from string
 */
SplitStrategy ParseSplitStrategy(const std::string& name);

} // namespace v2
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_V2_MULTIPATH_PAYMENTS_H
