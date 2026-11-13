// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "app/ApiServer.h"
#include "app/Authenticator.h"
#include "app/RequestStore.h"
#include "app/Settings.h"

#include <QCryptographicHash>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace {

struct HttpReply {
    int status = 0;
    QJsonObject body;
    QByteArray raw;
};

QJsonObject manifestFor(const QString& repository, const QString& ref, const QByteArray& bytes) {
    const QString digest =
        QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    return {
        {QStringLiteral("repository"), repository},
        {QStringLiteral("ref"), ref},
        {QStringLiteral("commit"), QStringLiteral("3b9d1e4f2c07a1b2c3d4e5f60718293a4b5c6d7e")},
        {QStringLiteral("workflow"), QStringLiteral("rust.yml")},
        {QStringLiteral("job"), QStringLiteral("package")},
        {QStringLiteral("run_id"), 1287},
        {QStringLiteral("run_url"),
         QStringLiteral("https://github.com/sigmastone-io/neurotrader/actions/runs/1287")},
        {QStringLiteral("actor"), QStringLiteral("sorou")},
        {QStringLiteral("files"),
         QJsonArray{
             QJsonObject{{QStringLiteral("name"), QStringLiteral("neurotrader-0.1.0-x64.msi")},
                         {QStringLiteral("size"), bytes.size()},
                         {QStringLiteral("sha256"), digest}}}},
    };
}

} // namespace

class ApiTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void healthAnswers();
    void malformedManifestIsNotListed();
    void unknownOwnerIsRefusedAndListed();
    void goodRequestBecomesPendingAfterUpload();
    void wrongHashRefusesTheRequest();
    void unknownFileNameIsRejected();
    void cancelExpiresThePendingRequest();
    void rejectFromTheWindow();

private:
    HttpReply call(const QByteArray& verb, const QString& path, const QByteArray& body = {});
    QString url(const QString& path) const;
    QString createGoodRequest(const QByteArray& bytes);

    QTemporaryDir m_dir;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<RequestStore> m_store;
    std::unique_ptr<UnconfiguredAuthenticator> m_auth;
    std::unique_ptr<ApiServer> m_server;
    QNetworkAccessManager m_network;
    const QByteArray m_payload = QByteArrayLiteral("not really an msi, but 32 bytes!");
};

void ApiTest::initTestCase() {
    QVERIFY(m_dir.isValid());
}

void ApiTest::init() {
    m_settings = std::make_unique<Settings>(m_dir.filePath(QStringLiteral("settings.ini")));
    m_settings->setWorkDirectory(m_dir.filePath(QStringLiteral("work")));
    m_store = std::make_unique<RequestStore>(m_settings.get());
    m_store->setCertificateProbe([] {
        CertificateInfo info;
        info.found = true;
        info.subject = QStringLiteral("Test Signing");
        info.thumbprint = QStringLiteral("0123456789abcdef0123456789abcdef01234567");
        info.notAfter = QDateTime::currentDateTimeUtc().addYears(1);
        info.selfSigned = true;
        return info;
    });
    m_auth = std::make_unique<UnconfiguredAuthenticator>();
    m_server = std::make_unique<ApiServer>(m_store.get(), m_auth.get());
    QString error;
    QVERIFY2(m_server->start(QHostAddress::LocalHost, 0, &error), qPrintable(error));
    QVERIFY(m_server->port() != 0);
}

void ApiTest::cleanup() {
    m_server.reset();
    m_store.reset();
    m_settings.reset();
    m_auth.reset();
}

QString ApiTest::url(const QString& path) const {
    return QStringLiteral("http://127.0.0.1:%1%2").arg(m_server->port()).arg(path);
}

HttpReply ApiTest::call(const QByteArray& verb, const QString& path, const QByteArray& body) {
    QNetworkRequest request{QUrl(url(path))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply* reply = m_network.sendCustomRequest(request, verb, body);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    HttpReply out;
    out.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    out.raw = reply->readAll();
    out.body = QJsonDocument::fromJson(out.raw).object();
    reply->deleteLater();
    return out;
}

QString ApiTest::createGoodRequest(const QByteArray& bytes) {
    const QJsonObject manifest = manifestFor(
        QStringLiteral("sigmastone-io/neurotrader"), QStringLiteral("refs/tags/v0.1.0"), bytes);
    const HttpReply reply =
        call("POST", QStringLiteral("/requests"), QJsonDocument(manifest).toJson());
    if (reply.status != 201) {
        return {};
    }
    return reply.body.value(QStringLiteral("id")).toString();
}

void ApiTest::healthAnswers() {
    const HttpReply reply = call("GET", QStringLiteral("/health"));
    QCOMPARE(reply.status, 200);
    QCOMPARE(reply.body.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(reply.body.value(QStringLiteral("pending")).toInt(), 0);
}

void ApiTest::malformedManifestIsNotListed() {
    QJsonObject manifest = manifestFor(
        QStringLiteral("sigmastone-io/neurotrader"), QStringLiteral("refs/tags/v0.1.0"), m_payload);
    manifest.remove(QStringLiteral("files"));
    const HttpReply reply =
        call("POST", QStringLiteral("/requests"), QJsonDocument(manifest).toJson());
    QCOMPARE(reply.status, 400);
    QVERIFY(reply.body.contains(QStringLiteral("error")));
    QCOMPARE(m_store->requests().size(), 0);

    const HttpReply garbage = call("POST", QStringLiteral("/requests"), QByteArrayLiteral("nope"));
    QCOMPARE(garbage.status, 400);
}

void ApiTest::unknownOwnerIsRefusedAndListed() {
    const QJsonObject manifest = manifestFor(
        QStringLiteral("someone-else/neurotrader"), QStringLiteral("refs/tags/v0.1.0"), m_payload);
    const HttpReply reply =
        call("POST", QStringLiteral("/requests"), QJsonDocument(manifest).toJson());
    QCOMPARE(reply.status, 403);
    QCOMPARE(reply.body.value(QStringLiteral("state")).toString(), QStringLiteral("refused"));
    QVERIFY(
        reply.body.value(QStringLiteral("reason")).toString().contains(QStringLiteral("owner")));
    QCOMPARE(m_store->requests().size(), 1);
    QVERIFY(m_store->requests().first().state == RequestState::Refused);
    // A refused request takes no files.
    const QString id = reply.body.value(QStringLiteral("id")).toString();
    const HttpReply put = call(
        "PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), m_payload);
    QCOMPARE(put.status, 409);
}

void ApiTest::goodRequestBecomesPendingAfterUpload() {
    QSignalSpy entered(m_store.get(), &RequestStore::entered);
    const QString id = createGoodRequest(m_payload);
    QVERIFY(!id.isEmpty());

    HttpReply state = call("GET", QStringLiteral("/requests/%1").arg(id));
    QCOMPARE(state.status, 200);
    QCOMPARE(state.body.value(QStringLiteral("state")).toString(), QStringLiteral("received"));
    QCOMPARE(m_store->pendingCount(), 0);

    const HttpReply put = call(
        "PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), m_payload);
    QCOMPARE(put.status, 200);
    QCOMPARE(put.body.value(QStringLiteral("state")).toString(), QStringLiteral("pending"));
    QCOMPARE(m_store->pendingCount(), 1);
    QCOMPARE(entered.count(), 1);
    QVERIFY(entered.first().at(1).value<RequestState>() == RequestState::Pending);

    // The file is on disk under the work directory and matches.
    const std::optional<SigningRequest> request = m_store->find(id);
    QVERIFY(request.has_value());
    QVERIFY(request->files.first().state == FileState::Received);
    QFile stored(request->files.first().path);
    QVERIFY(stored.open(QIODevice::ReadOnly));
    QCOMPARE(stored.readAll(), m_payload);

    // Checks: identity unverified, the rest passed.
    const QJsonArray checks = put.body.value(QStringLiteral("checks")).toArray();
    QCOMPARE(checks.size(), 6);
    QCOMPARE(checks.at(0).toObject().value(QStringLiteral("outcome")).toString(),
             QStringLiteral("unverified"));
    for (int i = 1; i < checks.size(); ++i) {
        QCOMPARE(checks.at(i).toObject().value(QStringLiteral("outcome")).toString(),
                 QStringLiteral("passed"));
    }

    // Nothing signed yet.
    const HttpReply signedFile =
        call("GET", QStringLiteral("/requests/%1/signed/neurotrader-0.1.0-x64.msi").arg(id));
    QCOMPARE(signedFile.status, 404);

    // A second upload of the same file is refused.
    const HttpReply again = call(
        "PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), m_payload);
    QCOMPARE(again.status, 409);
}

void ApiTest::wrongHashRefusesTheRequest() {
    const QString id = createGoodRequest(m_payload);
    QVERIFY(!id.isEmpty());
    QByteArray tampered = m_payload;
    tampered[0] = 'X';
    const HttpReply put = call(
        "PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), tampered);
    QCOMPARE(put.status, 400);
    QCOMPARE(put.body.value(QStringLiteral("state")).toString(), QStringLiteral("refused"));
    QVERIFY(
        put.body.value(QStringLiteral("reason")).toString().contains(QStringLiteral("SHA-256")));
    const std::optional<SigningRequest> request = m_store->find(id);
    QVERIFY(request.has_value());
    QVERIFY(request->state == RequestState::Refused);
    // The work directory is gone with the refusal.
    QVERIFY(!QDir(request->workDir).exists());

    const HttpReply shortOne = call("PUT",
                                    QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi")
                                        .arg(createGoodRequest(m_payload)),
                                    m_payload.left(5));
    QCOMPARE(shortOne.status, 400);
    QVERIFY(
        shortOne.body.value(QStringLiteral("reason")).toString().contains(QStringLiteral("bytes")));
}

void ApiTest::unknownFileNameIsRejected() {
    const QString id = createGoodRequest(m_payload);
    const HttpReply put =
        call("PUT", QStringLiteral("/requests/%1/files/other.exe").arg(id), m_payload);
    QCOMPARE(put.status, 404);
    const HttpReply missing = call("GET", QStringLiteral("/requests/doesnotexist"));
    QCOMPARE(missing.status, 404);
}

void ApiTest::cancelExpiresThePendingRequest() {
    const QString id = createGoodRequest(m_payload);
    call("PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), m_payload);
    const HttpReply gone = call("DELETE", QStringLiteral("/requests/%1").arg(id));
    QCOMPARE(gone.status, 200);
    QCOMPARE(gone.body.value(QStringLiteral("state")).toString(), QStringLiteral("expired"));
    QCOMPARE(m_store->pendingCount(), 0);
    // Cancelling twice is harmless.
    const HttpReply twice = call("DELETE", QStringLiteral("/requests/%1").arg(id));
    QCOMPARE(twice.status, 200);
    QCOMPARE(twice.body.value(QStringLiteral("state")).toString(), QStringLiteral("expired"));
}

void ApiTest::rejectFromTheWindow() {
    const QString id = createGoodRequest(m_payload);
    QVERIFY(!m_store->reject(id, QStringLiteral("too early")));
    call("PUT", QStringLiteral("/requests/%1/files/neurotrader-0.1.0-x64.msi").arg(id), m_payload);
    QVERIFY(m_store->reject(id, QStringLiteral("wrong tag, retagging")));
    const HttpReply state = call("GET", QStringLiteral("/requests/%1").arg(id));
    QCOMPARE(state.body.value(QStringLiteral("state")).toString(), QStringLiteral("rejected"));
    QCOMPARE(state.body.value(QStringLiteral("reason")).toString(),
             QStringLiteral("wrong tag, retagging"));
    QVERIFY(!m_store->reject(id, QStringLiteral("again")));
}

QTEST_MAIN(ApiTest)

#include "tst_api.moc"
