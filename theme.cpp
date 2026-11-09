// theme.cpp — see theme.h. The color values are TardisChat's Teal theme.
#include "theme.h"

#include <QApplication>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>

#include <functional>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>

namespace Theme {

QColor canvasColor()    { return QColor(0x18, 0x3c, 0x40); }
QColor canvasDotColor() { return QColor(0x2f, 0x5f, 0x65); }
QColor okColor()        { return QColor(0x3d, 0xdc, 0x84); }
QColor failColor()      { return QColor(0xff, 0x52, 0x52); }
QColor runningColor()   { return QColor(0xff, 0xb3, 0x66); }
QColor selectionColor() { return QColor(0xff, 0xd7, 0x00); }
QColor linkColor()      { return QColor(0x9f, 0xd8, 0xff); }

namespace {

// Qt drops its own check marks and spin arrows once those widgets are
// restyled, so small pictures are drawn once and kept in the cache folder
// (the same approach TardisChat uses).
QString asset(const QString &name, const std::function<void(QPainter &)> &draw, int w, int h)
{
    const QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                            .filePath(QStringLiteral("style"));
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath(name);
    QPixmap pm(w * 2, h * 2);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(2, 2);
        draw(p);
    }
    pm.save(path, "PNG");
    return path;
}

QString checkAsset()
{
    return asset(QStringLiteral("check.png"), [](QPainter &p) {
        p.setPen(QPen(QColor(0x0d, 0x1f, 0x14), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({QPointF(2.5, 7.5), QPointF(5.5, 10.5), QPointF(11.5, 3.5)}));
    }, 14, 14);
}

QString arrowAsset(bool up)
{
    return asset(up ? QStringLiteral("up.png") : QStringLiteral("down.png"), [up](QPainter &p) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xd8, 0xdd, 0xe5));
        p.drawPolygon(up ? QPolygonF({QPointF(1, 7), QPointF(9, 7), QPointF(5, 2)})
                         : QPolygonF({QPointF(1, 3), QPointF(9, 3), QPointF(5, 8)}));
    }, 10, 10);
}

QString sheet(int pt)
{
    const int small = qMax(7, pt - 1);
    QString s = QStringLiteral(R"(
/* ---- base ----------------------------------------------------------- */
QWidget { background-color: #2a5d63; color: #eaf3f4; }
QMainWindow, QDialog, QMessageBox { background-color: #2a5d63; }
QLabel { background: transparent; }
QToolTip { background-color: #fffbe6; color: #14171d; border: 1px solid #5a8d92;
           padding: 3px; }

/* ---- menus ---------------------------------------------------------- */
QMenuBar { background-color: #1f4a4f; border-bottom: 1px solid #143033; padding: 1px; }
QMenuBar::item { background: transparent; padding: 3px 8px; border-radius: 3px; }
QMenuBar::item:selected { background-color: #3a6d73; }
QMenu { background-color: #234f54; border: 1px solid #5a8d92; padding: 3px; }
QMenu::item { padding: 4px 22px 4px 22px; border-radius: 3px; }
QMenu::item:selected { background-color: #3f7d84; color: #ffffff; }
QMenu::item:disabled { color: #7fa2a5; }
QMenu::separator { height: 1px; background: #3e7177; margin: 4px 6px; }

/* ---- toolbar: raised silver 3-D buttons ------------------------------ */
QToolBar { background-color: #1f4a4f; border: none; border-bottom: 1px solid #143033;
           spacing: 4px; padding: 3px; }
QToolBar QToolButton, QPushButton {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0.00 #fcfdff, stop:0.10 #eef2f8, stop:0.45 #c5cdd9,
        stop:0.46 #adb7c5, stop:0.90 #dae0e9, stop:1.00 #f1f4f9);
    color: #14171d; font-weight: 700;
    border-width: 2px; border-style: solid;
    border-top-color: #ffffff;   border-left-color: #e9edf3;
    border-right-color: #3a424e; border-bottom-color: #29303b;
    border-radius: 5px; padding: 3px 10px; outline: 0;
}
QToolBar QToolButton:hover, QPushButton:hover {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0.00 #ffffff, stop:0.10 #f5f8fc, stop:0.45 #cfd7e2,
        stop:0.46 #b8c2d0, stop:0.90 #e4e9f0, stop:1.00 #f8fafc);
    border-right-color: #28303b; border-bottom-color: #1e242d;
}
QToolBar QToolButton:pressed, QPushButton:pressed {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #b2bac6, stop:0.5 #a0a9b6, stop:1 #909aa8);
    border-top-color: #29303b;   border-left-color: #3a424e;
    border-right-color: #e9edf3; border-bottom-color: #ffffff;
    padding-top: 4px; padding-bottom: 2px;
}
QPushButton:focus {
    border-top-color: #cfd9e6;   border-left-color: #cfd9e6;
    border-right-color: #7f92ab; border-bottom-color: #7f92ab;
}
QPushButton:default { border-right-color: #2b6f78; border-bottom-color: #1f5960; }
/* Toggles that are ON (Lock layout) stay pushed in and turn green. */
QToolBar QToolButton:checked, QPushButton:checked {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #8fd1a4, stop:0.5 #6fbd88, stop:1 #5aa874);
    color: #0d1f14;
    border-top-color: #1f3a29;   border-left-color: #29473a;
    border-right-color: #d9f2e2; border-bottom-color: #ffffff;
    padding-top: 4px; padding-bottom: 2px;
}
QToolBar QToolButton:disabled, QPushButton:disabled {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8d939c, stop:1 #7a808a);
    color: #50555d;
    border-top-color: #a3a9b2;   border-left-color: #a3a9b2;
    border-right-color: #50565f; border-bottom-color: #4a5058;
}
QToolBar QToolButton { padding: 2px 8px; }
QToolBar::separator { width: 1px; background: #143033; margin: 3px 4px; }
QPushButton#dangerButton {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #ff8a80, stop:0.45 #f2554a, stop:0.46 #e04338, stop:1 #f07267);
    color: #1a0705;
    border-top-color: #ffc9c4; border-left-color: #ffb8b2;
    border-right-color: #6e1610; border-bottom-color: #5a110c;
}
QPushButton#dangerButton:disabled {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8d939c, stop:1 #7a808a);
    color: #50555d;
    border-top-color: #a3a9b2;   border-left-color: #a3a9b2;
    border-right-color: #50565f; border-bottom-color: #4a5058;
}
QPushButton#dangerButton:pressed {
    background-color: #c9392f;
    border-top-color: #5a110c; border-left-color: #6e1610;
    border-right-color: #ffb8b2; border-bottom-color: #ffc9c4;
}
QPushButton#linkButton {
    background: transparent; border: none; color: #ffd47f; font-weight: 700;
    padding: 0px 6px; text-decoration: underline;
}

/* ---- small flat tool buttons (in dialogs and panels) ----------------- */
QToolButton { background: transparent; color: #cfe3e5; border: 1px solid transparent;
              border-radius: 4px; padding: 2px 6px; }
QToolButton:hover { background-color: #3a6d73; color: #ffffff; border-color: #5a8d92; }
QToolButton:disabled { color: #6f9295; }

/* ---- sunken 3-D fields ------------------------------------------------ */
QLineEdit, QPlainTextEdit, QTextEdit, QTextBrowser, QComboBox, QSpinBox,
QListWidget, QTreeWidget, QTableWidget, QListView, QTreeView, QTableView {
    background-color: #163538; color: #ffffff;
    border-width: 2px; border-style: solid;
    border-top-color: #14171d;   border-left-color: #14171d;
    border-right-color: #5a8d92; border-bottom-color: #5a8d92;
    border-radius: 3px;
    selection-background-color: #3f8f98; selection-color: #ffffff;
}
QLineEdit, QComboBox, QSpinBox { padding: 2px 6px; }
QPlainTextEdit, QTextEdit, QTextBrowser { padding: 3px; }
QLineEdit:read-only, QPlainTextEdit[readOnly="true"] { background-color: #1f4448; }
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus,
QListWidget:focus, QTreeWidget:focus {
    border-right-color: #8cc7cc; border-bottom-color: #8cc7cc;
}
QComboBox QAbstractItemView { background-color: #163538; border: 1px solid #5a8d92;
                              selection-background-color: #3f8f98; }
QListWidget::item, QTreeWidget::item { padding: 2px 3px; }
QHeaderView::section {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #eef2f8, stop:1 #c5cdd9);
    color: #14171d; font-weight: 700; padding: 2px 4px;
    border: 1px solid #3a424e; border-top-color: #ffffff; border-left-color: #e9edf3;
}

/* ---- the canvas well and pages --------------------------------------- */
QScrollArea#canvasScroll {
    background-color: #183c40;
    border-width: 2px; border-style: solid;
    border-top-color: #0b2225;   border-left-color: #0b2225;
    border-right-color: #5f9298; border-bottom-color: #5f9298;
    border-radius: 3px;
}
QTabWidget::pane { border: none; top: -1px; }
QTabBar::tab {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3f7a80, stop:1 #2f6066);
    color: #d9ecee; padding: 3px 12px; margin-right: 2px;
    border-width: 1px; border-style: solid;
    border-top-color: #6aa1a7; border-left-color: #5a8d92;
    border-right-color: #143033; border-bottom-color: #143033;
    border-top-left-radius: 5px; border-top-right-radius: 5px;
}
QTabBar::tab:selected {
    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fcfdff, stop:1 #c5cdd9);
    color: #14171d; font-weight: 700; border-bottom-color: #c5cdd9;
}
QTabBar::tab:hover:!selected { background-color: #4b8a91; }

/* ---- group boxes: etched frame --------------------------------------- */
QGroupBox {
    border-width: 2px; border-style: solid;
    border-top-color: #143033;   border-left-color: #143033;
    border-right-color: #5a8d92; border-bottom-color: #5a8d92;
    border-radius: 4px; margin-top: 1.1em; padding: 6px 4px 4px 4px; font-weight: 700;
}
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;
                   left: 8px; padding: 0 4px; color: #eaf3f4; }
QCheckBox, QRadioButton { spacing: 6px; background: transparent; }
QCheckBox::indicator {
    width: 13px; height: 13px; background-color: #163538;
    border-width: 2px; border-style: solid;
    border-top-color: #14171d;   border-left-color: #14171d;
    border-right-color: #5a8d92; border-bottom-color: #5a8d92;
    border-radius: 2px;
}
QCheckBox::indicator:hover { background-color: #1f4448; }
QCheckBox::indicator:checked { background-color: #6fbd88; image: url(%2); }
QSpinBox { padding-right: 18px; }
QSpinBox::up-button, QSpinBox::down-button {
    subcontrol-origin: border; width: 16px; background-color: #2f6066;
    border-left: 1px solid #5a8d92;
}
QSpinBox::up-button { subcontrol-position: top right; border-bottom: 1px solid #14171d; }
QSpinBox::down-button { subcontrol-position: bottom right; }
QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #3f7a80; }
QSpinBox::up-arrow { image: url(%3); width: 9px; height: 9px; }
QSpinBox::down-arrow { image: url(%4); width: 9px; height: 9px; }

/* ---- dock (Output panel) -------------------------------------------- */
QDockWidget { font-weight: 700; }
QDockWidget::title { background-color: #1f4a4f; padding: 3px 6px;
                     border-top: 1px solid #5a8d92; border-bottom: 1px solid #143033; }

/* ---- splitter / scroll bars ----------------------------------------- */
QSplitter::handle { background-color: #1f4a4f; }
QScrollBar:vertical { background: #183c40; width: 12px; margin: 0; }
QScrollBar:horizontal { background: #183c40; height: 12px; margin: 0; }
QScrollBar::handle {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #e6ebf2, stop:1 #aeb8c6);
    border: 1px solid #3a424e; border-radius: 4px; min-height: 24px; min-width: 24px;
}
QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* ---- status bar ----------------------------------------------------- */
QStatusBar { background-color: #1f4a4f; color: #cfe3e5; font-size: %1pt;
             border-top: 1px solid #143033; }
QStatusBar::item { border: none; }
QStatusBar QLabel { font-size: %1pt; padding: 0px 3px; }
QStatusBar QPushButton { font-size: %1pt; padding: 0px 6px; border-radius: 4px; }
QLabel#warningBadge { color: #ffd47f; font-weight: 700; }

/* ---- lint list in the Edit dialog ----------------------------------- */
QListWidget#lintList { background-color: #1f4448; }
QLabel#hint { color: #b5d3d6; }
)");
    return s.arg(small).arg(checkAsset(), arrowAsset(true), arrowAsset(false));
}

QPalette tealPalette()
{
    QPalette p;
    p.setColor(QPalette::Window, QColor(0x2a, 0x5d, 0x63));
    p.setColor(QPalette::WindowText, QColor(0xea, 0xf3, 0xf4));
    p.setColor(QPalette::Base, QColor(0x16, 0x35, 0x38));
    p.setColor(QPalette::AlternateBase, QColor(0x1f, 0x44, 0x48));
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(0xc5, 0xcd, 0xd9));
    p.setColor(QPalette::ButtonText, QColor(0x14, 0x17, 0x1d));
    p.setColor(QPalette::Highlight, QColor(0x3f, 0x8f, 0x98));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, QColor(0xff, 0xfb, 0xe6));
    p.setColor(QPalette::ToolTipText, QColor(0x14, 0x17, 0x1d));
    p.setColor(QPalette::Link, linkColor());
    p.setColor(QPalette::LinkVisited, linkColor());
    p.setColor(QPalette::PlaceholderText, QColor(0x8f, 0xb3, 0xb6));
    p.setColor(QPalette::Light, QColor(0x5a, 0x8d, 0x92));
    p.setColor(QPalette::Dark, QColor(0x14, 0x30, 0x33));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7f, 0xa2, 0xa5));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x7f, 0xa2, 0xa5));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x50, 0x55, 0x5d));
    return p;
}

} // namespace

void apply(QApplication &app, int basePt)
{
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
        app.setStyle(fusion);
    app.setPalette(tealPalette());
    QFont f = app.font();
    f.setPointSize(basePt);
    app.setFont(f);
    app.setStyleSheet(sheet(basePt));
}

QPixmap appIconPixmap(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal s = size;

    // Teal tile
    QPainterPath tile;
    tile.addRoundedRect(QRectF(s * 0.03, s * 0.03, s * 0.94, s * 0.94), s * 0.18, s * 0.18);
    QLinearGradient tg(0, 0, 0, s);
    tg.setColorAt(0, QColor(0x3f, 0x7a, 0x80));
    tg.setColorAt(1, QColor(0x18, 0x3c, 0x40));
    p.fillPath(tile, tg);
    p.setPen(QPen(QColor(0x0b, 0x22, 0x25), qMax(1.0, s * 0.02)));
    p.drawPath(tile);

    // The raised sky-blue button (the 1.x button colors)
    const QRectF br(s * 0.14, s * 0.28, s * 0.72, s * 0.44);
    QPainterPath btn;
    btn.addRoundedRect(br, s * 0.08, s * 0.08);
    QLinearGradient bg(br.topLeft(), br.bottomLeft());
    bg.setColorAt(0, QColor("#87CEEB"));
    bg.setColorAt(0.5, QColor("#4FC3F7"));
    bg.setColorAt(1, QColor("#29B6F6"));
    p.fillPath(btn, bg);
    p.setPen(QPen(QColor("#0277BD"), qMax(1.0, s * 0.03)));
    p.drawPath(btn);
    p.setPen(QPen(QColor(255, 255, 255, 170), qMax(1.0, s * 0.015)));
    p.drawLine(QPointF(br.left() + s * 0.06, br.top() + s * 0.04),
               QPointF(br.right() - s * 0.06, br.top() + s * 0.04));

    // ">_" prompt
    QFont f(QStringLiteral("DejaVu Sans Mono"));
    f.setBold(true);
    f.setPixelSize(int(s * 0.26));
    p.setFont(f);
    p.setPen(QColor(0x0b, 0x22, 0x25));
    p.drawText(br, Qt::AlignCenter, QStringLiteral(">_"));

    // A green status light
    const QRectF led(s * 0.70, s * 0.12, s * 0.16, s * 0.16);
    p.setBrush(okColor());
    p.setPen(QPen(QColor(0x0b, 0x22, 0x25), qMax(1.0, s * 0.02)));
    p.drawEllipse(led);
    return pm;
}

QIcon appIcon()
{
    QIcon icon;
    for (int s : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(appIconPixmap(s));
    return icon;
}

bool writeIconPng(const QString &path)
{
    return appIconPixmap(256).save(path, "PNG");
}

QString helpStyleSheet()
{
    return QStringLiteral(
        "body { color: #eaf3f4; }"
        "h1 { color: #ffffff; font-size: x-large; margin-bottom: 6px; }"
        "h2 { color: #bfe6ea; font-size: large; margin-top: 14px; }"
        "h3 { color: #bfe6ea; }"
        "a { color: #9fd8ff; }"
        "code, pre { font-family: 'DejaVu Sans Mono', monospace; color: #ffe8a8; }"
        "pre { background-color: #0f2a2d; }"
        "td, th { padding: 3px 8px; }"
        "th { background-color: #1f4a4f; color: #ffffff; }"
        ".tip { color: #c9f2d6; }"
        ".warn { color: #ffd47f; }"
        ".key { color: #ffffff; font-weight: bold; }");
}

} // namespace Theme
