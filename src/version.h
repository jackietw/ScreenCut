/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef VERSION_H
#define VERSION_H

#define SCREENCUT_VERSION_MAJOR 1
#define SCREENCUT_VERSION_MINOR 0
#define SCREENCUT_VERSION_PATCH 1

#define _SC_STRINGIFY_IMPL(x) #x
#define _SC_STRINGIFY(x) _SC_STRINGIFY_IMPL(x)

#define SCREENCUT_VERSION_STR _SC_STRINGIFY(SCREENCUT_VERSION_MAJOR) "." \
                              _SC_STRINGIFY(SCREENCUT_VERSION_MINOR) "." \
                              _SC_STRINGIFY(SCREENCUT_VERSION_PATCH)
#define SCREENCUT_APP_NAME "ScreenCut"
#define SCREENCUT_ORG_NAME "Jackie"
#define SCREENCUT_ORG_DOMAIN "https://github.com/jackietw/ScreenCut"

#endif // VERSION_H
