/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * INTcoin Qt Wallet - Settings Page
 */

#ifndef INTCOIN_QT_SETTINGSPAGE_H
#define INTCOIN_QT_SETTINGSPAGE_H

#include <QWidget>
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

namespace intcoin {
    namespace wallet {
        class Wallet;
    }
}

namespace intcoin {
namespace qt {

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(wallet::Wallet* wallet, QWidget *parent = nullptr);
    ~SettingsPage();

public slots:
    void saveSettings();
    void restoreDefaults();
    void browseDataDirectory();

private:
    void setupUi();
    void loadSettings();

    wallet::Wallet* wallet_;

    QTabWidget* tabWidget_;

    // Main settings
    QCheckBox* startOnBootCheckBox_;
    QCheckBox* minimizeToTrayCheckBox_;
    QLineEdit* dataDirectoryEdit_;

    // Network settings
    QCheckBox* enableUpnpCheckBox_;
    QCheckBox* allowIncomingCheckBox_;
    QSpinBox* maxConnectionsSpinBox_;
    QLineEdit* proxyEdit_;

    // Display settings
    QComboBox* languageComboBox_;
    QComboBox* unitComboBox_;
    QSpinBox* digitsAfterDecimalSpinBox_;

    // Buttons
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
    QPushButton* defaultsButton_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_SETTINGSPAGE_H
