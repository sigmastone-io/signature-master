#include "SignatureLog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>

QJsonObject LogEntry::toJson() const {
    return {
        {QStringLiteral("time"), time.toUTC().toString(Qt::ISODateWithMs)},
        {QStringLiteral("request"), requestId},
        {QStringLiteral("repository"), repository},
        {QStringLiteral("ref"), ref},
        {QStringLiteral("commit"), commit},
        {QStringLiteral("file"), file},
        {QStringLiteral("sha256"), sha256},
        {QStringLiteral("outcome"), outcome},
        {QStringLiteral("by"), by},
        {QStringLiteral("reason"), reason},
    };
}

std::optional<LogEntry> LogEntry::fromJson(const QJsonObject& json) {
    LogEntry entry;
    entry.time =
        QDateTime::fromString(json.value(QStringLiteral("time")).toString(), Qt::ISODateWithMs);
    if (!entry.time.isValid()) {
        return std::nullopt;
    }
    entry.requestId = json.value(QStringLiteral("request")).toString();
    entry.repository = json.value(QStringLiteral("repository")).toString();
    entry.ref = json.value(QStringLiteral("ref")).toString();
    entry.commit = json.value(QStringLiteral("commit")).toString();
    entry.file = json.value(QStringLiteral("file")).toString();
    entry.sha256 = json.value(QStringLiteral("sha256")).toString();
    entry.outcome = json.value(QStringLiteral("outcome")).toString();
    entry.by = json.value(QStringLiteral("by")).toString();
    entry.reason = json.value(QStringLiteral("reason")).toString();
    return entry;
}

SignatureLog::SignatureLog(const QString& path, QObject* parent) : QObject(parent), m_path(path) {}

QString SignatureLog::defaultPath() {
    return QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/signatures.log"));
}

bool SignatureLog::append(const LogEntry& entry) {
    QFileInfo(m_path).dir().mkpath(QStringLiteral("."));
    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    const QByteArray line = QJsonDocument(entry.toJson()).toJson(QJsonDocument::Compact) + '\n';
    if (file.write(line) != line.size()) {
        return false;
    }
    file.close();
    Q_EMIT appended(entry);
    return true;
}

QList<LogEntry> SignatureLog::readAll() const {
    QList<LogEntry> entries;
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return entries;
    }
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject()) {
            continue; // a torn line is skipped, never repaired
        }
        if (const std::optional<LogEntry> entry = LogEntry::fromJson(document.object())) {
            entries.append(*entry);
        }
    }
    return entries;
}
