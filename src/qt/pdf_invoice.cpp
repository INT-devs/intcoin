// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qt/pdf_invoice.h>
#include <intcoin/qrcode.h>

#include <QPdfWriter>
#include <QPainter>
#include <QFont>
#include <QImage>
#include <QBuffer>
#include <QPixmap>
#include <QRect>
#include <QFontMetrics>
#include <QDateTime>

#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

namespace intcoin {
namespace qt {

/**
 * Implementation class
 */
class PDFInvoiceGenerator::Impl {
public:
    PDFInvoiceGenerator::Style style_{Style::PROFESSIONAL};
    QImage logo_;
    bool has_logo_{false};

    // Page layout constants (in points, 1/72 inch)
    static constexpr int PAGE_WIDTH = 595;   // A4 width (8.27 inches)
    static constexpr int PAGE_HEIGHT = 842;  // A4 height (11.69 inches)
    static constexpr int MARGIN = 50;
    static constexpr int CONTENT_WIDTH = PAGE_WIDTH - 2 * MARGIN;

    bool Generate(const InvoiceData& data, const std::string& output_path, bool include_qr) {
        QPdfWriter pdf_writer(QString::fromStdString(output_path));
        pdf_writer.setPageSize(QPageSize(QPageSize::A4));
        pdf_writer.setPageMargins(QMarginsF(MARGIN, MARGIN, MARGIN, MARGIN), QPageLayout::Point);
        pdf_writer.setTitle("INTcoin Invoice");
        pdf_writer.setCreator("INTcoin Wallet");

        QPainter painter(&pdf_writer);
        if (!painter.isActive()) {
            return false;
        }

        DrawInvoice(painter, data, include_qr);
        return true;
    }

    std::vector<uint8_t> GeneratePreview(const InvoiceData& data, bool include_qr) {
        QByteArray byte_array;
        QBuffer buffer(&byte_array);
        buffer.open(QIODevice::WriteOnly);

        QPdfWriter pdf_writer(&buffer);
        pdf_writer.setPageSize(QPageSize(QPageSize::A4));
        pdf_writer.setPageMargins(QMarginsF(MARGIN, MARGIN, MARGIN, MARGIN), QPageLayout::Point);

        QPainter painter(&pdf_writer);
        if (!painter.isActive()) {
            return {};
        }

        DrawInvoice(painter, data, include_qr);
        painter.end();

        buffer.close();
        return std::vector<uint8_t>(byte_array.begin(), byte_array.end());
    }

private:
    void DrawInvoice(QPainter& painter, const InvoiceData& data, bool include_qr) {
        int y_pos = 0;

        // Header with logo and title
        y_pos = DrawHeader(painter, data, y_pos);
        y_pos += 30;

        // Invoice metadata (number, date)
        y_pos = DrawMetadata(painter, data, y_pos);
        y_pos += 30;

        // Addresses (from/to)
        y_pos = DrawAddresses(painter, data, y_pos);
        y_pos += 40;

        // Line items table
        y_pos = DrawItemsTable(painter, data, y_pos);
        y_pos += 30;

        // Totals
        y_pos = DrawTotals(painter, data, y_pos);
        y_pos += 40;

        // Payment information with QR code
        if (include_qr && !data.payment_address.empty()) {
            y_pos = DrawPaymentInfo(painter, data, y_pos);
            y_pos += 30;
        }

        // Notes and terms
        if (!data.notes.empty() || !data.terms.empty()) {
            DrawFooter(painter, data, y_pos);
        }
    }

    int DrawHeader(QPainter& painter, const InvoiceData& data, int y) {
        // Logo (if available)
        if (has_logo_) {
            QRect logo_rect(0, y, 100, 50);
            painter.drawImage(logo_rect, logo_, logo_.rect());
        }

        // Title "INVOICE"
        QFont title_font("Arial", 28, QFont::Bold);
        painter.setFont(title_font);
        painter.setPen(Qt::black);

        QRect title_rect(CONTENT_WIDTH - 200, y, 200, 50);
        painter.drawText(title_rect, Qt::AlignRight | Qt::AlignVCenter, "INVOICE");

        return y + 50;
    }

    int DrawMetadata(QPainter& painter, const InvoiceData& data, int y) {
        QFont label_font("Arial", 10, QFont::Bold);
        QFont value_font("Arial", 10);

        // Invoice number
        painter.setFont(label_font);
        painter.drawText(0, y, "Invoice Number:");
        painter.setFont(value_font);
        painter.drawText(150, y, QString::fromStdString(data.invoice_number));
        y += 20;

        // Date
        painter.setFont(label_font);
        painter.drawText(0, y, "Date:");
        painter.setFont(value_font);
        painter.drawText(150, y, QString::fromStdString(
            PDFInvoiceGenerator::FormatDate(data.timestamp)));
        y += 20;

        // Due date
        if (data.due_date > 0) {
            painter.setFont(label_font);
            painter.drawText(0, y, "Due Date:");
            painter.setFont(value_font);
            painter.drawText(150, y, QString::fromStdString(
                PDFInvoiceGenerator::FormatDate(data.due_date)));
            y += 20;
        }

        return y;
    }

    int DrawAddresses(QPainter& painter, const InvoiceData& data, int y) {
        QFont label_font("Arial", 10, QFont::Bold);
        QFont value_font("Arial", 10);

        int col_width = CONTENT_WIDTH / 2 - 20;

        // From (left column)
        painter.setFont(label_font);
        painter.drawText(0, y, "From:");
        y += 20;

        painter.setFont(value_font);
        if (!data.from_name.empty()) {
            painter.drawText(0, y, QString::fromStdString(data.from_name));
            y += 15;
        }
        if (!data.from_address_line1.empty()) {
            painter.drawText(0, y, QString::fromStdString(data.from_address_line1));
            y += 15;
        }
        if (!data.from_address_line2.empty()) {
            painter.drawText(0, y, QString::fromStdString(data.from_address_line2));
            y += 15;
        }
        if (!data.from_email.empty()) {
            painter.drawText(0, y, QString::fromStdString(data.from_email));
            y += 15;
        }
        if (!data.from_phone.empty()) {
            painter.drawText(0, y, QString::fromStdString(data.from_phone));
            y += 15;
        }

        // To (right column)
        int to_y = y - (y % 100); // Align to same starting height
        painter.setFont(label_font);
        painter.drawText(col_width + 40, to_y, "Bill To:");
        to_y += 20;

        painter.setFont(value_font);
        if (!data.to_name.empty()) {
            painter.drawText(col_width + 40, to_y, QString::fromStdString(data.to_name));
            to_y += 15;
        }
        if (!data.to_address_line1.empty()) {
            painter.drawText(col_width + 40, to_y, QString::fromStdString(data.to_address_line1));
            to_y += 15;
        }
        if (!data.to_address_line2.empty()) {
            painter.drawText(col_width + 40, to_y, QString::fromStdString(data.to_address_line2));
            to_y += 15;
        }
        if (!data.to_email.empty()) {
            painter.drawText(col_width + 40, to_y, QString::fromStdString(data.to_email));
            to_y += 15;
        }

        return std::max(y, to_y);
    }

    int DrawItemsTable(QPainter& painter, const InvoiceData& data, int y) {
        QFont header_font("Arial", 10, QFont::Bold);
        QFont cell_font("Arial", 10);

        // Table header
        painter.setFont(header_font);
        painter.setPen(Qt::black);
        painter.setBrush(QColor(240, 240, 240));

        QRect header_rect(0, y, CONTENT_WIDTH, 25);
        painter.drawRect(header_rect);

        // Column headers
        painter.drawText(10, y + 18, "Description");
        painter.drawText(CONTENT_WIDTH - 300, y + 18, "Quantity");
        painter.drawText(CONTENT_WIDTH - 200, y + 18, "Unit Price");
        painter.drawText(CONTENT_WIDTH - 100, y + 18, "Total");

        y += 25;

        // Table rows
        painter.setFont(cell_font);
        painter.setBrush(Qt::white);

        for (const auto& item : data.items) {
            painter.drawRect(0, y, CONTENT_WIDTH, 25);

            painter.drawText(10, y + 18, QString::fromStdString(item.description));
            painter.drawText(CONTENT_WIDTH - 300, y + 18, QString::number(item.quantity, 'f', 2));
            painter.drawText(CONTENT_WIDTH - 200, y + 18,
                QString::fromStdString(PDFInvoiceGenerator::FormatAmount(item.unit_price, false)));
            painter.drawText(CONTENT_WIDTH - 100, y + 18,
                QString::fromStdString(PDFInvoiceGenerator::FormatAmount(item.total, false)));

            y += 25;
        }

        return y;
    }

    int DrawTotals(QPainter& painter, const InvoiceData& data, int y) {
        QFont label_font("Arial", 11, QFont::Bold);
        QFont value_font("Arial", 11);
        QFont total_font("Arial", 12, QFont::Bold);

        int label_x = CONTENT_WIDTH - 250;
        int value_x = CONTENT_WIDTH - 100;

        // Subtotal
        painter.setFont(label_font);
        painter.drawText(label_x, y, "Subtotal:");
        painter.setFont(value_font);
        painter.drawText(value_x, y,
            QString::fromStdString(PDFInvoiceGenerator::FormatAmount(data.subtotal)));
        y += 20;

        // Tax
        if (data.tax_rate > 0) {
            painter.setFont(label_font);
            painter.drawText(label_x, y, QString("Tax (%1%):").arg(data.tax_rate * 100, 0, 'f', 1));
            painter.setFont(value_font);
            painter.drawText(value_x, y,
                QString::fromStdString(PDFInvoiceGenerator::FormatAmount(data.tax_amount)));
            y += 20;
        }

        // Draw line
        painter.drawLine(label_x, y, CONTENT_WIDTH, y);
        y += 10;

        // Total
        painter.setFont(total_font);
        painter.drawText(label_x, y, "TOTAL:");
        painter.drawText(value_x, y,
            QString::fromStdString(PDFInvoiceGenerator::FormatAmount(data.total_amount)));

        return y + 20;
    }

    int DrawPaymentInfo(QPainter& painter, const InvoiceData& data, int y) {
        QFont header_font("Arial", 12, QFont::Bold);
        QFont value_font("Arial", 10);

        painter.setFont(header_font);
        painter.drawText(0, y, "Payment Information");
        y += 25;

        // Payment address
        painter.setFont(value_font);
        painter.drawText(0, y, "INTcoin Address:");
        y += 18;

        QFont mono_font("Courier", 9);
        painter.setFont(mono_font);
        painter.drawText(0, y, QString::fromStdString(data.payment_address));
        y += 25;

        // QR Code
        auto qr_data = QRCode::Generate(
            data.payment_uri.empty() ? data.payment_address : data.payment_uri,
            QRCode::ECLevel::HIGH
        );

        if (qr_data) {
            int qr_size = 150;
            int module_size = qr_size / qr_data->width;
            int border = 2;

            QImage qr_image(
                (qr_data->width + 2 * border) * module_size,
                (qr_data->width + 2 * border) * module_size,
                QImage::Format_RGB32
            );
            qr_image.fill(Qt::white);

            QPainter qr_painter(&qr_image);
            qr_painter.setPen(Qt::NoPen);
            qr_painter.setBrush(Qt::black);

            for (int qy = 0; qy < qr_data->width; qy++) {
                for (int qx = 0; qx < qr_data->width; qx++) {
                    if (qr_data->GetModule(qx, qy)) {
                        int px = (border + qx) * module_size;
                        int py = (border + qy) * module_size;
                        qr_painter.drawRect(px, py, module_size, module_size);
                    }
                }
            }

            // Draw QR code image
            painter.drawImage(QRect(0, y, qr_size, qr_size), qr_image);

            painter.setFont(value_font);
            painter.drawText(qr_size + 20, y + 20, "Scan to pay");

            y += qr_size + 10;
        }

        return y;
    }

    void DrawFooter(QPainter& painter, const InvoiceData& data, int y) {
        QFont font("Arial", 9);
        painter.setFont(font);

        if (!data.notes.empty()) {
            painter.drawText(0, y, "Notes:");
            y += 15;
            painter.drawText(QRect(0, y, CONTENT_WIDTH, 100),
                Qt::TextWordWrap, QString::fromStdString(data.notes));
            y += 50;
        }

        if (!data.terms.empty()) {
            painter.drawText(0, y, "Terms & Conditions:");
            y += 15;
            painter.drawText(QRect(0, y, CONTENT_WIDTH, 100),
                Qt::TextWordWrap, QString::fromStdString(data.terms));
        }
    }
};

// ============================================================================
// PDFInvoiceGenerator Implementation
// ============================================================================

PDFInvoiceGenerator::PDFInvoiceGenerator()
    : pimpl_(std::make_unique<Impl>())
{}

PDFInvoiceGenerator::~PDFInvoiceGenerator() = default;

void PDFInvoiceGenerator::SetStyle(Style style) {
    pimpl_->style_ = style;
}

bool PDFInvoiceGenerator::SetLogo(const std::string& logo_path) {
    QImage logo(QString::fromStdString(logo_path));
    if (logo.isNull()) {
        return false;
    }

    pimpl_->logo_ = logo;
    pimpl_->has_logo_ = true;
    return true;
}

bool PDFInvoiceGenerator::Generate(
    const InvoiceData& data,
    const std::string& output_path,
    bool include_qr_code
) {
    std::string error = ValidateInvoiceData(data);
    if (!error.empty()) {
        return false;
    }

    return pimpl_->Generate(data, output_path, include_qr_code);
}

std::vector<uint8_t> PDFInvoiceGenerator::GeneratePreview(
    const InvoiceData& data,
    bool include_qr_code
) {
    std::string error = ValidateInvoiceData(data);
    if (!error.empty()) {
        return {};
    }

    return pimpl_->GeneratePreview(data, include_qr_code);
}

std::string PDFInvoiceGenerator::ValidateInvoiceData(const InvoiceData& data) {
    if (data.invoice_number.empty()) {
        return "Invoice number is required";
    }

    if (data.from_name.empty()) {
        return "Sender name is required";
    }

    if (data.to_name.empty()) {
        return "Recipient name is required";
    }

    if (data.items.empty()) {
        return "At least one item is required";
    }

    if (data.total_amount <= 0) {
        return "Total amount must be greater than zero";
    }

    return "";
}

std::string PDFInvoiceGenerator::GenerateInvoiceNumber() {
    auto now = std::time(nullptr);
    auto* tm = std::localtime(&now);

    std::ostringstream oss;
    oss << "INV-"
        << std::setfill('0')
        << std::setw(4) << (tm->tm_year + 1900)
        << std::setw(2) << (tm->tm_mon + 1)
        << std::setw(2) << tm->tm_mday
        << "-";

    // Add random 4-digit suffix
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    oss << std::setw(4) << dis(gen);

    return oss.str();
}

std::string PDFInvoiceGenerator::FormatAmount(double amount, bool include_symbol) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << amount;

    if (include_symbol) {
        oss << " INT";
    }

    return oss.str();
}

std::string PDFInvoiceGenerator::FormatDate(uint64_t timestamp) {
    if (timestamp == 0) {
        return "";
    }

    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm->tm_year + 1900) << "-"
        << std::setw(2) << (tm->tm_mon + 1) << "-"
        << std::setw(2) << tm->tm_mday;

    return oss.str();
}

} // namespace qt
} // namespace intcoin
