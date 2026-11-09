// commandbutton.h — one button on the canvas.
//
// It only DRAWS and REPORTS: it paints itself (color, status light, run
// result, selection, resize grip) and tells the canvas about presses, drags
// and releases. Deciding whether a press was a click, a move or a resize —
// and doing it to every selected button — is the canvas's job.
#ifndef COMMANDBUTTON_H
#define COMMANDBUTTON_H

#include "buttonspec.h"
#include "statusmonitor.h"

#include <QPoint>
#include <QString>
#include <QTimer>
#include <QWidget>

class CommandButton : public QWidget
{
    Q_OBJECT
public:
    enum class RunState { Idle, Running, Ok, Failed };
    static constexpr int HandleSize = 14;

    explicit CommandButton(const ButtonSpec &spec, QWidget *parent = nullptr);

    const ButtonSpec &spec() const { return m_spec; }
    QString id() const { return m_spec.id; }
    void setSpec(const ButtonSpec &spec);

    bool isSelected() const { return m_selected; }
    void setSelected(bool on);
    void setLocked(bool locked);
    void setLabelPointSize(int pt);
    void setStatus(const StatusInfo &info);
    void setRunState(RunState state, const QString &detail);
    RunState runState() const { return m_runState; }

    bool isInResizeHandle(const QPoint &local) const;

signals:
    void pressed(CommandButton *b, const QPoint &globalPos, Qt::KeyboardModifiers mods,
                 bool onHandle);
    void dragged(CommandButton *b, const QPoint &globalPos);
    void released(CommandButton *b, const QPoint &globalPos);
    void activated(CommandButton *b);                         // Enter / Space
    void contextMenuRequested(CommandButton *b, const QPoint &globalPos);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    void refreshToolTip();
    void paintBadge(class QPainter &p, const QRectF &r);
    QFont fittedFont(const QRectF &area) const;

    ButtonSpec m_spec;
    StatusInfo m_status;
    RunState m_runState = RunState::Idle;
    QString m_runDetail;
    bool m_selected = false;
    bool m_locked = false;
    bool m_hover = false;
    bool m_down = false;
    int m_labelPt = 12;
    int m_spinAngle = 0;
    QTimer m_spinTimer;
    QTimer m_okFadeTimer;
};

#endif // COMMANDBUTTON_H
