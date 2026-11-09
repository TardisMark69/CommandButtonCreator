// preferencesdialog.h — Tools → Preferences: which terminal to use, how
// often the status lights are checked, whether the Output panel opens by
// itself, and snapping to the grid. Saved when OK is pressed.
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

signals:
    void helpRequested(const QString &topic);

private:
    void save();

    QComboBox *m_terminal;
    QSpinBox *m_interval;
    QCheckBox *m_autoOutput;
    QCheckBox *m_snap;
};

#endif // PREFERENCESDIALOG_H
