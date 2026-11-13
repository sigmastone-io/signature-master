// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

#include <optional>

// One line of the signature log: what happened to one file of one request.
struct LogEntry {
    QDateTime time; // UTC
    QString requestId;
    QString repository;
    QString ref;
    QString commit;
    QString file;
    QString sha256;
    QString outcome; // signed, failed, refused, rejected, expired
    QString by;      // the signer, "automatic", "checks", "nobody"
    QString reason;

    QJsonObject toJson() const;
    static std::optional<LogEntry> fromJson(const QJsonObject& json);
};

// The append-only record of every signature, refusal, rejection, expiry and failure: one JSON
// object per line, never rewritten. This is what gets read if the certificate is ever revoked.
class SignatureLog : public QObject {
    Q_OBJECT

public:
    explicit SignatureLog(const QString& path, QObject* parent = nullptr);

    static QString defaultPath();

    QString path() const { return m_path; }
    bool append(const LogEntry& entry);
    QList<LogEntry> readAll() const;

Q_SIGNALS:
    void appended(const LogEntry& entry);

private:
    QString m_path;
};
