/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Send Coins Page
 */

#ifndef INTCOIN_QT_SENDCOINSPAGE_H
#define INTCOIN_QT_SENDCOINSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

namespace intcoin {
    namespace wallet {
        class Wallet;
    }
}

namespace intcoin {
namespace qt {

class SendCoinsPage : public QWidget {
    Q_OBJECT

public:
    explicit SendCoinsPage(wallet::Wallet* wallet, QWidget *parent = nullptr);
    ~SendCoinsPage();

public slots:
    void sendCoins();
    void clearFields();
    void pasteAddress();
    void scanQRCode();
    void calculateFee();

private:
    void setupUi();
    bool validateAddress(const QString& address);
    bool validateAmount(double amount);

    wallet::Wallet* wallet_;

    // Input fields
    QLineEdit* addressEdit_;
    QLineEdit* labelEdit_;
    QDoubleSpinBox* amountSpinBox_;
    QDoubleSpinBox* feeSpinBox_;
    QLabel* totalLabel_;

    // Buttons
    QPushButton* sendButton_;
    QPushButton* clearButton_;
    QPushButton* pasteButton_;
    QPushButton* qrButton_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_SENDCOINSPAGE_H
