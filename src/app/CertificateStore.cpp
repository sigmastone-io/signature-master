// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "CertificateStore.h"

#include <QByteArray>
#include <QTimeZone>

#ifdef Q_OS_WIN

#include <memory>
#include <vector>

#include <windows.h>

#include <cryptuiapi.h>
#include <wincrypt.h>

namespace {

struct StoreCloser {
    void operator()(HCERTSTORE store) const {
        if (store != nullptr) {
            CertCloseStore(store, 0);
        }
    }
};
using StoreHandle = std::unique_ptr<std::remove_pointer_t<HCERTSTORE>, StoreCloser>;

struct ContextFreer {
    void operator()(PCCERT_CONTEXT context) const {
        if (context != nullptr) {
            CertFreeCertificateContext(context);
        }
    }
};
using ContextHandle = std::unique_ptr<const CERT_CONTEXT, ContextFreer>;

StoreHandle openPersonalStore() {
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                     0,
                                     0,
                                     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG |
                                         CERT_STORE_OPEN_EXISTING_FLAG,
                                     L"MY");
    return StoreHandle(store);
}

QString nameString(PCCERT_CONTEXT context, DWORD flags) {
    const DWORD length =
        CertGetNameStringW(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, flags, nullptr, nullptr, 0);
    if (length <= 1) {
        return {};
    }
    std::vector<wchar_t> buffer(length);
    CertGetNameStringW(
        context, CERT_NAME_SIMPLE_DISPLAY_TYPE, flags, nullptr, buffer.data(), length);
    return QString::fromWCharArray(buffer.data());
}

QString thumbprintOf(PCCERT_CONTEXT context) {
    BYTE hash[20];
    DWORD size = sizeof(hash);
    if (!CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID, hash, &size)) {
        return {};
    }
    return QString::fromLatin1(
        QByteArray(reinterpret_cast<const char*>(hash), static_cast<int>(size)).toHex());
}

QDateTime fromFileTime(const FILETIME& time) {
    ULARGE_INTEGER ticks;
    ticks.LowPart = time.dwLowDateTime;
    ticks.HighPart = time.dwHighDateTime;
    // 100 ns ticks since 1601-01-01 to ms since 1970-01-01.
    constexpr quint64 kEpochDifference = 116444736000000000ULL;
    const quint64 ms = (ticks.QuadPart - kEpochDifference) / 10000ULL;
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms), QTimeZone::UTC);
}

CertificateInfo describeContext(PCCERT_CONTEXT context) {
    CertificateInfo info;
    info.found = true;
    info.thumbprint = thumbprintOf(context);
    info.subject = nameString(context, 0);
    info.issuer = nameString(context, CERT_NAME_ISSUER_FLAG);
    info.notAfter = fromFileTime(context->pCertInfo->NotAfter);
    info.selfSigned = CertCompareCertificateName(X509_ASN_ENCODING,
                                                 &context->pCertInfo->Subject,
                                                 &context->pCertInfo->Issuer) != FALSE;
    DWORD keySpec = 0;
    DWORD size = sizeof(keySpec);
    info.hasPrivateKey = CertGetCertificateContextProperty(
                             context, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size) != FALSE;
    return info;
}

} // namespace

CertificateInfo CertificateStore::describe(const QString& thumbprint) {
    const QByteArray hash = QByteArray::fromHex(thumbprint.toLatin1());
    if (hash.size() != 20) {
        return {};
    }
    const StoreHandle store = openPersonalStore();
    if (!store) {
        return {};
    }
    CRYPT_HASH_BLOB blob;
    blob.cbData = static_cast<DWORD>(hash.size());
    blob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(hash.constData()));
    const ContextHandle context(CertFindCertificateInStore(store.get(),
                                                           X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                           0,
                                                           CERT_FIND_SHA1_HASH,
                                                           &blob,
                                                           nullptr));
    if (!context) {
        return {};
    }
    return describeContext(context.get());
}

std::optional<QString> CertificateStore::pickFromUserStore(quintptr ownerWindow) {
    const StoreHandle store = openPersonalStore();
    if (!store) {
        return std::nullopt;
    }
    const ContextHandle context(CryptUIDlgSelectCertificateFromStore(
        store.get(),
        reinterpret_cast<HWND>(ownerWindow),
        L"Choose the code signing certificate",
        L"Certificates in your personal store. Pick the one with the code signing key.",
        CRYPTUI_SELECT_LOCATION_COLUMN,
        0,
        nullptr));
    if (!context) {
        return std::nullopt;
    }
    return thumbprintOf(context.get());
}

#else

CertificateInfo CertificateStore::describe(const QString& thumbprint) {
    Q_UNUSED(thumbprint);
    return {};
}

std::optional<QString> CertificateStore::pickFromUserStore(quintptr ownerWindow) {
    Q_UNUSED(ownerWindow);
    return std::nullopt;
}

#endif
