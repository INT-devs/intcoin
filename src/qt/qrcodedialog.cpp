// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qt/qrcodedialog.h>
#include <intcoin/qrcode.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

namespace intcoin {
namespace qt {

QRCodeDialog::QRCodeDialog(
    const std::string& text,
    const QString& title,
    QWidget* parent
)
    : QDialog(parent)
    , content_(text)
{
    setWindowTitle(title);
    setupUi();
    generateQRCode();
}

QRCodeDialog::~QRCodeDialog() = default;

void QRCodeDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // QR Code display
    qrCodeLabel_ = new QLabel(this);
    qrCodeLabel_->setAlignment(Qt::AlignCenter);
    qrCodeLabel_->setMinimumSize(300, 300);
    qrCodeLabel_->setStyleSheet("border: 1px solid #ccc; background: white;");
    mainLayout->addWidget(qrCodeLabel_);

    // Text display (read-only)
    textEdit_ = new QLineEdit(this);
    textEdit_->setText(QString::fromStdString(content_));
    textEdit_->setReadOnly(true);
    textEdit_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(textEdit_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    copyButton_ = new QPushButton(tr("Copy Image"), this);
    copyTextButton_ = new QPushButton(tr("Copy Text"), this);
    saveButton_ = new QPushButton(tr("Save As..."), this);
    closeButton_ = new QPushButton(tr("Close"), this);

    buttonLayout->addWidget(copyButton_);
    buttonLayout->addWidget(copyTextButton_);
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(copyButton_, &QPushButton::clicked, this, &QRCodeDialog::onCopyClicked);
    connect(copyTextButton_, &QPushButton::clicked, this, &QRCodeDialog::onCopyTextClicked);
    connect(saveButton_, &QPushButton::clicked, this, &QRCodeDialog::onSaveClicked);
    connect(closeButton_, &QPushButton::clicked, this, &QRCodeDialog::accept);

    setLayout(mainLayout);
    resize(400, 500);
}

bool QRCodeDialog::SetContent(const std::string& text) {
    content_ = text;
    textEdit_->setText(QString::fromStdString(content_));
    generateQRCode();
    return !qrPixmap_.isNull();
}

void QRCodeDialog::generateQRCode() {
    // Generate QR code using our utility
    auto qr_data = QRCode::Generate(content_, QRCode::ECLevel::HIGH);

    if (!qr_data) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to generate QR code"));
        return;
    }

    // Render QR code to pixmap
    int module_size = 6;  // 6 pixels per module
    int border = 4;       // 4 modules border
    int width = qr_data->width;
    int img_size = (width + 2 * border) * module_size;

    QImage img(img_size, img_size, QImage::Format_RGB32);
    img.fill(Qt::white);

    QPainter painter(&img);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < width; y++) {
        for (int x = 0; x < width; x++) {
            if (qr_data->GetModule(x, y)) {
                int px = (border + x) * module_size;
                int py = (border + y) * module_size;
                painter.drawRect(px, py, module_size, module_size);
            }
        }
    }

    qrPixmap_ = QPixmap::fromImage(img);
    qrCodeLabel_->setPixmap(qrPixmap_.scaled(
        qrCodeLabel_->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
}

void QRCodeDialog::onCopyClicked() {
    if (qrPixmap_.isNull()) {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(qrPixmap_);

    QMessageBox::information(this, tr("Copied"), tr("QR code copied to clipboard"));
}

void QRCodeDialog::onSaveClicked() {
    if (qrPixmap_.isNull()) {
        return;
    }

    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Save QR Code"),
        "qrcode.png",
        tr("PNG Images (*.png);;All Files (*)")
    );

    if (filename.isEmpty()) {
        return;
    }

    if (qrPixmap_.save(filename, "PNG")) {
        QMessageBox::information(this, tr("Saved"), tr("QR code saved successfully"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save QR code"));
    }
}

void QRCodeDialog::onCopyTextClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromStdString(content_));

    QMessageBox::information(this, tr("Copied"), tr("Text copied to clipboard"));
}

// ============================================================================
// QRCodeWidget Implementation
// ============================================================================

QRCodeWidget::QRCodeWidget(QWidget* parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet("border: 1px solid #ccc; background: white;");
    setMinimumSize(150, 150);
    setCursor(Qt::PointingHandCursor);
}

bool QRCodeWidget::SetContent(const std::string& text, int module_size) {
    content_ = text;

    if (text.empty()) {
        Clear();
        return false;
    }

    // Generate QR code
    auto qr_data = QRCode::Generate(text, QRCode::ECLevel::HIGH);

    if (!qr_data) {
        Clear();
        return false;
    }

    // Render QR code
    int border = 2;  // Smaller border for widget
    int width = qr_data->width;
    int img_size = (width + 2 * border) * module_size;

    QImage img(img_size, img_size, QImage::Format_RGB32);
    img.fill(Qt::white);

    QPainter painter(&img);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < width; y++) {
        for (int x = 0; x < width; x++) {
            if (qr_data->GetModule(x, y)) {
                int px = (border + x) * module_size;
                int py = (border + y) * module_size;
                painter.drawRect(px, py, module_size, module_size);
            }
        }
    }

    QPixmap pixmap = QPixmap::fromImage(img);
    setPixmap(pixmap.scaled(
        size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));

    return true;
}

void QRCodeWidget::Clear() {
    clear();
    content_.clear();
}

void QRCodeWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !content_.empty()) {
        emit clicked();
    }
    QLabel::mousePressEvent(event);
}

} // namespace qt
} // namespace intcoin
