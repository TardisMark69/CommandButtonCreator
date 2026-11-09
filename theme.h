// theme.h — the Tardis look: TardisChat's teal with the investment
// tracker's raised silver 3-D buttons and sunken fields.
//
// The interface font size is the zoom (Ctrl+wheel / Ctrl + −); everything
// else in the sheet is relative to it so the window stays compact with
// large system text.
#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

class QApplication;

namespace Theme {

void apply(QApplication &app, int basePt);

QIcon appIcon();
QPixmap appIconPixmap(int size);
bool writeIconPng(const QString &path);

// Colors used outside the stylesheet (painted widgets).
QColor canvasColor();        // the sunken well the buttons sit in
QColor canvasDotColor();     // the dot grid shown while unlocked
QColor okColor();
QColor failColor();
QColor runningColor();
QColor selectionColor();     // the gold selection border (as in 1.x)
QColor linkColor();

// Rich-text colors for the help browser.
QString helpStyleSheet();

} // namespace Theme

#endif // THEME_H
