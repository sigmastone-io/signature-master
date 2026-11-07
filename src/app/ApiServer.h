#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QObject>
#include <QString>

#include <memory>

class QHttpServer;

// The REST listener. For now it answers GET /health; the request routes come with the request
// store. One instance per bind: start() builds a fresh server, stop() tears it down.
class ApiServer : public QObject {
    Q_OBJECT

public:
    explicit ApiServer(QObject* parent = nullptr);
    ~ApiServer() override;

    bool start(const QHostAddress& address, quint16 port, QString* error);
    void stop();
    bool isListening() const { return m_http != nullptr; }
    QDateTime listeningSince() const { return m_since; }

private:
    void addRoutes();

    std::unique_ptr<QHttpServer> m_http;
    QDateTime m_since;
};
