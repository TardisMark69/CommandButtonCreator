// secretsdialog.cpp — see secretsdialog.h.
#include "secretsdialog.h"

#include "appsettings.h"
#include "secretstore.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QVBoxLayout>

// ===================================================== SecretEditDialog
SecretEditDialog::SecretEditDialog(const QString &title, const QString &name, bool nameEditable,
                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    m_name = new QLineEdit(name);
    m_name->setReadOnly(!nameEditable);
    m_name->setMaxLength(SecretStore::MaxName);
    m_name->setPlaceholderText(QStringLiteral("e.g. HA_TOKEN"));
    m_name->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*")), m_name));
    m_name->setToolTip(QStringLiteral("The name you write in commands as $NAME. Capital "
                                      "letters, digits and _ (typed letters become capitals)."));
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &t) {
        const int pos = m_name->cursorPosition();
        m_name->setText(t.toUpper());
        m_name->setCursorPosition(pos);
    });

    m_value = new QLineEdit;
    m_value->setEchoMode(QLineEdit::Password);
    m_value->setMaxLength(SecretStore::MaxValue);
    m_value->setPlaceholderText(QStringLiteral("paste the password or token here"));
    m_value->setToolTip(QStringLiteral("The secret itself. It's saved only in the vault file, "
                                       "readable by you alone."));
    auto *show = new QCheckBox(QStringLiteral("Show"));
    connect(show, &QCheckBox::toggled, this, [this](bool on) {
        m_value->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
    });
    auto *valueRow = new QHBoxLayout;
    valueRow->addWidget(m_value, 1);
    valueRow->addWidget(show);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Name:"), m_name);
    form->addRow(QStringLiteral("Value:"), valueRow);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, this, [this] {
        QString why;
        if (!SecretStore::isValidName(this->name(), &why) || !SecretStore::isValidValue(this->value(), &why)) {
            QMessageBox::information(this, windowTitle(), why);
            return;
        }
        accept();
    });
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(box);
    resize(460, sizeHint().height());
    (nameEditable ? m_name : m_value)->setFocus();
}

QString SecretEditDialog::name() const { return m_name->text().trimmed().toUpper(); }
QString SecretEditDialog::value() const { return m_value->text(); }

// ========================================================= SecretsDialog
SecretsDialog::SecretsDialog(SecretStore *secrets, ButtonsFn buttons, QWidget *parent)
    : QDialog(parent)
    , m_secrets(secrets)
    , m_buttons(std::move(buttons))
{
    setWindowTitle(QStringLiteral("Secrets Vault"));
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setSizeGripEnabled(true);

    auto *intro = new QLabel(QStringLiteral(
        "Passwords and tokens live here instead of inside button commands. In a command, "
        "write <b>$NAME</b> (for example <code>Bearer $HA_TOKEN</code>); the value is handed "
        "to the command privately when the button runs, and blanked out of any output."));
    intro->setWordWrap(true);
    intro->setTextFormat(Qt::RichText);

    m_table = new QTableWidget(0, 3);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Name"), QStringLiteral("Value"), QStringLiteral("Used by")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setToolTip(QStringLiteral("Double-click a secret to change its value."));

    m_show = new QCheckBox(QStringLiteral("Show values"));
    m_show->setToolTip(QStringLiteral("Show the secrets themselves. Untick before anyone "
                                      "looks over your shoulder."));
    connect(m_show, &QCheckBox::toggled, this, &SecretsDialog::reload);

    auto *add = new QPushButton(QStringLiteral("Add…"));
    m_change = new QPushButton(QStringLiteral("Change value…"));
    m_rename = new QPushButton(QStringLiteral("Rename…"));
    m_delete = new QPushButton(QStringLiteral("Delete"));
    m_delete->setObjectName(QStringLiteral("dangerButton"));
    auto *check = new QPushButton(QStringLiteral("Security check…"));
    add->setToolTip(QStringLiteral("Save a new secret."));
    m_change->setToolTip(QStringLiteral("Replace the value, e.g. after making a new token."));
    m_rename->setToolTip(QStringLiteral("Rename it — every button using it is updated too."));
    m_delete->setToolTip(QStringLiteral("Remove it from the vault."));
    check->setToolTip(QStringLiteral("Look through all buttons for passwords or tokens typed "
                                     "into commands, and move them in here."));
    for (QPushButton *b : {add, m_change, m_rename, m_delete, check})
        b->setAutoDefault(false);

    auto *side = new QVBoxLayout;
    side->addWidget(add);
    side->addWidget(m_change);
    side->addWidget(m_rename);
    side->addWidget(m_delete);
    side->addSpacing(12);
    side->addWidget(check);
    side->addStretch();

    auto *middle = new QHBoxLayout;
    middle->addWidget(m_table, 1);
    middle->addLayout(side);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(box, &QDialogButtonBox::helpRequested, this,
            [this] { emit helpRequested(QStringLiteral("secrets")); });

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addLayout(middle, 1);
    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_show);
    bottom->addStretch();
    bottom->addWidget(box);
    layout->addLayout(bottom);

    connect(add, &QPushButton::clicked, this, &SecretsDialog::addSecret);
    connect(m_change, &QPushButton::clicked, this, &SecretsDialog::changeValue);
    connect(m_rename, &QPushButton::clicked, this, &SecretsDialog::renameSecret);
    connect(m_delete, &QPushButton::clicked, this, &SecretsDialog::deleteSecret);
    connect(check, &QPushButton::clicked, this, [this] { emit securityCheckRequested(); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SecretsDialog::changeValue);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const bool any = !currentName().isEmpty();
        m_change->setEnabled(any);
        m_rename->setEnabled(any);
        m_delete->setEnabled(any);
    });
    connect(secrets, &SecretStore::changed, this, &SecretsDialog::reload);

    reload();
    const QByteArray geo = AppSettings::dialogGeometry(QStringLiteral("secrets"));
    if (geo.isEmpty() || !restoreGeometry(geo))
        resize(640, 380);
}

void SecretsDialog::done(int result)
{
    AppSettings::setDialogGeometry(QStringLiteral("secrets"), saveGeometry());
    QDialog::done(result);
}

QStringList SecretsDialog::usedBy(const QString &name) const
{
    QStringList out;
    for (const ButtonSpec &b : m_buttons())
        if (SecretStore::variableReferences(b.command + QLatin1Char('\n') + b.statusCheck)
                .contains(name))
            out.append(b.label);
    return out;
}

void SecretsDialog::reload()
{
    const QString keep = currentName();
    const QStringList names = m_secrets->names();
    m_table->setRowCount(int(names.size()));
    for (int r = 0; r < names.size(); ++r) {
        const QString &n = names.at(r);
        const QString v = m_secrets->value(n);
        auto *nameItem = new QTableWidgetItem(QStringLiteral("$") + n);
        nameItem->setData(Qt::UserRole, n);
        m_table->setItem(r, 0, nameItem);
        m_table->setItem(r, 1, new QTableWidgetItem(
                                   m_show->isChecked() ? v
                                                       : QStringLiteral("•••••••• (%1 characters)")
                                                             .arg(v.size())));
        const QStringList users = usedBy(n);
        auto *usedItem = new QTableWidgetItem(users.isEmpty() ? QStringLiteral("(no buttons)")
                                                              : users.join(QStringLiteral(", ")));
        usedItem->setToolTip(users.join(QLatin1Char('\n')));
        m_table->setItem(r, 2, usedItem);
        if (n == keep)
            m_table->selectRow(r);
    }
    const bool any = !currentName().isEmpty();
    m_change->setEnabled(any);
    m_rename->setEnabled(any);
    m_delete->setEnabled(any);
}

QString SecretsDialog::currentName() const
{
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0) || !m_table->item(row, 0)->isSelected())
        return QString();
    return m_table->item(row, 0)->data(Qt::UserRole).toString();
}

void SecretsDialog::addSecret()
{
    SecretEditDialog d(QStringLiteral("Add Secret"), QString(), true, this);
    if (d.exec() != QDialog::Accepted)
        return;
    if (m_secrets->contains(d.name())
        && QMessageBox::question(this, windowTitle(),
                                 QStringLiteral("%1 already exists. Replace its value?").arg(d.name()))
               != QMessageBox::Yes)
        return;
    QString why;
    if (!m_secrets->set(d.name(), d.value(), &why))
        QMessageBox::warning(this, windowTitle(), why);
}

void SecretsDialog::changeValue()
{
    const QString name = currentName();
    if (name.isEmpty())
        return;
    SecretEditDialog d(QStringLiteral("Change Secret"), name, false, this);
    if (d.exec() != QDialog::Accepted)
        return;
    QString why;
    if (!m_secrets->set(name, d.value(), &why))
        QMessageBox::warning(this, windowTitle(), why);
}

void SecretsDialog::renameSecret()
{
    const QString name = currentName();
    if (name.isEmpty())
        return;
    bool ok = false;
    const QString newName = QInputDialog::getText(this, QStringLiteral("Rename Secret"),
                                                  QStringLiteral("New name for $%1:").arg(name),
                                                  QLineEdit::Normal, name, &ok)
                                .trimmed()
                                .toUpper();
    if (!ok || newName == name)
        return;
    QString why;
    if (!m_secrets->rename(name, newName, &why)) {
        QMessageBox::warning(this, windowTitle(), why);
        return;
    }
    emit renamed(name, newName);
    reload();
}

void SecretsDialog::deleteSecret()
{
    const QString name = currentName();
    if (name.isEmpty())
        return;
    const QStringList users = usedBy(name);
    QString text = QStringLiteral("Delete $%1 from the vault?").arg(name);
    if (!users.isEmpty())
        text += QStringLiteral("\n\nThese buttons use it and would get an empty value:\n• %1")
                    .arg(users.join(QStringLiteral("\n• ")));
    if (QMessageBox::question(this, windowTitle(), text) != QMessageBox::Yes)
        return;
    m_secrets->remove(name);
}
