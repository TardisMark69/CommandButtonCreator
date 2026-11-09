// apppaths.cpp — see apppaths.h.
#include "apppaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

QString inDir(const QString &dir, const QString &name)
{
    return QDir(dir).filePath(name);
}

QString privateSubdir(const QString &base, const QString &name)
{
    const QString d = name.isEmpty() ? base : inDir(base, name);
    AppPaths::ensurePrivateDir(d);
    return d;
}

} // namespace

namespace AppPaths {

QString configDir()
{
    return privateSubdir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), QString());
}

QString buttonsFile()  { return inDir(configDir(), QStringLiteral("buttons.json")); }
QString secretsFile()  { return inDir(configDir(), QStringLiteral("secrets.json")); }
QString backupDir()    { return privateSubdir(configDir(), QStringLiteral("backups")); }
QString lockFile()     { return inDir(configDir(), QStringLiteral("instance.lock")); }

QString logDir()
{
    const QString base = privateSubdir(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), QString());
    return privateSubdir(base, QStringLiteral("logs"));
}

QString iconFile()
{
    const QString base = privateSubdir(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), QString());
    return inDir(base, QStringLiteral("commandbuttoncreator.png"));
}

QString launcherFile()
{
    const QString apps =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(apps);
    return inDir(apps, QStringLiteral("commandbuttoncreator.desktop"));
}

QString legacyButtonsFile()
{
    return inDir(QCoreApplication::applicationDirPath(), QStringLiteral("buttons.json"));
}

bool ensurePrivateDir(const QString &path)
{
    if (path.isEmpty())
        return false;
    if (!QDir().mkpath(path))
        return false;
    return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                           | QFileDevice::ExeOwner);
}

bool makePrivateFile(const QString &path)
{
    return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

bool isExposed(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists())
        return false;
    const QFileDevice::Permissions p = fi.permissions();
    return p & (QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther
                | QFileDevice::WriteOther);
}

} // namespace AppPaths
