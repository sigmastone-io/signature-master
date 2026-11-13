// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QString>

namespace DisplayPath {

// A path as the window shows it: native separators, and the profile folders written as the
// variables that name them ("%APPDATA%\Sigmastone\SignatureMaster.ini"), so the window never
// carries the user's name. For display only; never pass the result to a file API.
QString of(const QString& path);

} // namespace DisplayPath
