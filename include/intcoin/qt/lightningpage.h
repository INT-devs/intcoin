/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Lightning Network Page
 */

#ifndef INTCOIN_QT_LIGHTNINGPAGE_H
#define INTCOIN_QT_LIGHTNINGPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

namespace intcoin {
    class Blockchain;
    class P2PNode;
    class LightningNetwork;
    namespace wallet {
        class Wallet;
    }
}

namespace intcoin {
namespace qt {

class LightningPage : public QWidget {
    Q_OBJECT

public:
    explicit LightningPage(wallet::Wallet* wallet,
                          Blockchain* blockchain,
                          P2PNode* p2p,
                          LightningNetwork* lightning,
                          QWidget *parent = nullptr);
    ~LightningPage();

public slots:
    void updateChannelList();
    void updateBalance();
    void updateNodeInfo();
    void refreshData();

private slots:
    void openChannel();
    void closeChannel();
    void sendPayment();
    void createInvoice();
    void payInvoice();
    void onChannelSelected();

private:
    void setupUI();
    void createChannelManagementSection();
    void createPaymentSection();
    void createInvoiceSection();
    void createNodeInfoSection();
    void connectSignals();

    // Core components
    wallet::Wallet* wallet_;
    Blockchain* blockchain_;
    P2PNode* p2p_;
    LightningNetwork* lightning_;

    // UI Components
    QTabWidget* tabWidget_;

    // Channels Tab
    QWidget* channelsTab_;
    QTableWidget* channelTable_;
    QPushButton* openChannelBtn_;
    QPushButton* closeChannelBtn_;
    QPushButton* refreshChannelsBtn_;
    QLabel* totalCapacityLabel_;
    QLabel* localBalanceLabel_;
    QLabel* remoteBalanceLabel_;

    // Send Payment Tab
    QWidget* sendTab_;
    QLineEdit* paymentRequestEdit_;
    QLineEdit* amountEdit_;
    QTextEdit* paymentMemoEdit_;
    QPushButton* sendPaymentBtn_;
    QPushButton* decodeInvoiceBtn_;
    QLabel* decodedInfoLabel_;
    QTableWidget* paymentHistoryTable_;

    // Receive Payment Tab
    QWidget* receiveTab_;
    QSpinBox* invoiceAmountSpin_;
    QLineEdit* invoiceMemoEdit_;
    QSpinBox* invoiceExpirySpinBox_;
    QPushButton* createInvoiceBtn_;
    QTextEdit* invoiceDisplay_;
    QPushButton* copyInvoiceBtn_;
    QLabel* qrCodeLabel_;
    QTableWidget* invoiceHistoryTable_;

    // Node Info Tab
    QWidget* nodeInfoTab_;
    QLabel* nodeIdLabel_;
    QLabel* nodePubKeyLabel_;
    QLabel* nodeAddressLabel_;
    QLabel* nodePortLabel_;
    QLabel* totalChannelsLabel_;
    QLabel* activeChannelsLabel_;
    QLabel* pendingChannelsLabel_;
    QLabel* networkHashRateLabel_;
    QPushButton* copyNodeInfoBtn_;

    // Update timer
    QTimer* updateTimer_;

    // State
    QString selectedChannelId_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_LIGHTNINGPAGE_H
