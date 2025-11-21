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
#include "intcoin/lightning.h"
#include "intcoin/lightning_network.h"
#include "intcoin/rpc.h"
#include "mobile/mobile_wallet.h"

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
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
    void on_actionLightning_triggered();
    void on_actionSecurity_triggered();
    void on_actionTokens_triggered();
    void on_actionContracts_triggered();
    void on_actionDApps_triggered();

    // Wallet tab
    void on_sendButton_clicked();
    void on_receiveButton_clicked();
    void on_newAddressButton_clicked();
    void on_generateQRButton_clicked();
    void on_scanQRButton_clicked();

    // Lightning tab
    void on_openChannelButton_clicked();
    void on_closeChannelButton_clicked();
    void on_sendLightningButton_clicked();
    void on_createInvoiceButton_clicked();
    void on_payInvoiceButton_clicked();
    void update_lightning_channels();
    void update_lightning_balance();

    // Smart Contracts tab
    void on_deployContractButton_clicked();
    void on_callContractButton_clicked();
    void on_readContractButton_clicked();
    void on_loadContractABIButton_clicked();
    void on_estimateGasButton_clicked();
    void update_contract_events();

    // Tokens tab
    void on_addTokenButton_clicked();
    void on_removeTokenButton_clicked();
    void on_sendTokenButton_clicked();
    void on_refreshTokensButton_clicked();
    void update_token_balances();

    // DApp Browser tab
    void on_connectDAppButton_clicked();
    void on_disconnectDAppButton_clicked();
    void on_signMessageButton_clicked();
    void on_signTypedDataButton_clicked();
    void update_connected_dapps();

    // Security settings
    void on_enableBiometricButton_clicked();
    void on_disableBiometricButton_clicked();
    void on_enableTxPINButton_clicked();
    void on_disableTxPINButton_clicked();
    void on_setAutoLockButton_clicked();
    void on_setTxLimitButton_clicked();
    void on_wipeWalletButton_clicked();

    // Backup & Recovery
    void on_exportBackupButton_clicked();
    void on_importBackupButton_clicked();
    void on_enableCloudBackupButton_clicked();
    void on_disableCloudBackupButton_clicked();
    void on_exportMnemonicButton_clicked();

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
    QWidget* create_wallet_tab();
    QWidget* create_transactions_tab();
    QWidget* create_lightning_tab();
    QWidget* create_contracts_tab();
    QWidget* create_tokens_tab();
    QWidget* create_dapps_tab();
    QWidget* create_mining_tab();
    QWidget* create_network_tab();
    QWidget* create_console_tab();

    // Core components
    std::unique_ptr<Blockchain> blockchain_;
    std::unique_ptr<Mempool> mempool_;
    std::unique_ptr<HDWallet> wallet_;
    std::unique_ptr<Miner> miner_;
    std::unique_ptr<p2p::Network> network_;
    std::shared_ptr<lightning::LightningNode> ln_node_;
    std::shared_ptr<lightning::LightningNetworkManager> ln_network_;
    std::unique_ptr<mobile::MobileWallet> mobile_wallet_;

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
    QLabel* difficultyLabel_;
    QLineEdit* threadsEdit_;

    // UI components - Network tab
    QTableWidget* peerTable_;
    QLineEdit* peerAddressEdit_;
    QPushButton* connectPeerButton_;
    QPushButton* disconnectPeerButton_;

    // UI components - Console tab
    QTextEdit* consoleOutput_;
    QLineEdit* consoleInput_;

    // UI components - Lightning tab
    QTableWidget* channelsTable_;
    QPushButton* openChannelButton_;
    QPushButton* closeChannelButton_;
    QLineEdit* channelPeerEdit_;
    QLineEdit* channelAmountEdit_;
    QLabel* lightningBalanceLabel_;
    QPushButton* createInvoiceButton_;
    QLineEdit* invoiceAmountEdit_;
    QLineEdit* invoiceDescEdit_;
    QTextEdit* invoiceDisplay_;
    QPushButton* payInvoiceButton_;
    QLineEdit* payInvoiceEdit_;
    QPushButton* sendLightningButton_;

    // UI components - Smart Contracts tab
    QTextEdit* contractBytecodeEdit_;
    QTextEdit* contractArgsEdit_;
    QLineEdit* contractGasLimitEdit_;
    QPushButton* deployContractButton_;
    QLineEdit* contractAddressEdit_;
    QTextEdit* contractABIEdit_;
    QPushButton* loadContractABIButton_;
    QComboBox* contractFunctionCombo_;
    QTextEdit* contractFunctionArgsEdit_;
    QLineEdit* contractValueEdit_;
    QPushButton* callContractButton_;
    QPushButton* readContractButton_;
    QPushButton* estimateGasButton_;
    QTableWidget* contractEventsTable_;
    QTextEdit* contractResultDisplay_;

    // UI components - Tokens tab
    QTableWidget* tokensTable_;
    QLineEdit* tokenAddressEdit_;
    QPushButton* addTokenButton_;
    QPushButton* removeTokenButton_;
    QPushButton* refreshTokensButton_;
    QComboBox* tokenSelectCombo_;
    QLineEdit* tokenSendAddressEdit_;
    QLineEdit* tokenSendAmountEdit_;
    QPushButton* sendTokenButton_;

    // UI components - DApps tab
    QTableWidget* dappsTable_;
    QLineEdit* dappUrlEdit_;
    QPushButton* connectDAppButton_;
    QPushButton* disconnectDAppButton_;
    QTextEdit* signMessageEdit_;
    QPushButton* signMessageButton_;
    QTextEdit* signTypedDataEdit_;
    QPushButton* signTypedDataButton_;
    QTextEdit* signatureDisplay_;

    // RPC client
    std::unique_ptr<rpc::Client> rpcClient_;
    QString rpcHost_;
    uint16_t rpcPort_;

    // Wallet state
    QString currentWalletPath_;
    bool biometricEnabled_;
    bool txPinEnabled_;
    uint32_t autoLockTimeout_;
    uint64_t transactionLimit_;
    QString displayCurrency_;
    QString denomination_;
    bool privacyMode_;

    // Update timer
    QTimer* updateTimer_;

    // Helper functions
    void show_error(const QString& title, const QString& message);
    void show_success(const QString& title, const QString& message);
    void show_info(const QString& title, const QString& message);
    bool load_wallet(const QString& filepath);
    void save_settings();
    void load_settings();

    // Security helpers
    bool verify_password(const QString& prompt);
    bool verify_pin(const QString& prompt);
    void lock_wallet();
    void unlock_wallet();

    // Format helpers
    QString format_amount(uint64_t satoshis);
    QString format_timestamp(uint64_t timestamp);
    QString format_gas(uint64_t gas);

    // Dialog helpers
    void show_qr_code(const QString& data, const QString& title);
    QString scan_qr_code();
    QString select_file(const QString& title, const QString& filter);
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_MAINWINDOW_H
