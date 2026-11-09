// buttonspec.cpp — see buttonspec.h.
#include "buttonspec.h"

#include <QUuid>
#include <QtGlobal>

// ---------------------------------------------------------------- run modes
namespace RunModes {

const QList<RunMode> &all()
{
    static const QList<RunMode> modes{RunMode::Terminal, RunMode::Output, RunMode::Background};
    return modes;
}

QString key(RunMode m)
{
    switch (m) {
    case RunMode::Output:     return QStringLiteral("output");
    case RunMode::Background: return QStringLiteral("background");
    case RunMode::Terminal:   break;
    }
    return QStringLiteral("terminal");
}

RunMode fromKey(const QString &k, RunMode fallback)
{
    for (RunMode m : all())
        if (key(m) == k)
            return m;
    return fallback;
}

QString title(RunMode m)
{
    switch (m) {
    case RunMode::Output:     return QStringLiteral("Output panel");
    case RunMode::Background: return QStringLiteral("Background");
    case RunMode::Terminal:   break;
    }
    return QStringLiteral("Terminal window");
}

QString glyph(RunMode m)
{
    switch (m) {
    case RunMode::Output:     return QStringLiteral("≡");
    case RunMode::Background: return QStringLiteral("↗");
    case RunMode::Terminal:   break;
    }
    return QStringLiteral(">_");
}

QString summary(RunMode m)
{
    switch (m) {
    case RunMode::Output:
        return QStringLiteral("Runs quietly. What it prints appears in the Output panel, "
                              "and the button shows ✓ or ✗ when it finishes.");
    case RunMode::Background:
        return QStringLiteral("Starts it and leaves it running, even after you close this "
                              "program. What it prints goes to a log you can open.");
    case RunMode::Terminal:
        break;
    }
    return QStringLiteral("Opens a terminal window you can type into. Use this for ssh, "
                          "ollama run, sudo, or anything that asks you questions.");
}

} // namespace RunModes

// ------------------------------------------------------------------ colors
namespace Swatches {

const QList<Swatch> &all()
{
    // "blue" is exactly the 1.x button gradient and border.
    static const QList<Swatch> list{
        {QStringLiteral("blue"),   QStringLiteral("Sky blue"),
         QColor("#87CEEB"), QColor("#4FC3F7"), QColor("#29B6F6"), QColor("#0277BD"), Qt::black},
        {QStringLiteral("teal"),   QStringLiteral("Teal"),
         QColor("#a4ece5"), QColor("#5ad6ca"), QColor("#2cb9ab"), QColor("#0f7c73"), Qt::black},
        {QStringLiteral("green"),  QStringLiteral("Green"),
         QColor("#bfeaa9"), QColor("#93d873"), QColor("#6cbd49"), QColor("#3b7d23"), Qt::black},
        {QStringLiteral("amber"),  QStringLiteral("Amber"),
         QColor("#ffe597"), QColor("#ffcd57"), QColor("#ffb612"), QColor("#a87400"), Qt::black},
        {QStringLiteral("red"),    QStringLiteral("Coral red"),
         QColor("#ffb7ab"), QColor("#ff8a77"), QColor("#f2643f"), QColor("#a3330f"), Qt::black},
        {QStringLiteral("purple"), QStringLiteral("Lavender"),
         QColor("#ddc4f7"), QColor("#c29aec"), QColor("#a676de"), QColor("#5e3593"), Qt::black},
        {QStringLiteral("silver"), QStringLiteral("Silver"),
         QColor("#f6f8fb"), QColor("#d3dae3"), QColor("#b3bdc9"), QColor("#5d6773"), Qt::black},
    };
    return list;
}

const Swatch &byKey(const QString &key)
{
    for (const Swatch &s : all())
        if (s.key == key)
            return s;
    return all().first();
}

} // namespace Swatches

// ------------------------------------------------------------- button spec
QString ButtonSpec::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QJsonObject ButtonSpec::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), id);
    o.insert(QStringLiteral("label"), label);
    o.insert(QStringLiteral("command"), command);
    o.insert(QStringLiteral("mode"), RunModes::key(mode));
    o.insert(QStringLiteral("confirm"), confirm);
    o.insert(QStringLiteral("color"), color);
    if (!statusCheck.isEmpty())
        o.insert(QStringLiteral("statusCheck"), statusCheck);
    o.insert(QStringLiteral("x"), geometry.x());
    o.insert(QStringLiteral("y"), geometry.y());
    o.insert(QStringLiteral("width"), geometry.width());
    o.insert(QStringLiteral("height"), geometry.height());
    return o;
}

namespace {

QString readString(const QJsonObject &o, const char *key, int maxLen,
                   const QString &who, QStringList *warnings)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (v.isUndefined() || v.isNull())
        return QString();
    if (!v.isString()) {
        if (warnings)
            warnings->append(QStringLiteral("%1: \"%2\" was not text and was ignored.")
                                 .arg(who, QLatin1String(key)));
        return QString();
    }
    QString s = v.toString();
    if (s.size() > maxLen) {
        s.truncate(maxLen);
        if (warnings)
            warnings->append(QStringLiteral("%1: \"%2\" was too long and was shortened.")
                                 .arg(who, QLatin1String(key)));
    }
    return s;
}

int readInt(const QJsonObject &o, const char *key, int fallback)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (!v.isDouble())
        return fallback;
    const double d = v.toDouble();
    if (d != d || d > 1e7 || d < -1e7)   // NaN / absurd
        return fallback;
    return int(d);
}

} // namespace

ButtonSpec ButtonSpec::fromJson(const QJsonObject &o, QStringList *warnings)
{
    ButtonSpec b;
    const QString who = o.value(QStringLiteral("label")).isString()
                            ? QStringLiteral("Button \"%1\"")
                                  .arg(o.value(QStringLiteral("label")).toString().left(40))
                            : QStringLiteral("A button");

    b.id = readString(o, "id", 64, who, warnings);
    if (b.id.isEmpty() || QUuid::fromString(b.id).isNull())
        b.id = newId();
    b.label       = readString(o, "label", MaxLabel, who, warnings);
    b.command     = readString(o, "command", MaxCommand, who, warnings);
    b.statusCheck = readString(o, "statusCheck", MaxStatusCheck, who, warnings);
    b.mode  = RunModes::fromKey(o.value(QStringLiteral("mode")).toString(), RunMode::Terminal);
    b.confirm = o.value(QStringLiteral("confirm")).toBool(false);
    b.color = Swatches::byKey(o.value(QStringLiteral("color")).toString()).key;
    b.geometry = QRect(readInt(o, "x", 20), readInt(o, "y", 20),
                       readInt(o, "width", DefaultWidth), readInt(o, "height", DefaultHeight));
    b.sanitize();
    return b;
}

void ButtonSpec::sanitize()
{
    label.truncate(MaxLabel);
    command.truncate(MaxCommand);
    statusCheck.truncate(MaxStatusCheck);
    label.remove(QChar(0));
    command.remove(QChar(0));
    statusCheck.remove(QChar(0));
    if (id.isEmpty())
        id = newId();
    color = Swatches::byKey(color).key;
    geometry = QRect(qBound(0, geometry.x(), MaxCoord), qBound(0, geometry.y(), MaxCoord),
                     qBound(MinWidth, geometry.width(), MaxWidth),
                     qBound(MinHeight, geometry.height(), MaxHeight));
}
