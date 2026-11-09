// pagecanvas.h — one page of buttons: placing, selecting, moving, resizing.
//
// Mouse rules (the same as 1.x, plus a few):
//   click                       run the button
//   Ctrl+click / Shift+click    select / add to the selection
//   drag a button               move it (and every other selected button)
//   drag the bottom-right grip  resize it (and every other selected button)
//   drag on empty space         draw a box to select several buttons
//   Ctrl+wheel                  zoom
// Moves and resizes snap to a 10-pixel grid and are reported to the window
// as ONE change when the mouse is released, so one Undo puts them back.
// When the layout is locked, drags do nothing and never run the button.
#ifndef PAGECANVAS_H
#define PAGECANVAS_H

#include "buttonspec.h"

#include <QHash>
#include <QMap>
#include <QPoint>
#include <QRect>
#include <QStringList>
#include <QWidget>

class CommandButton;
class QRubberBand;

class PageCanvas : public QWidget
{
    Q_OBJECT
public:
    static constexpr int Grid = 10;
    static constexpr int Margin = 20;
    static constexpr int Gap = 10;
    static constexpr int DragThreshold = 6;

    explicit PageCanvas(QWidget *parent = nullptr);

    void setButtons(const QList<ButtonSpec> &buttons);
    QList<CommandButton *> buttons() const { return m_buttons.values(); }
    CommandButton *buttonFor(const QString &id) const { return m_buttons.value(id); }

    QStringList selectedIds() const { return m_selection; }    // in the order picked
    QString lastSelectedId() const;
    void setSelection(const QStringList &ids);
    void selectAll();
    void clearSelection();

    void setLocked(bool locked);
    bool isLocked() const { return m_locked; }
    void setSnap(bool snap) { m_snap = snap; }
    void setLabelPointSize(int pt);

    // Top-left of the first free spot for a button of this size.
    QPoint freeSpot(const QSize &size, int viewportWidth) const;
    // New positions that lay the buttons out in a neat grid.
    QHash<QString, QRect> organized(int viewportWidth, bool selectionOnly) const;

signals:
    void runRequested(const QString &id);
    void buttonContextMenu(const QString &id, const QPoint &globalPos);
    void canvasContextMenu(const QPoint &globalPos, const QPoint &canvasPos);
    void geometriesEdited(const QHash<QString, QRect> &geometries, const QString &what);
    void deleteRequested(const QStringList &ids);
    void selectionChanged();
    void zoomRequested(int steps);
    void lockedDragAttempt();
    void buttonCreated(CommandButton *b);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    enum class Gesture { None, Pending, Moving, Resizing, Blocked };

    void onPressed(CommandButton *b, const QPoint &globalPos, Qt::KeyboardModifiers mods,
                   bool onHandle);
    void onDragged(CommandButton *b, const QPoint &globalPos);
    void onReleased(CommandButton *b, const QPoint &globalPos);
    void beginGesture(Gesture g);
    QHash<QString, QRect> currentGeometries(const QStringList &ids) const;
    void applySelection(const QStringList &ids);
    void updateExtent();
    int snap(int v) const;

    QMap<QString, CommandButton *> m_buttons;
    QStringList m_order;                     // ids in stacking/page order
    QStringList m_selection;
    bool m_locked = false;
    bool m_snap = true;
    int m_labelPt = 12;

    Gesture m_gesture = Gesture::None;
    CommandButton *m_pressButton = nullptr;
    QPoint m_pressGlobal;
    bool m_pressOnHandle = false;
    Qt::KeyboardModifiers m_pressMods;
    QHash<QString, QRect> m_startGeometries;

    QRubberBand *m_rubber = nullptr;
    QPoint m_rubberOrigin;
    QStringList m_rubberBase;
};

#endif // PAGECANVAS_H
