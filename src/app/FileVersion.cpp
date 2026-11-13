// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "FileVersion.h"

#ifdef Q_OS_WIN

#include <QDir>

#include <vector>

#include <windows.h>

QString FileVersion::of(const QString& path) {
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(native.c_str(), &handle);
    if (size == 0) {
        return {};
    }
    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(native.c_str(), 0, size, buffer.data())) {
        return {};
    }
    VS_FIXEDFILEINFO* info = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID*>(&info), &length) ||
        info == nullptr || length == 0) {
        return {};
    }
    return QStringLiteral("%1.%2.%3.%4")
        .arg(HIWORD(info->dwFileVersionMS))
        .arg(LOWORD(info->dwFileVersionMS))
        .arg(HIWORD(info->dwFileVersionLS))
        .arg(LOWORD(info->dwFileVersionLS));
}

#else

QString FileVersion::of(const QString& path) {
    Q_UNUSED(path);
    return {};
}

#endif
