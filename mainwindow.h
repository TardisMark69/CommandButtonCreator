// mainwindow.h — the window: menus, toolbar, page tabs, status bar, and the
// glue between the parts.
//
//   ButtonStore    buttons, pages, saving, undo, backups
//   SecretStore    the secrets vault
//   CommandRunner  runs commands, keeps the run history
//   StatusMonitor  the status lights
//   PageCanvas     one per page; selection and arranging
//   OutputPanel    the run history and output
//
// The window never edits button data itself: every change goes through the
// store (one undo step each), and the store's changed() signal redraws.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "buttonspec.h"

#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPointer>

class ButtonStore;
class CommandButton;
class CommandRunner;
class OutputPanel;
class PageCanvas;
class QAction;
class QLabel;
class QMenu;
class QPushButton;
class QScrollArea;
class QTabWidget;
class SecretStore;
class StatusMonitor;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Called once after show(): loads files and reports anything notable.
    void start();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e) override;

private:
    // ---- building
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createPageArea();

    // ---- syncing the view with the store
    void syncPages();
    PageCanvas *addCanvas();
    PageCanvas *currentCanvas() const;
    int currentPage() const;
    QScrollArea *scrollFor(PageCanvas *c) const;
    void initButton(CommandButton *b);
    CommandButton *buttonWidget(const QString &id) const;
    void updateActions();
    void updateStatusBar();
    void updateSecretWarning();

    // ---- running
    void runButton(const QString &id);
    void runSpec(const ButtonSpec &spec, bool test);
    void onRunStarted(quint64 runId);
    void onRunFinished(quint64 runId);

    // ---- buttons
    void addButton(const QPoint &at = QPoint(-1, -1));
    void editButton(const QString &id);
    void editSelected();
    void duplicateSelected();
    void deleteButtons(const QStringList &ids);
    void setModeFor(const QStringList &ids, RunMode mode);
    void setColorFor(const QStringList &ids, const QString &color);
    void moveToPage(const QStringList &ids, int page);
    void showButtonMenu(const QString &id, const QPoint &globalPos);
    void showCanvasMenu(const QPoint &globalPos, const QPoint &canvasPos);
    QMenu *pageMenuFor(const QStringList &ids, QWidget *parent);

    // ---- arranging
    void organize();
    void makeSameSize();
    void setLocked(bool locked);

    // ---- pages
    void newPage();
    void renamePage(int index);
    void deletePage(int index);
    void showTabMenu(const QPoint &pos);

    // ---- tools
    void openSecrets();
    void securityCheck(bool quietIfClean);
    void scrubOldCopies();
    void openPreferences();
    void applyPreferences();
    void exportButtons();
    void importButtons();
    void restoreBackup();
    void createLauncher();
    void setZoom(int pt);
    void showHelp(const QString &topic);
    void showAbout();
    // A read-only text window (test results, import preview). Returns true
    // when the accept button was pressed (if one was given).
    bool showTextDialog(QWidget *parent, const QString &title, const QString &header,
                        const QString &text, const QString &acceptText = QString());

    ButtonStore *m_store;
    SecretStore *m_secrets;
    CommandRunner *m_runner;
    StatusMonitor *m_status;
    OutputPanel *m_output;

    QTabWidget *m_tabs;
    QList<PageCanvas *> m_canvases;
    bool m_syncing = false;

    QLabel *m_countLabel;
    QLabel *m_runningLabel;
    QPushButton *m_secretWarning;

    QHash<QString, qint64> m_lastClick;     // double-click guard
    QPointer<QWidget> m_editDialog;          // open Edit dialog (for Test run results)
    int m_zoomPt;

    QAction *m_actNew, *m_actNewPage, *m_actImport, *m_actExport, *m_actRestore,
        *m_actLauncher, *m_actQuit;
    QAction *m_actUndo, *m_actRedo, *m_actEdit, *m_actDuplicate, *m_actDelete,
        *m_actSelectAll;
    QAction *m_actLock, *m_actOrganize, *m_actSameSize;
    QAction *m_actOutput, *m_actZoomIn, *m_actZoomOut, *m_actZoomReset, *m_actNextPage,
        *m_actPrevPage;
    QAction *m_actSecrets, *m_actSecurity, *m_actRefresh, *m_actPrefs;
    QAction *m_actHelp, *m_actWhatsNew, *m_actAbout;
};

#endif // MAINWINDOW_H
