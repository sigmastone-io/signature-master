// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "Authenticode.h"

#ifdef Q_OS_WIN

#include <QByteArray>
#include <QDir>

#include <vector>

#include <windows.h>

#include <wincrypt.h>

std::optional<QString> Authenticode::signerThumbprint(const QString& path) {
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    HCERTSTORE store = nullptr;
    HCRYPTMSG message = nullptr;
    DWORD encoding = 0;
    DWORD contentType = 0;
    DWORD formatType = 0;
    if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                          native.c_str(),
                          CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                          CERT_QUERY_FORMAT_FLAG_BINARY,
                          0,
                          &encoding,
                          &contentType,
                          &formatType,
                          &store,
                          &message,
                          nullptr)) {
        return std::nullopt;
    }

    std::optional<QString> result;
    DWORD size = 0;
    if (CryptMsgGetParam(message, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &size) && size > 0) {
        std::vector<BYTE> buffer(size);
        if (CryptMsgGetParam(message, CMSG_SIGNER_INFO_PARAM, 0, buffer.data(), &size)) {
            auto* signer = reinterpret_cast<CMSG_SIGNER_INFO*>(buffer.data());
            CERT_INFO wanted{};
            wanted.Issuer = signer->Issuer;
            wanted.SerialNumber = signer->SerialNumber;
            PCCERT_CONTEXT context =
                CertFindCertificateInStore(store,
                                           X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                           0,
                                           CERT_FIND_SUBJECT_CERT,
                                           &wanted,
                                           nullptr);
            if (context != nullptr) {
                BYTE hash[20];
                DWORD hashSize = sizeof(hash);
                if (CertGetCertificateContextProperty(
                        context, CERT_HASH_PROP_ID, hash, &hashSize)) {
                    result = QString::fromLatin1(
                        QByteArray(reinterpret_cast<const char*>(hash), static_cast<int>(hashSize))
                            .toHex());
                }
                CertFreeCertificateContext(context);
            }
        }
    }
    CryptMsgClose(message);
    CertCloseStore(store, 0);
    return result;
}

#else

std::optional<QString> Authenticode::signerThumbprint(const QString& path) {
    Q_UNUSED(path);
    return std::nullopt;
}

#endif
