#pragma once

#include "Jwt.h"

#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

#include <optional>

// Where the authenticator gets the public key a token names.
class KeySource {
public:
    virtual ~KeySource() = default;
    // The key with this id. `allowRefresh` lets the source go and fetch new keys when the id is
    // not one it holds, which happens after GitHub rotates them.
    virtual std::optional<RsaPublicKey> key(const QString& kid, bool allowRefresh) = 0;
};

// GitHub's signing keys, from its JWKS document, fetched at start and again when a token names
// a key that is not cached, at most once a minute. A refresh waits for the answer (a few hundred
// milliseconds on a good day) so that a request can be answered in one go.
class JwksCache : public QObject, public KeySource {
    Q_OBJECT

public:
    explicit JwksCache(const QUrl& url, QObject* parent = nullptr);

    static QUrl gitHubActionsJwks();

    std::optional<RsaPublicKey> key(const QString& kid, bool allowRefresh) override;
    // Fetches now and waits up to `timeoutMs`. True when keys were read.
    bool refresh(int timeoutMs = 10000);
    void refreshInBackground();
    int count() const { return static_cast<int>(m_keys.size()); }
    QDateTime fetchedAt() const { return m_fetchedAt; }
    QString lastError() const { return m_lastError; }

    static QHash<QString, RsaPublicKey> parse(const QByteArray& jwks);

private:
    QUrl m_url;
    QNetworkAccessManager m_network;
    QHash<QString, RsaPublicKey> m_keys;
    QDateTime m_fetchedAt;
    QDateTime m_lastAttempt;
    QString m_lastError;
};
