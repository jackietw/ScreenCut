/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef ICON_UTILS_H
#define ICON_UTILS_H

#include <QString>
#include <QIcon>
#include <QPixmap>

namespace ScreenCut {

// SVG Definitions matching Python prototype
extern const QString SVG_RECORD;
extern const QString SVG_STOP;
extern const QString SVG_CANCEL;
extern const QString SVG_DONE;
extern const QString SVG_MOUSE;
extern const QString SVG_MIC;
extern const QString SVG_MIC_OFF;
extern const QString SVG_SYS_AUDIO;
extern const QString SVG_SYS_AUDIO_OFF;
extern const QString SVG_EDITOR;
extern const QString SVG_PREF;
extern const QString SVG_MORE;
extern const QString SVG_CLOSE;
extern const QString SVG_ABOUT;
extern const QString SVG_TAB_IMAGE;
extern const QString SVG_TAB_VIDEO;
extern const QString SVG_APP_ICON;
extern const QString SVG_EDITOR_APP_ICON;
extern const QString SVG_SELECT;
extern const QString SVG_ARROW;
extern const QString SVG_TEXT;
extern const QString SVG_SHAPE;
extern const QString SVG_STAMP;
extern const QString SVG_CROP;
extern const QString SVG_BLUR;
extern const QString SVG_PEN;
extern const QString SVG_STEP;
extern const QString SVG_UNDO;
extern const QString SVG_REDO;
extern const QString SVG_SAVE;
extern const QString SVG_COPY;
extern const QString SVG_ZOOM_IN;
extern const QString SVG_ZOOM_OUT;
extern const QString SVG_RECENT;
extern const QString SVG_TAG;
extern const QString SVG_EFFECTS;
extern const QString SVG_PROPERTIES;
extern const QString SVG_OPEN;
extern const QString SVG_RECT;
extern const QString SVG_ELLIPSE;
extern const QString SVG_MOSAIC;
extern const QString SVG_HIGHLIGHT;
extern const QString SVG_COLOR;

// Helper function to render SVG string to QIcon
QIcon createSvgIcon(const QString& svgString, int width = 24, int height = 24);

} // namespace ScreenCut

#endif // ICON_UTILS_H
