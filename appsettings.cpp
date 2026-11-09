// appsettings.cpp — see appsettings.h.
#include "appsettings.h"

#include <QSettings>
#include <QtGlobal>

namespace {
const char kZoom[]        = "view/zoomPt";
const char kLocked[]      = "arrange/locked";
const char kSnap[]        = "arrange/snapToGrid";
const char kTerminal[]    = "run/terminal";
const char kInterval[]    = "status/intervalSec";
const char kAutoOutput[]  = "run/autoShowOutput";
const char kPage[]        = "view/currentPage";
const char kGeometry[]    = "window/geometry";
const char kState[]       = "window/state";
const char kWhatsNew[]    = "help/whatsNewSeen";
} // namespace

int AppSettings::zoomPt()
{
    return qBound(MinZoomPt, QSettings().value(kZoom, DefaultZoomPt).toInt(), MaxZoomPt);
}
void AppSettings::setZoomPt(int pt) { QSettings().setValue(kZoom, qBound(MinZoomPt, pt, MaxZoomPt)); }

bool AppSettings::locked() { return QSettings().value(kLocked, false).toBool(); }
void AppSettings::setLocked(bool on) { QSettings().setValue(kLocked, on); }

QString AppSettings::terminal()
{
    return QSettings().value(kTerminal, QStringLiteral("auto")).toString();
}
void AppSettings::setTerminal(const QString &id) { QSettings().setValue(kTerminal, id); }

int AppSettings::statusIntervalSec()
{
    return qBound(10, QSettings().value(kInterval, 30).toInt(), 600);
}
void AppSettings::setStatusIntervalSec(int s) { QSettings().setValue(kInterval, qBound(10, s, 600)); }

bool AppSettings::autoShowOutput() { return QSettings().value(kAutoOutput, true).toBool(); }
void AppSettings::setAutoShowOutput(bool on) { QSettings().setValue(kAutoOutput, on); }

bool AppSettings::snapToGrid() { return QSettings().value(kSnap, true).toBool(); }
void AppSettings::setSnapToGrid(bool on) { QSettings().setValue(kSnap, on); }

int AppSettings::currentPage() { return qMax(0, QSettings().value(kPage, 0).toInt()); }
void AppSettings::setCurrentPage(int index) { QSettings().setValue(kPage, index); }

QByteArray AppSettings::windowGeometry() { return QSettings().value(kGeometry).toByteArray(); }
void AppSettings::setWindowGeometry(const QByteArray &g) { QSettings().setValue(kGeometry, g); }
QByteArray AppSettings::windowState() { return QSettings().value(kState).toByteArray(); }
void AppSettings::setWindowState(const QByteArray &s) { QSettings().setValue(kState, s); }

QByteArray AppSettings::dialogGeometry(const QString &dialog)
{
    return QSettings().value(QStringLiteral("dialogs/") + dialog).toByteArray();
}
void AppSettings::setDialogGeometry(const QString &dialog, const QByteArray &g)
{
    QSettings().setValue(QStringLiteral("dialogs/") + dialog, g);
}

QString AppSettings::whatsNewSeen() { return QSettings().value(kWhatsNew).toString(); }
void AppSettings::setWhatsNewSeen(const QString &version) { QSettings().setValue(kWhatsNew, version); }
