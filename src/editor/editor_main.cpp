/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "editor_window.h"
#include "../version.h"
#include "../config.h"
#include "../resources/IconUtils.h"
#include <QFontDatabase>
#include <QFont>

using namespace ScreenCut;

int main(int argc, char *argv[]) {
    // 1. High DPI scaling attributes are enabled by default in Qt 6
    QApplication app(argc, argv);
    app.setApplicationName(QString("%1 Editor").arg(SCREENCUT_APP_NAME));
    app.setOrganizationName(SCREENCUT_ORG_NAME);
    app.setOrganizationDomain(SCREENCUT_ORG_DOMAIN);
    app.setApplicationVersion(SCREENCUT_VERSION_STR);

    int fontId = QFontDatabase::addApplicationFont(":/fonts/NotoSans-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/NotoSans-SemiBold.ttf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont defaultFont(family);
        defaultFont.setPixelSize(13);
        app.setFont(defaultFont);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("ScreenCut Editor - Screenshot Annotation Tool");
    QCommandLineOption versionOption(QStringList() << "v" << "version", "Displays version information.");
    parser.addOption(versionOption);
    parser.addHelpOption();
    parser.addPositionalArgument("file", "Image file to open.");
    QCommandLineOption debugOption(QStringList() << "d" << "debug", "Enable debug logging output.");
    parser.addOption(debugOption);

#ifdef Q_OS_WIN
    if (argc > 1 && AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
#endif

    parser.process(app);

    if (parser.isSet(versionOption)) {
        printf("%s Editor %s\n", SCREENCUT_APP_NAME, SCREENCUT_VERSION_STR);
        fflush(stdout);
        return 0;
    }

    // Initialize global configuration and logging system
    Config::setupLogging();
    qDebug() << "[EditorMain] ScreenCut Editor Application started (Standalone Studio). Version:" << SCREENCUT_VERSION_STR << "| DebugMode:" << Config::isDebugMode();

    // 2. Instantiate standalone EditorMainWindow
    EditorMainWindow* editor = EditorMainWindow::instance();

    // 3. Parse command line positional arguments for image file loading (e.g. from Capture IPC or Windows Explorer)
    const QStringList posArgs = parser.positionalArguments();
    if (!posArgs.isEmpty()) {
        QString filePath = posArgs.first();
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
