// helpcontent.h — the text of the help, one entry per topic.
//
// Topics link to each other with <a href="topic:ID">. `parent` places a
// topic under another in the Contents tree ("" = top level); `keywords`
// feed the Index tab.
#ifndef HELPCONTENT_H
#define HELPCONTENT_H

#include <QList>
#include <QString>
#include <QStringList>

struct HelpTopic {
    QString id;
    QString parent;
    QString title;
    QStringList keywords;
    QString html;
};

namespace HelpContent {
const QList<HelpTopic> &topics();
const HelpTopic *find(const QString &id);
} // namespace HelpContent

#endif // HELPCONTENT_H
