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
#include <QDebug>
#include "version.h"
#include "config.h"
#include "core/capture_engine.h"
#include "core/common_project.h"
#include "capture/capture_window.h"
#include "resources/IconUtils.h"
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

    QAction* aboutAction = new QAction("ℹ️ About ScreenCut...", trayMenu);
    QAction* quitAction = new QAction("❌ Exit", trayMenu);

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
        CaptureEngine::instance()->startRegionCapture();
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

    // 5. Connect capture completion to launch standalone ScreenCutEditor app via IPC
    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureCompleted, [](const QPixmap& pixmap) {
        qDebug() << "Capture completed! Size:" << pixmap.size();
        if (!pixmap.isNull()) {
            QString libraryDir = ScutProject::getLibraryDir();
            QString scutFilePath = QDir(libraryDir).filePath(
                QString("Capture_%1.scut").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz")));
            
            if (ScutProject::saveImageAsScut(pixmap, scutFilePath)) {
                qDebug() << "Saved capture to My ScreenCut Library .scut file for IPC:" << scutFilePath;
                QString editorPath = QDir(QCoreApplication::applicationDirPath()).filePath("ScreenCutEditor");
#ifdef Q_OS_WIN
                editorPath += ".exe";
#endif
                bool started = QProcess::startDetached(editorPath, { scutFilePath });
                if (!started) {
                    started = QProcess::startDetached("ScreenCutEditor", { scutFilePath });
                }
                if (!started) {
                    QMessageBox::critical(nullptr, "ScreenCut Error", "Failed to launch standalone ScreenCutEditor application!");
                }
            } else {
                QMessageBox::critical(nullptr, "ScreenCut Error", "Failed to save capture as .scut in My ScreenCut Library!");
            }
        }
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureCopied, [&trayIcon](const QPixmap& pixmap) {
        qDebug() << "Capture copied to clipboard! Size:" << pixmap.size();
        QApplication::clipboard()->setPixmap(pixmap);
        trayIcon.showMessage("ScreenCut", "✅ Screenshot copied to clipboard!", QSystemTrayIcon::Information, 2500);
    });

    QObject::connect(CaptureEngine::instance(), &CaptureEngine::captureSaved, [&trayIcon](const QString& filePath) {
        qDebug() << "Capture saved to file:" << filePath;
        trayIcon.showMessage("ScreenCut", QString("✅ Screenshot saved to:\n%1").arg(filePath), QSystemTrayIcon::Information, 3500);
    });

    trayIcon.showMessage("ScreenCut Native Ready", 
                         "ScreenCut 2.0 (C++ Qt6) is running in the background.\nUse Ctrl+Shift+A or click tray icon to capture screen!",
                         QSystemTrayIcon::Information, 3000);

    // 6. Show CaptureMainWindow on launch so user gets immediate visual feedback matching the Python prototype!
    QTimer::singleShot(100, []() {
        CaptureMainWindow::instance()->show();
    });

    return app.exec();
}
