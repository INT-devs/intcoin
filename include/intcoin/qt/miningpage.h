// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_MININGPAGE_H
#define INTCOIN_QT_MININGPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimer>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <memory>

namespace intcoin {

// Forward declarations
namespace wallet { class Wallet; }
class Blockchain;

namespace mining {
    class MiningManager;
    struct MiningStats;
    struct MiningConfig;
}

namespace qt {

/**
 * Mining Page - Solo and Pool Mining Interface
 *
 * Allows users to mine directly from the wallet using their CPU.
 * Supports both solo mining and pool mining.
 */
class MiningPage : public QWidget {
    Q_OBJECT

public:
    explicit MiningPage(wallet::Wallet* wallet, Blockchain* blockchain, QWidget* parent = nullptr);
    ~MiningPage() override;

public slots:
    /**
     * Start mining
     */
    void startMining();

    /**
     * Stop mining
     */
    void stopMining();

    /**
     * Update mining statistics
     */
    void updateStats();

    /**
     * Update mining configuration
     */
    void updateConfig();

    /**
     * Toggle pool mining
     */
    void togglePoolMining(bool enabled);

    /**
     * Handle block found
     */
    void onBlockFound();

    /**
     * Handle share submitted
     */
    void onShareSubmitted(bool accepted);

private:
    void setupUi();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    // UI Components
    QGroupBox* statusGroup_;
    QLabel* miningStatusLabel_;
    QLabel* hashrateLabel_;
    QLabel* blocksFoundLabel_;
    QLabel* uptimeLabel_;
    QLabel* totalHashesLabel_;

    QGroupBox* configGroup_;
    QSpinBox* threadCountSpinBox_;
    QLineEdit* miningAddressEdit_;
    QPushButton* generateAddressButton_;
    QCheckBox* poolMiningCheckbox_;

    QGroupBox* poolGroup_;
    QLineEdit* poolHostEdit_;
    QSpinBox* poolPortSpinBox_;
    QLineEdit* poolUsernameEdit_;
    QLineEdit* poolPasswordEdit_;

    QPushButton* startButton_;
    QPushButton* stopButton_;

    QTextEdit* logTextEdit_;

    QTimer* updateTimer_;

    // Backend
    wallet::Wallet* wallet_;
    Blockchain* blockchain_;
    std::unique_ptr<mining::MiningManager> miner_;

    // State
    bool is_mining_{false};
    uint64_t start_time_{0};
    uint64_t blocks_found_{0};
    uint64_t shares_submitted_{0};
    uint64_t shares_accepted_{0};
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_MININGPAGE_H
