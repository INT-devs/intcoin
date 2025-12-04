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

namespace intcoin {
namespace qt {

AddressBookPage::AddressBookPage(wallet::Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
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
    QMessageBox::information(this, tr("Add Address"),
        tr("Add address not yet implemented."));
}

void AddressBookPage::editAddress() {
    QMessageBox::information(this, tr("Edit Address"),
        tr("Edit address not yet implemented."));
}

void AddressBookPage::deleteAddress() {
    QMessageBox::information(this, tr("Delete Address"),
        tr("Delete address not yet implemented."));
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
    addressTable_->setRowCount(0);
    // TODO: Load addresses from wallet
}

} // namespace qt
} // namespace intcoin
