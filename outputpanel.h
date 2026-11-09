// outputpanel.h — View → Output panel (F9): every run this session.
//
// Left: the list of runs (● running, ✓ worked, ✗ failed, ▣ terminal,
// ↗ background). Right: what the selected run printed — or, for a
// Background run, the end of its log — with secrets blanked out.
#ifndef OUTPUTPANEL_H
#define OUTPUTPANEL_H

#include <QDockWidget>
#include <QTimer>

class CommandRunner;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class SecretStore;

class OutputPanel : public QDockWidget
{
    Q_OBJECT
public:
    OutputPanel(CommandRunner *runner, SecretStore *secrets, QWidget *parent = nullptr);

    void showRun(quint64 id);          // select it (and show the panel)
    void showLogFor(const QString &buttonId, const QString &label);

signals:
    void helpRequested(const QString &topic);

private:
    void rebuildList();
    void updateItem(QListWidgetItem *item, quint64 id);
    QListWidgetItem *itemFor(quint64 id) const;
    quint64 currentId() const;
    void refreshView();
    void refreshButtons();
    QString readLogTail(const QString &path) const;

    CommandRunner *m_runner;
    SecretStore *m_secrets;
    QListWidget *m_list;
    QLabel *m_header;
    QPlainTextEdit *m_view;
    QPushButton *m_stop;
    QPushButton *m_copy;
    QPushButton *m_openLog;
    QPushButton *m_clear;
    QTimer m_refresh;
    QString m_logPath;                 // shown instead of a run, when set
    QString m_logLabel;
};

#endif // OUTPUTPANEL_H
