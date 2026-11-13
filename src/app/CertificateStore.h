// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

// What the desk needs to know about the signing certificate, read from the current user's
// personal store ("MY"). Windows only; elsewhere every lookup reports "not found".
struct CertificateInfo {
    bool found = false;
    QString thumbprint; // SHA-1, lower-case hex, no separators
    QString subject;    // simple display name, e.g. "Sigmastone Test Signing"
    QString issuer;
    QDateTime notAfter;
    bool selfSigned = false;
    bool hasPrivateKey = false;
};

namespace CertificateStore {

// The certificate with this thumbprint, or found == false.
CertificateInfo describe(const QString& thumbprint);

// The Windows certificate picker over the user's personal store. Returns the chosen certificate's
// thumbprint, or nothing if the dialog was cancelled. `ownerWindow` is the native handle of the
// window the dialog belongs to, or 0.
std::optional<QString> pickFromUserStore(quintptr ownerWindow);

} // namespace CertificateStore
