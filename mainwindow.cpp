// mainwindow.cpp — see mainwindow.h.
#include "mainwindow.h"

#include "apppaths.h"
#include "appsettings.h"
#include "buttonstore.h"
#include "commandbutton.h"
#include "commandrunner.h"
#include "editdialog.h"
#include "helpbrowser.h"
#include "outputpanel.h"
#include "pagecanvas.h"
#include "preferencesdialog.h"
#include "secretscanner.h"
#include "secretsdialog.h"
#include "secretstore.h"
#include "statusmonitor.h"
#include "theme.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kDoubleClickGuardMs = 700;

QIcon swatchIcon(const Swatch &s)
{
    QPixmap pm(16, 12);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(s.mid);
    p.setPen(QPen(s.border, 1.5));
    p.drawRoundedRect(QRectF(1, 1, 14, 10), 3, 3);
    return QIcon(pm);
}

QString preview(const QString &command, int lines = 6)
{
    QStringList l = command.split(QLatin1Char('\n'));
    const bool more = l.size() > lines;
    l = l.mid(0, lines);
    QString out = l.join(QLatin1Char('\n'));
    if (out.size() > 600)
        out = out.left(600) + QStringLiteral("…");
    else if (more)
        out += QStringLiteral("\n…");
    return out;
}

// Replace secret values inside every string of a JSON tree.
QJsonValue scrubJson(const QJsonValue &v, const QMap<QString, QString> &valueToName, bool *changed)
{
    if (v.isString()) {
        QString s = v.toString();
        for (auto it = valueToName.cbegin(); it != valueToName.cend(); ++it)
            if (s.contains(it.key())) {
                s = SecretScanner::replaceValue(s, it.key(), it.value());
                *changed = true;
            }
        return s;
    }
    if (v.isArray()) {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i)
            a[i] = scrubJson(a.at(i), valueToName, changed);
        return a;
    }
    if (v.isObject()) {
        QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it)
            it.value() = scrubJson(it.value(), valueToName, changed);
        return o;
    }
    return v;
}

bool jsonContainsAny(const QJsonValue &v, const QStringList &needles)
{
    if (v.isString()) {
        for (const QString &n : needles)
            if (v.toString().contains(n))
                return true;
        return false;
    }
    if (v.isArray()) {
        for (const QJsonValue &x : v.toArray())
            if (jsonContainsAny(x, needles))
                return true;
        return false;
    }
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        for (auto it = o.begin(); it != o.end(); ++it)
            if (jsonContainsAny(it.value(), needles))
                return true;
    }
    return false;
}

QString desktopExecQuote(const QString &path)
{
    // Desktop Entry spec: quote, escape " ` $ \ inside, then escape \ again
    // for the string value itself.
    QString q;
    for (const QChar c : path) {
        if (c == QLatin1Char('"') || c == QLatin1Char('`') || c == QLatin1Char('$')
            || c == QLatin1Char('\\'))
            q += QLatin1Char('\\');
        q += c;
    }
    q = QLatin1Char('"') + q + QLatin1Char('"');
    q.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    return q;
}


// Menu item that runs a function. (Works the same on every Qt 6 version,
// unlike the addAction(text, receiver, functor) overloads.)
template <typename F>
QAction *addItem(QMenu *menu, const QString &text, F fn)
{
    QAction *a = menu->addAction(text);
    QObject::connect(a, &QAction::triggered, menu, fn);
    return a;
}

template <typename F>
QAction *addItem(QMenu *menu, const QIcon &icon, const QString &text, F fn)
{
    QAction *a = menu->addAction(icon, text);
    QObject::connect(a, &QAction::triggered, menu, fn);
    return a;
}
} // namespace

// ================================================================ set-up
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_store(new ButtonStore(this))
    , m_secrets(new SecretStore(this))
    , m_runner(new CommandRunner(m_secrets, this))
    , m_status(new StatusMonitor(m_secrets, this))
    , m_zoomPt(AppSettings::zoomPt())
{
    setWindowTitle(QStringLiteral("Command Button Creator"));
    setWindowIcon(Theme::appIcon());
    m_runner->setTerminal(AppSettings::terminal());
    m_status->setIntervalSec(AppSettings::statusIntervalSec());

    createActions();
    createMenus();
    createToolBar();
    createPageArea();
    createStatusBar();

    m_output = new OutputPanel(m_runner, m_secrets, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_output);
    m_output->hide();
    m_actOutput = m_output->toggleViewAction();
    m_actOutput->setText(QStringLiteral("&Output panel"));
    m_actOutput->setShortcut(QKeySequence(Qt::Key_F9));
    m_actOutput->setToolTip(QStringLiteral("Show or hide the Output panel (F9): every run and "
                                           "what it printed."));
    menuBar()->actions().at(3)->menu()->insertAction(m_actZoomIn, m_actOutput);
    menuBar()->actions().at(3)->menu()->insertSeparator(m_actZoomIn);
    findChild<QToolBar *>(QStringLiteral("mainToolBar"))
        ->insertAction(m_actSecrets, m_actOutput);
    connect(m_output, &OutputPanel::helpRequested, this, &MainWindow::showHelp);

    connect(m_store, &ButtonStore::changed, this, &MainWindow::syncPages);
    connect(m_store, &ButtonStore::undoStateChanged, this, &MainWindow::updateActions);
    connect(m_store, &ButtonStore::saveFailed, this, [this](const QString &msg) {
        statusBar()->showMessage(QStringLiteral("⚠ Couldn't save: %1").arg(msg));
        QMessageBox::warning(this, windowTitle(), msg);
    });
    connect(m_runner, &CommandRunner::runStarted, this, &MainWindow::onRunStarted);
    connect(m_runner, &CommandRunner::runFinished, this, &MainWindow::onRunFinished);
    connect(m_runner, &CommandRunner::historyChanged, this, &MainWindow::updateStatusBar);
    connect(m_status, &StatusMonitor::statusChanged, this, [this](const QString &id) {
        if (CommandButton *b = buttonWidget(id))
            b->setStatus(m_status->info(id));
    });

    const QByteArray geo = AppSettings::windowGeometry();
    if (geo.isEmpty() || !restoreGeometry(geo))
        resize(940, 520);
    if (!restoreState(AppSettings::windowState()))
        resizeDocks({m_output}, {height() / 3}, Qt::Vertical);
    m_actLock->setChecked(AppSettings::locked());
}

MainWindow::~MainWindow() = default;

void MainWindow::start()
{
    QString err;
    if (!m_secrets->load(&err))
        QMessageBox::warning(this, windowTitle(),
                             QStringLiteral("%1\n\nButtons that use secrets will get empty "
                                            "values until this is fixed.")
                                 .arg(err));

    const ButtonStore::LoadResult r = m_store->load();
    if (!r.ok)
        QMessageBox::warning(this, QStringLiteral("Buttons file couldn't be read"), r.message);
    else if (r.migrated)
        QMessageBox::information(this, QStringLiteral("Welcome to version 2.0"), r.message);
    if (!r.warnings.isEmpty())
        statusBar()->showMessage(QStringLiteral("%1 small problem(s) in the buttons file were "
                                                "corrected.")
                                     .arg(r.warnings.size()),
                                 8000);

    m_tabs->setCurrentIndex(qBound(0, AppSettings::currentPage(), m_tabs->count() - 1));
    setLocked(AppSettings::locked());
    m_status->refreshAll();

    for (const QString &f : {AppPaths::buttonsFile(), AppPaths::secretsFile()})
        if (AppPaths::isExposed(f))
            AppPaths::makePrivateFile(f);
    if (m_secrets->permissionsWereFixed())
        statusBar()->showMessage(QStringLiteral("The secrets file was readable by others; "
                                                "that has been corrected."),
                                 8000);

    if (AppSettings::whatsNewSeen() != QStringLiteral("2.0")) {
        AppSettings::setWhatsNewSeen(QStringLiteral("2.0"));
        showHelp(QStringLiteral("whatsnew"));
    }
}

void MainWindow::createActions()
{
    auto make = [this](const QString &text, const QKeySequence &key, const QString &tip,
                       auto slot) {
        auto *a = new QAction(text, this);
        if (!key.isEmpty())
            a->setShortcut(key);
        a->setToolTip(tip);
        a->setStatusTip(tip);
        connect(a, &QAction::triggered, this, slot);
        addAction(a);
        return a;
    };

    m_actNew = make(QStringLiteral("&New button…"), QKeySequence::New,
                    QStringLiteral("Make a new button on this page (Ctrl+N)."),
                    [this] { addButton(); });
    m_actNewPage = make(QStringLiteral("New &page…"), QKeySequence(Qt::CTRL | Qt::Key_T),
                        QStringLiteral("Add a page (tab) for another group of buttons."),
                        [this] { newPage(); });
    m_actImport = make(QStringLiteral("&Import buttons…"), {},
                       QStringLiteral("Add buttons from an exported file (shown to you first)."),
                       [this] { importButtons(); });
    m_actExport = make(QStringLiteral("&Export buttons…"), {},
                       QStringLiteral("Save this page or all pages to a file. Secrets are not "
                                      "included."),
                       [this] { exportButtons(); });
    m_actRestore = make(QStringLiteral("&Restore from backup…"), {},
                        QStringLiteral("Bring back an earlier copy of your buttons."),
                        [this] { restoreBackup(); });
    m_actLauncher = make(QStringLiteral("Create desktop &launcher"), {},
                         QStringLiteral("Add this program to your Applications, ready to pin "
                                        "to the dock."),
                         [this] { createLauncher(); });
    m_actQuit = make(QStringLiteral("&Quit"), QKeySequence::Quit, QStringLiteral("Close the program."),
                     [this] { close(); });

    m_actUndo = make(QStringLiteral("&Undo"), QKeySequence::Undo,
                     QStringLiteral("Take back the last change (Ctrl+Z)."),
                     [this] { m_store->undo(); });
    m_actRedo = make(QStringLiteral("&Redo"), {}, QStringLiteral("Put back what Undo took back."),
                     [this] { m_store->redo(); });
    m_actRedo->setShortcuts({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z),
                             QKeySequence(Qt::CTRL | Qt::Key_Y)});
    m_actEdit = make(QStringLiteral("&Edit button…"), QKeySequence(Qt::Key_F2),
                     QStringLiteral("Change the selected button (F2)."), [this] { editSelected(); });
    m_actDuplicate = make(QStringLiteral("&Duplicate"), QKeySequence(Qt::CTRL | Qt::Key_D),
                          QStringLiteral("Copy the selected buttons (Ctrl+D)."),
                          [this] { duplicateSelected(); });
    m_actDelete = make(QStringLiteral("De&lete"), {},
                       QStringLiteral("Delete the selected buttons (Delete key). Undo brings them "
                                      "back."),
                       [this] {
                           if (PageCanvas *c = currentCanvas())
                               deleteButtons(c->selectedIds());
                       });
    m_actDelete->setShortcut(QKeySequence::Delete);
    m_actDelete->setShortcutContext(Qt::WidgetShortcut);   // the canvas handles the key
    m_actSelectAll = make(QStringLiteral("Select &all"), QKeySequence::SelectAll,
                          QStringLiteral("Select every button on this page (Ctrl+A)."), [this] {
                              if (PageCanvas *c = currentCanvas())
                                  c->selectAll();
                          });

    m_actLock = make(QStringLiteral("🔒 Lock"), QKeySequence(Qt::CTRL | Qt::Key_L),
                     QStringLiteral("Lock the layout so buttons can't be moved or resized by "
                                    "accident (Ctrl+L). Green = locked."),
                     [this](bool on) { setLocked(on); });
    m_actLock->setCheckable(true);
    m_actOrganize = make(QStringLiteral("Organize"), QKeySequence(Qt::CTRL | Qt::Key_G),
                         QStringLiteral("Line the buttons up in neat rows (Ctrl+G). With several "
                                        "selected, only those."),
                         [this] { organize(); });
    m_actSameSize = make(QStringLiteral("Make All Same Size"), QKeySequence(Qt::CTRL | Qt::Key_M),
                         QStringLiteral("Resize every button (or the other selected ones) to match "
                                        "the button selected last (Ctrl+M)."),
                         [this] { makeSameSize(); });

    m_actZoomIn = make(QStringLiteral("Zoom &in"), QKeySequence::ZoomIn,
                       QStringLiteral("Larger text everywhere (Ctrl+= or Ctrl+wheel)."),
                       [this] { setZoom(m_zoomPt + 1); });
    m_actZoomIn->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Equal), QKeySequence::ZoomIn});
    m_actZoomOut = make(QStringLiteral("Zoom &out"), QKeySequence::ZoomOut,
                        QStringLiteral("Smaller text everywhere (Ctrl+− or Ctrl+wheel)."),
                        [this] { setZoom(m_zoomPt - 1); });
    m_actZoomReset = make(QStringLiteral("&Normal size"), QKeySequence(Qt::CTRL | Qt::Key_0),
                          QStringLiteral("Back to the normal text size (Ctrl+0)."),
                          [this] { setZoom(AppSettings::DefaultZoomPt); });
    m_actNextPage = make(QStringLiteral("Ne&xt page"), QKeySequence(Qt::CTRL | Qt::Key_PageDown),
                         QStringLiteral("Show the next page."), [this] {
                             m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count());
                         });
    m_actPrevPage = make(QStringLiteral("Pre&vious page"), QKeySequence(Qt::CTRL | Qt::Key_PageUp),
                         QStringLiteral("Show the previous page."), [this] {
                             m_tabs->setCurrentIndex((m_tabs->currentIndex() + m_tabs->count() - 1)
                                                     % m_tabs->count());
                         });

    m_actSecrets = make(QStringLiteral("🔑 Secrets"), QKeySequence(Qt::CTRL | Qt::Key_K),
                        QStringLiteral("The secrets vault: passwords and tokens kept out of "
                                       "commands (Ctrl+K)."),
                        [this] { openSecrets(); });
    m_actSecurity = make(QStringLiteral("Security &check…"), {},
                         QStringLiteral("Find passwords or tokens typed into commands and move "
                                        "them into the vault."),
                         [this] { securityCheck(false); });
    m_actRefresh = make(QStringLiteral("&Refresh status lights"), QKeySequence(Qt::Key_F5),
                        QStringLiteral("Check every status light now (F5)."),
                        [this] {
                            m_status->refreshAll();
                            statusBar()->showMessage(QStringLiteral("Checking status lights…"), 3000);
                        });
    m_actPrefs = make(QStringLiteral("&Preferences…"), QKeySequence(Qt::CTRL | Qt::Key_Comma),
                      QStringLiteral("Terminal program, status-light timing and more."),
                      [this] { openPreferences(); });

    m_actHelp = make(QStringLiteral("Help"), QKeySequence::HelpContents,
                     QStringLiteral("Open the help (F1)."),
                     [this] { showHelp(QStringLiteral("welcome")); });
    m_actWhatsNew = make(QStringLiteral("What's &new in 2.0"), {},
                         QStringLiteral("The changes in this version."),
                         [this] { showHelp(QStringLiteral("whatsnew")); });
    m_actAbout = make(QStringLiteral("&About"), {}, QStringLiteral("About Command Button Creator."),
                      [this] { showAbout(); });
}

void MainWindow::createMenus()
{
    QMenu *file = menuBar()->addMenu(QStringLiteral("&File"));
    file->addAction(m_actNew);
    file->addAction(m_actNewPage);
    file->addSeparator();
    file->addAction(m_actImport);
    file->addAction(m_actExport);
    file->addAction(m_actRestore);
    file->addSeparator();
    file->addAction(m_actLauncher);
    file->addSeparator();
    file->addAction(m_actQuit);

    QMenu *edit = menuBar()->addMenu(QStringLiteral("&Edit"));
    edit->addAction(m_actUndo);
    edit->addAction(m_actRedo);
    edit->addSeparator();
    edit->addAction(m_actEdit);
    edit->addAction(m_actDuplicate);
    edit->addAction(m_actDelete);
    edit->addSeparator();
    edit->addAction(m_actSelectAll);

    QMenu *arrange = menuBar()->addMenu(QStringLiteral("&Arrange"));
    arrange->addAction(m_actLock);
    arrange->addSeparator();
    arrange->addAction(m_actOrganize);
    arrange->addAction(m_actSameSize);

    QMenu *view = menuBar()->addMenu(QStringLiteral("&View"));
    view->addAction(m_actZoomIn);
    view->addAction(m_actZoomOut);
    view->addAction(m_actZoomReset);
    view->addSeparator();
    view->addAction(m_actNextPage);
    view->addAction(m_actPrevPage);

    QMenu *tools = menuBar()->addMenu(QStringLiteral("&Tools"));
    tools->addAction(m_actSecrets);
    tools->addAction(m_actSecurity);
    tools->addSeparator();
    tools->addAction(m_actRefresh);
    tools->addSeparator();
    tools->addAction(m_actPrefs);

    QMenu *help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(m_actHelp);
    help->addAction(m_actWhatsNew);
    help->addSeparator();
    help->addAction(m_actAbout);

    for (QMenu *m : {file, edit, arrange, view, tools, help})
        m->setToolTipsVisible(true);
}

void MainWindow::createToolBar()
{
    QToolBar *tb = addToolBar(QStringLiteral("Main Toolbar"));
    tb->setObjectName(QStringLiteral("mainToolBar"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto *add = new QAction(QStringLiteral("Add Button"), this);
    add->setToolTip(m_actNew->toolTip());
    connect(add, &QAction::triggered, this, [this] { addButton(); });
    tb->addAction(add);
    tb->addAction(m_actOrganize);
    tb->addAction(m_actSameSize);
    tb->addAction(m_actLock);
    tb->addSeparator();
    tb->addAction(m_actSecrets);
    tb->addSeparator();
    // The menu's Undo says what it will undo; the toolbar's stays "Undo" so
    // the toolbar doesn't change width.
    auto *undo = new QAction(QStringLiteral("Undo"), this);
    connect(undo, &QAction::triggered, m_actUndo, &QAction::trigger);
    connect(m_actUndo, &QAction::changed, undo, [undo, this] {
        undo->setEnabled(m_actUndo->isEnabled());
        undo->setToolTip(m_actUndo->isEnabled()
                             ? QStringLiteral("%1 (Ctrl+Z)").arg(QString(m_actUndo->text()).remove(QLatin1Char('&')))
                             : QStringLiteral("Nothing to undo"));
    });
    tb->addAction(undo);
    tb->addAction(m_actHelp);
}

void MainWindow::createPageArea()
{
    m_tabs = new QTabWidget;
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabs->tabBar()->setToolTip(QStringLiteral("Pages. Double-click to rename, drag to "
                                                "reorder, right-click for more."));

    auto *plus = new QToolButton;
    plus->setText(QStringLiteral("+"));
    plus->setToolTip(QStringLiteral("New page (Ctrl+T)"));
    connect(plus, &QToolButton::clicked, this, [this] { newPage(); });
    m_tabs->setCornerWidget(plus, Qt::TopRightCorner);

    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (!m_syncing && index >= 0)
            AppSettings::setCurrentPage(index);
        updateActions();
        updateStatusBar();
    });
    connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this](int from, int to) {
        if (m_syncing)
            return;
        m_canvases.move(from, to);
        m_store->movePage(from, to);
    });
    connect(m_tabs, &QTabWidget::tabBarDoubleClicked, this, [this](int index) {
        if (index >= 0)
            renamePage(index);
        else
            newPage();
    });
    connect(m_tabs->tabBar(), &QTabBar::customContextMenuRequested, this, &MainWindow::showTabMenu);

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->addWidget(m_tabs);
    setCentralWidget(central);
}

void MainWindow::createStatusBar()
{
    m_secretWarning = new QPushButton;
    m_secretWarning->setObjectName(QStringLiteral("linkButton"));
    m_secretWarning->setCursor(Qt::PointingHandCursor);
    m_secretWarning->setToolTip(QStringLiteral("Some commands have a password or token typed into "
                                               "them. Click to move them into the vault."));
    m_secretWarning->hide();
    connect(m_secretWarning, &QPushButton::clicked, this, [this] { securityCheck(false); });
    m_runningLabel = new QLabel;
    m_runningLabel->setToolTip(QStringLiteral("Output-panel commands still running (F9 to see "
                                              "them)."));
    m_countLabel = new QLabel;
    statusBar()->addPermanentWidget(m_secretWarning);
    statusBar()->addPermanentWidget(m_runningLabel);
    statusBar()->addPermanentWidget(m_countLabel);
}

// ================================================================ syncing
PageCanvas *MainWindow::addCanvas()
{
    auto *canvas = new PageCanvas;
    canvas->setLocked(m_actLock->isChecked());
    canvas->setSnap(AppSettings::snapToGrid());
    canvas->setLabelPointSize(m_zoomPt + 2);
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("canvasScroll"));
    scroll->setWidgetResizable(true);
    scroll->setWidget(canvas);

    connect(canvas, &PageCanvas::runRequested, this, &MainWindow::runButton);
    connect(canvas, &PageCanvas::buttonContextMenu, this, &MainWindow::showButtonMenu);
    connect(canvas, &PageCanvas::canvasContextMenu, this, &MainWindow::showCanvasMenu);
    connect(canvas, &PageCanvas::geometriesEdited, m_store, &ButtonStore::setGeometries);
    connect(canvas, &PageCanvas::deleteRequested, this, &MainWindow::deleteButtons);
    connect(canvas, &PageCanvas::selectionChanged, this, [this] {
        updateActions();
        updateStatusBar();
    });
    connect(canvas, &PageCanvas::zoomRequested, this,
            [this](int steps) { setZoom(m_zoomPt + steps); });
    connect(canvas, &PageCanvas::lockedDragAttempt, this, [this] {
        statusBar()->showMessage(QStringLiteral("The layout is locked — press 🔒 Lock (Ctrl+L) "
                                                "to move or resize buttons."),
                                 5000);
    });
    connect(canvas, &PageCanvas::buttonCreated, this, &MainWindow::initButton);

    m_canvases.append(canvas);
    m_tabs->addTab(scroll, QString());
    return canvas;
}

void MainWindow::syncPages()
{
    m_syncing = true;
    const QList<Page> &pages = m_store->pages();
    while (m_canvases.size() < pages.size())
        addCanvas();
    while (m_canvases.size() > pages.size()) {
        const int last = int(m_canvases.size()) - 1;
        QWidget *w = m_tabs->widget(last);
        m_tabs->removeTab(last);
        m_canvases.removeLast();
        w->deleteLater();
    }
    for (int i = 0; i < pages.size(); ++i) {
        m_tabs->setTabText(i, pages.at(i).name);
        m_tabs->setTabToolTip(i, QStringLiteral("%1 — %2 button(s)")
                                     .arg(pages.at(i).name)
                                     .arg(pages.at(i).buttons.size()));
        m_canvases.at(i)->setButtons(pages.at(i).buttons);
    }
    m_status->setButtons(m_store->allButtons());
    m_syncing = false;
    updateActions();
    updateStatusBar();
    updateSecretWarning();
}

PageCanvas *MainWindow::currentCanvas() const
{
    const int i = m_tabs->currentIndex();
    return (i >= 0 && i < m_canvases.size()) ? m_canvases.at(i) : nullptr;
}

int MainWindow::currentPage() const
{
    return qMax(0, m_tabs->currentIndex());
}

QScrollArea *MainWindow::scrollFor(PageCanvas *c) const
{
    const int i = int(m_canvases.indexOf(c));
    return i >= 0 ? qobject_cast<QScrollArea *>(m_tabs->widget(i)) : nullptr;
}

CommandButton *MainWindow::buttonWidget(const QString &id) const
{
    for (PageCanvas *c : m_canvases)
        if (CommandButton *b = c->buttonFor(id))
            return b;
    return nullptr;
}

void MainWindow::initButton(CommandButton *b)
{
    b->setStatus(m_status->info(b->id()));
    if (const RunRecord *r = m_runner->record(m_runner->lastRunFor(b->id()))) {
        if (r->running)
            b->setRunState(CommandButton::RunState::Running, QStringLiteral("running"));
        else if (!r->succeeded() && r->mode == RunMode::Output)
            b->setRunState(CommandButton::RunState::Failed,
                           QStringLiteral("%1 at %2").arg(r->resultText(),
                                                          r->finished.toString(QStringLiteral("HH:mm:ss"))));
    }
}

void MainWindow::updateActions()
{
    m_actUndo->setEnabled(m_store->canUndo());
    m_actRedo->setEnabled(m_store->canRedo());
    m_actUndo->setText(m_store->canUndo() ? QStringLiteral("&Undo %1").arg(m_store->undoText())
                                          : QStringLiteral("&Undo"));
    m_actRedo->setText(m_store->canRedo() ? QStringLiteral("&Redo %1").arg(m_store->redoText())
                                          : QStringLiteral("&Redo"));
    PageCanvas *c = currentCanvas();
    const bool sel = c && !c->selectedIds().isEmpty();
    const bool any = c && !c->buttons().isEmpty();
    m_actEdit->setEnabled(sel);
    m_actDuplicate->setEnabled(sel);
    m_actDelete->setEnabled(sel);
    m_actOrganize->setEnabled(any && !m_actLock->isChecked());
    m_actSameSize->setEnabled(any && !m_actLock->isChecked());
}

void MainWindow::updateStatusBar()
{
    const int total = m_store->totalButtons();
    const int pages = m_store->pageCount();
    QString text = QStringLiteral("%1 button%2 · %3 page%4")
                       .arg(total)
                       .arg(total == 1 ? QString() : QStringLiteral("s"))
                       .arg(pages)
                       .arg(pages == 1 ? QString() : QStringLiteral("s"));
    if (PageCanvas *c = currentCanvas())
        if (const int n = int(c->selectedIds().size()))
            text += QStringLiteral(" · %1 selected").arg(n);
    m_countLabel->setText(text);
    const int running = m_runner->runningCount();
    m_runningLabel->setText(running ? QStringLiteral("⏳ %1 running").arg(running) : QString());
    m_runningLabel->setVisible(running > 0);
}

void MainWindow::updateSecretWarning()
{
    int count = 0;
    for (const ButtonSpec &b : m_store->allButtons())
        if (!SecretScanner::scan(b.command).isEmpty() || !SecretScanner::scan(b.statusCheck).isEmpty())
            ++count;
    m_secretWarning->setText(QStringLiteral("⚠ %1 button%2 hold%3 a typed-in secret — fix")
                                 .arg(count)
                                 .arg(count == 1 ? QString() : QStringLiteral("s"))
                                 .arg(count == 1 ? QStringLiteral("s") : QString()));
    m_secretWarning->setVisible(count > 0);
}

// ================================================================ running
void MainWindow::runButton(const QString &id)
{
    const ButtonSpec *found = m_store->find(id);
    if (!found)
        return;
    const ButtonSpec spec = *found;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastClick.value(id, 0) < kDoubleClickGuardMs)
        return;
    m_lastClick.insert(id, now);

    if (spec.command.trimmed().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("No Command"),
                                 QStringLiteral("No command has been set for this button."));
        return;
    }
    if (spec.mode == RunMode::Output && m_runner->isRunning(id)) {
        QMessageBox q(QMessageBox::Question, windowTitle(),
                      QStringLiteral("“%1” is still running.").arg(spec.label),
                      QMessageBox::NoButton, this);
        QPushButton *stop = q.addButton(QStringLiteral("Stop it"), QMessageBox::DestructiveRole);
        QPushButton *again = q.addButton(QStringLiteral("Run another copy"), QMessageBox::AcceptRole);
        q.addButton(QStringLiteral("Leave it running"), QMessageBox::RejectRole);
        q.exec();
        if (q.clickedButton() == stop) {
            for (quint64 rid : m_runner->history())
                if (const RunRecord *r = m_runner->record(rid))
                    if (r->buttonId == id && r->running)
                        m_runner->stop(rid);
            return;
        }
        if (q.clickedButton() != again)
            return;
    }
    if (spec.confirm) {
        QMessageBox q(QMessageBox::Question, QStringLiteral("Run?"),
                      QStringLiteral("Run “%1”?").arg(spec.label), QMessageBox::Yes | QMessageBox::No,
                      this);
        q.setInformativeText(QStringLiteral("<pre>%1</pre>").arg(preview(spec.command).toHtmlEscaped()));
        q.setDefaultButton(QMessageBox::Yes);
        q.setEscapeButton(QMessageBox::No);
        if (q.exec() != QMessageBox::Yes)
            return;
        m_lastClick.insert(id, QDateTime::currentMSecsSinceEpoch());
    }
    runSpec(spec, false);
}

void MainWindow::runSpec(const ButtonSpec &spec, bool test)
{
    m_runner->setTerminal(AppSettings::terminal());
    m_runner->run(spec, test);
}

void MainWindow::onRunStarted(quint64 runId)
{
    const RunRecord *r = m_runner->record(runId);
    if (!r)
        return;
    if (r->mode == RunMode::Output && r->running) {
        if (!r->test)
            if (CommandButton *b = buttonWidget(r->buttonId))
                b->setRunState(CommandButton::RunState::Running, QStringLiteral("running…"));
        if (AppSettings::autoShowOutput() || r->test)
            m_output->showRun(runId);
        statusBar()->showMessage(QStringLiteral("Running “%1”…").arg(r->label), 4000);
    }
    updateStatusBar();
}

void MainWindow::onRunFinished(quint64 runId)
{
    const RunRecord *r = m_runner->record(runId);
    if (!r)
        return;
    const QString when = r->finished.toString(QStringLiteral("HH:mm:ss"));
    QString detail;
    bool ok = r->succeeded();
    if (!r->error.isEmpty()) {
        detail = QStringLiteral("didn't start at %1").arg(when);
        ok = false;
    } else if (r->mode == RunMode::Output) {
        detail = QStringLiteral("%1 at %2 (%3 s)")
                     .arg(r->resultText(), when)
                     .arg(r->durationMs() / 1000.0, 0, 'f', 1);
    } else {
        detail = QStringLiteral("%1 at %2").arg(r->note, when);
    }

    if (!r->test) {
        if (CommandButton *b = buttonWidget(r->buttonId))
            b->setRunState(ok ? CommandButton::RunState::Ok : CommandButton::RunState::Failed, detail);
        m_status->refreshSoon(r->buttonId, 3000);
        m_status->refreshSoon(r->buttonId, 12000);
    }

    if (!r->error.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("✗ “%1” couldn't start.").arg(r->label), 8000);
        QMessageBox::warning(m_editDialog ? m_editDialog.data() : this, windowTitle(),
                             QStringLiteral("“%1” couldn't be started.\n\n%2").arg(r->label, r->error));
    } else if (r->mode == RunMode::Output) {
        statusBar()->showMessage(
            ok ? QStringLiteral("✓ “%1” finished — %2").arg(r->label, detail)
               : QStringLiteral("✗ “%1” — %2. Press F9 to see why.").arg(r->label, detail),
            8000);
    } else {
        statusBar()->showMessage(QStringLiteral("“%1” %2.").arg(r->label, r->note), 5000);
    }

    // A Test run from the Edit dialog: the Output panel is behind the
    // dialog, so show the result over it.
    if (r->test && m_editDialog && r->mode != RunMode::Terminal && r->error.isEmpty()) {
        const QString header = r->mode == RunMode::Output
                                   ? QStringLiteral("<b>%1</b> — %2").arg(ok ? QStringLiteral("✓ Worked")
                                                                             : QStringLiteral("✗ Failed"),
                                                                          detail.toHtmlEscaped())
                                   : QStringLiteral("<b>Started in the background.</b> Its log is "
                                                    "shown below and in the Output panel.");
        QString text = r->mode == RunMode::Output ? m_runner->displayOutput(runId)
                                                  : QStringLiteral("(The log fills as it runs: "
                                                                   "right-click the button → Show log.)");
        if (text.trimmed().isEmpty())
            text = QStringLiteral("(it printed nothing)");
        showTextDialog(m_editDialog, QStringLiteral("Test run"), header, text);
    }
    updateStatusBar();
}

// ================================================================ buttons
void MainWindow::addButton(const QPoint &at)
{
    PageCanvas *canvas = currentCanvas();
    if (!canvas)
        return;
    if (!m_store->canAddButtons(1)) {
        QMessageBox::information(this, windowTitle(),
                                 QStringLiteral("You've reached %1 buttons.").arg(ButtonStore::MaxButtons));
        return;
    }
    ButtonSpec spec;
    spec.id = ButtonSpec::newId();
    QSize size(ButtonSpec::DefaultWidth, ButtonSpec::DefaultHeight);
    if (const ButtonSpec *ref = m_store->find(canvas->lastSelectedId()))
        size = ref->geometry.size();
    QScrollArea *scroll = scrollFor(canvas);
    const int viewW = scroll ? scroll->viewport()->width() : width();
    const QPoint pos = at.x() >= 0 ? QPoint(qRound(at.x() / 10.0) * 10, qRound(at.y() / 10.0) * 10)
                                   : canvas->freeSpot(size, viewW);
    spec.geometry = QRect(pos, size);

    QStringList names;
    for (const Page &p : m_store->pages())
        names.append(p.name);
    EditDialog d(spec, names, currentPage(), m_secrets, true, this);
    m_editDialog = &d;
    connect(&d, &EditDialog::testRequested, this, [this](const ButtonSpec &s) { runSpec(s, true); });
    connect(&d, &EditDialog::helpRequested, this, &MainWindow::showHelp);
    const int result = d.exec();
    m_editDialog = nullptr;
    if (result != QDialog::Accepted)
        return;

    ButtonSpec made = d.spec();
    const int page = d.page();
    if (page != currentPage() && page < m_canvases.size())
        made.geometry.moveTopLeft(m_canvases.at(page)->freeSpot(made.geometry.size(), viewW));
    m_store->addButton(page, made, QStringLiteral("New button"));
    m_tabs->setCurrentIndex(page);
    m_canvases.at(page)->setSelection({made.id});
    statusBar()->showMessage(QStringLiteral("Added “%1”.").arg(made.label), 4000);
}

void MainWindow::editButton(const QString &id)
{
    const ButtonSpec *found = m_store->find(id);
    if (!found)
        return;
    const ButtonSpec spec = *found;
    const int page = m_store->pageOf(id);
    QStringList names;
    for (const Page &p : m_store->pages())
        names.append(p.name);

    EditDialog d(spec, names, page, m_secrets, false, this);
    m_editDialog = &d;
    connect(&d, &EditDialog::testRequested, this, [this](const ButtonSpec &s) { runSpec(s, true); });
    connect(&d, &EditDialog::helpRequested, this, &MainWindow::showHelp);
    const int result = d.exec();
    m_editDialog = nullptr;
    if (result != QDialog::Accepted)
        return;

    if (d.deleteRequested()) {
        m_store->removeButtons({id});
        statusBar()->showMessage(QStringLiteral("Deleted “%1”. Ctrl+Z brings it back.").arg(spec.label),
                                 6000);
        return;
    }
    ButtonSpec edited = d.spec();
    const int newPage = d.page();
    if (newPage != page) {
        m_store->moveButtonsToPage({id}, newPage);
        if (const ButtonSpec *moved = m_store->find(id))
            edited.geometry = moved->geometry;
    }
    if (edited.label != spec.label || edited.command != spec.command || edited.mode != spec.mode
        || edited.confirm != spec.confirm || edited.color != spec.color
        || edited.statusCheck != spec.statusCheck)
        m_store->updateButton(edited, QStringLiteral("Edit “%1”").arg(edited.label));
    if (newPage != page)
        m_tabs->setCurrentIndex(newPage);
}

void MainWindow::editSelected()
{
    if (PageCanvas *c = currentCanvas())
        if (!c->lastSelectedId().isEmpty())
            editButton(c->lastSelectedId());
}

void MainWindow::duplicateSelected()
{
    PageCanvas *c = currentCanvas();
    if (!c || c->selectedIds().isEmpty())
        return;
    const QStringList ids = c->selectedIds();
    if (!m_store->canAddButtons(int(ids.size())))
        return;
    int bottom = PageCanvas::Margin;
    for (CommandButton *b : c->buttons())
        bottom = qMax(bottom, b->geometry().bottom() + PageCanvas::Gap + 1);
    int minY = ButtonSpec::MaxCoord;
    QList<ButtonSpec> copies;
    for (const QString &id : ids)
        if (const ButtonSpec *b = m_store->find(id)) {
            minY = qMin(minY, b->geometry.y());
            copies.append(*b);
        }
    QStringList newIds;
    for (ButtonSpec &b : copies) {
        b.id = ButtonSpec::newId();
        b.label = (b.label + QStringLiteral(" (copy)")).left(ButtonSpec::MaxLabel);
        b.geometry.moveTop(b.geometry.y() - minY + bottom);
        newIds.append(b.id);
    }
    m_store->addButtons(currentPage(), copies,
                        copies.size() == 1 ? QStringLiteral("Duplicate")
                                           : QStringLiteral("Duplicate %1 buttons").arg(copies.size()));
    c->setSelection(newIds);
    if (QScrollArea *s = scrollFor(c))
        if (CommandButton *w = c->buttonFor(newIds.first()))
            s->ensureWidgetVisible(w);
    statusBar()->showMessage(QStringLiteral("Copied — the copies are selected below. Right-click "
                                            "→ Edit to change them."),
                             6000);
}

void MainWindow::deleteButtons(const QStringList &ids)
{
    if (ids.isEmpty())
        return;
    QStringList labels;
    for (const QString &id : ids)
        if (const ButtonSpec *b = m_store->find(id))
            labels.append(b->label);
    const QString question =
        ids.size() == 1 ? QStringLiteral("Delete “%1”?").arg(labels.value(0))
                        : QStringLiteral("Delete these %1 buttons?\n\n• %2")
                              .arg(ids.size())
                              .arg(labels.join(QStringLiteral("\n• ")));
    if (QMessageBox::question(this, QStringLiteral("Delete"),
                              question + QStringLiteral("\n\n(Edit → Undo brings them back.)"))
        != QMessageBox::Yes)
        return;
    m_store->removeButtons(ids);
}

void MainWindow::setModeFor(const QStringList &ids, RunMode mode)
{
    QList<ButtonSpec> list;
    for (const QString &id : ids)
        if (const ButtonSpec *b = m_store->find(id)) {
            ButtonSpec c = *b;
            c.mode = mode;
            list.append(c);
        }
    m_store->updateButtons(list, QStringLiteral("Runs in %1").arg(RunModes::title(mode)));
}

void MainWindow::setColorFor(const QStringList &ids, const QString &color)
{
    QList<ButtonSpec> list;
    for (const QString &id : ids)
        if (const ButtonSpec *b = m_store->find(id)) {
            ButtonSpec c = *b;
            c.color = color;
            list.append(c);
        }
    m_store->updateButtons(list, QStringLiteral("Color"));
}

void MainWindow::moveToPage(const QStringList &ids, int page)
{
    m_store->moveButtonsToPage(ids, page);
    m_tabs->setCurrentIndex(page);
    if (page < m_canvases.size())
        m_canvases.at(page)->setSelection(ids);
}

QMenu *MainWindow::pageMenuFor(const QStringList &ids, QWidget *parent)
{
    auto *menu = new QMenu(QStringLiteral("Move to page"), parent);
    const int here = m_store->pageOf(ids.value(0));
    for (int i = 0; i < m_store->pageCount(); ++i) {
        QAction *a = addItem(menu,m_store->pages().at(i).name,
                                     [this, ids, i] { moveToPage(ids, i); });
        a->setEnabled(i != here);
    }
    menu->addSeparator();
    addItem(menu,QStringLiteral("New page…"), [this, ids] {
        const int before = m_store->pageCount();
        newPage();
        if (m_store->pageCount() > before)
            moveToPage(ids, m_store->pageCount() - 1);
    });
    return menu;
}

void MainWindow::showButtonMenu(const QString &id, const QPoint &globalPos)
{
    PageCanvas *c = currentCanvas();
    const ButtonSpec *found = m_store->find(id);
    if (!c || !found)
        return;
    const ButtonSpec spec = *found;
    QStringList ids = c->selectedIds();
    if (!ids.contains(id))
        ids = QStringList{id};
    const QString many = ids.size() > 1 ? QStringLiteral(" (%1 buttons)").arg(ids.size()) : QString();

    QMenu menu(this);
    menu.setToolTipsVisible(true);
    QAction *run = addItem(&menu,QStringLiteral("▶ Run “%1”").arg(spec.label),
                                  [this, id] { runButton(id); });
    menu.setDefaultAction(run);
    addItem(&menu,QStringLiteral("Edit…"), [this, id] { editButton(id); });
    addItem(&menu,QStringLiteral("Duplicate") + many, [this] { duplicateSelected(); });
    menu.addSeparator();

    QMenu *modes = menu.addMenu(QStringLiteral("Runs in") + many);
    for (RunMode m : RunModes::all()) {
        QAction *a = addItem(modes,QStringLiteral("%1   %2").arg(RunModes::glyph(m), RunModes::title(m)), [this, ids, m] { setModeFor(ids, m); });
        a->setCheckable(true);
        a->setChecked(spec.mode == m);
        a->setToolTip(RunModes::summary(m));
    }
    modes->setToolTipsVisible(true);
    QMenu *colors = menu.addMenu(QStringLiteral("Color") + many);
    for (const Swatch &s : Swatches::all()) {
        QAction *a = addItem(colors,swatchIcon(s), s.name,
                                       [this, ids, key = s.key] { setColorFor(ids, key); });
        a->setCheckable(true);
        a->setChecked(spec.color == s.key);
    }
    menu.addMenu(pageMenuFor(ids, &menu))->setText(QStringLiteral("Move to page") + many);
    menu.addSeparator();

    const quint64 last = m_runner->lastRunFor(id);
    QAction *showOut = addItem(&menu,QStringLiteral("Show last output"), [this, last] {
        m_output->showRun(last);
    });
    showOut->setEnabled(last != 0);
    if (spec.mode == RunMode::Background || QFile::exists(CommandRunner::logPathFor(id)))
        addItem(&menu,QStringLiteral("Show log"),
                       [this, id, label = spec.label] { m_output->showLogFor(id, label); });
    if (!spec.statusCheck.isEmpty())
        addItem(&menu,QStringLiteral("Refresh status light"), [this, id] { m_status->refresh(id); });
    menu.addSeparator();
    addItem(&menu,QStringLiteral("Delete") + many, [this, ids] { deleteButtons(ids); });
    menu.exec(globalPos);
}

void MainWindow::showCanvasMenu(const QPoint &globalPos, const QPoint &canvasPos)
{
    QMenu menu(this);
    addItem(&menu,QStringLiteral("Add button here…"), [this, canvasPos] { addButton(canvasPos); });
    menu.addAction(m_actSelectAll);
    menu.addSeparator();
    menu.addAction(m_actOrganize);
    menu.addAction(m_actSameSize);
    menu.addAction(m_actLock);
    menu.addSeparator();
    menu.addAction(m_actNewPage);
    menu.exec(globalPos);
}

// ============================================================= arranging
void MainWindow::organize()
{
    PageCanvas *c = currentCanvas();
    QScrollArea *s = scrollFor(c);
    if (!c || !s)
        return;
    const auto g = c->organized(s->viewport()->width(), c->selectedIds().size() > 1);
    m_store->setGeometries(g, QStringLiteral("Organize"));
}

void MainWindow::makeSameSize()
{
    PageCanvas *c = currentCanvas();
    if (!c)
        return;
    if (c->buttons().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("No Buttons"),
                                 QStringLiteral("There are no command buttons to resize."));
        return;
    }
    const ButtonSpec *ref = m_store->find(c->lastSelectedId());
    if (!ref) {
        QMessageBox::information(this, QStringLiteral("Select a Reference Button"),
                                 QStringLiteral("Select the button whose size you want to copy "
                                                "(Ctrl+click it), then click Make All Same Size."));
        return;
    }
    const QSize size = ref->geometry.size();
    QHash<QString, QRect> g;
    const QStringList sel = c->selectedIds();
    const bool all = sel.size() == 1;
    for (CommandButton *b : c->buttons())
        if (all || sel.contains(b->id()))
            g.insert(b->id(), QRect(b->geometry().topLeft(), size));
    m_store->setGeometries(g, QStringLiteral("Make same size"));
}

void MainWindow::setLocked(bool locked)
{
    AppSettings::setLocked(locked);
    m_actLock->setChecked(locked);
    m_actLock->setText(locked ? QStringLiteral("🔒 Locked") : QStringLiteral("🔒 Lock"));
    for (PageCanvas *c : std::as_const(m_canvases))
        c->setLocked(locked);
    updateActions();
}

// ================================================================= pages
void MainWindow::newPage()
{
    if (m_store->pageCount() >= ButtonStore::MaxPages)
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Page"),
                                               QStringLiteral("Name for the new page:"),
                                               QLineEdit::Normal,
                                               m_store->uniquePageName(QStringLiteral("Page %1")
                                                                           .arg(m_store->pageCount() + 1)),
                                               &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    const int index = m_store->addPage(name);
    if (index >= 0)
        m_tabs->setCurrentIndex(index);
}

void MainWindow::renamePage(int index)
{
    if (index < 0 || index >= m_store->pageCount())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("Rename Page"),
                                               QStringLiteral("New name:"), QLineEdit::Normal,
                                               m_store->pages().at(index).name, &ok);
    if (ok)
        m_store->renamePage(index, name);
}

void MainWindow::deletePage(int index)
{
    if (index < 0 || index >= m_store->pageCount())
        return;
    if (m_store->pageCount() == 1) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("This is the only page."));
        return;
    }
    const Page &p = m_store->pages().at(index);
    if (!p.buttons.isEmpty()
        && QMessageBox::question(this, QStringLiteral("Delete Page"),
                                 QStringLiteral("Delete the page “%1” and its %2 button(s)?\n\n"
                                                "(Edit → Undo brings it back.)")
                                     .arg(p.name)
                                     .arg(p.buttons.size()))
               != QMessageBox::Yes)
        return;
    m_store->removePage(index);
}

void MainWindow::showTabMenu(const QPoint &pos)
{
    const int index = m_tabs->tabBar()->tabAt(pos);
    QMenu menu(this);
    if (index >= 0) {
        addItem(&menu,QStringLiteral("Rename…"), [this, index] { renamePage(index); });
        addItem(&menu,QStringLiteral("Delete page…"), [this, index] { deletePage(index); });
        menu.addSeparator();
    }
    menu.addAction(m_actNewPage);
    menu.exec(m_tabs->tabBar()->mapToGlobal(pos));
}

// ================================================================= tools
void MainWindow::openSecrets()
{
    SecretsDialog d(m_secrets, [this] { return m_store->allButtons(); }, this);
    connect(&d, &SecretsDialog::helpRequested, this, &MainWindow::showHelp);
    connect(&d, &SecretsDialog::securityCheckRequested, this, [this] { securityCheck(false); });
    connect(&d, &SecretsDialog::renamed, this, [this](const QString &oldName, const QString &newName) {
        QList<ButtonSpec> changed;
        for (ButtonSpec b : m_store->allButtons()) {
            const QString c = SecretStore::renameReferences(b.command, oldName, newName);
            const QString s = SecretStore::renameReferences(b.statusCheck, oldName, newName);
            if (c != b.command || s != b.statusCheck) {
                b.command = c;
                b.statusCheck = s;
                changed.append(b);
            }
        }
        m_store->updateButtons(changed, QStringLiteral("Rename secret"));
    });
    d.exec();
}

void MainWindow::securityCheck(bool quietIfClean)
{
    for (const QString &f : {AppPaths::buttonsFile(), AppPaths::secretsFile()})
        if (AppPaths::isExposed(f))
            AppPaths::makePrivateFile(f);

    QList<SecretScanner::Finding> findings;
    QMap<QString, QStringList> usedIn;      // value -> labels
    for (const ButtonSpec &b : m_store->allButtons()) {
        for (const QString &text : {b.command, b.statusCheck})
            for (const SecretScanner::Finding &f : SecretScanner::scan(text)) {
                if (!usedIn.contains(f.value))
                    findings.append(f);
                if (!usedIn.value(f.value).contains(b.label))
                    usedIn[f.value].append(b.label);
            }
    }

    if (findings.isEmpty()) {
        if (!quietIfClean)
            QMessageBox::information(this, QStringLiteral("Security Check"),
                                     QStringLiteral("✓ No passwords or tokens are typed into any "
                                                    "command, and your files are private."));
        scrubOldCopies();
        return;
    }

    QStringList lines;
    for (const SecretScanner::Finding &f : findings) {
        const QString existing = m_secrets->nameForValue(f.value);
        lines.append(QStringLiteral("• %1 (%2…) in: %3\n   → will be saved as $%4")
                         .arg(f.kind, f.value.left(6),
                              usedIn.value(f.value).join(QStringLiteral(", ")),
                              existing.isEmpty() ? m_secrets->uniqueName(f.suggestedName) : existing));
    }
    QMessageBox q(QMessageBox::Warning, QStringLiteral("Security Check"),
                  QStringLiteral("%1 password(s) or token(s) are typed into button commands.")
                      .arg(findings.size()),
                  QMessageBox::NoButton, this);
    q.setInformativeText(lines.join(QLatin1Char('\n'))
                         + QStringLiteral("\n\nMove them into the secrets vault? The commands will "
                                          "use the names instead; they'll work exactly as before."));
    QPushButton *move = q.addButton(QStringLiteral("Move to vault"), QMessageBox::AcceptRole);
    q.addButton(QStringLiteral("Not now"), QMessageBox::RejectRole);
    QPushButton *help = q.addButton(QMessageBox::Help);
    q.setDefaultButton(move);
    q.exec();
    if (q.clickedButton() == help) {
        showHelp(QStringLiteral("securitycheck"));
        return;
    }
    if (q.clickedButton() != move)
        return;

    QString why;
    const QMap<QString, QString> filed = m_secrets->vault(findings, &why);
    if (filed.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Security Check"),
                             QStringLiteral("Nothing could be saved in the vault.\n\n%1").arg(why));
        return;
    }
    QList<ButtonSpec> changed;
    for (ButtonSpec b : m_store->allButtons()) {
        QString c = b.command, s = b.statusCheck;
        for (auto it = filed.cbegin(); it != filed.cend(); ++it) {
            c = SecretScanner::replaceValue(c, it.key(), it.value());
            s = SecretScanner::replaceValue(s, it.key(), it.value());
        }
        if (c != b.command || s != b.statusCheck) {
            b.command = c;
            b.statusCheck = s;
            changed.append(b);
        }
    }
    m_store->updateButtons(changed, QStringLiteral("Move secrets to vault"));
    m_store->saveNow();

    QStringList names;
    for (const QString &n : filed)
        if (!names.contains(QStringLiteral("$") + n))
            names.append(QStringLiteral("$") + n);
    QMessageBox::information(
        this, QStringLiteral("Security Check"),
        QStringLiteral("Done: %1 button(s) now use %2 from the vault.\n\n"
                       "Because these were stored in plain text, it's wise to replace them: make a "
                       "new token or password, delete the old one, then use Change value… in the "
                       "vault (Ctrl+K). No button needs editing.")
            .arg(changed.size())
            .arg(names.join(QStringLiteral(", "))));
    if (!why.isEmpty())
        QMessageBox::warning(this, QStringLiteral("Security Check"), why);
    scrubOldCopies();
}

void MainWindow::scrubOldCopies()
{
    const QStringList names = m_secrets->names();
    if (names.isEmpty())
        return;
    QMap<QString, QString> valueToName;
    QStringList values;
    for (const QString &n : names) {
        const QString v = m_secrets->value(n);
        if (v.size() >= 6) {
            valueToName.insert(v, n);
            values.append(v);
        }
    }
    if (values.isEmpty())
        return;

    QStringList candidates = m_store->backupFiles();
    const QString legacy = AppPaths::legacyButtonsFile();
    if (QFile::exists(legacy) && QFileInfo(legacy).canonicalFilePath()
                                     != QFileInfo(AppPaths::buttonsFile()).canonicalFilePath())
        candidates.append(legacy);

    QStringList dirty;
    for (const QString &f : candidates) {
        QJsonDocument doc;
        if (!ButtonStore::readJsonFile(f, &doc, nullptr))
            continue;
        const QJsonValue root = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
        if (jsonContainsAny(root, values))
            dirty.append(f);
    }
    if (dirty.isEmpty())
        return;

    const bool hasLegacy = dirty.contains(legacy);
    QString text = QStringLiteral("%1 older cop%2 of your buttons still hold vault secrets in "
                                  "plain text:\n\n")
                       .arg(dirty.size())
                       .arg(dirty.size() == 1 ? QStringLiteral("y") : QStringLiteral("ies"));
    const int backups = int(dirty.size()) - (hasLegacy ? 1 : 0);
    if (backups > 0)
        text += QStringLiteral("• %1 automatic backup(s)\n").arg(backups);
    if (hasLegacy)
        text += QStringLiteral("• the version-1 file beside the old program:\n   %1\n").arg(legacy);
    text += QStringLiteral("\nClean them? Only the secret text is replaced by its $NAME; nothing "
                           "else in those files changes.");
    if (QMessageBox::question(this, QStringLiteral("Clean Old Copies"), text) != QMessageBox::Yes)
        return;

    int cleaned = 0;
    QStringList failed;
    for (const QString &f : dirty) {
        QJsonDocument doc;
        if (!ButtonStore::readJsonFile(f, &doc, nullptr))
            continue;
        bool changed = false;
        const QJsonValue root = scrubJson(doc.isArray() ? QJsonValue(doc.array())
                                                        : QJsonValue(doc.object()),
                                          valueToName, &changed);
        if (!changed)
            continue;
        const QJsonDocument out = root.isArray() ? QJsonDocument(root.toArray())
                                                 : QJsonDocument(root.toObject());
        QSaveFile sf(f);
        const QByteArray bytes = out.toJson(QJsonDocument::Indented);
        if (sf.open(QIODevice::WriteOnly) && sf.write(bytes) == bytes.size() && sf.commit()) {
            AppPaths::makePrivateFile(f);
            ++cleaned;
        } else {
            failed.append(f);
        }
    }
    QString done = QStringLiteral("Cleaned %1 file(s).").arg(cleaned);
    if (!failed.isEmpty())
        done += QStringLiteral("\n\nThese couldn't be changed:\n%1").arg(failed.join(QLatin1Char('\n')));
    QMessageBox::information(this, QStringLiteral("Clean Old Copies"), done);
}

void MainWindow::openPreferences()
{
    PreferencesDialog d(this);
    connect(&d, &PreferencesDialog::helpRequested, this, &MainWindow::showHelp);
    if (d.exec() == QDialog::Accepted)
        applyPreferences();
}

void MainWindow::applyPreferences()
{
    m_runner->setTerminal(AppSettings::terminal());
    m_status->setIntervalSec(AppSettings::statusIntervalSec());
    for (PageCanvas *c : std::as_const(m_canvases))
        c->setSnap(AppSettings::snapToGrid());
}

void MainWindow::exportButtons()
{
    QList<Page> pages;
    if (m_store->pageCount() > 1) {
        QMessageBox q(QMessageBox::Question, QStringLiteral("Export Buttons"),
                      QStringLiteral("Export which buttons?"), QMessageBox::NoButton, this);
        QPushButton *thisPage = q.addButton(QStringLiteral("This page (“%1”)")
                                                .arg(m_store->pages().at(currentPage()).name),
                                            QMessageBox::AcceptRole);
        QPushButton *all = q.addButton(QStringLiteral("All pages"), QMessageBox::AcceptRole);
        q.addButton(QMessageBox::Cancel);
        q.exec();
        if (q.clickedButton() == thisPage)
            pages.append(m_store->pages().at(currentPage()));
        else if (q.clickedButton() == all)
            pages = m_store->pages();
        else
            return;
    } else {
        pages = m_store->pages();
    }

    int typedIn = 0;
    for (const Page &p : pages)
        for (const ButtonSpec &b : p.buttons)
            if (!SecretScanner::scan(b.command + QLatin1Char('\n') + b.statusCheck).isEmpty())
                ++typedIn;
    if (typedIn > 0
        && QMessageBox::warning(this, QStringLiteral("Export Buttons"),
                                QStringLiteral("%1 of these buttons have a password or token typed "
                                               "into the command, so it would be in the exported "
                                               "file.\n\nExport anyway? (Tools → Security check "
                                               "fixes this first.)")
                                    .arg(typedIn),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
               != QMessageBox::Yes)
        return;

    const QString suggested = QDir::home().filePath(
        QStringLiteral("CommandButtons-%1.json")
            .arg(pages.size() == 1 ? pages.first().name.simplified().replace(QLatin1Char(' '), QLatin1Char('-'))
                                   : QStringLiteral("all")));
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export Buttons"), suggested,
                                                      QStringLiteral("Button files (*.json)"));
    if (path.isEmpty())
        return;
    QSaveFile f(path);
    const QByteArray bytes = m_store->toDocument(pages).toJson(QJsonDocument::Indented);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
        QMessageBox::warning(this, QStringLiteral("Export Buttons"),
                             QStringLiteral("Couldn't write %1:\n%2").arg(path, f.errorString()));
        return;
    }
    AppPaths::makePrivateFile(path);
    int count = 0;
    for (const Page &p : pages)
        count += int(p.buttons.size());
    statusBar()->showMessage(QStringLiteral("Exported %1 button(s) to %2").arg(count).arg(path), 8000);
}

void MainWindow::importButtons()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Import Buttons"),
                                                      QDir::homePath(),
                                                      QStringLiteral("Button files (*.json)"));
    if (path.isEmpty())
        return;
    QJsonDocument doc;
    QList<Page> pages;
    QStringList warnings;
    QString err;
    if (!ButtonStore::readJsonFile(path, &doc, &err)
        || !ButtonStore::parseDocument(doc, &pages, &warnings, &err)) {
        QMessageBox::warning(this, QStringLiteral("Import Buttons"), err);
        return;
    }
    int count = 0;
    QString listing;
    for (const Page &p : pages) {
        listing += QStringLiteral("═══ Page: %1 ═══\n\n").arg(p.name);
        for (const ButtonSpec &b : p.buttons) {
            ++count;
            listing += QStringLiteral("■ %1   [%2]\n").arg(b.label, RunModes::title(b.mode));
            for (const QString &line : b.command.split(QLatin1Char('\n')))
                listing += QStringLiteral("    %1\n").arg(line);
            if (!b.statusCheck.isEmpty())
                listing += QStringLiteral("    (status light: %1)\n").arg(b.statusCheck);
            listing += QLatin1Char('\n');
        }
    }
    if (count == 0) {
        QMessageBox::information(this, QStringLiteral("Import Buttons"),
                                 QStringLiteral("That file has no buttons."));
        return;
    }
    if (!m_store->canAddButtons(count)) {
        QMessageBox::information(this, QStringLiteral("Import Buttons"),
                                 QStringLiteral("That would be more than %1 buttons.")
                                     .arg(ButtonStore::MaxButtons));
        return;
    }
    if (!showTextDialog(this, QStringLiteral("Import Buttons"),
                        QStringLiteral("<b>%1 button(s) on %2 page(s).</b> Read the commands below — "
                                       "importing means trusting them. They're added as new pages; "
                                       "nothing you have changes.")
                            .arg(count)
                            .arg(pages.size()),
                        listing, QStringLiteral("Import")))
        return;
    m_store->backupNow(QStringLiteral("before-import"));
    const int first = m_store->pageCount();
    if (m_store->appendPages(pages, QStringLiteral("Import")) > 0) {
        m_tabs->setCurrentIndex(first);
        statusBar()->showMessage(QStringLiteral("Imported %1 button(s). Ctrl+Z removes them.").arg(count),
                                 8000);
    }
}

void MainWindow::restoreBackup()
{
    const QStringList files = m_store->backupFiles();
    if (files.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Restore from Backup"),
                                 QStringLiteral("There are no backups yet. One is made the first time "
                                                "your buttons are saved in each session."));
        return;
    }
    QStringList items;
    for (const QString &f : files) {
        const QFileInfo fi(f);
        QString why = fi.completeBaseName().section(QLatin1Char('-'), 4, -1);
        if (why.isEmpty())
            why = QStringLiteral("backup");
        int count = -1;
        QJsonDocument doc;
        QList<Page> pages;
        if (ButtonStore::readJsonFile(f, &doc, nullptr)
            && ButtonStore::parseDocument(doc, &pages, nullptr, nullptr)) {
            count = 0;
            for (const Page &p : pages)
                count += int(p.buttons.size());
        }
        items.append(QStringLiteral("%1 — %2 — %3")
                         .arg(fi.lastModified().toString(QStringLiteral("ddd d MMM yyyy, HH:mm")), why,
                              count < 0 ? QStringLiteral("unreadable")
                                        : QStringLiteral("%1 buttons").arg(count)));
    }
    bool ok = false;
    const QString pick = QInputDialog::getItem(this, QStringLiteral("Restore from Backup"),
                                               QStringLiteral("Pick a copy to bring back (newest "
                                                              "first). Your current buttons are "
                                                              "backed up first, and Undo reverses "
                                                              "this."),
                                               items, 0, false, &ok);
    if (!ok)
        return;
    const QString file = files.value(int(items.indexOf(pick)));
    QString err;
    if (!m_store->restoreBackup(file, &err))
        QMessageBox::warning(this, QStringLiteral("Restore from Backup"), err);
    else
        statusBar()->showMessage(QStringLiteral("Restored. Ctrl+Z goes back."), 8000);
}

void MainWindow::createLauncher()
{
    const QString icon = AppPaths::iconFile();
    const QString file = AppPaths::launcherFile();
    if (!Theme::writeIconPng(icon)) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Couldn't save the icon to %1.").arg(icon));
        return;
    }
    const QString exe = QCoreApplication::applicationFilePath();
    const QString entry = QStringLiteral("[Desktop Entry]\n"
                                         "Type=Application\n"
                                         "Name=Command Button Creator\n"
                                         "Comment=One-press buttons for shell commands\n"
                                         "Exec=%1\n"
                                         "Icon=%2\n"
                                         "Terminal=false\n"
                                         "Categories=Utility;\n"
                                         "StartupWMClass=CommandButtonCreator\n")
                              .arg(desktopExecQuote(exe), icon);
    QSaveFile f(file);
    const QByteArray bytes = entry.toUtf8();
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Couldn't write %1.").arg(file));
        return;
    }
    QMessageBox::information(this, QStringLiteral("Desktop Launcher"),
                             QStringLiteral("Command Button Creator is now in your Applications.\n\n"
                                            "To keep it in the dock: open Applications, right-click "
                                            "Command Button Creator, and choose Add to Favorites."));
}

void MainWindow::setZoom(int pt)
{
    pt = qBound(AppSettings::MinZoomPt, pt, AppSettings::MaxZoomPt);
    if (pt == m_zoomPt)
        return;
    m_zoomPt = pt;
    AppSettings::setZoomPt(pt);
    Theme::apply(*qApp, pt);
    for (PageCanvas *c : std::as_const(m_canvases))
        c->setLabelPointSize(pt + 2);
    statusBar()->showMessage(QStringLiteral("Text size %1 pt (Ctrl+0 for normal)").arg(pt), 2500);
}

void MainWindow::showHelp(const QString &topic)
{
    HelpBrowser::showTopic(this, topic);
}

void MainWindow::showAbout()
{
    QMessageBox about(this);
    about.setWindowTitle(QStringLiteral("About"));
    about.setIconPixmap(Theme::appIconPixmap(64));
    about.setText(QStringLiteral("<b>Command Button Creator %1</b><br>Tardis Software")
                      .arg(QStringLiteral(APP_VERSION)));
    about.setInformativeText(QStringLiteral("Created by Mark Mcdougal<br>March 5, 2026<br><br>"
                                            "Version 2.0, November 2026.<br>Runs entirely on this "
                                            "computer."));
    about.exec();
}

bool MainWindow::showTextDialog(QWidget *parent, const QString &title, const QString &header,
                                const QString &text, const QString &acceptText)
{
    QDialog d(parent ? parent : this);
    d.setWindowTitle(title);
    d.setWindowFlags(d.windowFlags() | Qt::WindowMinMaxButtonsHint);
    d.setSizeGripEnabled(true);
    auto *label = new QLabel(header);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    auto *view = new QPlainTextEdit(text);
    view->setReadOnly(true);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(font().pointSize());
    view->setFont(mono);
    auto *box = new QDialogButtonBox;
    if (acceptText.isEmpty()) {
        box->addButton(QDialogButtonBox::Close);
    } else {
        box->addButton(acceptText, QDialogButtonBox::AcceptRole);
        box->addButton(QDialogButtonBox::Cancel);
    }
    connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    auto *layout = new QVBoxLayout(&d);
    layout->addWidget(label);
    layout->addWidget(view, 1);
    layout->addWidget(box);
    d.resize(640, 420);
    return d.exec() == QDialog::Accepted;
}

// ================================================================ events
void MainWindow::closeEvent(QCloseEvent *e)
{
    const int running = m_runner->runningCount();
    if (running > 0
        && QMessageBox::question(this, windowTitle(),
                                 QStringLiteral("%1 Output-panel command(s) are still running and "
                                                "will be stopped. Close anyway?\n\n(Background "
                                                "commands are not affected.)")
                                     .arg(running))
               != QMessageBox::Yes) {
        e->ignore();
        return;
    }
    m_runner->stopAll();
    AppSettings::setWindowGeometry(saveGeometry());
    AppSettings::setWindowState(saveState());
    QString err;
    if (!m_store->saveNow(&err) && !err.isEmpty())
        QMessageBox::warning(this, windowTitle(), err);
    e->accept();
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
        m_status->setPaused(isMinimized());
    QMainWindow::changeEvent(e);
}
