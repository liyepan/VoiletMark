TEMPLATE = app
TARGET   = VoiletMark
VERSION  = 0.11.0

QT += core gui widgets

CONFIG += c++17 warn_on

# 让 main.cpp 里的 QStringLiteral(VM_VERSION) 拿到版本号字符串
DEFINES += VM_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += $$PWD/src

RESOURCES += resources.qrc

# 各平台的可执行文件图标
win32: RC_ICONS = packaging/voiletmark.ico
macx:  ICON = packaging/voiletmark.icns

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/MarkdownEditor.cpp \
    src/MarkdownHighlighter.cpp \
    src/MarkdownRenderer.cpp \
    src/CodeHighlight.cpp \
    src/GitRunner.cpp \
    src/LanguageManager.cpp \
    src/ThemeManager.cpp

HEADERS += \
    src/MainWindow.h \
    src/MarkdownEditor.h \
    src/MarkdownHighlighter.h \
    src/MarkdownRenderer.h \
    src/CodeHighlight.h \
    src/GitRunner.h \
    src/LanguageManager.h \
    src/ThemeManager.h

# Linux 桌面集成：make install 之后能在应用菜单里找到
isEmpty(PREFIX): PREFIX = /usr/local

target.path = $$PREFIX/bin
desktop.files = packaging/voiletmark.desktop
desktop.path = $$PREFIX/share/applications
icon.files = packaging/voiletmark.png
icon.path = $$PREFIX/share/icons/hicolor/256x256/apps
INSTALLS += target desktop icon
