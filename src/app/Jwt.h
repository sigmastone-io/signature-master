// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include <optional>

// An RSA public key as a JWKS lists it: modulus and exponent, big-endian bytes.
struct RsaPublicKey {
    QString kid;
    QByteArray modulus;
    QByteArray exponent;
};

// A parsed JSON Web Token, and RS256 verification with OpenSSL.
struct Jwt {
    QJsonObject header;
    QJsonObject payload;
    QByteArray signingInput; // "header.payload" as sent
    QByteArray signature;    // decoded

    static std::optional<Jwt> parse(const QByteArray& token);
    bool verifyRs256(const RsaPublicKey& key) const;

    static QByteArray base64UrlDecode(const QByteArray& text);
    static QByteArray base64UrlEncode(const QByteArray& bytes);
};
