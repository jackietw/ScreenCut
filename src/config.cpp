/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDateTime>
#include <QMutexLocker>
#include <QDebug>
#include <iostream>
#include <cstdio>

namespace ScreenCut {

QMutex Config::s_mutex;
QJsonObject Config::s_cache;
bool Config::s_cacheLoaded = false;

QString Config::getAppConfigDir() {
#ifdef Q_OS_WIN
    QString appdata = qEnvironmentVariable("APPDATA");
    if (appdata.isEmpty()) {
        appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QString dirPath = QDir(appdata).filePath("ScreenCut");
#elif defined(Q_OS_MACOS)
    QString dirPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("ScreenCut");
#else
    QString xdgConfig = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (xdgConfig.isEmpty()) {
        xdgConfig = QDir::home().filePath(".config");
    }
    QString dirPath = QDir(xdgConfig).filePath("ScreenCut");
#endif
    QDir().mkpath(dirPath);
    return dirPath;
}

QString Config::getConfigPath() {
    return QDir(getAppConfigDir()).filePath("config.json");
}

QString Config::getLogPath() {
    return QDir(getAppConfigDir()).filePath("screencut.log");
}

QJsonObject Config::loadConfig() {
    QMutexLocker locker(&s_mutex);
    if (s_cacheLoaded) {
        return s_cache;
    }

    QString path = getConfigPath();
    QFile file(path);
    if (!file.exists()) {
        // Check legacy location for migration
        QString legacyPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                 .filePath("ScreenCutLibrary/config.json");
        QFile legacyFile(legacyPath);
        if (legacyFile.exists() && legacyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(legacyFile.readAll());
            s_cache = doc.object();
            legacyFile.close();
        } else {
            // Default configuration
            s_cache = QJsonObject();
            s_cache["debug_mode"] = false;
        }
        locker.unlock();
        saveConfig(s_cache);
        locker.relock();
        s_cacheLoaded = true;
        return s_cache;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        s_cache = doc.object();
        file.close();
    } else {
        s_cache = QJsonObject();
        s_cache["debug_mode"] = false;
    }
    s_cacheLoaded = true;
    return s_cache;
}

void Config::saveConfig(const QJsonObject& data) {
    QMutexLocker locker(&s_mutex);
    s_cache = data;
    s_cacheLoaded = true;

    QString path = getConfigPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(data);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

QJsonValue Config::getValue(const QString& key, const QJsonValue& defaultValue) {
    QJsonObject obj = loadConfig();
    return obj.value(key).isUndefined() ? defaultValue : obj.value(key);
}

void Config::setValue(const QString& key, const QJsonValue& value) {
    QJsonObject obj = loadConfig();
    obj[key] = value;
    saveConfig(obj);
}

bool Config::isDebugMode() {
    // 1. Check command line arguments
    QStringList args = QCoreApplication::arguments();
    if (args.contains("--debug") || args.contains("-d")) {
        return true;
    }
    // 2. Check config.json
    return getValue("debug_mode", false).toBool();
}

void Config::setDebugMode(bool enabled) {
    setValue("debug_mode", enabled);
}

void Config::rotateLogsIfNeeded(const QString& logPath) {
    QFileInfo info(logPath);
    if (info.exists() && info.size() > 2 * 1024 * 1024) { // 2MB limit
        QString backupPath = logPath + ".1";
        if (QFile::exists(backupPath)) {
            QFile::remove(backupPath);
        }
        QFile::rename(logPath, backupPath);
    }
}

void Config::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    Q_UNUSED(context);
    bool debugMode = isDebugMode();
    if (!debugMode && type == QtDebugMsg) {
        return; // Suppress debug messages when not in debug mode
    }

    QString levelStr;
    switch (type) {
    case QtDebugMsg:    levelStr = "[DEBUG]"; break;
    case QtInfoMsg:     levelStr = "[INFO] "; break;
    case QtWarningMsg:  levelStr = "[WARN] "; break;
    case QtCriticalMsg: levelStr = "[CRIT] "; break;
    case QtFatalMsg:    levelStr = "[FATAL]"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formattedMsg = QString("%1 %2 %3").arg(timestamp, levelStr, msg);

    // Write to screencut.log
    QMutexLocker locker(&s_mutex);
    QString logPath = getLogPath();
    rotateLogsIfNeeded(logPath);

    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        file.write(formattedMsg.toUtf8() + "\n");
        file.close();
    }
    locker.unlock();

    // Also output to console
    if (debugMode || type != QtDebugMsg) {
        fprintf(stderr, "%s\n", formattedMsg.toLocal8Bit().constData());
        fflush(stderr);
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void Config::setupLogging() {
    qInstallMessageHandler(messageHandler);
    qDebug() << "[Config] Logging initialized. Log path:" << getLogPath() << "| DebugMode:" << isDebugMode();
}

} // namespace ScreenCut
