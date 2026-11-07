#include "ApiServer.h"

#include "Authenticator.h"
#include "RequestStore.h"

#include <QCoreApplication>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>

namespace {

QHttpServerResponse reply(const RequestStore::Reply& reply) {
    return QHttpServerResponse(reply.body,
                               static_cast<QHttpServerResponder::StatusCode>(reply.status));
}

QHttpServerResponse problem(int status, const QString& message) {
    return reply(RequestStore::Reply{status, QJsonObject{{QStringLiteral("error"), message}}});
}

} // namespace

ApiServer::ApiServer(RequestStore* store, Authenticator* authenticator, QObject* parent)
    : QObject(parent), m_store(store), m_authenticator(authenticator) {}

ApiServer::~ApiServer() = default;

bool ApiServer::start(const QHostAddress& address, quint16 port, QString* error) {
    stop();
    m_http = std::make_unique<QHttpServer>();
    addRoutes();

    // The TCP server is a child of the HTTP server, so both go together on stop().
    m_tcp = new QTcpServer(m_http.get());
    if (!m_tcp->listen(address, port)) {
        if (error != nullptr) {
            *error = m_tcp->errorString();
        }
        stop();
        return false;
    }
    if (!m_http->bind(m_tcp)) {
        if (error != nullptr) {
            *error = QStringLiteral("the HTTP server did not accept the socket");
        }
        stop();
        return false;
    }
    m_since = QDateTime::currentDateTimeUtc();
    return true;
}

void ApiServer::stop() {
    m_tcp = nullptr;
    m_http.reset();
    m_since = {};
}

quint16 ApiServer::port() const {
    return m_tcp != nullptr ? m_tcp->serverPort() : 0;
}

void ApiServer::addRoutes() {
    using Method = QHttpServerRequest::Method;

    m_http->route(QStringLiteral("/health"), Method::Get, [this] {
        const QJsonObject body{
            {QStringLiteral("status"), QStringLiteral("ok")},
            {QStringLiteral("application"), QStringLiteral("signature-master")},
            {QStringLiteral("version"), QCoreApplication::applicationVersion()},
            {QStringLiteral("listening_since"), m_since.toString(Qt::ISODate)},
            {QStringLiteral("pending"), m_store->pendingCount()},
        };
        return QHttpServerResponse(body);
    });

    // A job opens a request with its manifest: repository, ref, commit, workflow, job, run_id,
    // run_url, actor, files[{name, size, sha256}].
    m_http->route(
        QStringLiteral("/requests"), Method::Post, [this](const QHttpServerRequest& request) {
            QJsonParseError parseError{};
            const QJsonDocument document = QJsonDocument::fromJson(request.body(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                return problem(400, QStringLiteral("the body must be a JSON object"));
            }
            const AuthResult auth = m_authenticator->authenticate(request);
            return reply(m_store->create(document.object(), auth));
        });

    m_http->route(QStringLiteral("/requests/<arg>"), Method::Get, [this](const QString& id) {
        return reply(m_store->describe(id));
    });

    // The job gave up waiting.
    m_http->route(QStringLiteral("/requests/<arg>"), Method::Delete, [this](const QString& id) {
        return reply(m_store->cancel(id));
    });

    // One unsigned file, whole, checked against the manifest on arrival.
    m_http->route(
        QStringLiteral("/requests/<arg>/files/<arg>"),
        Method::Put,
        [this](const QString& id, const QString& name, const QHttpServerRequest& request) {
            return reply(m_store->receiveFile(id, name, request.body()));
        });

    // The signed copy, once the request is signed.
    m_http->route(QStringLiteral("/requests/<arg>/signed/<arg>"),
                  Method::Get,
                  [this](const QString& id, const QString& name) {
                      const QString path = m_store->signedFilePath(id, name);
                      if (path.isEmpty()) {
                          return problem(404, QStringLiteral("no signed copy of \"%1\"").arg(name));
                      }
                      return QHttpServerResponse::fromFile(path);
                  });
}
