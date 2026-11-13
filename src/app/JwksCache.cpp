// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "JwksCache.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

JwksCache::JwksCache(const QUrl& url, QObject* parent) : QObject(parent), m_url(url) {}

QUrl JwksCache::gitHubActionsJwks() {
    return QUrl(QStringLiteral("https://token.actions.githubusercontent.com/.well-known/jwks"));
}

QHash<QString, RsaPublicKey> JwksCache::parse(const QByteArray& jwks) {
    QHash<QString, RsaPublicKey> keys;
    const QJsonDocument document = QJsonDocument::fromJson(jwks);
    const QJsonArray list = document.object().value(QStringLiteral("keys")).toArray();
    for (const QJsonValue& value : list) {
        const QJsonObject object = value.toObject();
        if (object.value(QStringLiteral("kty")).toString() != QStringLiteral("RSA")) {
            continue;
        }
        RsaPublicKey key;
        key.kid = object.value(QStringLiteral("kid")).toString();
        key.modulus = Jwt::base64UrlDecode(object.value(QStringLiteral("n")).toString().toLatin1());
        key.exponent =
            Jwt::base64UrlDecode(object.value(QStringLiteral("e")).toString().toLatin1());
        if (key.kid.isEmpty() || key.modulus.isEmpty() || key.exponent.isEmpty()) {
            continue;
        }
        keys.insert(key.kid, key);
    }
    return keys;
}

std::optional<RsaPublicKey> JwksCache::key(const QString& kid, bool allowRefresh) {
    if (m_keys.contains(kid)) {
        return m_keys.value(kid);
    }
    if (allowRefresh &&
        (!m_lastAttempt.isValid() || m_lastAttempt.secsTo(QDateTime::currentDateTimeUtc()) >= 60)) {
        refresh();
        if (m_keys.contains(kid)) {
            return m_keys.value(kid);
        }
    }
    return std::nullopt;
}

bool JwksCache::refresh(int timeoutMs) {
    m_lastAttempt = QDateTime::currentDateTimeUtc();
    QNetworkReply* reply = m_network.get(QNetworkRequest(m_url));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutMs);
    loop.exec();
    reply->deleteLater();
    if (!reply->isFinished()) {
        reply->abort();
        m_lastError = QStringLiteral("no answer from %1 within %2 s")
                          .arg(m_url.toString())
                          .arg(timeoutMs / 1000);
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        return false;
    }
    const QHash<QString, RsaPublicKey> keys = parse(reply->readAll());
    if (keys.isEmpty()) {
        m_lastError = QStringLiteral("%1 holds no RSA keys").arg(m_url.toString());
        return false;
    }
    m_keys = keys;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    m_lastError.clear();
    return true;
}

void JwksCache::refreshInBackground() {
    m_lastAttempt = QDateTime::currentDateTimeUtc();
    QNetworkReply* reply = m_network.get(QNetworkRequest(m_url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = reply->errorString();
            return;
        }
        const QHash<QString, RsaPublicKey> keys = parse(reply->readAll());
        if (!keys.isEmpty()) {
            m_keys = keys;
            m_fetchedAt = QDateTime::currentDateTimeUtc();
            m_lastError.clear();
        }
    });
}
