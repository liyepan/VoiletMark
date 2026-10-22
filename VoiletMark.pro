TEMPLATE = app
TARGET   = VoiletMark
VERSION  = 0.2.0

QT += core gui widgets

CONFIG += c++17 warn_on

# 让 main.cpp 里的 QStringLiteral(VM_VERSION) 拿到 "0.1.0"
DEFINES += VM_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/MarkdownEditor.cpp \
    src/MarkdownHighlighter.cpp \
    src/ThemeManager.cpp

HEADERS += \
    src/MainWindow.h \
    src/MarkdownEditor.h \
    src/MarkdownHighlighter.h \
    src/ThemeManager.h
