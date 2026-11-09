// buttonstore.cpp — see buttonstore.h.
#include "buttonstore.h"

#include "apppaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace {

const QString kDefaultPage = QStringLiteral("Main");

QString timestamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
}

QString cleanPageName(QString name)
{
    name = name.simplified();
    name.remove(QChar(0));
    name.truncate(ButtonStore::MaxPageName);
    return name;
}

} // namespace

ButtonStore::ButtonStore(QObject *parent)
    : QObject(parent)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, [this] {
        QString err;
        if (!saveNow(&err))
            emit saveFailed(err);
    });
    ensureOnePage();
}

ButtonStore::~ButtonStore()
{
    if (m_dirty)
        saveNow();
}

// ===================================================================== load
bool ButtonStore::readJsonFile(const QString &path, QJsonDocument *doc, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Couldn't open %1: %2").arg(path, f.errorString());
        return false;
    }
    if (f.size() > MaxFileBytes) {
        if (error)
            *error = QStringLiteral("%1 is too large to be a buttons file.").arg(path);
        return false;
    }
    QJsonParseError pe;
    *doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError) {
        if (error)
            *error = QStringLiteral("%1 isn't valid JSON (%2 at character %3).")
                         .arg(path, pe.errorString())
                         .arg(pe.offset);
        return false;
    }
    return true;
}

bool ButtonStore::parseDocument(const QJsonDocument &doc, QList<Page> *out,
                                QStringList *warnings, QString *error)
{
    out->clear();
    int total = 0;
    QSet<QString> ids;

    auto takeButtons = [&](const QJsonArray &arr, Page &page) {
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) {
                if (warnings)
                    warnings->append(QStringLiteral("Skipped an entry that wasn't a button."));
                continue;
            }
            if (total >= MaxButtons) {
                if (warnings)
                    warnings->append(QStringLiteral("Stopped at %1 buttons.").arg(MaxButtons));
                return;
            }
            ButtonSpec b = ButtonSpec::fromJson(v.toObject(), warnings);
            if (ids.contains(b.id))
                b.id = ButtonSpec::newId();   // duplicate ids would confuse selection
            ids.insert(b.id);
            page.buttons.append(b);
            ++total;
        }
    };

    if (doc.isArray()) {                                   // 1.x: a bare array
        Page p;
        p.name = kDefaultPage;
        takeButtons(doc.array(), p);
        out->append(p);
        return true;
    }
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("The file doesn't contain buttons.");
        return false;
    }
    const QJsonObject root = doc.object();
    const int version = root.value(QStringLiteral("version")).toInt(0);
    if (version > SchemaVersion && warnings)
        warnings->append(QStringLiteral("This file was written by a newer version; "
                                        "unknown settings were ignored."));
    const QJsonValue pagesValue = root.value(QStringLiteral("pages"));
    if (!pagesValue.isArray()) {
        if (error)
            *error = QStringLiteral("The file has no \"pages\" list.");
        return false;
    }
    QSet<QString> names;
    for (const QJsonValue &pv : pagesValue.toArray()) {
        if (!pv.isObject())
            continue;
        if (out->size() >= MaxPages) {
            if (warnings)
                warnings->append(QStringLiteral("Stopped at %1 pages.").arg(MaxPages));
            break;
        }
        const QJsonObject po = pv.toObject();
        Page p;
        p.name = cleanPageName(po.value(QStringLiteral("name")).toString());
        if (p.name.isEmpty())
            p.name = QStringLiteral("Page %1").arg(out->size() + 1);
        QString unique = p.name;
        for (int n = 2; names.contains(unique); ++n)
            unique = QStringLiteral("%1 (%2)").arg(p.name).arg(n);
        p.name = unique;
        names.insert(p.name);
        takeButtons(po.value(QStringLiteral("buttons")).toArray(), p);
        out->append(p);
    }
    if (out->isEmpty()) {
        Page p;
        p.name = kDefaultPage;
        out->append(p);
    }
    return true;
}

ButtonStore::LoadResult ButtonStore::load()
{
    LoadResult result;
    const QString path = AppPaths::buttonsFile();
    QString source = path;

    if (!QFile::exists(path)) {
        const QString legacy = AppPaths::legacyButtonsFile();
        if (!QFile::exists(legacy)) {
            m_pages.clear();
            ensureOnePage();
            emit changed();
            return result;                      // a fresh start
        }
        source = legacy;
        result.migrated = true;
    }

    QJsonDocument doc;
    QString err;
    QList<Page> pages;
    if (!readJsonFile(source, &doc, &err) || !parseDocument(doc, &pages, &result.warnings, &err)) {
        result.ok = false;
        if (source == path) {
            // Move the damaged file aside (never delete it) so the next save
            // can't overwrite whatever might still be rescued from it.
            const QString aside = QDir(AppPaths::backupDir())
                                      .filePath(QStringLiteral("buttons-unreadable-%1.json")
                                                    .arg(timestamp()));
            if (QFile::rename(path, aside)) {
                result.message = QStringLiteral("%1\n\nThe file was moved to:\n%2\n\n"
                                                "You can use File → Restore from backup… "
                                                "to bring back an earlier copy.")
                                     .arg(err, aside);
            } else {
                m_savingBlocked = true;
                result.message = QStringLiteral("%1\n\nIt couldn't be moved aside, so "
                                                "nothing will be saved this session to "
                                                "protect it.")
                                     .arg(err);
            }
        } else {
            result.message = err;
            result.migrated = false;
        }
        m_pages.clear();
        ensureOnePage();
        emit changed();
        return result;
    }

    m_pages = pages;
    ensureOnePage();
    m_undo.clear();
    m_redo.clear();
    if (result.migrated) {
        result.message = QStringLiteral("Your %1 buttons were copied from\n%2\ninto\n%3\n\n"
                                        "The old file was left exactly as it was.")
                             .arg(totalButtons())
                             .arg(source, path);
        m_dirty = true;
        saveNow();
    } else if (AppPaths::isExposed(path)) {
        AppPaths::makePrivateFile(path);
    }
    emit changed();
    emit undoStateChanged();
    return result;
}

// ===================================================================== save
QJsonDocument ButtonStore::toDocument() const
{
    return toDocument(m_pages);
}

QJsonDocument ButtonStore::toDocument(const QList<Page> &pages) const
{
    QJsonArray pageArray;
    for (const Page &p : pages) {
        QJsonArray buttons;
        for (const ButtonSpec &b : p.buttons)
            buttons.append(b.toJson());
        QJsonObject po;
        po.insert(QStringLiteral("name"), p.name);
        po.insert(QStringLiteral("buttons"), buttons);
        pageArray.append(po);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), SchemaVersion);
    root.insert(QStringLiteral("app"), QStringLiteral("CommandButtonCreator"));
    root.insert(QStringLiteral("pages"), pageArray);
    return QJsonDocument(root);
}

bool ButtonStore::saveNow(QString *error)
{
    m_saveTimer.stop();
    if (m_savingBlocked) {
        if (error)
            *error = QStringLiteral("Saving is switched off this session because the "
                                    "buttons file couldn't be read or moved aside.");
        return false;
    }
    const QString path = AppPaths::buttonsFile();
    if (!m_sessionBackupDone && QFile::exists(path)) {
        backupNow(QStringLiteral("session"));
        m_sessionBackupDone = true;
    }

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Couldn't save %1: %2").arg(path, f.errorString());
        return false;
    }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QByteArray bytes = toDocument().toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        if (error)
            *error = QStringLiteral("Couldn't save %1: %2").arg(path, f.errorString());
        return false;
    }
    AppPaths::makePrivateFile(path);
    m_dirty = false;
    return true;
}

void ButtonStore::scheduleSave()
{
    m_dirty = true;
    m_saveTimer.start();
}

// ================================================================= backups
bool ButtonStore::backupNow(const QString &reason, QString *error)
{
    const QString path = AppPaths::buttonsFile();
    if (!QFile::exists(path))
        return true;
    const QString dest = QDir(AppPaths::backupDir())
                             .filePath(QStringLiteral("buttons-%1-%2.json")
                                           .arg(timestamp(), reason));
    if (!QFile::copy(path, dest)) {
        if (error)
            *error = QStringLiteral("Couldn't make a backup copy at %1.").arg(dest);
        return false;
    }
    AppPaths::makePrivateFile(dest);
    pruneBackups();
    return true;
}

QStringList ButtonStore::backupFiles() const
{
    const QDir dir(AppPaths::backupDir());
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("buttons-*.json")},
                                                  QDir::Files, QDir::Time);
    QStringList out;
    for (const QFileInfo &fi : files)
        out.append(fi.absoluteFilePath());
    return out;
}

void ButtonStore::pruneBackups()
{
    const QStringList files = backupFiles();
    int kept = 0;
    for (const QString &f : files) {
        // Unreadable originals are evidence; never prune them automatically.
        if (QFileInfo(f).fileName().contains(QStringLiteral("-unreadable-")))
            continue;
        if (++kept > BackupKeep)
            QFile::remove(f);
    }
}

bool ButtonStore::restoreBackup(const QString &path, QString *error)
{
    QJsonDocument doc;
    QList<Page> pages;
    QStringList warnings;
    if (!readJsonFile(path, &doc, error) || !parseDocument(doc, &pages, &warnings, error))
        return false;
    backupNow(QStringLiteral("before-restore"));
    beginChange(QStringLiteral("Restore backup"));
    m_pages = pages;
    ensureOnePage();
    m_savingBlocked = false;
    commit();
    return true;
}

// ================================================================= reading
int ButtonStore::totalButtons() const
{
    int n = 0;
    for (const Page &p : m_pages)
        n += int(p.buttons.size());
    return n;
}

QList<ButtonSpec> ButtonStore::allButtons() const
{
    QList<ButtonSpec> out;
    for (const Page &p : m_pages)
        out += p.buttons;
    return out;
}

bool ButtonStore::locate(const QString &id, int *page, int *index) const
{
    for (int p = 0; p < m_pages.size(); ++p)
        for (int i = 0; i < m_pages[p].buttons.size(); ++i)
            if (m_pages[p].buttons[i].id == id) {
                *page = p;
                *index = i;
                return true;
            }
    return false;
}

const ButtonSpec *ButtonStore::find(const QString &id) const
{
    int p = 0, i = 0;
    return locate(id, &p, &i) ? &m_pages[p].buttons[i] : nullptr;
}

int ButtonStore::pageOf(const QString &id) const
{
    int p = -1, i = 0;
    return locate(id, &p, &i) ? p : -1;
}

bool ButtonStore::canAddButtons(int count) const
{
    return totalButtons() + count <= MaxButtons;
}

QString ButtonStore::uniquePageName(const QString &base) const
{
    QString clean = cleanPageName(base);
    if (clean.isEmpty())
        clean = QStringLiteral("Page");
    QString name = clean;
    auto taken = [this](const QString &n) {
        for (const Page &p : m_pages)
            if (p.name.compare(n, Qt::CaseInsensitive) == 0)
                return true;
        return false;
    };
    for (int n = 2; taken(name); ++n)
        name = QStringLiteral("%1 (%2)").arg(clean).arg(n);
    return name;
}

// ================================================================ changing
void ButtonStore::beginChange(const QString &what)
{
    m_undo.append({m_pages, what});
    while (m_undo.size() > UndoLimit)
        m_undo.removeFirst();
    m_redo.clear();
}

void ButtonStore::commit()
{
    ensureOnePage();
    scheduleSave();
    emit changed();
    emit undoStateChanged();
}

void ButtonStore::ensureOnePage()
{
    if (m_pages.isEmpty())
        m_pages.append(Page{kDefaultPage, {}});
}

void ButtonStore::addButton(int page, const ButtonSpec &b, const QString &what)
{
    addButtons(page, {b}, what);
}

void ButtonStore::addButtons(int page, const QList<ButtonSpec> &list, const QString &what)
{
    if (list.isEmpty() || !canAddButtons(int(list.size())))
        return;
    page = qBound(0, page, pageCount() - 1);
    beginChange(what);
    for (ButtonSpec copy : list) {
        copy.sanitize();
        if (find(copy.id))
            copy.id = ButtonSpec::newId();
        m_pages[page].buttons.append(copy);
    }
    commit();
}

void ButtonStore::updateButton(const ButtonSpec &b, const QString &what)
{
    updateButtons({b}, what);
}

void ButtonStore::updateButtons(const QList<ButtonSpec> &list, const QString &what)
{
    bool any = false;
    for (const ButtonSpec &b : list) {
        int p = 0, i = 0;
        if (!locate(b.id, &p, &i))
            continue;
        if (!any)
            beginChange(what);
        any = true;
        ButtonSpec copy = b;
        copy.sanitize();
        m_pages[p].buttons[i] = copy;
    }
    if (any)
        commit();
}

void ButtonStore::removeButtons(const QStringList &ids)
{
    if (ids.isEmpty())
        return;
    const QSet<QString> doomed(ids.begin(), ids.end());
    beginChange(ids.size() == 1 ? QStringLiteral("Delete button")
                                : QStringLiteral("Delete %1 buttons").arg(ids.size()));
    for (Page &p : m_pages)
        p.buttons.erase(std::remove_if(p.buttons.begin(), p.buttons.end(),
                                       [&](const ButtonSpec &b) { return doomed.contains(b.id); }),
                        p.buttons.end());
    commit();
}

void ButtonStore::setGeometries(const QHash<QString, QRect> &geometries, const QString &what)
{
    if (geometries.isEmpty())
        return;
    bool changedAny = false;
    for (const Page &p : m_pages)
        for (const ButtonSpec &b : p.buttons)
            if (geometries.contains(b.id) && geometries.value(b.id) != b.geometry)
                changedAny = true;
    if (!changedAny)
        return;
    beginChange(what);
    for (Page &p : m_pages)
        for (ButtonSpec &b : p.buttons)
            if (geometries.contains(b.id)) {
                b.geometry = geometries.value(b.id);
                b.sanitize();
            }
    commit();
}

void ButtonStore::moveButtonsToPage(const QStringList &ids, int page)
{
    if (ids.isEmpty() || page < 0 || page >= pageCount())
        return;
    const QSet<QString> moving(ids.begin(), ids.end());
    beginChange(QStringLiteral("Move to page \"%1\"").arg(m_pages[page].name));
    QList<ButtonSpec> taken;
    for (int p = 0; p < m_pages.size(); ++p) {
        if (p == page)
            continue;
        auto &list = m_pages[p].buttons;
        for (int i = int(list.size()) - 1; i >= 0; --i)
            if (moving.contains(list[i].id))
                taken.prepend(list.takeAt(i));
    }
    // Put them below whatever is already on the destination page.
    int bottom = 20;
    for (const ButtonSpec &b : m_pages[page].buttons)
        bottom = qMax(bottom, b.geometry.bottom() + 11);
    int minY = ButtonSpec::MaxCoord;
    for (const ButtonSpec &b : taken)
        minY = qMin(minY, b.geometry.y());
    for (ButtonSpec &b : taken) {
        b.geometry.moveTop(b.geometry.y() - minY + bottom);
        b.sanitize();
        m_pages[page].buttons.append(b);
    }
    commit();
}

int ButtonStore::addPage(const QString &name)
{
    if (pageCount() >= MaxPages)
        return -1;
    beginChange(QStringLiteral("New page"));
    m_pages.append(Page{uniquePageName(name), {}});
    commit();
    return pageCount() - 1;
}

void ButtonStore::renamePage(int index, const QString &name)
{
    if (index < 0 || index >= pageCount())
        return;
    const QString clean = cleanPageName(name);
    if (clean.isEmpty() || clean == m_pages[index].name)
        return;
    beginChange(QStringLiteral("Rename page"));
    m_pages[index].name = QStringLiteral("\x01");     // so it doesn't collide with itself
    m_pages[index].name = uniquePageName(clean);
    commit();
}

void ButtonStore::removePage(int index)
{
    if (index < 0 || index >= pageCount())
        return;
    beginChange(QStringLiteral("Delete page \"%1\"").arg(m_pages[index].name));
    m_pages.removeAt(index);
    commit();
}

void ButtonStore::movePage(int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= pageCount() || to >= pageCount())
        return;
    beginChange(QStringLiteral("Move page"));
    m_pages.move(from, to);
    commit();
}

int ButtonStore::appendPages(const QList<Page> &pages, const QString &what)
{
    int count = 0;
    for (const Page &p : pages)
        count += int(p.buttons.size());
    if (!canAddButtons(count) || pageCount() + pages.size() > MaxPages)
        return 0;
    beginChange(what);
    QSet<QString> ids;
    for (const Page &p : m_pages)
        for (const ButtonSpec &b : p.buttons)
            ids.insert(b.id);
    for (Page p : pages) {
        p.name = uniquePageName(p.name);
        for (ButtonSpec &b : p.buttons) {
            if (ids.contains(b.id))
                b.id = ButtonSpec::newId();
            ids.insert(b.id);
            b.sanitize();
        }
        m_pages.append(p);
    }
    commit();
    return count;
}

// ============================================================== undo/redo
QString ButtonStore::undoText() const
{
    return m_undo.isEmpty() ? QString() : m_undo.last().what;
}

QString ButtonStore::redoText() const
{
    return m_redo.isEmpty() ? QString() : m_redo.last().what;
}

void ButtonStore::undo()
{
    if (m_undo.isEmpty())
        return;
    Snapshot s = m_undo.takeLast();
    m_redo.append({m_pages, s.what});
    m_pages = s.pages;
    commit();
}

void ButtonStore::redo()
{
    if (m_redo.isEmpty())
        return;
    Snapshot s = m_redo.takeLast();
    m_undo.append({m_pages, s.what});
    m_pages = s.pages;
    commit();
}
