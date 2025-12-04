// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/transactionspage.h"
#include "intcoin/wallet.h"
#include "intcoin/util.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QColor>
#include <algorithm>
#include <cmath>

namespace intcoin {
namespace qt {

TransactionsPage::TransactionsPage(wallet::Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
    updateTransactionList();
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
    if (!wallet_) {
        return;
    }

    transactionTable_->setRowCount(0);

    // Get all transactions from wallet
    auto txsResult = wallet_->GetTransactions();
    if (!txsResult.IsOk() || txsResult.GetValue().empty()) {
        return;
    }

    const auto& transactions = txsResult.GetValue();
    transactionTable_->setRowCount(transactions.size());

    for (size_t i = 0; i < transactions.size(); ++i) {
        const auto& wtx = transactions[i];

        // Date column
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(wtx.timestamp);
        QTableWidgetItem* dateItem = new QTableWidgetItem(dateTime.toString("yyyy-MM-dd hh:mm:ss"));
        transactionTable_->setItem(i, 0, dateItem);

        // Type column
        QString type;
        if (wtx.is_coinbase) {
            type = tr("Mined");
        } else if (wtx.amount >= 0) {
            type = tr("Received");
        } else {
            type = tr("Sent");
        }
        QTableWidgetItem* typeItem = new QTableWidgetItem(type);
        transactionTable_->setItem(i, 1, typeItem);

        // Address column (extract from transaction inputs/outputs)
        QString address = tr("Multiple");
        if (!wtx.is_coinbase) {
            if (wtx.amount >= 0 && !wtx.tx.outputs.empty()) {
                // For received, show first output address (would need address extraction from script)
                address = tr("(See details)");
            } else if (wtx.amount < 0 && !wtx.tx.outputs.empty()) {
                // For sent, show first output address
                address = tr("(See details)");
            }
        }
        QTableWidgetItem* addressItem = new QTableWidgetItem(address);
        transactionTable_->setItem(i, 2, addressItem);

        // Amount column
        QString amountStr;
        if (wtx.amount >= 0) {
            amountStr = "+" + QString::number(IntsToInt(wtx.amount), 'f', 6) + " INT";
        } else {
            amountStr = QString::number(IntsToInt(wtx.amount), 'f', 6) + " INT";
        }
        QTableWidgetItem* amountItem = new QTableWidgetItem(amountStr);
        if (wtx.amount >= 0) {
            amountItem->setForeground(QColor(0, 128, 0)); // Green for received
        } else {
            amountItem->setForeground(QColor(192, 0, 0)); // Red for sent
        }
        transactionTable_->setItem(i, 3, amountItem);

        // Status column
        QString status;
        if (wtx.IsConfirmed()) {
            // Get confirmations (would need current blockchain height)
            status = tr("Confirmed");
        } else {
            status = tr("Pending");
        }
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        transactionTable_->setItem(i, 4, statusItem);

        // Store txid in row data for details dialog
        dateItem->setData(Qt::UserRole, QVariant::fromValue(i));
    }
}

void TransactionsPage::filterTransactions() {
    QString typeFilter = typeFilter_->currentText();
    QString searchText = searchEdit_->text().toLower();

    for (int row = 0; row < transactionTable_->rowCount(); ++row) {
        bool showRow = true;

        // Filter by type
        if (typeFilter != tr("All")) {
            QTableWidgetItem* typeItem = transactionTable_->item(row, 1);
            if (typeItem && typeItem->text() != typeFilter) {
                showRow = false;
            }
        }

        // Filter by search text (search in all columns)
        if (showRow && !searchText.isEmpty()) {
            bool matchFound = false;
            for (int col = 0; col < transactionTable_->columnCount(); ++col) {
                QTableWidgetItem* item = transactionTable_->item(row, col);
                if (item && item->text().toLower().contains(searchText)) {
                    matchFound = true;
                    break;
                }
            }
            showRow = matchFound;
        }

        transactionTable_->setRowHidden(row, !showRow);
    }
}

void TransactionsPage::exportTransactions() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Transaction History"), "transactions.csv", tr("CSV Files (*.csv)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"),
            tr("Failed to open file for writing: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);

    // Write CSV header
    out << "Date,Type,Address,Amount,Status\n";

    // Write visible rows only
    int exportCount = 0;
    for (int row = 0; row < transactionTable_->rowCount(); ++row) {
        if (transactionTable_->isRowHidden(row)) {
            continue;
        }

        // Export each column
        for (int col = 0; col < transactionTable_->columnCount(); ++col) {
            QTableWidgetItem* item = transactionTable_->item(row, col);
            if (item) {
                QString text = item->text();
                // Escape quotes and wrap in quotes if contains comma
                if (text.contains(',') || text.contains('"')) {
                    text.replace("\"", "\"\"");
                    out << "\"" << text << "\"";
                } else {
                    out << text;
                }
            }
            if (col < transactionTable_->columnCount() - 1) {
                out << ",";
            }
        }
        out << "\n";
        exportCount++;
    }

    file.close();

    QMessageBox::information(this, tr("Export Successful"),
        tr("Exported %1 transaction(s) to:\n%2").arg(exportCount).arg(fileName));
}

void TransactionsPage::showTransactionDetails(int row, int column) {
    (void)column;

    if (!wallet_ || row < 0 || row >= transactionTable_->rowCount()) {
        return;
    }

    // Get transaction index from row data
    QTableWidgetItem* dateItem = transactionTable_->item(row, 0);
    if (!dateItem) {
        return;
    }

    size_t txIndex = dateItem->data(Qt::UserRole).toUInt();

    // Get transaction from wallet
    auto txsResult = wallet_->GetTransactions();
    if (!txsResult.IsOk() || txIndex >= txsResult.GetValue().size()) {
        return;
    }

    const auto& wtx = txsResult.GetValue()[txIndex];

    // Build detailed information string
    QString details;
    details += tr("Transaction ID:\n%1\n\n").arg(QString::fromStdString(ToHex(wtx.txid)));
    details += tr("Date: %1\n").arg(QDateTime::fromSecsSinceEpoch(wtx.timestamp).toString("yyyy-MM-dd hh:mm:ss"));

    QString type;
    if (wtx.is_coinbase) {
        type = tr("Mined");
    } else if (wtx.amount >= 0) {
        type = tr("Received");
    } else {
        type = tr("Sent");
    }
    details += tr("Type: %1\n").arg(type);

    details += tr("Amount: %1 INT\n").arg(QString::number(IntsToInt(std::abs(wtx.amount)), 'f', 6));
    details += tr("Fee: %1 INT\n").arg(QString::number(IntsToInt(wtx.fee), 'f', 6));

    QString status;
    if (wtx.IsConfirmed()) {
        status = tr("Confirmed (Block %1)").arg(wtx.block_height);
    } else {
        status = tr("Pending (Unconfirmed)");
    }
    details += tr("Status: %1\n").arg(status);

    details += tr("\nInputs: %1\n").arg(wtx.tx.inputs.size());
    details += tr("Outputs: %1\n").arg(wtx.tx.outputs.size());

    if (!wtx.comment.empty()) {
        details += tr("\nComment: %1").arg(QString::fromStdString(wtx.comment));
    }

    QMessageBox::information(this, tr("Transaction Details"), details);
}

} // namespace qt
} // namespace intcoin
