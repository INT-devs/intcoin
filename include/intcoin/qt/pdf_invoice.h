// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_PDF_INVOICE_H
#define INTCOIN_QT_PDF_INVOICE_H

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Invoice item (line item on invoice)
 */
struct InvoiceItem {
    std::string description;
    double quantity{1.0};
    double unit_price{0.0};  // In INT
    double total{0.0};       // quantity * unit_price

    InvoiceItem() = default;
    InvoiceItem(const std::string& desc, double qty, double price)
        : description(desc), quantity(qty), unit_price(price), total(qty * price) {}
};

/**
 * Invoice data structure
 */
struct InvoiceData {
    // Invoice metadata
    std::string invoice_number;
    uint64_t timestamp{0};        // Unix timestamp
    uint64_t due_date{0};         // Unix timestamp

    // Merchant/sender information
    std::string from_name;
    std::string from_address_line1;
    std::string from_address_line2;
    std::string from_email;
    std::string from_phone;

    // Customer/recipient information
    std::string to_name;
    std::string to_address_line1;
    std::string to_address_line2;
    std::string to_email;

    // Payment information
    std::string payment_address;  // INTcoin address
    std::string payment_uri;      // intcoin: URI with amount

    // Invoice items
    std::vector<InvoiceItem> items;

    // Totals
    double subtotal{0.0};         // In INT
    double tax_rate{0.0};         // Percentage (e.g., 0.10 for 10%)
    double tax_amount{0.0};       // In INT
    double total_amount{0.0};     // In INT

    // Notes
    std::string notes;
    std::string terms;

    /**
     * Calculate totals from items
     */
    void CalculateTotals() {
        subtotal = 0.0;
        for (const auto& item : items) {
            subtotal += item.total;
        }
        tax_amount = subtotal * tax_rate;
        total_amount = subtotal + tax_amount;
    }
};

/**
 * PDF Invoice Generator
 *
 * Generates professional PDF invoices for INTcoin payments using Qt's
 * built-in PDF support (QPdfWriter).
 */
class PDFInvoiceGenerator {
public:
    /**
     * Invoice style/theme
     */
    enum class Style {
        PROFESSIONAL,  // Clean, corporate style
        MODERN,        // Modern, colorful style
        MINIMAL,       // Minimalist black & white
        CLASSIC        // Traditional invoice style
    };

    PDFInvoiceGenerator();
    ~PDFInvoiceGenerator();

    /**
     * Set invoice style
     *
     * @param style Invoice style/theme
     */
    void SetStyle(Style style);

    /**
     * Set company logo
     *
     * @param logo_path Path to logo image (PNG, JPG)
     * @return True if logo loaded successfully
     */
    bool SetLogo(const std::string& logo_path);

    /**
     * Generate PDF invoice
     *
     * @param data Invoice data
     * @param output_path Path to save PDF file
     * @param include_qr_code Include QR code for payment address
     * @return True if PDF generated successfully
     */
    bool Generate(
        const InvoiceData& data,
        const std::string& output_path,
        bool include_qr_code = true
    );

    /**
     * Generate invoice preview (returns PDF as bytes)
     *
     * @param data Invoice data
     * @param include_qr_code Include QR code
     * @return PDF bytes or empty on error
     */
    std::vector<uint8_t> GeneratePreview(
        const InvoiceData& data,
        bool include_qr_code = true
    );

    /**
     * Validate invoice data
     *
     * @param data Invoice data to validate
     * @return Error message or empty string if valid
     */
    static std::string ValidateInvoiceData(const InvoiceData& data);

    /**
     * Generate invoice number
     *
     * Format: INV-YYYYMMDD-XXXX
     *
     * @return Generated invoice number
     */
    static std::string GenerateInvoiceNumber();

    /**
     * Format amount for display
     *
     * @param amount Amount in INT
     * @param include_symbol Include "INT" symbol
     * @return Formatted amount string
     */
    static std::string FormatAmount(double amount, bool include_symbol = true);

    /**
     * Format date for display
     *
     * @param timestamp Unix timestamp
     * @return Formatted date string (YYYY-MM-DD)
     */
    static std::string FormatDate(uint64_t timestamp);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_PDF_INVOICE_H
