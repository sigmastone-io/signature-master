#include "DeskStatus.h"

#include "ApiServer.h"
#include "RequestStore.h"
#include "Settings.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QUrl>

namespace {
DeskStatus* g_instance = nullptr;
}

DeskStatus::DeskStatus(Settings* settings,
                       RequestStore* store,
                       Authenticator* authenticator,
                       QObject* parent)
    : QObject(parent), m_settings(settings), m_store(store),
      m_server(new ApiServer(store, authenticator, this)) {
    Q_ASSERT(g_instance == nullptr);
    g_instance = this;

    connect(m_settings, &Settings::bindAddressChanged, this, &DeskStatus::restartListener);
    connect(m_settings, &Settings::portChanged, this, &DeskStatus::restartListener);
    connect(m_settings, &Settings::pauseListeningChanged, this, &DeskStatus::restartListener);
    connect(
        m_settings, &Settings::certificateThumbprintChanged, this, &DeskStatus::refreshCertificate);
    connect(m_store, &RequestStore::added, this, &DeskStatus::changed);
    connect(m_store, &RequestStore::changed, this, &DeskStatus::changed);
    connect(m_store, &RequestStore::removed, this, &DeskStatus::changed);

    m_certificate = CertificateStore::describe(m_settings->certificateThumbprint());
    restartListener();
}

DeskStatus::~DeskStatus() {
    g_instance = nullptr;
}

DeskStatus* DeskStatus::instance() {
    Q_ASSERT(g_instance != nullptr);
    return g_instance;
}

DeskStatus* DeskStatus::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    DeskStatus* desk = instance();
    QJSEngine::setObjectOwnership(desk, QJSEngine::CppOwnership);
    return desk;
}

bool DeskStatus::certificateExpired() const {
    return m_certificate.found && m_certificate.notAfter < QDateTime::currentDateTimeUtc();
}

QString DeskStatus::certificateSummary() const {
    if (!m_certificate.found) {
        return QStringLiteral("No certificate chosen");
    }
    QString summary = QStringLiteral("Certificate: %1").arg(m_certificate.subject);
    if (m_certificate.selfSigned) {
        summary += QStringLiteral(", self-signed");
    }
    summary +=
        (certificateExpired() ? QStringLiteral(", expired %1") : QStringLiteral(", expires %1"))
            .arg(m_certificate.notAfter.toLocalTime().toString(QStringLiteral("yyyy-MM-dd")));
    return summary;
}

int DeskStatus::pendingCount() const {
    return m_store->pendingCount();
}

QDateTime DeskStatus::lastRequestAt() const {
    return m_store->lastRequestAt().toLocalTime();
}

QString DeskStatus::logFilePath() const {
    return QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/signatures.log"));
}

void DeskStatus::chooseCertificate(quintptr ownerWindow) {
    const std::optional<QString> thumbprint = CertificateStore::pickFromUserStore(ownerWindow);
    if (thumbprint.has_value()) {
        m_settings->setCertificateThumbprint(*thumbprint);
        // The thumbprint may be unchanged while the store changed underneath; read it anyway.
        refreshCertificate();
    }
}

void DeskStatus::refreshCertificate() {
    m_certificate = CertificateStore::describe(m_settings->certificateThumbprint());
    Q_EMIT changed();
}

void DeskStatus::openLogFolder() {
    const QDir dir = QFileInfo(logFilePath()).dir();
    dir.mkpath(QStringLiteral("."));
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void DeskStatus::restartListener() {
    m_server->stop();
    m_listening = false;
    m_listenError.clear();

    const QString bind = m_settings->bindAddress();
    const int port = m_settings->port();
    m_endpoint = QStringLiteral("http://%1:%2").arg(reachableAddress(bind)).arg(port);

    if (!m_settings->pauseListening()) {
        const QHostAddress address(bind);
        if (address.isNull()) {
            m_listenError = QStringLiteral("\"%1\" is not an IP address.").arg(bind);
        } else {
            QString error;
            if (m_server->start(address, static_cast<quint16>(port), &error)) {
                m_listening = true;
            } else {
                m_listenError = QStringLiteral("Port %1 on %2 could not be opened: %3.")
                                    .arg(port)
                                    .arg(bind, error);
            }
        }
    }
    Q_EMIT changed();
}

QString DeskStatus::reachableAddress(const QString& bindAddress) {
    const QHostAddress bind(bindAddress);
    if (bind != QHostAddress::AnyIPv4 && bind != QHostAddress::Any &&
        bind != QHostAddress::AnyIPv6) {
        return bindAddress;
    }
    // Listening on every interface: name the first routable IPv4 address so the footer and the
    // empty states can say where a job reaches this machine.
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback() &&
            !address.isLinkLocal()) {
            return address.toString();
        }
    }
    return QStringLiteral("127.0.0.1");
}
