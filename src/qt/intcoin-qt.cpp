// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/mainwindow.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/network.h"

#include <QApplication>
#include <QMessageBox>
#include <QDir>
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

            // Create wallet with mnemonic
            auto createResult = wallet->Create(mnemonicResult.GetValue());
            if (!createResult.IsOk()) {
                QMessageBox::critical(nullptr, QObject::tr("Error"),
                    QObject::tr("Failed to create wallet."));
                return 1;
            }
        }

        // Initialize blockchain
        auto blockchainDB = std::make_shared<intcoin::BlockchainDB>(
            QDir::homePath().toStdString() + "/.intcoin/blockchain"
        );
        auto blockchain = std::make_unique<intcoin::Blockchain>(blockchainDB);

        auto initResult = blockchain->Initialize();
        if (!initResult.IsOk()) {
            QMessageBox::critical(nullptr, QObject::tr("Error"),
                QObject::tr("Failed to initialize blockchain."));
            return 1;
        }

        // Initialize P2P network
        auto p2p = std::make_unique<intcoin::P2PNode>(
            intcoin::network::MAINNET_MAGIC,
            intcoin::network::MAINNET_P2P_PORT
        );

        // Start P2P node (optional - can be delayed until wallet is ready)
        auto startResult = p2p->Start();
        if (!startResult.IsOk()) {
            // Non-fatal - can run without network
            qWarning() << "Warning: Failed to start P2P network";
        }

        // Create and show main window with actual instances
        intcoin::qt::MainWindow mainWindow(wallet.get(), blockchain.get(), p2p.get());
        mainWindow.show();

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
