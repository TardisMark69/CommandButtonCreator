// commandlinter.cpp — see commandlinter.h.
#include "commandlinter.h"

#include "secretscanner.h"
#include "secretstore.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

// Rough split of a command into simple commands: by line, then by ; && || |.
// Good enough for checking the FIRST word of each piece; quoting inside a
// piece doesn't matter for that.
struct Piece {
    int line;            // 1-based
    QString text;
    bool background;     // ended with a single &
};

QList<Piece> pieces(const QString &command)
{
    QList<Piece> out;
    const QStringList lines = command.split(QLatin1Char('\n'));
    static const QRegularExpression sep(QStringLiteral(R"(\s*(?:;|&&|\|\||\|)\s*)"));
    for (int n = 0; n < lines.size(); ++n) {
        QString line = lines.at(n).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const QStringList parts = line.split(sep, Qt::SkipEmptyParts);
        for (int p = 0; p < parts.size(); ++p) {
            QString t = parts.at(p).trimmed();
            bool bg = false;
            if (t.endsWith(QLatin1Char('&')) && !t.endsWith(QStringLiteral("&&"))) {
                bg = true;
                t.chop(1);
                t = t.trimmed();
            }
            if (!t.isEmpty())
                out.append({n + 1, t, bg});
        }
    }
    return out;
}

QStringList words(const QString &piece)
{
    QString s = piece;
    s.remove(QLatin1Char('"'));
    s.remove(QLatin1Char('\''));
    return s.split(QRegularExpression(QStringLiteral(R"(\s+)")), Qt::SkipEmptyParts);
}

// Skip leading VAR=value assignments, "sudo", "exec", "nohup", "time".
int firstCommandWord(const QStringList &w, bool *viaSudo)
{
    static const QRegularExpression assign(QStringLiteral(R"(^[A-Za-z_][A-Za-z0-9_]*=)"));
    *viaSudo = false;
    int i = 0;
    while (i < w.size()) {
        const QString &t = w.at(i);
        if (assign.match(t).hasMatch() || t == QLatin1String("exec") || t == QLatin1String("nohup")
            || t == QLatin1String("time") || t == QLatin1String("command")) {
            ++i;
        } else if (t == QLatin1String("sudo")) {
            *viaSudo = true;
            ++i;
            while (i < w.size() && w.at(i).startsWith(QLatin1Char('-')))
                ++i;
        } else {
            break;
        }
    }
    return i;
}

// ssh with no remote command opens an interactive login.
bool isInteractiveSsh(const QStringList &w, int at)
{
    if (at >= w.size() || w.at(at) != QLatin1String("ssh"))
        return false;
    static const QSet<QString> withArg{
        QStringLiteral("-b"), QStringLiteral("-c"), QStringLiteral("-D"), QStringLiteral("-E"),
        QStringLiteral("-e"), QStringLiteral("-F"), QStringLiteral("-I"), QStringLiteral("-i"),
        QStringLiteral("-J"), QStringLiteral("-L"), QStringLiteral("-l"), QStringLiteral("-m"),
        QStringLiteral("-O"), QStringLiteral("-o"), QStringLiteral("-p"), QStringLiteral("-Q"),
        QStringLiteral("-R"), QStringLiteral("-S"), QStringLiteral("-W"), QStringLiteral("-w"),
        QStringLiteral("-B")};
    int i = at + 1;
    while (i < w.size() && w.at(i).startsWith(QLatin1Char('-'))) {
        if (withArg.contains(w.at(i)))
            ++i;
        ++i;
    }
    // i is the host; anything after it is the remote command.
    return i < w.size() && i == w.size() - 1;
}

bool waitsForTyping(const QStringList &w, int at, QString *what)
{
    if (at >= w.size())
        return false;
    const QString cmd = w.at(at);
    const int args = int(w.size()) - at - 1;
    static const QSet<QString> alwaysInteractive{
        QStringLiteral("nano"), QStringLiteral("vim"), QStringLiteral("vi"),
        QStringLiteral("nvim"), QStringLiteral("emacs"), QStringLiteral("htop"),
        QStringLiteral("top"), QStringLiteral("btop"), QStringLiteral("less"),
        QStringLiteral("more"), QStringLiteral("man"), QStringLiteral("mc"),
        QStringLiteral("read"), QStringLiteral("passwd"), QStringLiteral("su")};
    static const QSet<QString> interactiveWithoutArgs{
        QStringLiteral("python"), QStringLiteral("python3"), QStringLiteral("ipython"),
        QStringLiteral("node"), QStringLiteral("bash"), QStringLiteral("sh"),
        QStringLiteral("mysql"), QStringLiteral("psql"), QStringLiteral("sqlite3")};
    if (alwaysInteractive.contains(cmd)
        || (interactiveWithoutArgs.contains(cmd) && args == 0)) {
        *what = cmd + QStringLiteral(", which waits for you to type");
        return true;
    }
    if (cmd == QLatin1String("ollama") && args == 2 && w.at(at + 1) == QLatin1String("run")) {
        *what = QStringLiteral("ollama run (a chat that waits for you to type)");
        return true;
    }
    if (isInteractiveSsh(w, at)) {
        *what = QStringLiteral("ssh with no command (an interactive login)");
        return true;
    }
    return false;
}

const QSet<QString> &shellVariables()
{
    static const QSet<QString> v{
        QStringLiteral("RANDOM"), QStringLiteral("SECONDS"), QStringLiteral("LINENO"),
        QStringLiteral("HOSTNAME"), QStringLiteral("UID"), QStringLiteral("EUID"),
        QStringLiteral("PPID"), QStringLiteral("BASHPID"), QStringLiteral("REPLY"),
        QStringLiteral("OPTARG"), QStringLiteral("OPTIND"), QStringLiteral("PIPESTATUS"),
        QStringLiteral("BASH_SOURCE"), QStringLiteral("FUNCNAME"), QStringLiteral("HOSTTYPE"),
        QStringLiteral("OSTYPE"), QStringLiteral("MACHTYPE"), QStringLiteral("SHLVL"),
        QStringLiteral("PWD"), QStringLiteral("OLDPWD"), QStringLiteral("IFS"),
        QStringLiteral("CBC_LABEL"), QStringLiteral("CBC_LOG"), QStringLiteral("BASH_VERSION")};
    return v;
}

bool assignedInCommand(const QString &command, const QString &name)
{
    const QString n = QRegularExpression::escape(name);
    const QRegularExpression re(
        QStringLiteral(R"((?:^|[\s;&|(])(?:export\s+|local\s+|declare\s+(?:-\w+\s+)*|readonly\s+)?%1=|\bfor\s+%1\s+in\b|\bread\b[^\n;]*\b%1\b)")
            .arg(n));
    return re.match(command).hasMatch();
}

void add(QList<LintIssue> &out, LintIssue::Level level, const QString &code, const QString &text)
{
    for (const LintIssue &i : out)
        if (i.code == code && i.text == text)
            return;
    out.append({level, code, text});
}

void commonChecks(const QString &command, const SecretStore &secrets, QList<LintIssue> &out)
{
    // Syntax
    const QString syntax = CommandLinter::syntaxError(command);
    if (!syntax.isEmpty())
        add(out, LintIssue::Error, QStringLiteral("syntax"), syntax);

    // Secrets typed into the command
    const QList<SecretScanner::Finding> found = SecretScanner::scan(command);
    for (const SecretScanner::Finding &f : found)
        add(out, LintIssue::Warning, QStringLiteral("secret"),
            QStringLiteral("A %1 is typed into the command (%2…). Anyone who sees the "
                           "buttons file or the screen can copy it. Press “Move to vault”.")
                .arg(f.kind.toLower(), f.value.left(6)));

    // Secrets inside single quotes are never filled in
    static const QRegularExpression ref(
        QStringLiteral(R"(\$\{?([A-Za-z_][A-Za-z0-9_]*))"));
    auto it = ref.globalMatch(command);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (secrets.contains(m.captured(1))
            && SecretScanner::quoteStateAt(command, int(m.capturedStart()))
                   == SecretScanner::Quote::Single)
            add(out, LintIssue::Warning, QStringLiteral("singlequote"),
                QStringLiteral("$%1 is inside 'single quotes', where bash doesn't fill it in. "
                               "Write it as '\"${%1}\"' (close the quote, insert, reopen).")
                    .arg(m.captured(1)));
    }

    // Variables that will be empty
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &name : SecretStore::variableReferences(command)) {
        if (name != name.toUpper() || secrets.contains(name) || env.contains(name)
            || shellVariables().contains(name) || assignedInCommand(command, name))
            continue;
        add(out, LintIssue::Info, QStringLiteral("unknownvar"),
            QStringLiteral("$%1 isn't a saved secret or a variable on this computer, so it "
                           "will be empty. (Typo? Add it in Tools → Secrets vault.)")
                .arg(name));
    }

    // Placeholders left in
    static const QRegularExpression placeholder(
        QStringLiteral(R"(\b(your[_-]?(?:user(?:name)?|host|server|password|token|key|ip)|youruser|CHANGE_?ME|REPLACE_?ME|TODO|FIXME|xxx+)\b|<[A-Za-z][A-Za-z _-]*>)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch ph = placeholder.match(command);
    if (ph.hasMatch())
        add(out, LintIssue::Warning, QStringLiteral("placeholder"),
            QStringLiteral("“%1” looks like a placeholder that was never filled in.")
                .arg(ph.captured(0)));
}

} // namespace

namespace CommandLinter {

QString syntaxError(const QString &command)
{
    if (command.trimmed().isEmpty())
        return QString();
    QProcess bash;
    bash.setProcessChannelMode(QProcess::MergedChannels);
    bash.setStandardInputFile(QProcess::nullDevice());
    bash.start(QStringLiteral("/bin/bash"), {QStringLiteral("-n"), QStringLiteral("-c"), command});
    if (!bash.waitForStarted(2000))
        return QString();                 // can't check; don't claim an error
    if (!bash.waitForFinished(2000)) {
        bash.kill();
        bash.waitForFinished(500);
        return QString();
    }
    if (bash.exitStatus() == QProcess::NormalExit && bash.exitCode() == 0)
        return QString();
    QString msg = QString::fromLocal8Bit(bash.readAll()).trimmed();
    // "bash: -c: line 3: syntax error …"  ->  "Line 3: syntax error …"
    static const QRegularExpression prefix(QStringLiteral(R"(bash: -c: line (\d+): )"));
    const QStringList lines = msg.split(QLatin1Char('\n'));
    msg = lines.isEmpty() ? msg : lines.first();
    const QRegularExpressionMatch m = prefix.match(msg);
    if (m.hasMatch())
        msg = QStringLiteral("Line %1: %2").arg(m.captured(1), msg.mid(m.capturedEnd()));
    return QStringLiteral("Bash can't read this command — %1").arg(msg);
}

QList<LintIssue> check(const QString &command, RunMode mode, const SecretStore &secrets)
{
    QList<LintIssue> out;
    if (command.trimmed().isEmpty()) {
        add(out, LintIssue::Error, QStringLiteral("empty"),
            QStringLiteral("There's no command yet."));
        return out;
    }
    commonChecks(command, secrets, out);

    const QList<Piece> ps = pieces(command);
    int lastLine = 0;
    for (const Piece &p : ps)
        lastLine = qMax(lastLine, p.line);

    for (const Piece &p : ps) {
        const QStringList w = words(p.text);
        bool viaSudo = false;
        const int at = firstCommandWord(w, &viaSudo);

        if (viaSudo && mode != RunMode::Terminal)
            add(out, LintIssue::Warning, QStringLiteral("sudo"),
                QStringLiteral("sudo asks for your password, and %1 mode has no terminal to "
                               "type it in, so it will fail. Use Terminal window mode, or "
                               "write pkexec instead of sudo to get a password pop-up.")
                    .arg(RunModes::title(mode)));

        QString what;
        if (mode != RunMode::Terminal && waitsForTyping(w, at, &what))
            add(out, LintIssue::Warning, QStringLiteral("interactive"),
                QStringLiteral("Line %1 starts %2. %3 mode has nowhere to type, so use "
                               "Terminal window mode.")
                    .arg(p.line)
                    .arg(what, RunModes::title(mode)));

        if (isInteractiveSsh(w, at) && p.line < lastLine)
            add(out, LintIssue::Warning, QStringLiteral("sshafter"),
                QStringLiteral("Line %1 logs in with ssh. The lines after it do NOT run on the "
                               "other computer — they run here, after you log out. To run "
                               "something there, put it on the ssh line: "
                               "ssh -t host 'command'.")
                    .arg(p.line));

        if (p.background && mode == RunMode::Terminal)
            add(out, LintIssue::Info, QStringLiteral("ampersand"),
                QStringLiteral("Line %1 ends with &, so it runs in the background of the "
                               "terminal and is stopped when you close that terminal. To "
                               "keep it running, use Background mode (and drop the &).")
                    .arg(p.line));
    }

    static const QRegularExpression longRunning(
        QStringLiteral(R"(\b(?:serve|tail\s+-[a-zA-Z]*f|journalctl\b[^\n]*\s-[a-zA-Z]*f|watch)\b|\bping\b(?![^\n]*\s-c))"));
    if (mode == RunMode::Output && longRunning.match(command).hasMatch())
        add(out, LintIssue::Info, QStringLiteral("longrunning"),
            QStringLiteral("This looks like it keeps running until stopped. In Output panel "
                           "mode it runs until you press Stop; Background mode may suit it "
                           "better."));
    return out;
}

QList<LintIssue> checkStatus(const QString &statusCheck, const SecretStore &secrets)
{
    QList<LintIssue> out;
    if (statusCheck.trimmed().isEmpty())
        return out;
    commonChecks(statusCheck, secrets, out);
    for (const Piece &p : pieces(statusCheck)) {
        const QStringList w = words(p.text);
        bool viaSudo = false;
        const int at = firstCommandWord(w, &viaSudo);
        QString what;
        if (viaSudo)
            add(out, LintIssue::Warning, QStringLiteral("sudo"),
                QStringLiteral("Status checks run without a terminal, so sudo can't ask for "
                               "a password and the light will always be red."));
        if (waitsForTyping(w, at, &what))
            add(out, LintIssue::Warning, QStringLiteral("interactive"),
                QStringLiteral("The status check starts %1; it will time out after a few "
                               "seconds and show red.")
                    .arg(what));
    }
    for (LintIssue &i : out)
        i.text.prepend(QStringLiteral("Status check: "));
    return out;
}

} // namespace CommandLinter
