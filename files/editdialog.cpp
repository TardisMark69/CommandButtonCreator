#include "editdialog.h"
#include <QHBoxLayout>

EditDialog::EditDialog(const QString &buttonLabel, const QString &command, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Button");
    setMinimumSize(500, 350);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Button label section
    QLabel *labelLabel = new QLabel("Button Label:", this);
    mainLayout->addWidget(labelLabel);
    
    labelEdit = new QLineEdit(buttonLabel, this);
    mainLayout->addWidget(labelEdit);
    
    // Command section
    QLabel *commandLabel = new QLabel("Command (multiple lines supported):", this);
    mainLayout->addWidget(commandLabel);
    
    commandEdit = new QTextEdit(command, this);
    commandEdit->setMinimumHeight(150);
    mainLayout->addWidget(commandEdit);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *okButton = new QPushButton("OK", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString EditDialog::getButtonLabel() const
{
    return labelEdit->text();
}

QString EditDialog::getCommand() const
{
    return commandEdit->toPlainText();
}
