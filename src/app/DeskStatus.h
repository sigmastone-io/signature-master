#pragma once

#include "CertificateStore.h"

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QString>

class ApiServer;
class Settings;

// What the desk is doing right now: whether the port is open, which certificate is in use, how
// many requests wait. Exposed to QML as the Desk singleton. It owns the listener and restarts it
// when the listening settings change.
class DeskStatus : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Desk)
    QML_SINGLETON

    Q_PROPERTY(bool listening READ listening NOTIFY changed)
    Q_PROPERTY(QString listenError READ listenError NOTIFY changed)
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY changed)

    Q_PROPERTY(bool certificateFound READ certificateFound NOTIFY changed)
    Q_PROPERTY(QString certificateThumbprint READ certificateThumbprint NOTIFY changed)
    Q_PROPERTY(QString certificateSubject READ certificateSubject NOTIFY changed)
    Q_PROPERTY(QString certificateIssuer READ certificateIssuer NOTIFY changed)
    Q_PROPERTY(QDateTime certificateExpires READ certificateExpires NOTIFY changed)
    Q_PROPERTY(bool certificateSelfSigned READ certificateSelfSigned NOTIFY changed)
    Q_PROPERTY(bool certificateExpired READ certificateExpired NOTIFY changed)
    Q_PROPERTY(QString certificateSummary READ certificateSummary NOTIFY changed)

    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY changed)
    Q_PROPERTY(QDateTime lastRequestAt READ lastRequestAt NOTIFY changed)
    Q_PROPERTY(QString logFilePath READ logFilePath CONSTANT)

public:
    explicit DeskStatus(Settings* settings, QObject* parent = nullptr);
    ~DeskStatus() override;

    static DeskStatus* instance();
    static DeskStatus* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    bool listening() const { return m_listening; }
    QString listenError() const { return m_listenError; }
    QString endpoint() const { return m_endpoint; }

    bool certificateFound() const { return m_certificate.found; }
    QString certificateThumbprint() const { return m_certificate.thumbprint; }
    QString certificateSubject() const { return m_certificate.subject; }
    QString certificateIssuer() const { return m_certificate.issuer; }
    QDateTime certificateExpires() const { return m_certificate.notAfter; }
    bool certificateSelfSigned() const { return m_certificate.selfSigned; }
    bool certificateExpired() const;
    QString certificateSummary() const;

    int pendingCount() const { return m_pendingCount; }
    QDateTime lastRequestAt() const { return m_lastRequestAt; }
    QString logFilePath() const;

    Q_INVOKABLE void chooseCertificate(quintptr ownerWindow = 0);
    Q_INVOKABLE void refreshCertificate();
    Q_INVOKABLE void openLogFolder();

Q_SIGNALS:
    void changed();

private:
    void restartListener();
    static QString reachableAddress(const QString& bindAddress);

    Settings* m_settings;
    ApiServer* m_server;
    bool m_listening = false;
    QString m_listenError;
    QString m_endpoint;
    CertificateInfo m_certificate;
    int m_pendingCount = 0;
    QDateTime m_lastRequestAt;
};
