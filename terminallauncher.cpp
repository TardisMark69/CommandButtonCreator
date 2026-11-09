// terminallauncher.cpp — see terminallauncher.h.
#include "terminallauncher.h"

#include <QStandardPaths>

namespace {

struct Known {
    const char *id;
    const char *name;
};

// Best first. gnome-terminal is the Pop!_OS / GNOME default.
const Known kKnown[] = {
    {"gnome-terminal",      "GNOME Terminal"},
    {"kgx",                 "GNOME Console"},
    {"konsole",             "Konsole"},
    {"xfce4-terminal",      "Xfce Terminal"},
    {"x-terminal-emulator", "System default terminal"},
    {"xterm",               "xterm"},
};

} // namespace

QList<TerminalLauncher::Terminal> TerminalLauncher::detected()
{
    QList<Terminal> out;
    for (const Known &k : kKnown) {
        const QString path = QStandardPaths::findExecutable(QString::fromLatin1(k.id));
        if (!path.isEmpty())
            out.append({QString::fromLatin1(k.id), QString::fromLatin1(k.name), path});
    }
    return out;
}

bool TerminalLauncher::resolve(const QString &preferredId, Terminal *out)
{
    const QList<Terminal> all = detected();
    for (const Terminal &t : all)
        if (t.id == preferredId) {
            *out = t;
            return true;
        }
    if (all.isEmpty())
        return false;
    *out = all.first();
    return true;
}

QStringList TerminalLauncher::arguments(const QString &id, const QString &script)
{
    const QStringList bashArgs{QStringLiteral("/bin/bash"), QStringLiteral("-c"), script};
    if (id == QLatin1String("gnome-terminal") || id == QLatin1String("kgx"))
        return QStringList{QStringLiteral("--")} + bashArgs;
    if (id == QLatin1String("xfce4-terminal"))
        return QStringList{QStringLiteral("-x")} + bashArgs;
    // konsole, x-terminal-emulator, xterm: -e takes the program and its args
    return QStringList{QStringLiteral("-e")} + bashArgs;
}
