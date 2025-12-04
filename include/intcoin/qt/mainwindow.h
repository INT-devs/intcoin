/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Main Window
 */

#ifndef INTCOIN_QT_MAINWINDOW_H
#define INTCOIN_QT_MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <memory>

namespace intcoin {
    class Wallet;
    class Blockchain;
    class P2PNode;
}

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace intcoin {
namespace qt {

class OverviewPage;
class SendCoinsPage;
class ReceiveCoinsPage;
class TransactionsPage;
class AddressBookPage;
class SettingsPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Wallet* wallet, Blockchain* blockchain, P2PNode* p2p, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showOverviewPage();
    void showSendCoinsPage();
    void showReceiveCoinsPage();
    void showTransactionsPage();
    void showAddressBookPage();
    void showSettingsPage();
    void aboutINTcoin();
    void updateStatus();
    void createNewWallet();
    void openWallet();
    void backupWallet();
    void encryptWallet();
    void unlockWallet();
    void lockWallet();

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createPages();

    // Wallet and blockchain
    Wallet* wallet_;
    Blockchain* blockchain_;
    P2PNode* p2p_;

    // UI Components
    QStackedWidget* centralStack_;

    // Pages
    OverviewPage* overviewPage_;
    SendCoinsPage* sendCoinsPage_;
    ReceiveCoinsPage* receiveCoinsPage_;
    TransactionsPage* transactionsPage_;
    AddressBookPage* addressBookPage_;
    SettingsPage* settingsPage_;

    // Status bar
    QLabel* connectionsLabel_;
    QLabel* blocksLabel_;
    QLabel* syncLabel_;
    QTimer* statusTimer_;

    // Actions
    QAction* overviewAction_;
    QAction* sendCoinsAction_;
    QAction* receiveCoinsAction_;
    QAction* transactionsAction_;
    QAction* addressBookAction_;
    QAction* settingsAction_;
    QAction* exitAction_;
    QAction* aboutAction_;
    QAction* aboutQtAction_;

    // Wallet actions
    QAction* newWalletAction_;
    QAction* openWalletAction_;
    QAction* backupWalletAction_;
    QAction* encryptWalletAction_;
    QAction* unlockWalletAction_;
    QAction* lockWalletAction_;

    // Menus
    QMenu* fileMenu_;
    QMenu* walletMenu_;
    QMenu* viewMenu_;
    QMenu* helpMenu_;

    // State
    bool walletEncrypted_;
    bool walletLocked_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_MAINWINDOW_H
