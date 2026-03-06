#include "mainwindow.h"
#include "editdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    loadButtons();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Command Button Creator");
    resize(1000, 700);
    
    // Create toolbar
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    QPushButton *addBtn = new QPushButton("Add Button", this);
    QPushButton *saveBtn = new QPushButton("Save", this);
    QPushButton *loadBtn = new QPushButton("Load", this);
    
    toolbar->addWidget(addBtn);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(loadBtn);
    
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addNewButton);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveButtons);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadButtons);
    
    // Create canvas
    canvas = new QWidget(this);
    canvas->setStyleSheet("background-color: white;");
    canvas->setMinimumSize(800, 600);
    
    setCentralWidget(canvas);
}

void MainWindow::addNewButton()
{
    createButton("New Button", "", QPoint(50, 50), QSize(120, 40));
}

void MainWindow::createButton(const QString &label, const QString &command, 
                              const QPoint &pos, const QSize &size)
{
    CommandButton *btn = new CommandButton(canvas);
    btn->setButtonLabel(label);
    btn->setCommandText(command);
    btn->move(pos);
    btn->resize(size);
    btn->show();
    
    connect(btn, &CommandButton::rightClicked, this, &MainWindow::editButton);
    connect(btn, &CommandButton::executeCommand, this, &MainWindow::executeCommand);
    
    buttons.append(btn);
}

void MainWindow::editButton()
{
    CommandButton *btn = qobject_cast<CommandButton*>(sender());
    if (!btn) return;
    
    EditDialog dialog(btn->getButtonLabel(), btn->getCommandText(), this);
    if (dialog.exec() == QDialog::Accepted) {
        btn->setButtonLabel(dialog.getButtonLabel());
        btn->setCommandText(dialog.getCommand());
    }
}

void MainWindow::executeCommand(const QString &command)
{
    if (command.trimmed().isEmpty()) {
        QMessageBox::information(this, "No Command", 
                                "No command has been set for this button.");
        return;
    }
    
    // Create a bash script to execute the commands
    QString script = "#!/bin/bash\n" + command;
    
    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    // Execute using bash
    process->start("bash", QStringList() << "-c" << command);
    
    // Wait for the process to finish (with 5 second timeout)
    if (!process->waitForFinished(5000)) {
        process->kill();
        QMessageBox::warning(this, "Timeout", 
                           "Command timed out after 5 seconds and was terminated.");
        process->deleteLater();
        return;
    }
    
    QString output = process->readAllStandardOutput();
    QString error = process->readAllStandardError();
    int exitCode = process->exitCode();
    
    process->deleteLater();
    
    // Show result if there's output or error
    if (!output.isEmpty() || !error.isEmpty() || exitCode != 0) {
        QString message;
        if (!output.isEmpty()) {
            message += "Output:\n" + output + "\n";
        }
        if (!error.isEmpty()) {
            message += "\nErrors:\n" + error;
        }
        if (exitCode != 0) {
            message += "\n\nExit code: " + QString::number(exitCode);
        }
        
        QMessageBox::information(this, "Command Result", message);
    }
}

QJsonObject MainWindow::buttonToJson(CommandButton *button) const
{
    QJsonObject obj;
    obj["label"] = button->getButtonLabel();
    obj["command"] = button->getCommandText();
    obj["x"] = button->x();
    obj["y"] = button->y();
    obj["width"] = button->width();
    obj["height"] = button->height();
    return obj;
}

void MainWindow::saveButtons()
{
    saveToJson("buttons.json");
}

void MainWindow::loadButtons()
{
    loadFromJson("buttons.json");
}

void MainWindow::saveToJson(const QString &filename)
{
    QJsonArray array;
    for (CommandButton *btn : buttons) {
        array.append(buttonToJson(btn));
    }
    
    QJsonDocument doc(array);
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save Error", 
                           "Could not save to file: " + filename);
        return;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    QMessageBox::information(this, "Save", "Buttons saved successfully!");
}

void MainWindow::loadFromJson(const QString &filename)
{
    QFile file(filename);
    
    if (!file.exists()) {
        return; // No file to load, silently return
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Load Error", 
                           "Could not open file: " + filename);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Load Error", "Invalid file format");
        return;
    }
    
    // Clear existing buttons
    for (CommandButton *btn : buttons) {
        btn->deleteLater();
    }
    buttons.clear();
    
    // Load buttons from JSON
    QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        QString label = obj["label"].toString();
        QString command = obj["command"].toString();
        int x = obj["x"].toInt();
        int y = obj["y"].toInt();
        int width = obj["width"].toInt();
        int height = obj["height"].toInt();
        
        createButton(label, command, QPoint(x, y), QSize(width, height));
    }
    
    QMessageBox::information(this, "Load", "Buttons loaded successfully!");
}
