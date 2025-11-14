// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <iostream>

using namespace intcoin::qt;

int main(int argc, char *argv[]) {
    try {
        QApplication app(argc, argv);

        // Set application metadata
        QCoreApplication::setOrganizationName("INTcoin");
        QCoreApplication::setOrganizationDomain("international-coin.org");
        QCoreApplication::setApplicationName("INTcoin-Qt");
        QCoreApplication::setApplicationVersion("1.1.0");

        // Load and apply INTcoin stylesheet
        QFile styleFile(":/styles/res/intcoin.qss");
        if (styleFile.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(styleFile.readAll());
            app.setStyleSheet(styleSheet);
            styleFile.close();
        }

        // Create and show main window
        MainWindow window;
        window.show();

        return app.exec();

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Application failed to start:\n\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        std::cerr << "FATAL ERROR: Unknown exception" << std::endl;
        QMessageBox::critical(nullptr, "Fatal Error",
            "Application failed to start due to unknown error");
        return 1;
    }
}
