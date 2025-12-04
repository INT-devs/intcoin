/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Receive Coins Page
 */

#ifndef INTCOIN_QT_RECEIVECOINSPAGE_H
#define INTCOIN_QT_RECEIVECOINSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

namespace intcoin {
    class Wallet;
}

namespace intcoin {
namespace qt {

class ReceiveCoinsPage : public QWidget {
    Q_OBJECT

public:
    explicit ReceiveCoinsPage(Wallet* wallet, QWidget *parent = nullptr);
    ~ReceiveCoinsPage();

public slots:
    void generateNewAddress();
    void copyAddress();
    void showQRCode();
    void updateAddressList();

private:
    void setupUi();

    Wallet* wallet_;

    // Current receive address
    QLineEdit* currentAddressEdit_;
    QLineEdit* labelEdit_;
    QDoubleSpinBox* amountSpinBox_;
    QLabel* qrCodeLabel_;

    // Buttons
    QPushButton* newAddressButton_;
    QPushButton* copyButton_;
    QPushButton* qrButton_;

    // Address list
    QTableWidget* addressTable_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_RECEIVECOINSPAGE_H
