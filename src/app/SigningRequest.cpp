// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "SigningRequest.h"

#include <QJsonArray>

QString toString(RequestState state) {
    switch (state) {
    case RequestState::Received:
        return QStringLiteral("received");
    case RequestState::Pending:
        return QStringLiteral("pending");
    case RequestState::Refused:
        return QStringLiteral("refused");
    case RequestState::Signing:
        return QStringLiteral("signing");
    case RequestState::Signed:
        return QStringLiteral("signed");
    case RequestState::Failed:
        return QStringLiteral("failed");
    case RequestState::Rejected:
        return QStringLiteral("rejected");
    case RequestState::Expired:
        return QStringLiteral("expired");
    }
    return {};
}

QString toLabel(RequestState state) {
    switch (state) {
    case RequestState::Received:
        return QStringLiteral("Receiving files");
    case RequestState::Pending:
        return QStringLiteral("Awaiting approval");
    case RequestState::Refused:
        return QStringLiteral("Refused");
    case RequestState::Signing:
        return QStringLiteral("Signing");
    case RequestState::Signed:
        return QStringLiteral("Signed");
    case RequestState::Failed:
        return QStringLiteral("Failed");
    case RequestState::Rejected:
        return QStringLiteral("Rejected");
    case RequestState::Expired:
        return QStringLiteral("Expired");
    }
    return {};
}

QString toString(FileState state) {
    switch (state) {
    case FileState::Expected:
        return QStringLiteral("expected");
    case FileState::Received:
        return QStringLiteral("received");
    case FileState::Signing:
        return QStringLiteral("signing");
    case FileState::Signed:
        return QStringLiteral("signed");
    case FileState::Failed:
        return QStringLiteral("failed");
    }
    return {};
}

QString toString(CheckOutcome outcome) {
    switch (outcome) {
    case CheckOutcome::Pending:
        return QStringLiteral("pending");
    case CheckOutcome::Passed:
        return QStringLiteral("passed");
    case CheckOutcome::Failed:
        return QStringLiteral("failed");
    case CheckOutcome::Unverified:
        return QStringLiteral("unverified");
    }
    return {};
}

bool isTerminal(RequestState state) {
    switch (state) {
    case RequestState::Received:
    case RequestState::Pending:
    case RequestState::Signing:
        return false;
    case RequestState::Refused:
    case RequestState::Signed:
    case RequestState::Failed:
    case RequestState::Rejected:
    case RequestState::Expired:
        return true;
    }
    return true;
}

QJsonObject Check::toJson() const {
    return {
        {QStringLiteral("key"), key},
        {QStringLiteral("title"), title},
        {QStringLiteral("outcome"), toString(outcome)},
        {QStringLiteral("detail"), detail},
    };
}

QJsonObject RequestFile::toJson() const {
    QJsonObject json{
        {QStringLiteral("name"), name},
        {QStringLiteral("size"), size},
        {QStringLiteral("sha256"), sha256},
        {QStringLiteral("state"), toString(state)},
    };
    if (!version.isEmpty()) {
        json.insert(QStringLiteral("version"), version);
    }
    if (!error.isEmpty()) {
        json.insert(QStringLiteral("error"), error);
    }
    return json;
}

QString SigningRequest::owner() const {
    return repository.section(QLatin1Char('/'), 0, 0);
}

QString SigningRequest::name() const {
    return repository.section(QLatin1Char('/'), 1, 1);
}

QString SigningRequest::refShort() const {
    const QString tags = QStringLiteral("refs/tags/");
    return ref.startsWith(tags) ? ref.mid(tags.size()) : ref;
}

bool SigningRequest::allFilesReceived() const {
    for (const RequestFile& file : files) {
        if (file.state == FileState::Expected || file.state == FileState::Failed) {
            return false;
        }
    }
    return !files.isEmpty();
}

bool SigningRequest::anyCheckFailed() const {
    for (const Check& item : checks) {
        if (item.outcome == CheckOutcome::Failed) {
            return true;
        }
    }
    return false;
}

bool SigningRequest::anyCheckUnverified() const {
    for (const Check& item : checks) {
        if (item.outcome == CheckOutcome::Unverified) {
            return true;
        }
    }
    return false;
}

const Check* SigningRequest::check(const QString& key) const {
    for (const Check& item : checks) {
        if (item.key == key) {
            return &item;
        }
    }
    return nullptr;
}

Check* SigningRequest::check(const QString& key) {
    for (Check& item : checks) {
        if (item.key == key) {
            return &item;
        }
    }
    return nullptr;
}

QJsonObject SigningRequest::toJson() const {
    QJsonArray fileArray;
    for (const RequestFile& file : files) {
        fileArray.append(file.toJson());
    }
    QJsonArray checkArray;
    for (const Check& item : checks) {
        checkArray.append(item.toJson());
    }
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("state"), toString(state)},
        {QStringLiteral("reason"), reason.isEmpty() ? QJsonValue() : QJsonValue(reason)},
        {QStringLiteral("repository"), repository},
        {QStringLiteral("ref"), ref},
        {QStringLiteral("commit"), commit},
        {QStringLiteral("workflow"), workflow},
        {QStringLiteral("job"), job},
        {QStringLiteral("run_id"), runId},
        {QStringLiteral("run_url"), runUrl},
        {QStringLiteral("actor"), actor},
        {QStringLiteral("authenticated_as"), authenticatedAs},
        {QStringLiteral("requested_at"), requestedAt.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("updated_at"), updatedAt.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("approved_by"), approvedBy},
        {QStringLiteral("files"), fileArray},
        {QStringLiteral("checks"), checkArray},
    };
}
