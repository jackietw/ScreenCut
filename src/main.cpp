/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QApplication>
#include <QClipboard>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QIcon>
#include <QKeySequence>
#include <QShortcut>
#include <QStyle>
#include <QSharedMemory>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "version.h"
#include "config.h"
#include "core/capture_engine.h"
#include "core/common_project.h"
#include "capture/capture_window.h"
#include "resources/IconUtils.h"
#include "widgets/common_notification.h"
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

using namespace ScreenCut;

int main(int argc, char *argv[]) {
    // 1. High DPI scaling attributes are enabled by default in Qt 6
    QApplication app(argc, argv);
    app.setApplicationName(SCREENCUT_APP_NAME);
    app.setOrganizationName(SCREENCUT_ORG_NAME);
    app.setOrganizationDomain(SCREENCUT_ORG_DOMAIN);
    app.setApplicationVersion(SCREENCUT_VERSION_STR);
    app.setQuitOnLastWindowClosed(false); // Keep running in background tray

    QCommandLineParser parser;
    parser.setApplicationDescription("ScreenCut - Native Screen Capture Tool");
    QCommandLineOption versionOption(QStringList() << "v" << "version", "Displays version information.");
    parser.addOption(versionOption);
    parser.addHelpOption();
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
        printf("%s %s\n", SCREENCUT_APP_NAME, SCREENCUT_VERSION_STR);
        fflush(stdout);
        return 0;
    }

    // Initialize global configuration and logging system
    Config::setupLogging();
    qDebug() << "[Main] ScreenCut Application started. Version:" << SCREENCUT_VERSION_STR << "| DebugMode:" << Config::isDebugMode();

    // 2. Single instance lock using QSharedMemory (Temporarily disabled to prevent Windows zombie memory lock during dev!)
    // QSharedMemory sharedMem("ScreenCut_2_0_Native_Qt6_SingleInstanceLock");
    // if (!sharedMem.create(1)) {
    //     QMessageBox::warning(nullptr, "ScreenCut", "ScreenCut is already running in the background system tray!");
    //     return 0;
    // }

    // 3. System Tray Icon Setup
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "ScreenCut", "System tray is not available on this system.");
        return 1;
    }

    QSystemTrayIcon trayIcon;
    
    // Create high-resolution SVG vector icon for System Tray
    QIcon icon = createSvgIcon(SVG_APP_ICON, 64, 64);
    trayIcon.setIcon(icon);
    trayIcon.setToolTip(QString("%1 %2\n0ms Native Screen Capture Engine").arg(SCREENCUT_APP_NAME, SCREENCUT_VERSION_STR));

    QMenu* trayMenu = new QMenu();
    
    QAction* openMainAction = new QAction("📷 Open Capture Window", trayMenu);
    openMainAction->setShortcut(QKeySequence("Ctrl+Shift+C"));

    QAction* regionAction = new QAction("✂️ Region Capture", trayMenu);
    regionAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    
    QAction* windowAction = new QAction("🪟 Window Capture", trayMenu);
    windowAction->setShortcut(QKeySequence("Ctrl+Shift+W"));

    QAction* scrollAction = new QAction("📜 Scrolling Capture", trayMenu);
    scrollAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    QAction* fullScreenAction = new QAction("🖥️ Full Screen", trayMenu);
    fullScreenAction->setShortcut(QKeySequence("Ctrl+Shift+F"));

    trayMenu->addAction(openMainAction);
    trayMenu->addSeparator();
    trayMenu->addAction(regionAction);
    trayMenu->addAction(windowAction);
    trayMenu->addAction(scrollAction);
    trayMenu->addAction(fullScreenAction);
    trayMenu->addSeparator();

    QAction* prefAction = new QAction("⚙️ Preferences...", trayMenu);
    QAction* aboutAction = new QAction("ℹ️ About ScreenCut...", trayMenu);
    QAction* quitAction = new QAction("❌ Exit", trayMenu);

    trayMenu->addAction(prefAction);
    trayMenu->addAction(aboutAction);
    trayMenu->addAction(quitAction);

    trayIcon.setContextMenu(trayMenu);
    trayIcon.show();

    // 4. Connect signals
    QObject::connect(openMainAction, &QAction::triggered, []() {
        CaptureMainWindow::instance()->show();
        CaptureMainWindow::instance()->activateWindow();
        CaptureMainWindow::instance()->raise();
    });
    QObject::connect(regionAction, &QAction::triggered, []() {
        bool scrollCapture = CaptureMainWindow::instance() && CaptureMainWindow::instance()->isSettingEnabled("Scroll Capture");
        if (scrollCapture) {
            CaptureEngine::instance()->startScrollingCapture();
        } else {
            CaptureEngine::instance()->startRegionCapture();
        }
    });
    QObject::connect(windowAction, &QAction::triggered, []() {
        CaptureEngine::instance()->startWindowCapture();
    });
    QObject::connect(scrollAction, &QAction::triggered, []() {
        CaptureEngine::instance()->startScrollingCapture();
    });
    QObject::connect(fullScreenAction, &QAction::triggered, []() {
        CaptureEngine::instance()->startFullScreenCapture();
    });
    QObject::connect(prefAction, &QAction::triggered, []() {
        CaptureMainWindow::instance()->show();
        CaptureMainWindow::instance()->activateWindow();
        CaptureMainWindow::instance()->raise();
        CaptureMainWindow::instance()->showPreferencesOverlay();
    });
    QObject::connect(aboutAction, &QAction::triggered, []() {
        CaptureMainWindow::instance()->show();
        CaptureMainWindow::instance()->activateWindow();
        CaptureMainWindow::instance()->raise();
        CaptureMainWindow::instance()->showAboutOverlay();
    });
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            CaptureMainWindow::instance()->show();
            CaptureMainWindow::instance()->activateWindow();
            CaptureMainWindow::instance()->raise();
        }
    });

    // 5. Connect capture completion and edit requests to check Image tab toggles or launch editor
    auto launchEditorFunc = [](const QString& scutFilePath) {
        qDebug() << "Launching SCEditor for:" << scutFilePath;
        bool started = ScutProject::launchEditor({ scutFilePath });
        if (!started) {
            QMessageBox::critical(nullptr, "ScreenCut Error", "Failed to launch standalone SCEditor application!");
        }
    };

    auto restoreMainWindowFunc = []() {
        if (CaptureMainWindow::instance()) {
            qDebug() << "Restoring CaptureMainWindow automatically after capture completion or cancellation...";
            CaptureMainWindow::instance()->showNormal();
            CaptureMainWindow::instance()->activateWindow();
            CaptureMainWindow::instance()->raise();
        }
    };

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureEdited, [launchEditorFunc, restoreMainWindowFunc](const QPixmap& pixmap) {
        qDebug() << "Capture edited! Size:" << pixmap.size();
        restoreMainWindowFunc();
        if (!pixmap.isNull()) {
            QString libraryDir = ScutProject::getLibraryDir();
            QString scutFilePath = QDir(libraryDir).filePath(
                QString("Capture_%1.scut").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz")));
            if (ScutProject::saveImageAsScut(pixmap, scutFilePath)) {
                launchEditorFunc(scutFilePath);
            } else {
                QMessageBox::critical(nullptr, "ScreenCut Error", "Failed to save capture as .scut in My ScreenCut Library!");
            }
        }
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureCompleted, [launchEditorFunc, restoreMainWindowFunc](const QPixmap& pixmap) {
        qDebug() << "Capture completed! Size:" << pixmap.size();
        restoreMainWindowFunc();
        if (!pixmap.isNull()) {
            QString libraryDir = ScutProject::getLibraryDir();
            QString scutFilePath = QDir(libraryDir).filePath(
                QString("Capture_%1.scut").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz")));
            
            if (ScutProject::saveImageAsScut(pixmap, scutFilePath)) {
                qDebug() << "Saved capture to My ScreenCut Library .scut file for IPC:" << scutFilePath;
                bool previewInEditor = CaptureEngine::instance()->isSessionEditorEnabled();
                bool copyToClipboard = CaptureEngine::instance()->isSessionClipboardEnabled();

                if (copyToClipboard) {
                    qDebug() << "Copy to Clipboard is enabled. Copying to clipboard...";
                    QApplication::clipboard()->setPixmap(pixmap);
                    Notification::showMessage("✅ Screenshot captured & copied to clipboard!", 2500);
                }

                if (previewInEditor) {
                    qDebug() << "Preview in Editor is enabled. Launching SCEditor...";
                    launchEditorFunc(scutFilePath);
                } else if (!copyToClipboard) {
                    Notification::showMessage(QString("✅ Screenshot saved to:\n%1").arg(scutFilePath), 3500);
                }
            } else {
                QMessageBox::critical(nullptr, "ScreenCut Error", "Failed to save capture as .scut in My ScreenCut Library!");
            }
        }
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureCopied, [restoreMainWindowFunc](const QPixmap& pixmap) {
        qDebug() << "Capture copied to clipboard! Size:" << pixmap.size();
        restoreMainWindowFunc();
        QApplication::clipboard()->setPixmap(pixmap);
        Notification::showMessage("✅ Screenshot copied to clipboard!", 2500);
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureSaved, [restoreMainWindowFunc](const QString& filePath) {
        qDebug() << "Capture saved to file:" << filePath;
        restoreMainWindowFunc();
        Notification::showMessage(QString("✅ Screenshot saved to:\n%1").arg(filePath), 3500);
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureCancelled, [restoreMainWindowFunc]() {
        qDebug() << "Capture cancelled by user.";
        restoreMainWindowFunc();
    });

    Notification::showMessage(QString("%1 Ready\nClick tray icon to capture screen!").arg(SCREENCUT_APP_NAME), 3000);

    // 6. Show CaptureMainWindow on launch so user gets immediate visual feedback matching the Python prototype!
    QTimer::singleShot(100, []() {
        CaptureMainWindow::instance()->show();
    });

    return app.exec();
}
