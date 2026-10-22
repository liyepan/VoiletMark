#pragma once

#include <QObject>

class QTranslator;

/// 界面语言管理：目前只支持中文（zh_CN）和英文（en）。
///
/// 源码里的界面文案是中文，所以中文不需要额外的翻译文件，
/// 只要把 Qt 自带对话框（文件选择、消息框按钮等）切到中文；
/// 英文则加载一份中文 → 英文的翻译文件。
class LanguageManager : public QObject
{
    Q_OBJECT

public:
    static LanguageManager *instance();

    /// 读取设置（首次按系统语言猜一个）并应用。
    void initialize();

    void setLanguage(const QString &code);
    QString language() const { return m_language; }

    static QStringList availableLanguages();
    static QString displayName(const QString &code);

signals:
    void languageChanged();

private:
    explicit LanguageManager(QObject *parent = nullptr);

    QString m_language = QStringLiteral("zh_CN");
    QTranslator *m_appTranslator = nullptr;
    QTranslator *m_qtTranslator = nullptr;
};
