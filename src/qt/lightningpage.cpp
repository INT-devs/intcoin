// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/lightningpage.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/network.h"
#include "intcoin/lightning.h"
#include "intcoin/util.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <QApplication>
#include <QPixmap>
#include <QPainter>

namespace intcoin {
namespace qt {

LightningPage::LightningPage(wallet::Wallet* wallet,
                             Blockchain* blockchain,
                             P2PNode* p2p,
                             LightningNetwork* lightning,
                             QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
    , p2p_(p2p)
    , lightning_(lightning)
{
    setupUI();
    connectSignals();

    // Start update timer
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &LightningPage::refreshData);
    updateTimer_->start(5000); // Update every 5 seconds

    // Initial data load
    refreshData();
}

LightningPage::~LightningPage() {
    if (updateTimer_) {
        updateTimer_->stop();
    }
}

void LightningPage::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget for different Lightning features
    tabWidget_ = new QTabWidget(this);

    // Create tabs
    createChannelManagementSection();
    createPaymentSection();
    createInvoiceSection();
    createNodeInfoSection();

    mainLayout->addWidget(tabWidget_);
    setLayout(mainLayout);
}

void LightningPage::createChannelManagementSection() {
    channelsTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(channelsTab_);

    // Balance summary
    QGroupBox* balanceGroup = new QGroupBox(tr("Channel Balances"));
    QGridLayout* balanceLayout = new QGridLayout();

    totalCapacityLabel_ = new QLabel(tr("Total Capacity: 0 INT"));
    localBalanceLabel_ = new QLabel(tr("Local Balance: 0 INT"));
    remoteBalanceLabel_ = new QLabel(tr("Remote Balance: 0 INT"));

    QFont boldFont;
    boldFont.setBold(true);
    totalCapacityLabel_->setFont(boldFont);
    localBalanceLabel_->setFont(boldFont);
    remoteBalanceLabel_->setFont(boldFont);

    balanceLayout->addWidget(totalCapacityLabel_, 0, 0);
    balanceLayout->addWidget(localBalanceLabel_, 0, 1);
    balanceLayout->addWidget(remoteBalanceLabel_, 0, 2);
    balanceGroup->setLayout(balanceLayout);

    layout->addWidget(balanceGroup);

    // Channel list
    QGroupBox* channelGroup = new QGroupBox(tr("Active Channels"));
    QVBoxLayout* channelLayout = new QVBoxLayout();

    channelTable_ = new QTableWidget();
    channelTable_->setColumnCount(7);
    channelTable_->setHorizontalHeaderLabels({
        tr("Channel ID"),
        tr("Peer"),
        tr("Status"),
        tr("Capacity"),
        tr("Local Balance"),
        tr("Remote Balance"),
        tr("State")
    });
    channelTable_->horizontalHeader()->setStretchLastSection(true);
    channelTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    channelTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    channelTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    channelLayout->addWidget(channelTable_);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    openChannelBtn_ = new QPushButton(tr("Open Channel"));
    closeChannelBtn_ = new QPushButton(tr("Close Channel"));
    refreshChannelsBtn_ = new QPushButton(tr("Refresh"));

    closeChannelBtn_->setEnabled(false); // Enable only when channel selected

    buttonLayout->addWidget(openChannelBtn_);
    buttonLayout->addWidget(closeChannelBtn_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshChannelsBtn_);

    channelLayout->addLayout(buttonLayout);
    channelGroup->setLayout(channelLayout);

    layout->addWidget(channelGroup);
    tabWidget_->addTab(channelsTab_, tr("Channels"));
}

void LightningPage::createPaymentSection() {
    sendTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(sendTab_);

    // Send payment
    QGroupBox* sendGroup = new QGroupBox(tr("Send Lightning Payment"));
    QFormLayout* sendLayout = new QFormLayout();

    paymentRequestEdit_ = new QLineEdit();
    paymentRequestEdit_->setPlaceholderText(tr("lnbc1... (BOLT #11 invoice)"));

    amountEdit_ = new QLineEdit();
    amountEdit_->setPlaceholderText(tr("Amount in INT (optional if specified in invoice)"));

    paymentMemoEdit_ = new QTextEdit();
    paymentMemoEdit_->setMaximumHeight(60);
    paymentMemoEdit_->setPlaceholderText(tr("Optional memo"));

    decodedInfoLabel_ = new QLabel();
    decodedInfoLabel_->setWordWrap(true);
    decodedInfoLabel_->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");

    QHBoxLayout* sendButtonLayout = new QHBoxLayout();
    decodeInvoiceBtn_ = new QPushButton(tr("Decode Invoice"));
    sendPaymentBtn_ = new QPushButton(tr("Send Payment"));
    sendPaymentBtn_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");

    sendButtonLayout->addWidget(decodeInvoiceBtn_);
    sendButtonLayout->addWidget(sendPaymentBtn_);

    sendLayout->addRow(tr("Payment Request:"), paymentRequestEdit_);
    sendLayout->addRow(tr("Amount:"), amountEdit_);
    sendLayout->addRow(tr("Memo:"), paymentMemoEdit_);
    sendLayout->addRow(tr("Invoice Info:"), decodedInfoLabel_);
    sendLayout->addRow(sendButtonLayout);

    sendGroup->setLayout(sendLayout);
    layout->addWidget(sendGroup);

    // Payment history
    QGroupBox* historyGroup = new QGroupBox(tr("Payment History"));
    QVBoxLayout* historyLayout = new QVBoxLayout();

    paymentHistoryTable_ = new QTableWidget();
    paymentHistoryTable_->setColumnCount(6);
    paymentHistoryTable_->setHorizontalHeaderLabels({
        tr("Date"),
        tr("Direction"),
        tr("Amount"),
        tr("Fee"),
        tr("Status"),
        tr("Payment Hash")
    });
    paymentHistoryTable_->horizontalHeader()->setStretchLastSection(true);
    paymentHistoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    historyLayout->addWidget(paymentHistoryTable_);
    historyGroup->setLayout(historyLayout);

    layout->addWidget(historyGroup);
    tabWidget_->addTab(sendTab_, tr("Send Payment"));
}

void LightningPage::createInvoiceSection() {
    receiveTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(receiveTab_);

    // Create invoice
    QGroupBox* createGroup = new QGroupBox(tr("Create Payment Invoice"));
    QFormLayout* createLayout = new QFormLayout();

    invoiceAmountSpin_ = new QSpinBox();
    invoiceAmountSpin_->setMinimum(1);
    invoiceAmountSpin_->setMaximum(1000000000); // 10 INT max
    invoiceAmountSpin_->setSuffix(tr(" INTS"));
    invoiceAmountSpin_->setValue(100000); // 0.001 INT default

    invoiceMemoEdit_ = new QLineEdit();
    invoiceMemoEdit_->setPlaceholderText(tr("Optional description"));

    invoiceExpirySpinBox_ = new QSpinBox();
    invoiceExpirySpinBox_->setMinimum(60);
    invoiceExpirySpinBox_->setMaximum(86400);
    invoiceExpirySpinBox_->setSuffix(tr(" seconds"));
    invoiceExpirySpinBox_->setValue(3600); // 1 hour default

    createInvoiceBtn_ = new QPushButton(tr("Generate Invoice"));
    createInvoiceBtn_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; }");

    createLayout->addRow(tr("Amount:"), invoiceAmountSpin_);
    createLayout->addRow(tr("Description:"), invoiceMemoEdit_);
    createLayout->addRow(tr("Expiry:"), invoiceExpirySpinBox_);
    createLayout->addRow(createInvoiceBtn_);

    createGroup->setLayout(createLayout);
    layout->addWidget(createGroup);

    // Invoice display
    QGroupBox* displayGroup = new QGroupBox(tr("Generated Invoice"));
    QVBoxLayout* displayLayout = new QVBoxLayout();

    invoiceDisplay_ = new QTextEdit();
    invoiceDisplay_->setReadOnly(true);
    invoiceDisplay_->setMaximumHeight(100);
    invoiceDisplay_->setPlaceholderText(tr("Invoice will appear here..."));
    invoiceDisplay_->setStyleSheet("QTextEdit { font-family: 'Courier New'; font-size: 10pt; }");

    QHBoxLayout* copyLayout = new QHBoxLayout();
    copyInvoiceBtn_ = new QPushButton(tr("Copy to Clipboard"));
    copyInvoiceBtn_->setEnabled(false);

    copyLayout->addStretch();
    copyLayout->addWidget(copyInvoiceBtn_);

    displayLayout->addWidget(invoiceDisplay_);
    displayLayout->addLayout(copyLayout);

    // QR Code placeholder
    qrCodeLabel_ = new QLabel();
    qrCodeLabel_->setAlignment(Qt::AlignCenter);
    qrCodeLabel_->setMinimumSize(200, 200);
    qrCodeLabel_->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: white; }");
    qrCodeLabel_->setText(tr("QR Code"));

    displayLayout->addWidget(qrCodeLabel_);
    displayGroup->setLayout(displayLayout);

    layout->addWidget(displayGroup);

    // Invoice history
    QGroupBox* invoiceHistoryGroup = new QGroupBox(tr("Invoice History"));
    QVBoxLayout* invoiceHistoryLayout = new QVBoxLayout();

    invoiceHistoryTable_ = new QTableWidget();
    invoiceHistoryTable_->setColumnCount(5);
    invoiceHistoryTable_->setHorizontalHeaderLabels({
        tr("Created"),
        tr("Amount"),
        tr("Description"),
        tr("Status"),
        tr("Payment Hash")
    });
    invoiceHistoryTable_->horizontalHeader()->setStretchLastSection(true);
    invoiceHistoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    invoiceHistoryLayout->addWidget(invoiceHistoryTable_);
    invoiceHistoryGroup->setLayout(invoiceHistoryLayout);

    layout->addWidget(invoiceHistoryGroup);
    tabWidget_->addTab(receiveTab_, tr("Receive Payment"));
}

void LightningPage::createNodeInfoSection() {
    nodeInfoTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(nodeInfoTab_);

    QGroupBox* infoGroup = new QGroupBox(tr("Lightning Node Information"));
    QFormLayout* infoLayout = new QFormLayout();

    nodeIdLabel_ = new QLabel(tr("Not connected"));
    nodePubKeyLabel_ = new QLabel(tr("N/A"));
    nodePubKeyLabel_->setWordWrap(true);
    nodePubKeyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    nodeAddressLabel_ = new QLabel(tr("N/A"));
    nodePortLabel_ = new QLabel(tr("2213"));

    totalChannelsLabel_ = new QLabel(tr("0"));
    activeChannelsLabel_ = new QLabel(tr("0"));
    pendingChannelsLabel_ = new QLabel(tr("0"));

    networkHashRateLabel_ = new QLabel(tr("N/A"));

    infoLayout->addRow(tr("Node ID:"), nodeIdLabel_);
    infoLayout->addRow(tr("Public Key:"), nodePubKeyLabel_);
    infoLayout->addRow(tr("Address:"), nodeAddressLabel_);
    infoLayout->addRow(tr("Port:"), nodePortLabel_);
    infoLayout->addRow(tr(""), new QLabel()); // Spacer
    infoLayout->addRow(tr("Total Channels:"), totalChannelsLabel_);
    infoLayout->addRow(tr("Active Channels:"), activeChannelsLabel_);
    infoLayout->addRow(tr("Pending Channels:"), pendingChannelsLabel_);
    infoLayout->addRow(tr(""), new QLabel()); // Spacer
    infoLayout->addRow(tr("Network Hash Rate:"), networkHashRateLabel_);

    copyNodeInfoBtn_ = new QPushButton(tr("Copy Node Info"));

    infoLayout->addRow(copyNodeInfoBtn_);

    infoGroup->setLayout(infoLayout);
    layout->addWidget(infoGroup);

    layout->addStretch();
    tabWidget_->addTab(nodeInfoTab_, tr("Node Info"));
}

void LightningPage::connectSignals() {
    // Channel management
    connect(openChannelBtn_, &QPushButton::clicked, this, &LightningPage::openChannel);
    connect(closeChannelBtn_, &QPushButton::clicked, this, &LightningPage::closeChannel);
    connect(refreshChannelsBtn_, &QPushButton::clicked, this, &LightningPage::updateChannelList);
    connect(channelTable_, &QTableWidget::itemSelectionChanged, this, &LightningPage::onChannelSelected);

    // Payments
    connect(sendPaymentBtn_, &QPushButton::clicked, this, &LightningPage::sendPayment);
    connect(decodeInvoiceBtn_, &QPushButton::clicked, this, &LightningPage::payInvoice);

    // Invoices
    connect(createInvoiceBtn_, &QPushButton::clicked, this, &LightningPage::createInvoice);
    connect(copyInvoiceBtn_, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(invoiceDisplay_->toPlainText());
        QMessageBox::information(this, tr("Copied"), tr("Invoice copied to clipboard!"));
    });

    // Node info
    connect(copyNodeInfoBtn_, &QPushButton::clicked, [this]() {
        QString info = QString("Node ID: %1\nPublic Key: %2\nAddress: %3:%4")
            .arg(nodeIdLabel_->text())
            .arg(nodePubKeyLabel_->text())
            .arg(nodeAddressLabel_->text())
            .arg(nodePortLabel_->text());
        QApplication::clipboard()->setText(info);
        QMessageBox::information(this, tr("Copied"), tr("Node info copied to clipboard!"));
    });
}

void LightningPage::openChannel() {
    if (!lightning_ || !lightning_->IsRunning()) {
        QMessageBox::warning(this, tr("Lightning Not Running"),
            tr("Lightning Network is not running. Please start it first."));
        return;
    }

    bool ok;
    QString peerAddress = QInputDialog::getText(this,
        tr("Open Channel"),
        tr("Enter peer node public key (hex):"),
        QLineEdit::Normal,
        "",
        &ok);

    if (!ok || peerAddress.isEmpty()) {
        return;
    }

    // Parse public key
    auto peerKeyBytes = HexToBytes(peerAddress.toStdString());
    if (peerKeyBytes.IsError() || peerKeyBytes.value->size() != 1952) {
        QMessageBox::warning(this, tr("Invalid Public Key"),
            tr("Invalid public key format. Expected 1952 bytes (Dilithium3)."));
        return;
    }

    PublicKey peerKey;
    std::copy(peerKeyBytes.value->begin(), peerKeyBytes.value->end(), peerKey.begin());

    int64_t capacity = QInputDialog::getInt(this,
        tr("Open Channel"),
        tr("Channel capacity (in INTS):"),
        100000000, // 1 INT default
        100000,    // 0.001 INT minimum
        1000000000, // 10 INT maximum
        1000000,   // Step
        &ok);

    if (!ok) {
        return;
    }

    // Call Lightning Network API to open channel
    auto result = lightning_->OpenChannel(peerKey, capacity);
    if (result.IsError()) {
        QMessageBox::critical(this, tr("Channel Opening Failed"),
            tr("Failed to open channel: %1").arg(QString::fromStdString(result.error)));
        return;
    }

    QMessageBox::information(this, tr("Channel Opening"),
        tr("Opening channel with capacity %1 INTS...\n"
           "Channel ID: %2\n"
           "This may take a few minutes to confirm on-chain.")
        .arg(capacity)
        .arg(QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            result.value->begin(), result.value->end())))));

    updateChannelList();
}

void LightningPage::closeChannel() {
    if (selectedChannelId_.isEmpty()) {
        QMessageBox::warning(this, tr("No Channel Selected"),
            tr("Please select a channel to close."));
        return;
    }

    if (!lightning_ || !lightning_->IsRunning()) {
        QMessageBox::warning(this, tr("Lightning Not Running"),
            tr("Lightning Network is not running."));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Close Channel"),
        tr("Are you sure you want to close this channel?\nThis will require on-chain confirmation."),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Parse channel ID
        auto channelIdBytes = HexToBytes(selectedChannelId_.toStdString());
        if (channelIdBytes.IsError() || channelIdBytes.value->size() != 32) {
            QMessageBox::critical(this, tr("Error"),
                tr("Invalid channel ID format."));
            return;
        }

        uint256 channelId;
        std::copy(channelIdBytes.value->begin(), channelIdBytes.value->end(), channelId.begin());

        // Call Lightning Network API to close channel
        auto result = lightning_->CloseChannel(channelId, false);
        if (result.IsError()) {
            QMessageBox::critical(this, tr("Channel Closure Failed"),
                tr("Failed to close channel: %1").arg(QString::fromStdString(result.error)));
            return;
        }

        QMessageBox::information(this, tr("Closing Channel"),
            tr("Channel closure initiated. This may take several blocks to confirm."));

        updateChannelList();
    }
}

void LightningPage::sendPayment() {
    QString paymentRequest = paymentRequestEdit_->text().trimmed();

    if (paymentRequest.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"),
            tr("Please enter a payment request."));
        return;
    }

    if (!lightning_ || !lightning_->IsRunning()) {
        QMessageBox::warning(this, tr("Lightning Not Running"),
            tr("Lightning Network is not running."));
        return;
    }

    // Call Lightning Network API to send payment
    auto result = lightning_->SendPayment(paymentRequest.toStdString());
    if (result.IsError()) {
        QMessageBox::critical(this, tr("Payment Failed"),
            tr("Failed to send payment: %1").arg(QString::fromStdString(result.error)));
        return;
    }

    QMessageBox::information(this, tr("Payment Sent"),
        tr("Lightning payment sent successfully!\nPayment Hash: %1")
        .arg(QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            result.value->begin(), result.value->end())))));

    paymentRequestEdit_->clear();
    amountEdit_->clear();
    paymentMemoEdit_->clear();
}

void LightningPage::createInvoice() {
    uint64_t amount = invoiceAmountSpin_->value();
    QString memo = invoiceMemoEdit_->text();
    [[maybe_unused]] uint32_t expiry = invoiceExpirySpinBox_->value();

    if (!lightning_ || !lightning_->IsRunning()) {
        QMessageBox::warning(this, tr("Lightning Not Running"),
            tr("Lightning Network is not running."));
        return;
    }

    // Call Lightning Network API to create invoice
    auto result = lightning_->CreateInvoice(amount, memo.toStdString());
    if (result.IsError()) {
        QMessageBox::critical(this, tr("Invoice Creation Failed"),
            tr("Failed to create invoice: %1").arg(QString::fromStdString(result.error)));
        return;
    }

    QString invoice = QString::fromStdString(result.value->Encode());
    invoiceDisplay_->setText(invoice);
    copyInvoiceBtn_->setEnabled(true);

    // TODO: Generate QR code (requires QR code library)
    QPixmap qrCode(200, 200);
    qrCode.fill(Qt::white);
    QPainter painter(&qrCode);
    painter.setPen(Qt::black);
    painter.drawText(qrCode.rect(), Qt::AlignCenter, "QR\nCode\n(TODO)");
    qrCodeLabel_->setPixmap(qrCode);

    QMessageBox::information(this, tr("Invoice Created"),
        tr("Lightning invoice generated successfully!\nAmount: %1 INTS")
        .arg(amount));
}

void LightningPage::payInvoice() {
    QString paymentRequest = paymentRequestEdit_->text().trimmed();

    if (paymentRequest.isEmpty()) {
        decodedInfoLabel_->setText(tr("Please enter a payment request to decode."));
        return;
    }

    // Decode invoice using BOLT #11 decoder
    auto result = Invoice::Decode(paymentRequest.toStdString());
    if (result.IsError()) {
        decodedInfoLabel_->setText(tr("<b>Error:</b> %1")
            .arg(QString::fromStdString(result.error)));
        return;
    }

    auto& invoice = *result.value;
    QString decodedInfo = tr(
        "<b>Invoice Details:</b><br>"
        "Amount: %1 INTS<br>"
        "Description: %2<br>"
        "Expiry: %3 seconds<br>"
        "Payee: %4<br>"
        "Payment Hash: %5"
    )
        .arg(invoice.amount)
        .arg(QString::fromStdString(invoice.description))
        .arg(invoice.expiry)
        .arg(QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            invoice.payee.begin(), invoice.payee.begin() + 16))) + "...")
        .arg(QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            invoice.payment_hash.begin(), invoice.payment_hash.begin() + 16))) + "...");

    decodedInfoLabel_->setText(decodedInfo);
}

void LightningPage::onChannelSelected() {
    QList<QTableWidgetItem*> selected = channelTable_->selectedItems();
    if (!selected.isEmpty()) {
        selectedChannelId_ = channelTable_->item(selected[0]->row(), 0)->text();
        closeChannelBtn_->setEnabled(true);
    } else {
        selectedChannelId_.clear();
        closeChannelBtn_->setEnabled(false);
    }
}

void LightningPage::updateChannelList() {
    channelTable_->setRowCount(0);

    if (!lightning_ || !lightning_->IsRunning()) {
        return;
    }

    // Get channels from Lightning Network API
    auto channels = lightning_->ListChannels();

    for (const auto& channel : channels) {
        int row = channelTable_->rowCount();
        channelTable_->insertRow(row);

        // Channel ID
        QString channelId = QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            channel.channel_id.begin(), channel.channel_id.begin() + 8))) + "...";
        channelTable_->setItem(row, 0, new QTableWidgetItem(channelId));

        // Peer
        QString peer = QString::fromStdString(BytesToHex(std::vector<uint8_t>(
            channel.remote_node_id.begin(), channel.remote_node_id.begin() + 8))) + "...";
        channelTable_->setItem(row, 1, new QTableWidgetItem(peer));

        // Status
        QString status = channel.state == ChannelState::OPEN ? "Active" :
                        channel.state == ChannelState::OPENING ? "Opening" :
                        channel.state == ChannelState::CLOSING_MUTUAL ? "Closing" :
                        channel.state == ChannelState::CLOSED ? "Closed" : "Unknown";
        channelTable_->setItem(row, 2, new QTableWidgetItem(status));

        // Capacity
        channelTable_->setItem(row, 3, new QTableWidgetItem(
            QString::number(channel.capacity / 1000000000.0, 'f', 8)));

        // Local Balance
        channelTable_->setItem(row, 4, new QTableWidgetItem(
            QString::number(channel.local_balance / 1000000000.0, 'f', 8)));

        // Remote Balance
        channelTable_->setItem(row, 5, new QTableWidgetItem(
            QString::number(channel.remote_balance / 1000000000.0, 'f', 8)));

        // State
        QString stateStr = status;  // Use same as status for now
        channelTable_->setItem(row, 6, new QTableWidgetItem(stateStr));
    }
}

void LightningPage::updateBalance() {
    if (!lightning_ || !lightning_->IsRunning()) {
        totalCapacityLabel_->setText(tr("Total Capacity: 0 INT"));
        localBalanceLabel_->setText(tr("Local Balance: 0 INT"));
        remoteBalanceLabel_->setText(tr("Remote Balance: 0 INT"));
        return;
    }

    // Get balance from Lightning Network API
    auto stats = lightning_->GetStats();

    totalCapacityLabel_->setText(tr("Total Capacity: %1 INT")
        .arg(stats.total_capacity / 1000000000.0, 0, 'f', 8));
    localBalanceLabel_->setText(tr("Local Balance: %1 INT")
        .arg(stats.local_balance / 1000000000.0, 0, 'f', 8));
    remoteBalanceLabel_->setText(tr("Remote Balance: %1 INT")
        .arg(stats.remote_balance / 1000000000.0, 0, 'f', 8));
}

void LightningPage::updateNodeInfo() {
    if (!lightning_ || !lightning_->IsRunning()) {
        nodeIdLabel_->setText(tr("Not running"));
        nodePubKeyLabel_->setText(tr("N/A"));
        nodeAddressLabel_->setText(tr("N/A"));
        totalChannelsLabel_->setText(tr("0"));
        activeChannelsLabel_->setText(tr("0"));
        pendingChannelsLabel_->setText(tr("0"));
        return;
    }

    // Get node info from Lightning Network API
    auto nodeId = lightning_->GetNodeId();
    auto stats = lightning_->GetStats();

    nodeIdLabel_->setText(tr("Running"));
    nodePubKeyLabel_->setText(QString::fromStdString(BytesToHex(std::vector<uint8_t>(
        nodeId.begin(), nodeId.end()))));
    nodeAddressLabel_->setText(tr("127.0.0.1")); // TODO: Get actual address
    nodePortLabel_->setText(tr("2213"));

    totalChannelsLabel_->setText(QString::number(stats.num_channels));
    activeChannelsLabel_->setText(QString::number(stats.num_active_channels));
    pendingChannelsLabel_->setText(QString::number(
        stats.num_channels - stats.num_active_channels));
}

void LightningPage::refreshData() {
    updateChannelList();
    updateBalance();
    updateNodeInfo();
}

} // namespace qt
} // namespace intcoin
