#ifndef COMMANDBUTTON_H
#define COMMANDBUTTON_H

#include <QPushButton>
#include <QMouseEvent>
#include <QPoint>
#include <QString>

class CommandButton : public QPushButton
{
    Q_OBJECT

public:
    explicit CommandButton(QWidget *parent = nullptr);
    
    void setCommandText(const QString &text);
    QString getCommandText() const;
    
    void setButtonLabel(const QString &label);
    QString getButtonLabel() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void rightClicked();
    void executeCommand(const QString &command);

private:
    QString commandText;
    QPoint dragStartPosition;
    QPoint resizeStartPosition;
    QSize resizeStartSize;
    bool isDragging;
    bool isResizing;
    static const int RESIZE_HANDLE_SIZE = 10;
    
    bool isInResizeHandle(const QPoint &pos) const;
};

#endif // COMMANDBUTTON_H
