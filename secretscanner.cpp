// secretscanner.cpp — see secretscanner.h.
#include "secretscanner.h"

#include <QRegularExpression>

namespace SecretScanner {

namespace {

struct Pattern {
    QRegularExpression re;
    int group;               // capture group holding the secret
    QString kind;
    QString name;            // suggested vault name ("" = decide from context)
};

const QList<Pattern> &patterns()
{
    using RE = QRegularExpression;
    static const QList<Pattern> list{
        {RE(QStringLiteral(R"(Bearer\s+([A-Za-z0-9._~+/=-]{16,}))")), 1,
         QStringLiteral("Bearer token"), QString()},
        {RE(QStringLiteral(R"(\b(eyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}))")), 1,
         QStringLiteral("Web token (JWT)"), QString()},
        {RE(QStringLiteral(R"(\b(gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}))")), 1,
         QStringLiteral("GitHub token"), QStringLiteral("GITHUB_TOKEN")},
        {RE(QStringLiteral(R"(\b(sk-[A-Za-z0-9_-]{20,}))")), 1,
         QStringLiteral("API key"), QStringLiteral("API_KEY")},
        {RE(QStringLiteral(R"(\b(xox[abprs]-[A-Za-z0-9-]{10,}))")), 1,
         QStringLiteral("Slack token"), QStringLiteral("SLACK_TOKEN")},
        {RE(QStringLiteral(R"(\b(AKIA[0-9A-Z]{16})\b)")), 1,
         QStringLiteral("AWS access key"), QStringLiteral("AWS_KEY")},
        {RE(QStringLiteral(R"([A-Za-z][A-Za-z0-9+.-]*://[^/\s:@'"]+:([^@\s/'"$]{3,})@)")), 1,
         QStringLiteral("Password in a web address"), QStringLiteral("PASSWORD")},
        {RE(QStringLiteral(R"((?:\s-u|--user)\s+["']?[^\s:"']+:([^\s"'$]{3,}))")), 1,
         QStringLiteral("Password after -u"), QStringLiteral("PASSWORD")},
        {RE(QStringLiteral(R"(\bsshpass\s+-p\s*["']?([^\s"'$]+))")), 1,
         QStringLiteral("sshpass password"), QStringLiteral("SSH_PASSWORD")},
        {RE(QStringLiteral(R"(["']?\b(password|passwd|pwd|pass|token|api[_-]?key|apikey|secret|access[_-]?key|auth)\b["']?\s*[=:]\s*["']?([^\s"'&;|$<>(){}]{6,}))"),
            QRegularExpression::CaseInsensitiveOption),
         2, QStringLiteral("Password or key"), QString()},
    };
    return list;
}

QString contextualName(const QString &command, const QString &kind, const QString &key)
{
    const QString c = command.toLower();
    if (c.contains(QStringLiteral(":8123")) || c.contains(QStringLiteral("homeassistant"))
        || c.contains(QStringLiteral("home-assistant")) || c.contains(QStringLiteral("/api/services")))
        return QStringLiteral("HA_TOKEN");
    const QString k = key.toLower();
    if (k.startsWith(QStringLiteral("pass")) || k == QStringLiteral("pwd"))
        return QStringLiteral("PASSWORD");
    if (k.contains(QStringLiteral("key")))
        return QStringLiteral("API_KEY");
    if (kind.contains(QStringLiteral("token"), Qt::CaseInsensitive) || k == QStringLiteral("token")
        || k == QStringLiteral("auth"))
        return QStringLiteral("TOKEN");
    return QStringLiteral("SECRET");
}

} // namespace

QList<Finding> scan(const QString &command)
{
    QList<Finding> found;
    auto known = [&found](const QString &v) {
        for (const Finding &f : found)
            if (f.value == v || f.value.contains(v) || v.contains(f.value))
                return true;
        return false;
    };
    for (const Pattern &p : patterns()) {
        auto it = p.re.globalMatch(command);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString value = m.captured(p.group);
            if (value.isEmpty() || value.contains(QLatin1Char('$')) || known(value))
                continue;
            Finding f;
            f.value = value;
            f.kind = p.kind;
            f.suggestedName = p.name.isEmpty()
                                  ? contextualName(command, p.kind, p.group == 2 ? m.captured(1)
                                                                                  : QString())
                                  : p.name;
            found.append(f);
        }
    }
    return found;
}

Quote quoteStateAt(const QString &text, int pos)
{
    Quote q = Quote::None;
    bool comment = false;
    for (int i = 0; i < pos && i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (comment) {
            if (c == QLatin1Char('\n'))
                comment = false;
            continue;
        }
        switch (q) {
        case Quote::None:
            if (c == QLatin1Char('\\'))
                ++i;
            else if (c == QLatin1Char('\''))
                q = Quote::Single;
            else if (c == QLatin1Char('"'))
                q = Quote::Double;
            else if (c == QLatin1Char('#') && (i == 0 || text.at(i - 1).isSpace()))
                comment = true;
            break;
        case Quote::Single:
            if (c == QLatin1Char('\''))
                q = Quote::None;
            break;
        case Quote::Double:
            if (c == QLatin1Char('\\'))
                ++i;
            else if (c == QLatin1Char('"'))
                q = Quote::None;
            break;
        }
    }
    return q;
}

QString replaceValue(const QString &command, const QString &value, const QString &name)
{
    if (value.isEmpty())
        return command;
    QList<int> positions;
    for (int at = command.indexOf(value); at >= 0; at = command.indexOf(value, at + int(value.size())))
        positions.append(at);
    QString out = command;
    for (int i = int(positions.size()) - 1; i >= 0; --i) {
        const int at = positions.at(i);
        QString with;
        switch (quoteStateAt(command, at)) {
        case Quote::Double: with = QStringLiteral("${%1}").arg(name); break;
        case Quote::Single: with = QStringLiteral("'\"${%1}\"'").arg(name); break;
        case Quote::None:   with = QStringLiteral("\"${%1}\"").arg(name); break;
        }
        out.replace(at, int(value.size()), with);
    }
    return out;
}

} // namespace SecretScanner
