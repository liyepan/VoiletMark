#pragma once

#include <QSettings>
#include <QString>

/// 统一的应用配置：所有设置写进同一个 INI 文件，
/// 而不是散落在系统注册表 / 各自的配置目录里。
///
/// Linux: ~/.config/VoiletMark/VoiletMark/config.ini
/// Windows: %LOCALAPPDATA%/VoiletMark/VoiletMark/config.ini
/// macOS: ~/Library/Preferences/VoiletMark/VoiletMark/config.ini
namespace AppConfig {

QSettings &settings();
QString filePath();

} // namespace AppConfig
