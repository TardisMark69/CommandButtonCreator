// terminallauncher.h — finds a terminal program and builds the arguments
// that make it run a bash script.
//
// The script is always passed as ONE argument (bash -c SCRIPT), never
// pasted into a shell string, so nothing in a command can break out of
// its quoting.
#ifndef TERMINALLAUNCHER_H
#define TERMINALLAUNCHER_H

#include <QList>
#include <QString>
#include <QStringList>

class TerminalLauncher
{
public:
    struct Terminal {
        QString id;     // program name, e.g. "gnome-terminal"
        QString name;   // shown in Preferences
        QString path;   // absolute path found on this computer
    };

    // Terminals known to this program that are installed, best first.
    static QList<Terminal> detected();

    // The preferred one if installed, else the first detected. Returns
    // false when no terminal is installed at all.
    static bool resolve(const QString &preferredId, Terminal *out);

    // Arguments that make terminal `id` run: bash -c script
    static QStringList arguments(const QString &id, const QString &script);
};

#endif // TERMINALLAUNCHER_H
