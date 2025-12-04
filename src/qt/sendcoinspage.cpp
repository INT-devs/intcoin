// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/sendcoinspage.h"
#include "intcoin/wallet.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

namespace intcoin {
namespace qt {

SendCoinsPage::SendCoinsPage(Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
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
    QString address = addressEdit_->text();
    double amount = amountSpinBox_->value();

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

    // TODO: Actually send the transaction
    QMessageBox::information(this, tr("Send Coins"),
        tr("Transaction sending not yet implemented."));
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
