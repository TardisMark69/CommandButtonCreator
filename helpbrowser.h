// helpbrowser.h — the Help window (F1), in the same style as TardisChat's:
// Contents tree, Index (type to narrow, Enter to open), Search (every
// topic containing all the words typed), Back / Forward / Contents, and
// A− / A+ for the text size. One window is shared; opening a topic again
// just brings it forward.
#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QPushButton;
class QTextBrowser;
class QTreeWidget;

class HelpBrowser : public QDialog
{
    Q_OBJECT
public:
    // Open (or bring forward) the help at a topic.
    static void showTopic(QWidget *parent, const QString &topicId = QStringLiteral("welcome"));

    void openTopic(const QString &id, bool record = true);

protected:
    void done(int result) override;

private:
    explicit HelpBrowser(QWidget *parent);

    void buildContents();
    void buildIndex();
    void filterIndex(const QString &text);
    void runSearch(const QString &text);
    void updateNav();
    void zoom(int step);

    QTreeWidget *m_contents;
    QLineEdit *m_indexFilter;
    QListWidget *m_index;
    QLineEdit *m_searchBox;
    QListWidget *m_results;
    QTextBrowser *m_view;
    QPushButton *m_back;
    QPushButton *m_forward;
    QStringList m_history;
    int m_pos = -1;
    int m_zoom = 0;
};

#endif // HELPBROWSER_H
