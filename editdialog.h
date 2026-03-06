#ifndef EDITDIALOG_H
#define EDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class EditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditDialog(const QString &buttonLabel, const QString &command, QWidget *parent = nullptr);
    
    QString getButtonLabel() const;
    QString getCommand() const;

private:
    QLineEdit *labelEdit;
    QTextEdit *commandEdit;
};

#endif // EDITDIALOG_H
