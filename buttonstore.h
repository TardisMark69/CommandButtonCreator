// buttonstore.h — the buttons and pages, their file, undo/redo and backups.
//
// This is the only class that reads or writes buttons.json. The window
// never edits buttons directly: it asks the store, the store records an undo
// step, changes its data, emits changed(), and saves a moment later.
//
// Saving is atomic (QSaveFile: write a temp file, then rename), so a crash
// or full disk can never leave a half-written buttons.json. The first save
// of each session copies the previous file into backups/ (20 kept).
#ifndef BUTTONSTORE_H
#define BUTTONSTORE_H

#include "buttonspec.h"

#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

struct Page {
    QString name;
    QList<ButtonSpec> buttons;
};

class ButtonStore : public QObject
{
    Q_OBJECT
public:
    static constexpr int SchemaVersion = 2;
    static constexpr int MaxButtons    = 2000;
    static constexpr int MaxPages      = 100;
    static constexpr int MaxPageName   = 60;
    static constexpr int UndoLimit     = 100;
    static constexpr int BackupKeep    = 20;
    static constexpr qint64 MaxFileBytes = 16 * 1024 * 1024;

    explicit ButtonStore(QObject *parent = nullptr);
    ~ButtonStore() override;

    struct LoadResult {
        bool ok = true;            // false: file unreadable; started empty
        bool migrated = false;     // imported from the 1.x file
        QString message;           // for the user, when there is something to say
        QStringList warnings;
    };
    LoadResult load();
    bool saveNow(QString *error = nullptr);

    // ---- reading
    const QList<Page> &pages() const { return m_pages; }
    int pageCount() const { return int(m_pages.size()); }
    int totalButtons() const;
    QList<ButtonSpec> allButtons() const;
    const ButtonSpec *find(const QString &id) const;
    int pageOf(const QString &id) const;

    // ---- changing (each is one undo step)
    void addButton(int page, const ButtonSpec &b, const QString &what);
    void addButtons(int page, const QList<ButtonSpec> &list, const QString &what);
    void updateButton(const ButtonSpec &b, const QString &what);
    void updateButtons(const QList<ButtonSpec> &list, const QString &what);
    void removeButtons(const QStringList &ids);
    void setGeometries(const QHash<QString, QRect> &geometries, const QString &what);
    void moveButtonsToPage(const QStringList &ids, int page);
    int  addPage(const QString &name);
    void renamePage(int index, const QString &name);
    void removePage(int index);
    void movePage(int from, int to);
    int  appendPages(const QList<Page> &pages, const QString &what);   // import
    bool canAddButtons(int count) const;

    // ---- undo / redo
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    QString undoText() const;
    QString redoText() const;
    void undo();
    void redo();

    // ---- documents and backups
    QJsonDocument toDocument() const;
    QJsonDocument toDocument(const QList<Page> &pages) const;
    // Parses either the 1.x array or the 2.x object. Untrusted input.
    static bool parseDocument(const QJsonDocument &doc, QList<Page> *out,
                              QStringList *warnings, QString *error);
    static bool readJsonFile(const QString &path, QJsonDocument *doc, QString *error);

    QStringList backupFiles() const;          // newest first
    bool backupNow(const QString &reason, QString *error = nullptr);
    bool restoreBackup(const QString &path, QString *error);

    QString uniquePageName(const QString &base) const;

signals:
    void changed();
    void undoStateChanged();
    void saveFailed(const QString &message);

private:
    struct Snapshot {
        QList<Page> pages;
        QString what;
    };

    void beginChange(const QString &what);
    void commit();
    void scheduleSave();
    void pruneBackups();
    void ensureOnePage();
    bool locate(const QString &id, int *page, int *index) const;

    QList<Page> m_pages;
    QList<Snapshot> m_undo;
    QList<Snapshot> m_redo;
    QTimer m_saveTimer;
    bool m_dirty = false;
    bool m_sessionBackupDone = false;
    bool m_savingBlocked = false;   // set if the file on disk couldn't be read
};

#endif // BUTTONSTORE_H
