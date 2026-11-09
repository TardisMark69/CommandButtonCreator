// buttonspec.h — what one command button IS (pure data, no widgets).
//
// A ButtonSpec is saved to and loaded from buttons.json. Loading never
// trusts the file: every field is type-checked, lengths are capped and the
// geometry is clamped, so a damaged or hand-edited file can't crash the
// program or put a button somewhere unreachable.
#ifndef BUTTONSPEC_H
#define BUTTONSPEC_H

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QRect>
#include <QString>
#include <QStringList>

// How a button's command is run.
enum class RunMode {
    Terminal,    // in a terminal window you can type into (ssh, ollama run, sudo …)
    Output,      // quietly; the text it prints is shown in the Output panel
    Background   // started and left running after this program closes; output to a log
};

namespace RunModes {
const QList<RunMode> &all();
QString key(RunMode m);                    // "terminal" / "output" / "background"
RunMode fromKey(const QString &k, RunMode fallback = RunMode::Terminal);
QString title(RunMode m);                  // "Terminal window" …
QString glyph(RunMode m);                  // small mark painted on the button
QString summary(RunMode m);                // one-line explanation
} // namespace RunModes

// A button color. All are light enough for the bold black label.
struct Swatch {
    QString key;
    QString name;
    QColor top, mid, bottom, border, text;
};

namespace Swatches {
const QList<Swatch> &all();
const Swatch &byKey(const QString &key);   // unknown key -> the original blue
} // namespace Swatches

struct ButtonSpec {
    static constexpr int MaxLabel       = 200;
    static constexpr int MaxCommand     = 65536;
    static constexpr int MaxStatusCheck = 4096;
    static constexpr int MinWidth  = 60,   MinHeight = 28;
    static constexpr int MaxWidth  = 1600, MaxHeight = 800;
    static constexpr int MaxCoord  = 20000;
    static constexpr int DefaultWidth = 209, DefaultHeight = 50;   // the 1.x size

    QString id;                 // stable unique id (never shown)
    QString label;
    QString command;
    RunMode mode = RunMode::Terminal;
    bool    confirm = false;    // ask "Run …?" first
    QString color = QStringLiteral("blue");
    QString statusCheck;        // optional: exit 0 = green light
    QRect   geometry{20, 20, DefaultWidth, DefaultHeight};

    static QString newId();

    QJsonObject toJson() const;
    // Reads both the 1.x layout (label/command/x/y/width/height) and 2.x.
    // Problems are fixed up and described in *warnings (may be null).
    static ButtonSpec fromJson(const QJsonObject &o, QStringList *warnings);

    // Clamp lengths and geometry into the allowed ranges.
    void sanitize();
};

#endif // BUTTONSPEC_H
