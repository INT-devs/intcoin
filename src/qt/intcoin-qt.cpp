// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/mainwindow.h"
#include "intcoin/qt/mnemonicDialog.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/network.h"

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <memory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setOrganizationName("INTcoin");
    QCoreApplication::setOrganizationDomain("international-coin.org");
    QCoreApplication::setApplicationName("INTcoin Core");
    QCoreApplication::setApplicationVersion("1.0.0-alpha");

    try {
        // Initialize wallet configuration
        intcoin::wallet::WalletConfig walletConfig;
        walletConfig.data_dir = QDir::homePath().toStdString() + "/.intcoin/wallet";
        walletConfig.backup_dir = QDir::homePath().toStdString() + "/.intcoin/backups";

        // Create wallet instance
        auto wallet = std::make_unique<intcoin::wallet::Wallet>(walletConfig);

        // Try to load existing wallet
        auto loadResult = wallet->Load();
        if (!loadResult.IsOk()) {
            // No wallet exists - for now, create a simple one
            // In production, we'd show a wizard to create/restore wallet
            QMessageBox::information(nullptr, QObject::tr("First Run"),
                QObject::tr("No wallet found. A new wallet will be created."));

            // Generate a new mnemonic
            auto mnemonicResult = intcoin::wallet::Mnemonic::Generate(24);
            if (!mnemonicResult.IsOk()) {
                QMessageBox::critical(nullptr, QObject::tr("Error"),
                    QObject::tr("Failed to generate mnemonic seed."));
                return 1;
            }

            // Get the mnemonic words
            auto mnemonicWords = mnemonicResult.GetValue();

            // Create wallet with mnemonic
            auto createResult = wallet->Create(mnemonicWords);
            if (!createResult.IsOk()) {
                QMessageBox::critical(nullptr, QObject::tr("Error"),
                    QObject::tr("Failed to create wallet."));
                return 1;
            }

            // IMPORTANT: Show mnemonic to user for backup
            // This dialog forces user to confirm they've backed up their seed
            intcoin::qt::MnemonicDialog mnemonicDialog(mnemonicWords);
            if (mnemonicDialog.exec() != QDialog::Accepted) {
                QMessageBox::critical(nullptr, QObject::tr("Wallet Not Backed Up"),
                    QObject::tr("You must backup your recovery seed before using the wallet."));
                return 1;
            }
        }

        // Create main window first (wallet-only mode initially)
        intcoin::qt::MainWindow mainWindow(wallet.get(), nullptr, nullptr);
        mainWindow.show();
        mainWindow.setWindowTitle("INTcoin Wallet - Initializing...");

        // Process events to show the window immediately
        app.processEvents();

        // Initialize blockchain in background-friendly way
        auto blockchainDB = std::make_shared<intcoin::BlockchainDB>(
            QDir::homePath().toStdString() + "/.intcoin/blockchain"
        );
        auto blockchain = std::make_unique<intcoin::Blockchain>(blockchainDB);

        auto initResult = blockchain->Initialize();
        if (!initResult.IsOk()) {
            qWarning() << "Warning: Failed to initialize blockchain - running in offline mode";
        }

        // Initialize P2P network (non-blocking)
        auto p2p = std::make_unique<intcoin::P2PNode>(
            intcoin::network::MAINNET_MAGIC,
            intcoin::network::MAINNET_P2P_PORT
        );

        // Start P2P node in background
        QTimer::singleShot(100, [&p2p, &blockchain]() {
            auto startResult = p2p->Start();
            if (startResult.IsOk() && blockchain) {
                // Register blockchain callbacks for P2P relay
                blockchain->RegisterBlockCallback([p2p_ptr = p2p.get()](const intcoin::Block& block) {
                    p2p_ptr->BroadcastBlock(block.GetHash());
                });
                blockchain->RegisterTransactionCallback([p2p_ptr = p2p.get()](const intcoin::Transaction& tx) {
                    p2p_ptr->BroadcastTransaction(tx.GetHash());
                });
            }
        });

        // Update window title when ready
        mainWindow.setWindowTitle("INTcoin Wallet");

        // Run application event loop
        int result = app.exec();

        // Cleanup
        p2p->Stop();
        wallet->Close();

        return result;

    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, QObject::tr("INTcoin Core - Error"),
            QObject::tr("An error occurred:\n\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, QObject::tr("INTcoin Core - Error"),
            QObject::tr("An unknown error occurred."));
        return 1;
    }
}
