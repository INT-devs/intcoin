/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Transactions Page
 */

#ifndef INTCOIN_QT_TRANSACTIONSPAGE_H
#define INTCOIN_QT_TRANSACTIONSPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace intcoin {
    class Wallet;
}

namespace intcoin {
namespace qt {

class TransactionsPage : public QWidget {
    Q_OBJECT

public:
    explicit TransactionsPage(Wallet* wallet, QWidget *parent = nullptr);
    ~TransactionsPage();

public slots:
    void updateTransactionList();
    void filterTransactions();
    void exportTransactions();
    void showTransactionDetails(int row, int column);

private:
    void setupUi();

    Wallet* wallet_;

    // Filter options
    QComboBox* typeFilter_;
    QLineEdit* searchEdit_;
    QPushButton* exportButton_;

    // Transaction table
    QTableWidget* transactionTable_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_TRANSACTIONSPAGE_H
