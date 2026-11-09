// outputpanel.cpp — see outputpanel.h.
#include "outputpanel.h"

#include "commandrunner.h"
#include "secretstore.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr qint64 kLogTailBytes = 256 * 1024;
}

OutputPanel::OutputPanel(CommandRunner *runner, SecretStore *secrets, QWidget *parent)
    : QDockWidget(QStringLiteral("Output"), parent)
    , m_runner(runner)
    , m_secrets(secrets)
{
    setObjectName(QStringLiteral("outputPanel"));
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
                | QDockWidget::DockWidgetFloatable);

    m_list = new QListWidget;
    m_list->setToolTip(QStringLiteral("Every run this session, newest at the bottom.\n"
                                      "● running   ✓ worked   ✗ failed\n"
                                      "▣ opened a terminal   ↗ started in the background"));
    m_header = new QLabel;
    m_header->setWordWrap(true);
    m_header->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_view = new QPlainTextEdit;
    m_view->setReadOnly(true);
    m_view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(font().pointSize());
    m_view->setFont(mono);
    m_view->setPlaceholderText(QStringLiteral("Run a button set to “Output panel” and what it "
                                              "prints appears here."));

    m_stop = new QPushButton(QStringLiteral("■ Stop"));
    m_stop->setToolTip(QStringLiteral("Stop the selected run, and everything it started."));
    m_copy = new QPushButton(QStringLiteral("Copy"));
    m_copy->setToolTip(QStringLiteral("Copy the output shown (secrets stay blanked out)."));
    m_openLog = new QPushButton(QStringLiteral("Open log"));
    m_openLog->setToolTip(QStringLiteral("Open this Background button's log in your text editor."));
    m_clear = new QPushButton(QStringLiteral("Clear finished"));
    m_clear->setToolTip(QStringLiteral("Remove finished runs from the list."));
    auto *help = new QPushButton(QStringLiteral("?"));
    help->setToolTip(QStringLiteral("Help on the Output panel"));
    help->setFixedWidth(help->sizeHint().height() + 6);

    auto *buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(m_stop);
    buttons->addWidget(m_copy);
    buttons->addWidget(m_openLog);
    buttons->addStretch();
    buttons->addWidget(m_clear);
    buttons->addWidget(help);

    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);
    rightLayout->addWidget(m_header);
    rightLayout->addWidget(m_view, 1);
    rightLayout->addLayout(buttons);

    auto *split = new QSplitter(Qt::Horizontal);
    split->addWidget(m_list);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 3);
    split->setChildrenCollapsible(false);

    auto *body = new QWidget;
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(4, 4, 4, 4);
    bodyLayout->addWidget(split);
    setWidget(body);

    m_refresh.setSingleShot(true);
    m_refresh.setInterval(150);
    connect(&m_refresh, &QTimer::timeout, this, &OutputPanel::refreshView);

    connect(m_list, &QListWidget::currentItemChanged, this, [this] {
        m_logPath.clear();
        refreshView();
        refreshButtons();
    });
    connect(m_runner, &CommandRunner::historyChanged, this, &OutputPanel::rebuildList);
    connect(m_runner, &CommandRunner::runUpdated, this, [this](quint64 id) {
        if (QListWidgetItem *it = itemFor(id))
            updateItem(it, id);
        if (id == currentId())
            m_refresh.start();
    });
    connect(m_runner, &CommandRunner::runFinished, this, [this](quint64 id) {
        if (QListWidgetItem *it = itemFor(id))
            updateItem(it, id);
        if (id == currentId()) {
            refreshView();
            refreshButtons();
        }
    });
    connect(m_stop, &QPushButton::clicked, this, [this] { m_runner->stop(currentId()); });
    connect(m_copy, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(m_view->toPlainText());
    });
    connect(m_openLog, &QPushButton::clicked, this, [this] {
        QString path = m_logPath;
        if (path.isEmpty())
            if (const RunRecord *r = m_runner->record(currentId()))
                path = r->logPath;
        if (!path.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    connect(m_clear, &QPushButton::clicked, m_runner, &CommandRunner::clearFinished);
    connect(help, &QPushButton::clicked, this,
            [this] { emit helpRequested(QStringLiteral("outputpanel")); });

    rebuildList();
}

QListWidgetItem *OutputPanel::itemFor(quint64 id) const
{
    for (int i = 0; i < m_list->count(); ++i)
        if (m_list->item(i)->data(Qt::UserRole).toULongLong() == id)
            return m_list->item(i);
    return nullptr;
}

quint64 OutputPanel::currentId() const
{
    const QListWidgetItem *it = m_list->currentItem();
    return it ? it->data(Qt::UserRole).toULongLong() : 0;
}

void OutputPanel::updateItem(QListWidgetItem *item, quint64 id)
{
    const RunRecord *r = m_runner->record(id);
    if (!r)
        return;
    QString glyph;
    QColor color(0xea, 0xf3, 0xf4);
    if (r->running) {
        glyph = QStringLiteral("●");
        color = QColor(0xff, 0xb3, 0x66);
    } else if (!r->error.isEmpty()) {
        glyph = QStringLiteral("✗");
        color = QColor(0xff, 0x8a, 0x80);
    } else if (r->mode == RunMode::Terminal) {
        glyph = QStringLiteral("▣");
    } else if (r->mode == RunMode::Background) {
        glyph = QStringLiteral("↗");
    } else if (r->succeeded()) {
        glyph = QStringLiteral("✓");
        color = QColor(0x8f, 0xe3, 0xaa);
    } else {
        glyph = QStringLiteral("✗");
        color = QColor(0xff, 0x8a, 0x80);
    }
    QString text = QStringLiteral("%1  %2  %3%4")
                       .arg(r->started.toString(QStringLiteral("HH:mm:ss")), glyph,
                            r->test ? QStringLiteral("Test: ") : QString(), r->label);
    item->setText(text);
    item->setForeground(color);
    item->setToolTip(QStringLiteral("%1 — %2").arg(r->label, r->resultText()));
}

void OutputPanel::rebuildList()
{
    const quint64 keep = currentId();
    const QList<quint64> ids = m_runner->history();
    m_list->blockSignals(true);
    m_list->clear();
    for (quint64 id : ids) {
        auto *item = new QListWidgetItem;
        item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(id));
        updateItem(item, id);
        m_list->addItem(item);
        if (id == keep)
            m_list->setCurrentItem(item);
    }
    m_list->blockSignals(false);
    if (!m_list->currentItem() && m_logPath.isEmpty())
        refreshView();
    refreshButtons();
}

void OutputPanel::showRun(quint64 id)
{
    if (QListWidgetItem *it = itemFor(id)) {
        m_logPath.clear();
        m_list->setCurrentItem(it);
        m_list->scrollToItem(it);
        refreshView();
        refreshButtons();
    }
    show();
    raise();
}

void OutputPanel::showLogFor(const QString &buttonId, const QString &label)
{
    m_list->blockSignals(true);
    m_list->setCurrentItem(nullptr);
    m_list->clearSelection();
    m_list->blockSignals(false);
    m_logPath = CommandRunner::logPathFor(buttonId);
    m_logLabel = label;
    refreshView();
    refreshButtons();
    show();
    raise();
}

QString OutputPanel::readLogTail(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QStringLiteral("(no log yet — the button hasn't been run in Background mode)");
    QString prefix;
    if (f.size() > kLogTailBytes) {
        f.seek(f.size() - kLogTailBytes);
        f.readLine();                                  // skip the partial first line
        prefix = QStringLiteral("[… earlier part of the log not shown]\n");
    }
    return prefix + m_secrets->redact(ProcessUtil::cleanForDisplay(QString::fromLocal8Bit(f.readAll())));
}

void OutputPanel::refreshView()
{
    QScrollBar *sb = m_view->verticalScrollBar();
    const bool atBottom = sb->value() >= sb->maximum() - 4;
    QString text;

    if (!m_logPath.isEmpty()) {
        m_header->setText(QStringLiteral("<b>Log: %1</b><br><small>%2</small>")
                              .arg(m_logLabel.toHtmlEscaped(), m_logPath.toHtmlEscaped()));
        text = readLogTail(m_logPath);
    } else if (const RunRecord *r = m_runner->record(currentId())) {
        QString result = r->resultText();
        if (r->mode == RunMode::Output && !r->running)
            result += QStringLiteral(" · %1 s").arg(r->durationMs() / 1000.0, 0, 'f', 1);
        m_header->setText(QStringLiteral("<b>%1%2</b> — %3 — started %4 — %5")
                              .arg(r->test ? QStringLiteral("Test: ") : QString(),
                                   r->label.toHtmlEscaped(), RunModes::title(r->mode),
                                   r->started.toString(QStringLiteral("HH:mm:ss")),
                                   result.toHtmlEscaped()));
        if (!r->error.isEmpty())
            text = QStringLiteral("It couldn't be started:\n%1").arg(r->error);
        else if (r->mode == RunMode::Output)
            text = m_runner->displayOutput(r->id);
        else if (r->mode == RunMode::Background)
            text = readLogTail(r->logPath);
        else
            text = QStringLiteral("This button ran in a terminal window, so what it printed is "
                                  "in that window.");
    } else {
        m_header->setText(QStringLiteral("<i>Nothing selected.</i>"));
    }

    if (m_view->toPlainText() != text) {
        m_view->setPlainText(text);
        if (atBottom)
            sb->setValue(sb->maximum());
    }
}

void OutputPanel::refreshButtons()
{
    const RunRecord *r = m_runner->record(currentId());
    m_stop->setEnabled(r && r->running);
    m_openLog->setEnabled(!m_logPath.isEmpty() || (r && !r->logPath.isEmpty()));
    m_copy->setEnabled(true);
}
