#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

LanguageManager *LanguageManager::instance()
{
    static LanguageManager *manager = new LanguageManager();
    return manager;
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
}

QStringList LanguageManager::availableLanguages()
{
    return {QStringLiteral("zh_CN"), QStringLiteral("en")};
}

QString LanguageManager::displayName(const QString &code)
{
    if (code == QLatin1String("en"))
        return QStringLiteral("English");
    return QStringLiteral("中文");
}

void LanguageManager::initialize()
{
    QSettings settings;
    QString code = settings.value(QStringLiteral("ui/language")).toString();
    if (!availableLanguages().contains(code)) {
        code = QLocale::system().language() == QLocale::English
                   ? QStringLiteral("en")
                   : QStringLiteral("zh_CN");
    }
    setLanguage(code);
}

void LanguageManager::setLanguage(const QString &code)
{
    const QString normalized =
        (code == QLatin1String("en")) ? QStringLiteral("en") : QStringLiteral("zh_CN");

    if (m_appTranslator) {
        QCoreApplication::removeTranslator(m_appTranslator);
        delete m_appTranslator;
        m_appTranslator = nullptr;
    }
    if (m_qtTranslator) {
        QCoreApplication::removeTranslator(m_qtTranslator);
        delete m_qtTranslator;
        m_qtTranslator = nullptr;
    }

    if (normalized == QLatin1String("en")) {
        // 源码文案是中文，英文需要一份中文 → 英文的翻译。
        auto *translator = new QTranslator(this);
        if (translator->load(QStringLiteral(":/translations/voiletmark_en.qm"))) {
            QCoreApplication::installTranslator(translator);
            m_appTranslator = translator;
        } else {
            delete translator;
        }
    } else {
        // 中文界面只需要把 Qt 自带的文件对话框 / 消息框也切过来。
        auto *translator = new QTranslator(this);
        if (translator->load(QStringLiteral(":/translations/qtbase_zh_CN.qm"))) {
            QCoreApplication::installTranslator(translator);
            m_qtTranslator = translator;
        } else {
            delete translator;
        }
    }

    m_language = normalized;
    QSettings().setValue(QStringLiteral("ui/language"), m_language);
    emit languageChanged();
}
