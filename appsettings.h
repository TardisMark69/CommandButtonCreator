// appsettings.h — typed access to the program's preferences (QSettings,
// MarkCo/CommandButtonCreator). One place for every key and its default, so
// no other file spells a settings key by hand.
#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QByteArray>
#include <QString>

class AppSettings
{
public:
    static constexpr int MinZoomPt = 7;
    static constexpr int MaxZoomPt = 24;
    static constexpr int DefaultZoomPt = 10;

    // Base point size for the whole interface (Ctrl+wheel, Ctrl +/-).
    static int zoomPt();
    static void setZoomPt(int pt);

    // Lock layout: buttons can't be dragged or resized.
    static bool locked();
    static void setLocked(bool on);

    // Terminal program id ("auto" = first one found).
    static QString terminal();
    static void setTerminal(const QString &id);

    // Seconds between status-light checks.
    static int statusIntervalSec();
    static void setStatusIntervalSec(int s);

    static bool autoShowOutput();
    static void setAutoShowOutput(bool on);

    static bool snapToGrid();
    static void setSnapToGrid(bool on);

    static int currentPage();
    static void setCurrentPage(int index);

    static QByteArray windowGeometry();
    static void setWindowGeometry(const QByteArray &g);
    static QByteArray windowState();
    static void setWindowState(const QByteArray &s);

    static QByteArray dialogGeometry(const QString &dialog);
    static void setDialogGeometry(const QString &dialog, const QByteArray &g);

    // Version whose "What's new" page has been shown.
    static QString whatsNewSeen();
    static void setWhatsNewSeen(const QString &version);
};

#endif // APPSETTINGS_H
