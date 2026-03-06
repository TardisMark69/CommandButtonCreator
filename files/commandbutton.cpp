#include "commandbutton.h"
#include <QPainter>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>

CommandButton::CommandButton(QWidget *parent)
    : QPushButton(parent)
    , isDragging(false)
    , isResizing(false)
{
    setMinimumSize(50, 30);
    setCursor(Qt::SizeAllCursor);
    setStyleSheet("background-color: lightblue; border: 1px solid gray;");
}

void CommandButton::setCommandText(const QString &text)
{
    commandText = text;
}

QString CommandButton::getCommandText() const
{
    return commandText;
}

void CommandButton::setButtonLabel(const QString &label)
{
    setText(label);
}

QString CommandButton::getButtonLabel() const
{
    return text();
}

bool CommandButton::isInResizeHandle(const QPoint &pos) const
{
    QRect resizeRect(width() - RESIZE_HANDLE_SIZE, 
                     height() - RESIZE_HANDLE_SIZE,
                     RESIZE_HANDLE_SIZE, 
                     RESIZE_HANDLE_SIZE);
    return resizeRect.contains(pos);
}

void CommandButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (isInResizeHandle(event->pos())) {
            isResizing = true;
            resizeStartPosition = event->globalPos();
            resizeStartSize = size();
            event->accept();
        } else {
            isDragging = true;
            dragStartPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked();
        event->accept();
    } else {
        QPushButton::mousePressEvent(event);
    }
}

void CommandButton::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - dragStartPosition);
        event->accept();
    } else if (isResizing && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPos() - resizeStartPosition;
        int newWidth = qMax(minimumWidth(), resizeStartSize.width() + delta.x());
        int newHeight = qMax(minimumHeight(), resizeStartSize.height() + delta.y());
        resize(newWidth, newHeight);
        event->accept();
    } else {
        // Update cursor based on position
        if (isInResizeHandle(event->pos())) {
            setCursor(Qt::SizeFDiagCursor);
        } else {
            setCursor(Qt::SizeAllCursor);
        }
        QPushButton::mouseMoveEvent(event);
    }
}

void CommandButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!isDragging && !isResizing && !commandText.isEmpty()) {
            // This was a click, not a drag or resize - execute command
            emit executeCommand(commandText);
        }
        isDragging = false;
        isResizing = false;
        event->accept();
    } else {
        QPushButton::mouseReleaseEvent(event);
    }
}

void CommandButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    
    // Draw resize handle indicator
    QPainter painter(this);
    painter.setPen(QPen(Qt::gray, 2));
    int x = width() - RESIZE_HANDLE_SIZE;
    int y = height() - RESIZE_HANDLE_SIZE;
    painter.drawLine(x + 3, y + RESIZE_HANDLE_SIZE - 3, 
                     x + RESIZE_HANDLE_SIZE - 3, y + 3);
    painter.drawLine(x + 6, y + RESIZE_HANDLE_SIZE - 3, 
                     x + RESIZE_HANDLE_SIZE - 3, y + 6);
}
