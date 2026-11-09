// secretstore.cpp — see secretstore.h.
#include "secretstore.h"

#include "apppaths.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace {

// Variables a secret must never replace: changing them would change how
// every program in the command behaves (or let a value be executed).
const QSet<QString> &reservedNames()
{
    static const QSet<QString> names{
        QStringLiteral("PATH"),     QStringLiteral("HOME"),      QStringLiteral("USER"),
        QStringLiteral("LOGNAME"),  QStringLiteral("SHELL"),     QStringLiteral("PWD"),
        QStringLiteral("OLDPWD"),   QStringLiteral("IFS"),       QStringLiteral("ENV"),
        QStringLiteral("CDPATH"),   QStringLiteral("PS1"),       QStringLiteral("PS2"),
        QStringLiteral("PS4"),      QStringLiteral("PROMPT_COMMAND"),
        QStringLiteral("DISPLAY"),  QStringLiteral("WAYLAND_DISPLAY"),
        QStringLiteral("TERM"),     QStringLiteral("LANG"),      QStringLiteral("LANGUAGE"),
        QStringLiteral("TMPDIR"),   QStringLiteral("EDITOR"),    QStringLiteral("VISUAL"),
        QStringLiteral("PAGER"),    QStringLiteral("SSH_AUTH_SOCK"),
        QStringLiteral("DBUS_SESSION_BUS_ADDRESS"), QStringLiteral("GLOBIGNORE"),
        QStringLiteral("SHELLOPTS"), QStringLiteral("HISTFILE"), QStringLiteral("MAIL"),
        QStringLiteral("CBC_LABEL"), QStringLiteral("CBC_LOG"),
    };
    return names;
}

const QStringList &reservedPrefixes()
{
    static const QStringList p{QStringLiteral("LD_"),   QStringLiteral("BASH"),
                               QStringLiteral("XDG_"),  QStringLiteral("LC_"),
                               QStringLiteral("QT_"),   QStringLiteral("GTK_"),
                               QStringLiteral("PYTHON"), QStringLiteral("PERL"),
                               QStringLiteral("NODE_"), QStringLiteral("SUDO_"),
                               QStringLiteral("GIT_")};
    return p;
}

const QRegularExpression &referenceRe()
{
    // $NAME  or  ${NAME}  or  ${NAME:-default} / ${NAME:?…} etc.
    static const QRegularExpression re(
        QStringLiteral(R"(\$(?:\{([A-Za-z_][A-Za-z0-9_]*)(?=[}:#%/^,@+=?-])|([A-Za-z_][A-Za-z0-9_]*)))"));
    return re;
}

} // namespace

SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
}

bool SecretStore::load(QString *error)
{
    m_secrets.clear();
    const QString path = AppPaths::secretsFile();
    if (!QFile::exists(path))
        return true;
    if (AppPaths::isExposed(path)) {
        AppPaths::makePrivateFile(path);
        m_fixedPermissions = true;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Couldn't open the secrets vault: %1").arg(f.errorString());
        return false;
    }
    if (f.size() > 8 * 1024 * 1024) {
        if (error)
            *error = QStringLiteral("The secrets file is unexpectedly large and was not read.");
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("The secrets file is damaged (%1).").arg(pe.errorString());
        return false;
    }
    const QJsonObject secrets = doc.object().value(QStringLiteral("secrets")).toObject();
    for (auto it = secrets.begin(); it != secrets.end(); ++it) {
        QString why;
        if (m_secrets.size() >= MaxSecrets)
            break;
        if (!it.value().isString() || !isValidName(it.key(), &why)
            || !isValidValue(it.value().toString(), &why))
            continue;
        m_secrets.insert(it.key(), it.value().toString());
    }
    return true;
}

bool SecretStore::save(QString *error)
{
    const QString path = AppPaths::secretsFile();
    QJsonObject secrets;
    for (auto it = m_secrets.cbegin(); it != m_secrets.cend(); ++it)
        secrets.insert(it.key(), it.value());
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("secrets"), secrets);

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Couldn't save the secrets vault: %1").arg(f.errorString());
        return false;
    }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        if (error)
            *error = QStringLiteral("Couldn't save the secrets vault: %1").arg(f.errorString());
        return false;
    }
    AppPaths::makePrivateFile(path);
    return true;
}

bool SecretStore::isValidName(const QString &name, QString *why)
{
    static const QRegularExpression re(QStringLiteral("^[A-Z_][A-Z0-9_]*$"));
    auto fail = [why](const QString &msg) {
        if (why)
            *why = msg;
        return false;
    };
    if (name.isEmpty())
        return fail(QStringLiteral("The name is empty."));
    if (name.size() > MaxName)
        return fail(QStringLiteral("The name is longer than %1 characters.").arg(MaxName));
    if (!re.match(name).hasMatch())
        return fail(QStringLiteral("Use capital letters, digits and _ only, not starting "
                                   "with a digit (for example HA_TOKEN)."));
    if (reservedNames().contains(name))
        return fail(QStringLiteral("%1 is a system variable and can't be a secret.").arg(name));
    for (const QString &p : reservedPrefixes())
        if (name.startsWith(p))
            return fail(QStringLiteral("Names starting with %1 are used by the system.").arg(p));
    return true;
}

bool SecretStore::isValidValue(const QString &value, QString *why)
{
    if (value.isEmpty()) {
        if (why)
            *why = QStringLiteral("The value is empty.");
        return false;
    }
    if (value.size() > MaxValue) {
        if (why)
            *why = QStringLiteral("The value is longer than %1 characters.").arg(MaxValue);
        return false;
    }
    if (value.contains(QChar(0))) {
        if (why)
            *why = QStringLiteral("The value contains a NUL character.");
        return false;
    }
    return true;
}

bool SecretStore::set(const QString &name, const QString &value, QString *why)
{
    if (!isValidName(name, why) || !isValidValue(value, why))
        return false;
    if (!m_secrets.contains(name) && m_secrets.size() >= MaxSecrets) {
        if (why)
            *why = QStringLiteral("The vault is full (%1 secrets).").arg(MaxSecrets);
        return false;
    }
    const QString old = m_secrets.value(name);
    const bool existed = m_secrets.contains(name);
    m_secrets.insert(name, value);
    if (!save(why)) {
        if (existed)
            m_secrets.insert(name, old);
        else
            m_secrets.remove(name);
        return false;
    }
    emit changed();
    return true;
}

bool SecretStore::remove(const QString &name)
{
    if (!m_secrets.contains(name))
        return false;
    const QString old = m_secrets.take(name);
    if (!save(nullptr)) {
        m_secrets.insert(name, old);
        return false;
    }
    emit changed();
    return true;
}

bool SecretStore::rename(const QString &oldName, const QString &newName, QString *why)
{
    if (!m_secrets.contains(oldName)) {
        if (why)
            *why = QStringLiteral("There is no secret called %1.").arg(oldName);
        return false;
    }
    if (oldName == newName)
        return true;
    if (!isValidName(newName, why))
        return false;
    if (m_secrets.contains(newName)) {
        if (why)
            *why = QStringLiteral("A secret called %1 already exists.").arg(newName);
        return false;
    }
    const QString v = m_secrets.take(oldName);
    m_secrets.insert(newName, v);
    if (!save(why)) {
        m_secrets.remove(newName);
        m_secrets.insert(oldName, v);
        return false;
    }
    emit changed();
    return true;
}

QStringList SecretStore::variableReferences(const QString &command)
{
    QStringList out;
    auto it = referenceRe().globalMatch(command);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString name = m.captured(1).isEmpty() ? m.captured(2) : m.captured(1);
        if (!out.contains(name))
            out.append(name);
    }
    return out;
}

QStringList SecretStore::referencedSecrets(const QString &command) const
{
    QStringList out;
    for (const QString &n : variableReferences(command))
        if (m_secrets.contains(n))
            out.append(n);
    return out;
}

QString SecretStore::renameReferences(const QString &command, const QString &oldName,
                                      const QString &newName)
{
    QString out;
    int last = 0;
    auto it = referenceRe().globalMatch(command);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const bool braced = !m.captured(1).isEmpty();
        const QString name = braced ? m.captured(1) : m.captured(2);
        if (name != oldName)
            continue;
        const int nameStart = braced ? m.capturedStart(1) : m.capturedStart(2);
        out += command.mid(last, m.capturedStart() - last);
        // ${OLD…  ->  ${NEW…   ;   $OLD  ->  ${NEW}
        out += braced ? QStringLiteral("${") + newName : QStringLiteral("${%1}").arg(newName);
        last = nameStart + int(name.size());
    }
    out += command.mid(last);
    return out;
}

QProcessEnvironment SecretStore::environmentFor(const QStringList &commands) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &c : commands)
        for (const QString &n : referencedSecrets(c))
            env.insert(n, m_secrets.value(n));
    return env;
}

QString SecretStore::redact(const QString &text) const
{
    if (m_secrets.isEmpty() || text.isEmpty())
        return text;
    // Longest first, so a secret that contains another is blanked whole.
    QList<QPair<QString, QString>> byLength;
    for (auto it = m_secrets.cbegin(); it != m_secrets.cend(); ++it)
        if (it.value().size() >= 4)
            byLength.append({it.value(), it.key()});
    std::sort(byLength.begin(), byLength.end(),
              [](const auto &a, const auto &b) { return a.first.size() > b.first.size(); });
    QString out = text;
    for (const auto &pair : byLength)
        out.replace(pair.first, QStringLiteral("••••(%1)").arg(pair.second));
    return out;
}

QString SecretStore::nameForValue(const QString &value) const
{
    for (auto it = m_secrets.cbegin(); it != m_secrets.cend(); ++it)
        if (it.value() == value)
            return it.key();
    return QString();
}

QString SecretStore::uniqueName(const QString &base) const
{
    QString clean;
    for (const QChar c : base.toUpper())
        clean += (c.isLetterOrNumber() && c.unicode() < 128) ? c : QChar('_');
    if (clean.isEmpty() || clean.at(0).isDigit())
        clean.prepend(QStringLiteral("SECRET_"));
    QString why;
    if (!isValidName(clean, &why))
        clean = QStringLiteral("SECRET_") + clean;
    clean.truncate(MaxName - 4);
    QString name = clean;
    for (int n = 2; m_secrets.contains(name); ++n)
        name = QStringLiteral("%1_%2").arg(clean).arg(n);
    return name;
}

QMap<QString, QString> SecretStore::vault(const QList<SecretScanner::Finding> &findings,
                                          QString *why)
{
    QMap<QString, QString> filed;
    for (const SecretScanner::Finding &f : findings) {
        if (filed.contains(f.value))
            continue;
        QString name = nameForValue(f.value);
        if (name.isEmpty()) {
            name = uniqueName(f.suggestedName);
            if (!set(name, f.value, why))
                break;
        }
        filed.insert(f.value, name);
    }
    return filed;
}
