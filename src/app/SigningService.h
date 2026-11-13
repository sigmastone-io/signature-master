// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SigningRequest.h"

#include <QObject>
#include <QString>

class RequestStore;
class Settings;
class Signer;

// Takes approved requests through signing, one file at a time, and approves on its own when
// automatic signing is on and every check passed (an unverified check never qualifies).
class SigningService : public QObject {
    Q_OBJECT

public:
    SigningService(Settings* settings,
                   RequestStore* store,
                   Signer* signer,
                   QObject* parent = nullptr);

    // A person (or "automatic") approved the request. False when it is not pending.
    bool approve(const QString& id, const QString& by);
    bool busy() const { return m_busy; }

private:
    void considerAutomatic(const QString& id, RequestState state);
    void pump();
    void
    onFinished(const QString& requestId, const QString& fileName, bool ok, const QString& error);

    Settings* m_settings;
    RequestStore* m_store;
    Signer* m_signer;
    bool m_busy = false;
};
