// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/qt/mainwindow.h"
#include "intcoin/wallet.h"
#include "intcoin/blockchain.h"
#include "intcoin/network.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setOrganizationName("INTcoin");
    QCoreApplication::setOrganizationDomain("international-coin.org");
    QCoreApplication::setApplicationName("INTcoin Core");
    QCoreApplication::setApplicationVersion("1.0.0-alpha");

    // TODO: Initialize wallet, blockchain, and network
    // For now, create nullptr placeholders
    intcoin::Wallet* wallet = nullptr;
    intcoin::Blockchain* blockchain = nullptr;
    intcoin::P2PNode* p2p = nullptr;

    try {
        // Create and show main window
        intcoin::qt::MainWindow mainWindow(wallet, blockchain, p2p);
        mainWindow.show();

        return app.exec();
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
