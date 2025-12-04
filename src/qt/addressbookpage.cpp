// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/addressbookpage.h"
#include "intcoin/wallet.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QTableWidgetItem>
#include <QDateTime>

namespace intcoin {
namespace qt {

AddressBookPage::AddressBookPage(wallet::Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
    updateAddressList();
}

AddressBookPage::~AddressBookPage() {}

void AddressBookPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search addresses..."));
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);

    // Address table
    addressTable_ = new QTableWidget(this);
    addressTable_->setColumnCount(2);
    addressTable_->setHorizontalHeaderLabels(QStringList() << tr("Label") << tr("Address"));
    addressTable_->horizontalHeader()->setStretchLastSection(true);
    addressTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(addressTable_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    newButton_ = new QPushButton(tr("&New"), this);
    editButton_ = new QPushButton(tr("&Edit"), this);
    deleteButton_ = new QPushButton(tr("&Delete"), this);
    copyButton_ = new QPushButton(tr("&Copy"), this);
    buttonLayout->addWidget(newButton_);
    buttonLayout->addWidget(editButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(copyButton_);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(newButton_, &QPushButton::clicked, this, &AddressBookPage::addAddress);
    connect(editButton_, &QPushButton::clicked, this, &AddressBookPage::editAddress);
    connect(deleteButton_, &QPushButton::clicked, this, &AddressBookPage::deleteAddress);
    connect(copyButton_, &QPushButton::clicked, this, &AddressBookPage::copyAddress);
    connect(searchEdit_, &QLineEdit::textChanged, this, &AddressBookPage::updateAddressList);

    setLayout(mainLayout);
}

void AddressBookPage::addAddress() {
    if (!wallet_) {
        QMessageBox::critical(this, tr("Error"), tr("Wallet not loaded."));
        return;
    }

    // Prompt for label
    bool ok;
    QString label = QInputDialog::getText(this, tr("New Address"),
        tr("Enter a label for the new address:"), QLineEdit::Normal,
        tr("Address %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")), &ok);

    if (!ok || label.isEmpty()) {
        return;
    }

    // Generate new address
    auto addressResult = wallet_->GetNewAddress(label.toStdString());
    if (!addressResult.IsOk()) {
        QMessageBox::critical(this, tr("Address Generation Error"),
            tr("Failed to generate new address: %1").arg(QString::fromStdString(addressResult.error)));
        return;
    }

    QString newAddress = QString::fromStdString(addressResult.GetValue());

    // Refresh address list
    updateAddressList();

    QMessageBox::information(this, tr("Address Added"),
        tr("New address generated:\n\n%1\n\nLabel: %2").arg(newAddress, label));
}

void AddressBookPage::editAddress() {
    if (!wallet_) {
        QMessageBox::critical(this, tr("Error"), tr("Wallet not loaded."));
        return;
    }

    int currentRow = addressTable_->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, tr("No Selection"),
            tr("Please select an address to edit."));
        return;
    }

    QString address = addressTable_->item(currentRow, 1)->text();
    QString currentLabel = addressTable_->item(currentRow, 0)->text();

    // Prompt for new label
    bool ok;
    QString newLabel = QInputDialog::getText(this, tr("Edit Address Label"),
        tr("Enter a new label for address:\n%1").arg(address),
        QLineEdit::Normal, currentLabel, &ok);

    if (!ok || newLabel.isEmpty()) {
        return;
    }

    // Update label in wallet
    auto result = wallet_->SetAddressLabel(address.toStdString(), newLabel.toStdString());
    if (!result.IsOk()) {
        QMessageBox::critical(this, tr("Edit Error"),
            tr("Failed to update address label: %1").arg(QString::fromStdString(result.error)));
        return;
    }

    // Refresh address list
    updateAddressList();

    QMessageBox::information(this, tr("Label Updated"),
        tr("Address label updated successfully."));
}

void AddressBookPage::deleteAddress() {
    int currentRow = addressTable_->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, tr("No Selection"),
            tr("Please select an address to delete."));
        return;
    }

    QString address = addressTable_->item(currentRow, 1)->text();

    // Show warning about HD wallet addresses
    QMessageBox::warning(this, tr("Cannot Delete Address"),
        tr("Addresses in an HD (Hierarchical Deterministic) wallet cannot be deleted.\n\n"
           "All addresses are derived from your wallet seed and remain part of your wallet "
           "even if removed from the list. The address will still be monitored for incoming transactions.\n\n"
           "Address: %1\n\n"
           "You can edit the label to mark it as unused if desired.").arg(address));
}

void AddressBookPage::copyAddress() {
    int currentRow = addressTable_->currentRow();
    if (currentRow >= 0) {
        QString address = addressTable_->item(currentRow, 1)->text();
        QApplication::clipboard()->setText(address);
        QMessageBox::information(this, tr("Address Copied"),
            tr("Address copied to clipboard."));
    }
}

void AddressBookPage::updateAddressList() {
    if (!wallet_) {
        return;
    }

    addressTable_->setRowCount(0);

    // Get search filter
    QString searchText = searchEdit_->text().toLower();

    // Load addresses from wallet
    auto addressesResult = wallet_->GetAddresses();
    if (!addressesResult.IsOk()) {
        return;
    }

    const auto& addresses = addressesResult.GetValue();

    // Filter addresses by search text
    std::vector<wallet::WalletAddress> filteredAddresses;
    for (const auto& addr : addresses) {
        if (searchText.isEmpty() ||
            QString::fromStdString(addr.label).toLower().contains(searchText) ||
            QString::fromStdString(addr.address).toLower().contains(searchText)) {
            filteredAddresses.push_back(addr);
        }
    }

    addressTable_->setRowCount(filteredAddresses.size());

    for (size_t i = 0; i < filteredAddresses.size(); ++i) {
        const auto& addr = filteredAddresses[i];

        // Label column
        QTableWidgetItem* labelItem = new QTableWidgetItem(QString::fromStdString(addr.label));
        addressTable_->setItem(i, 0, labelItem);

        // Address column
        QTableWidgetItem* addressItem = new QTableWidgetItem(QString::fromStdString(addr.address));
        addressTable_->setItem(i, 1, addressItem);
    }
}

} // namespace qt
} // namespace intcoin
