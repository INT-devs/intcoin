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
#include <algorithm>
#include <cmath>

namespace intcoin {
namespace qt {

OverviewPage::OverviewPage(wallet::Wallet* wallet, Blockchain* blockchain, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
{
    setupUi();
    updateBalance();
    updateRecentTransactions();
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

    // Get actual balance from wallet
    auto balanceResult = wallet_->GetBalance();
    uint64_t availableBalance = balanceResult.IsOk() ? balanceResult.GetValue() : 0;

    auto unconfirmedResult = wallet_->GetUnconfirmedBalance();
    uint64_t pendingBalance = unconfirmedResult.IsOk() ? unconfirmedResult.GetValue() : 0;

    uint64_t totalBalance = availableBalance + pendingBalance;

    // Convert from INTS (smallest unit) to INT (display unit)
    // 1 INT = 1,000,000 INTS
    availableBalanceValue_->setText(QString::number(IntsToInt(availableBalance), 'f', 6) + " INT");
    pendingBalanceValue_->setText(QString::number(IntsToInt(pendingBalance), 'f', 6) + " INT");
    totalBalanceValue_->setText(QString::number(IntsToInt(totalBalance), 'f', 6) + " INT");
}

void OverviewPage::updateRecentTransactions() {
    if (!wallet_) {
        return;
    }

    recentTransactionsList_->clear();

    // Get recent transactions from wallet
    auto txsResult = wallet_->GetTransactions();
    if (!txsResult.IsOk() || txsResult.GetValue().empty()) {
        recentTransactionsList_->addItem(tr("No recent transactions"));
        return;
    }

    // Show up to 10 most recent transactions
    const auto& transactions = txsResult.GetValue();
    size_t count = std::min<size_t>(10, transactions.size());

    for (size_t i = 0; i < count; ++i) {
        const auto& wtx = transactions[i];

        // Format transaction item
        QString type = wtx.amount >= 0 ? tr("Received") : tr("Sent");
        QString amount = QString::number(std::abs(IntsToInt(wtx.amount)), 'f', 6) + " INT";
        QString status = wtx.IsConfirmed() ? tr("Confirmed") : tr("Pending");

        QString item = QString("%1: %2 (%3)")
            .arg(type)
            .arg(amount)
            .arg(status);

        recentTransactionsList_->addItem(item);
    }
}

} // namespace qt
} // namespace intcoin
