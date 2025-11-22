// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_EXCHANGE_API_H
#define INTCOIN_EXCHANGE_API_H

#include "primitives.h"
#include "transaction.h"
#include "wallet.h"
#include "blockchain.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
#include <chrono>
#include <mutex>

namespace intcoin {
namespace exchange {

/**
 * Deposit status
 */
enum class DepositStatus {
    PENDING,            // Waiting for confirmations
    CONFIRMED,          // Confirmed and credited
    COMPLETED,          // Fully processed
    FAILED              // Failed (invalid tx, etc)
};

/**
 * Withdrawal status
 */
enum class WithdrawalStatus {
    PENDING,            // Queued for processing
    PROCESSING,         // Being processed
    BROADCAST,          // Transaction broadcast
    CONFIRMED,          // Confirmed on-chain
    COMPLETED,          // Fully processed
    FAILED,             // Failed (insufficient balance, etc)
    CANCELLED           // Cancelled by user/admin
};

/**
 * Deposit information
 */
struct Deposit {
    Hash256 txid;                   // Transaction ID
    std::string deposit_address;    // User's deposit address
    std::string user_id;            // User identifier
    uint64_t amount;                // Amount deposited
    uint32_t confirmations;         // Current confirmations
    uint32_t required_confirmations;// Required confirmations
    DepositStatus status;           // Current status
    uint64_t received_at;           // Timestamp received
    uint64_t confirmed_at;          // Timestamp confirmed
    uint64_t block_height;          // Block height included

    Deposit()
        : amount(0)
        , confirmations(0)
        , required_confirmations(6)
        , status(DepositStatus::PENDING)
        , received_at(0)
        , confirmed_at(0)
        , block_height(0)
    {}
};

/**
 * Withdrawal information
 */
struct Withdrawal {
    Hash256 withdrawal_id;          // Unique withdrawal ID
    std::string user_id;            // User identifier
    std::string destination_address;// Destination address
    uint64_t amount;                // Amount to withdraw
    uint64_t fee;                   // Network fee
    WithdrawalStatus status;        // Current status
    Hash256 txid;                   // Transaction ID (if broadcast)
    uint32_t confirmations;         // Current confirmations
    uint64_t requested_at;          // Timestamp requested
    uint64_t processed_at;          // Timestamp processed
    uint64_t completed_at;          // Timestamp completed
    std::string notes;              // Admin notes

    Withdrawal()
        : amount(0)
        , fee(0)
        , status(WithdrawalStatus::PENDING)
        , confirmations(0)
        , requested_at(0)
        , processed_at(0)
        , completed_at(0)
    {}
};

/**
 * User balance tracking
 */
struct UserBalance {
    std::string user_id;
    uint64_t available_balance;     // Available for withdrawal
    uint64_t pending_balance;       // Pending deposits
    uint64_t locked_balance;        // Locked (in orders, etc)
    uint64_t total_deposited;       // Lifetime deposits
    uint64_t total_withdrawn;       // Lifetime withdrawals
    uint64_t last_updated;          // Last update timestamp

    UserBalance()
        : available_balance(0)
        , pending_balance(0)
        , locked_balance(0)
        , total_deposited(0)
        , total_withdrawn(0)
        , last_updated(0)
    {}

    uint64_t get_total_balance() const {
        return available_balance + pending_balance + locked_balance;
    }
};

/**
 * Wallet type for separation
 */
enum class WalletType {
    HOT,                // Hot wallet (online, for daily operations)
    WARM,               // Warm wallet (semi-online)
    COLD                // Cold wallet (offline, for long-term storage)
};

/**
 * Wallet segregation for security
 */
struct ExchangeWallet {
    WalletType type;
    std::string wallet_id;
    std::vector<std::string> addresses;
    uint64_t balance;
    uint64_t min_threshold;         // Minimum balance before refill
    uint64_t max_threshold;         // Maximum balance before sweep
    bool is_active;

    ExchangeWallet()
        : type(WalletType::HOT)
        , balance(0)
        , min_threshold(1000000)    // 1M sats
        , max_threshold(100000000)  // 100M sats
        , is_active(true)
    {}
};

/**
 * Withdrawal batch for efficient processing
 */
struct WithdrawalBatch {
    Hash256 batch_id;
    std::vector<Hash256> withdrawal_ids;
    Transaction batch_tx;
    uint64_t total_amount;
    uint64_t total_fee;
    uint64_t created_at;
    bool is_broadcast;

    WithdrawalBatch()
        : total_amount(0)
        , total_fee(0)
        , created_at(0)
        , is_broadcast(false)
    {}
};

/**
 * Rate limit configuration
 */
struct RateLimit {
    uint32_t max_deposits_per_hour;
    uint32_t max_withdrawals_per_hour;
    uint64_t max_withdrawal_amount_per_hour;
    uint64_t max_withdrawal_amount_per_day;
    uint32_t min_withdrawal_amount;
    uint32_t max_withdrawal_amount;

    RateLimit()
        : max_deposits_per_hour(1000)
        , max_withdrawals_per_hour(100)
        , max_withdrawal_amount_per_hour(100000000000)  // 1M INT
        , max_withdrawal_amount_per_day(1000000000000)  // 10M INT
        , min_withdrawal_amount(100000)                  // 100K sats
        , max_withdrawal_amount(10000000000)             // 100 INT
    {}
};

/**
 * Exchange API Manager
 *
 * Comprehensive exchange integration with:
 * - Automated deposit tracking
 * - Batched withdrawal processing
 * - Hot/cold wallet management
 * - Rate limiting
 * - Audit logging
 * - Multi-signature support
 */
class ExchangeAPIManager {
public:
    ExchangeAPIManager(Blockchain& blockchain, HDWallet& wallet);
    ~ExchangeAPIManager();

    // ========================================================================
    // Deposit Management
    // ========================================================================

    /**
     * Generate unique deposit address for user
     */
    std::optional<std::string> generate_deposit_address(const std::string& user_id);

    /**
     * Get deposit address for user
     */
    std::optional<std::string> get_deposit_address(const std::string& user_id) const;

    /**
     * Track new deposit
     */
    bool track_deposit(const Hash256& txid, const std::string& user_id);

    /**
     * Update deposit confirmations
     */
    bool update_deposit_confirmations(const Hash256& txid, uint32_t confirmations);

    /**
     * Get deposit information
     */
    std::optional<Deposit> get_deposit(const Hash256& txid) const;

    /**
     * List user deposits
     */
    std::vector<Deposit> get_user_deposits(
        const std::string& user_id,
        DepositStatus status = DepositStatus::CONFIRMED
    ) const;

    /**
     * List all pending deposits
     */
    std::vector<Deposit> get_pending_deposits() const;

    // ========================================================================
    // Withdrawal Management
    // ========================================================================

    /**
     * Request withdrawal
     */
    std::optional<Hash256> request_withdrawal(
        const std::string& user_id,
        const std::string& destination_address,
        uint64_t amount
    );

    /**
     * Process pending withdrawal
     */
    bool process_withdrawal(const Hash256& withdrawal_id);

    /**
     * Cancel withdrawal (admin only)
     */
    bool cancel_withdrawal(const Hash256& withdrawal_id, const std::string& reason);

    /**
     * Get withdrawal information
     */
    std::optional<Withdrawal> get_withdrawal(const Hash256& withdrawal_id) const;

    /**
     * List user withdrawals
     */
    std::vector<Withdrawal> get_user_withdrawals(
        const std::string& user_id,
        WithdrawalStatus status = WithdrawalStatus::COMPLETED
    ) const;

    /**
     * List all pending withdrawals
     */
    std::vector<Withdrawal> get_pending_withdrawals() const;

    // ========================================================================
    // Batched Withdrawals (more efficient)
    // ========================================================================

    /**
     * Create withdrawal batch from pending withdrawals
     */
    std::optional<Hash256> create_withdrawal_batch(
        const std::vector<Hash256>& withdrawal_ids
    );

    /**
     * Process withdrawal batch
     */
    bool process_withdrawal_batch(const Hash256& batch_id);

    /**
     * Get batch information
     */
    std::optional<WithdrawalBatch> get_withdrawal_batch(const Hash256& batch_id) const;

    // ========================================================================
    // Balance Management
    // ========================================================================

    /**
     * Get user balance
     */
    std::optional<UserBalance> get_user_balance(const std::string& user_id) const;

    /**
     * Credit user balance (internal)
     */
    bool credit_user(const std::string& user_id, uint64_t amount);

    /**
     * Debit user balance (internal)
     */
    bool debit_user(const std::string& user_id, uint64_t amount);

    /**
     * Lock user balance (for orders, etc)
     */
    bool lock_balance(const std::string& user_id, uint64_t amount);

    /**
     * Unlock user balance
     */
    bool unlock_balance(const std::string& user_id, uint64_t amount);

    /**
     * Get total exchange balance
     */
    uint64_t get_total_exchange_balance() const;

    // ========================================================================
    // Wallet Segregation (Hot/Warm/Cold)
    // ========================================================================

    /**
     * Create segregated wallet
     */
    bool create_wallet(WalletType type, const std::string& wallet_id);

    /**
     * Get wallet information
     */
    std::optional<ExchangeWallet> get_wallet(const std::string& wallet_id) const;

    /**
     * Transfer between wallets (hot -> cold, etc)
     */
    bool transfer_between_wallets(
        const std::string& from_wallet_id,
        const std::string& to_wallet_id,
        uint64_t amount
    );

    /**
     * Sweep hot wallet to cold storage
     */
    bool sweep_hot_wallet_to_cold();

    /**
     * Refill hot wallet from warm/cold
     */
    bool refill_hot_wallet(uint64_t target_balance);

    // ========================================================================
    // Rate Limiting & Security
    // ========================================================================

    /**
     * Set rate limits
     */
    void set_rate_limits(const RateLimit& limits);

    /**
     * Check if withdrawal is within rate limits
     */
    bool check_withdrawal_rate_limit(const std::string& user_id, uint64_t amount) const;

    /**
     * Get user's withdrawal history (for rate limiting)
     */
    struct WithdrawalHistory {
        uint32_t withdrawals_last_hour;
        uint32_t withdrawals_last_day;
        uint64_t amount_last_hour;
        uint64_t amount_last_day;
    };

    WithdrawalHistory get_withdrawal_history(const std::string& user_id) const;

    // ========================================================================
    // Audit Logging
    // ========================================================================

    /**
     * Audit log entry
     */
    struct AuditEntry {
        uint64_t timestamp;
        std::string user_id;
        std::string action;         // "DEPOSIT", "WITHDRAWAL", "CREDIT", "DEBIT", etc
        uint64_t amount;
        Hash256 txid;
        std::string details;
        std::string ip_address;

        AuditEntry() : timestamp(0), amount(0) {}
    };

    /**
     * Get audit log
     */
    std::vector<AuditEntry> get_audit_log(
        const std::string& user_id = "",
        uint64_t start_time = 0,
        uint64_t end_time = 0
    ) const;

    /**
     * Export audit log (CSV)
     */
    std::string export_audit_log_csv(uint64_t start_time, uint64_t end_time) const;

    // ========================================================================
    // Statistics & Reporting
    // ========================================================================

    struct ExchangeStats {
        uint64_t total_users;
        uint64_t total_deposits;
        uint64_t total_withdrawals;
        uint64_t total_deposit_amount;
        uint64_t total_withdrawal_amount;
        uint64_t hot_wallet_balance;
        uint64_t cold_wallet_balance;
        uint32_t pending_deposits;
        uint32_t pending_withdrawals;
        double avg_deposit_confirmation_time;
        double avg_withdrawal_processing_time;
    };

    ExchangeStats get_stats() const;

    /**
     * Get daily volume report
     */
    struct DailyVolume {
        uint64_t date;              // Unix timestamp (start of day)
        uint32_t deposit_count;
        uint32_t withdrawal_count;
        uint64_t deposit_volume;
        uint64_t withdrawal_volume;
        uint64_t net_flow;          // deposits - withdrawals
    };

    std::vector<DailyVolume> get_daily_volume(uint32_t days) const;

    // ========================================================================
    // Webhooks & Notifications
    // ========================================================================

    /**
     * Webhook callback types
     */
    using DepositCallback = std::function<void(const Deposit&)>;
    using WithdrawalCallback = std::function<void(const Withdrawal&)>;
    using BalanceCallback = std::function<void(const std::string&, uint64_t)>;

    /**
     * Register callbacks for notifications
     */
    void on_deposit_confirmed(DepositCallback callback);
    void on_withdrawal_completed(WithdrawalCallback callback);
    void on_balance_low(BalanceCallback callback);

    // ========================================================================
    // Multi-Signature Support
    // ========================================================================

    /**
     * Create multi-sig withdrawal (requires multiple approvals)
     */
    std::optional<Hash256> create_multisig_withdrawal(
        const std::string& user_id,
        const std::string& destination_address,
        uint64_t amount,
        uint32_t required_signatures
    );

    /**
     * Approve multi-sig withdrawal
     */
    bool approve_multisig_withdrawal(
        const Hash256& withdrawal_id,
        const std::string& approver_id,
        const DilithiumSignature& signature
    );

    /**
     * Check if multi-sig withdrawal is fully signed
     */
    bool is_multisig_withdrawal_approved(const Hash256& withdrawal_id) const;

    // ========================================================================
    // Maintenance & Admin
    // ========================================================================

    /**
     * Reconcile balances (compare database vs blockchain)
     */
    struct ReconciliationReport {
        uint64_t database_total;
        uint64_t blockchain_total;
        int64_t difference;
        std::vector<std::string> discrepancies;
        bool is_balanced;
    };

    ReconciliationReport reconcile_balances();

    /**
     * Export user balances (CSV)
     */
    std::string export_user_balances_csv() const;

    /**
     * Import user balances (CSV)
     */
    bool import_user_balances_csv(const std::string& csv_data);

    /**
     * Backup exchange data
     */
    std::vector<uint8_t> backup_exchange_data() const;

    /**
     * Restore exchange data
     */
    bool restore_exchange_data(const std::vector<uint8_t>& backup_data);

private:
    Blockchain& blockchain_;
    HDWallet& wallet_;

    // Internal state
    std::unordered_map<std::string, std::string> user_deposit_addresses_;
    std::unordered_map<Hash256, Deposit> deposits_;
    std::unordered_map<Hash256, Withdrawal> withdrawals_;
    std::unordered_map<std::string, UserBalance> user_balances_;
    std::unordered_map<std::string, ExchangeWallet> wallets_;
    std::unordered_map<Hash256, WithdrawalBatch> withdrawal_batches_;
    std::vector<AuditEntry> audit_log_;

    mutable std::mutex state_mutex_;

    // Configuration
    RateLimit rate_limits_;
    uint32_t required_confirmations_;

    // Callbacks
    DepositCallback deposit_callback_;
    WithdrawalCallback withdrawal_callback_;
    BalanceCallback balance_callback_;

    // Helper functions
    Hash256 generate_withdrawal_id() const;
    Hash256 generate_batch_id() const;
    bool validate_address(const std::string& address) const;
    bool validate_withdrawal_amount(uint64_t amount) const;
    uint64_t calculate_withdrawal_fee(uint64_t amount) const;
    void log_audit(const AuditEntry& entry);
    void trigger_deposit_callback(const Deposit& deposit);
    void trigger_withdrawal_callback(const Withdrawal& withdrawal);
};

/**
 * Exchange API configuration
 */
struct ExchangeAPIConfig {
    uint32_t required_confirmations;        // Required confirmations for deposits
    bool enable_batched_withdrawals;        // Enable withdrawal batching
    uint32_t batch_size;                    // Max withdrawals per batch
    uint32_t batch_interval_seconds;        // Time between batches
    bool enable_hot_cold_segregation;       // Enable wallet segregation
    uint64_t hot_wallet_max_balance;        // Max hot wallet balance
    uint64_t cold_wallet_min_balance;       // Min cold wallet balance
    bool enable_multisig;                   // Enable multi-sig withdrawals
    uint32_t multisig_threshold;            // Required signatures
    bool enable_rate_limiting;              // Enable rate limiting
    bool enable_audit_logging;              // Enable comprehensive audit logs

    ExchangeAPIConfig()
        : required_confirmations(6)
        , enable_batched_withdrawals(true)
        , batch_size(100)
        , batch_interval_seconds(300)       // 5 minutes
        , enable_hot_cold_segregation(true)
        , hot_wallet_max_balance(100000000000)  // 1M INT
        , cold_wallet_min_balance(1000000000000) // 10M INT
        , enable_multisig(true)
        , multisig_threshold(2)             // 2-of-3
        , enable_rate_limiting(true)
        , enable_audit_logging(true)
    {}
};

} // namespace exchange
} // namespace intcoin

#endif // INTCOIN_EXCHANGE_API_H
