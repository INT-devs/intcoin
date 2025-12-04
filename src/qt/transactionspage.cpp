// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/transactionspage.h"
#include "intcoin/wallet.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>

namespace intcoin {
namespace qt {

TransactionsPage::TransactionsPage(wallet::Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
}

TransactionsPage::~TransactionsPage() {}

void TransactionsPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Filter section
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Type:"), this));

    typeFilter_ = new QComboBox(this);
    typeFilter_->addItem(tr("All"));
    typeFilter_->addItem(tr("Sent"));
    typeFilter_->addItem(tr("Received"));
    typeFilter_->addItem(tr("Mined"));
    filterLayout->addWidget(typeFilter_);

    filterLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search transactions..."));
    filterLayout->addWidget(searchEdit_);

    exportButton_ = new QPushButton(tr("&Export"), this);
    filterLayout->addWidget(exportButton_);

    mainLayout->addLayout(filterLayout);

    // Transaction table
    transactionTable_ = new QTableWidget(this);
    transactionTable_->setColumnCount(5);
    transactionTable_->setHorizontalHeaderLabels(
        QStringList() << tr("Date") << tr("Type") << tr("Address") << tr("Amount") << tr("Status"));
    transactionTable_->horizontalHeader()->setStretchLastSection(true);
    transactionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(transactionTable_);

    // Connections
    connect(typeFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TransactionsPage::filterTransactions);
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &TransactionsPage::filterTransactions);
    connect(exportButton_, &QPushButton::clicked,
            this, &TransactionsPage::exportTransactions);
    connect(transactionTable_, &QTableWidget::cellDoubleClicked,
            this, &TransactionsPage::showTransactionDetails);

    setLayout(mainLayout);
}

void TransactionsPage::updateTransactionList() {
    transactionTable_->setRowCount(0);
    // TODO: Load transactions from wallet
}

void TransactionsPage::filterTransactions() {
    // TODO: Implement transaction filtering
}

void TransactionsPage::exportTransactions() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Transaction History"), "transactions.csv", tr("CSV Files (*.csv)"));

    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export Transactions"),
            tr("Transaction export not yet implemented."));
    }
}

void TransactionsPage::showTransactionDetails(int row, int column) {
    (void)row;
    (void)column;
    QMessageBox::information(this, tr("Transaction Details"),
        tr("Transaction details not yet implemented."));
}

} // namespace qt
} // namespace intcoin
