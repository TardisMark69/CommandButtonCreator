// commandrunner.h — runs a button's command in the button's mode and keeps
// a record of every run for the Output panel.
//
//  Terminal window  – opens the terminal; the window stays open until you
//                     press Enter and shows the exit code.
//  Output panel     – runs bash in its own process group, captures what it
//                     prints (last 1 MB), and reports the exit code. Stop
//                     ends the whole group, including anything it started.
//  Background       – starts it in a new session (setsid) so it outlives
//                     this program; output is appended to a private log.
//
// Secrets are passed through the environment (see SecretStore), and output
// is shown with secret values blanked out.
#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include "buttonspec.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QProcessEnvironment>
#include <QString>

class QProcess;
class SecretStore;

struct RunRecord {
    quint64 id = 0;
    QString buttonId;
    QString label;
    RunMode mode = RunMode::Terminal;
    bool test = false;               // "Test run" from the Edit dialog
    QDateTime started;
    QDateTime finished;
    bool running = false;
    bool stopRequested = false;
    int exitCode = 0;
    bool crashed = false;            // ended by a signal (Stop, crash)
    QString error;                   // couldn't start
    QString note;                    // e.g. "Opened in GNOME Terminal"
    QByteArray raw;                  // captured output (Output mode)
    bool truncated = false;
    QString logPath;                 // Background mode

    // Finished with exit code 0 (or launched fine, for Terminal/Background).
    bool succeeded() const;
    QString resultText() const;      // "exit code 0", "stopped", …
    qint64 durationMs() const;
};

namespace ProcessUtil {
// bash -c COMMAND in its own process group, no stdin, given environment.
void prepareBash(QProcess *p, const QString &command, const QProcessEnvironment &env);
// Signal the whole process group the QProcess leads.
void signalGroup(QProcess *p, int sig);
// Remove terminal colour codes and carriage-return redraws for display.
QString cleanForDisplay(const QString &text);
} // namespace ProcessUtil

class CommandRunner : public QObject
{
    Q_OBJECT
public:
    static constexpr int MaxOutputBytes = 1024 * 1024;
    static constexpr int HistoryLimit = 200;
    static constexpr qint64 MaxLogBytes = 2 * 1024 * 1024;

    CommandRunner(SecretStore *secrets, QObject *parent = nullptr);
    ~CommandRunner() override;

    void setTerminal(const QString &id) { m_terminal = id; }

    quint64 run(const ButtonSpec &b, bool test = false);
    bool stop(quint64 id);
    void stopAll();

    bool isRunning(const QString &buttonId) const;
    int runningCount() const;
    QList<quint64> history() const { return m_order; }       // oldest first
    const RunRecord *record(quint64 id) const;
    quint64 lastRunFor(const QString &buttonId) const;
    QString displayOutput(quint64 id) const;                  // cleaned + redacted
    void clearFinished();

    static QString logPathFor(const QString &buttonId);

signals:
    void runStarted(quint64 id);
    void runUpdated(quint64 id);
    void runFinished(quint64 id);
    void historyChanged();

private:
    void startTerminal(RunRecord &r, const ButtonSpec &b);
    void startOutput(RunRecord &r, const ButtonSpec &b);
    void startBackground(RunRecord &r, const ButtonSpec &b);
    void finish(quint64 id);
    void trimHistory();
    QProcessEnvironment environment(const ButtonSpec &b) const;

    SecretStore *m_secrets;
    QString m_terminal = QStringLiteral("auto");
    quint64 m_nextId = 1;
    QHash<quint64, RunRecord> m_records;
    QList<quint64> m_order;
    QHash<quint64, QPointer<QProcess>> m_processes;
};

#endif // COMMANDRUNNER_H
