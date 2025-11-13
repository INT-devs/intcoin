// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Lightning Network GUI Window

#ifndef INTCOIN_QT_LIGHTNINGWINDOW_H
#define INTCOIN_QT_LIGHTNINGWINDOW_H

#include "intcoin/lightning.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QTabWidget>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Lightning Network management window
 */
class LightningWindow : public QWidget {
    Q_OBJECT

public:
    explicit LightningWindow(
        std::shared_ptr<lightning::LightningNode> ln_node,
        std::shared_ptr<HDWallet> wallet,
        std::shared_ptr<Blockchain> blockchain,
        QWidget *parent = nullptr
    );
    ~LightningWindow();

signals:
    void channelOpened(const QString& channel_id);
    void channelClosed(const QString& channel_id);
    void paymentSent(const QString& payment_hash);
    void paymentReceived(const QString& payment_hash, uint64_t amount);

private slots:
    // Channel management slots
    void on_openChannelButton_clicked();
    void on_closeChannelButton_clicked();
    void on_refreshChannelsButton_clicked();
    void on_channelTable_selectionChanged();

    // Invoice management slots
    void on_createInvoiceButton_clicked();
    void on_payInvoiceButton_clicked();
    void on_copyInvoiceButton_clicked();
    void on_refreshInvoicesButton_clicked();

    // Node management slots
    void on_startNodeButton_clicked();
    void on_stopNodeButton_clicked();
    void on_connectPeerButton_clicked();
    void on_disconnectPeerButton_clicked();

    // Update functions
    void update_channel_list();
    void update_invoice_list();
    void update_peer_list();
    void update_stats();
    void update_payment_history();

private:
    // UI setup
    void setup_ui();
    void create_channel_tab();
    void create_invoice_tab();
    void create_payment_tab();
    void create_node_tab();
    void create_stats_tab();

    // Core components
    std::shared_ptr<lightning::LightningNode> ln_node_;
    std::shared_ptr<HDWallet> wallet_;
    std::shared_ptr<Blockchain> blockchain_;

    // UI components - Main
    QTabWidget* tabWidget_;
    QTimer* updateTimer_;
    QLabel* statusLabel_;

    // UI components - Channel tab
    QTableWidget* channelTable_;
    QLineEdit* peerIdEdit_;
    QSpinBox* channelCapacitySpin_;
    QPushButton* openChannelButton_;
    QPushButton* closeChannelButton_;
    QPushButton* refreshChannelsButton_;
    QLabel* totalCapacityLabel_;
    QLabel* activeChannelsLabel_;
    QLabel* localBalanceLabel_;
    QLabel* remoteBalanceLabel_;

    // UI components - Invoice tab
    QTableWidget* invoiceTable_;
    QLineEdit* invoiceAmountEdit_;
    QLineEdit* invoiceDescriptionEdit_;
    QSpinBox* invoiceExpirySpinBox_;
    QPushButton* createInvoiceButton_;
    QPushButton* copyInvoiceButton_;
    QPushButton* refreshInvoicesButton_;
    QTextEdit* invoiceTextEdit_;
    QLabel* generatedInvoiceLabel_;

    // UI components - Payment tab
    QLineEdit* payInvoiceEdit_;
    QPushButton* payInvoiceButton_;
    QTableWidget* paymentHistoryTable_;
    QLabel* totalSentLabel_;
    QLabel* totalReceivedLabel_;
    QLabel* totalFeesLabel_;

    // UI components - Node tab
    QLabel* nodeIdLabel_;
    QLabel* nodeAliasLabel_;
    QLabel* nodePortLabel_;
    QLabel* nodeStatusLabel_;
    QPushButton* startNodeButton_;
    QPushButton* stopNodeButton_;
    QTableWidget* peerTable_;
    QLineEdit* connectPeerEdit_;
    QLineEdit* connectPortEdit_;
    QPushButton* connectPeerButton_;
    QPushButton* disconnectPeerButton_;

    // UI components - Stats tab
    QLabel* numChannelsLabel_;
    QLabel* numActiveChannelsLabel_;
    QLabel* numPaymentsSentLabel_;
    QLabel* numPaymentsReceivedLabel_;
    QLabel* avgPaymentSizeLabel_;
    QLabel* networkGraphNodesLabel_;
    QLabel* networkGraphChannelsLabel_;
    QLabel* uptimeLabel_;

    // Helper functions
    void show_error(const QString& title, const QString& message);
    void show_success(const QString& title, const QString& message);
    void show_info(const QString& title, const QString& message);
    QString format_satoshis(uint64_t amount) const;
    QString format_millisatoshis(uint64_t amount) const;
    QString format_channel_state(lightning::ChannelState state) const;
    QString format_duration(std::chrono::seconds seconds) const;

    // Selected items
    QString selectedChannelId_;
    QString selectedInvoice_;

    // Node state
    bool nodeRunning_;
    std::chrono::steady_clock::time_point nodeStartTime_;

    // Payment history tracking
    struct PaymentRecord {
        QString payment_hash;
        QString direction; // "sent" or "received"
        uint64_t amount_sat;
        uint64_t fees_sat;
        QString status; // "success", "pending", "failed"
        qint64 timestamp;
    };
    std::vector<PaymentRecord> payment_history_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_LIGHTNINGWINDOW_H
