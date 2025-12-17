// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#ifndef INTCOIN_QT_MNEMONICDIALOG_H
#define INTCOIN_QT_MNEMONICDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <vector>
#include <string>

namespace intcoin {
namespace qt {

/**
 * @brief Custom QTextEdit that disables copy/paste for security
 *
 * This widget displays the mnemonic seed phrase but prevents
 * clipboard operations to reduce risk of malware stealing seeds.
 */
class SecureTextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit SecureTextEdit(QWidget *parent = nullptr);

protected:
    // Override context menu to disable copy/paste
    void contextMenuEvent(QContextMenuEvent *event) override;

    // Override keyboard shortcuts
    void keyPressEvent(QKeyEvent *event) override;

    // Disable clipboard operations
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
};

/**
 * @brief Dialog for securely displaying mnemonic seed phrase
 *
 * Features:
 * - Displays 24-word mnemonic in a secure read-only field
 * - Clipboard copy/paste disabled for security
 * - Print button for physical backup
 * - Warning messages about backup importance
 * - Confirmation checkbox before closing
 */
class MnemonicDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct mnemonic display dialog
     * @param words The 24-word mnemonic phrase
     * @param parent Parent widget
     */
    explicit MnemonicDialog(const std::vector<std::string>& words, QWidget *parent = nullptr);
    ~MnemonicDialog() override = default;

private slots:
    void onPrintClicked();
    void onConfirmationChanged(Qt::CheckState state);

private:
    void setupUI();
    void printMnemonic();

    std::vector<std::string> mnemonic_words_;

    // UI Components
    SecureTextEdit *mnemonicText_;
    QPushButton *printButton_;
    QPushButton *closeButton_;
    QLabel *warningLabel_;
    QCheckBox *confirmCheckBox_;
};

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_MNEMONICDIALOG_H
