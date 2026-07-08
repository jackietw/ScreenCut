/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>

namespace ScreenCut {

/**
 * @brief Global Application Configuration & Logging Manager
 * Handles config.json persistence, debug mode toggling (--debug / -d),
 * and rotating log file output (screencut.log).
 */
class Config {
public:
    static QString getAppConfigDir();
    static QString getConfigPath();
    static QString getLogPath();

    static QJsonObject loadConfig();
    static void saveConfig(const QJsonObject& data);

    static QJsonValue getValue(const QString& key, const QJsonValue& defaultValue = QJsonValue());
    static void setValue(const QString& key, const QJsonValue& value);

    static bool isDebugMode();
    static void setDebugMode(bool enabled);

    static void setupLogging();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void rotateLogsIfNeeded(const QString& logPath);
    
    static QMutex s_mutex;
    static QJsonObject s_cache;
    static bool s_cacheLoaded;
};

} // namespace ScreenCut

#endif // CONFIG_H
