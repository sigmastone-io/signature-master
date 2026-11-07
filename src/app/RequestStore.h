#pragma once

#include "Authenticator.h"
#include "CertificateStore.h"
#include "SigningRequest.h"

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>
#include <optional>

class Settings;

// Every request the desk has seen since it started, newest last, with the files of each under
// the work directory. The REST routes and the window both go through here; nothing else changes
// a request's state. Requests live in memory only: the log is the history.
class RequestStore : public QObject {
    Q_OBJECT

public:
    struct Reply {
        int status = 200;
        QJsonObject body;
    };

    explicit RequestStore(Settings* settings, QObject* parent = nullptr);

    // How the store learns whether a usable certificate is in place; the default reads the
    // user's store for the configured thumbprint.
    void setCertificateProbe(std::function<CertificateInfo()> probe);

    // POST /requests. 201 when accepted, 403 when refused (the request is still listed), 400 when
    // the manifest is malformed (nothing is listed).
    Reply create(const QJsonObject& manifest, const AuthResult& auth);
    // PUT /requests/{id}/files/{name}.
    Reply receiveFile(const QString& id, const QString& name, const QByteArray& bytes);
    // GET /requests/{id}.
    Reply describe(const QString& id) const;
    // DELETE /requests/{id}: the job gave up.
    Reply cancel(const QString& id);
    // The signed copy of a file, or empty while there is none.
    QString signedFilePath(const QString& id, const QString& name) const;

    // From the window.
    bool reject(const QString& id, const QString& reason);

    const QList<SigningRequest>& requests() const { return m_requests; }
    std::optional<SigningRequest> find(const QString& id) const;
    int pendingCount() const;
    QDateTime lastRequestAt() const { return m_lastRequestAt; }

    static constexpr int kApprovalTimeoutMinutes = 60;
    static constexpr int kRetentionHours = 24;

Q_SIGNALS:
    void added(const QString& id);
    void changed(const QString& id);
    void removed(const QString& id);
    // A request reached a state worth telling the person about.
    void entered(const QString& id, RequestState state);

private:
    SigningRequest* mutableFind(const QString& id);
    void transition(SigningRequest& request, RequestState state, const QString& reason);
    void expireStale();
    void wipeLeftovers();
    static void wipe(const QString& directory);
    static bool safeFileName(const QString& name);
    static Reply error(int status, const QString& message);

    Settings* m_settings;
    std::function<CertificateInfo()> m_certificateProbe;
    QList<SigningRequest> m_requests;
    QDateTime m_lastRequestAt;
    QTimer m_housekeeping;
};
