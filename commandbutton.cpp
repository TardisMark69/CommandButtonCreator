// commandbutton.cpp — see commandbutton.h.
#include "commandbutton.h"

#include "theme.h"

#include <QContextMenuEvent>
#include <QEnterEvent>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

CommandButton::CommandButton(const ButtonSpec &spec, QWidget *parent)
    : QWidget(parent)
    , m_spec(spec)
{
    // Tab reaches buttons for keyboard use; a mouse click leaves focus on the
    // canvas so no focus ring appears on the button you just clicked.
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
    setGeometry(spec.geometry);

    m_spinTimer.setInterval(70);
    connect(&m_spinTimer, &QTimer::timeout, this, [this] {
        m_spinAngle = (m_spinAngle + 30) % 360;
        update();
    });
    m_okFadeTimer.setSingleShot(true);
    m_okFadeTimer.setInterval(6000);
    connect(&m_okFadeTimer, &QTimer::timeout, this, [this] {
        if (m_runState == RunState::Ok) {
            m_runState = RunState::Idle;
            update();
        }
    });
    refreshToolTip();
}

void CommandButton::setSpec(const ButtonSpec &spec)
{
    const bool moved = spec.geometry != m_spec.geometry;
    m_spec = spec;
    if (moved)
        setGeometry(spec.geometry);
    setAccessibleName(spec.label);
    refreshToolTip();
    update();
}

void CommandButton::setSelected(bool on)
{
    if (m_selected != on) {
        m_selected = on;
        update();
    }
}

void CommandButton::setLocked(bool locked)
{
    m_locked = locked;
    update();
}

void CommandButton::setLabelPointSize(int pt)
{
    m_labelPt = pt;
    update();
}

void CommandButton::setStatus(const StatusInfo &info)
{
    m_status = info;
    refreshToolTip();
    update();
}

void CommandButton::setRunState(RunState state, const QString &detail)
{
    m_runState = state;
    m_runDetail = detail;
    if (state == RunState::Running)
        m_spinTimer.start();
    else
        m_spinTimer.stop();
    if (state == RunState::Ok)
        m_okFadeTimer.start();
    refreshToolTip();
    update();
}

bool CommandButton::isInResizeHandle(const QPoint &local) const
{
    if (m_locked)
        return false;
    return local.x() >= width() - HandleSize && local.y() >= height() - HandleSize;
}

void CommandButton::refreshToolTip()
{
    QString t = QStringLiteral("<b>%1</b><br>").arg(m_spec.label.toHtmlEscaped());
    t += QStringLiteral("Runs in: %1").arg(RunModes::title(m_spec.mode));
    if (m_spec.confirm)
        t += QStringLiteral(" · asks first");
    switch (m_status.state) {
    case StatusState::Up:
        t += QStringLiteral("<br><span style='color:#1b7a3f'>● Up</span>");
        break;
    case StatusState::Down:
        t += QStringLiteral("<br><span style='color:#b3261e'>● Down</span>");
        break;
    case StatusState::Checking:
        t += QStringLiteral("<br>● Checking…");
        break;
    case StatusState::None:
        break;
    }
    if (m_status.state == StatusState::Up || m_status.state == StatusState::Down) {
        if (!m_status.detail.isEmpty())
            t += QStringLiteral(" — %1").arg(m_status.detail.toHtmlEscaped());
        t += QStringLiteral(" <i>(%1)</i>").arg(m_status.checked.toString(QStringLiteral("HH:mm:ss")));
    }
    if (!m_runDetail.isEmpty())
        t += QStringLiteral("<br>Last run: %1").arg(m_runDetail.toHtmlEscaped());

    QStringList lines = m_spec.command.split(QLatin1Char('\n'));
    const bool more = lines.size() > 4;
    lines = lines.mid(0, 4);
    QString preview = lines.join(QLatin1Char('\n'));
    if (preview.size() > 300)
        preview = preview.left(300) + QStringLiteral("…");
    else if (more)
        preview += QStringLiteral("\n…");
    t += QStringLiteral("<hr><pre style='margin:0'>%1</pre>").arg(preview.toHtmlEscaped());
    t += QStringLiteral("<i>Click to run · right-click for more</i>");
    setToolTip(t);
}

QFont CommandButton::fittedFont(const QRectF &area) const
{
    QFont f = font();
    f.setBold(true);
    const qreal start = m_labelPt;
    const qreal smallest = qMax<qreal>(6.0, start * 0.65);
    for (qreal pt = start; pt >= smallest; pt -= 0.5) {
        f.setPointSizeF(pt);
        const QRectF need = QFontMetricsF(f).boundingRect(
            area, Qt::AlignCenter | Qt::TextWordWrap, m_spec.label);
        if (need.width() <= area.width() + 0.5 && need.height() <= area.height() + 0.5)
            return f;
    }
    f.setPointSizeF(smallest);
    return f;
}

void CommandButton::paintBadge(QPainter &p, const QRectF &r)
{
    const qreal d = qMin<qreal>(16, r.height() * 0.42);
    const QRectF badge(r.right() - d - 3, r.top() + 3, d, d);
    switch (m_runState) {
    case RunState::Idle:
        return;
    case RunState::Running: {
        QPen pen(Theme::runningColor(), 2.4);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(QPen(QColor(0, 0, 0, 60), 2.4));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(badge.adjusted(2, 2, -2, -2));
        p.setPen(QPen(QColor(0xa8, 0x55, 0x00), 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(badge.adjusted(2, 2, -2, -2), -m_spinAngle * 16, 110 * 16);
        return;
    }
    case RunState::Ok:
    case RunState::Failed: {
        const bool ok = m_runState == RunState::Ok;
        p.setPen(QPen(QColor(0, 0, 0, 120), 1));
        p.setBrush(ok ? QColor(0x1f, 0xa4, 0x55) : QColor(0xd3, 0x2f, 0x2f));
        p.drawEllipse(badge);
        p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QRectF in = badge.adjusted(d * 0.27, d * 0.27, -d * 0.27, -d * 0.27);
        if (ok) {
            QPainterPath tick;
            tick.moveTo(in.left(), in.center().y());
            tick.lineTo(in.left() + in.width() * 0.38, in.bottom());
            tick.lineTo(in.right(), in.top());
            p.setBrush(Qt::NoBrush);
            p.drawPath(tick);
        } else {
            p.drawLine(in.topLeft(), in.bottomRight());
            p.drawLine(in.topRight(), in.bottomLeft());
        }
        return;
    }
    }
}

void CommandButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const Swatch &sw = Swatches::byKey(m_spec.color);
    const qreal radius = 7.0;
    const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    const bool down = m_down;
    const qreal shift = down ? 1.0 : 0.0;

    // Body
    QColor top = sw.top, mid = sw.mid, bottom = sw.bottom;
    if (down) {
        top = top.darker(112);
        mid = mid.darker(112);
        bottom = bottom.darker(112);
    } else if (m_hover) {
        top = top.lighter(108);
        mid = mid.lighter(106);
        bottom = bottom.lighter(104);
    }
    QPainterPath body;
    body.addRoundedRect(r, radius, radius);
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0.0, top);
    g.setColorAt(0.5, mid);
    g.setColorAt(1.0, bottom);
    p.fillPath(body, g);

    // 3-D bevel: light top edge, dark bottom edge (reversed when pressed)
    const QColor light(255, 255, 255, down ? 40 : 170);
    const QColor dark(0, 0, 0, down ? 110 : 55);
    p.setPen(QPen(down ? dark : light, 1.2));
    p.drawLine(QPointF(r.left() + radius, r.top() + 1.6), QPointF(r.right() - radius, r.top() + 1.6));
    p.setPen(QPen(down ? light : dark, 1.2));
    p.drawLine(QPointF(r.left() + radius, r.bottom() - 1.6),
               QPointF(r.right() - radius, r.bottom() - 1.6));

    // Border (gold when selected, as in 1.x)
    if (m_selected) {
        p.setPen(QPen(Theme::selectionColor(), 3.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    } else {
        p.setPen(QPen(sw.border, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(body);
    }
    if (hasFocus()) {
        QPen fp(QColor(0, 0, 0, 140), 1.0, Qt::DotLine);
        p.setPen(fp);
        p.drawRoundedRect(r.adjusted(4, 4, -4, -4), radius - 3, radius - 3);
    }

    // Status light, top-left
    qreal leftPad = 7, rightPad = 7;
    if (m_status.state != StatusState::None) {
        const qreal d = qMin<qreal>(11, r.height() * 0.3);
        const QRectF led(r.left() + 5, r.top() + 5, d, d);
        QColor c = m_status.state == StatusState::Up     ? Theme::okColor()
                   : m_status.state == StatusState::Down ? Theme::failColor()
                                                         : QColor(0x9e, 0x9e, 0x9e);
        QRadialGradient rg(led.center() - QPointF(d * 0.2, d * 0.2), d * 0.7);
        rg.setColorAt(0, c.lighter(150));
        rg.setColorAt(1, c);
        p.setPen(QPen(QColor(0, 0, 0, 150), 1.0));
        p.setBrush(rg);
        p.drawEllipse(led);
        leftPad = d + 8;
    }
    if (m_runState != RunState::Idle)
        rightPad = qMin<qreal>(16, r.height() * 0.42) + 6;
    paintBadge(p, r);

    // Mode mark, bottom-left
    QFont small = font();
    small.setPointSizeF(qMax(6.0, m_labelPt * 0.55));
    small.setBold(true);
    p.setFont(small);
    QColor markColor = sw.text;
    markColor.setAlpha(120);
    p.setPen(markColor);
    const QRectF markRect(r.left() + 6, r.bottom() - QFontMetricsF(small).height() - 2,
                          40, QFontMetricsF(small).height());
    if (r.height() >= 40)
        p.drawText(markRect, Qt::AlignLeft | Qt::AlignVCenter, RunModes::glyph(m_spec.mode));

    // Label
    const qreal pad = qMax(leftPad, rightPad);
    const QRectF textArea = r.adjusted(pad, 4, -pad, -4).translated(shift, shift);
    p.setFont(fittedFont(textArea));
    p.setPen(sw.text);
    p.setClipRect(r.adjusted(2, 2, -2, -2));
    p.drawText(textArea, Qt::AlignCenter | Qt::TextWordWrap,
               m_spec.label.isEmpty() ? QStringLiteral("(no label)") : m_spec.label);
    p.setClipping(false);

    // Resize grip, bottom-right, only when the layout can be changed
    if (!m_locked && (m_hover || m_selected)) {
        p.setPen(QPen(sw.border.darker(120), 1.3));
        const QPointF c = r.bottomRight() - QPointF(3, 3);
        for (int i = 1; i <= 3; ++i)
            p.drawLine(c - QPointF(i * 3.5, 0), c - QPointF(0, i * 3.5));
    }
}

// ----------------------------------------------------------------- mouse
void CommandButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    m_down = true;
    update();
    emit pressed(this, e->globalPosition().toPoint(), e->modifiers(),
                 isInResizeHandle(e->position().toPoint()));
}

void CommandButton::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
        emit dragged(this, e->globalPosition().toPoint());
        return;
    }
    setCursor(isInResizeHandle(e->position().toPoint()) ? Qt::SizeFDiagCursor
                                                        : Qt::PointingHandCursor);
}

void CommandButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    m_down = false;
    update();
    emit released(this, e->globalPosition().toPoint());
}

void CommandButton::contextMenuEvent(QContextMenuEvent *e)
{
    m_down = false;
    emit contextMenuRequested(this, e->globalPos());
}

void CommandButton::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (e->modifiers() == Qt::NoModifier) {
            emit activated(this);
            return;
        }
        break;
    case Qt::Key_Menu:
        emit contextMenuRequested(this, mapToGlobal(rect().center()));
        return;
    default:
        break;
    }
    e->ignore();   // arrows, Delete, Ctrl+A … go to the canvas
}

void CommandButton::enterEvent(QEnterEvent *e)
{
    m_hover = true;
    update();
    QWidget::enterEvent(e);
}

void CommandButton::leaveEvent(QEvent *e)
{
    m_hover = false;
    m_down = false;
    update();
    QWidget::leaveEvent(e);
}

void CommandButton::focusInEvent(QFocusEvent *e)
{
    update();
    QWidget::focusInEvent(e);
}

void CommandButton::focusOutEvent(QFocusEvent *e)
{
    update();
    QWidget::focusOutEvent(e);
}
