/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef MIGRATION_MANAGER_H
#define MIGRATION_MANAGER_H

#include <QString>

namespace ScreenCut {

class MigrationManager {
public:
    static void checkAndMigrate();
};

} // namespace ScreenCut

#endif // MIGRATION_MANAGER_H
