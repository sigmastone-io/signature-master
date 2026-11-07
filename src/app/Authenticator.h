#pragma once

#include "SigningRequest.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>

class KeySource;
class Settings;

// Who is asking, and whether that can be trusted.
struct AuthResult {
    CheckOutcome outcome = CheckOutcome::Unverified;
    QString identity; // what the desk shows under "Authenticated as"
    QString detail;   // the check's explanation
};

class Authenticator {
public:
    virtual ~Authenticator() = default;
    // `bearer` is the Authorization header's token, or empty; `body` the request's exact bytes
    // and `manifest` the same parsed, so that a token can be bound to them and its claims
    // compared with what the manifest says.
    virtual AuthResult authenticate(const QByteArray& bearer,
                                    const QByteArray& body,
                                    const QJsonObject& manifest,
                                    const QString& remoteAddress) = 0;
};

// No scheme at all: every caller is unverified. For tests.
class UnconfiguredAuthenticator : public Authenticator {
public:
    AuthResult authenticate(const QByteArray& bearer,
                            const QByteArray& body,
                            const QJsonObject& manifest,
                            const QString& remoteAddress) override;
};

// A GitHub Actions OIDC token, bound to the request (decided 2026-11-07):
//
//   - no token: the caller is unverified; a person may still approve, never automatic;
//   - a token: RS256 against GitHub's JWKS, issuer token.actions.githubusercontent.com, not
//     expired, audience "signature-master:<sha256 of the request body>", so a token cannot be
//     reused for other files; repository_owner must be the allowed owner, and the repository,
//     ref, sha and run_id claims must say what the manifest says. Anything wrong with a token
//     that is present fails the check and refuses the request.
class GitHubOidcAuthenticator : public Authenticator {
public:
    GitHubOidcAuthenticator(Settings* settings, KeySource* keys);

    AuthResult authenticate(const QByteArray& bearer,
                            const QByteArray& body,
                            const QJsonObject& manifest,
                            const QString& remoteAddress) override;

    static QString audienceFor(const QByteArray& body);
    static constexpr auto kIssuer = "https://token.actions.githubusercontent.com";
    static constexpr int kLeewaySeconds = 60;

private:
    Settings* m_settings;
    KeySource* m_keys;
};
