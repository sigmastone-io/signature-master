#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>

// One signing request as a job sent it and as the desk sees it.

enum class RequestState { Received, Pending, Refused, Signing, Signed, Failed, Rejected, Expired };
enum class FileState { Expected, Received, Signing, Signed, Failed };
enum class CheckOutcome { Pending, Passed, Failed, Unverified };

QString toString(RequestState state); // wire form: "pending"
QString toLabel(RequestState state);  // for people: "Awaiting approval"
QString toString(FileState state);
QString toString(CheckOutcome outcome);
bool isTerminal(RequestState state);

struct Check {
    QString key;   // identity, owner, repository, ref, certificate, files
    QString title; // "Caller identity verified through GitHub"
    CheckOutcome outcome = CheckOutcome::Pending;
    QString detail;

    QJsonObject toJson() const;
};

struct RequestFile {
    QString name;
    qint64 size = 0;
    QString sha256;  // lower-case hex
    QString version; // from the file's version resource, once received
    FileState state = FileState::Expected;
    QString path;       // the unsigned file on disk, once received
    QString signedPath; // the signed file, once signed
    QString error;

    QJsonObject toJson() const;
};

struct SigningRequest {
    QString id;
    QString repository; // owner/name
    QString ref;
    QString commit;
    QString workflow;
    QString job;
    QString runId;
    QString runUrl;
    QString actor;
    QString authenticatedAs;
    QDateTime requestedAt;
    QDateTime updatedAt;
    RequestState state = RequestState::Received;
    QString reason;
    QString approvedBy; // the signer, or "automatic"
    QString workDir;
    QList<RequestFile> files;
    QList<Check> checks;

    QString owner() const;
    QString name() const;
    QString refShort() const; // the tag name for refs/tags/*, else the ref
    bool allFilesReceived() const;
    bool anyCheckFailed() const;
    bool anyCheckUnverified() const;
    const Check* check(const QString& key) const;
    Check* check(const QString& key);
    QJsonObject toJson() const;
};
