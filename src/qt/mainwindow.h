// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Qt GUI Main Window

#ifndef INTCOIN_QT_MAINWINDOW_H
#define INTCOIN_QT_MAINWINDOW_H

#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/mempool.h"
#include "intcoin/miner.h"
#include "intcoin/p2p.h"

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Main application window
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Menu actions
    void on_actionNew_Wallet_triggered();
    void on_actionOpen_Wallet_triggered();
    void on_actionBackup_Wallet_triggered();
    void on_actionEncrypt_Wallet_triggered();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void on_actionSettings_triggered();

    // Wallet tab
    void on_sendButton_clicked();
    void on_receiveButton_clicked();
    void on_newAddressButton_clicked();

    // Mining tab
    void on_startMiningButton_clicked();
    void on_stopMiningButton_clicked();

    // Network tab
    void on_connectPeerButton_clicked();
    void on_disconnectPeerButton_clicked();

    // Update functions
    void update_balance();
    void update_transaction_history();
    void update_peer_list();
    void update_mining_stats();
    void update_blockchain_info();

private:
    // UI setup
    void setup_ui();
    void create_menu_bar();
    void create_status_bar();
    void create_wallet_tab();
    void create_transactions_tab();
    void create_mining_tab();
    void create_network_tab();
    void create_console_tab();

    // Core components
    std::unique_ptr<Blockchain> blockchain_;
    std::unique_ptr<Mempool> mempool_;
    std::unique_ptr<HDWallet> wallet_;
    std::unique_ptr<Miner> miner_;
    std::unique_ptr<p2p::Network> network_;

    // UI components - Status bar
    QLabel* statusLabel_;
    QLabel* blockHeightLabel_;
    QLabel* connectionsLabel_;
    QLabel* hashRateLabel_;
    QProgressBar* syncProgressBar_;

    // UI components - Wallet tab
    QLabel* balanceLabel_;
    QLineEdit* sendAddressEdit_;
    QLineEdit* sendAmountEdit_;
    QPushButton* sendButton_;
    QPushButton* receiveButton_;
    QLabel* receiveAddressLabel_;
    QPushButton* newAddressButton_;
    QTableWidget* addressTable_;

    // UI components - Transactions tab
    QTableWidget* transactionTable_;

    // UI components - Mining tab
    QPushButton* startMiningButton_;
    QPushButton* stopMiningButton_;
    QLabel* miningStatusLabel_;
    QLabel* hashRateValueLabel_;
    QLabel* blocksFoundLabel_;
    QLineEdit* threadsEdit_;

    // UI components - Network tab
    QTableWidget* peerTable_;
    QLineEdit* peerAddressEdit_;
    QPushButton* connectPeerButton_;
    QPushButton* disconnectPeerButton_;

    // UI components - Console tab
    QTextEdit* consoleOutput_;
    QLineEdit* consoleInput_;

    // Update timer
    QTimer* updateTimer_;

    // Helper functions
    void show_error(const QString& title, const QString& message);
    void show_success(const QString& title, const QString& message);
    bool load_wallet(const QString& filepath);
    void save_settings();
    void load_settings();
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_MAINWINDOW_H
