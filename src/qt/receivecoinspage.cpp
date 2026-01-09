// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/receivecoinspage.h"
#include "intcoin/qt/qrcodedialog.h"
#include "intcoin/wallet.h"
#include "intcoin/util.h"
#include "intcoin/qrcode.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>

namespace intcoin {
namespace qt {

ReceiveCoinsPage::ReceiveCoinsPage(wallet::Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();

    // Load initial address from wallet
    if (wallet_) {
        auto addressesResult = wallet_->GetAddresses();
        if (addressesResult.IsOk() && !addressesResult.GetValue().empty()) {
            // Use first address
            currentAddressEdit_->setText(QString::fromStdString(addressesResult.GetValue()[0].address));
        } else {
            // Generate first address if none exist
            auto newAddrResult = wallet_->GetNewAddress("Default");
            if (newAddrResult.IsOk()) {
                currentAddressEdit_->setText(QString::fromStdString(newAddrResult.GetValue()));
            }
        }
        updateAddressList();
    }
}

ReceiveCoinsPage::~ReceiveCoinsPage() {}

void ReceiveCoinsPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Current address section
    QGroupBox* receiveGroup = new QGroupBox(tr("Receive Address"), this);
    QFormLayout* formLayout = new QFormLayout(receiveGroup);

    currentAddressEdit_ = new QLineEdit(this);
    currentAddressEdit_->setReadOnly(true);
    currentAddressEdit_->setPlaceholderText(tr("Generating address..."));
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
    if (!wallet_) {
        QMessageBox::critical(this, tr("Error"), tr("Wallet not loaded."));
        return;
    }

    // Get label from input
    QString label = labelEdit_->text();
    if (label.isEmpty()) {
        label = tr("Address %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }

    // Generate new address
    auto addressResult = wallet_->GetNewAddress(label.toStdString());
    if (!addressResult.IsOk()) {
        QMessageBox::critical(this, tr("Address Generation Error"),
            tr("Failed to generate new address: %1").arg(QString::fromStdString(addressResult.error)));
        return;
    }

    // Update UI with new address
    QString newAddress = QString::fromStdString(addressResult.GetValue());
    currentAddressEdit_->setText(newAddress);

    // Clear label field for next address
    labelEdit_->clear();

    // Refresh address list
    updateAddressList();

    QMessageBox::information(this, tr("New Address Generated"),
        tr("A new receiving address has been generated:\n\n%1").arg(newAddress));
}

void ReceiveCoinsPage::copyAddress() {
    QApplication::clipboard()->setText(currentAddressEdit_->text());
    QMessageBox::information(this, tr("Address Copied"),
        tr("Address copied to clipboard."));
}

void ReceiveCoinsPage::showQRCode() {
    QString address = currentAddressEdit_->text();
    if (address.isEmpty()) {
        QMessageBox::warning(this, tr("No Address"),
            tr("Please generate an address first."));
        return;
    }

    // Get amount if specified
    double amount = amountSpinBox_->value();
    std::string label = labelEdit_->text().toStdString();

    // Generate payment URI
    std::string uri;
    if (amount > 0.0 || !label.empty()) {
        uri = QRCode::GenerateAddress(
            address.toStdString(),
            amount,
            label
        ) ? address.toStdString() : "";
    } else {
        uri = "intcoin:" + address.toStdString();
    }

    if (uri.empty()) {
        QMessageBox::warning(this, tr("QR Code Error"),
            tr("Failed to generate QR code."));
        return;
    }

    // Show QR code in dialog
    QRCodeDialog dialog(uri, tr("Receive Address QR Code"), this);
    dialog.exec();
}

void ReceiveCoinsPage::updateAddressList() {
    if (!wallet_) {
        return;
    }

    addressTable_->setRowCount(0);

    // Load addresses from wallet
    auto addressesResult = wallet_->GetAddresses();
    if (!addressesResult.IsOk()) {
        return;
    }

    const auto& addresses = addressesResult.GetValue();
    addressTable_->setRowCount(addresses.size());

    for (size_t i = 0; i < addresses.size(); ++i) {
        const auto& addr = addresses[i];

        // Label
        QTableWidgetItem* labelItem = new QTableWidgetItem(QString::fromStdString(addr.label));
        addressTable_->setItem(i, 0, labelItem);

        // Address
        QTableWidgetItem* addressItem = new QTableWidgetItem(QString::fromStdString(addr.address));
        addressTable_->setItem(i, 1, addressItem);

        // Balance (get from wallet)
        auto balanceResult = wallet_->GetAddressBalance(addr.address);
        uint64_t balance = balanceResult.IsOk() ? balanceResult.GetValue() : 0;
        QString balanceStr = QString::number(IntsToInt(balance), 'f', 6) + " INT";
        QTableWidgetItem* balanceItem = new QTableWidgetItem(balanceStr);
        addressTable_->setItem(i, 2, balanceItem);
    }
}

} // namespace qt
} // namespace intcoin
