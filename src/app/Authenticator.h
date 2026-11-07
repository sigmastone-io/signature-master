#pragma once

#include "SigningRequest.h"

#include <QString>

class QHttpServerRequest;

// Who is asking. The GitHub scheme is not decided yet, so the one implementation reports every
// caller as unverified: such a request can still be approved by a person, never automatically.
struct AuthResult {
    CheckOutcome outcome = CheckOutcome::Unverified;
    QString identity; // what the desk shows under "Authenticated as"
    QString detail;   // the check's explanation
};

class Authenticator {
public:
    virtual ~Authenticator() = default;
    virtual AuthResult authenticate(const QHttpServerRequest& request) = 0;
};

class UnconfiguredAuthenticator : public Authenticator {
public:
    AuthResult authenticate(const QHttpServerRequest& request) override;
};
