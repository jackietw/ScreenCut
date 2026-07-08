/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include "editor_window.h"
#include "../version.h"
#include "../config.h"
#include "../resources/IconUtils.h"

using namespace ScreenCut;

int main(int argc, char *argv[]) {
    // 1. High DPI scaling attributes are enabled by default in Qt 6
    QApplication app(argc, argv);
    app.setApplicationName(QString("%1 Editor").arg(SCREENCUT_APP_NAME));
    app.setOrganizationName(SCREENCUT_ORG_NAME);
    app.setOrganizationDomain(SCREENCUT_ORG_DOMAIN);
    app.setApplicationVersion(SCREENCUT_VERSION_STR);

    // Initialize global configuration and logging system
    Config::setupLogging();
    qDebug() << "[EditorMain] ScreenCut Editor Application started (Standalone Studio). Version:" << SCREENCUT_VERSION_STR << "| DebugMode:" << Config::isDebugMode();

    // 2. Instantiate standalone EditorMainWindow
    EditorMainWindow* editor = EditorMainWindow::instance();

    // 3. Parse command line arguments for image file loading (e.g. from Capture IPC or Windows Explorer)
    QStringList args = app.arguments();
    if (args.size() > 1) {
        QString filePath = args.at(1);
        qDebug() << "Opening file from command line argument:" << filePath;
        if (!editor->loadImageFile(filePath)) {
            QMessageBox::warning(nullptr, "ScreenCut Editor", 
                                 QString("Failed to load image file:\n%1").arg(filePath));
        }
    }

    editor->show();
    editor->activateWindow();
    editor->raise();

    return app.exec();
}
