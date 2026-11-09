// commandrunner.cpp — see commandrunner.h.
#include "commandrunner.h"

#include "apppaths.h"
#include "secretstore.h"
#include "terminallauncher.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <csignal>
#include <sys/types.h>
#include <unistd.h>

// ============================================================ RunRecord
bool RunRecord::succeeded() const
{
    return error.isEmpty() && !crashed && exitCode == 0 && !stopRequested;
}

QString RunRecord::resultText() const
{
    if (running)
        return QStringLiteral("running");
    if (!error.isEmpty())
        return QStringLiteral("didn't start");
    if (mode != RunMode::Output)
        return note;
    if (stopRequested)
        return QStringLiteral("stopped");
    if (crashed)
        return QStringLiteral("ended abnormally");
    return QStringLiteral("exit code %1").arg(exitCode);
}

qint64 RunRecord::durationMs() const
{
    const QDateTime end = finished.isValid() ? finished : QDateTime::currentDateTime();
    return started.msecsTo(end);
}

// ========================================================= ProcessUtil
namespace ProcessUtil {

void prepareBash(QProcess *p, const QString &command, const QProcessEnvironment &env)
{
    p->setProgram(QStringLiteral("/bin/bash"));
    p->setArguments({QStringLiteral("-c"), command});
    p->setProcessEnvironment(env);
    p->setStandardInputFile(QProcess::nullDevice());
    p->setWorkingDirectory(QDir::homePath());
    // Its own session and process group, so Stop can end everything the
    // command started (not just bash) and a terminal hang-up can't reach it.
    p->setChildProcessModifier([] { ::setsid(); });
}

void signalGroup(QProcess *p, int sig)
{
    if (!p)
        return;
    const qint64 pid = p->processId();
    if (pid > 1)
        ::kill(-pid_t(pid), sig);
}

QString cleanForDisplay(const QString &text)
{
    static const QRegularExpression csi(QStringLiteral("\x1B\\[[0-9;?]*[ -/]*[@-~]"));
    static const QRegularExpression osc(QStringLiteral("\x1B\\][^\x07\x1B]*(?:\x07|\x1B\\\\)"));
    static const QRegularExpression other(QStringLiteral("\x1B[@-Z\\\\-_]"));
    QString s = text;
    s.remove(osc);
    s.remove(csi);
    s.remove(other);
    s.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    // A progress bar redraws its line with \r: keep only the last drawing.
    QStringList lines = s.split(QLatin1Char('\n'));
    for (QString &line : lines) {
        const int cr = int(line.lastIndexOf(QLatin1Char('\r')));
        if (cr >= 0)
            line = line.mid(cr + 1);
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace ProcessUtil

// ======================================================== CommandRunner
CommandRunner::CommandRunner(SecretStore *secrets, QObject *parent)
    : QObject(parent)
    , m_secrets(secrets)
{
}

CommandRunner::~CommandRunner()
{
    for (const QPointer<QProcess> &p : std::as_const(m_processes)) {
        if (p) {
            p->disconnect(this);
            ProcessUtil::signalGroup(p, SIGTERM);
            if (!p->waitForFinished(1500)) {
                ProcessUtil::signalGroup(p, SIGKILL);
                p->waitForFinished(1000);
            }
        }
    }
}

QProcessEnvironment CommandRunner::environment(const ButtonSpec &b) const
{
    QProcessEnvironment env = m_secrets->environmentFor({b.command});
    env.insert(QStringLiteral("CBC_LABEL"), b.label);
    return env;
}

QString CommandRunner::logPathFor(const QString &buttonId)
{
    QString safe = buttonId;
    safe.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9-]")));
    if (safe.isEmpty())
        safe = QStringLiteral("unnamed");
    return QDir(AppPaths::logDir()).filePath(safe + QStringLiteral(".log"));
}

quint64 CommandRunner::run(const ButtonSpec &b, bool test)
{
    RunRecord r;
    r.id = m_nextId++;
    r.buttonId = b.id;
    r.label = b.label.isEmpty() ? QStringLiteral("(no label)") : b.label;
    r.mode = b.mode;
    r.test = test;
    r.started = QDateTime::currentDateTime();
    m_records.insert(r.id, r);
    m_order.append(r.id);
    RunRecord &rec = m_records[r.id];

    switch (b.mode) {
    case RunMode::Terminal:   startTerminal(rec, b);   break;
    case RunMode::Output:     startOutput(rec, b);     break;
    case RunMode::Background: startBackground(rec, b); break;
    }
    const quint64 id = r.id;
    emit runStarted(id);
    emit historyChanged();
    if (!m_records.value(id).running)
        finish(id);
    trimHistory();
    return id;
}

void CommandRunner::startTerminal(RunRecord &r, const ButtonSpec &b)
{
    TerminalLauncher::Terminal term;
    if (!TerminalLauncher::resolve(m_terminal, &term)) {
        r.error = QStringLiteral("No terminal program was found (looked for gnome-terminal, "
                                 "konsole, xfce4-terminal, xterm …).");
        return;
    }
    // The command runs in a ( subshell ) so that `exit`, a trailing &, or a
    // comment on its last line can't swallow the closing message.
    const QString script =
        QStringLiteral("printf '\\033]0;%s\\007' \"$CBC_LABEL\"\n"
                       "(\n%1\n)\n"
                       "__cbc_rc=$?\n"
                       "printf '\\n\\033[1;36m── %s finished · exit code %d · "
                       "press Enter to close ──\\033[0m\\n' \"$CBC_LABEL\" \"$__cbc_rc\"\n"
                       "read -r __cbc_wait\n")
            .arg(b.command);
    QProcess p;
    p.setProgram(term.path);
    p.setArguments(TerminalLauncher::arguments(term.id, script));
    p.setProcessEnvironment(environment(b));
    p.setWorkingDirectory(QDir::homePath());
    p.setStandardInputFile(QProcess::nullDevice());
    qint64 pid = 0;
    if (!p.startDetached(&pid)) {
        r.error = QStringLiteral("Couldn't open %1 (%2).").arg(term.name, term.path);
        return;
    }
    r.note = QStringLiteral("opened in %1").arg(term.name);
}

void CommandRunner::startOutput(RunRecord &r, const ButtonSpec &b)
{
    auto *p = new QProcess(this);
    ProcessUtil::prepareBash(p, b.command, environment(b));
    p->setProcessChannelMode(QProcess::MergedChannels);
    const quint64 id = r.id;

    connect(p, &QProcess::readyReadStandardOutput, this, [this, p, id] {
        auto it = m_records.find(id);
        if (it == m_records.end())
            return;
        it->raw += p->readAllStandardOutput();
        if (it->raw.size() > MaxOutputBytes) {
            it->raw.remove(0, it->raw.size() - MaxOutputBytes);
            it->truncated = true;
        }
        emit runUpdated(id);
    });
    connect(p, &QProcess::finished, this, [this, p, id](int code, QProcess::ExitStatus st) {
        auto it = m_records.find(id);
        if (it != m_records.end()) {
            it->raw += p->readAllStandardOutput();
            it->exitCode = code;
            it->crashed = (st == QProcess::CrashExit);
            it->running = false;
        }
        m_processes.remove(id);
        p->deleteLater();
        finish(id);
    });
    connect(p, &QProcess::errorOccurred, this, [this, p, id](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart)
            return;
        auto it = m_records.find(id);
        if (it != m_records.end()) {
            it->error = QStringLiteral("bash couldn't be started: %1").arg(p->errorString());
            it->running = false;
        }
        m_processes.remove(id);
        p->deleteLater();
        finish(id);
    });

    r.running = true;
    m_processes.insert(id, p);
    p->start();
}

void CommandRunner::startBackground(RunRecord &r, const ButtonSpec &b)
{
    const QString log = logPathFor(b.id);
    r.logPath = log;

    // Keep logs bounded: past 2 MB the old log becomes .log.1.
    if (QFileInfo(log).size() > MaxLogBytes) {
        QFile::remove(log + QStringLiteral(".1"));
        QFile::rename(log, log + QStringLiteral(".1"));
        AppPaths::makePrivateFile(log + QStringLiteral(".1"));
    }
    QFile f(log);
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        r.error = QStringLiteral("Couldn't open the log %1: %2").arg(log, f.errorString());
        return;
    }
    AppPaths::makePrivateFile(log);
    f.write(QStringLiteral("\n===== %1 — started “%2” =====\n")
                .arg(r.started.toString(Qt::ISODate), r.label)
                .toUtf8());
    f.close();

    QProcess p;
    const QString setsid = QStandardPaths::findExecutable(QStringLiteral("setsid"));
    if (!setsid.isEmpty()) {
        p.setProgram(setsid);
        p.setArguments({QStringLiteral("/bin/bash"), QStringLiteral("-c"), b.command});
    } else {
        p.setProgram(QStringLiteral("/bin/bash"));
        p.setArguments({QStringLiteral("-c"), b.command});
    }
    QProcessEnvironment env = environment(b);
    env.insert(QStringLiteral("CBC_LOG"), log);
    p.setProcessEnvironment(env);
    p.setWorkingDirectory(QDir::homePath());
    p.setStandardInputFile(QProcess::nullDevice());
    p.setStandardOutputFile(log, QIODevice::Append);
    p.setStandardErrorFile(log, QIODevice::Append);
    qint64 pid = 0;
    if (!p.startDetached(&pid)) {
        r.error = QStringLiteral("Couldn't start it in the background.");
        return;
    }
    r.note = QStringLiteral("started in the background (process %1)").arg(pid);
}

void CommandRunner::finish(quint64 id)
{
    auto it = m_records.find(id);
    if (it == m_records.end())
        return;
    it->running = false;
    if (!it->finished.isValid())
        it->finished = QDateTime::currentDateTime();
    emit runFinished(id);
    emit historyChanged();
}

bool CommandRunner::stop(quint64 id)
{
    QPointer<QProcess> p = m_processes.value(id);
    auto it = m_records.find(id);
    if (!p || it == m_records.end() || !it->running)
        return false;
    it->stopRequested = true;
    ProcessUtil::signalGroup(p, SIGTERM);
    // Anything that ignores a polite stop is ended after 3 seconds.
    QTimer::singleShot(3000, this, [p] {
        if (p && p->state() != QProcess::NotRunning)
            ProcessUtil::signalGroup(p, SIGKILL);
    });
    emit runUpdated(id);
    return true;
}

void CommandRunner::stopAll()
{
    const QList<quint64> ids = m_processes.keys();
    for (quint64 id : ids)
        stop(id);
}

bool CommandRunner::isRunning(const QString &buttonId) const
{
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it) {
        const RunRecord *r = record(it.key());
        if (r && r->running && r->buttonId == buttonId)
            return true;
    }
    return false;
}

int CommandRunner::runningCount() const
{
    int n = 0;
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it)
        if (it.value())
            ++n;
    return n;
}

const RunRecord *CommandRunner::record(quint64 id) const
{
    auto it = m_records.constFind(id);
    return it == m_records.cend() ? nullptr : &it.value();
}

quint64 CommandRunner::lastRunFor(const QString &buttonId) const
{
    for (int i = int(m_order.size()) - 1; i >= 0; --i) {
        const RunRecord *r = record(m_order.at(i));
        if (r && r->buttonId == buttonId && !r->test)
            return r->id;
    }
    return 0;
}

QString CommandRunner::displayOutput(quint64 id) const
{
    const RunRecord *r = record(id);
    if (!r)
        return QString();
    QString text = ProcessUtil::cleanForDisplay(QString::fromLocal8Bit(r->raw));
    if (r->truncated)
        text.prepend(QStringLiteral("[… earlier output dropped — only the last 1 MB is kept]\n"));
    return m_secrets->redact(text);
}

void CommandRunner::clearFinished()
{
    QList<quint64> keep;
    for (quint64 id : std::as_const(m_order)) {
        if (m_records.value(id).running)
            keep.append(id);
        else
            m_records.remove(id);
    }
    m_order = keep;
    emit historyChanged();
}

void CommandRunner::trimHistory()
{
    bool trimmed = false;
    while (m_order.size() > HistoryLimit) {
        int victim = -1;
        for (int i = 0; i < m_order.size(); ++i)
            if (!m_records.value(m_order.at(i)).running) {
                victim = i;
                break;
            }
        if (victim < 0)
            break;
        m_records.remove(m_order.takeAt(victim));
        trimmed = true;
    }
    if (trimmed)
        emit historyChanged();
}
