// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/settingspage.h"
#include "intcoin/wallet.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>

namespace intcoin {
namespace qt {

SettingsPage::SettingsPage(Wallet* wallet, QWidget *parent)
    : QWidget(parent)
    , wallet_(wallet)
{
    setupUi();
    loadSettings();
}

SettingsPage::~SettingsPage() {}

void SettingsPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    tabWidget_ = new QTabWidget(this);

    // Main settings tab
    QWidget* mainTab = new QWidget();
    QFormLayout* mainLayout_ = new QFormLayout(mainTab);

    startOnBootCheckBox_ = new QCheckBox(tr("Start on system boot"), this);
    mainLayout_->addRow(startOnBootCheckBox_);

    minimizeToTrayCheckBox_ = new QCheckBox(tr("Minimize to system tray"), this);
    mainLayout_->addRow(minimizeToTrayCheckBox_);

    dataDirectoryEdit_ = new QLineEdit(this);
    dataDirectoryEdit_->setReadOnly(true);
    QPushButton* browseButton = new QPushButton(tr("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &SettingsPage::browseDataDirectory);
    QHBoxLayout* dirLayout = new QHBoxLayout();
    dirLayout->addWidget(dataDirectoryEdit_);
    dirLayout->addWidget(browseButton);
    mainLayout_->addRow(tr("Data directory:"), dirLayout);

    tabWidget_->addTab(mainTab, tr("Main"));

    // Network settings tab
    QWidget* networkTab = new QWidget();
    QFormLayout* networkLayout = new QFormLayout(networkTab);

    enableUpnpCheckBox_ = new QCheckBox(tr("Map port using UPnP"), this);
    networkLayout->addRow(enableUpnpCheckBox_);

    allowIncomingCheckBox_ = new QCheckBox(tr("Allow incoming connections"), this);
    networkLayout->addRow(allowIncomingCheckBox_);

    maxConnectionsSpinBox_ = new QSpinBox(this);
    maxConnectionsSpinBox_->setMinimum(1);
    maxConnectionsSpinBox_->setMaximum(125);
    maxConnectionsSpinBox_->setValue(8);
    networkLayout->addRow(tr("Maximum connections:"), maxConnectionsSpinBox_);

    proxyEdit_ = new QLineEdit(this);
    proxyEdit_->setPlaceholderText(tr("127.0.0.1:9050"));
    networkLayout->addRow(tr("SOCKS5 proxy:"), proxyEdit_);

    tabWidget_->addTab(networkTab, tr("Network"));

    // Display settings tab
    QWidget* displayTab = new QWidget();
    QFormLayout* displayLayout = new QFormLayout(displayTab);

    languageComboBox_ = new QComboBox(this);
    languageComboBox_->addItem(tr("English"));
    displayLayout->addRow(tr("Language:"), languageComboBox_);

    unitComboBox_ = new QComboBox(this);
    unitComboBox_->addItem(tr("INT"));
    unitComboBox_->addItem(tr("INTS"));
    displayLayout->addRow(tr("Unit:"), unitComboBox_);

    digitsAfterDecimalSpinBox_ = new QSpinBox(this);
    digitsAfterDecimalSpinBox_->setMinimum(2);
    digitsAfterDecimalSpinBox_->setMaximum(8);
    digitsAfterDecimalSpinBox_->setValue(6);
    displayLayout->addRow(tr("Decimal digits:"), digitsAfterDecimalSpinBox_);

    tabWidget_->addTab(displayTab, tr("Display"));

    mainLayout->addWidget(tabWidget_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    defaultsButton_ = new QPushButton(tr("&Restore Defaults"), this);
    saveButton_ = new QPushButton(tr("&Save"), this);
    cancelButton_ = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(defaultsButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(saveButton_);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(saveButton_, &QPushButton::clicked, this, &SettingsPage::saveSettings);
    connect(defaultsButton_, &QPushButton::clicked, this, &SettingsPage::restoreDefaults);

    setLayout(mainLayout);
}

void SettingsPage::loadSettings() {
    // TODO: Load settings from configuration file
    dataDirectoryEdit_->setText(QDir::homePath() + "/.intcoin");
}

void SettingsPage::saveSettings() {
    // TODO: Save settings to configuration file
    QMessageBox::information(this, tr("Settings Saved"),
        tr("Settings saved successfully."));
}

void SettingsPage::restoreDefaults() {
    startOnBootCheckBox_->setChecked(false);
    minimizeToTrayCheckBox_->setChecked(false);
    enableUpnpCheckBox_->setChecked(true);
    allowIncomingCheckBox_->setChecked(true);
    maxConnectionsSpinBox_->setValue(8);
    proxyEdit_->clear();
    languageComboBox_->setCurrentIndex(0);
    unitComboBox_->setCurrentIndex(0);
    digitsAfterDecimalSpinBox_->setValue(6);
}

void SettingsPage::browseDataDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Data Directory"), dataDirectoryEdit_->text());

    if (!dir.isEmpty()) {
        dataDirectoryEdit_->setText(dir);
    }
}

} // namespace qt
} // namespace intcoin
