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
    setStyleSheet(
        "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #87CEEB, stop:0.5 #4FC3F7, stop:1 #29B6F6); "
        "border: 2px solid #0277BD; "
        "border-radius: 6px; "
        "color: black; "
        "font-weight: bold; "
        "font-size: 12pt; "
        "padding: 5px;"
    );
    setContextMenuPolicy(Qt::NoContextMenu);  // Allow custom right-click handling
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
            resizeStartPosition = event->globalPosition().toPoint();
            resizeStartSize = size();
            event->accept();
        } else {
            // Store the starting position but don't set isDragging yet
            dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
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
    if (event->buttons() & Qt::LeftButton) {
        if (isResizing) {
            QPoint delta = event->globalPosition().toPoint() - resizeStartPosition;
            int newWidth = qMax(minimumWidth(), resizeStartSize.width() + delta.x());
            int newHeight = qMax(minimumHeight(), resizeStartSize.height() + delta.y());
            resize(newWidth, newHeight);
            event->accept();
        } else {
            // Only start dragging if mouse has moved
            isDragging = true;
            move(event->globalPosition().toPoint() - dragStartPosition);
            event->accept();
        }
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
