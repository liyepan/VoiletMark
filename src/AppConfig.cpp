#include "AppConfig.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

QString configFilePath()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(directory).filePath(QStringLiteral("config.ini"));
}

} // namespace

QString AppConfig::filePath()
{
    return configFilePath();
}

QSettings &AppConfig::settings()
{
    static QSettings *instance = nullptr;
    if (!instance) {
        const QString path = configFilePath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        instance = new QSettings(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        instance->setIniCodec("UTF-8");
#endif
    }
    return *instance;
}
