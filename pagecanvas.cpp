// pagecanvas.cpp — see pagecanvas.h.
#include "pagecanvas.h"

#include "commandbutton.h"
#include "theme.h"

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>
#include <QSet>
#include <QWheelEvent>

#include <algorithm>

PageCanvas::PageCanvas(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_rubber = new QRubberBand(QRubberBand::Rectangle, this);
}

// =============================================================== content
void PageCanvas::setButtons(const QList<ButtonSpec> &buttons)
{
    QSet<QString> wanted;
    for (const ButtonSpec &b : buttons)
        wanted.insert(b.id);

    // Remove buttons that are gone.
    for (auto it = m_buttons.begin(); it != m_buttons.end();) {
        if (!wanted.contains(it.key())) {
            if (m_pressButton == it.value()) {
                m_pressButton = nullptr;
                m_gesture = Gesture::None;
            }
            it.value()->deleteLater();
            it = m_buttons.erase(it);
        } else {
            ++it;
        }
    }
    // Update or create the rest, in page order.
    m_order.clear();
    for (const ButtonSpec &b : buttons) {
        m_order.append(b.id);
        CommandButton *w = m_buttons.value(b.id);
        if (w) {
            w->setSpec(b);
        } else {
            w = new CommandButton(b, this);
            w->setLocked(m_locked);
            w->setLabelPointSize(m_labelPt);
            connect(w, &CommandButton::pressed, this, &PageCanvas::onPressed);
            connect(w, &CommandButton::dragged, this, &PageCanvas::onDragged);
            connect(w, &CommandButton::released, this, &PageCanvas::onReleased);
            connect(w, &CommandButton::activated, this,
                    [this](CommandButton *cb) { emit runRequested(cb->id()); });
            connect(w, &CommandButton::contextMenuRequested, this,
                    [this](CommandButton *cb, const QPoint &pos) {
                        if (!m_selection.contains(cb->id()))
                            applySelection({cb->id()});
                        emit buttonContextMenu(cb->id(), pos);
                    });
            m_buttons.insert(b.id, w);
            w->show();
            emit buttonCreated(w);
        }
        w->raise();
    }
    QStringList keep;
    for (const QString &id : std::as_const(m_selection))
        if (wanted.contains(id))
            keep.append(id);
    if (keep != m_selection)
        applySelection(keep);
    updateExtent();
    update();
}

void PageCanvas::setLocked(bool locked)
{
    m_locked = locked;
    for (CommandButton *b : std::as_const(m_buttons))
        b->setLocked(locked);
    update();
}

void PageCanvas::setLabelPointSize(int pt)
{
    m_labelPt = pt;
    for (CommandButton *b : std::as_const(m_buttons))
        b->setLabelPointSize(pt);
}

// ============================================================= selection
QString PageCanvas::lastSelectedId() const
{
    return m_selection.isEmpty() ? QString() : m_selection.last();
}

void PageCanvas::applySelection(const QStringList &ids)
{
    QStringList clean;
    for (const QString &id : ids)
        if (m_buttons.contains(id) && !clean.contains(id))
            clean.append(id);
    const bool changed = clean != m_selection;
    m_selection = clean;
    for (auto it = m_buttons.cbegin(); it != m_buttons.cend(); ++it)
        it.value()->setSelected(m_selection.contains(it.key()));
    if (changed)
        emit selectionChanged();
}

void PageCanvas::setSelection(const QStringList &ids) { applySelection(ids); }
void PageCanvas::selectAll() { applySelection(m_order); }
void PageCanvas::clearSelection() { applySelection({}); }

// ============================================================ geometry
int PageCanvas::snap(int v) const
{
    if (!m_snap)
        return v;
    return int(qRound(double(v) / Grid)) * Grid;
}

QHash<QString, QRect> PageCanvas::currentGeometries(const QStringList &ids) const
{
    QHash<QString, QRect> out;
    for (const QString &id : ids)
        if (CommandButton *b = m_buttons.value(id))
            out.insert(id, b->geometry());
    return out;
}

void PageCanvas::updateExtent()
{
    int right = 0, bottom = 0;
    for (CommandButton *b : std::as_const(m_buttons)) {
        right = qMax(right, b->geometry().right());
        bottom = qMax(bottom, b->geometry().bottom());
    }
    setMinimumSize(right + Margin, bottom + Margin);
}

QPoint PageCanvas::freeSpot(const QSize &size, int viewportWidth) const
{
    const int cellW = size.width() + Gap;
    const int cellH = size.height() + Gap;
    const int cols = qMax(1, (qMax(viewportWidth, size.width() + 2 * Margin) - 2 * Margin + Gap) / cellW);
    for (int row = 0; row < 1000; ++row)
        for (int col = 0; col < cols; ++col) {
            const QRect cell(Margin + col * cellW, Margin + row * cellH, size.width(), size.height());
            bool clash = false;
            for (CommandButton *b : std::as_const(m_buttons))
                if (b->geometry().adjusted(-Gap / 2, -Gap / 2, Gap / 2, Gap / 2).intersects(cell)) {
                    clash = true;
                    break;
                }
            if (!clash)
                return cell.topLeft();
        }
    return QPoint(Margin, Margin);
}

QHash<QString, QRect> PageCanvas::organized(int viewportWidth, bool selectionOnly) const
{
    QStringList ids = (selectionOnly && m_selection.size() > 1) ? m_selection : m_order;
    QList<CommandButton *> list;
    for (const QString &id : ids)
        if (CommandButton *b = m_buttons.value(id))
            list.append(b);
    if (list.isEmpty())
        return {};

    // Reading order: rows top to bottom (buttons whose tops are within half
    // a button of each other count as one row), then left to right.
    int cellW = 0, cellH = 0;
    QRect bounds = list.first()->geometry();
    for (CommandButton *b : list) {
        cellW = qMax(cellW, b->width());
        cellH = qMax(cellH, b->height());
        bounds = bounds.united(b->geometry());
    }
    const int tolerance = qMax(1, cellH / 2);
    std::stable_sort(list.begin(), list.end(), [tolerance](CommandButton *a, CommandButton *b) {
        const int ra = a->y() / tolerance, rb = b->y() / tolerance;
        return ra != rb ? ra < rb : a->x() < b->x();
    });

    const bool partial = list.size() != m_buttons.size();
    const QPoint origin = partial ? bounds.topLeft() : QPoint(Margin, Margin);
    const int usable = qMax(cellW, viewportWidth - origin.x() - Margin);
    const int cols = qMax(1, (usable + Gap) / (cellW + Gap));

    QHash<QString, QRect> out;
    for (int i = 0; i < list.size(); ++i) {
        CommandButton *b = list.at(i);
        const QPoint pos(origin.x() + (i % cols) * (cellW + Gap),
                         origin.y() + (i / cols) * (cellH + Gap));
        out.insert(b->id(), QRect(pos, b->size()));
    }
    return out;
}

// =========================================================== gestures
void PageCanvas::onPressed(CommandButton *b, const QPoint &globalPos,
                           Qt::KeyboardModifiers mods, bool onHandle)
{
    setFocus(Qt::MouseFocusReason);
    m_gesture = Gesture::Pending;
    m_pressButton = b;
    m_pressGlobal = globalPos;
    m_pressOnHandle = onHandle;
    m_pressMods = mods;
}

void PageCanvas::beginGesture(Gesture g)
{
    m_gesture = g;
    if (g != Gesture::Moving && g != Gesture::Resizing)
        return;
    if (!m_selection.contains(m_pressButton->id()))
        applySelection({m_pressButton->id()});
    m_startGeometries = currentGeometries(m_selection);
}

void PageCanvas::onDragged(CommandButton *b, const QPoint &globalPos)
{
    if (b != m_pressButton || m_gesture == Gesture::None)
        return;
    const QPoint delta = globalPos - m_pressGlobal;
    if (m_gesture == Gesture::Pending) {
        if (delta.manhattanLength() < DragThreshold)
            return;
        if (m_locked) {
            m_gesture = Gesture::Blocked;
            emit lockedDragAttempt();
            return;
        }
        beginGesture(m_pressOnHandle ? Gesture::Resizing : Gesture::Moving);
    }
    if (m_gesture == Gesture::Moving) {
        // Snap the pressed button, then move the rest by the same amount so
        // the group keeps its shape.
        const QRect anchor = m_startGeometries.value(b->id());
        QPoint target(snap(anchor.x() + delta.x()), snap(anchor.y() + delta.y()));
        QPoint shift = target - anchor.topLeft();
        int minX = ButtonSpec::MaxCoord, minY = ButtonSpec::MaxCoord;
        for (const QRect &r : std::as_const(m_startGeometries)) {
            minX = qMin(minX, r.x());
            minY = qMin(minY, r.y());
        }
        shift.setX(qMax(shift.x(), -minX));
        shift.setY(qMax(shift.y(), -minY));
        for (auto it = m_startGeometries.cbegin(); it != m_startGeometries.cend(); ++it)
            if (CommandButton *w = m_buttons.value(it.key()))
                w->move(it.value().topLeft() + shift);
    } else if (m_gesture == Gesture::Resizing) {
        const int dw = snap(delta.x()), dh = snap(delta.y());
        for (auto it = m_startGeometries.cbegin(); it != m_startGeometries.cend(); ++it)
            if (CommandButton *w = m_buttons.value(it.key()))
                w->resize(qBound(ButtonSpec::MinWidth, it.value().width() + dw, ButtonSpec::MaxWidth),
                          qBound(ButtonSpec::MinHeight, it.value().height() + dh, ButtonSpec::MaxHeight));
    }
    updateExtent();
}

void PageCanvas::onReleased(CommandButton *b, const QPoint &)
{
    if (b != m_pressButton)
        return;
    const Gesture g = m_gesture;
    m_gesture = Gesture::None;
    m_pressButton = nullptr;
    const QString id = b->id();

    switch (g) {
    case Gesture::Pending:
        if (m_pressMods & Qt::ControlModifier) {
            QStringList sel = m_selection;
            if (sel.contains(id))
                sel.removeAll(id);
            else
                sel.append(id);
            applySelection(sel);
        } else if (m_pressMods & Qt::ShiftModifier) {
            QStringList sel = m_selection;
            sel.removeAll(id);
            sel.append(id);
            applySelection(sel);
        } else {
            emit runRequested(id);
        }
        break;
    case Gesture::Moving:
    case Gesture::Resizing: {
        const QHash<QString, QRect> now = currentGeometries(m_startGeometries.keys());
        const QString what = g == Gesture::Moving
                                 ? (now.size() > 1 ? QStringLiteral("Move %1 buttons").arg(now.size())
                                                   : QStringLiteral("Move button"))
                                 : (now.size() > 1 ? QStringLiteral("Resize %1 buttons").arg(now.size())
                                                   : QStringLiteral("Resize button"));
        m_startGeometries.clear();
        emit geometriesEdited(now, what);
        break;
    }
    case Gesture::Blocked:
    case Gesture::None:
        break;
    }
}

// ======================================================== canvas events
void PageCanvas::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(e->rect(), Theme::canvasColor());
    if (m_locked)
        return;
    // A faint dot grid says "the layout can be changed".
    p.setPen(Theme::canvasDotColor());
    const QRect r = e->rect();
    const int step = Grid * 2;
    for (int x = (r.left() / step) * step; x <= r.right(); x += step)
        for (int y = (r.top() / step) * step; y <= r.bottom(); y += step)
            p.drawPoint(x, y);
}

void PageCanvas::mousePressEvent(QMouseEvent *e)
{
    setFocus(Qt::MouseFocusReason);
    if (e->button() != Qt::LeftButton)
        return;
    m_rubberOrigin = e->position().toPoint();
    m_rubberBase = (e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) ? m_selection
                                                                                 : QStringList();
    if (m_rubberBase.isEmpty())
        clearSelection();
    m_rubber->setGeometry(QRect(m_rubberOrigin, QSize()));
    m_rubber->show();
}

void PageCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_rubber->isVisible())
        return;
    const QRect box = QRect(m_rubberOrigin, e->position().toPoint()).normalized();
    m_rubber->setGeometry(box);
    QStringList sel = m_rubberBase;
    for (const QString &id : std::as_const(m_order))
        if (m_buttons.value(id)->geometry().intersects(box) && !sel.contains(id))
            sel.append(id);
    applySelection(sel);
}

void PageCanvas::mouseReleaseEvent(QMouseEvent *)
{
    m_rubber->hide();
}

void PageCanvas::contextMenuEvent(QContextMenuEvent *e)
{
    emit canvasContextMenu(e->globalPos(), e->pos());
}

void PageCanvas::keyPressEvent(QKeyEvent *e)
{
    const bool fine = e->modifiers() & Qt::ShiftModifier;
    QPoint nudge;
    switch (e->key()) {
    case Qt::Key_Delete:
        if (!m_selection.isEmpty())
            emit deleteRequested(m_selection);
        return;
    case Qt::Key_Escape:
        clearSelection();
        return;
    case Qt::Key_Left:  nudge = QPoint(-1, 0); break;
    case Qt::Key_Right: nudge = QPoint(1, 0);  break;
    case Qt::Key_Up:    nudge = QPoint(0, -1); break;
    case Qt::Key_Down:  nudge = QPoint(0, 1);  break;
    default:
        QWidget::keyPressEvent(e);
        return;
    }
    if (m_selection.isEmpty())
        return;
    if (m_locked) {
        emit lockedDragAttempt();
        return;
    }
    nudge *= fine ? 1 : Grid;
    QHash<QString, QRect> g = currentGeometries(m_selection);
    for (auto it = g.begin(); it != g.end(); ++it)
        it.value().translate(nudge);
    for (const QRect &r : std::as_const(g))
        if (r.x() < 0 || r.y() < 0)
            return;
    emit geometriesEdited(g, QStringLiteral("Nudge"));
}

void PageCanvas::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        const int dy = e->angleDelta().y();
        if (dy != 0)
            emit zoomRequested(dy > 0 ? 1 : -1);
        e->accept();
        return;
    }
    e->ignore();
}
