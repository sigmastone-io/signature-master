#include "Authenticator.h"

#include "JwksCache.h"
#include "Jwt.h"
#include "Settings.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>

AuthResult UnconfiguredAuthenticator::authenticate(const QByteArray& bearer,
                                                   const QByteArray& body,
                                                   const QJsonObject& manifest,
                                                   const QString& remoteAddress) {
    Q_UNUSED(bearer);
    Q_UNUSED(body);
    Q_UNUSED(manifest);
    AuthResult result;
    result.outcome = CheckOutcome::Unverified;
    result.identity = QStringLiteral("unverified caller at %1").arg(remoteAddress);
    result.detail = QStringLiteral(
        "No authentication scheme is configured yet; approve only a request you started yourself.");
    return result;
}

GitHubOidcAuthenticator::GitHubOidcAuthenticator(Settings* settings, KeySource* keys)
    : m_settings(settings), m_keys(keys) {}

QString GitHubOidcAuthenticator::audienceFor(const QByteArray& body) {
    return QStringLiteral("signature-master:") +
           QString::fromLatin1(QCryptographicHash::hash(body, QCryptographicHash::Sha256).toHex());
}

namespace {

AuthResult failed(const QString& detail) {
    AuthResult result;
    result.outcome = CheckOutcome::Failed;
    result.identity = QStringLiteral("token refused");
    result.detail = detail;
    return result;
}

QString claim(const QJsonObject& payload, const char* name) {
    const QJsonValue value = payload.value(QLatin1String(name));
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 0);
    }
    return value.toString();
}

QString manifestField(const QJsonObject& manifest, const char* name) {
    const QJsonValue value = manifest.value(QLatin1String(name));
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 0);
    }
    return value.toString().trimmed();
}

} // namespace

AuthResult GitHubOidcAuthenticator::authenticate(const QByteArray& bearer,
                                                 const QByteArray& body,
                                                 const QJsonObject& manifest,
                                                 const QString& remoteAddress) {
    if (bearer.trimmed().isEmpty()) {
        AuthResult result;
        result.outcome = CheckOutcome::Unverified;
        result.identity = QStringLiteral("unverified caller at %1").arg(remoteAddress);
        result.detail =
            QStringLiteral("No token: a GitHub Actions run with id-token: write sends one. "
                           "Approve only a request you started yourself.");
        return result;
    }

    const std::optional<Jwt> jwt = Jwt::parse(bearer);
    if (!jwt.has_value()) {
        return failed(QStringLiteral("the bearer token is not a JWT"));
    }
    if (jwt->header.value(QStringLiteral("alg")).toString() != QStringLiteral("RS256")) {
        return failed(QStringLiteral("the token is not RS256"));
    }
    const QString kid = jwt->header.value(QStringLiteral("kid")).toString();
    const std::optional<RsaPublicKey> key = m_keys->key(kid, true);
    if (!key.has_value()) {
        return failed(
            QStringLiteral("the token names a key (%1) GitHub does not publish").arg(kid));
    }
    if (!jwt->verifyRs256(*key)) {
        return failed(QStringLiteral("the token's signature does not verify"));
    }

    const QJsonObject& payload = jwt->payload;
    if (claim(payload, "iss") != QLatin1String(kIssuer)) {
        return failed(QStringLiteral("the token was issued by %1, not by GitHub Actions")
                          .arg(claim(payload, "iss")));
    }
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 exp = static_cast<qint64>(payload.value(QStringLiteral("exp")).toDouble());
    if (exp == 0 || exp + kLeewaySeconds < now) {
        return failed(QStringLiteral("the token expired at %1; the job has to be re-run")
                          .arg(QDateTime::fromSecsSinceEpoch(exp).toLocalTime().toString(
                              QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    }
    const qint64 nbf = static_cast<qint64>(payload.value(QStringLiteral("nbf")).toDouble());
    if (nbf > now + kLeewaySeconds) {
        return failed(QStringLiteral("the token is not valid yet"));
    }

    // The audience binds the token to these exact bytes.
    const QString wanted = audienceFor(body);
    const QJsonValue aud = payload.value(QStringLiteral("aud"));
    bool audienceMatches = false;
    if (aud.isString()) {
        audienceMatches = aud.toString() == wanted;
    } else if (aud.isArray()) {
        for (const QJsonValue& value : aud.toArray()) {
            audienceMatches = audienceMatches || value.toString() == wanted;
        }
    }
    if (!audienceMatches) {
        return failed(
            QStringLiteral("the token is bound to a different manifest (audience mismatch)"));
    }

    // What the token says must be what the manifest says.
    const QString owner = m_settings->allowedOwner();
    if (claim(payload, "repository_owner") != owner) {
        return failed(QStringLiteral("the token belongs to %1, not to %2")
                          .arg(claim(payload, "repository_owner"), owner));
    }
    struct Pair {
        const char* claimName;
        const char* manifestName;
        const char* label;
    };
    const Pair pairs[] = {
        {"repository", "repository", "repository"},
        {"ref", "ref", "ref"},
        {"sha", "commit", "commit"},
        {"run_id", "run_id", "run id"},
    };
    for (const Pair& pair : pairs) {
        const QString fromToken = claim(payload, pair.claimName);
        const QString fromManifest = manifestField(manifest, pair.manifestName);
        if (fromToken.isEmpty()) {
            return failed(QStringLiteral("the token carries no %1").arg(QLatin1String(pair.label)));
        }
        if (fromToken != fromManifest) {
            return failed(
                QStringLiteral("the token says %1 %2, the manifest says %3")
                    .arg(QLatin1String(pair.label),
                         fromToken,
                         fromManifest.isEmpty() ? QStringLiteral("nothing") : fromManifest));
        }
    }

    AuthResult result;
    result.outcome = CheckOutcome::Passed;
    result.identity =
        QStringLiteral("GitHub Actions, %1 run %2 by %3")
            .arg(claim(payload, "repository"), claim(payload, "run_id"), claim(payload, "actor"));
    result.detail =
        QStringLiteral("%1, %2").arg(claim(payload, "workflow_ref"), claim(payload, "event_name"));
    return result;
}
