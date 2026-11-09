// editdialog.h — make or change one button.
//
// Everything about a button is here: label, how it runs, the command,
// an optional status check, color, page and "ask before running". The
// Command checker underneath re-reads the command as you type and explains
// anything that won't work as it looks. "Insert secret" puts ${NAME} at the
// cursor, so a token never has to be typed or pasted into a command.
#ifndef EDITDIALOG_H
#define EDITDIALOG_H

#include "buttonspec.h"

#include <QDialog>
#include <QPointer>
#include <QStringList>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QToolButton;
class SecretStore;

class EditDialog : public QDialog
{
    Q_OBJECT
public:
    EditDialog(const ButtonSpec &spec, const QStringList &pageNames, int page,
               SecretStore *secrets, bool isNew, QWidget *parent = nullptr);
    ~EditDialog() override;

    ButtonSpec spec() const;          // the edited button
    int page() const;                 // the page chosen
    bool deleteRequested() const { return m_delete; }

signals:
    void testRequested(const ButtonSpec &spec);
    void helpRequested(const QString &topic);

protected:
    void done(int result) override;

private:
    void relint();
    void updateModeHint();
    void moveSecretsToVault();
    void rebuildSecretMenu();
    void insertSecret(const QString &name);
    void tryStatusCheck();
    void accept() override;

    ButtonSpec m_original;
    SecretStore *m_secrets;
    bool m_isNew;
    bool m_delete = false;

    QLineEdit *m_label;
    QComboBox *m_mode;
    QLabel *m_modeHint;
    QPlainTextEdit *m_command;
    QPushButton *m_insertSecret;
    QPushButton *m_test;
    QLineEdit *m_status;
    QPushButton *m_statusTry;
    QLabel *m_statusResult;
    QComboBox *m_color;
    QComboBox *m_page;
    QCheckBox *m_confirm;
    QListWidget *m_lint;
    QPushButton *m_vault;
    QLabel *m_vaultNote;

    QTimer m_lintTimer;
    QPointer<QProcess> m_statusProc;
};

#endif // EDITDIALOG_H
