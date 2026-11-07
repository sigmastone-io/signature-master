#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QObject>
#include <QString>

#include <memory>

class Authenticator;
class QHttpServer;
class QTcpServer;
class RequestStore;

// The REST listener a release job talks to. Every route is a thin skin over the request store;
// this class parses, answers and nothing more. One instance per bind: start() builds a fresh
// server, stop() tears it down.
class ApiServer : public QObject {
    Q_OBJECT

public:
    ApiServer(RequestStore* store, Authenticator* authenticator, QObject* parent = nullptr);
    ~ApiServer() override;

    bool start(const QHostAddress& address, quint16 port, QString* error);
    void stop();
    bool isListening() const { return m_http != nullptr; }
    quint16 port() const;
    QDateTime listeningSince() const { return m_since; }

private:
    void addRoutes();

    RequestStore* m_store;
    Authenticator* m_authenticator;
    std::unique_ptr<QHttpServer> m_http;
    QTcpServer* m_tcp = nullptr;
    QDateTime m_since;
};
