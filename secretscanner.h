// secretscanner.h — finds passwords and tokens typed straight into a
// command, and rewrites the command to use a vault secret instead.
//
// The rewrite is quote-aware, so the result still means the same thing to
// bash:
//   inside "double quotes"   tok          ->  ${NAME}
//   inside 'single quotes'   tok          ->  '"${NAME}"'   (close, insert, reopen)
//   unquoted                 tok          ->  "${NAME}"
#ifndef SECRETSCANNER_H
#define SECRETSCANNER_H

#include <QList>
#include <QString>

namespace SecretScanner {

struct Finding {
    QString value;           // the secret text itself
    QString kind;            // "Bearer token", "Password", …
    QString suggestedName;   // HA_TOKEN, PASSWORD, …
};

// Unique values found in one command (empty if none).
QList<Finding> scan(const QString &command);

enum class Quote { None, Single, Double };
// The bash quoting in effect at position pos.
Quote quoteStateAt(const QString &text, int pos);

// Replace every copy of value with a reference to secret `name`.
QString replaceValue(const QString &command, const QString &value, const QString &name);

} // namespace SecretScanner

#endif // SECRETSCANNER_H
