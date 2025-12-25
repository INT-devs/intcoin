// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/mainwindow.h"
#include "intcoin/qt/overviewpage.h"
#include "intcoin/qt/sendcoinspage.h"
#include "intcoin/qt/receivecoinspage.h"
#include "intcoin/qt/transactionspage.h"
#include "intcoin/qt/addressbookpage.h"
#include "intcoin/qt/lightningpage.h"
#include "intcoin/qt/settingspage.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/network.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QStackedWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QApplication>

namespace intcoin {
namespace qt {

MainWindow::MainWindow(wallet::Wallet* wallet, Blockchain* blockchain, P2PNode* p2p, LightningNetwork* lightning, QWidget *parent)
    : QMainWindow(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
    , p2p_(p2p)
    , lightning_(lightning)
    , walletEncrypted_(false)
    , walletLocked_(false)
{
    setWindowTitle(tr("INTcoin Core Wallet"));
    resize(1000, 600);

    createActions();
    createMenus();
    createToolBar();
    createPages();
    createStatusBar();

    // Start with overview page
    showOverviewPage();

    // Start status update timer
    statusTimer_ = new QTimer(this);
    connect(statusTimer_, &QTimer::timeout, this, &MainWindow::updateStatus);
    statusTimer_->start(1000); // Update every second
}

MainWindow::~MainWindow() {
    if (statusTimer_) {
        statusTimer_->stop();
    }
}

void MainWindow::createActions() {
    // File menu actions
    exitAction_ = new QAction(tr("E&xit"), this);
    exitAction_->setShortcuts(QKeySequence::Quit);
    exitAction_->setStatusTip(tr("Exit application"));
    connect(exitAction_, &QAction::triggered, qApp, &QApplication::quit);

    // Wallet menu actions
    newWalletAction_ = new QAction(tr("&New Wallet..."), this);
    newWalletAction_->setStatusTip(tr("Create a new wallet"));
    connect(newWalletAction_, &QAction::triggered, this, &MainWindow::createNewWallet);

    openWalletAction_ = new QAction(tr("&Open Wallet..."), this);
    openWalletAction_->setStatusTip(tr("Open an existing wallet"));
    connect(openWalletAction_, &QAction::triggered, this, &MainWindow::openWallet);

    backupWalletAction_ = new QAction(tr("&Backup Wallet..."), this);
    backupWalletAction_->setStatusTip(tr("Backup wallet to a file"));
    connect(backupWalletAction_, &QAction::triggered, this, &MainWindow::backupWallet);

    encryptWalletAction_ = new QAction(tr("&Encrypt Wallet..."), this);
    encryptWalletAction_->setStatusTip(tr("Encrypt wallet with a passphrase"));
    connect(encryptWalletAction_, &QAction::triggered, this, &MainWindow::encryptWallet);

    unlockWalletAction_ = new QAction(tr("&Unlock Wallet..."), this);
    unlockWalletAction_->setStatusTip(tr("Unlock wallet for sending"));
    unlockWalletAction_->setEnabled(false);
    connect(unlockWalletAction_, &QAction::triggered, this, &MainWindow::unlockWallet);

    lockWalletAction_ = new QAction(tr("&Lock Wallet"), this);
    lockWalletAction_->setStatusTip(tr("Lock wallet"));
    lockWalletAction_->setEnabled(false);
    connect(lockWalletAction_, &QAction::triggered, this, &MainWindow::lockWallet);

    // View menu actions
    overviewAction_ = new QAction(tr("&Overview"), this);
    overviewAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_1));
    overviewAction_->setStatusTip(tr("Show general overview of wallet"));
    connect(overviewAction_, &QAction::triggered, this, &MainWindow::showOverviewPage);

    sendCoinsAction_ = new QAction(tr("&Send"), this);
    sendCoinsAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_2));
    sendCoinsAction_->setStatusTip(tr("Send coins to an INTcoin address"));
    connect(sendCoinsAction_, &QAction::triggered, this, &MainWindow::showSendCoinsPage);

    receiveCoinsAction_ = new QAction(tr("&Receive"), this);
    receiveCoinsAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_3));
    receiveCoinsAction_->setStatusTip(tr("Request payments"));
    connect(receiveCoinsAction_, &QAction::triggered, this, &MainWindow::showReceiveCoinsPage);

    transactionsAction_ = new QAction(tr("&Transactions"), this);
    transactionsAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_4));
    transactionsAction_->setStatusTip(tr("Browse transaction history"));
    connect(transactionsAction_, &QAction::triggered, this, &MainWindow::showTransactionsPage);

    addressBookAction_ = new QAction(tr("&Address Book"), this);
    addressBookAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_5));
    addressBookAction_->setStatusTip(tr("Edit the address book"));
    connect(addressBookAction_, &QAction::triggered, this, &MainWindow::showAddressBookPage);

    lightningAction_ = new QAction(tr("&Lightning Network"), this);
    lightningAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_6));
    lightningAction_->setStatusTip(tr("Manage Lightning Network channels and payments"));
    connect(lightningAction_, &QAction::triggered, this, &MainWindow::showLightningPage);

    settingsAction_ = new QAction(tr("Se&ttings"), this);
    settingsAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_7));
    settingsAction_->setStatusTip(tr("Configure wallet settings"));
    connect(settingsAction_, &QAction::triggered, this, &MainWindow::showSettingsPage);

    // Help menu actions
    aboutAction_ = new QAction(tr("&About INTcoin Core"), this);
    aboutAction_->setStatusTip(tr("Show information about INTcoin Core"));
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::aboutINTcoin);

    aboutQtAction_ = new QAction(tr("About &Qt"), this);
    aboutQtAction_->setStatusTip(tr("Show information about Qt"));
    connect(aboutQtAction_, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus() {
    fileMenu_ = menuBar()->addMenu(tr("&File"));
    fileMenu_->addAction(exitAction_);

    walletMenu_ = menuBar()->addMenu(tr("&Wallet"));
    walletMenu_->addAction(newWalletAction_);
    walletMenu_->addAction(openWalletAction_);
    walletMenu_->addSeparator();
    walletMenu_->addAction(backupWalletAction_);
    walletMenu_->addSeparator();
    walletMenu_->addAction(encryptWalletAction_);
    walletMenu_->addAction(unlockWalletAction_);
    walletMenu_->addAction(lockWalletAction_);

    viewMenu_ = menuBar()->addMenu(tr("&View"));
    viewMenu_->addAction(overviewAction_);
    viewMenu_->addAction(sendCoinsAction_);
    viewMenu_->addAction(receiveCoinsAction_);
    viewMenu_->addAction(transactionsAction_);
    viewMenu_->addAction(addressBookAction_);
    viewMenu_->addAction(lightningAction_);
    viewMenu_->addSeparator();
    viewMenu_->addAction(settingsAction_);

    helpMenu_ = menuBar()->addMenu(tr("&Help"));
    helpMenu_->addAction(aboutAction_);
    helpMenu_->addAction(aboutQtAction_);
}

void MainWindow::createToolBar() {
    QToolBar* toolbar = addToolBar(tr("Main toolbar"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(overviewAction_);
    toolbar->addAction(sendCoinsAction_);
    toolbar->addAction(receiveCoinsAction_);
    toolbar->addAction(transactionsAction_);
    toolbar->addAction(addressBookAction_);
    toolbar->addAction(lightningAction_);
}

void MainWindow::createStatusBar() {
    connectionsLabel_ = new QLabel(this);
    connectionsLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(connectionsLabel_);

    blocksLabel_ = new QLabel(this);
    blocksLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(blocksLabel_);

    syncLabel_ = new QLabel(this);
    syncLabel_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(syncLabel_);

    updateStatus();
}

void MainWindow::createPages() {
    centralStack_ = new QStackedWidget(this);
    setCentralWidget(centralStack_);

    overviewPage_ = new OverviewPage(wallet_, blockchain_, centralStack_);
    sendCoinsPage_ = new SendCoinsPage(wallet_, blockchain_, centralStack_);
    receiveCoinsPage_ = new ReceiveCoinsPage(wallet_, centralStack_);
    transactionsPage_ = new TransactionsPage(wallet_, centralStack_);
    addressBookPage_ = new AddressBookPage(wallet_, centralStack_);
    lightningPage_ = new LightningPage(wallet_, blockchain_, p2p_, lightning_, centralStack_);
    settingsPage_ = new SettingsPage(wallet_, centralStack_);

    centralStack_->addWidget(overviewPage_);
    centralStack_->addWidget(sendCoinsPage_);
    centralStack_->addWidget(receiveCoinsPage_);
    centralStack_->addWidget(transactionsPage_);
    centralStack_->addWidget(addressBookPage_);
    centralStack_->addWidget(lightningPage_);
    centralStack_->addWidget(settingsPage_);
}

void MainWindow::showOverviewPage() {
    centralStack_->setCurrentWidget(overviewPage_);
    overviewPage_->updateBalance();
    overviewPage_->updateRecentTransactions();
}

void MainWindow::showSendCoinsPage() {
    centralStack_->setCurrentWidget(sendCoinsPage_);
}

void MainWindow::showReceiveCoinsPage() {
    centralStack_->setCurrentWidget(receiveCoinsPage_);
    receiveCoinsPage_->updateAddressList();
}

void MainWindow::showTransactionsPage() {
    centralStack_->setCurrentWidget(transactionsPage_);
    transactionsPage_->updateTransactionList();
}

void MainWindow::showAddressBookPage() {
    centralStack_->setCurrentWidget(addressBookPage_);
    addressBookPage_->updateAddressList();
}

void MainWindow::showLightningPage() {
    centralStack_->setCurrentWidget(lightningPage_);
    lightningPage_->refreshData();
}

void MainWindow::showSettingsPage() {
    centralStack_->setCurrentWidget(settingsPage_);
}

void MainWindow::aboutINTcoin() {
    QMessageBox::about(this, tr("About INTcoin Core"),
        tr("<h3>INTcoin Core Wallet v1.0.0-alpha</h3>"
           "<p>Quantum-resistant cryptocurrency for the post-quantum era.</p>"
           "<p><b>Features:</b></p>"
           "<ul>"
           "<li>Post-quantum cryptography (Dilithium3, Kyber768)</li>"
           "<li>ASIC-resistant mining (RandomX)</li>"
           "<li>Lightning Network support</li>"
           "<li>HD wallet with BIP39 mnemonic</li>"
           "</ul>"
           "<p>Copyright &copy; 2025 INTcoin Team (Neil Adamson)</p>"
           "<p>Licensed under the MIT License</p>"
           "<p><a href='https://international-coin.org'>https://international-coin.org</a></p>"));
}

void MainWindow::updateStatus() {
    if (!p2p_ || !blockchain_) {
        return;
    }

    // Update connections count
    size_t connections = p2p_->GetPeerCount();
    connectionsLabel_->setText(tr("Connections: %1").arg(connections));

    // Update block count
    uint64_t blockHeight = blockchain_->GetBestHeight();
    blocksLabel_->setText(tr("Blocks: %1").arg(blockHeight));

    // Update sync status using verification progress from blockchain info
    auto info = blockchain_->GetInfo();
    bool syncing = (info.verification_progress < 0.9999); // Consider synced if >99.99% complete
    if (syncing && connections > 0) {
        // Show sync progress percentage
        syncLabel_->setText(tr("Synchronizing... %1%").arg(
            QString::number(info.verification_progress * 100.0, 'f', 2)));
    } else if (connections == 0) {
        syncLabel_->setText(tr("No connections"));
    } else {
        syncLabel_->setText(tr("Up to date"));
    }

    // Update balance on Overview page if visible
    if (overviewPage_ && centralStack_->currentWidget() == overviewPage_) {
        overviewPage_->updateBalance();
        overviewPage_->updateRecentTransactions();
    }
}

void MainWindow::createNewWallet() {
    // Create dialog for password input
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Create New Wallet"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* infoLabel = new QLabel(tr("A new HD wallet will be created with a BIP39 mnemonic phrase.\n"
                                       "Please enter a strong password to encrypt your wallet:"));
    layout->addWidget(infoLabel);

    QLineEdit* passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Enter password (min 8 characters)"));
    layout->addWidget(passwordEdit);

    QLineEdit* confirmEdit = new QLineEdit();
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setPlaceholderText(tr("Confirm password"));
    layout->addWidget(confirmEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString password = passwordEdit->text();
        QString confirm = confirmEdit->text();

        if (password != confirm) {
            QMessageBox::warning(this, tr("Error"), tr("Passwords do not match"));
            return;
        }

        if (password.length() < 8) {
            QMessageBox::warning(this, tr("Error"),
                tr("Password must be at least 8 characters"));
            return;
        }

        // Generate new mnemonic (wallet will generate internally)
        std::vector<std::string> mnemonic;  // Empty = auto-generate

        // Create new wallet
        auto create_result = wallet_->Create(mnemonic, "");
        if (create_result.IsError()) {
            QMessageBox::critical(this, tr("Error"),
                QString::fromStdString("Failed to create wallet: " + create_result.error));
            return;
        }

        // Encrypt wallet
        auto encrypt_result = wallet_->Encrypt(password.toStdString());
        if (encrypt_result.IsError()) {
            QMessageBox::critical(this, tr("Error"),
                QString::fromStdString("Failed to encrypt wallet: " + encrypt_result.error));
            return;
        }

        // Get mnemonic for backup
        auto mnemonic_result = wallet_->GetMnemonic();
        if (mnemonic_result.IsOk()) {
            auto mnemonic_words = mnemonic_result.GetValue();

            // Show mnemonic phrase
            QString mnemonic_text;
            for (size_t i = 0; i < mnemonic_words.size(); i++) {
                mnemonic_text += QString::number(i + 1) + ". " +
                                QString::fromStdString(mnemonic_words[i]) + "\n";
            }

            QMessageBox::information(this, tr("Backup Your Wallet"),
                tr("IMPORTANT: Write down this recovery phrase and store it safely.\n\n"
                   "Recovery Phrase:\n\n%1\n\n"
                   "Anyone with this phrase can restore your wallet!").arg(mnemonic_text));
        }

        QMessageBox::information(this, tr("Success"),
            tr("New wallet created and encrypted successfully!"));
    }
}

void MainWindow::openWallet() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Wallet"), "", tr("Wallet Files (*.dat)"));

    if (!fileName.isEmpty()) {
        // Close current wallet if loaded
        if (wallet_->IsLoaded()) {
            auto close_result = wallet_->Close();
            if (close_result.IsError()) {
                QMessageBox::warning(this, tr("Warning"),
                    QString::fromStdString("Failed to close current wallet: " + close_result.error));
            }
        }

        // Load new wallet
        // Note: This requires wallet path configuration, which may need wallet config update
        QMessageBox::information(this, tr("Open Wallet"),
            tr("Wallet file selected: %1\n\nNote: Full wallet loading requires "
               "wallet path configuration update.").arg(fileName));
    }
}

void MainWindow::backupWallet() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Backup Wallet"), "wallet-backup.dat", tr("Wallet Files (*.dat)"));

    if (!fileName.isEmpty()) {
        auto result = wallet_->BackupWallet(fileName.toStdString());

        if (result.IsOk()) {
            QMessageBox::information(this, tr("Success"),
                tr("Wallet backed up successfully to:\n%1").arg(fileName));
        } else {
            QMessageBox::critical(this, tr("Error"),
                QString::fromStdString("Backup failed: " + result.error));
        }
    }
}

void MainWindow::encryptWallet() {
    if (wallet_->IsEncrypted()) {
        QMessageBox::information(this, tr("Already Encrypted"),
            tr("Wallet is already encrypted."));
        return;
    }

    // Get password
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("Encrypt Wallet"));
    dialog.setLabelText(tr("Enter new password (min 8 characters):"));
    dialog.setTextEchoMode(QLineEdit::Password);

    if (dialog.exec() == QDialog::Accepted) {
        QString password = dialog.textValue();

        if (password.length() < 8) {
            QMessageBox::warning(this, tr("Error"),
                tr("Password must be at least 8 characters"));
            return;
        }

        // Confirm password
        QInputDialog confirmDialog(this);
        confirmDialog.setWindowTitle(tr("Confirm Password"));
        confirmDialog.setLabelText(tr("Confirm password:"));
        confirmDialog.setTextEchoMode(QLineEdit::Password);

        if (confirmDialog.exec() == QDialog::Accepted) {
            QString confirm = confirmDialog.textValue();

            if (password != confirm) {
                QMessageBox::warning(this, tr("Error"),
                    tr("Passwords do not match"));
                return;
            }

            auto result = wallet_->Encrypt(password.toStdString());

            if (result.IsOk()) {
                QMessageBox::information(this, tr("Success"),
                    tr("Wallet encrypted successfully!\n\nYou will need this password "
                       "to unlock your wallet and send transactions."));
            } else {
                QMessageBox::critical(this, tr("Error"),
                    QString::fromStdString("Encryption failed: " + result.error));
            }
        }
    }
}

void MainWindow::unlockWallet() {
    if (!wallet_->IsEncrypted()) {
        QMessageBox::information(this, tr("Not Encrypted"),
            tr("Wallet is not encrypted."));
        return;
    }

    if (!wallet_->IsLocked()) {
        QMessageBox::information(this, tr("Already Unlocked"),
            tr("Wallet is already unlocked."));
        return;
    }

    // Get password
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("Unlock Wallet"));
    dialog.setLabelText(tr("Enter wallet password:"));
    dialog.setTextEchoMode(QLineEdit::Password);

    if (dialog.exec() == QDialog::Accepted) {
        QString password = dialog.textValue();

        auto result = wallet_->Unlock(password.toStdString(), 0);  // 0 = no timeout

        if (result.IsOk()) {
            QMessageBox::information(this, tr("Success"),
                tr("Wallet unlocked successfully!"));
        } else {
            QMessageBox::critical(this, tr("Error"),
                QString::fromStdString("Unlock failed: " + result.error));
        }
    }
}

void MainWindow::lockWallet() {
    if (!wallet_->IsEncrypted()) {
        QMessageBox::information(this, tr("Not Encrypted"),
            tr("Wallet is not encrypted."));
        return;
    }

    if (wallet_->IsLocked()) {
        QMessageBox::information(this, tr("Already Locked"),
            tr("Wallet is already locked."));
        return;
    }

    auto result = wallet_->Lock();

    if (result.IsOk()) {
        QMessageBox::information(this, tr("Success"),
            tr("Wallet locked successfully."));
    } else {
        QMessageBox::critical(this, tr("Error"),
            QString::fromStdString("Lock failed: " + result.error));
    }
}

} // namespace qt
} // namespace intcoin
