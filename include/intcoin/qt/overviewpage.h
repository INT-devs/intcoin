/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Overview Page
 */

#ifndef INTCOIN_QT_OVERVIEWPAGE_H
#define INTCOIN_QT_OVERVIEWPAGE_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>

namespace intcoin {
    class Blockchain;
    namespace wallet {
        class Wallet;
    }
}

namespace intcoin {
namespace qt {

class OverviewPage : public QWidget {
    Q_OBJECT

public:
    explicit OverviewPage(wallet::Wallet* wallet, Blockchain* blockchain, QWidget *parent = nullptr);
    ~OverviewPage();

public slots:
    void updateBalance();
    void updateRecentTransactions();

private:
    void setupUi();

    wallet::Wallet* wallet_;
    Blockchain* blockchain_;

    // Balance display
    QLabel* availableBalanceLabel_;
    QLabel* pendingBalanceLabel_;
    QLabel* totalBalanceLabel_;
    QLabel* availableBalanceValue_;
    QLabel* pendingBalanceValue_;
    QLabel* totalBalanceValue_;

    // Recent transactions
    QListWidget* recentTransactionsList_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_OVERVIEWPAGE_H
