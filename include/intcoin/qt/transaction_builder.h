// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_TRANSACTION_BUILDER_H
#define INTCOIN_QT_TRANSACTION_BUILDER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace qt {

// Forward declarations
struct CoinEntry;
struct CoinControlSettings;

/**
 * Transaction recipient
 */
struct Recipient {
    std::string address;
    uint64_t amount{0};              // Amount in ints
    std::string label;
    std::string message;
};

/**
 * Transaction options
 */
struct TransactionOptions {
    bool enable_rbf{true};           // Enable Replace-By-Fee
    bool subtract_fee_from_amount{false};
    uint64_t custom_fee_rate{0};     // Custom fee rate (0 = auto)
    uint32_t locktime{0};            // Transaction locktime
    bool use_coin_control{false};    // Use manual coin selection
};

/**
 * Transaction preview
 */
struct TransactionPreview {
    std::vector<Recipient> recipients;
    std::vector<CoinEntry> inputs;
    uint64_t total_input{0};
    uint64_t total_output{0};
    uint64_t fee{0};
    uint64_t change{0};
    std::string change_address;
    size_t transaction_size{0};      // Virtual size in bytes
    double fee_rate{0.0};            // Ints per byte
    uint32_t estimated_confirmations{0};
    std::string raw_hex;
    std::string error_message;
};

/**
 * Batch transaction template
 */
struct BatchTemplate {
    std::string template_id;
    std::string name;
    std::vector<Recipient> recipients;
    TransactionOptions options;
    uint64_t created_at{0};
};

/**
 * Transaction Builder
 *
 * Advanced transaction building with coin control, batch payments,
 * Replace-By-Fee, and CSV import support.
 */
class TransactionBuilder {
public:
    TransactionBuilder();
    ~TransactionBuilder();

    /**
     * Add recipient
     *
     * @param recipient Recipient to add
     */
    void AddRecipient(const Recipient& recipient);

    /**
     * Remove recipient
     *
     * @param index Recipient index
     */
    void RemoveRecipient(size_t index);

    /**
     * Clear all recipients
     */
    void ClearRecipients();

    /**
     * Get recipients
     */
    std::vector<Recipient> GetRecipients() const;

    /**
     * Set transaction options
     *
     * @param options Transaction options
     */
    void SetOptions(const TransactionOptions& options);

    /**
     * Get transaction options
     */
    TransactionOptions GetOptions() const;

    /**
     * Set coin control settings
     *
     * @param settings Coin control settings
     */
    void SetCoinControl(const CoinControlSettings& settings);

    /**
     * Select specific coins
     *
     * @param coins Coins to use for inputs
     */
    void SelectCoins(const std::vector<CoinEntry>& coins);

    /**
     * Preview transaction
     *
     * Builds transaction and returns preview without broadcasting
     *
     * @return Transaction preview
     */
    TransactionPreview PreviewTransaction() const;

    /**
     * Build and sign transaction
     *
     * @return Signed transaction hex (empty on error)
     */
    std::string BuildTransaction();

    /**
     * Build, sign, and broadcast transaction
     *
     * @return Transaction ID (empty on error)
     */
    std::string SendTransaction();

    /**
     * Import batch from CSV
     *
     * CSV format: address,amount,label,message
     *
     * @param csv_data CSV file content
     * @return Number of recipients imported
     */
    uint32_t ImportBatchFromCSV(const std::string& csv_data);

    /**
     * Export batch to CSV
     *
     * @return CSV string
     */
    std::string ExportBatchToCSV() const;

    /**
     * Save batch as template
     *
     * @param name Template name
     * @return Template ID
     */
    std::string SaveBatchTemplate(const std::string& name);

    /**
     * Load batch template
     *
     * @param template_id Template ID
     * @return True if loaded successfully
     */
    bool LoadBatchTemplate(const std::string& template_id);

    /**
     * Get saved batch templates
     */
    std::vector<BatchTemplate> GetBatchTemplates() const;

    /**
     * Delete batch template
     *
     * @param template_id Template ID
     * @return True if deleted successfully
     */
    bool DeleteBatchTemplate(const std::string& template_id);

    /**
     * Bump transaction fee (RBF)
     *
     * Increase fee for unconfirmed transaction
     *
     * @param txid Transaction ID to replace
     * @param new_fee_rate New fee rate
     * @return New transaction ID (empty on error)
     */
    std::string BumpFee(const std::string& txid, double new_fee_rate);

    /**
     * Estimate transaction fee
     *
     * @param num_recipients Number of recipients
     * @param num_inputs Number of inputs (0 = auto)
     * @param fee_rate Fee rate (0 = auto)
     * @return Estimated fee in ints
     */
    uint64_t EstimateFee(
        size_t num_recipients,
        size_t num_inputs = 0,
        double fee_rate = 0.0
    ) const;

    /**
     * Estimate confirmation time
     *
     * @param fee_rate Fee rate
     * @return Estimated blocks to confirmation
     */
    uint32_t EstimateConfirmationTime(double fee_rate) const;

    /**
     * Validate transaction
     *
     * @return Error message (empty if valid)
     */
    std::string ValidateTransaction() const;

    /**
     * Get total send amount
     */
    uint64_t GetTotalAmount() const;

    /**
     * Get recipient count
     */
    size_t GetRecipientCount() const;

    /**
     * Reset builder
     */
    void Reset();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_TRANSACTION_BUILDER_H
