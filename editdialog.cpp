// editdialog.cpp — see editdialog.h.
#include "editdialog.h"

#include "appsettings.h"
#include "commandlinter.h"
#include "commandrunner.h"
#include "secretscanner.h"
#include "secretstore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <csignal>

namespace {

QIcon swatchIcon(const Swatch &s)
{
    QPixmap pm(18, 14);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, 0, 14);
    g.setColorAt(0, s.top);
    g.setColorAt(0.5, s.mid);
    g.setColorAt(1, s.bottom);
    p.setBrush(g);
    p.setPen(QPen(s.border, 1.5));
    p.drawRoundedRect(QRectF(1, 1, 16, 12), 3, 3);
    return QIcon(pm);
}

QFont monoFont(const QWidget *w)
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(w->font().pointSize());
    return f;
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

EditDialog::EditDialog(const ButtonSpec &spec, const QStringList &pageNames, int page,
                       SecretStore *secrets, bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_original(spec)
    , m_secrets(secrets)
    , m_isNew(isNew)
{
    setWindowTitle(isNew ? QStringLiteral("New Button") : QStringLiteral("Edit Button"));
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setSizeGripEnabled(true);

    // ---- label
    m_label = new QLineEdit(spec.label);
    m_label->setMaxLength(ButtonSpec::MaxLabel);
    m_label->setPlaceholderText(QStringLiteral("What the button says, e.g. Restart Ollama"));
    m_label->setToolTip(QStringLiteral("The text shown on the button. It wraps and shrinks "
                                       "to fit, so short is best."));

    // ---- mode
    m_mode = new QComboBox;
    for (RunMode m : RunModes::all())
        m_mode->addItem(QStringLiteral("%1   %2").arg(RunModes::glyph(m), RunModes::title(m)),
                        RunModes::key(m));
    m_mode->setCurrentIndex(m_mode->findData(RunModes::key(spec.mode)));
    m_mode->setToolTip(QStringLiteral(
        "<b>Terminal window</b> — for anything you type into: ssh, ollama run, sudo.<br>"
        "<b>Output panel</b> — runs quietly and shows the result inside this program.<br>"
        "<b>Background</b> — starts a server or service and leaves it running."));
    m_modeHint = new QLabel;
    m_modeHint->setObjectName(QStringLiteral("hint"));
    m_modeHint->setWordWrap(true);

    // ---- command
    m_command = new QPlainTextEdit(spec.command);
    m_command->setFont(monoFont(this));
    m_command->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_command->setTabChangesFocus(true);
    m_command->setPlaceholderText(QStringLiteral(
        "One or more lines of bash, exactly as you'd type them in a terminal.\n"
        "Use $NAME for a saved secret, e.g.  -H \"Authorization: Bearer $HA_TOKEN\""));
    m_command->setMinimumHeight(m_command->fontMetrics().lineSpacing() * 7 + 12);
    m_command->setToolTip(QStringLiteral("The command, run by bash. Several lines run one "
                                         "after another. Tab moves to the next field."));

    m_insertSecret = new QPushButton(QStringLiteral("🔑 Insert secret"));
    m_insertSecret->setAutoDefault(false);
    m_insertSecret->setToolTip(QStringLiteral("Put ${NAME} for a saved secret at the cursor, "
                                              "so a token never has to be pasted into a command."));
    m_insertSecret->setMenu(new QMenu(m_insertSecret));
    rebuildSecretMenu();

    m_test = new QPushButton(QStringLiteral("▶ Test run"));
    m_test->setAutoDefault(false);
    m_test->setToolTip(QStringLiteral("Run the command as it is now, without saving. "
                                      "Output-panel and Background results appear in the "
                                      "Output panel."));

    auto *commandTools = new QHBoxLayout;
    commandTools->setContentsMargins(0, 0, 0, 0);
    commandTools->addWidget(m_insertSecret);
    commandTools->addStretch();
    commandTools->addWidget(m_test);

    // ---- status check
    m_status = new QLineEdit(spec.statusCheck);
    m_status->setFont(monoFont(this));
    m_status->setMaxLength(ButtonSpec::MaxStatusCheck);
    m_status->setPlaceholderText(QStringLiteral("optional — e.g.  pgrep -x ollama"));
    m_status->setToolTip(QStringLiteral(
        "A short command that succeeds (exit code 0) when the thing this button controls is "
        "running. The button then shows a green light, or red when it fails. Leave empty "
        "for no light."));
    m_statusTry = new QPushButton(QStringLiteral("Try"));
    m_statusTry->setAutoDefault(false);
    m_statusTry->setToolTip(QStringLiteral("Run the status check once now."));
    m_statusResult = new QLabel;
    m_statusResult->setObjectName(QStringLiteral("hint"));
    auto *statusRow = new QHBoxLayout;
    statusRow->setContentsMargins(0, 0, 0, 0);
    statusRow->addWidget(m_status, 1);
    statusRow->addWidget(m_statusTry);
    statusRow->addWidget(m_statusResult);

    // ---- look and place
    m_color = new QComboBox;
    for (const Swatch &s : Swatches::all())
        m_color->addItem(swatchIcon(s), s.name, s.key);
    m_color->setCurrentIndex(m_color->findData(Swatches::byKey(spec.color).key));
    m_color->setToolTip(QStringLiteral("Color-code buttons by kind, e.g. green for "
                                       "lights, amber for restarts."));
    m_page = new QComboBox;
    m_page->addItems(pageNames);
    m_page->setCurrentIndex(qBound(0, page, int(pageNames.size()) - 1));
    m_page->setToolTip(QStringLiteral("Which page (tab) the button is on."));
    m_confirm = new QCheckBox(QStringLiteral("Ask before running"));
    m_confirm->setChecked(spec.confirm);
    m_confirm->setToolTip(QStringLiteral("Show a “Run …?” question first. Good for buttons "
                                         "you'd hate to press by accident."));
    auto *lookRow = new QHBoxLayout;
    lookRow->setContentsMargins(0, 0, 0, 0);
    lookRow->addWidget(new QLabel(QStringLiteral("Color:")));
    lookRow->addWidget(m_color);
    lookRow->addSpacing(10);
    lookRow->addWidget(new QLabel(QStringLiteral("Page:")));
    lookRow->addWidget(m_page);
    lookRow->addSpacing(10);
    lookRow->addWidget(m_confirm);
    lookRow->addStretch();

    // ---- checker
    m_lint = new QListWidget;
    m_lint->setObjectName(QStringLiteral("lintList"));
    m_lint->setWordWrap(true);
    m_lint->setSelectionMode(QAbstractItemView::NoSelection);
    m_lint->setFocusPolicy(Qt::NoFocus);
    m_lint->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lint->setResizeMode(QListView::Adjust);
    m_lint->setMinimumHeight(m_lint->fontMetrics().lineSpacing() * 4);
    m_lint->setToolTip(QStringLiteral("The command checker. It reads the command as you type "
                                      "and explains anything that won't work as it looks."));
    m_vault = new QPushButton(QStringLiteral("🔑 Move to vault"));
    m_vault->setAutoDefault(false);
    m_vault->setToolTip(QStringLiteral("Save the typed-in password or token in the secrets "
                                       "vault and replace it here with ${NAME}."));
    m_vault->hide();
    m_vaultNote = new QLabel;
    m_vaultNote->setObjectName(QStringLiteral("hint"));
    m_vaultNote->setWordWrap(true);
    auto *checkerHead = new QHBoxLayout;
    checkerHead->setContentsMargins(0, 0, 0, 0);
    checkerHead->addWidget(m_vaultNote, 1);
    checkerHead->addWidget(m_vault);

    // ---- form
    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setVerticalSpacing(5);
    form->addRow(QStringLiteral("Label:"), m_label);
    form->addRow(QStringLiteral("Runs in:"), m_mode);
    form->addRow(QString(), m_modeHint);
    form->addRow(QStringLiteral("Command:"), m_command);
    form->addRow(QString(), commandTools);
    form->addRow(QStringLiteral("Status light:"), statusRow);
    form->addRow(QString(), lookRow);
    form->addRow(QStringLiteral("Checker:"), m_lint);
    form->addRow(QString(), checkerHead);

    // ---- buttons
    auto *box = new QDialogButtonBox;
    QPushButton *save = box->addButton(QStringLiteral("Save"), QDialogButtonBox::AcceptRole);
    save->setDefault(true);
    box->addButton(QDialogButtonBox::Cancel);
    QPushButton *help = box->addButton(QDialogButtonBox::Help);
    help->setAutoDefault(false);
    if (!isNew) {
        QPushButton *del = box->addButton(QStringLiteral("Delete"), QDialogButtonBox::DestructiveRole);
        del->setObjectName(QStringLiteral("dangerButton"));
        del->setAutoDefault(false);
        del->setToolTip(QStringLiteral("Delete this button (Edit → Undo brings it back)."));
        connect(del, &QPushButton::clicked, this, [this] {
            m_delete = true;
            QDialog::accept();
        });
    }
    connect(box, &QDialogButtonBox::accepted, this, &EditDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &EditDialog::reject);
    connect(help, &QPushButton::clicked, this,
            [this] { emit helpRequested(QStringLiteral("editing")); });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 8);
    layout->addLayout(form, 1);
    layout->addWidget(box);

    // ---- behaviour
    m_lintTimer.setSingleShot(true);
    m_lintTimer.setInterval(350);
    connect(&m_lintTimer, &QTimer::timeout, this, &EditDialog::relint);
    connect(m_command, &QPlainTextEdit::textChanged, &m_lintTimer, qOverload<>(&QTimer::start));
    connect(m_status, &QLineEdit::textChanged, &m_lintTimer, qOverload<>(&QTimer::start));
    connect(m_mode, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        updateModeHint();
        relint();
    });
    connect(m_vault, &QPushButton::clicked, this, &EditDialog::moveSecretsToVault);
    connect(m_statusTry, &QPushButton::clicked, this, &EditDialog::tryStatusCheck);
    connect(m_test, &QPushButton::clicked, this, [this] { emit testRequested(this->spec()); });
    connect(secrets, &SecretStore::changed, this, [this] {
        rebuildSecretMenu();
        m_lintTimer.start();
    });

    updateModeHint();
    relint();

    const QByteArray geo = AppSettings::dialogGeometry(QStringLiteral("edit"));
    if (geo.isEmpty() || !restoreGeometry(geo))
        resize(620, 560);
    (isNew ? static_cast<QWidget *>(m_label) : m_command)->setFocus();
}

EditDialog::~EditDialog()
{
    if (m_statusProc) {
        m_statusProc->disconnect(this);
        ProcessUtil::signalGroup(m_statusProc, SIGKILL);
        m_statusProc->waitForFinished(1000);
    }
}

void EditDialog::done(int result)
{
    AppSettings::setDialogGeometry(QStringLiteral("edit"), saveGeometry());
    QDialog::done(result);
}

ButtonSpec EditDialog::spec() const
{
    ButtonSpec b = m_original;
    b.label = m_label->text().trimmed();
    b.command = m_command->toPlainText();
    b.mode = RunModes::fromKey(m_mode->currentData().toString());
    b.statusCheck = m_status->text().trimmed();
    b.color = m_color->currentData().toString();
    b.confirm = m_confirm->isChecked();
    b.sanitize();
    return b;
}

int EditDialog::page() const
{
    return m_page->currentIndex();
}

void EditDialog::updateModeHint()
{
    m_modeHint->setText(RunModes::summary(RunModes::fromKey(m_mode->currentData().toString())));
}

void EditDialog::relint()
{
    const ButtonSpec b = spec();
    QList<LintIssue> issues = CommandLinter::check(b.command, b.mode, *m_secrets);
    issues += CommandLinter::checkStatus(b.statusCheck, *m_secrets);

    m_lint->clear();
    bool secretFound = false;
    for (const LintIssue &i : issues) {
        QString glyph;
        QColor color;
        switch (i.level) {
        case LintIssue::Error:   glyph = QStringLiteral("✖"); color = QColor(0xff, 0x8a, 0x80); break;
        case LintIssue::Warning: glyph = QStringLiteral("⚠"); color = QColor(0xff, 0xd4, 0x7f); break;
        case LintIssue::Info:    glyph = QStringLiteral("ℹ"); color = QColor(0x9f, 0xd8, 0xff); break;
        }
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2").arg(glyph, i.text));
        item->setForeground(color);
        m_lint->addItem(item);
        if (i.code == QLatin1String("secret"))
            secretFound = true;
    }
    if (issues.isEmpty()) {
        auto *item = new QListWidgetItem(QStringLiteral("✓  No problems found."));
        item->setForeground(QColor(0x8f, 0xe3, 0xaa));
        m_lint->addItem(item);
    }
    m_vault->setVisible(secretFound);
}

void EditDialog::rebuildSecretMenu()
{
    QMenu *menu = m_insertSecret->menu();
    menu->clear();
    const QStringList names = m_secrets->names();
    for (const QString &n : names)
        addItem(menu,QStringLiteral("${%1}").arg(n), [this, n] { insertSecret(n); });
    if (names.isEmpty()) {
        QAction *none = menu->addAction(QStringLiteral("(the vault is empty — add one in "
                                                       "Tools → Secrets vault)"));
        none->setEnabled(false);
    }
}

void EditDialog::insertSecret(const QString &name)
{
    QTextCursor c = m_command->textCursor();
    const QString text = m_command->toPlainText();
    const SecretScanner::Quote q = SecretScanner::quoteStateAt(text, c.position());
    // Inside single quotes bash wouldn't fill it in, so step out and back in.
    c.insertText(q == SecretScanner::Quote::Single ? QStringLiteral("'\"${%1}\"'").arg(name)
                                                   : QStringLiteral("${%1}").arg(name));
    m_command->setTextCursor(c);
    m_command->setFocus();
}

void EditDialog::moveSecretsToVault()
{
    QString command = m_command->toPlainText();
    QString status = m_status->text();
    const QList<SecretScanner::Finding> found =
        SecretScanner::scan(command) + SecretScanner::scan(status);
    if (found.isEmpty())
        return;
    QString why;
    const QMap<QString, QString> filed = m_secrets->vault(found, &why);
    QStringList names;
    for (auto it = filed.cbegin(); it != filed.cend(); ++it) {
        command = SecretScanner::replaceValue(command, it.key(), it.value());
        status = SecretScanner::replaceValue(status, it.key(), it.value());
        if (!names.contains(it.value()))
            names.append(QStringLiteral("$") + it.value());
    }
    if (filed.size() < found.size() && !why.isEmpty())
        QMessageBox::warning(this, windowTitle(), why);
    m_command->setPlainText(command);
    m_status->setText(status);
    if (!names.isEmpty())
        m_vaultNote->setText(QStringLiteral("Saved in the vault as %1. The command now uses "
                                            "the name instead of the value.")
                                 .arg(names.join(QStringLiteral(", "))));
    relint();
}

void EditDialog::tryStatusCheck()
{
    const QString check = m_status->text().trimmed();
    if (check.isEmpty()) {
        m_statusResult->setText(QStringLiteral("(no check)"));
        return;
    }
    if (m_statusProc) {
        m_statusProc->disconnect(this);
        ProcessUtil::signalGroup(m_statusProc, SIGKILL);
        m_statusProc->waitForFinished(1000);
        m_statusProc->deleteLater();
    }
    auto *p = new QProcess(this);
    m_statusProc = p;
    ProcessUtil::prepareBash(p, check, m_secrets->environmentFor({check}));
    p->setProcessChannelMode(QProcess::MergedChannels);
    m_statusResult->setText(QStringLiteral("checking…"));
    QTimer::singleShot(8000, p, [p] { ProcessUtil::signalGroup(p, SIGKILL); });
    connect(p, &QProcess::finished, this, [this, p](int code, QProcess::ExitStatus st) {
        const bool up = st == QProcess::NormalExit && code == 0;
        m_statusResult->setText(up ? QStringLiteral("<span style='color:#8fe3aa'>● green "
                                                    "(exit 0)</span>")
                                   : QStringLiteral("<span style='color:#ff8a80'>● red "
                                                    "(%1)</span>")
                                         .arg(st == QProcess::NormalExit
                                                  ? QStringLiteral("exit %1").arg(code)
                                                  : QStringLiteral("stopped")));
        p->deleteLater();
    });
    p->start();
}

void EditDialog::accept()
{
    const ButtonSpec b = spec();
    if (b.label.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Please give the button a label."));
        m_label->setFocus();
        return;
    }
    if (b.command.trimmed().isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Please enter a command."));
        m_command->setFocus();
        return;
    }
    if (!SecretScanner::scan(b.command + QLatin1Char('\n') + b.statusCheck).isEmpty()) {
        QMessageBox q(QMessageBox::Question, windowTitle(),
                      QStringLiteral("This command has a password or token typed into it."),
                      QMessageBox::NoButton, this);
        q.setInformativeText(QStringLiteral("Move it into the secrets vault before saving? "
                                            "Anyone who can see the buttons file or your "
                                            "screen could otherwise copy it."));
        QPushButton *move = q.addButton(QStringLiteral("Move to vault and save"), QMessageBox::AcceptRole);
        q.addButton(QStringLiteral("Save as it is"), QMessageBox::DestructiveRole);
        QPushButton *cancel = q.addButton(QMessageBox::Cancel);
        q.setDefaultButton(move);
        q.exec();
        if (q.clickedButton() == cancel)
            return;
        if (q.clickedButton() == move)
            moveSecretsToVault();
    }
    const QString syntax = CommandLinter::syntaxError(m_command->toPlainText());
    if (!syntax.isEmpty()
        && QMessageBox::question(this, windowTitle(),
                                 QStringLiteral("%1\n\nSave it anyway?").arg(syntax))
               != QMessageBox::Yes)
        return;
    QDialog::accept();
}
