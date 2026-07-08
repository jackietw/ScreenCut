#ifndef COMMON_PROJECT_H
#define COMMON_PROJECT_H

#include <QString>
#include <QPixmap>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>
#include <QBuffer>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include "../version.h"

namespace ScreenCut {

class ScutProject {
public:
    static QString getLibraryDir() {
        QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString libraryDir = QDir(docsDir).filePath("My ScreenCut Library");
        QDir().mkpath(libraryDir);
        return libraryDir;
    }

    static bool saveImageAsScut(const QPixmap& pixmap, const QString& filePath, const QJsonArray& annotations = QJsonArray()) {
        try {
            if (pixmap.isNull()) {
                qWarning() << "[ScutProject] Cannot save null pixmap to scut:" << filePath;
                return false;
            }

            qDebug() << "[ScutProject] Packaging image and" << annotations.size() << "annotations into .scut file...";
            QByteArray imgBa;
            QBuffer imgBuf(&imgBa);
            imgBuf.open(QIODevice::WriteOnly);
            pixmap.save(&imgBuf, "PNG");
            QString imgB64 = QString::fromLatin1(imgBa.toBase64());

            QPixmap thumb = pixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QByteArray thumbBa;
            QBuffer thumbBuf(&thumbBa);
            thumbBuf.open(QIODevice::WriteOnly);
            thumb.save(&thumbBuf, "PNG");
            QString thumbB64 = QString::fromLatin1(thumbBa.toBase64());

            QJsonObject root;
            root["version"] = QString(SCREENCUT_VERSION_STR);
            root["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
            root["image_base64"] = imgB64;
            root["thumbnail_base64"] = thumbB64;
            root["annotations"] = annotations;
            root["default_canvas_w"] = pixmap.width();
            root["default_canvas_h"] = pixmap.height();

            QJsonDocument doc(root);
            QString tmpFilePath = filePath + QString(".tmp.%1").arg(QDateTime::currentMSecsSinceEpoch());
            QFile tmpFile(tmpFilePath);
            if (tmpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                tmpFile.write(doc.toJson(QJsonDocument::Indented));
                tmpFile.close();
                if (QFile::exists(filePath)) {
                    QFile::remove(filePath);
                }
                bool renamed = QFile::rename(tmpFilePath, filePath);
                if (renamed) {
                    qDebug() << "[ScutProject] Successfully packaged and saved .scut project to:" << filePath;
                    return true;
                } else {
                    qWarning() << "[ScutProject] Failed to rename temp scut file to:" << filePath;
                }
            } else {
                qWarning() << "[ScutProject] Failed to open temporary file for scut writing:" << tmpFilePath;
            }
        } catch (...) {
            qWarning() << "[ScutProject] Exception in saveImageAsScut for:" << filePath;
        }
        return false;
    }

    static bool loadScutFile(const QString& filePath, QPixmap& outPixmap, QJsonArray& outAnnotations) {
        qDebug() << "[ScutProject] Loading project file:" << filePath;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "[ScutProject] Failed to open scut file:" << filePath;
            return false;
        }
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[ScutProject] Failed to parse scut JSON:" << error.errorString();
            return false;
        }

        QJsonObject root = doc.object();
        QString imgB64 = root["image_base64"].toString();
        if (imgB64.isEmpty()) {
            qWarning() << "[ScutProject] No image_base64 found in scut file:" << filePath;
            return false;
        }

        QByteArray imgBa = QByteArray::fromBase64(imgB64.toLatin1());
        if (!outPixmap.loadFromData(imgBa, "PNG")) {
            qWarning() << "[ScutProject] Failed to load pixmap from base64 data in:" << filePath;
            return false;
        }

        outAnnotations = root["annotations"].toArray();
        qDebug() << "[ScutProject] Successfully loaded .scut project from:" << filePath << "| Size:" << outPixmap.size() << "| Annotations:" << outAnnotations.size();
        return true;
    }
};

} // namespace ScreenCut

#endif // COMMON_PROJECT_H
