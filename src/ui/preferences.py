'''
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.0-or-later
'''

import logging
from PySide6.QtWidgets import QDialog
from PySide6.QtCore import Qt
from config import load_config, save_config
import sounddevice as sd
from ui.preferences_ui import PreferencesUI

class Preferences(PreferencesUI):
    def __init__(self, parent=None):
        super().__init__(parent)

    def load_audio_devices(self):
        try:
            devices = sd.query_devices()
            for d in devices:
                if d['max_input_channels'] > 0:
                    self.cb_audio.addItem(d['name'])
            self.cb_audio.addItem("System Audio (WASAPI Loopback)")
        except Exception as e:
            logging.warning("Error loading sound devices: %s", e)

    def save_and_close(self):
        cfg = load_config()
        cfg["video_fps"] = int(self.cb_fps.currentText())
        cfg["video_res"] = self.cb_res.currentText()
        cfg["audio_source"] = self.cb_audio.currentText()
        cfg["60fps_limit_warning"] = self.toggle_1080p.isChecked()
        save_config(cfg)
        self.accept()

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drag_pos = event.globalPosition().toPoint() - self.frameGeometry().topLeft()
            event.accept()

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MouseButton.LeftButton and hasattr(self, 'drag_pos'):
            self.move(event.globalPosition().toPoint() - self.drag_pos)
            event.accept()

    def mouseReleaseEvent(self, event):
        if hasattr(self, 'drag_pos'):
            del self.drag_pos
            event.accept()
