// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network GUI Window Implementation

#include "lightningwindow.h"

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
#include <iomanip>
#include <sstream>

namespace intcoin {
namespace qt {

LightningWindow::LightningWindow(
    std::shared_ptr<lightning::LightningNode> ln_node,
    std::shared_ptr<HDWallet> wallet,
    std::shared_ptr<Blockchain> blockchain,
    QWidget *parent
) : QWidget(parent),
    ln_node_(ln_node),
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
    if (nodeRunning_ && ln_node_) {
        ln_node_->stop();
    }
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

    uint64_t capacity = static_cast<uint64_t>(channelCapacitySpin_->value()) * 100000000; // INT to satoshis

    // TODO: Implement actual channel opening using ln_node_->open_channel()
    show_info("Opening Channel", "Channel opening initiated. This may take several minutes...");

    emit channelOpened(peerIdStr);
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
        // TODO: Implement actual channel closing using ln_node_->close_channel()
        show_info("Closing Channel", "Channel closing initiated...");

        emit channelClosed(selectedChannelId_);
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

    uint64_t amount_msat = static_cast<uint64_t>(amount * 100000000000); // INT to millisatoshis
    QString description = invoiceDescriptionEdit_->text();
    uint32_t expiry_seconds = static_cast<uint32_t>(invoiceExpirySpinBox_->value() * 60);

    // TODO: Implement actual invoice creation using ln_node_->create_invoice()
    QString invoice = QString("lnint1") + QString::number(amount_msat) + "...";

    invoiceTextEdit_->setText(invoice);
    copyInvoiceButton_->setEnabled(true);

    show_success("Invoice Created", "Invoice has been generated successfully");
    update_invoice_list();
}

void LightningWindow::on_payInvoiceButton_clicked() {
    if (!nodeRunning_) {
        show_error("Error", "Lightning node must be running to send payments");
        return;
    }

    QString invoice = payInvoiceEdit_->text().trimmed();
    if (invoice.isEmpty()) {
        show_error("Error", "Please enter an invoice");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm Payment",
        "Are you sure you want to pay this invoice?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // TODO: Implement actual payment using ln_node_->send_payment()
        show_info("Sending Payment", "Payment is being processed...");

        payInvoiceEdit_->clear();
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

    // TODO: Implement actual node start using ln_node_->start()
    nodeRunning_ = true;
    nodeStartTime_ = std::chrono::steady_clock::now();

    startNodeButton_->setEnabled(false);
    stopNodeButton_->setEnabled(true);

    nodeStatusLabel_->setText("Running");
    statusLabel_->setText("Lightning Network: Running");
    statusLabel_->setStyleSheet("QLabel { padding: 8px; background-color: #d4edda; border-radius: 4px; }");

    // TODO: Get actual node ID
    nodeIdLabel_->setText("03abcd1234...");

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
        // TODO: Implement actual node stop using ln_node_->stop()
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

    // TODO: Implement actual peer connection using ln_node_->connect_peer()
    show_info("Connecting", "Connecting to peer...");
    connectPeerEdit_->clear();
    update_peer_list();
}

void LightningWindow::on_disconnectPeerButton_clicked() {
    // TODO: Implement peer disconnection
    update_peer_list();
}

// Update functions
void LightningWindow::update_channel_list() {
    if (!ln_node_) return;

    // TODO: Implement actual channel list retrieval
    // For now, this is a placeholder
    channelTable_->setRowCount(0);

    // Example: Add sample data when node is running
    if (nodeRunning_) {
        // This would be replaced with actual data from ln_node_->list_channels()
    }
}

void LightningWindow::update_invoice_list() {
    if (!ln_node_) return;

    // TODO: Implement actual invoice list retrieval
    invoiceTable_->setRowCount(0);
}

void LightningWindow::update_peer_list() {
    if (!ln_node_) return;

    // TODO: Implement actual peer list retrieval
    peerTable_->setRowCount(0);
}

void LightningWindow::update_stats() {
    if (!ln_node_ || !nodeRunning_) {
        return;
    }

    // TODO: Implement actual stats retrieval from ln_node_->get_stats()

    // Update uptime
    if (nodeRunning_) {
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - nodeStartTime_);
        uptimeLabel_->setText(format_duration(uptime));
    }
}

void LightningWindow::update_payment_history() {
    if (!ln_node_) return;

    // TODO: Implement actual payment history retrieval
    paymentHistoryTable_->setRowCount(0);
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

} // namespace qt
} // namespace intcoin
