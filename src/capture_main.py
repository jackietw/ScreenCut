'''
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.0-or-later
'''

import sys
import os

from PySide6.QtWidgets import QApplication, QSystemTrayIcon, QMenu
from PySide6.QtGui import QIcon, QAction
from ui.main_window import MainWindow

def get_documents_folder():
    if os.name == 'nt':
        import ctypes.wintypes
        CSIDL_PERSONAL = 5
        SHGFP_TYPE_CURRENT = 0
        buf = ctypes.create_unicode_buffer(ctypes.wintypes.MAX_PATH)
        ctypes.windll.shell32.SHGetFolderPathW(None, CSIDL_PERSONAL, None, SHGFP_TYPE_CURRENT, buf)
        return buf.value
    return os.path.join(os.path.expanduser("~"), "Documents")

def global_exception_handler(exc_type, exc_value, exc_tb):
    import traceback
    from PySide6.QtWidgets import QMessageBox
    error_msg = "".join(traceback.format_exception(exc_type, exc_value, exc_tb))
    with open("crash.log", "a") as f:
        f.write(error_msg + "\n")
    print(error_msg)
    QMessageBox.critical(None, "ScreenCut Error", f"An unexpected error occurred:\n\n{error_msg}")

def main():
    import sys
    sys.excepthook = global_exception_handler
    app = QApplication(sys.argv)
    
    # Keep application running when the main window is closed
    app.setQuitOnLastWindowClosed(False)

    # Ensure library folder exists
    docs_dir = get_documents_folder()
    library_dir = os.path.join(docs_dir, "My ScreenCut Library")
    os.makedirs(library_dir, exist_ok=True)

    window = MainWindow(library_dir)
    
    # Setup System Tray
    tray_icon = QSystemTrayIcon(window)
    # Using a standard system icon as placeholder
    icon = window.style().standardIcon(window.style().StandardPixmap.SP_ComputerIcon)
    tray_icon.setIcon(icon)
    
    tray_menu = QMenu()
    
    show_action = QAction("Open ScreenCut", window)
    show_action.triggered.connect(window.showNormal)
    tray_menu.addAction(show_action)
    
    capture_action = QAction("Capture", window)
    capture_action.triggered.connect(window.start_capture)
    tray_menu.addAction(capture_action)
    
    tray_menu.addSeparator()
    
    quit_action = QAction("Quit", window)
    quit_action.triggered.connect(app.quit)
    tray_menu.addAction(quit_action)
    
    tray_icon.setContextMenu(tray_menu)
    tray_icon.show()
    
    # Double click tray icon to show window
    tray_icon.activated.connect(
        lambda reason: window.showNormal() if reason == QSystemTrayIcon.ActivationReason.DoubleClick else None
    )

    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
