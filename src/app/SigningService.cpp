// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "SigningService.h"

#include "RequestStore.h"
#include "Settings.h"
#include "Signer.h"

#include <QDir>

SigningService::SigningService(Settings* settings,
                               RequestStore* store,
                               Signer* signer,
                               QObject* parent)
    : QObject(parent), m_settings(settings), m_store(store), m_signer(signer) {
    connect(m_store, &RequestStore::entered, this, &SigningService::considerAutomatic);
    connect(m_signer, &Signer::finished, this, &SigningService::onFinished);
    connect(m_signer,
            &Signer::progress,
            m_store,
            [this](const QString& requestId, const QString& fileName, const QString& message) {
                m_store->noteProgress(requestId, fileName, message);
            });
}

bool SigningService::approve(const QString& id, const QString& by) {
    if (!m_store->beginSigning(id, by)) {
        return false;
    }
    pump();
    return true;
}

void SigningService::considerAutomatic(const QString& id, RequestState state) {
    if (state != RequestState::Pending || !m_settings->autoSign()) {
        return;
    }
    const std::optional<SigningRequest> request = m_store->find(id);
    if (!request.has_value() || request->anyCheckFailed() || request->anyCheckUnverified()) {
        return;
    }
    approve(id, QStringLiteral("automatic"));
}

void SigningService::pump() {
    if (m_busy) {
        return;
    }
    for (const SigningRequest& request : m_store->requests()) {
        if (request.state != RequestState::Signing) {
            continue;
        }
        for (const RequestFile& file : request.files) {
            if (file.state != FileState::Received) {
                continue;
            }
            const QString output =
                QDir(request.workDir).filePath(QStringLiteral("signed/") + file.name);
            m_busy = true;
            m_store->markFileSigning(request.id, file.name);
            m_signer->sign(request.id, file.name, file.path, output);
            return;
        }
    }
}

void SigningService::onFinished(const QString& requestId,
                                const QString& fileName,
                                bool ok,
                                const QString& error) {
    m_busy = false;
    if (ok) {
        const std::optional<SigningRequest> request = m_store->find(requestId);
        const QString output =
            request.has_value()
                ? QDir(request->workDir).filePath(QStringLiteral("signed/") + fileName)
                : QString();
        m_store->markFileSigned(requestId, fileName, output);
    } else {
        m_store->markFileFailed(requestId, fileName, error);
    }
    pump();
}
