// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_QRCODEDIALOG_H
#define INTCOIN_QT_QRCODEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QPixmap>
#include <string>

namespace intcoin {
namespace qt {

/**
 * Dialog for displaying QR codes
 *
 * Shows a QR code with optional save and copy functionality
 */
class QRCodeDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param text Text to encode in QR code
     * @param title Dialog title
     * @param parent Parent widget
     */
    explicit QRCodeDialog(
        const std::string& text,
        const QString& title = "QR Code",
        QWidget* parent = nullptr
    );

    ~QRCodeDialog() override;

    /**
     * Set QR code content
     *
     * @param text New text to encode
     * @return true if QR code generated successfully
     */
    bool SetContent(const std::string& text);

    /**
     * Get current QR code content
     */
    std::string GetContent() const { return content_; }

public slots:
    /**
     * Copy QR code to clipboard
     */
    void onCopyClicked();

    /**
     * Save QR code as image
     */
    void onSaveClicked();

    /**
     * Copy text to clipboard
     */
    void onCopyTextClicked();

private:
    void setupUi();
    void generateQRCode();

    std::string content_;
    QLabel* qrCodeLabel_;
    QLineEdit* textEdit_;
    QPushButton* copyButton_;
    QPushButton* copyTextButton_;
    QPushButton* saveButton_;
    QPushButton* closeButton_;
    QPixmap qrPixmap_;
};

/**
 * QR Code Widget
 *
 * Standalone widget for embedding QR codes in other pages
 */
class QRCodeWidget : public QLabel {
    Q_OBJECT

public:
    explicit QRCodeWidget(QWidget* parent = nullptr);

    /**
     * Set QR code content
     *
     * @param text Text to encode
     * @param module_size Size of each QR code module in pixels
     * @return true if successful
     */
    bool SetContent(const std::string& text, int module_size = 4);

    /**
     * Clear QR code
     */
    void Clear();

signals:
    /**
     * Emitted when QR code is clicked
     */
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    std::string content_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_QRCODEDIALOG_H
