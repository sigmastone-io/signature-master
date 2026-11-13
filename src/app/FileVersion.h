// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QString>

namespace FileVersion {

// The file version from a Windows executable's version resource ("0.1.0.0"), or empty.
QString of(const QString& path);

} // namespace FileVersion
