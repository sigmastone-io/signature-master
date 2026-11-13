// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "DisplayPath.h"

#include <QDir>

QString DisplayPath::of(const QString& path) {
    const QString native = QDir::toNativeSeparators(path);
    // %LOCALAPPDATA% and %APPDATA% both sit inside %USERPROFILE%, so they are tried first.
    static const char* const kVariables[] = {"LOCALAPPDATA", "APPDATA", "USERPROFILE"};
    for (const char* name : kVariables) {
        const QString folder = QDir::toNativeSeparators(qEnvironmentVariable(name));
        if (folder.isEmpty()) {
            continue;
        }
        if (native.compare(folder, Qt::CaseInsensitive) == 0 ||
            native.startsWith(folder + QDir::separator(), Qt::CaseInsensitive)) {
            return QLatin1Char('%') + QLatin1String(name) + QLatin1Char('%') +
                   native.mid(folder.size());
        }
    }
    return native;
}
