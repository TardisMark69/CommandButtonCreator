// preferencesdialog.cpp — see preferencesdialog.h.
#include "preferencesdialog.h"

#include "appsettings.h"
#include "terminallauncher.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Preferences"));

    m_terminal = new QComboBox;
    const QList<TerminalLauncher::Terminal> found = TerminalLauncher::detected();
    m_terminal->addItem(found.isEmpty()
                            ? QStringLiteral("Automatic (none found!)")
                            : QStringLiteral("Automatic (%1)").arg(found.first().name),
                        QStringLiteral("auto"));
    for (const TerminalLauncher::Terminal &t : found)
        m_terminal->addItem(t.name, t.id);
    const int at = m_terminal->findData(AppSettings::terminal());
    m_terminal->setCurrentIndex(at < 0 ? 0 : at);
    m_terminal->setToolTip(QStringLiteral("The terminal program used by Terminal-window "
                                          "buttons. Only programs installed here are listed."));

    m_interval = new QSpinBox;
    m_interval->setRange(10, 600);
    m_interval->setSingleStep(10);
    m_interval->setSuffix(QStringLiteral(" seconds"));
    m_interval->setValue(AppSettings::statusIntervalSec());
    m_interval->setToolTip(QStringLiteral("How often the status lights are re-checked. They "
                                          "are also checked a few seconds after a button runs, "
                                          "and paused while the window is minimized."));

    m_autoOutput = new QCheckBox(QStringLiteral("Open the Output panel when an Output-panel "
                                                "button runs"));
    m_autoOutput->setChecked(AppSettings::autoShowOutput());
    m_autoOutput->setToolTip(QStringLiteral("If off, open it yourself with View → Output "
                                            "panel (F9)."));

    m_snap = new QCheckBox(QStringLiteral("Snap buttons to a 10-pixel grid when moving "
                                          "or resizing"));
    m_snap->setChecked(AppSettings::snapToGrid());
    m_snap->setToolTip(QStringLiteral("Keeps rows and columns lined up. Turn off for "
                                      "pixel-exact placement."));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Terminal:"), m_terminal);
    form->addRow(QStringLiteral("Status lights every:"), m_interval);
    form->addRow(m_autoOutput);
    form->addRow(m_snap);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel
                                     | QDialogButtonBox::Help);
    connect(box, &QDialogButtonBox::accepted, this, [this] {
        save();
        accept();
    });
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(box, &QDialogButtonBox::helpRequested, this,
            [this] { emit helpRequested(QStringLiteral("preferences")); });

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(box);
}

void PreferencesDialog::save()
{
    AppSettings::setTerminal(m_terminal->currentData().toString());
    AppSettings::setStatusIntervalSec(m_interval->value());
    AppSettings::setAutoShowOutput(m_autoOutput->isChecked());
    AppSettings::setSnapToGrid(m_snap->isChecked());
}
