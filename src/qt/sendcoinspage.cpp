// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/sendcoinspage.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/util.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

namespace intcoin {
namespace qt {

SendCoinsPage::SendCoinsPage(wallet::Wallet* wallet, Blockchain* blockchain, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
{
    setupUi();
}

SendCoinsPage::~SendCoinsPage() {}

void SendCoinsPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* sendGroup = new QGroupBox(tr("Send Coins"), this);
    QFormLayout* formLayout = new QFormLayout(sendGroup);

    // Pay to
    addressEdit_ = new QLineEdit(this);
    addressEdit_->setPlaceholderText(tr("Enter an INTcoin address (int1...)"));
    QHBoxLayout* addressLayout = new QHBoxLayout();
    addressLayout->addWidget(addressEdit_);
    pasteButton_ = new QPushButton(tr("Paste"), this);
    qrButton_ = new QPushButton(tr("QR"), this);
    addressLayout->addWidget(pasteButton_);
    addressLayout->addWidget(qrButton_);
    formLayout->addRow(tr("Pay &To:"), addressLayout);

    // Label
    labelEdit_ = new QLineEdit(this);
    labelEdit_->setPlaceholderText(tr("Optional label"));
    formLayout->addRow(tr("&Label:"), labelEdit_);

    // Amount
    amountSpinBox_ = new QDoubleSpinBox(this);
    amountSpinBox_->setMaximum(999999999.999999);
    amountSpinBox_->setDecimals(6);
    amountSpinBox_->setSuffix(" INT");
    formLayout->addRow(tr("&Amount:"), amountSpinBox_);

    // Fee
    feeSpinBox_ = new QDoubleSpinBox(this);
    feeSpinBox_->setMaximum(1000.0);
    feeSpinBox_->setDecimals(6);
    feeSpinBox_->setSuffix(" INT");
    feeSpinBox_->setValue(0.0001);
    formLayout->addRow(tr("&Fee:"), feeSpinBox_);

    // Total
    totalLabel_ = new QLabel(tr("0.000000 INT"), this);
    QFont boldFont;
    boldFont.setBold(true);
    totalLabel_->setFont(boldFont);
    formLayout->addRow(tr("Total:"), totalLabel_);

    mainLayout->addWidget(sendGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    sendButton_ = new QPushButton(tr("&Send"), this);
    clearButton_ = new QPushButton(tr("&Clear"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addWidget(sendButton_);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();

    // Connections
    connect(pasteButton_, &QPushButton::clicked, this, &SendCoinsPage::pasteAddress);
    connect(qrButton_, &QPushButton::clicked, this, &SendCoinsPage::scanQRCode);
    connect(amountSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SendCoinsPage::calculateFee);
    connect(feeSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SendCoinsPage::calculateFee);
    connect(sendButton_, &QPushButton::clicked, this, &SendCoinsPage::sendCoins);
    connect(clearButton_, &QPushButton::clicked, this, &SendCoinsPage::clearFields);

    setLayout(mainLayout);
}

void SendCoinsPage::sendCoins() {
    if (!wallet_) {
        QMessageBox::critical(this, tr("Error"), tr("Wallet not loaded."));
        return;
    }

    QString address = addressEdit_->text();
    double amount = amountSpinBox_->value();
    double feeRate = feeSpinBox_->value();

    if (!validateAddress(address)) {
        QMessageBox::warning(this, tr("Invalid Address"),
            tr("The address you entered is not valid."));
        return;
    }

    if (!validateAmount(amount)) {
        QMessageBox::warning(this, tr("Invalid Amount"),
            tr("The amount must be greater than 0."));
        return;
    }

    // Convert amount from INT to INTS (smallest unit)
    uint64_t amountInts = IntToInts(amount);
    uint64_t feeRateInts = feeRate > 0 ? IntToInts(feeRate) : 0;

    // Create transaction recipient
    std::vector<intcoin::wallet::Wallet::Recipient> recipients;
    recipients.push_back({address.toStdString(), amountInts});

    // Create and sign transaction
    auto txResult = wallet_->CreateTransaction(recipients, feeRateInts, labelEdit_->text().toStdString());
    if (!txResult.IsOk()) {
        QMessageBox::critical(this, tr("Transaction Error"),
            tr("Failed to create transaction: %1").arg(QString::fromStdString(txResult.error)));
        return;
    }

    auto signResult = wallet_->SignTransaction(txResult.GetValue());
    if (!signResult.IsOk()) {
        QMessageBox::critical(this, tr("Signing Error"),
            tr("Failed to sign transaction: %1").arg(QString::fromStdString(signResult.error)));
        return;
    }

    // Broadcast transaction to blockchain
    if (!blockchain_) {
        QMessageBox::critical(this, tr("Error"),
            tr("Blockchain not available for broadcasting."));
        return;
    }

    auto broadcastResult = wallet_->SendTransaction(signResult.GetValue(), *blockchain_);
    if (!broadcastResult.IsOk()) {
        QMessageBox::critical(this, tr("Broadcast Error"),
            tr("Failed to broadcast transaction: %1").arg(QString::fromStdString(broadcastResult.error)));
        return;
    }

    // Get transaction ID
    QString txid = QString::fromStdString(ToHex(broadcastResult.GetValue()));

    // Clear fields after successful send
    clearFields();

    QMessageBox::information(this, tr("Transaction Sent"),
        tr("Transaction successfully broadcast to the network!\n\nTransaction ID:\n%1\n\n"
           "The transaction will be confirmed once it is included in a block.").arg(txid));
}

void SendCoinsPage::clearFields() {
    addressEdit_->clear();
    labelEdit_->clear();
    amountSpinBox_->setValue(0.0);
    feeSpinBox_->setValue(0.0001);
    calculateFee();
}

void SendCoinsPage::pasteAddress() {
    QString clipboard = QApplication::clipboard()->text();
    if (!clipboard.isEmpty()) {
        addressEdit_->setText(clipboard);
    }
}

void SendCoinsPage::scanQRCode() {
    QMessageBox::information(this, tr("Scan QR Code"),
        tr("QR code scanning not yet implemented."));
}

void SendCoinsPage::calculateFee() {
    double amount = amountSpinBox_->value();
    double fee = feeSpinBox_->value();
    double total = amount + fee;
    totalLabel_->setText(QString::number(total, 'f', 6) + " INT");
}

bool SendCoinsPage::validateAddress(const QString& address) {
    // TODO: Implement proper address validation
    return address.startsWith("int1") && address.length() > 40;
}

bool SendCoinsPage::validateAmount(double amount) {
    return amount > 0.0;
}

} // namespace qt
} // namespace intcoin
