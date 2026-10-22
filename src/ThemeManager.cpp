#include "ThemeManager.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>
#include <QXmlStreamReader>

namespace {

QString runCommand(const QString &program, const QStringList &arguments, int timeoutMs)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(200);
        return QString();
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool looksDark(const QString &themeName)
{
    return themeName.contains(QLatin1String("dark"), Qt::CaseInsensitive);
}

/// XFCE 把界面主题记录在 xfconf 的 xsettings.xml 里。
QString xfceThemeName()
{
    QFile file(QDir::homePath() + QStringLiteral("/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml"));
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name().toString() != QLatin1String("property"))
            continue;
        const QXmlStreamAttributes attributes = xml.attributes();
        if (attributes.value(QLatin1String("name")).toString() == QLatin1String("ThemeName"))
            return attributes.value(QLatin1String("value")).toString();
    }
    return QString();
}

QColor hex(const char *value)
{
    return QColor(QString::fromLatin1(value));
}

ThemeColors lightColors()
{
    ThemeColors c;
    c.window              = hex("#f2f3f5");
    c.windowText          = hex("#1f2328");
    c.base                = hex("#ffffff");
    c.alternateBase       = hex("#f6f7f9");
    c.text                = hex("#1f2328");
    c.button              = hex("#ffffff");
    c.buttonText          = hex("#1f2328");
    c.border              = hex("#dcdfe4");
    c.accent              = hex("#0969da");
    c.highlight           = hex("#cfe3ff");
    c.highlightedText     = hex("#0b1f38");
    c.disabled            = hex("#9aa2ab");
    c.placeholder         = hex("#6b7480");
    c.menuBackground      = hex("#ffffff");
    c.scrollHandle        = hex("#c9ced6");
    c.scrollHandleHover   = hex("#aeb5bf");
    c.sidebarBackground   = hex("#f8f9fb");
    c.hoverBackground     = hex("#eaedf1");

    c.editorBase          = hex("#ffffff");
    c.editorText          = hex("#24292f");
    c.gutterBackground    = hex("#fafbfc");
    c.gutterText          = hex("#b0b7c0");
    c.gutterActiveText    = hex("#5c6670");
    c.currentLine         = hex("#f4f6f9");

    c.heading             = hex("#0a52a3");
    c.headingMarker       = hex("#b3b9c2");
    c.strong              = hex("#11151a");
    c.emphasis            = hex("#4b535c");
    c.code                = hex("#b4295a");
    c.codeBackground      = hex("#f2f4f7");
    c.codeKeyword         = hex("#0550ae");
    c.codeString          = hex("#0a7d3a");
    c.codeNumber          = hex("#953800");
    c.codeComment         = hex("#6e7781");
    c.codePreprocessor    = hex("#8250df");
    c.link                = hex("#0969da");
    c.linkUrl             = hex("#6b7480");
    c.quote               = hex("#6b7480");
    c.quoteMarker         = hex("#c3c9d2");
    c.listMarker          = hex("#0969da");
    c.rule                = hex("#d6dae0");
    c.taskDone            = hex("#1a7f37");
    return c;
}

ThemeColors darkColors()
{
    ThemeColors c;
    c.window              = hex("#1b1d21");
    c.windowText          = hex("#d6d9dd");
    c.base                = hex("#1e2024");
    c.alternateBase       = hex("#23262b");
    c.text                = hex("#d6d9dd");
    c.button              = hex("#26292f");
    c.buttonText          = hex("#d6d9dd");
    c.border              = hex("#2d3137");
    c.accent              = hex("#3b8eea");
    c.highlight           = hex("#26466b");
    c.highlightedText     = hex("#eef3f8");
    c.disabled            = hex("#5d646d");
    c.placeholder         = hex("#8b939d");
    c.menuBackground      = hex("#242730");
    c.scrollHandle        = hex("#3a3f47");
    c.scrollHandleHover   = hex("#4d535d");
    c.sidebarBackground   = hex("#1a1c20");
    c.hoverBackground     = hex("#272b31");

    c.editorBase          = hex("#1e2024");
    c.editorText          = hex("#d7dade");
    c.gutterBackground    = hex("#1a1c20");
    c.gutterText          = hex("#4f565f");
    c.gutterActiveText    = hex("#aeb5bf");
    c.currentLine         = hex("#23262c");

    c.heading             = hex("#79b8ff");
    c.headingMarker       = hex("#5d6570");
    c.strong              = hex("#f2f4f7");
    c.emphasis            = hex("#c3c9d1");
    c.code                = hex("#ff8fbf");
    c.codeBackground      = hex("#292c33");
    c.codeKeyword         = hex("#79b8ff");
    c.codeString          = hex("#7ee787");
    c.codeNumber          = hex("#ffab70");
    c.codeComment         = hex("#8b949e");
    c.codePreprocessor    = hex("#d2a8ff");
    c.link                = hex("#62a8ff");
    c.linkUrl             = hex("#7f8791");
    c.quote               = hex("#8b939d");
    c.quoteMarker         = hex("#4f565f");
    c.listMarker          = hex("#62a8ff");
    c.rule                = hex("#33373e");
    c.taskDone            = hex("#5fbf7f");
    return c;
}

} // namespace

ThemeManager *ThemeManager::instance()
{
    static ThemeManager *manager = new ThemeManager();
    return manager;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    if (qApp) {
        const QColor window = qApp->palette().color(QPalette::Window);
        m_paletteGuess = window.lightness() < 128 ? Scheme::Dark : Scheme::Light;
    }

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(2000);
    connect(m_pollTimer, &QTimer::timeout, this, &ThemeManager::pollSystemTheme);
}

void ThemeManager::initialize(Mode forcedMode, bool persist)
{
    if (forcedMode != Mode::System) {
        setMode(forcedMode, persist);
    } else {
        const QString saved = QSettings().value(QStringLiteral("theme/mode")).toString();
        m_mode = modeFromString(saved, nullptr);
        applyScheme(schemeForMode(m_mode), true);
    }
    m_pollTimer->start();
}

void ThemeManager::setMode(Mode mode, bool persist)
{
    m_mode = mode;
    if (persist)
        QSettings().setValue(QStringLiteral("theme/mode"), modeToString(mode));

    applyScheme(schemeForMode(m_mode), true);
}

void ThemeManager::pollSystemTheme()
{
    if (m_mode == Mode::Light || m_mode == Mode::Dark)
        return;

    // 跟随系统：读桌面设置；按时段：到 6:00 / 18:00 自动翻转。
    const Scheme target = schemeForMode(m_mode);
    if (target != m_scheme)
        applyScheme(target, false);
}

ThemeManager::Scheme ThemeManager::schemeForMode(Mode mode) const
{
    switch (mode) {
    case Mode::Light:    return Scheme::Light;
    case Mode::Dark:     return Scheme::Dark;
    case Mode::Schedule: return scheduleScheme();
    case Mode::System:   break;
    }
    return detectSystemScheme();
}

ThemeManager::Scheme ThemeManager::scheduleScheme() const
{
    const int hour = QTime::currentTime().hour();
    return (hour >= 6 && hour < 18) ? Scheme::Light : Scheme::Dark;
}

void ThemeManager::applyScheme(Scheme scheme, bool force)
{
    const bool changed = (scheme != m_scheme) || force;
    m_scheme = scheme;
    buildColors(m_scheme);
    applyToApplication();
    if (changed)
        emit schemeChanged(m_scheme);
}

void ThemeManager::buildColors(Scheme scheme)
{
    m_colors = (scheme == Scheme::Dark) ? darkColors() : lightColors();
}

void ThemeManager::applyToApplication()
{
    QPalette palette;
    palette.setColor(QPalette::Window, m_colors.window);
    palette.setColor(QPalette::WindowText, m_colors.windowText);
    palette.setColor(QPalette::Base, m_colors.base);
    palette.setColor(QPalette::AlternateBase, m_colors.alternateBase);
    palette.setColor(QPalette::Text, m_colors.text);
    palette.setColor(QPalette::Button, m_colors.button);
    palette.setColor(QPalette::ButtonText, m_colors.buttonText);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, m_colors.highlight);
    palette.setColor(QPalette::HighlightedText, m_colors.highlightedText);
    palette.setColor(QPalette::ToolTipBase, m_colors.menuBackground);
    palette.setColor(QPalette::ToolTipText, m_colors.text);
    palette.setColor(QPalette::PlaceholderText, m_colors.placeholder);
    palette.setColor(QPalette::Link, m_colors.link);
    palette.setColor(QPalette::LinkVisited, m_colors.link);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, m_colors.disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, m_colors.disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, m_colors.disabled);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, m_colors.disabled);

    qApp->setPalette(palette);
    qApp->setStyleSheet(styleSheet());
}

QString ThemeManager::styleSheet() const
{
    QString qss = QString::fromLatin1(R"(
QMainWindow, QDialog { background-color: @window; }
QMenuBar { background-color: @window; color: @windowText; border-bottom: 1px solid @border; }
QMenuBar::item { background: transparent; padding: 5px 10px; border-radius: 5px; }
QMenuBar::item:selected { background-color: @highlight; color: @highlightedText; }
QMenu { background-color: @menuBackground; color: @text; border: 1px solid @border; padding: 5px; }
QMenu::item { padding: 6px 28px 6px 22px; border-radius: 5px; }
QMenu::item:selected { background-color: @highlight; color: @highlightedText; }
QMenu::item:disabled { color: @disabled; }
QMenu::separator { height: 1px; background-color: @border; margin: 5px 8px; }
QStatusBar { background-color: @window; color: @placeholder; border-top: 1px solid @border; }
QStatusBar::item { border: none; }
QPlainTextEdit { background-color: @editorBase; color: @editorText; border: none;
                 selection-background-color: @highlight; selection-color: @highlightedText; }
QScrollBar:vertical { background: transparent; width: 11px; margin: 0; }
QScrollBar::handle:vertical { background-color: @scrollHandle; border-radius: 4px;
                              min-height: 28px; margin: 2px 2px; }
QScrollBar::handle:vertical:hover { background-color: @scrollHandleHover; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }
QScrollBar::handle:horizontal { background-color: @scrollHandle; border-radius: 4px;
                                min-width: 28px; margin: 2px 2px; }
QScrollBar::handle:horizontal:hover { background-color: @scrollHandleHover; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QPushButton { background-color: @button; color: @buttonText; border: 1px solid @border;
              border-radius: 6px; padding: 5px 16px; min-width: 68px; }
QPushButton:hover { background-color: @highlight; color: @highlightedText; }
QPushButton:default { background-color: @accent; color: #ffffff; border-color: @accent; }
QToolTip { background-color: @menuBackground; color: @text; border: 1px solid @border; padding: 4px; }
QToolButton { border: none; border-radius: 5px; padding: 3px 8px; color: @text; background: transparent; }
QToolButton:hover { background-color: @hoverBackground; }
QToolButton:checked { background-color: @highlight; color: @highlightedText; }
QDockWidget { color: @text; }
QDockWidget::title { background-color: @window; padding: 6px 8px; }
QListWidget { background-color: @sidebarBackground; border: 1px solid @border; border-radius: 6px;
              padding: 3px; outline: 0; }
QListWidget::item { padding: 4px 6px; border-radius: 4px; }
QListWidget::item:hover { background-color: @hoverBackground; }
QListWidget::item:selected { background-color: @highlight; color: @highlightedText; }
)");

    // 先替换长 token，避免 @window 吃掉 @windowText 这类前缀冲突。
    qss.replace(QLatin1String("@scrollHandleHover"), m_colors.scrollHandleHover.name());
    qss.replace(QLatin1String("@highlightedText"), m_colors.highlightedText.name());
    qss.replace(QLatin1String("@buttonText"), m_colors.buttonText.name());
    qss.replace(QLatin1String("@windowText"), m_colors.windowText.name());
    qss.replace(QLatin1String("@editorText"), m_colors.editorText.name());
    qss.replace(QLatin1String("@menuBackground"), m_colors.menuBackground.name());
    qss.replace(QLatin1String("@scrollHandle"), m_colors.scrollHandle.name());
    qss.replace(QLatin1String("@editorBase"), m_colors.editorBase.name());
    qss.replace(QLatin1String("@placeholder"), m_colors.placeholder.name());
    qss.replace(QLatin1String("@disabled"), m_colors.disabled.name());
    qss.replace(QLatin1String("@border"), m_colors.border.name());
    qss.replace(QLatin1String("@hoverBackground"), m_colors.hoverBackground.name());
    qss.replace(QLatin1String("@sidebarBackground"), m_colors.sidebarBackground.name());
    qss.replace(QLatin1String("@accent"), m_colors.accent.name());
    qss.replace(QLatin1String("@highlight"), m_colors.highlight.name());
    qss.replace(QLatin1String("@button"), m_colors.button.name());
    qss.replace(QLatin1String("@window"), m_colors.window.name());
    qss.replace(QLatin1String("@text"), m_colors.text.name());
    return qss;
}

QString ThemeManager::modeToString(Mode mode)
{
    switch (mode) {
    case Mode::Light:    return QStringLiteral("light");
    case Mode::Dark:     return QStringLiteral("dark");
    case Mode::Schedule: return QStringLiteral("schedule");
    case Mode::System:   break;
    }
    return QStringLiteral("system");
}

ThemeManager::Mode ThemeManager::modeFromString(const QString &value, bool *ok)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QLatin1String("light"))    { if (ok) *ok = true; return Mode::Light; }
    if (normalized == QLatin1String("dark"))     { if (ok) *ok = true; return Mode::Dark; }
    if (normalized == QLatin1String("schedule")) { if (ok) *ok = true; return Mode::Schedule; }
    if (ok) *ok = (normalized == QLatin1String("system") || normalized.isEmpty());
    return Mode::System;
}

QString ThemeManager::modeLabel(Mode mode)
{
    switch (mode) {
    case Mode::Light:    return QObject::tr("浅色");
    case Mode::Dark:     return QObject::tr("深色");
    case Mode::Schedule: return QObject::tr("按时段自动");
    case Mode::System:   break;
    }
    return QObject::tr("跟随系统");
}

ThemeManager::Scheme ThemeManager::detectSystemScheme() const
{
    // 1) 显式覆盖：打包、测试或用户强制指定时使用。
    const QString override = qEnvironmentVariable("VOILETMARK_THEME").trimmed().toLower();
    if (override == QLatin1String("dark"))
        return Scheme::Dark;
    if (override == QLatin1String("light"))
        return Scheme::Light;

#if defined(Q_OS_WIN)
    QSettings registry(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    const QVariant appsUseLight = registry.value(QStringLiteral("AppsUseLightTheme"));
    if (appsUseLight.isValid())
        return appsUseLight.toInt() == 0 ? Scheme::Dark : Scheme::Light;
    return m_paletteGuess;
#elif defined(Q_OS_MACOS)
    const QString style = runCommand(QStringLiteral("defaults"),
                                     {QStringLiteral("read"), QStringLiteral("-g"),
                                      QStringLiteral("AppleInterfaceStyle")},
                                     500);
    return style.compare(QLatin1String("Dark"), Qt::CaseInsensitive) == 0 ? Scheme::Dark : Scheme::Light;
#else
    // 2) gsettings 需要启动子进程，做 3 秒缓存；找不到命令就整段跳过。
    static const QString gsettings = QStandardPaths::findExecutable(QStringLiteral("gsettings"));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!gsettings.isEmpty()
        && (now - m_lastGsettingsCheck > 3000 || m_cachedColorScheme.isEmpty())) {
        m_cachedColorScheme = runCommand(gsettings,
                                         {QStringLiteral("get"),
                                          QStringLiteral("org.gnome.desktop.interface"),
                                          QStringLiteral("color-scheme")},
                                         500);
        m_cachedGtkTheme = runCommand(gsettings,
                                      {QStringLiteral("get"),
                                       QStringLiteral("org.gnome.desktop.interface"),
                                       QStringLiteral("gtk-theme")},
                                      500);
        m_lastGsettingsCheck = now;
    }

    const QString colorScheme = m_cachedColorScheme.toLower();
    if (colorScheme.contains(QLatin1String("prefer-dark")))
        return Scheme::Dark;
    if (colorScheme.contains(QLatin1String("prefer-light")))
        return Scheme::Light;

    // 3) KDE 全局配色。
    QSettings kdeGlobals(QDir::homePath() + QStringLiteral("/.config/kdeglobals"),
                         QSettings::IniFormat);
    const QString kdeScheme = kdeGlobals.value(QStringLiteral("General/ColorScheme")).toString();
    if (!kdeScheme.isEmpty())
        return looksDark(kdeScheme) ? Scheme::Dark : Scheme::Light;

    // 4) GTK 主题名。
    if (!m_cachedGtkTheme.isEmpty())
        return looksDark(m_cachedGtkTheme) ? Scheme::Dark : Scheme::Light;

    // 5) XFCE 主题名。
    const QString xfceName = xfceThemeName();
    if (!xfceName.isEmpty())
        return looksDark(xfceName) ? Scheme::Dark : Scheme::Light;

    // 6) 兜底。
    return m_paletteGuess;
#endif
}
