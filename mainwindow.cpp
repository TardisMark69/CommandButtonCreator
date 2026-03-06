#include "mainwindow.h"
#include "editdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    // Load buttons from the executable's directory
    QString exeDir = QCoreApplication::applicationDirPath();
    loadFromJson(exeDir + "/buttons.json");
}

MainWindow::~MainWindow()
{
    // Auto-save buttons when closing to executable's directory
    QString exeDir = QCoreApplication::applicationDirPath();
    saveToJson(exeDir + "/buttons.json");
}

void MainWindow::setupUI()
{
    setWindowTitle("Command Button Creator");
    resize(1000, 700);
    
    // Create menu bar
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About", 
            "Command Button Creator\n\nCreated by Mark Mcdougal\nMarch 5, 2026");
    });
    
    // Create toolbar
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    QPushButton *addBtn = new QPushButton("Add Button", this);
    toolbar->addWidget(addBtn);
    
    QPushButton *organizeBtn = new QPushButton("Organize", this);
    toolbar->addWidget(organizeBtn);
    
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addNewButton);
    connect(organizeBtn, &QPushButton::clicked, this, &MainWindow::organizeButtons);
    
    // Create canvas
    canvas = new QWidget(this);
    canvas->setStyleSheet("background-color: #20B2AA;");  // Lighter turquoise (LightSeaGreen)
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
    int result = dialog.exec();
    
    if (result == QDialog::Accepted) {
        btn->setButtonLabel(dialog.getButtonLabel());
        btn->setCommandText(dialog.getCommand());
    } else if (result == 2) {  // Delete was clicked
        // Remove from list
        buttons.removeOne(btn);
        // Delete the button widget
        btn->deleteLater();
    }
}

void MainWindow::organizeButtons()
{
    const int MARGIN = 20;
    const int SPACING = 10;
    int x = MARGIN;
    int y = MARGIN;
    int maxHeightInRow = 0;
    int canvasWidth = canvas->width();
    
    for (CommandButton *btn : buttons) {
        // Check if button fits in current row
        if (x + btn->width() > canvasWidth - MARGIN && x > MARGIN) {
            // Move to next row
            x = MARGIN;
            y += maxHeightInRow + SPACING;
            maxHeightInRow = 0;
        }
        
        // Position button
        btn->move(x, y);
        
        // Update for next button
        x += btn->width() + SPACING;
        maxHeightInRow = qMax(maxHeightInRow, btn->height());
    }
}

void MainWindow::executeCommand(const QString &command)
{
    if (command.trimmed().isEmpty()) {
        QMessageBox::information(this, "No Command", 
                                "No command has been set for this button.");
        return;
    }
    
    // Open a terminal window and run the command
    QProcess *process = new QProcess(this);
    
    // Use gnome-terminal (common on Pop!_OS/Ubuntu)
    // The command will run in a visible terminal window
    QStringList args;
    args << "--" << "bash" << "-c" << command + "; echo 'Press Enter to close...'; read";
    
    process->startDetached("gnome-terminal", args);
    process->deleteLater();
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

void MainWindow::saveToJson(const QString &filename)
{
    QJsonArray array;
    for (CommandButton *btn : buttons) {
        array.append(buttonToJson(btn));
    }
    
    QJsonDocument doc(array);
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not save to file:" << filename;
        return;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::loadFromJson(const QString &filename)
{
    QFile file(filename);
    
    if (!file.exists()) {
        return; // No file to load, silently return
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file:" << filename;
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid file format";
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
}
