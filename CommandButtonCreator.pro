# Command Button Creator 2.0 — Tardis Software
# One-press buttons for the shell commands Mark would otherwise have to type.
#
# Build:  qmake + make (Qt Creator does this for you).
# Needs only Qt Widgets — no network modules, no extra libraries.

QT       += core gui widgets
CONFIG   += c++17 warn_on
TARGET    = CommandButtonCreator
TEMPLATE  = app
VERSION   = 2.0.0

DEFINES  += QT_DEPRECATED_WARNINGS QT_NO_CAST_TO_ASCII
DEFINES  += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    main.cpp \
    apppaths.cpp \
    appsettings.cpp \
    buttonspec.cpp \
    buttonstore.cpp \
    secretstore.cpp \
    secretscanner.cpp \
    commandlinter.cpp \
    terminallauncher.cpp \
    commandrunner.cpp \
    statusmonitor.cpp \
    theme.cpp \
    commandbutton.cpp \
    pagecanvas.cpp \
    editdialog.cpp \
    secretsdialog.cpp \
    preferencesdialog.cpp \
    outputpanel.cpp \
    helpbrowser.cpp \
    helpcontent.cpp \
    mainwindow.cpp

HEADERS += \
    apppaths.h \
    appsettings.h \
    buttonspec.h \
    buttonstore.h \
    secretstore.h \
    secretscanner.h \
    commandlinter.h \
    terminallauncher.h \
    commandrunner.h \
    statusmonitor.h \
    theme.h \
    commandbutton.h \
    pagecanvas.h \
    editdialog.h \
    secretsdialog.h \
    preferencesdialog.h \
    outputpanel.h \
    helpbrowser.h \
    helpcontent.h \
    mainwindow.h

target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
