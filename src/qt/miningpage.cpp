// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qt/miningpage.h>
#include <intcoin/wallet.h>
#include <intcoin/blockchain.h>
#include <intcoin/mining.h>
#include <intcoin/util.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <chrono>

namespace intcoin {
namespace qt {

MiningPage::MiningPage(wallet::Wallet* wallet, Blockchain* blockchain, QWidget* parent)
    : QWidget(parent)
    , wallet_(wallet)
    , blockchain_(blockchain)
{
    setupUi();
    connectSignals();
    loadSettings();

    // Update timer (every 2 seconds)
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MiningPage::updateStats);
    updateTimer_->start(2000);
}

MiningPage::~MiningPage() {
    if (is_mining_) {
        stopMining();
    }
    saveSettings();
}

void MiningPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Status group
    statusGroup_ = new QGroupBox(tr("Mining Status"), this);
    QGridLayout* statusLayout = new QGridLayout(statusGroup_);

    miningStatusLabel_ = new QLabel(tr("<b>Stopped</b>"), this);
    hashrateLabel_ = new QLabel(tr("0 H/s"), this);
    blocksFoundLabel_ = new QLabel(tr("0"), this);
    uptimeLabel_ = new QLabel(tr("0:00:00"), this);
    totalHashesLabel_ = new QLabel(tr("0"), this);

    statusLayout->addWidget(new QLabel(tr("Status:")), 0, 0);
    statusLayout->addWidget(miningStatusLabel_, 0, 1);
    statusLayout->addWidget(new QLabel(tr("Hashrate:")), 1, 0);
    statusLayout->addWidget(hashrateLabel_, 1, 1);
    statusLayout->addWidget(new QLabel(tr("Blocks Found:")), 2, 0);
    statusLayout->addWidget(blocksFoundLabel_, 2, 1);
    statusLayout->addWidget(new QLabel(tr("Uptime:")), 3, 0);
    statusLayout->addWidget(uptimeLabel_, 3, 1);
    statusLayout->addWidget(new QLabel(tr("Total Hashes:")), 4, 0);
    statusLayout->addWidget(totalHashesLabel_, 4, 1);

    mainLayout->addWidget(statusGroup_);

    // Configuration group
    configGroup_ = new QGroupBox(tr("Mining Configuration"), this);
    QFormLayout* configLayout = new QFormLayout(configGroup_);

    threadCountSpinBox_ = new QSpinBox(this);
    threadCountSpinBox_->setMinimum(1);
    threadCountSpinBox_->setMaximum(std::thread::hardware_concurrency());
    threadCountSpinBox_->setValue(std::thread::hardware_concurrency());
    threadCountSpinBox_->setSuffix(tr(" threads"));
    configLayout->addRow(tr("Thread Count:"), threadCountSpinBox_);

    QHBoxLayout* addressLayout = new QHBoxLayout();
    miningAddressEdit_ = new QLineEdit(this);
    miningAddressEdit_->setPlaceholderText(tr("Enter mining address or generate new"));
    generateAddressButton_ = new QPushButton(tr("Generate"), this);
    addressLayout->addWidget(miningAddressEdit_);
    addressLayout->addWidget(generateAddressButton_);
    configLayout->addRow(tr("Mining Address:"), addressLayout);

    poolMiningCheckbox_ = new QCheckBox(tr("Enable Pool Mining"), this);
    configLayout->addRow("", poolMiningCheckbox_);

    mainLayout->addWidget(configGroup_);

    // Pool configuration group
    poolGroup_ = new QGroupBox(tr("Pool Configuration"), this);
    poolGroup_->setEnabled(false);
    QFormLayout* poolLayout = new QFormLayout(poolGroup_);

    poolHostEdit_ = new QLineEdit(this);
    poolHostEdit_->setPlaceholderText(tr("pool.intcoin.org"));
    poolLayout->addRow(tr("Pool Host:"), poolHostEdit_);

    poolPortSpinBox_ = new QSpinBox(this);
    poolPortSpinBox_->setMinimum(1);
    poolPortSpinBox_->setMaximum(65535);
    poolPortSpinBox_->setValue(3333);
    poolLayout->addRow(tr("Pool Port:"), poolPortSpinBox_);

    poolUsernameEdit_ = new QLineEdit(this);
    poolUsernameEdit_->setPlaceholderText(tr("Your INTcoin address or username"));
    poolLayout->addRow(tr("Username:"), poolUsernameEdit_);

    poolPasswordEdit_ = new QLineEdit(this);
    poolPasswordEdit_->setEchoMode(QLineEdit::Password);
    poolPasswordEdit_->setPlaceholderText(tr("x or worker password"));
    poolLayout->addRow(tr("Password:"), poolPasswordEdit_);

    mainLayout->addWidget(poolGroup_);

    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton(tr("Start Mining"), this);
    stopButton_ = new QPushButton(tr("Stop Mining"), this);
    stopButton_->setEnabled(false);

    startButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
    stopButton_->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 10px; }");

    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    mainLayout->addLayout(buttonLayout);

    // Log
    QGroupBox* logGroup = new QGroupBox(tr("Mining Log"), this);
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);

    logTextEdit_ = new QTextEdit(this);
    logTextEdit_->setReadOnly(true);
    logTextEdit_->setMaximumHeight(150);
    logLayout->addWidget(logTextEdit_);

    mainLayout->addWidget(logGroup);

    setLayout(mainLayout);
}

void MiningPage::connectSignals() {
    connect(startButton_, &QPushButton::clicked, this, &MiningPage::startMining);
    connect(stopButton_, &QPushButton::clicked, this, &MiningPage::stopMining);
    connect(poolMiningCheckbox_, &QCheckBox::toggled, this, &MiningPage::togglePoolMining);
    connect(generateAddressButton_, &QPushButton::clicked, [this]() {
        if (!wallet_) return;

        auto result = wallet_->GetNewAddress("Mining");
        if (result.IsOk()) {
            miningAddressEdit_->setText(QString::fromStdString(result.GetValue()));
            logTextEdit_->append(tr("[%1] Generated new mining address")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        } else {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to generate address: %1")
                .arg(QString::fromStdString(result.error)));
        }
    });

    connect(threadCountSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MiningPage::updateConfig);
}

void MiningPage::startMining() {
    // Validate configuration
    QString address = miningAddressEdit_->text().trimmed();
    if (address.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Address"),
            tr("Please enter a mining address or generate a new one."));
        return;
    }

    // Create mining configuration
    mining::MiningConfig config;
    config.thread_count = threadCountSpinBox_->value();
    config.mining_address = address.toStdString();
    config.pool_mining = poolMiningCheckbox_->isChecked();

    if (config.pool_mining) {
        config.pool_host = poolHostEdit_->text().toStdString();
        config.pool_port = poolPortSpinBox_->value();
        config.pool_username = poolUsernameEdit_->text().toStdString();
        config.pool_password = poolPasswordEdit_->text().toStdString();

        if (config.pool_host.empty()) {
            QMessageBox::warning(this, tr("Missing Pool Configuration"),
                tr("Please enter pool host and port."));
            return;
        }
    }

    // Create miner
    miner_ = std::make_unique<mining::MiningManager>(config);

    // Set callbacks
    miner_->SetBlockFoundCallback([this](const Block& block) {
        QMetaObject::invokeMethod(this, &MiningPage::onBlockFound, Qt::QueuedConnection);
    });

    miner_->SetShareFoundCallback([this](const mining::MiningResult& result) {
        QMetaObject::invokeMethod(this, [this]() {
            onShareSubmitted(true);
        }, Qt::QueuedConnection);
    });

    // Start mining
    if (!blockchain_) {
        QMessageBox::critical(this, tr("Error"), tr("Blockchain not available"));
        return;
    }

    auto result = miner_->Start(*blockchain_);
    if (!result.IsOk()) {
        QMessageBox::critical(this, tr("Mining Error"),
            tr("Failed to start mining: %1")
            .arg(QString::fromStdString(result.error)));
        miner_.reset();
        return;
    }

    // Update UI
    is_mining_ = true;
    start_time_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    miningStatusLabel_->setText(tr("<b style='color: green;'>Mining...</b>"));
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    configGroup_->setEnabled(false);
    poolGroup_->setEnabled(false);

    logTextEdit_->append(tr("[%1] Mining started with %2 thread(s)")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(config.thread_count));

    if (config.pool_mining) {
        logTextEdit_->append(tr("[%1] Connected to pool: %2:%3")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(QString::fromStdString(config.pool_host))
            .arg(config.pool_port));
    } else {
        logTextEdit_->append(tr("[%1] Solo mining to address: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(address));
    }
}

void MiningPage::stopMining() {
    if (!miner_ || !is_mining_) {
        return;
    }

    miner_->Stop();
    miner_.reset();

    is_mining_ = false;

    miningStatusLabel_->setText(tr("<b style='color: red;'>Stopped</b>"));
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    configGroup_->setEnabled(true);
    poolGroup_->setEnabled(poolMiningCheckbox_->isChecked());

    logTextEdit_->append(tr("[%1] Mining stopped")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MiningPage::updateStats() {
    if (!is_mining_ || !miner_) {
        return;
    }

    auto stats = miner_->GetStats();

    // Update hashrate
    hashrateLabel_->setText(QString::fromStdString(
        mining::FormatHashrate(stats.hashrate)));

    // Update blocks found
    if (stats.blocks_found > blocks_found_) {
        blocks_found_ = stats.blocks_found;
        onBlockFound();
    }
    blocksFoundLabel_->setText(QString::number(blocks_found_));

    // Update uptime
    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    uint64_t uptime = current_time - start_time_;

    uint64_t hours = uptime / 3600;
    uint64_t minutes = (uptime % 3600) / 60;
    uint64_t seconds = uptime % 60;

    uptimeLabel_->setText(QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));

    // Update total hashes
    totalHashesLabel_->setText(QString::number(stats.hashes_computed));

    // Update shares (for pool mining)
    if (poolMiningCheckbox_->isChecked()) {
        if (stats.shares_submitted > shares_submitted_) {
            shares_submitted_ = stats.shares_submitted;
            shares_accepted_ = stats.shares_accepted;
        }
    }
}

void MiningPage::updateConfig() {
    // Configuration changes while mining require restart
    if (is_mining_) {
        QMessageBox::information(this, tr("Configuration Changed"),
            tr("Mining configuration has changed. Please restart mining for changes to take effect."));
    }
}

void MiningPage::togglePoolMining(bool enabled) {
    poolGroup_->setEnabled(enabled);
}

void MiningPage::onBlockFound() {
    blocks_found_++;

    logTextEdit_->append(tr("[%1] <b style='color: green;'>BLOCK FOUND!</b> Total: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(blocks_found_));

    QMessageBox::information(this, tr("Block Found!"),
        tr("Congratulations! You found a block!\n\n"
           "Block reward will be credited to your mining address once confirmed."));
}

void MiningPage::onShareSubmitted(bool accepted) {
    shares_submitted_++;
    if (accepted) {
        shares_accepted_++;
    }

    logTextEdit_->append(tr("[%1] Share submitted (%2) - Accepted: %3/%4")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(accepted ? "ACCEPTED" : "REJECTED")
        .arg(shares_accepted_)
        .arg(shares_submitted_));
}

void MiningPage::loadSettings() {
    QSettings settings("INTcoin", "INTcoin-Qt");

    settings.beginGroup("Mining");
    threadCountSpinBox_->setValue(
        settings.value("threadCount", std::thread::hardware_concurrency()).toInt());
    miningAddressEdit_->setText(settings.value("miningAddress").toString());
    poolMiningCheckbox_->setChecked(settings.value("poolMining", false).toBool());
    poolHostEdit_->setText(settings.value("poolHost", "pool.intcoin.org").toString());
    poolPortSpinBox_->setValue(settings.value("poolPort", 3333).toInt());
    poolUsernameEdit_->setText(settings.value("poolUsername").toString());
    settings.endGroup();
}

void MiningPage::saveSettings() {
    QSettings settings("INTcoin", "INTcoin-Qt");

    settings.beginGroup("Mining");
    settings.setValue("threadCount", threadCountSpinBox_->value());
    settings.setValue("miningAddress", miningAddressEdit_->text());
    settings.setValue("poolMining", poolMiningCheckbox_->isChecked());
    settings.setValue("poolHost", poolHostEdit_->text());
    settings.setValue("poolPort", poolPortSpinBox_->value());
    settings.setValue("poolUsername", poolUsernameEdit_->text());
    settings.endGroup();
}

} // namespace qt
} // namespace intcoin
