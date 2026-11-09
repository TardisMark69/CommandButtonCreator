// statusmonitor.h — the little status light on a button.
//
// A button may have a "status check": a short command that exits 0 when
// the thing the button controls is up (e.g.  pgrep -x ollama). The monitor
// runs every check now and then — at most 4 at a time, each stopped after 8
// seconds — and turns the light green (exit 0) or red (anything else).
#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include "buttonspec.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

class QProcess;
class SecretStore;

enum class StatusState { None, Checking, Up, Down };

struct StatusInfo {
    StatusState state = StatusState::None;
    QDateTime checked;
    QString detail;       // last line printed, or the reason it's red
};

class StatusMonitor : public QObject
{
    Q_OBJECT
public:
    static constexpr int MaxParallel = 4;
    static constexpr int TimeoutMs = 8000;

    StatusMonitor(SecretStore *secrets, QObject *parent = nullptr);
    ~StatusMonitor() override;

    void setButtons(const QList<ButtonSpec> &buttons);
    void setIntervalSec(int seconds);
    void setPaused(bool paused);          // e.g. while minimized

    void refreshAll();
    void refresh(const QString &buttonId);
    void refreshSoon(const QString &buttonId, int delayMs);

    StatusInfo info(const QString &buttonId) const { return m_info.value(buttonId); }

signals:
    void statusChanged(const QString &buttonId);

private:
    void pump();
    void startCheck(const QString &buttonId);
    void done(const QString &buttonId, StatusState state, const QString &detail);

    SecretStore *m_secrets;
    QHash<QString, QString> m_checks;           // id -> status command
    QHash<QString, StatusInfo> m_info;
    QStringList m_queue;
    QHash<QString, QPointer<QProcess>> m_running;
    QTimer m_timer;
    bool m_paused = false;
};

#endif // STATUSMONITOR_H
