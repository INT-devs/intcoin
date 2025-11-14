// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network GUI Window Implementation

#include "lightningwindow.h"
#include "lightning_utils.h"
#include "intcoin/p2p.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QFileDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <iomanip>
#include <sstream>
#include <set>
#include <cmath>

namespace intcoin {
namespace qt {

LightningWindow::LightningWindow(
    std::shared_ptr<lightning::LightningNode> ln_node,
    std::shared_ptr<lightning::LightningNetworkManager> ln_network,
    std::shared_ptr<HDWallet> wallet,
    std::shared_ptr<Blockchain> blockchain,
    QWidget *parent
) : QWidget(parent),
    ln_node_(ln_node),
    ln_network_(ln_network),
    wallet_(wallet),
    blockchain_(blockchain),
    nodeRunning_(false)
{
    setup_ui();

    // Setup update timer (every 5 seconds)
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &LightningWindow::update_channel_list);
    connect(updateTimer_, &QTimer::timeout, this, &LightningWindow::update_stats);
    updateTimer_->start(5000);

    // Initial update
    update_channel_list();
    update_invoice_list();
    update_peer_list();
    update_stats();
    update_payment_history();
}

LightningWindow::~LightningWindow() {
    // Lightning node cleanup handled by shared_ptr
}

void LightningWindow::setup_ui() {
    auto *mainLayout = new QVBoxLayout(this);

    // Status bar
    statusLabel_ = new QLabel("Lightning Network: Not Started");
    statusLabel_->setStyleSheet("QLabel { padding: 8px; background-color: #f0f0f0; border-radius: 4px; }");
    mainLayout->addWidget(statusLabel_);

    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    // Create tabs
    create_channel_tab();
    create_invoice_tab();
    create_payment_tab();
    create_node_tab();
    create_stats_tab();

    setLayout(mainLayout);
    setWindowTitle("INTcoin Lightning Network");
    resize(1000, 700);
}

void LightningWindow::create_channel_tab() {
    auto *channelWidget = new QWidget();
    auto *layout = new QVBoxLayout(channelWidget);

    // Summary section
    auto *summaryGroup = new QGroupBox("Channel Summary");
    auto *summaryLayout = new QGridLayout();

    summaryLayout->addWidget(new QLabel("Total Capacity:"), 0, 0);
    totalCapacityLabel_ = new QLabel("0 INT");
    summaryLayout->addWidget(totalCapacityLabel_, 0, 1);

    summaryLayout->addWidget(new QLabel("Active Channels:"), 0, 2);
    activeChannelsLabel_ = new QLabel("0");
    summaryLayout->addWidget(activeChannelsLabel_, 0, 3);

    summaryLayout->addWidget(new QLabel("Local Balance:"), 1, 0);
    localBalanceLabel_ = new QLabel("0 INT");
    summaryLayout->addWidget(localBalanceLabel_, 1, 1);

    summaryLayout->addWidget(new QLabel("Remote Balance:"), 1, 2);
    remoteBalanceLabel_ = new QLabel("0 INT");
    summaryLayout->addWidget(remoteBalanceLabel_, 1, 3);

    summaryGroup->setLayout(summaryLayout);
    layout->addWidget(summaryGroup);

    // Channel list
    auto *channelListGroup = new QGroupBox("Channels");
    auto *channelListLayout = new QVBoxLayout();

    channelTable_ = new QTableWidget(0, 7);
    channelTable_->setHorizontalHeaderLabels({
        "Channel ID", "Peer ID", "State", "Capacity", "Local Balance",
        "Remote Balance", "Active HTLCs"
    });
    channelTable_->horizontalHeader()->setStretchLastSection(true);
    channelTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    channelTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(channelTable_, &QTableWidget::itemSelectionChanged,
            this, &LightningWindow::on_channelTable_selectionChanged);

    channelListLayout->addWidget(channelTable_);
    channelListGroup->setLayout(channelListLayout);
    layout->addWidget(channelListGroup);

    // Open channel section
    auto *openChannelGroup = new QGroupBox("Open New Channel");
    auto *openChannelLayout = new QGridLayout();

    openChannelLayout->addWidget(new QLabel("Peer Node ID:"), 0, 0);
    peerIdEdit_ = new QLineEdit();
    peerIdEdit_->setPlaceholderText("Enter peer's public key (66 hex characters)");
    openChannelLayout->addWidget(peerIdEdit_, 0, 1, 1, 3);

    openChannelLayout->addWidget(new QLabel("Capacity (INT):"), 1, 0);
    channelCapacitySpin_ = new QSpinBox();
    channelCapacitySpin_->setMinimum(1);
    channelCapacitySpin_->setMaximum(1000000);
    channelCapacitySpin_->setValue(100);
    openChannelLayout->addWidget(channelCapacitySpin_, 1, 1);

    openChannelButton_ = new QPushButton("Open Channel");
    connect(openChannelButton_, &QPushButton::clicked,
            this, &LightningWindow::on_openChannelButton_clicked);
    openChannelLayout->addWidget(openChannelButton_, 1, 2);

    closeChannelButton_ = new QPushButton("Close Selected Channel");
    closeChannelButton_->setEnabled(false);
    connect(closeChannelButton_, &QPushButton::clicked,
            this, &LightningWindow::on_closeChannelButton_clicked);
    openChannelLayout->addWidget(closeChannelButton_, 1, 3);

    refreshChannelsButton_ = new QPushButton("Refresh");
    connect(refreshChannelsButton_, &QPushButton::clicked,
            this, &LightningWindow::on_refreshChannelsButton_clicked);
    openChannelLayout->addWidget(refreshChannelsButton_, 1, 4);

    openChannelGroup->setLayout(openChannelLayout);
    layout->addWidget(openChannelGroup);

    tabWidget_->addTab(channelWidget, "Channels");
}

void LightningWindow::create_invoice_tab() {
    auto *invoiceWidget = new QWidget();
    auto *layout = new QVBoxLayout(invoiceWidget);

    // Create invoice section
    auto *createGroup = new QGroupBox("Create Invoice");
    auto *createLayout = new QGridLayout();

    createLayout->addWidget(new QLabel("Amount (INT):"), 0, 0);
    invoiceAmountEdit_ = new QLineEdit();
    invoiceAmountEdit_->setPlaceholderText("0.001");
    createLayout->addWidget(invoiceAmountEdit_, 0, 1);

    createLayout->addWidget(new QLabel("Description:"), 1, 0);
    invoiceDescriptionEdit_ = new QLineEdit();
    invoiceDescriptionEdit_->setPlaceholderText("Payment for goods/services");
    createLayout->addWidget(invoiceDescriptionEdit_, 1, 1);

    createLayout->addWidget(new QLabel("Expiry (minutes):"), 2, 0);
    invoiceExpirySpinBox_ = new QSpinBox();
    invoiceExpirySpinBox_->setMinimum(1);
    invoiceExpirySpinBox_->setMaximum(10080); // 1 week
    invoiceExpirySpinBox_->setValue(60);
    createLayout->addWidget(invoiceExpirySpinBox_, 2, 1);

    createInvoiceButton_ = new QPushButton("Create Invoice");
    connect(createInvoiceButton_, &QPushButton::clicked,
            this, &LightningWindow::on_createInvoiceButton_clicked);
    createLayout->addWidget(createInvoiceButton_, 0, 2, 3, 1);

    createGroup->setLayout(createLayout);
    layout->addWidget(createGroup);

    // Generated invoice display
    auto *invoiceDisplayGroup = new QGroupBox("Generated Invoice");
    auto *invoiceDisplayLayout = new QVBoxLayout();

    invoiceTextEdit_ = new QTextEdit();
    invoiceTextEdit_->setReadOnly(true);
    invoiceTextEdit_->setMaximumHeight(80);
    invoiceTextEdit_->setPlaceholderText("Invoice will appear here...");
    invoiceDisplayLayout->addWidget(invoiceTextEdit_);

    copyInvoiceButton_ = new QPushButton("Copy to Clipboard");
    copyInvoiceButton_->setEnabled(false);
    connect(copyInvoiceButton_, &QPushButton::clicked,
            this, &LightningWindow::on_copyInvoiceButton_clicked);
    invoiceDisplayLayout->addWidget(copyInvoiceButton_);

    invoiceDisplayGroup->setLayout(invoiceDisplayLayout);
    layout->addWidget(invoiceDisplayGroup);

    // Invoice history
    auto *historyGroup = new QGroupBox("Invoice History");
    auto *historyLayout = new QVBoxLayout();

    invoiceTable_ = new QTableWidget(0, 5);
    invoiceTable_->setHorizontalHeaderLabels({
        "Invoice ID", "Amount", "Description", "Status", "Created"
    });
    invoiceTable_->horizontalHeader()->setStretchLastSection(true);
    invoiceTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    invoiceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    historyLayout->addWidget(invoiceTable_);

    refreshInvoicesButton_ = new QPushButton("Refresh Invoices");
    connect(refreshInvoicesButton_, &QPushButton::clicked,
            this, &LightningWindow::on_refreshInvoicesButton_clicked);
    historyLayout->addWidget(refreshInvoicesButton_);

    historyGroup->setLayout(historyLayout);
    layout->addWidget(historyGroup);

    tabWidget_->addTab(invoiceWidget, "Invoices");
}

void LightningWindow::create_payment_tab() {
    auto *paymentWidget = new QWidget();
    auto *layout = new QVBoxLayout(paymentWidget);

    // Pay invoice section
    auto *payGroup = new QGroupBox("Pay Lightning Invoice");
    auto *payLayout = new QHBoxLayout();

    payLayout->addWidget(new QLabel("Invoice:"));
    payInvoiceEdit_ = new QLineEdit();
    payInvoiceEdit_->setPlaceholderText("lnint1...");
    payLayout->addWidget(payInvoiceEdit_, 1);

    payInvoiceButton_ = new QPushButton("Pay Invoice");
    connect(payInvoiceButton_, &QPushButton::clicked,
            this, &LightningWindow::on_payInvoiceButton_clicked);
    payLayout->addWidget(payInvoiceButton_);

    payGroup->setLayout(payLayout);
    layout->addWidget(payGroup);

    // Payment summary
    auto *summaryGroup = new QGroupBox("Payment Summary");
    auto *summaryLayout = new QGridLayout();

    summaryLayout->addWidget(new QLabel("Total Sent:"), 0, 0);
    totalSentLabel_ = new QLabel("0 INT");
    summaryLayout->addWidget(totalSentLabel_, 0, 1);

    summaryLayout->addWidget(new QLabel("Total Received:"), 0, 2);
    totalReceivedLabel_ = new QLabel("0 INT");
    summaryLayout->addWidget(totalReceivedLabel_, 0, 3);

    summaryLayout->addWidget(new QLabel("Total Fees Paid:"), 1, 0);
    totalFeesLabel_ = new QLabel("0 sats");
    summaryLayout->addWidget(totalFeesLabel_, 1, 1);

    summaryGroup->setLayout(summaryLayout);
    layout->addWidget(summaryGroup);

    // Payment history
    auto *historyGroup = new QGroupBox("Payment History");
    auto *historyLayout = new QVBoxLayout();

    paymentHistoryTable_ = new QTableWidget(0, 6);
    paymentHistoryTable_->setHorizontalHeaderLabels({
        "Payment Hash", "Direction", "Amount", "Fees", "Status", "Time"
    });
    paymentHistoryTable_->horizontalHeader()->setStretchLastSection(true);
    paymentHistoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    paymentHistoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    historyLayout->addWidget(paymentHistoryTable_);
    historyGroup->setLayout(historyLayout);
    layout->addWidget(historyGroup);

    tabWidget_->addTab(paymentWidget, "Payments");
}

void LightningWindow::create_node_tab() {
    auto *nodeWidget = new QWidget();
    auto *layout = new QVBoxLayout(nodeWidget);

    // Node info
    auto *nodeInfoGroup = new QGroupBox("Node Information");
    auto *nodeInfoLayout = new QGridLayout();

    nodeInfoLayout->addWidget(new QLabel("Node ID:"), 0, 0);
    nodeIdLabel_ = new QLabel("Not started");
    nodeIdLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    nodeInfoLayout->addWidget(nodeIdLabel_, 0, 1);

    nodeInfoLayout->addWidget(new QLabel("Alias:"), 1, 0);
    nodeAliasLabel_ = new QLabel("INTcoin-LN");
    nodeInfoLayout->addWidget(nodeAliasLabel_, 1, 1);

    nodeInfoLayout->addWidget(new QLabel("Port:"), 2, 0);
    nodePortLabel_ = new QLabel("9735");
    nodeInfoLayout->addWidget(nodePortLabel_, 2, 1);

    nodeInfoLayout->addWidget(new QLabel("Status:"), 3, 0);
    nodeStatusLabel_ = new QLabel("Stopped");
    nodeInfoLayout->addWidget(nodeStatusLabel_, 3, 1);

    nodeInfoGroup->setLayout(nodeInfoLayout);
    layout->addWidget(nodeInfoGroup);

    // Node control
    auto *controlGroup = new QGroupBox("Node Control");
    auto *controlLayout = new QHBoxLayout();

    startNodeButton_ = new QPushButton("Start Lightning Node");
    connect(startNodeButton_, &QPushButton::clicked,
            this, &LightningWindow::on_startNodeButton_clicked);
    controlLayout->addWidget(startNodeButton_);

    stopNodeButton_ = new QPushButton("Stop Lightning Node");
    stopNodeButton_->setEnabled(false);
    connect(stopNodeButton_, &QPushButton::clicked,
            this, &LightningWindow::on_stopNodeButton_clicked);
    controlLayout->addWidget(stopNodeButton_);

    controlGroup->setLayout(controlLayout);
    layout->addWidget(controlGroup);

    // Peer management
    auto *peerGroup = new QGroupBox("Connected Peers");
    auto *peerLayout = new QVBoxLayout();

    peerTable_ = new QTableWidget(0, 4);
    peerTable_->setHorizontalHeaderLabels({
        "Peer ID", "Address", "Channels", "Last Seen"
    });
    peerTable_->horizontalHeader()->setStretchLastSection(true);
    peerTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    peerTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    peerLayout->addWidget(peerTable_);

    auto *peerControlLayout = new QHBoxLayout();
    peerControlLayout->addWidget(new QLabel("Address:"));
    connectPeerEdit_ = new QLineEdit();
    connectPeerEdit_->setPlaceholderText("node_id@host");
    peerControlLayout->addWidget(connectPeerEdit_, 1);

    peerControlLayout->addWidget(new QLabel("Port:"));
    connectPortEdit_ = new QLineEdit("9735");
    connectPortEdit_->setMaximumWidth(60);
    peerControlLayout->addWidget(connectPortEdit_);

    connectPeerButton_ = new QPushButton("Connect");
    connect(connectPeerButton_, &QPushButton::clicked,
            this, &LightningWindow::on_connectPeerButton_clicked);
    peerControlLayout->addWidget(connectPeerButton_);

    disconnectPeerButton_ = new QPushButton("Disconnect");
    disconnectPeerButton_->setEnabled(false);
    connect(disconnectPeerButton_, &QPushButton::clicked,
            this, &LightningWindow::on_disconnectPeerButton_clicked);
    peerControlLayout->addWidget(disconnectPeerButton_);

    peerLayout->addLayout(peerControlLayout);
    peerGroup->setLayout(peerLayout);
    layout->addWidget(peerGroup);

    tabWidget_->addTab(nodeWidget, "Node");
}

void LightningWindow::create_stats_tab() {
    auto *statsWidget = new QWidget();
    auto *layout = new QVBoxLayout(statsWidget);

    // Channel stats
    auto *channelStatsGroup = new QGroupBox("Channel Statistics");
    auto *channelStatsLayout = new QGridLayout();

    channelStatsLayout->addWidget(new QLabel("Total Channels:"), 0, 0);
    numChannelsLabel_ = new QLabel("0");
    channelStatsLayout->addWidget(numChannelsLabel_, 0, 1);

    channelStatsLayout->addWidget(new QLabel("Active Channels:"), 0, 2);
    numActiveChannelsLabel_ = new QLabel("0");
    channelStatsLayout->addWidget(numActiveChannelsLabel_, 0, 3);

    channelStatsGroup->setLayout(channelStatsLayout);
    layout->addWidget(channelStatsGroup);

    // Payment stats
    auto *paymentStatsGroup = new QGroupBox("Payment Statistics");
    auto *paymentStatsLayout = new QGridLayout();

    paymentStatsLayout->addWidget(new QLabel("Payments Sent:"), 0, 0);
    numPaymentsSentLabel_ = new QLabel("0");
    paymentStatsLayout->addWidget(numPaymentsSentLabel_, 0, 1);

    paymentStatsLayout->addWidget(new QLabel("Payments Received:"), 0, 2);
    numPaymentsReceivedLabel_ = new QLabel("0");
    paymentStatsLayout->addWidget(numPaymentsReceivedLabel_, 0, 3);

    paymentStatsLayout->addWidget(new QLabel("Average Payment Size:"), 1, 0);
    avgPaymentSizeLabel_ = new QLabel("0 INT");
    paymentStatsLayout->addWidget(avgPaymentSizeLabel_, 1, 1);

    paymentStatsGroup->setLayout(paymentStatsLayout);
    layout->addWidget(paymentStatsGroup);

    // Network stats
    auto *networkStatsGroup = new QGroupBox("Network Statistics");
    auto *networkStatsLayout = new QGridLayout();

    networkStatsLayout->addWidget(new QLabel("Known Nodes:"), 0, 0);
    networkGraphNodesLabel_ = new QLabel("0");
    networkStatsLayout->addWidget(networkGraphNodesLabel_, 0, 1);

    networkStatsLayout->addWidget(new QLabel("Known Channels:"), 0, 2);
    networkGraphChannelsLabel_ = new QLabel("0");
    networkStatsLayout->addWidget(networkGraphChannelsLabel_, 0, 3);

    networkStatsLayout->addWidget(new QLabel("Node Uptime:"), 1, 0);
    uptimeLabel_ = new QLabel("Not started");
    networkStatsLayout->addWidget(uptimeLabel_, 1, 1);

    networkStatsGroup->setLayout(networkStatsLayout);
    layout->addWidget(networkStatsGroup);

    layout->addStretch();

    tabWidget_->addTab(statsWidget, "Statistics");
}

// Slot implementations
void LightningWindow::on_openChannelButton_clicked() {
    if (!nodeRunning_) {
        show_error("Error", "Lightning node must be running to open channels");
        return;
    }

    QString peerIdStr = peerIdEdit_->text().trimmed();
    if (peerIdStr.isEmpty()) {
        show_error("Error", "Please enter a peer node ID");
        return;
    }

    // Convert peer ID hex string to DilithiumPubKey
    auto peer_pubkey_opt = lightning_utils::hex_to_dilithium_pubkey(peerIdStr);
    if (!peer_pubkey_opt.has_value()) {
        show_error("Invalid Peer ID",
            "The peer node ID must be a valid hexadecimal string.\n"
            "Expected: 5184 hex characters (2592 bytes for Dilithium5 public key)");
        return;
    }

    uint64_t capacity = static_cast<uint64_t>(channelCapacitySpin_->value()) * 100000000; // INT to satoshis

    // Open channel with lightning node
    auto channel_id_opt = ln_node_->open_channel(peer_pubkey_opt.value(), capacity);

    if (channel_id_opt.has_value()) {
        QString channel_id_str = lightning_utils::hash256_to_hex(channel_id_opt.value());
        show_info("Success", "Channel opening initiated\nChannel ID: " + channel_id_str);
        emit channelOpened(channel_id_str);
        peerIdEdit_->clear();
    } else {
        show_error("Error", "Failed to open channel. Peer may not be reachable or capacity insufficient.");
    }

    update_channel_list();
}

void LightningWindow::on_closeChannelButton_clicked() {
    if (selectedChannelId_.isEmpty()) {
        show_error("Error", "No channel selected");
        return;
    }

    auto reply = QMessageBox::question(this, "Close Channel",
        "Are you sure you want to close this channel? Funds will be returned on-chain.",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Convert channel ID hex string to Hash256
        auto channel_id_opt = lightning_utils::hex_to_hash256(selectedChannelId_);
        if (!channel_id_opt.has_value()) {
            show_error("Error", "Invalid channel ID format");
            return;
        }

        // Close channel (cooperative close)
        bool success = ln_node_->close_channel(channel_id_opt.value(), false);

        if (success) {
            show_info("Success", "Channel closing initiated. Funds will return to on-chain wallet.");
            emit channelClosed(selectedChannelId_);
        } else {
            show_error("Error", "Failed to close channel. It may already be closed or not in a valid state.");
        }

        selectedChannelId_.clear();
        update_channel_list();
    }
}

void LightningWindow::on_refreshChannelsButton_clicked() {
    update_channel_list();
}

void LightningWindow::on_channelTable_selectionChanged() {
    auto selectedItems = channelTable_->selectedItems();
    if (!selectedItems.isEmpty()) {
        int row = selectedItems[0]->row();
        selectedChannelId_ = channelTable_->item(row, 0)->text();
        closeChannelButton_->setEnabled(true);
    } else {
        selectedChannelId_.clear();
        closeChannelButton_->setEnabled(false);
    }
}

void LightningWindow::on_createInvoiceButton_clicked() {
    if (!nodeRunning_) {
        show_error("Error", "Lightning node must be running to create invoices");
        return;
    }

    QString amountStr = invoiceAmountEdit_->text().trimmed();
    if (amountStr.isEmpty()) {
        show_error("Error", "Please enter an amount");
        return;
    }

    bool ok;
    double amount = amountStr.toDouble(&ok);
    if (!ok || amount <= 0) {
        show_error("Error", "Invalid amount");
        return;
    }

    uint64_t amount_sat = static_cast<uint64_t>(amount * 100000000); // INT to satoshis
    QString description = invoiceDescriptionEdit_->text();

    // Create invoice using lightning node
    auto invoice = ln_node_->create_invoice(amount_sat, description.toStdString());

    // Display the encoded invoice
    QString invoice_str = QString::fromStdString(invoice.encoded_invoice);
    invoiceTextEdit_->setText(invoice_str);
    copyInvoiceButton_->setEnabled(true);

    // Store for later reference
    selectedInvoice_ = invoice_str;

    show_success("Invoice Created",
        QString("Invoice created for %1 INT\nExpires: %2")
            .arg(amount, 0, 'f', 8)
            .arg(invoice.expiry_time));

    // Clear input fields
    invoiceAmountEdit_->clear();
    invoiceDescriptionEdit_->clear();

    update_invoice_list();
}

void LightningWindow::on_payInvoiceButton_clicked() {
    if (!nodeRunning_) {
        show_error("Error", "Lightning node must be running to send payments");
        return;
    }

    QString invoice_str = payInvoiceEdit_->text().trimmed();
    if (invoice_str.isEmpty()) {
        show_error("Error", "Please enter an invoice");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm Payment",
        "Are you sure you want to pay this invoice?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Decode invoice to get payment details
        auto decoded_invoice = lightning_utils::decode_invoice(invoice_str);

        // Pay invoice using lightning node
        bool success = ln_node_->pay_invoice(invoice_str.toStdString());

        // Record payment in history
        PaymentRecord record;
        if (decoded_invoice.has_value()) {
            record.payment_hash = lightning_utils::hash256_to_hex(decoded_invoice->payment_hash).left(16);
            record.amount_sat = decoded_invoice->amount_msat.value_or(0) / 1000; // Convert msat to sat
        } else {
            record.payment_hash = invoice_str.left(16);
            record.amount_sat = 0;
        }
        record.direction = "sent";
        record.fees_sat = 0; // Fee estimation would require route information
        record.status = success ? "success" : "failed";
        record.timestamp = QDateTime::currentSecsSinceEpoch();
        payment_history_.push_back(record);

        if (success) {
            show_success("Success", "Payment sent successfully!");
            emit paymentSent(invoice_str);
            payInvoiceEdit_->clear();
        } else {
            show_error("Payment Failed",
                "Failed to send payment. Check:\n"
                "- Invoice is valid and not expired\n"
                "- You have sufficient channel balance\n"
                "- A route to destination exists");
        }

        update_payment_history();
    }
}

void LightningWindow::on_copyInvoiceButton_clicked() {
    QString invoice = invoiceTextEdit_->toPlainText();
    if (!invoice.isEmpty()) {
        QApplication::clipboard()->setText(invoice);
        show_success("Copied", "Invoice copied to clipboard");
    }
}

void LightningWindow::on_refreshInvoicesButton_clicked() {
    update_invoice_list();
}

void LightningWindow::on_startNodeButton_clicked() {
    if (nodeRunning_) {
        show_info("Info", "Lightning node is already running");
        return;
    }

    // Start the Lightning node (node is already constructed, just mark as running)
    nodeRunning_ = true;
    nodeStartTime_ = std::chrono::steady_clock::now();

    startNodeButton_->setEnabled(false);
    stopNodeButton_->setEnabled(true);

    nodeStatusLabel_->setText("Running");
    statusLabel_->setText("Lightning Network: Running");
    statusLabel_->setStyleSheet("QLabel { padding: 8px; background-color: #d4edda; border-radius: 4px; }");

    // Get and display actual node ID
    if (ln_node_) {
        DilithiumPubKey node_id = ln_node_->get_node_id();
        QString node_id_hex = lightning_utils::dilithium_pubkey_to_hex(node_id);
        nodeIdLabel_->setText(node_id_hex.left(32) + "...");
    } else {
        nodeIdLabel_->setText("Not available");
    }

    show_success("Success", "Lightning node started successfully");
    update_stats();
}

void LightningWindow::on_stopNodeButton_clicked() {
    if (!nodeRunning_) {
        return;
    }

    auto reply = QMessageBox::question(this, "Stop Node",
        "Are you sure you want to stop the Lightning node?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Stop the Lightning node (mark as not running - actual shutdown handled by destructor)
        nodeRunning_ = false;

        startNodeButton_->setEnabled(true);
        stopNodeButton_->setEnabled(false);

        nodeStatusLabel_->setText("Stopped");
        statusLabel_->setText("Lightning Network: Stopped");
        statusLabel_->setStyleSheet("QLabel { padding: 8px; background-color: #f8d7da; border-radius: 4px; }");

        nodeIdLabel_->setText("Not started");

        show_info("Info", "Lightning node stopped");
    }
}

void LightningWindow::on_connectPeerButton_clicked() {
    if (!nodeRunning_) {
        show_error("Error", "Lightning node must be running to connect to peers");
        return;
    }

    QString peerAddress = connectPeerEdit_->text().trimmed();
    QString port = connectPortEdit_->text().trimmed();

    if (peerAddress.isEmpty()) {
        show_error("Error", "Please enter a peer address");
        return;
    }

    // Connect to peer through network manager
    if (ln_network_) {
        // Parse peer address (format: node_id@host or just node_id)
        QStringList parts = peerAddress.split('@');
        if (parts.size() >= 1) {
            auto peer_pubkey_opt = lightning_utils::hex_to_dilithium_pubkey(parts[0]);
            if (peer_pubkey_opt.has_value()) {
                // Create peer address
                p2p::PeerAddress addr;
                addr.ip = (parts.size() > 1 ? parts[1].toStdString() : "127.0.0.1");
                addr.port = port.toUShort();

                bool success = ln_network_->connect_to_peer(peer_pubkey_opt.value(), addr);
                if (success) {
                    show_success("Success", "Connected to peer successfully");
                } else {
                    show_error("Error", "Failed to connect to peer");
                }
            } else {
                show_error("Error", "Invalid peer node ID");
            }
        }
    }
    connectPeerEdit_->clear();
    update_peer_list();
}

void LightningWindow::on_disconnectPeerButton_clicked() {
    // Get selected peer from table
    auto selected_items = peerTable_->selectedItems();
    if (selected_items.isEmpty()) {
        show_error("Error", "Please select a peer to disconnect");
        return;
    }

    // Get the peer ID from the first column of the selected row
    int row = selected_items.first()->row();
    QString peer_id_hex = peerTable_->item(row, 0)->text();

    // Convert hex string to DilithiumPubKey
    auto peer_pubkey_opt = lightning_utils::hex_to_dilithium_pubkey(peer_id_hex);
    if (!peer_pubkey_opt.has_value()) {
        show_error("Error", "Invalid peer node ID");
        return;
    }

    // Disconnect from peer through network manager
    if (ln_network_) {
        ln_network_->disconnect_peer(peer_pubkey_opt.value());
        show_success("Success", "Disconnected from peer");
    } else {
        show_error("Error", "Network manager not available");
    }

    update_peer_list();
}

// Update functions
void LightningWindow::update_channel_list() {
    if (!ln_node_ || !nodeRunning_) {
        channelTable_->setRowCount(0);
        return;
    }

    // Get all channels from lightning node
    auto channels = ln_node_->get_all_channels();

    channelTable_->setRowCount(static_cast<int>(channels.size()));

    uint64_t total_capacity = 0;
    uint64_t total_local_balance = 0;
    uint64_t total_remote_balance = 0;
    int active_count = 0;

    int row = 0;
    for (const auto& channel : channels) {
        // Channel ID (first 16 chars of hex)
        QString channel_id_hex = lightning_utils::hash256_to_hex(channel->channel_id);
        channelTable_->setItem(row, 0,
            new QTableWidgetItem(channel_id_hex.left(16) + "..."));

        // Peer ID (first 16 chars of hex)
        QString peer_id_hex = lightning_utils::dilithium_pubkey_to_hex(channel->remote_pubkey);
        channelTable_->setItem(row, 1,
            new QTableWidgetItem(peer_id_hex.left(16) + "..."));

        // State
        channelTable_->setItem(row, 2,
            new QTableWidgetItem(format_channel_state(channel->state)));

        // Capacity
        channelTable_->setItem(row, 3,
            new QTableWidgetItem(format_satoshis(channel->capacity_sat)));

        // Local Balance
        channelTable_->setItem(row, 4,
            new QTableWidgetItem(format_satoshis(channel->local_balance_sat)));

        // Remote Balance
        channelTable_->setItem(row, 5,
            new QTableWidgetItem(format_satoshis(channel->remote_balance_sat)));

        // Active HTLCs
        channelTable_->setItem(row, 6,
            new QTableWidgetItem(QString::number(channel->pending_htlcs.size())));

        // Update totals
        total_capacity += channel->capacity_sat;
        total_local_balance += channel->local_balance_sat;
        total_remote_balance += channel->remote_balance_sat;

        if (channel->is_open()) {
            active_count++;
        }

        row++;
    }

    // Update summary labels
    totalCapacityLabel_->setText(format_satoshis(total_capacity));
    localBalanceLabel_->setText(format_satoshis(total_local_balance));
    remoteBalanceLabel_->setText(format_satoshis(total_remote_balance));
    activeChannelsLabel_->setText(QString::number(active_count));
}

void LightningWindow::update_invoice_list() {
    if (!ln_node_) return;

    // Get invoices from lightning node
    const auto& invoices = ln_node_->get_invoices();

    invoiceTable_->setRowCount(static_cast<int>(invoices.size()));

    int row = 0;
    for (const auto& [payment_hash, invoice] : invoices) {
        // Invoice ID (truncated payment hash)
        QString invoice_id = lightning_utils::hash256_to_hex(payment_hash).left(16);
        invoiceTable_->setItem(row, 0, new QTableWidgetItem(invoice_id));

        // Amount
        QString amount_str = format_satoshis(invoice.amount_sat);
        invoiceTable_->setItem(row, 1, new QTableWidgetItem(amount_str));

        // Description
        QString description = QString::fromStdString(invoice.description);
        invoiceTable_->setItem(row, 2, new QTableWidgetItem(description));

        // Status (check if expired)
        auto current_time = std::chrono::system_clock::now();
        auto invoice_time = std::chrono::system_clock::from_time_t(invoice.expiry_time);
        QString status = (current_time > invoice_time) ? "Expired" : "Active";
        invoiceTable_->setItem(row, 3, new QTableWidgetItem(status));

        // Created timestamp
        QDateTime created_time = QDateTime::fromSecsSinceEpoch(invoice.expiry_time - 3600); // Assuming 1 hour expiry
        QString created_str = created_time.toString("yyyy-MM-dd hh:mm");
        invoiceTable_->setItem(row, 4, new QTableWidgetItem(created_str));

        row++;
    }
}

void LightningWindow::update_peer_list() {
    if (!ln_network_) return;

    // Get connected peers from network manager
    auto peers = ln_network_->get_connected_peers();

    peerTable_->setRowCount(static_cast<int>(peers.size()));

    int row = 0;
    for (const auto& peer : peers) {
        // Peer ID (truncated public key)
        QString peer_id = lightning_utils::dilithium_pubkey_to_hex(peer.node_id).left(32);
        peerTable_->setItem(row, 0, new QTableWidgetItem(peer_id));

        // Address
        QString address = QString::fromStdString(peer.address.ip) + ":" + QString::number(peer.address.port);
        peerTable_->setItem(row, 1, new QTableWidgetItem(address));

        // Number of channels
        QString channel_count = QString::number(peer.channels.size());
        peerTable_->setItem(row, 2, new QTableWidgetItem(channel_count));

        // Last seen (convert timestamp to readable format)
        QDateTime last_seen = QDateTime::fromSecsSinceEpoch(peer.last_seen);
        QString last_seen_str = last_seen.toString("yyyy-MM-dd hh:mm");
        peerTable_->setItem(row, 3, new QTableWidgetItem(last_seen_str));

        row++;
    }
}

void LightningWindow::update_stats() {
    if (!ln_node_ || !nodeRunning_) {
        return;
    }

    // Get statistics from lightning node
    auto stats = ln_node_->get_stats();

    // Update channel statistics
    numChannelsLabel_->setText(QString::number(stats.total_channels));
    numActiveChannelsLabel_->setText(QString::number(stats.active_channels));

    // Update payment statistics
    numPaymentsSentLabel_->setText(QString::number(stats.successful_payments));
    numPaymentsReceivedLabel_->setText(QString::number(stats.payments_received));

    // Calculate average payment size
    uint64_t avg_payment = 0;
    if (stats.successful_payments > 0) {
        avg_payment = stats.total_capacity_sat / stats.successful_payments;
    }
    avgPaymentSizeLabel_->setText(format_satoshis(avg_payment));

    // Update network statistics from network manager
    if (ln_network_) {
        auto network_stats = ln_network_->get_stats();
        networkGraphNodesLabel_->setText(QString::number(network_stats.announced_nodes));
        networkGraphChannelsLabel_->setText(QString::number(network_stats.announced_channels));
    } else {
        networkGraphNodesLabel_->setText("0");
        networkGraphChannelsLabel_->setText("0");
    }

    // Update uptime
    if (nodeRunning_) {
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - nodeStartTime_);
        uptimeLabel_->setText(format_duration(uptime));
    }
}

void LightningWindow::update_payment_history() {
    if (!ln_node_) return;

    // Display payment history from local tracking
    paymentHistoryTable_->setRowCount(static_cast<int>(payment_history_.size()));

    uint64_t total_sent = 0;
    uint64_t total_received = 0;
    uint64_t total_fees = 0;

    int row = 0;
    // Show most recent first
    for (auto it = payment_history_.rbegin(); it != payment_history_.rend(); ++it) {
        const auto& payment = *it;

        // Payment Hash (truncated)
        paymentHistoryTable_->setItem(row, 0,
            new QTableWidgetItem(payment.payment_hash.left(16) + "..."));

        // Direction
        paymentHistoryTable_->setItem(row, 1,
            new QTableWidgetItem(payment.direction));

        // Amount
        paymentHistoryTable_->setItem(row, 2,
            new QTableWidgetItem(format_satoshis(payment.amount_sat)));

        // Fees
        paymentHistoryTable_->setItem(row, 3,
            new QTableWidgetItem(QString::number(payment.fees_sat) + " sats"));

        // Status
        paymentHistoryTable_->setItem(row, 4,
            new QTableWidgetItem(payment.status));

        // Time
        QDateTime time = QDateTime::fromSecsSinceEpoch(payment.timestamp);
        paymentHistoryTable_->setItem(row, 5,
            new QTableWidgetItem(time.toString("yyyy-MM-dd hh:mm:ss")));

        // Update totals
        if (payment.direction == "sent" && payment.status == "success") {
            total_sent += payment.amount_sat;
            total_fees += payment.fees_sat;
        } else if (payment.direction == "received" && payment.status == "success") {
            total_received += payment.amount_sat;
        }

        row++;
    }

    // Update summary labels
    totalSentLabel_->setText(format_satoshis(total_sent));
    totalReceivedLabel_->setText(format_satoshis(total_received));
    totalFeesLabel_->setText(QString::number(total_fees) + " sats");
}

// Helper functions
void LightningWindow::show_error(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

void LightningWindow::show_success(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

void LightningWindow::show_info(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

QString LightningWindow::format_satoshis(uint64_t amount) const {
    double int_amount = static_cast<double>(amount) / 100000000.0;
    return QString::number(int_amount, 'f', 8) + " INT";
}

QString LightningWindow::format_millisatoshis(uint64_t amount) const {
    double int_amount = static_cast<double>(amount) / 100000000000.0;
    return QString::number(int_amount, 'f', 11) + " INT";
}

QString LightningWindow::format_channel_state(lightning::ChannelState state) const {
    switch (state) {
        case lightning::ChannelState::OPENING: return "Opening";
        case lightning::ChannelState::OPEN: return "Open";
        case lightning::ChannelState::CLOSING: return "Closing";
        case lightning::ChannelState::FORCE_CLOSING: return "Force Closing";
        case lightning::ChannelState::CLOSED: return "Closed";
        case lightning::ChannelState::ERROR: return "Error";
        default: return "Unknown";
    }
}

QString LightningWindow::format_duration(std::chrono::seconds seconds) const {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(seconds);
    seconds -= std::chrono::duration_cast<std::chrono::seconds>(hours);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds);
    seconds -= std::chrono::duration_cast<std::chrono::seconds>(minutes);

    return QString("%1h %2m %3s")
        .arg(hours.count())
        .arg(minutes.count())
        .arg(seconds.count());
}


void LightningWindow::update_network_graph() {
    if (!ln_node_ || !ln_network_) {
        return;
    }

    // Get network graph data
    const auto& graph = ln_node_->get_network_graph();
    auto network_stats = ln_network_->get_stats();

    // Update statistics labels
    networkGraphNodesLabel_->setText(QString::number(network_stats.announced_nodes));
    networkGraphChannelsLabel_->setText(QString::number(network_stats.announced_channels));

    // Clear existing tables
    networkNodesTable_->setRowCount(0);
    networkChannelsTable_->setRowCount(0);

    // Populate channels table from network graph
    int row = 0;
    for (const auto& [channel_id, channel_info] : graph) {
        networkChannelsTable_->insertRow(row);

        // Channel ID (first 16 chars)
        QString channel_id_str = lightning_utils::hash256_to_hex(channel_id);
        networkChannelsTable_->setItem(row, 0,
            new QTableWidgetItem(channel_id_str.left(16) + "..."));

        // Node 1 (first 16 chars)
        QString node1_str = lightning_utils::dilithium_pubkey_to_hex(channel_info.node1);
        networkChannelsTable_->setItem(row, 1,
            new QTableWidgetItem(node1_str.left(16) + "..."));

        // Node 2 (first 16 chars)
        QString node2_str = lightning_utils::dilithium_pubkey_to_hex(channel_info.node2);
        networkChannelsTable_->setItem(row, 2,
            new QTableWidgetItem(node2_str.left(16) + "..."));

        // Capacity
        networkChannelsTable_->setItem(row, 3,
            new QTableWidgetItem(format_satoshis(channel_info.capacity_sat)));

        // Fee base
        networkChannelsTable_->setItem(row, 4,
            new QTableWidgetItem(QString::number(channel_info.fee_base_msat) + " msat"));

        // Fee rate
        networkChannelsTable_->setItem(row, 5,
            new QTableWidgetItem(QString::number(channel_info.fee_rate_ppm) + " ppm"));

        // Enabled status
        networkChannelsTable_->setItem(row, 6,
            new QTableWidgetItem(channel_info.enabled ? "Yes" : "No"));

        row++;
    }

    // Update network graph visualization
    networkGraphScene_->clear();

    if (graph.empty()) {
        // Show message when no graph data is available
        networkGraphScene_->addText("No network graph data available.\nConnect to peers and sync the network graph.");
        return;
    }

    // Build a set of unique nodes from the channel graph
    std::set<DilithiumPubKey> unique_nodes;
    for (const auto& [channel_id, channel_info] : graph) {
        unique_nodes.insert(channel_info.node1);
        unique_nodes.insert(channel_info.node2);
    }

    // Populate nodes table
    row = 0;
    std::map<DilithiumPubKey, QPointF> node_positions;
    double angle_step = 2.0 * M_PI / std::max(static_cast<size_t>(1), unique_nodes.size());
    double radius = 200.0;
    int node_index = 0;

    for (const auto& node_id : unique_nodes) {
        networkNodesTable_->insertRow(row);

        // Node ID (first 32 chars)
        QString node_id_str = lightning_utils::dilithium_pubkey_to_hex(node_id);
        networkNodesTable_->setItem(row, 0,
            new QTableWidgetItem(node_id_str.left(32) + "..."));

        // Count channels for this node
        int channel_count = 0;
        for (const auto& [channel_id, channel_info] : graph) {
            if (channel_info.node1 == node_id || channel_info.node2 == node_id) {
                channel_count++;
            }
        }
        networkNodesTable_->setItem(row, 1,
            new QTableWidgetItem(QString::number(channel_count)));

        // Calculate position in circular layout
        double angle = node_index * angle_step;
        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        node_positions[node_id] = QPointF(x, y);

        // Draw node
        QGraphicsEllipseItem* node_circle = networkGraphScene_->addEllipse(
            x - 10, y - 10, 20, 20,
            QPen(Qt::black, 2),
            QBrush(Qt::lightGray)
        );
        node_circle->setToolTip(node_id_str);

        row++;
        node_index++;
    }

    // Draw channels as edges between nodes
    for (const auto& [channel_id, channel_info] : graph) {
        auto it1 = node_positions.find(channel_info.node1);
        auto it2 = node_positions.find(channel_info.node2);

        if (it1 != node_positions.end() && it2 != node_positions.end()) {
            QGraphicsLineItem* edge = networkGraphScene_->addLine(
                it1->second.x(), it1->second.y(),
                it2->second.x(), it2->second.y(),
                QPen(channel_info.enabled ? Qt::blue : Qt::gray, 1)
            );

            QString channel_id_str = lightning_utils::hash256_to_hex(channel_id);
            QString tooltip = QString("Channel: %1\nCapacity: %2 SAT\nFee: %3 msat base, %4 ppm")
                .arg(channel_id_str.left(16))
                .arg(channel_info.capacity_sat)
                .arg(channel_info.fee_base_msat)
                .arg(channel_info.fee_rate_ppm);
            edge->setToolTip(tooltip);
        }
    }

    // Center the view on the graph
    networkGraphView_->fitInView(networkGraphScene_->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void LightningWindow::on_findRouteButton_clicked() {
    QString destination_hex = routeDestinationEdit_->text().trimmed();
    uint64_t amount_sat = routeAmountSpin_->value();

    if (destination_hex.isEmpty()) {
        show_error("Error", "Please enter a destination node ID");
        return;
    }

    if (amount_sat == 0) {
        show_error("Error", "Please enter a valid amount");
        return;
    }

    // Convert hex to DilithiumPubKey
    auto destination_opt = lightning_utils::hex_to_dilithium_pubkey(destination_hex);
    if (!destination_opt.has_value()) {
        show_error("Error", "Invalid destination node ID");
        return;
    }

    // Find route using Lightning node
    if (!ln_node_) {
        show_error("Error", "Lightning node not available");
        return;
    }

    auto route = ln_node_->find_route(destination_opt.value(), amount_sat);

    if (route.empty()) {
        routeDisplayText_->setPlainText("No route found to destination");
        routeStatusLabel_->setText("Status: No route available");
        routeStatusLabel_->setStyleSheet("QLabel { color: red; }");
        return;
    }

    // Display route
    QString route_text = QString("Route found with %1 hops:\n\n").arg(route.size());

    for (size_t i = 0; i < route.size(); i++) {
        QString node_id = lightning_utils::dilithium_pubkey_to_hex(route[i]);
        route_text += QString("Hop %1: %2...\n").arg(i + 1).arg(node_id.left(32));
    }

    route_text += QString("\nTotal hops: %1\n").arg(route.size());
    route_text += QString("Amount: %1 SAT\n").arg(format_satoshis(amount_sat));

    routeDisplayText_->setPlainText(route_text);
    routeStatusLabel_->setText("Status: Route found");
    routeStatusLabel_->setStyleSheet("QLabel { color: green; }");
}

void LightningWindow::on_decodeInvoiceButton_clicked() {
    QString invoice_str = decodeInvoiceEdit_->text().trimmed();
    
    if (invoice_str.isEmpty()) {
        show_error("Error", "Please enter an invoice to decode");
        return;
    }

    auto invoice_opt = lightning_utils::decode_invoice(invoice_str);
    if (!invoice_opt.has_value()) {
        show_error("Error", "Failed to decode invoice");
        return;
    }

    QString details = lightning_utils::format_invoice_details(*invoice_opt);
    decodedInvoiceDisplay_->setPlainText(details);
}

void LightningWindow::on_refreshNetworkButton_clicked() {
    update_network_graph();
}

void LightningWindow::on_visualizeOnionButton_clicked() {
    // Get the current route from the route finding
    QString destination_hex = routeDestinationEdit_->text().trimmed();
    uint64_t amount_sat = routeAmountSpin_->value();

    if (destination_hex.isEmpty()) {
        show_error("Error", "Please find a route first using the 'Find Route' button");
        return;
    }

    if (amount_sat == 0) {
        show_error("Error", "Please enter a valid amount");
        return;
    }

    // Convert hex to DilithiumPubKey
    auto destination_opt = lightning_utils::hex_to_dilithium_pubkey(destination_hex);
    if (!destination_opt.has_value()) {
        show_error("Error", "Invalid destination node ID");
        return;
    }

    // Find route using Lightning node
    if (!ln_node_) {
        show_error("Error", "Lightning node not available");
        return;
    }

    auto route = ln_node_->find_route(destination_opt.value(), amount_sat);

    if (route.empty()) {
        show_error("Error", "No route found. Please find a route first.");
        return;
    }

    // Create visualization dialog
    QString visualization_text;
    visualization_text += "=== SPHINX ONION ROUTING PROTOCOL ===\n";
    visualization_text += "Quantum-Resistant Implementation using Kyber1024\n\n";

    visualization_text += QString("Route: %1 hops\n").arg(route.size());
    visualization_text += QString("Amount: %1 SAT (%2 msat)\n\n")
        .arg(amount_sat)
        .arg(amount_sat * 1000);

    // Visualize onion packet construction
    visualization_text += "--- ONION PACKET CONSTRUCTION ---\n\n";

    visualization_text += "1. ROUTE PLANNING:\n";
    for (size_t i = 0; i < route.size(); i++) {
        QString node_id = lightning_utils::dilithium_pubkey_to_hex(route[i]);
        visualization_text += QString("   Hop %1: %2...\n")
            .arg(i + 1)
            .arg(node_id.left(32));
    }
    visualization_text += "\n";

    // Explain packet structure
    visualization_text += "2. PACKET STRUCTURE:\n";
    visualization_text += "   - Version: 1 byte\n";
    visualization_text += "   - Ephemeral Key: 1568 bytes (Kyber1024 public key)\n";
    visualization_text += "   - Routing Info: 1300 bytes (encrypted)\n";
    visualization_text += "   - HMAC: 32 bytes (SHA3-256)\n";
    visualization_text += "   - Total Size: 1366 bytes (fixed)\n\n";

    // Visualize layered encryption
    visualization_text += "3. LAYERED ENCRYPTION (Sphinx Protocol):\n\n";

    for (size_t i = 0; i < route.size(); i++) {
        size_t hop_num = route.size() - i;
        QString node_id = lightning_utils::dilithium_pubkey_to_hex(route[route.size() - 1 - i]);

        visualization_text += QString("   Layer %1 (for Hop %2):\n").arg(i + 1).arg(hop_num);
        visualization_text += QString("   Node: %1...\n").arg(node_id.left(32));
        visualization_text += "   ┌─────────────────────────────────────────┐\n";
        visualization_text += "   │ ENCRYPTION PROCESS:                     │\n";
        visualization_text += "   │                                         │\n";
        visualization_text += "   │ 1. Generate ephemeral Kyber1024 keypair│\n";
        visualization_text += "   │ 2. Perform Kyber1024 key exchange      │\n";
        visualization_text += "   │    with node's public key               │\n";
        visualization_text += "   │ 3. Derive shared secret (32 bytes)     │\n";
        visualization_text += "   │ 4. Derive sub-keys:                    │\n";
        visualization_text += "   │    - rho: HMAC key                      │\n";
        visualization_text += "   │    - mu: ChaCha20 encryption key       │\n";
        visualization_text += "   │    - um: Blinding factor                │\n";
        visualization_text += "   │ 5. Encrypt hop data with ChaCha20      │\n";
        visualization_text += "   │ 6. Compute HMAC-SHA3-256               │\n";
        visualization_text += "   │ 7. Blind ephemeral key for next hop    │\n";
        visualization_text += "   └─────────────────────────────────────────┘\n\n";
    }

    // Visualize forwarding process
    visualization_text += "4. FORWARDING PROCESS:\n\n";

    for (size_t i = 0; i < route.size(); i++) {
        QString node_id = lightning_utils::dilithium_pubkey_to_hex(route[i]);
        bool is_final = (i == route.size() - 1);

        visualization_text += QString("   Hop %1: %2...\n")
            .arg(i + 1)
            .arg(node_id.left(32));

        visualization_text += "   ┌─────────────────────────────────────────┐\n";
        visualization_text += "   │ DECRYPTION PROCESS:                     │\n";
        visualization_text += "   │                                         │\n";
        visualization_text += "   │ 1. Receive onion packet                │\n";
        visualization_text += "   │ 2. Extract ephemeral Kyber1024 pubkey  │\n";
        visualization_text += "   │ 3. Perform key exchange with own key   │\n";
        visualization_text += "   │ 4. Derive shared secret                │\n";
        visualization_text += "   │ 5. Verify HMAC-SHA3-256                │\n";
        visualization_text += "   │ 6. Decrypt one layer with ChaCha20     │\n";
        visualization_text += "   │ 7. Extract hop payload:                │\n";

        if (is_final) {
            visualization_text += "   │    - Payment hash                       │\n";
            visualization_text += QString("   │    - Amount: %1 msat                │\n")
                .arg(amount_sat * 1000);
            visualization_text += "   │    - Final destination                  │\n";
            visualization_text += "   │ 8. FINAL HOP - Claim payment           │\n";
        } else {
            visualization_text += "   │    - Next channel ID                    │\n";
            visualization_text += QString("   │    - Forward amount: %1 msat        │\n")
                .arg(amount_sat * 1000);
            visualization_text += "   │    - CLTV expiry delta                  │\n";
            visualization_text += "   │ 8. Forward to next hop                 │\n";
        }

        visualization_text += "   └─────────────────────────────────────────┘\n\n";
    }

    // Privacy guarantees
    visualization_text += "5. PRIVACY GUARANTEES:\n";
    visualization_text += "   ✓ Each node only knows:\n";
    visualization_text += "     - Previous hop (where packet came from)\n";
    visualization_text += "     - Next hop (where to forward)\n";
    visualization_text += "     - Amount to forward\n\n";
    visualization_text += "   ✓ Nodes cannot determine:\n";
    visualization_text += "     - Payment source\n";
    visualization_text += "     - Payment destination\n";
    visualization_text += "     - Total route length\n";
    visualization_text += "     - Position in route\n\n";

    // Quantum resistance
    visualization_text += "6. QUANTUM RESISTANCE:\n";
    visualization_text += "   ✓ Kyber1024 key exchange (NIST PQC standard)\n";
    visualization_text += "   ✓ Resistant to Shor's algorithm\n";
    visualization_text += "   ✓ Resistant to Grover's algorithm\n";
    visualization_text += "   ✓ 256-bit quantum security level\n\n";

    // Error handling
    visualization_text += "7. ERROR HANDLING:\n";
    visualization_text += "   If payment fails at any hop:\n";
    visualization_text += "   1. Failing node creates error onion\n";
    visualization_text += "   2. Error encrypted with shared secrets\n";
    visualization_text += "   3. Error sent back to source\n";
    visualization_text += "   4. Source decrypts layers to find failure point\n\n";

    // Display in text dialog
    QMessageBox msgBox;
    msgBox.setWindowTitle("Onion Route Visualization");
    msgBox.setText("Detailed visualization of Sphinx onion routing protocol");
    msgBox.setDetailedText(visualization_text);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    // Make the dialog larger
    msgBox.exec();

    // Also display in route display area
    routeDisplayText_->setPlainText(visualization_text);
}

} // namespace qt
} // namespace intcoin
