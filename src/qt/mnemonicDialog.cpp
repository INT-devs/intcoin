// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/mnemonicDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFont>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QPainter>
#include <QMessageBox>
#include <QDateTime>
#include <sstream>

namespace intcoin {
namespace qt {

// ============================================================================
// SecureTextEdit Implementation
// ============================================================================

SecureTextEdit::SecureTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);

    // Disable text interaction flags that enable selection/copy
    setTextInteractionFlags(Qt::NoTextInteraction | Qt::TextSelectableByMouse);

    // Set monospace font for better readability
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(12);
    setFont(font);

    // Styling
    setStyleSheet(
        "SecureTextEdit {"
        "    background-color: #f5f5f5;"
        "    border: 2px solid #d0d0d0;"
        "    border-radius: 5px;"
        "    padding: 10px;"
        "}"
    );
}

void SecureTextEdit::contextMenuEvent(QContextMenuEvent *event) {
    // Completely disable context menu (no copy option)
    event->accept();
}

void SecureTextEdit::keyPressEvent(QKeyEvent *event) {
    // Block all copy operations (Ctrl+C, Ctrl+Insert, etc.)
    if (event->matches(QKeySequence::Copy) ||
        event->matches(QKeySequence::Cut) ||
        event->matches(QKeySequence::Paste)) {
        event->accept();
        return;
    }

    // Allow text selection with keyboard for visual reference
    QTextEdit::keyPressEvent(event);
}

bool SecureTextEdit::canInsertFromMimeData(const QMimeData */*source*/) const {
    // Never allow pasting
    return false;
}

void SecureTextEdit::insertFromMimeData(const QMimeData */*source*/) {
    // Do nothing - paste disabled
}

// ============================================================================
// MnemonicDialog Implementation
// ============================================================================

MnemonicDialog::MnemonicDialog(const std::vector<std::string>& words, QWidget *parent)
    : QDialog(parent)
    , mnemonic_words_(words)
    , mnemonicText_(nullptr)
    , printButton_(nullptr)
    , closeButton_(nullptr)
    , warningLabel_(nullptr)
    , confirmCheckBox_(nullptr)
{
    setupUI();
}

void MnemonicDialog::setupUI() {
    setWindowTitle(tr("Backup Your Wallet - Mnemonic Seed Phrase"));
    setModal(true);
    setMinimumWidth(600);
    setMinimumHeight(500);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    auto *titleLabel = new QLabel(tr("<h2>Write Down Your Recovery Seed</h2>"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Warning message
    warningLabel_ = new QLabel(
        tr("<b>WARNING: This is your wallet's recovery phrase!</b><br><br>"
           "Write these 24 words down on paper in order. "
           "Store it in a secure location. "
           "Anyone with access to these words can steal your funds.<br><br>"
           "<b>Do NOT:</b><br>"
           "• Screenshot or photograph this phrase<br>"
           "• Store it on your computer or phone<br>"
           "• Share it with anyone<br>"
           "• Send it via email or messaging apps<br><br>"
           "<b>DO:</b><br>"
           "• Write it on paper with pen<br>"
           "• Store in a fireproof safe<br>"
           "• Make multiple copies in secure locations<br>"
           "• Use the Print button below for physical backup")
    );
    warningLabel_->setWordWrap(true);
    warningLabel_->setStyleSheet(
        "QLabel {"
        "    background-color: #fff3cd;"
        "    border: 2px solid #ffc107;"
        "    border-radius: 5px;"
        "    padding: 15px;"
        "    color: #856404;"
        "}"
    );
    mainLayout->addWidget(warningLabel_);

    // Mnemonic display (numbered list)
    auto *mnemonicLabel = new QLabel(tr("<b>Your 24-word recovery phrase:</b>"));
    mainLayout->addWidget(mnemonicLabel);

    mnemonicText_ = new SecureTextEdit(this);

    // Format mnemonic as numbered list
    std::ostringstream oss;
    for (size_t i = 0; i < mnemonic_words_.size(); ++i) {
        oss << (i + 1) << ". " << mnemonic_words_[i];
        if ((i + 1) % 4 == 0 && i + 1 < mnemonic_words_.size()) {
            oss << "\n\n";  // Group in rows of 4
        } else if (i + 1 < mnemonic_words_.size()) {
            oss << "\t\t";  // Tab separation
        }
    }

    mnemonicText_->setPlainText(QString::fromStdString(oss.str()));
    mnemonicText_->setMinimumHeight(250);
    mainLayout->addWidget(mnemonicText_);

    // Copy/paste disabled notice
    auto *noticeLabel = new QLabel(
        tr("<i>Note: Copy/paste is disabled for security. Use the Print button below.</i>")
    );
    noticeLabel->setStyleSheet("QLabel { color: #666; font-size: 10pt; }");
    noticeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(noticeLabel);

    // Confirmation checkbox
    confirmCheckBox_ = new QCheckBox(
        tr("I have written down my recovery phrase on paper and stored it securely")
    );
    confirmCheckBox_->setStyleSheet("QCheckBox { font-weight: bold; }");
    connect(confirmCheckBox_, &QCheckBox::checkStateChanged,
            this, &MnemonicDialog::onConfirmationChanged);
    mainLayout->addWidget(confirmCheckBox_);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    printButton_ = new QPushButton(tr("🖨 Print Backup"));
    printButton_->setMinimumHeight(40);
    printButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    font-size: 12pt;"
        "    font-weight: bold;"
        "    border-radius: 5px;"
        "    padding: 10px 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0056b3;"
        "}"
    );
    connect(printButton_, &QPushButton::clicked, this, &MnemonicDialog::onPrintClicked);
    buttonLayout->addWidget(printButton_);

    closeButton_ = new QPushButton(tr("I Have Backed Up My Seed - Close"));
    closeButton_->setMinimumHeight(40);
    closeButton_->setEnabled(false);  // Disabled until confirmation
    closeButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    font-size: 12pt;"
        "    font-weight: bold;"
        "    border-radius: 5px;"
        "    padding: 10px 20px;"
        "}"
        "QPushButton:hover:enabled {"
        "    background-color: #218838;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #6c757d;"
        "}"
    );
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);

    // Prevent closing without confirmation
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void MnemonicDialog::onPrintClicked() {
    printMnemonic();
}

void MnemonicDialog::onConfirmationChanged(Qt::CheckState state) {
    closeButton_->setEnabled(state == Qt::Checked);
}

void MnemonicDialog::printMnemonic() {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Portrait);

    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle(tr("Print Mnemonic Backup"));

    if (printDialog.exec() != QDialog::Accepted) {
        return;
    }

    // Create printable document
    QTextDocument doc;

    QString html = "<html><head><style>"
                   "body { font-family: 'Courier New', monospace; margin: 40px; }"
                   "h1 { text-align: center; color: #333; }"
                   ".warning { background-color: #fff3cd; border: 2px solid #ffc107; padding: 15px; margin: 20px 0; }"
                   ".mnemonic { background-color: #f5f5f5; border: 2px solid #d0d0d0; padding: 20px; font-size: 14pt; line-height: 2.0; }"
                   ".footer { margin-top: 30px; font-size: 10pt; color: #666; }"
                   "</style></head><body>";

    html += "<h1>INTcoin Wallet Recovery Seed</h1>";

    html += "<div class='warning'>"
            "<b>IMPORTANT:</b> Store this document in a secure location. "
            "Anyone with access to these words can access your wallet and steal your funds. "
            "Do not share this with anyone. "
            "Keep multiple copies in different secure locations."
            "</div>";

    html += "<div class='mnemonic'><b>Your 24-word recovery phrase (in order):</b><br><br>";

    // Format as 4 columns x 6 rows
    html += "<table width='100%' cellspacing='10'>";
    for (size_t i = 0; i < mnemonic_words_.size(); i += 4) {
        html += "<tr>";
        for (size_t j = 0; j < 4 && (i + j) < mnemonic_words_.size(); ++j) {
            html += QString("<td><b>%1.</b> %2</td>")
                    .arg(i + j + 1)
                    .arg(QString::fromStdString(mnemonic_words_[i + j]));
        }
        html += "</tr>";
    }
    html += "</table>";
    html += "</div>";

    html += "<div class='footer'>"
            "Wallet Created: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "<br>"
            "INTcoin Core v1.0.0 - https://international-coin.org"
            "</div>";

    html += "</body></html>";

    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, tr("Print Complete"),
                            tr("Your recovery seed has been sent to the printer.\n\n"
                               "Verify the printout is legible and store it securely."));
}

} // namespace qt
} // namespace intcoin
