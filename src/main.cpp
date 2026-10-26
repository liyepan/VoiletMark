#include "LanguageManager.h"
#include "MainWindow.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QPixmap>
#include <QStyleFactory>
#include <QTimer>

#ifdef Q_OS_LINUX
namespace {

bool isGnomeDesktop()
{
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
    return desktop.contains(QLatin1String("gnome"))
        || desktop.contains(QLatin1String("unity"))
        || desktop.contains(QLatin1String("pantheon"));
}

bool hasFileMatching(const QString &dirPath, const QString &pattern)
{
    const QDir dir(dirPath);
    return dir.exists() && !dir.entryList({pattern}, QDir::Files).isEmpty();
}

// GNOME 的圆角标题栏/窗口按钮是 GTK 自己画的（CSD），Qt 要拿到同样的外观，
// 得在 Wayland 下装 QAdwaitaDecorations（Fedora 的 qt5-qtwayland 还打了
// 让 Qt5 支持装饰插件的补丁）。没有插件时 Qt5 在 GNOME Wayland 下会是
// 无边框窗口，所以退到 XWayland —— Mutter 会给 X11 窗口画系统标题栏，
// Ubuntu 24.04 这类没有该插件的发行版就走这条路。
void configureLinuxWindowFrame()
{
    const bool waylandSession = !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
        || qEnvironmentVariable("XDG_SESSION_TYPE").compare(QLatin1String("wayland"),
                                                              Qt::CaseInsensitive) == 0;
    if (!waylandSession || !isGnomeDesktop())
        return;

    const QString platform = qEnvironmentVariable("QT_QPA_PLATFORM").toLower();
    if (!platform.isEmpty() && !platform.contains(QLatin1String("wayland")))
        return; // 用户/系统已经指定 xcb 之类，尊重它

    const QString pluginsPath = QLibraryInfo::location(QLibraryInfo::PluginsPath);
    const bool hasWayland = hasFileMatching(pluginsPath + QStringLiteral("/platforms"),
                                            QStringLiteral("libqwayland*.so"));

    bool hasAdwaita = false;
    if (hasWayland) {
        const QStringList decorationDirs{
            pluginsPath + QStringLiteral("/wayland-decoration-client"),
            QStringLiteral("/usr/lib64/qt5/plugins/wayland-decoration-client"),
            QStringLiteral("/usr/lib/qt5/plugins/wayland-decoration-client"),
            QStringLiteral("/usr/lib/x86_64-linux-gnu/qt5/plugins/wayland-decoration-client"),
            QStringLiteral("/usr/lib/aarch64-linux-gnu/qt5/plugins/wayland-decoration-client"),
        };
        for (const QString &dir : decorationDirs) {
            if (hasFileMatching(dir, QStringLiteral("*adwaita*.so"))) {
                hasAdwaita = true;
                break;
            }
        }
    }

    if (hasWayland && hasAdwaita) {
        if (qEnvironmentVariableIsEmpty("QT_WAYLAND_DECORATION"))
            qputenv("QT_WAYLAND_DECORATION", "adwaita");
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            qputenv("QT_QPA_PLATFORM", "wayland");
        return;
    }

    if (!qEnvironmentVariableIsEmpty("DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "xcb");
}

} // namespace
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    configureLinuxWindowFrame();
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/packaging/voiletmark.png")));
    QGuiApplication::setDesktopFileName(QStringLiteral("voiletmark"));

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
    LanguageManager::instance()->initialize();

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
