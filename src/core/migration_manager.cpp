#include "migration_manager.h"
#include "../config.h"
#include "../version.h"
#include "common_project.h"

#include <QVersionNumber>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressDialog>
#include <QProcess>
#include <QApplication>
#include <QDebug>

namespace ScreenCut {

const QString MIN_COMPATIBLE_SCUT_VERSION = "1.0.4";

void MigrationManager::checkAndMigrate() {
    QString lastVerStr = Config::getValue("last_executed_version", "0.0.0").toString();
    QString currentVerStr = SCREENCUT_VERSION_STR;

    QVersionNumber lastVer = QVersionNumber::fromString(lastVerStr);
    QVersionNumber currentVer = QVersionNumber::fromString(currentVerStr);

    if (lastVer == currentVer) {
        return; // No migration needed
    }

    QString libraryDir = ScutProject::getLibraryDir();
    QDir dir(libraryDir);
    QStringList filters;
    filters << "*.scut";
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList scutFiles = dir.entryInfoList();

    if (scutFiles.isEmpty()) {
        Config::setValue("last_executed_version", currentVerStr);
        return;
    }

    if (lastVer < currentVer) {
        // App upgraded
        QMessageBox::StandardButton reply = QMessageBox::question(
            nullptr, "ScreenCut Update",
            "Detected new version. Do you want to optimize the program, check and upgrade all files in the database (My ScreenCut Library) to the latest format?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            QProgressDialog progress("Upgrade Library", "Cancel", 0, scutFiles.size(), nullptr);
            progress.setWindowModality(Qt::WindowModal);
            QVersionNumber minVer = QVersionNumber::fromString(MIN_COMPATIBLE_SCUT_VERSION);

            int count = 0;
            for (const QFileInfo& fi : scutFiles) {
                progress.setValue(count++);
                if (progress.wasCanceled()) break;

                QFile file(fi.absoluteFilePath());
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
                QByteArray data = file.readAll();
                file.close();

                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                if (error.error != QJsonParseError::NoError || !doc.isObject()) continue;

                QJsonObject root = doc.object();
                QString fileVerStr = root["version"].toString("0.0.0");
                QVersionNumber fileVer = QVersionNumber::fromString(fileVerStr);

                if (fileVer < currentVer) {
                    if (fileVer < minVer) {
                        // Too old, rename
                        QString oldPath = fi.absoluteFilePath() + "_old";
                        QFile::rename(fi.absoluteFilePath(), oldPath);
                        qDebug() << "[MigrationManager] Renamed incompatible file:" << fi.absoluteFilePath();
                    } else {
                        // Upgrade safely
                        root["version"] = currentVerStr;
                        QJsonDocument newDoc(root);
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            file.write(newDoc.toJson(QJsonDocument::Indented));
                            file.close();
                            qDebug() << "[MigrationManager] Upgraded file:" << fi.absoluteFilePath();
                        }
                    }
                }
            }
            progress.setValue(scutFiles.size());
        }
    } else {
        // App downgraded
        QMessageBox::StandardButton reply = QMessageBox::warning(
            nullptr, "ScreenCut Downgrade",
            "Detected older version. For the safety of your data, a backup will be created. If no backup is performed, the program will exit.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::No) {
            qDebug() << "[MigrationManager] User refused backup on downgrade. Exiting.";
            exit(0);
        } else {
            // Backup
            QString backupFile = dir.filePath(QString("Backup_%1.zip").arg(lastVerStr));
            qDebug() << "[MigrationManager] Creating backup zip:" << backupFile;

#ifdef Q_OS_WIN
            QProcess process;
            process.setWorkingDirectory(libraryDir);
            process.start("tar", QStringList() << "-a" << "-c" << "-f" << backupFile << "*.scut");
            process.waitForFinished();
#else
            QProcess process;
            process.setWorkingDirectory(libraryDir);
            process.start("zip", QStringList() << backupFile << "*.scut");
            process.waitForFinished();
#endif

            // Force downgrade version string
            QProgressDialog progress("Downgrading Database Files...", "Cancel", 0, scutFiles.size(), nullptr);
            progress.setWindowModality(Qt::WindowModal);
            
            int count = 0;
            for (const QFileInfo& fi : scutFiles) {
                progress.setValue(count++);
                if (progress.wasCanceled()) break;

                QFile file(fi.absoluteFilePath());
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
                QByteArray data = file.readAll();
                file.close();

                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isObject()) continue;

                QJsonObject root = doc.object();
                root["version"] = currentVerStr;
                QJsonDocument newDoc(root);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(newDoc.toJson(QJsonDocument::Indented));
                    file.close();
                }
            }
            progress.setValue(scutFiles.size());
        }
    }

    Config::setValue("last_executed_version", currentVerStr);
}

} // namespace ScreenCut
