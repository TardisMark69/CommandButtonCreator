// apppaths.h — where Command Button Creator keeps its files.
//
// Everything lives in the user's own home folder, in folders only the user
// can open (permissions 0700), and every file holding commands or secrets is
// 0600. Nothing is ever sent anywhere.
//
//   ~/.config/MarkCo/CommandButtonCreator/buttons.json    the buttons
//   ~/.config/MarkCo/CommandButtonCreator/secrets.json    the secrets vault
//   ~/.config/MarkCo/CommandButtonCreator/backups/        automatic backups
//   ~/.local/share/MarkCo/CommandButtonCreator/logs/      Background-mode logs
//   ~/.config/MarkCo/CommandButtonCreator.conf            window/preferences
#ifndef APPPATHS_H
#define APPPATHS_H

#include <QString>

namespace AppPaths {

QString configDir();          // created 0700 on first use
QString buttonsFile();
QString secretsFile();
QString backupDir();          // created 0700 on first use
QString logDir();             // created 0700 on first use
QString lockFile();
QString iconFile();           // PNG written for the desktop launcher
QString launcherFile();       // ~/.local/share/applications/…desktop

// The 1.x program kept buttons.json beside the executable.
QString legacyButtonsFile();

// mkpath + chmod 0700. Returns false if the folder can't be made.
bool ensurePrivateDir(const QString &path);
// chmod 0600 on an existing file.
bool makePrivateFile(const QString &path);
// True when group or others can read or write the file.
bool isExposed(const QString &path);

} // namespace AppPaths

#endif // APPPATHS_H
