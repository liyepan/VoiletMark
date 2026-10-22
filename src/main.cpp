#include "MainWindow.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QPixmap>
#include <QStyleFactory>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/logo.png")));

    QCoreApplication::setOrganizationName(QStringLiteral("VoiletMark"));
    QCoreApplication::setApplicationName(QStringLiteral("VoiletMark"));
    QCoreApplication::setApplicationVersion(QStringLiteral(VM_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("VoiletMark — 轻量的跨平台 Markdown 编辑器"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption themeOption(
        QStringList{QStringLiteral("t"), QStringLiteral("theme")},
        QStringLiteral("启动主题：system / light / dark（仅在本次运行生效）"),
        QStringLiteral("mode"));
    parser.addOption(themeOption);

    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("渲染一张当前窗口截图后退出（用于调试与视觉回归）"),
        QStringLiteral("path"));
    parser.addOption(screenshotOption);

    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("要打开的 Markdown 文件"));
    parser.process(app);

    ThemeManager::Mode mode = ThemeManager::Mode::System;
    if (parser.isSet(themeOption)) {
        bool ok = false;
        mode = ThemeManager::modeFromString(parser.value(themeOption), &ok);
        if (!ok)
            mode = ThemeManager::Mode::System;
    }
    ThemeManager::instance()->initialize(mode, false);

    MainWindow window;
    window.show();

    const QStringList files = parser.positionalArguments();
    if (!files.isEmpty())
        window.openPath(files.first());

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        QTimer::singleShot(700, &window, [&window, path] {
            window.grab().save(path);
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
