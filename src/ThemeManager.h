#pragma once

#include <QColor>
#include <QObject>

class QTimer;

/// 一份完整的主题配色。界面控件与语法高亮共用同一套颜色，
/// 切换方案时只需要重建这一个结构体。
struct ThemeColors
{
    // 窗口与控件
    QColor window;
    QColor windowText;
    QColor base;
    QColor alternateBase;
    QColor text;
    QColor button;
    QColor buttonText;
    QColor border;
    QColor accent;
    QColor highlight;
    QColor highlightedText;
    QColor disabled;
    QColor placeholder;
    QColor menuBackground;
    QColor scrollHandle;
    QColor scrollHandleHover;
    QColor sidebarBackground;
    QColor hoverBackground;

    // 编辑器
    QColor editorBase;
    QColor editorText;
    QColor gutterBackground;
    QColor gutterText;
    QColor gutterActiveText;
    QColor currentLine;

    // Markdown 语法
    QColor heading;
    QColor headingMarker;
    QColor strong;
    QColor emphasis;
    QColor code;
    QColor codeBackground;
    QColor codeKeyword;
    QColor codeString;
    QColor codeNumber;
    QColor codeComment;
    QColor codePreprocessor;
    QColor link;
    QColor linkUrl;
    QColor quote;
    QColor quoteMarker;
    QColor listMarker;
    QColor rule;
    QColor taskDone;
};

/// 负责“跟随系统自动切换浅色 / 深色主题”。
///
/// 四种模式：跟随系统 / 浅色 / 深色 / 按时段自动（6:00 起浅色、18:00 起深色）。
/// 跟随系统时的检测顺序：显式覆盖（环境变量 / 命令行）→ 平台自身设置
/// （Windows 注册表、macOS defaults、Linux 的 KDE / GTK / XFCE / gsettings）
/// → 启动时的调色板亮度兜底。
/// 每 2 秒轮询一次，变化后立即重绘整个界面。
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class Mode { System, Light, Dark, Schedule };
    enum class Scheme { Light, Dark };

    static ThemeManager *instance();

    /// 由 main() 在 QApplication 之后调用一次。
    void initialize(Mode forcedMode, bool persist);

    Mode mode() const { return m_mode; }
    void setMode(Mode mode, bool persist = true);

    Scheme scheme() const { return m_scheme; }
    const ThemeColors &colors() const { return m_colors; }

    static QString modeToString(Mode mode);
    static Mode modeFromString(const QString &value, bool *ok = nullptr);
    static QString modeLabel(Mode mode);

signals:
    void schemeChanged(ThemeManager::Scheme scheme);

private slots:
    void pollSystemTheme();

private:
    explicit ThemeManager(QObject *parent = nullptr);

    void applyScheme(Scheme scheme, bool force);
    void buildColors(Scheme scheme);
    void applyToApplication();
    QString styleSheet() const;

    Scheme detectSystemScheme() const;
    /// 6:00–18:00 浅色，其余时间深色。
    Scheme scheduleScheme() const;
    Scheme schemeForMode(Mode mode) const;

    Mode m_mode = Mode::System;
    Scheme m_scheme = Scheme::Light;
    ThemeColors m_colors;

    QTimer *m_pollTimer = nullptr;

    /// 启动时读取的 QPalette 亮度，作为所有检测手段失败后的兜底。
    Scheme m_paletteGuess = Scheme::Light;

    /// gsettings 调用比较昂贵，做一次短时缓存。
    mutable qint64 m_lastGsettingsCheck = 0;
    mutable QString m_cachedColorScheme;
    mutable QString m_cachedGtkTheme;
};
