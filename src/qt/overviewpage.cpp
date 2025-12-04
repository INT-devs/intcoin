// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/overviewpage.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/util.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFont>

namespace intcoin {
namespace qt {

OverviewPage::OverviewPage(Wallet* wallet, Blockchain* blockchain, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
{
    setupUi();
}

OverviewPage::~OverviewPage() {}

void OverviewPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Balance section
    QGroupBox* balanceGroup = new QGroupBox(tr("Balances"), this);
    QVBoxLayout* balanceLayout = new QVBoxLayout(balanceGroup);

    QFont boldFont;
    boldFont.setBold(true);

    // Available balance
    QHBoxLayout* availableLayout = new QHBoxLayout();
    availableBalanceLabel_ = new QLabel(tr("Available:"), this);
    availableBalanceValue_ = new QLabel(tr("0.000000 INT"), this);
    availableBalanceValue_->setFont(boldFont);
    availableLayout->addWidget(availableBalanceLabel_);
    availableLayout->addStretch();
    availableLayout->addWidget(availableBalanceValue_);
    balanceLayout->addLayout(availableLayout);

    // Pending balance
    QHBoxLayout* pendingLayout = new QHBoxLayout();
    pendingBalanceLabel_ = new QLabel(tr("Pending:"), this);
    pendingBalanceValue_ = new QLabel(tr("0.000000 INT"), this);
    pendingLayout->addWidget(pendingBalanceLabel_);
    pendingLayout->addStretch();
    pendingLayout->addWidget(pendingBalanceValue_);
    balanceLayout->addLayout(pendingLayout);

    // Total balance
    QHBoxLayout* totalLayout = new QHBoxLayout();
    totalBalanceLabel_ = new QLabel(tr("Total:"), this);
    totalBalanceValue_ = new QLabel(tr("0.000000 INT"), this);
    totalBalanceValue_->setFont(boldFont);
    totalLayout->addWidget(totalBalanceLabel_);
    totalLayout->addStretch();
    totalLayout->addWidget(totalBalanceValue_);
    balanceLayout->addLayout(totalLayout);

    mainLayout->addWidget(balanceGroup);

    // Recent transactions section
    QGroupBox* transactionsGroup = new QGroupBox(tr("Recent Transactions"), this);
    QVBoxLayout* transactionsLayout = new QVBoxLayout(transactionsGroup);

    recentTransactionsList_ = new QListWidget(this);
    transactionsLayout->addWidget(recentTransactionsList_);

    mainLayout->addWidget(transactionsGroup);

    setLayout(mainLayout);
}

void OverviewPage::updateBalance() {
    if (!wallet_) {
        return;
    }

    // TODO: Get actual balance from wallet
    uint64_t availableBalance = 0; // wallet_->GetBalance();
    uint64_t pendingBalance = 0;   // wallet_->GetPendingBalance();
    uint64_t totalBalance = availableBalance + pendingBalance;

    availableBalanceValue_->setText(QString::number(IntsToInt(availableBalance), 'f', 6) + " INT");
    pendingBalanceValue_->setText(QString::number(IntsToInt(pendingBalance), 'f', 6) + " INT");
    totalBalanceValue_->setText(QString::number(IntsToInt(totalBalance), 'f', 6) + " INT");
}

void OverviewPage::updateRecentTransactions() {
    if (!wallet_) {
        return;
    }

    recentTransactionsList_->clear();

    // TODO: Get recent transactions from wallet
    // For now, show placeholder
    recentTransactionsList_->addItem(tr("No recent transactions"));
}

} // namespace qt
} // namespace intcoin
