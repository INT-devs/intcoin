// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_PAYMENT_REQUEST_H
#define INTCOIN_QT_PAYMENT_REQUEST_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Payment request status
 */
enum class PaymentStatus {
    PENDING,
    PAID,
    EXPIRED,
    CANCELLED
};

/**
 * Line item for invoice
 */
struct LineItem {
    std::string description;
    uint32_t quantity{1};
    uint64_t unit_price{0};          // Price per unit in ints
    uint64_t total_price{0};         // Total = quantity * unit_price
};

/**
 * Payment request (BIP21/BIP70)
 */
struct PaymentRequest {
    std::string request_id;          // Unique ID
    std::string address;             // Payment address
    uint64_t amount{0};              // Amount in ints
    std::string label;               // Label for payment
    std::string message;             // Message to payer
    uint64_t expires_at{0};          // Expiration timestamp
    std::string merchant_name;
    std::string merchant_url;
    std::vector<LineItem> line_items; // Invoice line items
    PaymentStatus status{PaymentStatus::PENDING};
    uint64_t created_at{0};
    uint64_t paid_at{0};
    std::string paid_txid;           // Transaction ID if paid
    std::vector<uint8_t> qr_code;    // QR code image (PNG)
};

/**
 * Payment Request Manager
 *
 * Manages payment requests, invoices, and QR codes.
 * Supports BIP21 URIs and BIP70 payment protocol.
 */
class PaymentRequestManager {
public:
    PaymentRequestManager();
    ~PaymentRequestManager();

    /**
     * Create payment request
     *
     * @param address Payment address
     * @param amount Amount in ints
     * @param label Label text
     * @param message Message to payer
     * @param expires_in_seconds Expiration time (0 = no expiry)
     * @return Request ID
     */
    std::string CreatePaymentRequest(
        const std::string& address,
        uint64_t amount,
        const std::string& label = "",
        const std::string& message = "",
        uint64_t expires_in_seconds = 0
    );

    /**
     * Create invoice with line items
     *
     * @param address Payment address
     * @param line_items Invoice line items
     * @param merchant_name Merchant name
     * @param merchant_url Merchant URL
     * @param expires_in_seconds Expiration time
     * @return Request ID
     */
    std::string CreateInvoice(
        const std::string& address,
        const std::vector<LineItem>& line_items,
        const std::string& merchant_name = "",
        const std::string& merchant_url = "",
        uint64_t expires_in_seconds = 0
    );

    /**
     * Get payment request
     *
     * @param request_id Request ID
     * @return Payment request (empty if not found)
     */
    PaymentRequest GetPaymentRequest(const std::string& request_id) const;

    /**
     * Get all payment requests
     *
     * @return Vector of all requests
     */
    std::vector<PaymentRequest> GetAllPaymentRequests() const;

    /**
     * Get payment requests by status
     *
     * @param status Payment status
     * @return Vector of requests with status
     */
    std::vector<PaymentRequest> GetPaymentRequestsByStatus(PaymentStatus status) const;

    /**
     * Update payment request status
     *
     * @param request_id Request ID
     * @param status New status
     * @return True if updated successfully
     */
    bool UpdateStatus(const std::string& request_id, PaymentStatus status);

    /**
     * Mark payment request as paid
     *
     * @param request_id Request ID
     * @param txid Transaction ID
     * @return True if updated successfully
     */
    bool MarkAsPaid(const std::string& request_id, const std::string& txid);

    /**
     * Cancel payment request
     *
     * @param request_id Request ID
     * @return True if cancelled successfully
     */
    bool CancelPaymentRequest(const std::string& request_id);

    /**
     * Delete payment request
     *
     * @param request_id Request ID
     * @return True if deleted successfully
     */
    bool DeletePaymentRequest(const std::string& request_id);

    /**
     * Generate BIP21 URI
     *
     * @param request Payment request
     * @return BIP21 URI string
     */
    std::string GenerateBIP21URI(const PaymentRequest& request) const;

    /**
     * Parse BIP21 URI
     *
     * @param uri BIP21 URI string
     * @return Payment request
     */
    PaymentRequest ParseBIP21URI(const std::string& uri) const;

    /**
     * Generate QR code for payment request
     *
     * @param request Payment request
     * @param size QR code size in pixels
     * @return QR code image (PNG)
     */
    std::vector<uint8_t> GenerateQRCode(
        const PaymentRequest& request,
        uint32_t size = 256
    ) const;

    /**
     * Expire old payment requests
     *
     * Marks expired requests as EXPIRED
     *
     * @return Number of requests expired
     */
    uint32_t ExpireOldRequests();

    /**
     * Email payment request
     *
     * @param request_id Request ID
     * @param recipient_email Email address
     * @return True if sent successfully
     */
    bool EmailPaymentRequest(
        const std::string& request_id,
        const std::string& recipient_email
    );

    /**
     * Export payment request as PDF invoice
     *
     * @param request_id Request ID
     * @param output_path Path to save PDF
     * @return True if exported successfully
     */
    bool ExportToPDF(
        const std::string& request_id,
        const std::string& output_path
    ) const;

    /**
     * Get total pending amount
     *
     * @return Total amount of pending requests
     */
    uint64_t GetTotalPendingAmount() const;

    /**
     * Get payment request count
     */
    size_t GetRequestCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get status name
 */
std::string GetStatusName(PaymentStatus status);

/**
 * Parse status from string
 */
PaymentStatus ParseStatus(const std::string& name);

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_PAYMENT_REQUEST_H
