#include "ApiServer.h"

#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QTcpServer>

ApiServer::ApiServer(QObject* parent) : QObject(parent) {}

ApiServer::~ApiServer() = default;

bool ApiServer::start(const QHostAddress& address, quint16 port, QString* error) {
    stop();
    m_http = std::make_unique<QHttpServer>();
    addRoutes();

    // The TCP server is a child of the HTTP server, so both go together on stop().
    auto* tcp = new QTcpServer(m_http.get());
    if (!tcp->listen(address, port)) {
        if (error != nullptr) {
            *error = tcp->errorString();
        }
        m_http.reset();
        return false;
    }
    if (!m_http->bind(tcp)) {
        if (error != nullptr) {
            *error = QStringLiteral("the HTTP server did not accept the socket");
        }
        m_http.reset();
        return false;
    }
    m_since = QDateTime::currentDateTimeUtc();
    return true;
}

void ApiServer::stop() {
    m_http.reset();
    m_since = {};
}

void ApiServer::addRoutes() {
    m_http->route(QStringLiteral("/health"), QHttpServerRequest::Method::Get, [this] {
        const QJsonObject body{
            {QStringLiteral("status"), QStringLiteral("ok")},
            {QStringLiteral("application"), QStringLiteral("signature-master")},
            {QStringLiteral("version"), QCoreApplication::applicationVersion()},
            {QStringLiteral("listening_since"), m_since.toString(Qt::ISODate)},
        };
        return QHttpServerResponse(body);
    });
}
