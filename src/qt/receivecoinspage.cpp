// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/receivecoinspage.h"
#include "intcoin/wallet.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>

namespace intcoin {
namespace qt {

ReceiveCoinsPage::ReceiveCoinsPage(Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
}

ReceiveCoinsPage::~ReceiveCoinsPage() {}

void ReceiveCoinsPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Current address section
    QGroupBox* receiveGroup = new QGroupBox(tr("Receive Address"), this);
    QFormLayout* formLayout = new QFormLayout(receiveGroup);

    currentAddressEdit_ = new QLineEdit(this);
    currentAddressEdit_->setReadOnly(true);
    currentAddressEdit_->setText(tr("int1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqegtg5n"));
    formLayout->addRow(tr("Address:"), currentAddressEdit_);

    labelEdit_ = new QLineEdit(this);
    labelEdit_->setPlaceholderText(tr("Optional label"));
    formLayout->addRow(tr("Label:"), labelEdit_);

    amountSpinBox_ = new QDoubleSpinBox(this);
    amountSpinBox_->setMaximum(999999999.999999);
    amountSpinBox_->setDecimals(6);
    amountSpinBox_->setSuffix(" INT");
    formLayout->addRow(tr("Amount:"), amountSpinBox_);

    qrCodeLabel_ = new QLabel(this);
    qrCodeLabel_->setMinimumSize(200, 200);
    qrCodeLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    qrCodeLabel_->setAlignment(Qt::AlignCenter);
    qrCodeLabel_->setText(tr("QR Code"));
    formLayout->addRow(tr("QR Code:"), qrCodeLabel_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    newAddressButton_ = new QPushButton(tr("&New Address"), this);
    copyButton_ = new QPushButton(tr("&Copy Address"), this);
    qrButton_ = new QPushButton(tr("Show &QR"), this);
    buttonLayout->addWidget(newAddressButton_);
    buttonLayout->addWidget(copyButton_);
    buttonLayout->addWidget(qrButton_);
    formLayout->addRow("", buttonLayout);

    mainLayout->addWidget(receiveGroup);

    // Address list
    QGroupBox* addressListGroup = new QGroupBox(tr("Your Addresses"), this);
    QVBoxLayout* addressListLayout = new QVBoxLayout(addressListGroup);

    addressTable_ = new QTableWidget(this);
    addressTable_->setColumnCount(3);
    addressTable_->setHorizontalHeaderLabels(QStringList() << tr("Label") << tr("Address") << tr("Balance"));
    addressTable_->horizontalHeader()->setStretchLastSection(true);
    addressListLayout->addWidget(addressTable_);

    mainLayout->addWidget(addressListGroup);

    // Connections
    connect(newAddressButton_, &QPushButton::clicked, this, &ReceiveCoinsPage::generateNewAddress);
    connect(copyButton_, &QPushButton::clicked, this, &ReceiveCoinsPage::copyAddress);
    connect(qrButton_, &QPushButton::clicked, this, &ReceiveCoinsPage::showQRCode);

    setLayout(mainLayout);
}

void ReceiveCoinsPage::generateNewAddress() {
    QMessageBox::information(this, tr("Generate New Address"),
        tr("Address generation not yet implemented."));
}

void ReceiveCoinsPage::copyAddress() {
    QApplication::clipboard()->setText(currentAddressEdit_->text());
    QMessageBox::information(this, tr("Address Copied"),
        tr("Address copied to clipboard."));
}

void ReceiveCoinsPage::showQRCode() {
    QMessageBox::information(this, tr("QR Code"),
        tr("QR code generation not yet implemented."));
}

void ReceiveCoinsPage::updateAddressList() {
    addressTable_->setRowCount(0);
    // TODO: Load addresses from wallet
}

} // namespace qt
} // namespace intcoin
