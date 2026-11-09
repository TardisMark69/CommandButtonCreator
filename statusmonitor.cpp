// statusmonitor.cpp — see statusmonitor.h.
#include "statusmonitor.h"

#include "commandrunner.h"
#include "secretstore.h"

#include <QProcess>

#include <csignal>

StatusMonitor::StatusMonitor(SecretStore *secrets, QObject *parent)
    : QObject(parent)
    , m_secrets(secrets)
{
    m_timer.setInterval(30 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &StatusMonitor::refreshAll);
    m_timer.start();
}

StatusMonitor::~StatusMonitor()
{
    for (const QPointer<QProcess> &p : std::as_const(m_running))
        if (p) {
            p->disconnect(this);
            ProcessUtil::signalGroup(p, SIGKILL);
            p->waitForFinished(1000);
        }
}

void StatusMonitor::setButtons(const QList<ButtonSpec> &buttons)
{
    QHash<QString, QString> checks;
    for (const ButtonSpec &b : buttons)
        if (!b.statusCheck.trimmed().isEmpty())
            checks.insert(b.id, b.statusCheck);

    QStringList changedIds;
    // Lights that no longer have a check go dark.
    for (auto it = m_info.begin(); it != m_info.end();) {
        if (!checks.contains(it.key())) {
            changedIds.append(it.key());
            it = m_info.erase(it);
        } else {
            ++it;
        }
    }
    // New or edited checks are run straight away.
    QStringList fresh;
    for (auto it = checks.cbegin(); it != checks.cend(); ++it)
        if (m_checks.value(it.key()) != it.value())
            fresh.append(it.key());
    m_checks = checks;
    for (const QString &id : changedIds)
        emit statusChanged(id);
    for (const QString &id : fresh)
        refresh(id);
}

void StatusMonitor::setIntervalSec(int seconds)
{
    m_timer.setInterval(qMax(10, seconds) * 1000);
}

void StatusMonitor::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    if (!paused)
        refreshAll();
}

void StatusMonitor::refreshAll()
{
    if (m_paused)
        return;
    for (auto it = m_checks.cbegin(); it != m_checks.cend(); ++it)
        if (!m_queue.contains(it.key()) && !m_running.contains(it.key()))
            m_queue.append(it.key());
    pump();
}

void StatusMonitor::refresh(const QString &buttonId)
{
    if (!m_checks.contains(buttonId) || m_queue.contains(buttonId)
        || m_running.contains(buttonId))
        return;
    m_queue.prepend(buttonId);
    pump();
}

void StatusMonitor::refreshSoon(const QString &buttonId, int delayMs)
{
    if (!m_checks.contains(buttonId))
        return;
    QTimer::singleShot(delayMs, this, [this, buttonId] { refresh(buttonId); });
}

void StatusMonitor::pump()
{
    while (m_running.size() < MaxParallel && !m_queue.isEmpty())
        startCheck(m_queue.takeFirst());
}

void StatusMonitor::startCheck(const QString &buttonId)
{
    const QString command = m_checks.value(buttonId);
    if (command.isEmpty())
        return;

    StatusInfo &info = m_info[buttonId];
    if (info.state == StatusState::None) {
        info.state = StatusState::Checking;
        emit statusChanged(buttonId);
    }

    auto *p = new QProcess(this);
    ProcessUtil::prepareBash(p, command, m_secrets->environmentFor({command}));
    p->setProcessChannelMode(QProcess::MergedChannels);
    m_running.insert(buttonId, p);

    auto *timeout = new QTimer(p);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [p] {
        p->setProperty("timedOut", true);
        ProcessUtil::signalGroup(p, SIGKILL);
    });

    connect(p, &QProcess::finished, this, [this, p, buttonId](int code, QProcess::ExitStatus st) {
        QString last = QString::fromLocal8Bit(p->readAll().right(400)).trimmed();
        last = m_secrets->redact(ProcessUtil::cleanForDisplay(last));
        const int nl = int(last.lastIndexOf(QLatin1Char('\n')));
        if (nl >= 0)
            last = last.mid(nl + 1);
        m_running.remove(buttonId);
        p->deleteLater();
        if (p->property("timedOut").toBool())
            done(buttonId, StatusState::Down, QStringLiteral("no answer within 8 seconds"));
        else if (st == QProcess::CrashExit)
            done(buttonId, StatusState::Down, QStringLiteral("check ended abnormally"));
        else
            done(buttonId, code == 0 ? StatusState::Up : StatusState::Down,
                 last.isEmpty() ? QStringLiteral("exit code %1").arg(code) : last);
        pump();
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, buttonId](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart)
            return;
        m_running.remove(buttonId);
        p->deleteLater();
        done(buttonId, StatusState::Down, QStringLiteral("couldn't start bash"));
        pump();
    });

    p->start();
    timeout->start(TimeoutMs);
}

void StatusMonitor::done(const QString &buttonId, StatusState state, const QString &detail)
{
    if (!m_checks.contains(buttonId))
        return;
    StatusInfo &info = m_info[buttonId];
    const bool changed = info.state != state || info.detail != detail;
    info.state = state;
    info.detail = detail.left(200);
    info.checked = QDateTime::currentDateTime();
    if (changed)
        emit statusChanged(buttonId);
}
