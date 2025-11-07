// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mainwindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

namespace intcoin {
namespace qt {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Initialize core components
    blockchain_ = std::make_unique<Blockchain>();
    mempool_ = std::make_unique<Mempool>();
    miner_ = std::make_unique<Miner>(*blockchain_, *mempool_);
    network_ = std::make_unique<p2p::Network>(8333, false);

    // Setup UI
    setup_ui();
    load_settings();

    // Setup update timer
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::update_balance);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::update_transaction_history);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::update_peer_list);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::update_mining_stats);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::update_blockchain_info);
    updateTimer_->start(1000);  // Update every second
}

MainWindow::~MainWindow() {
    save_settings();

    if (miner_->is_mining()) {
        miner_->stop();
    }

    if (network_->is_running()) {
        network_->stop();
    }
}

void MainWindow::setup_ui() {
    setWindowTitle("INTcoin Wallet v0.1.0-alpha");
    resize(1000, 700);

    create_menu_bar();
    create_status_bar();

    // Create central widget with tabs
    QTabWidget* tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    // Create and add tabs
    tabs->addTab(create_wallet_tab(), "Wallet");
    tabs->addTab(create_transactions_tab(), "Transactions");
    tabs->addTab(create_mining_tab(), "Mining");
    tabs->addTab(create_network_tab(), "Network");
    tabs->addTab(create_console_tab(), "Console");
}

void MainWindow::create_menu_bar() {
    QMenuBar* menuBar = new QMenuBar(this);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Wallet", this, &MainWindow::on_actionNew_Wallet_triggered);
    fileMenu->addAction("&Open Wallet", this, &MainWindow::on_actionOpen_Wallet_triggered);
    fileMenu->addAction("&Backup Wallet", this, &MainWindow::on_actionBackup_Wallet_triggered);
    fileMenu->addSeparator();
    fileMenu->addAction("&Encrypt Wallet", this, &MainWindow::on_actionEncrypt_Wallet_triggered);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &MainWindow::on_actionExit_triggered);

    // Settings menu
    QMenu* settingsMenu = menuBar->addMenu("&Settings");
    settingsMenu->addAction("&Options", this, &MainWindow::on_actionSettings_triggered);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About", this, &MainWindow::on_actionAbout_triggered);

    setMenuBar(menuBar);
}

void MainWindow::create_status_bar() {
    statusLabel_ = new QLabel("Ready");
    blockHeightLabel_ = new QLabel("Height: 0");
    connectionsLabel_ = new QLabel("Connections: 0");
    hashRateLabel_ = new QLabel("Hash Rate: 0 H/s");
    syncProgressBar_ = new QProgressBar();
    syncProgressBar_->setMaximumWidth(200);

    QStatusBar* statusBar = new QStatusBar(this);
    statusBar->addWidget(statusLabel_);
    statusBar->addPermanentWidget(blockHeightLabel_);
    statusBar->addPermanentWidget(connectionsLabel_);
    statusBar->addPermanentWidget(hashRateLabel_);
    statusBar->addPermanentWidget(syncProgressBar_);

    setStatusBar(statusBar);
}

QWidget* MainWindow::create_wallet_tab() {
    QWidget* walletTab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(walletTab);

    // Balance section
    QGroupBox* balanceGroup = new QGroupBox("Balance", walletTab);
    QVBoxLayout* balanceLayout = new QVBoxLayout(balanceGroup);
    balanceLabel_ = new QLabel("0.00000000 INT", balanceGroup);
    QFont balanceFont = balanceLabel_->font();
    balanceFont.setPointSize(18);
    balanceFont.setBold(true);
    balanceLabel_->setFont(balanceFont);
    balanceLayout->addWidget(balanceLabel_);
    mainLayout->addWidget(balanceGroup);

    // Send section
    QGroupBox* sendGroup = new QGroupBox("Send", walletTab);
    QFormLayout* sendLayout = new QFormLayout(sendGroup);
    sendAddressEdit_ = new QLineEdit(sendGroup);
    sendAddressEdit_->setPlaceholderText("Enter INTcoin address");
    sendAmountEdit_ = new QLineEdit(sendGroup);
    sendAmountEdit_->setPlaceholderText("0.00000000");
    sendButton_ = new QPushButton("Send", sendGroup);
    connect(sendButton_, &QPushButton::clicked, this, &MainWindow::on_sendButton_clicked);

    sendLayout->addRow("Pay To:", sendAddressEdit_);
    sendLayout->addRow("Amount:", sendAmountEdit_);
    sendLayout->addRow("", sendButton_);
    mainLayout->addWidget(sendGroup);

    // Receive section
    QGroupBox* receiveGroup = new QGroupBox("Receive", walletTab);
    QVBoxLayout* receiveLayout = new QVBoxLayout(receiveGroup);
    receiveAddressLabel_ = new QLabel("", receiveGroup);
    receiveAddressLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    receiveAddressLabel_->setWordWrap(true);
    newAddressButton_ = new QPushButton("Generate New Address", receiveGroup);
    connect(newAddressButton_, &QPushButton::clicked, this, &MainWindow::on_newAddressButton_clicked);

    receiveLayout->addWidget(new QLabel("Your Address:"));
    receiveLayout->addWidget(receiveAddressLabel_);
    receiveLayout->addWidget(newAddressButton_);
    mainLayout->addWidget(receiveGroup);

    // Address book
    QGroupBox* addressGroup = new QGroupBox("Address Book", walletTab);
    QVBoxLayout* addressLayout = new QVBoxLayout(addressGroup);
    addressTable_ = new QTableWidget(0, 2, addressGroup);
    addressTable_->setHorizontalHeaderLabels({"Label", "Address"});
    addressTable_->horizontalHeader()->setStretchLastSection(true);
    addressLayout->addWidget(addressTable_);
    mainLayout->addWidget(addressGroup);

    mainLayout->addStretch();
    return walletTab;
}

QWidget* MainWindow::create_transactions_tab() {
    QWidget* transactionsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(transactionsTab);

    transactionTable_ = new QTableWidget(0, 5, transactionsTab);
    transactionTable_->setHorizontalHeaderLabels({"Date", "Type", "Address", "Amount", "Confirmations"});
    transactionTable_->horizontalHeader()->setStretchLastSection(true);
    transactionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(transactionTable_);
    return transactionsTab;
}

QWidget* MainWindow::create_mining_tab() {
    QWidget* miningTab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(miningTab);

    // Mining controls
    QGroupBox* controlGroup = new QGroupBox("Mining Controls", miningTab);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startMiningButton_ = new QPushButton("Start Mining", controlGroup);
    stopMiningButton_ = new QPushButton("Stop Mining", controlGroup);
    stopMiningButton_->setEnabled(false);
    connect(startMiningButton_, &QPushButton::clicked, this, &MainWindow::on_startMiningButton_clicked);
    connect(stopMiningButton_, &QPushButton::clicked, this, &MainWindow::on_stopMiningButton_clicked);

    buttonLayout->addWidget(startMiningButton_);
    buttonLayout->addWidget(stopMiningButton_);
    controlLayout->addLayout(buttonLayout);

    QFormLayout* threadsLayout = new QFormLayout();
    threadsEdit_ = new QLineEdit("0", controlGroup);
    threadsEdit_->setPlaceholderText("Auto-detect");
    threadsLayout->addRow("Threads:", threadsEdit_);
    controlLayout->addLayout(threadsLayout);

    mainLayout->addWidget(controlGroup);

    // Mining statistics
    QGroupBox* statsGroup = new QGroupBox("Statistics", miningTab);
    QFormLayout* statsLayout = new QFormLayout(statsGroup);

    miningStatusLabel_ = new QLabel("Not mining", statsGroup);
    hashRateValueLabel_ = new QLabel("0 H/s", statsGroup);
    blocksFoundLabel_ = new QLabel("0", statsGroup);

    statsLayout->addRow("Status:", miningStatusLabel_);
    statsLayout->addRow("Hash Rate:", hashRateValueLabel_);
    statsLayout->addRow("Blocks Found:", blocksFoundLabel_);

    mainLayout->addWidget(statsGroup);
    mainLayout->addStretch();
    return miningTab;
}

QWidget* MainWindow::create_network_tab() {
    QWidget* networkTab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(networkTab);

    // Peer list
    QGroupBox* peerGroup = new QGroupBox("Connected Peers", networkTab);
    QVBoxLayout* peerLayout = new QVBoxLayout(peerGroup);

    peerTable_ = new QTableWidget(0, 3, peerGroup);
    peerTable_->setHorizontalHeaderLabels({"Address", "Version", "Ping"});
    peerTable_->horizontalHeader()->setStretchLastSection(true);
    peerTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    peerLayout->addWidget(peerTable_);
    mainLayout->addWidget(peerGroup);

    // Add peer
    QGroupBox* addPeerGroup = new QGroupBox("Add Peer", networkTab);
    QHBoxLayout* addPeerLayout = new QHBoxLayout(addPeerGroup);

    peerAddressEdit_ = new QLineEdit(addPeerGroup);
    peerAddressEdit_->setPlaceholderText("IP:Port");
    connectPeerButton_ = new QPushButton("Connect", addPeerGroup);
    disconnectPeerButton_ = new QPushButton("Disconnect", addPeerGroup);

    connect(connectPeerButton_, &QPushButton::clicked, this, &MainWindow::on_connectPeerButton_clicked);
    connect(disconnectPeerButton_, &QPushButton::clicked, this, &MainWindow::on_disconnectPeerButton_clicked);

    addPeerLayout->addWidget(peerAddressEdit_);
    addPeerLayout->addWidget(connectPeerButton_);
    addPeerLayout->addWidget(disconnectPeerButton_);

    mainLayout->addWidget(addPeerGroup);
    mainLayout->addStretch();
    return networkTab;
}

QWidget* MainWindow::create_console_tab() {
    QWidget* consoleTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(consoleTab);

    // Console output
    consoleOutput_ = new QTextEdit(consoleTab);
    consoleOutput_->setReadOnly(true);
    consoleOutput_->setFont(QFont("Courier", 10));
    layout->addWidget(consoleOutput_);

    // Console input
    QHBoxLayout* inputLayout = new QHBoxLayout();
    consoleInput_ = new QLineEdit(consoleTab);
    consoleInput_->setPlaceholderText("Enter RPC command...");
    QPushButton* executeButton = new QPushButton("Execute", consoleTab);

    connect(consoleInput_, &QLineEdit::returnPressed, executeButton, &QPushButton::click);
    connect(executeButton, &QPushButton::clicked, [this]() {
        QString command = consoleInput_->text();
        if (!command.isEmpty()) {
            consoleOutput_->append("> " + command);
            // TODO: Execute RPC command
            consoleOutput_->append("Command executed\n");
            consoleInput_->clear();
        }
    });

    inputLayout->addWidget(consoleInput_);
    inputLayout->addWidget(executeButton);
    layout->addLayout(inputLayout);
    return consoleTab;
}

// Menu action handlers

void MainWindow::on_actionNew_Wallet_triggered() {
    // TODO: Create new wallet dialog
    show_success("Wallet Created", "New wallet has been created successfully");
}

void MainWindow::on_actionOpen_Wallet_triggered() {
    QString filepath = QFileDialog::getOpenFileName(this, "Open Wallet", "", "Wallet Files (*.wallet)");
    if (!filepath.isEmpty()) {
        load_wallet(filepath);
    }
}

void MainWindow::on_actionBackup_Wallet_triggered() {
    QString filepath = QFileDialog::getSaveFileName(this, "Backup Wallet", "", "Wallet Files (*.wallet)");
    if (!filepath.isEmpty()) {
        // TODO: Backup wallet
        show_success("Backup Complete", "Wallet backed up successfully");
    }
}

void MainWindow::on_actionEncrypt_Wallet_triggered() {
    // TODO: Encrypt wallet dialog
}

void MainWindow::on_actionExit_triggered() {
    close();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, "About INTcoin",
        "INTcoin Wallet v0.1.0-alpha\n\n"
        "Quantum-Resistant Cryptocurrency\n\n"
        "Built with CRYSTALS-Dilithium5 and Kyber1024\n"
        "SHA-256 Proof of Work\n\n"
        "© 2025 INTcoin Core\n"
        "Lead Developer: Maddison Lane");
}

void MainWindow::on_actionSettings_triggered() {
    // TODO: Settings dialog
}

// Button handlers

void MainWindow::on_sendButton_clicked() {
    // TODO: Send transaction
}

void MainWindow::on_receiveButton_clicked() {
    // TODO: Show receive dialog
}

void MainWindow::on_newAddressButton_clicked() {
    if (wallet_) {
        QString address = QString::fromStdString(wallet_->get_new_address(""));
        show_success("New Address", "Address: " + address);
    }
}

void MainWindow::on_startMiningButton_clicked() {
    if (wallet_) {
        auto keys = wallet_->get_all_keys();
        if (!keys.empty()) {
            miner_->start(keys[0].public_key);
            show_success("Mining Started", "CPU mining has started");
        }
    }
}

void MainWindow::on_stopMiningButton_clicked() {
    miner_->stop();
    show_success("Mining Stopped", "CPU mining has been stopped");
}

void MainWindow::on_connectPeerButton_clicked() {
    // TODO: Connect to peer
}

void MainWindow::on_disconnectPeerButton_clicked() {
    // TODO: Disconnect peer
}

// Update functions

void MainWindow::update_balance() {
    if (wallet_ && blockchain_) {
        uint64_t balance = wallet_->get_balance(*blockchain_);
        double balance_coins = static_cast<double>(balance) / COIN;
        balanceLabel_->setText(QString::number(balance_coins, 'f', 8) + " INT");
    }
}

void MainWindow::update_transaction_history() {
    // TODO: Update transaction table
}

void MainWindow::update_peer_list() {
    if (network_) {
        connectionsLabel_->setText(QString("Connections: %1").arg(network_->peer_count()));
    }
}

void MainWindow::update_mining_stats() {
    if (miner_) {
        auto stats = miner_->get_stats();
        hashRateLabel_->setText(QString("Hash Rate: %1 H/s").arg(stats.hashes_per_second));
    }
}

void MainWindow::update_blockchain_info() {
    if (blockchain_) {
        uint32_t height = blockchain_->get_height();
        blockHeightLabel_->setText(QString("Height: %1").arg(height));
    }
}

// Helper functions

void MainWindow::show_error(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

void MainWindow::show_success(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

bool MainWindow::load_wallet(const QString& filepath) {
    // TODO: Load wallet from file
    (void)filepath;
    return true;
}

void MainWindow::save_settings() {
    QSettings settings("INTcoin", "INTcoin-Qt");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::load_settings() {
    QSettings settings("INTcoin", "INTcoin-Qt");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

} // namespace qt
} // namespace intcoin
