// commandlinter.h — the Command checker in the Edit dialog.
//
// Looks at a command before it's saved and points out things that won't
// work the way they look: bash syntax errors, a password prompt in a mode
// with no terminal, a program that waits for typing, lines that will run on
// the wrong computer after an ssh session, leftover placeholders, secrets
// typed into the command, and variables that will be empty.
//
// It never changes the command; it only explains.
#ifndef COMMANDLINTER_H
#define COMMANDLINTER_H

#include "buttonspec.h"

#include <QList>
#include <QString>

class SecretStore;

struct LintIssue {
    enum Level { Info, Warning, Error };
    Level level = Info;
    QString code;      // stable id: "syntax", "sudo", "secret", …
    QString text;
};

namespace CommandLinter {

QList<LintIssue> check(const QString &command, RunMode mode, const SecretStore &secrets);
QList<LintIssue> checkStatus(const QString &statusCheck, const SecretStore &secrets);

// bash -n: "" when the syntax is fine, otherwise bash's message.
QString syntaxError(const QString &command);

} // namespace CommandLinter

#endif // COMMANDLINTER_H
