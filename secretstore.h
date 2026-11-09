// secretstore.h — the secrets vault: tokens and passwords kept OUT of the
// button commands.
//
// A command refers to a secret by name, the normal shell way:  $HA_TOKEN  or
// ${HA_TOKEN}. When the button runs, the secret is handed to the command as
// an environment variable — never pasted into the command line, because
// command lines are visible to every user on the computer (ps, /proc), while
// a process's environment is readable only by you and root.
//
// Only the secrets a command actually names are passed to it. Anything a
// command prints is shown with secret values blanked out.
//
// Storage: secrets.json, permissions 0600 in a 0700 folder — the same
// protection as your ~/.ssh keys. It isn't encrypted with a password, by
// design: a password prompt would mean typing every session.
#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include "secretscanner.h"

#include <QMap>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

class SecretStore : public QObject
{
    Q_OBJECT
public:
    static constexpr int MaxValue   = 16384;
    static constexpr int MaxSecrets = 500;
    static constexpr int MaxName    = 64;

    explicit SecretStore(QObject *parent = nullptr);

    bool load(QString *error);
    bool save(QString *error);
    bool permissionsWereFixed() const { return m_fixedPermissions; }

    QStringList names() const { return m_secrets.keys(); }
    bool contains(const QString &name) const { return m_secrets.contains(name); }
    QString value(const QString &name) const { return m_secrets.value(name); }
    int count() const { return int(m_secrets.size()); }

    // Names must look like shell variables (A–Z, 0–9, _) and must not
    // replace a variable the system relies on (PATH, HOME, LD_*, …).
    static bool isValidName(const QString &name, QString *why);
    static bool isValidValue(const QString &value, QString *why);

    bool set(const QString &name, const QString &value, QString *why);
    bool remove(const QString &name);
    bool rename(const QString &oldName, const QString &newName, QString *why);

    // Every $NAME / ${NAME} written in a command (whether saved or not).
    static QStringList variableReferences(const QString &command);
    // Only the referenced names that are saved secrets.
    QStringList referencedSecrets(const QString &command) const;
    // Rewrite $OLD / ${OLD} as ${NEW} in a command.
    static QString renameReferences(const QString &command, const QString &oldName,
                                    const QString &newName);

    // The program's environment plus the secrets the commands name.
    QProcessEnvironment environmentFor(const QStringList &commands) const;

    // Blank out every secret value in text.
    QString redact(const QString &text) const;

    QString nameForValue(const QString &value) const;   // "" if not stored

    // File scanner findings in the vault (reusing a name when the same value
    // is already saved). Returns value -> secret name for each one filed.
    QMap<QString, QString> vault(const QList<SecretScanner::Finding> &findings, QString *why);
    QString uniqueName(const QString &base) const;

signals:
    void changed();

private:
    QMap<QString, QString> m_secrets;
    bool m_fixedPermissions = false;
};

#endif // SECRETSTORE_H
