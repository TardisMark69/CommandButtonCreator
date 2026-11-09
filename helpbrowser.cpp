// helpbrowser.cpp — see helpbrowser.h.
#include "helpbrowser.h"

#include "appsettings.h"
#include "helpcontent.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocumentFragment>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QPointer<HelpBrowser> g_instance;

// Plain text of each topic, worked out once, for Search.
const QHash<QString, QString> &plainTexts()
{
    static QHash<QString, QString> cache;
    if (cache.isEmpty())
        for (const HelpTopic &t : HelpContent::topics())
            cache.insert(t.id, (t.title + QLatin1Char(' ') + t.keywords.join(QLatin1Char(' '))
                                + QLatin1Char(' ')
                                + QTextDocumentFragment::fromHtml(t.html).toPlainText())
                                   .toLower());
    return cache;
}

} // namespace

void HelpBrowser::showTopic(QWidget *parent, const QString &topicId)
{
    if (!g_instance) {
        g_instance = new HelpBrowser(parent ? parent->window() : nullptr);
        g_instance->setAttribute(Qt::WA_DeleteOnClose);
    }
    g_instance->openTopic(topicId);
    g_instance->show();
    g_instance->raise();
    g_instance->activateWindow();
}

HelpBrowser::HelpBrowser(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Command Button Creator — Help"));
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setModal(false);
    setSizeGripEnabled(true);

    // ---- navigation bar
    m_back = new QPushButton(QStringLiteral("◀ Back"));
    m_forward = new QPushButton(QStringLiteral("Forward ▶"));
    auto *home = new QPushButton(QStringLiteral("⌂ Contents"));
    auto *smaller = new QPushButton(QStringLiteral("A−"));
    auto *bigger = new QPushButton(QStringLiteral("A+"));
    m_back->setToolTip(QStringLiteral("Previous page (Alt+Left)"));
    m_forward->setToolTip(QStringLiteral("Next page (Alt+Right)"));
    home->setToolTip(QStringLiteral("The welcome page"));
    smaller->setToolTip(QStringLiteral("Smaller help text"));
    bigger->setToolTip(QStringLiteral("Larger help text"));
    m_back->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
    m_forward->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
    for (QPushButton *b : {m_back, m_forward, home, smaller, bigger})
        b->setAutoDefault(false);
    auto *nav = new QHBoxLayout;
    nav->addWidget(m_back);
    nav->addWidget(m_forward);
    nav->addWidget(home);
    nav->addStretch();
    nav->addWidget(smaller);
    nav->addWidget(bigger);

    // ---- left: Contents / Index / Search
    m_contents = new QTreeWidget;
    m_contents->setHeaderHidden(true);
    m_contents->setToolTip(QStringLiteral("Click a heading to open it."));

    m_indexFilter = new QLineEdit;
    m_indexFilter->setPlaceholderText(QStringLiteral("Type a keyword…"));
    m_indexFilter->setClearButtonEnabled(true);
    m_index = new QListWidget;
    auto *indexPage = new QWidget;
    auto *indexLayout = new QVBoxLayout(indexPage);
    indexLayout->setContentsMargins(2, 4, 2, 2);
    indexLayout->addWidget(m_indexFilter);
    indexLayout->addWidget(m_index);

    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText(QStringLiteral("Search all pages…"));
    m_searchBox->setClearButtonEnabled(true);
    m_results = new QListWidget;
    auto *searchPage = new QWidget;
    auto *searchLayout = new QVBoxLayout(searchPage);
    searchLayout->setContentsMargins(2, 4, 2, 2);
    searchLayout->addWidget(m_searchBox);
    searchLayout->addWidget(m_results);

    auto *tabs = new QTabWidget;
    tabs->addTab(m_contents, QStringLiteral("Contents"));
    tabs->addTab(indexPage, QStringLiteral("Index"));
    tabs->addTab(searchPage, QStringLiteral("Search"));

    // ---- right: the page
    m_view = new QTextBrowser;
    m_view->setOpenLinks(false);
    m_view->document()->setDefaultStyleSheet(Theme::helpStyleSheet());

    auto *split = new QSplitter(Qt::Horizontal);
    split->addWidget(tabs);
    split->addWidget(m_view);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 3);
    split->setChildrenCollapsible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addLayout(nav);
    layout->addWidget(split, 1);

    buildContents();
    buildIndex();

    connect(m_back, &QPushButton::clicked, this, [this] {
        if (m_pos > 0)
            openTopic(m_history.at(--m_pos), false);
    });
    connect(m_forward, &QPushButton::clicked, this, [this] {
        if (m_pos + 1 < m_history.size())
            openTopic(m_history.at(++m_pos), false);
    });
    connect(home, &QPushButton::clicked, this, [this] { openTopic(QStringLiteral("welcome")); });
    connect(smaller, &QPushButton::clicked, this, [this] { zoom(-1); });
    connect(bigger, &QPushButton::clicked, this, [this] { zoom(1); });
    connect(m_contents, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *it) {
        openTopic(it->data(0, Qt::UserRole).toString());
    });
    connect(m_contents, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *it) {
        openTopic(it->data(0, Qt::UserRole).toString());
    });
    connect(m_indexFilter, &QLineEdit::textChanged, this, &HelpBrowser::filterIndex);
    connect(m_indexFilter, &QLineEdit::returnPressed, this, [this] {
        for (int i = 0; i < m_index->count(); ++i)
            if (!m_index->item(i)->isHidden()) {
                openTopic(m_index->item(i)->data(Qt::UserRole).toString());
                return;
            }
    });
    connect(m_index, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) {
        openTopic(it->data(Qt::UserRole).toString());
    });
    connect(m_index, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
        openTopic(it->data(Qt::UserRole).toString());
    });
    connect(m_searchBox, &QLineEdit::textChanged, this, &HelpBrowser::runSearch);
    connect(m_results, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
        openTopic(it->data(Qt::UserRole).toString());
    });
    connect(m_results, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) {
        openTopic(it->data(Qt::UserRole).toString());
    });
    connect(m_view, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        if (url.scheme() == QLatin1String("topic"))
            openTopic(url.path());
        else if (url.hasFragment() && url.path().isEmpty())
            m_view->scrollToAnchor(url.fragment());
        // Nothing else is followed: the help never opens web pages.
    });

    const QByteArray geo = AppSettings::dialogGeometry(QStringLiteral("help"));
    if (geo.isEmpty() || !restoreGeometry(geo))
        resize(900, 640);
}

void HelpBrowser::done(int result)
{
    AppSettings::setDialogGeometry(QStringLiteral("help"), saveGeometry());
    QDialog::done(result);
}

void HelpBrowser::buildContents()
{
    QHash<QString, QTreeWidgetItem *> items;
    for (const HelpTopic &t : HelpContent::topics()) {
        QTreeWidgetItem *parent = items.value(t.parent);
        auto *it = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_contents);
        it->setText(0, t.title);
        it->setData(0, Qt::UserRole, t.id);
        items.insert(t.id, it);
    }
    m_contents->expandAll();
}

void HelpBrowser::buildIndex()
{
    QMap<QString, QString> entries;   // sorted, case-insensitive key -> "word\ntopic"
    for (const HelpTopic &t : HelpContent::topics()) {
        entries.insert(t.title.toLower() + QLatin1Char('\x01') + t.id, t.title + QLatin1Char('\n') + t.id);
        for (const QString &k : t.keywords)
            entries.insert(k.toLower() + QLatin1Char('\x01') + t.id, k + QLatin1Char('\n') + t.id);
    }
    for (const QString &v : std::as_const(entries)) {
        const QStringList parts = v.split(QLatin1Char('\n'));
        const HelpTopic *t = HelpContent::find(parts.at(1));
        auto *item = new QListWidgetItem(
            parts.at(0) == t->title ? parts.at(0)
                                    : QStringLiteral("%1  —  %2").arg(parts.at(0), t->title));
        item->setData(Qt::UserRole, parts.at(1));
        m_index->addItem(item);
    }
}

void HelpBrowser::filterIndex(const QString &text)
{
    const QString needle = text.trimmed().toLower();
    for (int i = 0; i < m_index->count(); ++i) {
        QListWidgetItem *it = m_index->item(i);
        it->setHidden(!needle.isEmpty() && !it->text().toLower().startsWith(needle)
                      && !it->text().toLower().contains(QLatin1Char(' ') + needle));
    }
}

void HelpBrowser::runSearch(const QString &text)
{
    m_results->clear();
    const QStringList words = text.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty())
        return;
    for (const HelpTopic &t : HelpContent::topics()) {
        const QString &body = plainTexts().value(t.id);
        bool all = true;
        for (const QString &w : words)
            if (!body.contains(w)) {
                all = false;
                break;
            }
        if (all) {
            auto *item = new QListWidgetItem(t.title);
            item->setData(Qt::UserRole, t.id);
            m_results->addItem(item);
        }
    }
    if (m_results->count() == 0) {
        auto *none = new QListWidgetItem(QStringLiteral("(no pages contain all of those words)"));
        none->setFlags(Qt::NoItemFlags);
        m_results->addItem(none);
    }
}

void HelpBrowser::openTopic(const QString &id, bool record)
{
    const HelpTopic *t = HelpContent::find(id);
    if (!t)
        t = HelpContent::find(QStringLiteral("welcome"));
    if (!t)
        return;
    m_view->setHtml(t->html);
    if (record) {
        while (m_history.size() > m_pos + 1)
            m_history.removeLast();
        if (m_history.isEmpty() || m_history.last() != t->id) {
            m_history.append(t->id);
            m_pos = int(m_history.size()) - 1;
        }
    }
    // Keep the Contents tree in step with the page shown.
    QTreeWidgetItemIterator it(m_contents);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toString() == t->id) {
            m_contents->setCurrentItem(*it);
            break;
        }
        ++it;
    }
    updateNav();
}

void HelpBrowser::updateNav()
{
    m_back->setEnabled(m_pos > 0);
    m_forward->setEnabled(m_pos + 1 < m_history.size());
}

void HelpBrowser::zoom(int step)
{
    if ((step > 0 && m_zoom >= 8) || (step < 0 && m_zoom <= -4))
        return;
    m_zoom += step;
    if (step > 0)
        m_view->zoomIn(1);
    else
        m_view->zoomOut(1);
}
