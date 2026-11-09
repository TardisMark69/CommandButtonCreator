// main.cpp — Command Button Creator 2.0 (Tardis Software).
//
// One-press buttons for the shell commands you'd otherwise type.
// Created by Mark Mcdougal, March 5, 2026. Version 2.0, November 2026.
#include "apppaths.h"
#include "appsettings.h"
#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("MarkCo"));
    QApplication::setApplicationName(QStringLiteral("CommandButtonCreator"));
    QApplication::setApplicationDisplayName(QStringLiteral("Command Button Creator"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("commandbuttoncreator"));
    QApplication::setWindowIcon(Theme::appIcon());
    Theme::apply(app, AppSettings::zoomPt());

    // One copy at a time, so two windows can't overwrite each other's saves.
    QLockFile lock(AppPaths::lockFile());
    lock.setStaleLockTime(0);
    if (!lock.tryLock(200)) {
        QMessageBox::information(nullptr, QStringLiteral("Command Button Creator"),
                                 QStringLiteral("Command Button Creator is already running.\n\n"
                                                "Look for its window (it may be minimized)."));
        return 0;
    }

    MainWindow w;
    w.show();
    w.start();
    return app.exec();
}
