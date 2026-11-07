#include "Authenticator.h"

#include <QHttpServerRequest>

AuthResult UnconfiguredAuthenticator::authenticate(const QHttpServerRequest& request) {
    AuthResult result;
    result.outcome = CheckOutcome::Unverified;
    result.identity =
        QStringLiteral("unverified caller at %1").arg(request.remoteAddress().toString());
    result.detail = QStringLiteral(
        "No authentication scheme is configured yet; approve only a request you started yourself.");
    return result;
}
