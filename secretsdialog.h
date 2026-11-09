// secretsdialog.h — Tools → Secrets vault: see, add, change, rename and
// delete saved secrets. Values are hidden unless "Show values" is ticked.
//
// Renaming a secret also rewrites $OLD to ${NEW} in every button that uses
// it; the window does that part (it owns the buttons) when renamed() fires.
#ifndef SECRETSDIALOG_H
#define SECRETSDIALOG_H

#include "buttonspec.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QPushButton;
class QTableWidget;
class SecretStore;

class SecretsDialog : public QDialog
{
    Q_OBJECT
public:
    using ButtonsFn = std::function<QList<ButtonSpec>()>;

    SecretsDialog(SecretStore *secrets, ButtonsFn buttons, QWidget *parent = nullptr);

signals:
    void renamed(const QString &oldName, const QString &newName);
    void securityCheckRequested();
    void helpRequested(const QString &topic);

protected:
    void done(int result) override;

private:
    void reload();
    QString currentName() const;
    void addSecret();
    void changeValue();
    void renameSecret();
    void deleteSecret();
    QStringList usedBy(const QString &name) const;

    SecretStore *m_secrets;
    ButtonsFn m_buttons;
    QTableWidget *m_table;
    QCheckBox *m_show;
    QPushButton *m_change;
    QPushButton *m_rename;
    QPushButton *m_delete;
};

// The small "name + value" editor used by the vault dialog.
class SecretEditDialog : public QDialog
{
    Q_OBJECT
public:
    SecretEditDialog(const QString &title, const QString &name, bool nameEditable,
                     QWidget *parent = nullptr);
    QString name() const;
    QString value() const;

private:
    class QLineEdit *m_name;
    class QLineEdit *m_value;
};

#endif // SECRETSDIALOG_H
