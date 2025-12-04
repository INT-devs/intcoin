/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Address Book Page
 */

#ifndef INTCOIN_QT_ADDRESSBOOKPAGE_H
#define INTCOIN_QT_ADDRESSBOOKPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>

namespace intcoin {
    class Wallet;
}

namespace intcoin {
namespace qt {

class AddressBookPage : public QWidget {
    Q_OBJECT

public:
    explicit AddressBookPage(Wallet* wallet, QWidget *parent = nullptr);
    ~AddressBookPage();

public slots:
    void addAddress();
    void editAddress();
    void deleteAddress();
    void copyAddress();
    void updateAddressList();

private:
    void setupUi();

    Wallet* wallet_;

    // Search
    QLineEdit* searchEdit_;

    // Buttons
    QPushButton* newButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;
    QPushButton* copyButton_;

    // Address table
    QTableWidget* addressTable_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_ADDRESSBOOKPAGE_H
