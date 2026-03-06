#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QJsonObject>
#include "commandbutton.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addNewButton();
    void saveButtons();
    void loadButtons();
    void editButton();
    void executeCommand(const QString &command);

private:
    QWidget *centralWidget;
    QWidget *canvas;
    QList<CommandButton*> buttons;
    
    void setupUI();
    void createButton(const QString &label, const QString &command, const QPoint &pos, const QSize &size);
    QJsonObject buttonToJson(CommandButton *button) const;
    void loadFromJson(const QString &filename);
    void saveToJson(const QString &filename);
};

#endif // MAINWINDOW_H
