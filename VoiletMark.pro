TEMPLATE = app
TARGET   = VoiletMark
VERSION  = 0.3.0

QT += core gui widgets

CONFIG += c++17 warn_on

# 让 main.cpp 里的 QStringLiteral(VM_VERSION) 拿到版本号字符串
DEFINES += VM_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += $$PWD/src

RESOURCES += resources.qrc

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
