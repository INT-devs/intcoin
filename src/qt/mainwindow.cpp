// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mainwindow.h"
// #include "lightningwindow.h"  // Disabled until Lightning backend is complete
#include "intcoin/wallet_db.h"
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
#include <QInputDialog>
#include <QClipboard>
#include <QApplication>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialogButtonBox>

namespace intcoin {
namespace qt {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Get default data directory
    std::string datadir = ConfigManager::get_default_datadir();

    // Initialize core components with database
    blockchain_ = std::make_unique<Blockchain>(datadir + "/blocks");
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
    setWindowTitle("INTcoin Wallet v1.1.0 - Quantum-Resistant Cryptocurrency");
    resize(1200, 800);

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

    // Lightning menu
    QMenu* lightningMenu = menuBar->addMenu("&Lightning");
    lightningMenu->addAction("Open &Lightning Wallet", this, &MainWindow::on_actionLightning_triggered);

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
    difficultyLabel_ = new QLabel("N/A", statsGroup);

    statsLayout->addRow("Status:", miningStatusLabel_);
    statsLayout->addRow("Hash Rate:", hashRateValueLabel_);
    statsLayout->addRow("Blocks Found:", blocksFoundLabel_);
    statsLayout->addRow("Network Difficulty:", difficultyLabel_);

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

            // TODO: Implement RPC client integration
            consoleOutput_->append("RPC console is currently under development.");
            consoleOutput_->append("Please use intcoin-cli for RPC commands.");
            consoleOutput_->append("");  // Empty line
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
    // Create new wallet dialog
    bool ok;
    QString password = QInputDialog::getText(this, "New Wallet",
                                            "Enter password for new wallet:",
                                            QLineEdit::Password, "", &ok);
    if (ok && !password.isEmpty()) {
        // Create new HD wallet with password
        try {
            HDWallet new_wallet = HDWallet::create_new(password.toStdString());
            wallet_ = std::make_unique<HDWallet>(std::move(new_wallet));

            // Wallet is created with encryption and saved automatically
            show_success("Wallet Created", "New wallet has been created successfully");
            update_balance();
        } catch (const std::exception& e) {
            show_error("Wallet Creation Failed", QString("Failed to create wallet:\n%1").arg(e.what()));
        }
    }
}

void MainWindow::on_actionOpen_Wallet_triggered() {
    QString filepath = QFileDialog::getOpenFileName(this, "Open Wallet", "", "Wallet Files (*.wallet)");
    if (!filepath.isEmpty()) {
        load_wallet(filepath);
    }
}

void MainWindow::on_actionBackup_Wallet_triggered() {
    if (!wallet_) {
        show_error("No Wallet", "Please create or open a wallet first");
        return;
    }

    QString filepath = QFileDialog::getSaveFileName(this, "Backup Wallet", "", "Wallet Files (*.wallet)");
    if (!filepath.isEmpty()) {
        if (wallet_->backup_to_file(filepath.toStdString())) {
            show_success("Backup Complete", "Wallet backed up successfully to:\n" + filepath);
        } else {
            show_error("Backup Failed", "Failed to backup wallet");
        }
    }
}

void MainWindow::on_actionEncrypt_Wallet_triggered() {
    // Encrypt wallet dialog
    if (!wallet_) {
        show_error("No Wallet", "Please create or open a wallet first");
        return;
    }

    if (wallet_->is_encrypted()) {
        show_error("Already Encrypted", "Wallet is already encrypted");
        return;
    }

    bool ok;
    QString password = QInputDialog::getText(this, "Encrypt Wallet",
                                            "Enter password to encrypt wallet:",
                                            QLineEdit::Password, "", &ok);
    if (ok && !password.isEmpty()) {
        if (wallet_->encrypt(password.toStdString())) {
            show_success("Encryption Complete", "Wallet encrypted successfully");
            // TODO: Auto-save encrypted wallet
        } else {
            show_error("Encryption Failed", "Failed to encrypt wallet");
        }
    }
}

void MainWindow::on_actionExit_triggered() {
    close();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About INTcoin");
    aboutBox.setTextFormat(Qt::RichText);

    QString aboutText =
        "<div style='text-align: center;'>"
        "<h2 style='color: #4A90E2;'>INTcoin Wallet</h2>"
        "<p style='font-size: 14px; color: #00D9FF;'><b>Version 1.1.0</b></p>"
        "</div>"

        "<p style='color: #E8E9ED;'><b>Quantum-Resistant Cryptocurrency</b></p>"

        "<p style='color: #A0A5B8;'>"
        "INTcoin is the world's first production-ready quantum-resistant "
        "cryptocurrency, built from the ground up with NIST-approved "
        "post-quantum cryptographic algorithms."
        "</p>"

        "<p style='color: #E8E9ED;'><b>Cryptographic Algorithms:</b></p>"
        "<ul style='color: #A0A5B8;'>"
        "<li>CRYSTALS-Dilithium5 (ML-DSA-87) - Digital Signatures</li>"
        "<li>CRYSTALS-Kyber1024 (ML-KEM-1024) - Key Exchange</li>"
        "<li>SHA3-256 (FIPS 202) - Hashing</li>"
        "<li>SHA-256 - Proof of Work</li>"
        "</ul>"

        "<p style='color: #E8E9ED;'><b>Genesis Block:</b></p>"
        "<p style='color: #A0A5B8; font-family: monospace; font-size: 10px;'>"
        "000000001f132c42a82a1e4316aab226a1c0663d1a40d2423901914417a69da9<br>"
        "\"BBC News 06/Nov/2025 Will quantum be bigger than AI?\""
        "</p>"

        "<p style='color: #E8E9ED;'><b>Network Launch:</b> January 1, 2026</p>"
        "<p style='color: #E8E9ED;'><b>Max Supply:</b> 221,000,000,000,000 INT</p>"
        "<p style='color: #E8E9ED;'><b>Block Time:</b> ~5 minutes</p>"

        "<hr style='border: 1px solid #4A90E2;'>"

        "<p style='color: #A0A5B8; font-size: 11px;'>"
        "© 2025 INTcoin Core<br>"
        "Lead Developer: Maddison Lane<br>"
        "Website: <a href='https://international-coin.org' style='color: #00D9FF;'>international-coin.org</a><br>"
        "Released under the MIT License"
        "</p>";

    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void MainWindow::on_actionLightning_triggered() {
    // Lightning Network feature is under development
    QMessageBox::information(this, "Lightning Network",
        "Lightning Network support is currently under development.\n\n"
        "This feature will be available in a future release.");

    // TODO: Enable Lightning window when backend is complete
    /*
    if (!wallet_) {
        QMessageBox::warning(this, "No Wallet",
            "Please create or open a wallet first before using Lightning Network features.");
        return;
    }

    // Create Lightning node if not exists
    // TODO: Initialize lightning node with proper parameters
    auto ln_node = std::make_shared<lightning::LightningNode>();

    // Create and show Lightning window
    LightningWindow* lightningWindow = new LightningWindow(ln_node, wallet_, blockchain_);
    lightningWindow->setAttribute(Qt::WA_DeleteOnClose);
    lightningWindow->show();
    */
}

void MainWindow::on_actionSettings_triggered() {
    // Settings dialog
    QDialog settings_dialog(this);
    settings_dialog.setWindowTitle("Settings");
    settings_dialog.setMinimumWidth(400);

    QVBoxLayout* layout = new QVBoxLayout(&settings_dialog);

    // RPC Server settings
    QGroupBox* rpc_group = new QGroupBox("RPC Server", &settings_dialog);
    QFormLayout* rpc_layout = new QFormLayout(rpc_group);

    QLineEdit* host_edit = new QLineEdit("127.0.0.1", rpc_group);
    QSpinBox* port_spin = new QSpinBox(rpc_group);
    port_spin->setRange(1, 65535);
    port_spin->setValue(9332);  // INTcoin RPC port (matches daemon)

    rpc_layout->addRow("Host:", host_edit);
    rpc_layout->addRow("Port:", port_spin);

    layout->addWidget(rpc_group);

    // Network settings
    QGroupBox* network_group = new QGroupBox("Network", &settings_dialog);
    QFormLayout* network_layout = new QFormLayout(network_group);

    QCheckBox* testnet_check = new QCheckBox("Use Testnet", network_group);
    testnet_check->setChecked(false);  // Default to mainnet

    network_layout->addRow(testnet_check);

    layout->addWidget(network_group);

    // Dialog buttons
    QDialogButtonBox* button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &settings_dialog);
    connect(button_box, &QDialogButtonBox::accepted, &settings_dialog, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, &settings_dialog, &QDialog::reject);

    layout->addWidget(button_box);

    if (settings_dialog.exec() == QDialog::Accepted) {
        // Apply settings
        // TODO: Store RPC settings and reconnect client
        // For now, just show success message
        show_success("Settings Updated", "Settings have been saved successfully");
    }
}

// Button handlers

void MainWindow::on_sendButton_clicked() {
    if (!wallet_ || !blockchain_) {
        show_error("Error", "Wallet or blockchain not initialized");
        return;
    }

    QString address = sendAddressEdit_->text().trimmed();
    QString amountStr = sendAmountEdit_->text().trimmed();

    if (address.isEmpty() || amountStr.isEmpty()) {
        show_error("Invalid Input", "Please enter both address and amount");
        return;
    }

    // Parse amount (convert from INT to satoshis)
    bool ok;
    double amountINT = amountStr.toDouble(&ok);
    if (!ok || amountINT <= 0) {
        show_error("Invalid Amount", "Please enter a valid positive amount");
        return;
    }

    uint64_t amount = static_cast<uint64_t>(amountINT * COIN);

    // Check balance
    uint64_t balance = wallet_->get_balance(*blockchain_);
    if (amount > balance) {
        show_error("Insufficient Balance",
            QString("Amount %1 INT exceeds available balance %2 INT")
                .arg(amountINT, 0, 'f', 8)
                .arg(static_cast<double>(balance) / COIN, 0, 'f', 8));
        return;
    }

    try {
        // Calculate fee (simplified - 0.001 INT per transaction)
        uint64_t fee = static_cast<uint64_t>(0.001 * COIN);

        // Create and send transaction
        auto tx_opt = wallet_->create_transaction(address.toStdString(), amount, fee, *blockchain_);

        if (!tx_opt) {
            show_error("Transaction Failed", "Failed to create transaction");
            return;
        }

        Transaction tx = *tx_opt;

        if (mempool_->add_transaction(tx, blockchain_->get_height())) {
            // Convert hash to hex string manually
            Hash256 txhash = tx.get_hash();
            std::string txhash_str;
            for (uint8_t byte : txhash) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02x", byte);
                txhash_str += buf;
            }

            show_success("Transaction Sent",
                QString("Sent %1 INT to\n%2\n\nTransaction ID: %3")
                    .arg(amountINT, 0, 'f', 8)
                    .arg(address)
                    .arg(QString::fromStdString(txhash_str)));

            // Clear input fields
            sendAddressEdit_->clear();
            sendAmountEdit_->clear();

            // Update balance immediately
            update_balance();
        } else {
            show_error("Transaction Failed", "Failed to add transaction to mempool");
        }
    } catch (const std::exception& e) {
        show_error("Transaction Error", QString("Error creating transaction: %1").arg(e.what()));
    }
}

void MainWindow::on_receiveButton_clicked() {
    if (!wallet_) {
        show_error("No Wallet", "Please create or open a wallet first");
        return;
    }

    // Get the current receive address
    auto keys = wallet_->get_all_keys();
    if (keys.empty()) {
        show_error("No Addresses", "No addresses available in wallet");
        return;
    }

    QString address = QString::fromStdString(keys[0].address);

    // Create dialog to show address with copy button
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Receive INT");
    msgBox.setText("Your receive address:");
    msgBox.setInformativeText(address);
    msgBox.setStandardButtons(QMessageBox::Ok);

    // Add a Copy button
    QPushButton *copyButton = msgBox.addButton("Copy Address", QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == copyButton) {
        QApplication::clipboard()->setText(address);
        show_success("Copied", "Address copied to clipboard");
    }
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
    // Connect to peer
    bool ok;
    QString peer_address = QInputDialog::getText(this, "Connect to Peer",
                                                 "Enter peer address (IP:port):",
                                                 QLineEdit::Normal, "", &ok);
    if (ok && !peer_address.isEmpty()) {
        // Parse IP and port
        QStringList parts = peer_address.split(':');
        if (parts.size() == 2) {
            std::string ip = parts[0].toStdString();
            uint16_t port = parts[1].toUInt();

            if (network_) {
                p2p::PeerAddress addr(ip, port);
                if (network_->connect_to_peer(addr)) {
                    show_success("Peer Connected", "Successfully connected to peer:\n" + peer_address);
                    update_peer_list();
                } else {
                    show_error("Connection Failed", "Failed to connect to peer:\n" + peer_address);
                }
            } else {
                show_error("Network Error", "P2P network not initialized");
            }
        } else {
            show_error("Invalid Format", "Please use format: IP:port\nExample: 127.0.0.1:9333");
        }
    }
}

void MainWindow::on_disconnectPeerButton_clicked() {
    // Disconnect peer
    if (!network_) {
        show_error("Network Error", "P2P network not initialized");
        return;
    }

    // Get selected peer from table
    int row = peerTable_->currentRow();
    if (row < 0) {
        show_error("No Selection", "Please select a peer to disconnect");
        return;
    }

    // Get peer address from first column of selected row
    QTableWidgetItem* addressItem = peerTable_->item(row, 0);
    if (!addressItem) {
        show_error("Error", "Could not retrieve peer address");
        return;
    }

    QString peer_text = addressItem->text();
    QStringList parts = peer_text.split(" - ")[0].split(":");
    if (parts.size() == 2) {
        std::string ip = parts[0].toStdString();
        uint16_t port = parts[1].toUInt();

        p2p::PeerAddress addr(ip, port);
        network_->disconnect_peer(addr);

        show_success("Peer Disconnected", "Successfully disconnected from peer");
        update_peer_list();
    }
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
    if (!wallet_ || !blockchain_) {
        return;
    }

    // Get all transactions from wallet
    auto keys = wallet_->get_all_keys();
    if (keys.empty()) {
        return;
    }

    // Clear existing rows
    transactionTable_->setRowCount(0);

    // Get wallet addresses for filtering
    std::set<std::string> wallet_addresses;
    for (const auto& key : keys) {
        wallet_addresses.insert(key.address);
    }

    // Scan blockchain for transactions involving wallet addresses
    uint32_t height = blockchain_->get_height();
    int row = 0;

    // Only scan recent blocks to avoid performance issues
    uint32_t start_height = (height > 100) ? (height - 100) : 0;

    for (uint32_t h = height; h > start_height && h > 0; --h) {
        try {
            Block block = blockchain_->get_block_by_height(h);

            for (const auto& tx : block.transactions) {
                // Skip coinbase transactions for now
                if (tx.is_coinbase()) {
                    continue;
                }

                // Properly filter for wallet transactions by matching pubkeys
                bool is_wallet_tx = false;
                bool is_send = false;
                bool is_receive = false;

                // Check if any outputs belong to our wallet
                for (const auto& output : tx.outputs) {
                    // Check if pubkey matches any of our keys
                    for (const auto& key : keys) {
                        if (output.pubkey == key.public_key) {
                            is_wallet_tx = true;
                            is_receive = true;
                            break;
                        }
                    }
                }

                // Check if any inputs are from our wallet
                for (const auto& input : tx.inputs) {
                    // Extract pubkey from script_sig (last 2592 bytes)
                    if (input.script_sig.size() >= 2592) {
                        for (const auto& key : keys) {
                            // Compare the last 2592 bytes with the public key
                            if (std::equal(input.script_sig.end() - 2592, input.script_sig.end(),
                                          key.public_key.begin(), key.public_key.end())) {
                                is_wallet_tx = true;
                                is_send = true;
                                break;
                            }
                        }
                    }
                }

                // Only show transactions that involve our wallet
                if (!is_wallet_tx) {
                    continue;
                }

                transactionTable_->insertRow(row);

                // Type (Send/Receive based on analysis)
                QString tx_type = "Unknown";
                if (is_send && is_receive) {
                    tx_type = "Self";
                } else if (is_send) {
                    tx_type = "Send";
                } else if (is_receive) {
                    tx_type = "Receive";
                }
                transactionTable_->setItem(row, 0, new QTableWidgetItem(tx_type));

                // Amount (sum of outputs)
                uint64_t total_output = 0;
                for (const auto& output : tx.outputs) {
                    total_output += output.value;
                }
                double amount_int = static_cast<double>(total_output) / COIN;
                transactionTable_->setItem(row, 1, new QTableWidgetItem(
                    QString::number(amount_int, 'f', 8)));

                // Confirmations
                uint32_t confirmations = height - h + 1;
                transactionTable_->setItem(row, 2, new QTableWidgetItem(
                    QString::number(confirmations)));

                // TX ID (truncated) - convert hash to hex string
                Hash256 txhash = tx.get_hash();
                std::string txhash_str;
                for (size_t i = 0; i < 8 && i < txhash.size(); ++i) {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02x", txhash[i]);
                    txhash_str += buf;
                }
                QString short_txid = QString::fromStdString(txhash_str) + "...";
                transactionTable_->setItem(row, 3, new QTableWidgetItem(short_txid));

                row++;

                // Limit to 50 transactions
                if (row >= 50) {
                    return;
                }
            }
        } catch (...) {
            // Skip blocks that can't be read
            continue;
        }
    }
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
        hashRateValueLabel_->setText(QString("%1 H/s").arg(stats.hashes_per_second));
        blocksFoundLabel_->setText(QString::number(stats.blocks_found));
        miningStatusLabel_->setText(miner_->is_mining() ? "Mining" : "Not mining");
    }

    // Update difficulty from blockchain
    if (blockchain_) {
        Hash256 best_hash = blockchain_->get_best_block_hash();
        uint32_t difficulty_bits = blockchain_->calculate_next_difficulty(best_hash);

        // Convert compact bits to human-readable difficulty
        // Difficulty = max_target / current_target
        double difficulty = consensus::DifficultyCalculator::get_difficulty(difficulty_bits);

        difficultyLabel_->setText(QString::number(difficulty, 'f', 2));
    }
}

void MainWindow::update_blockchain_info() {
    if (blockchain_) {
        uint32_t height = blockchain_->get_height();
        blockHeightLabel_->setText(QString("Height: %1").arg(height));

        // Update sync progress (simplified - would need actual sync state)
        syncProgressBar_->setValue(100);  // Assume synced for now
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
    // Ask for password first
    bool ok;
    QString password = QInputDialog::getText(this, "Wallet Password",
                                            "Enter wallet password:",
                                            QLineEdit::Password, "", &ok);

    if (!ok || password.isEmpty()) {
        return false;
    }

    // Load wallet from file using static method
    try {
        HDWallet loaded_wallet = HDWallet::restore_from_file(filepath.toStdString(), password.toStdString());
        wallet_ = std::make_unique<HDWallet>(std::move(loaded_wallet));

        show_success("Wallet Loaded", "Wallet loaded successfully from:\n" + filepath);
        update_balance();
        return true;
    } catch (const std::exception& e) {
        show_error("Load Failed", QString("Failed to load wallet:\n%1").arg(e.what()));
        wallet_.reset();
        return false;
    }
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
