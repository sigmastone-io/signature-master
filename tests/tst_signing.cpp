#include "app/Authenticator.h"
#include "app/RequestStore.h"
#include "app/Settings.h"
#include "app/SignatureLog.h"
#include "app/Signer.h"
#include "app/SigningService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <memory>

namespace {

// Copies the input to the output (or not) and reports what it was told to, a tick later, the way
// the real signer reports from signtool.
class FakeSigner : public Signer {
public:
    using Signer::Signer;

    bool succeed = true;
    QString error = QStringLiteral("signtool sign failed: pretend");
    QStringList signedPaths;

    void sign(const QString& requestId,
              const QString& fileName,
              const QString& input,
              const QString& output) override {
        QTimer::singleShot(0, this, [this, requestId, fileName, input, output] {
            if (succeed) {
                QFile::copy(input, output);
                signedPaths.append(output);
                Q_EMIT finished(requestId, fileName, true, QString());
            } else {
                Q_EMIT finished(requestId, fileName, false, error);
            }
        });
    }
};

QString sha256(const QByteArray& bytes) {
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

} // namespace

class SigningTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void manualApprovalSignsEveryFile();
    void aFailedFileFailsTheRequest();
    void automaticSigningNeedsEveryCheckPassed();
    void rejectionAndRefusalAreLogged();
    void logRoundTrips();

private:
    QString openRequest(const AuthResult& auth, const QList<QByteArray>& payloads);
    AuthResult verified() const;
    AuthResult unverified() const;

    QTemporaryDir m_dir;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<SignatureLog> m_log;
    std::unique_ptr<RequestStore> m_store;
    std::unique_ptr<FakeSigner> m_signer;
    std::unique_ptr<SigningService> m_service;
};

void SigningTest::init() {
    QVERIFY(m_dir.isValid());
    m_settings = std::make_unique<Settings>(m_dir.filePath(QStringLiteral("settings.ini")));
    m_settings->setWorkDirectory(m_dir.filePath(QStringLiteral("work")));
    m_settings->setAutoSign(false);
    QFile::remove(m_dir.filePath(QStringLiteral("signatures.log")));
    m_log = std::make_unique<SignatureLog>(m_dir.filePath(QStringLiteral("signatures.log")));
    m_store = std::make_unique<RequestStore>(m_settings.get());
    m_store->setLog(m_log.get());
    m_store->setCertificateProbe([] {
        CertificateInfo info;
        info.found = true;
        info.subject = QStringLiteral("Test Signing");
        info.thumbprint = QStringLiteral("0123456789abcdef0123456789abcdef01234567");
        info.notAfter = QDateTime::currentDateTimeUtc().addYears(1);
        return info;
    });
    m_signer = std::make_unique<FakeSigner>();
    m_service = std::make_unique<SigningService>(m_settings.get(), m_store.get(), m_signer.get());
}

void SigningTest::cleanup() {
    m_service.reset();
    m_signer.reset();
    m_store.reset();
    m_log.reset();
    m_settings.reset();
}

AuthResult SigningTest::verified() const {
    return {CheckOutcome::Passed, QStringLiteral("GitHub Actions"), QString()};
}

AuthResult SigningTest::unverified() const {
    return {
        CheckOutcome::Unverified, QStringLiteral("unverified caller"), QStringLiteral("no scheme")};
}

QString SigningTest::openRequest(const AuthResult& auth, const QList<QByteArray>& payloads) {
    QJsonArray files;
    for (int i = 0; i < payloads.size(); ++i) {
        files.append(QJsonObject{{QStringLiteral("name"), QStringLiteral("file-%1.msi").arg(i)},
                                 {QStringLiteral("size"), payloads.at(i).size()},
                                 {QStringLiteral("sha256"), sha256(payloads.at(i))}});
    }
    const QJsonObject manifest{
        {QStringLiteral("repository"), QStringLiteral("sigmastone-io/neurotrader")},
        {QStringLiteral("ref"), QStringLiteral("refs/tags/v0.1.0")},
        {QStringLiteral("commit"), QStringLiteral("abc123")},
        {QStringLiteral("actor"), QStringLiteral("sorou")},
        {QStringLiteral("files"), files},
    };
    const RequestStore::Reply reply = m_store->create(manifest, auth);
    if (reply.status != 201) {
        return {};
    }
    const QString id = reply.body.value(QStringLiteral("id")).toString();
    for (int i = 0; i < payloads.size(); ++i) {
        m_store->receiveFile(id, QStringLiteral("file-%1.msi").arg(i), payloads.at(i));
    }
    return id;
}

void SigningTest::manualApprovalSignsEveryFile() {
    const QString id =
        openRequest(unverified(), {QByteArrayLiteral("one"), QByteArrayLiteral("two")});
    QVERIFY(!id.isEmpty());
    QVERIFY(m_store->find(id)->state == RequestState::Pending);

    QVERIFY(m_service->approve(id, QStringLiteral("sorou")));
    QVERIFY(m_store->find(id)->state == RequestState::Signing);
    QVERIFY(!m_service->approve(id, QStringLiteral("again")));

    QTRY_VERIFY(m_store->find(id)->state == RequestState::Signed);
    const SigningRequest request = *m_store->find(id);
    QCOMPARE(request.approvedBy, QStringLiteral("sorou"));
    QCOMPARE(m_signer->signedPaths.size(), 2);
    for (const RequestFile& file : request.files) {
        QVERIFY(file.state == FileState::Signed);
        QVERIFY(QFile::exists(file.signedPath));
        QCOMPARE(m_store->signedFilePath(id, file.name), file.signedPath);
    }
    // The unsigned originals are gone, the signed copies stay for the job.
    QVERIFY(!QDir(QDir(request.workDir).filePath(QStringLiteral("files"))).exists());
    QVERIFY(QDir(QDir(request.workDir).filePath(QStringLiteral("signed"))).exists());

    const QList<LogEntry> entries = m_log->readAll();
    QCOMPARE(entries.size(), 2);
    for (const LogEntry& entry : entries) {
        QCOMPARE(entry.outcome, QStringLiteral("signed"));
        QCOMPARE(entry.by, QStringLiteral("sorou"));
        QCOMPARE(entry.requestId, id);
        QVERIFY(entry.file.startsWith(QStringLiteral("file-")));
        QCOMPARE(entry.sha256.size(), 64);
    }
}

void SigningTest::aFailedFileFailsTheRequest() {
    m_signer->succeed = false;
    const QString id =
        openRequest(unverified(), {QByteArrayLiteral("one"), QByteArrayLiteral("two")});
    QVERIFY(m_service->approve(id, QStringLiteral("sorou")));
    QTRY_VERIFY(m_store->find(id)->state == RequestState::Failed);
    const SigningRequest request = *m_store->find(id);
    QVERIFY(request.reason.contains(QStringLiteral("pretend")));
    QVERIFY(request.files.at(0).state == FileState::Failed);
    QVERIFY(request.files.at(1).state == FileState::Received); // never reached
    QVERIFY(m_store->signedFilePath(id, QStringLiteral("file-0.msi")).isEmpty());
    QVERIFY(!QDir(request.workDir).exists());

    const QList<LogEntry> entries = m_log->readAll();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).outcome, QStringLiteral("failed"));
    QCOMPARE(entries.at(1).outcome, QStringLiteral("failed"));
    QVERIFY(entries.at(0).reason.contains(QStringLiteral("pretend")));
}

void SigningTest::automaticSigningNeedsEveryCheckPassed() {
    m_settings->setAutoSign(true);

    // Unverified caller: stays pending for a person.
    const QString waiting = openRequest(unverified(), {QByteArrayLiteral("one")});
    QVERIFY(m_store->find(waiting)->state == RequestState::Pending);
    QTest::qWait(20);
    QVERIFY(m_store->find(waiting)->state == RequestState::Pending);

    // Verified caller, every check green: signed on its own.
    const QString automatic = openRequest(verified(), {QByteArrayLiteral("one")});
    QTRY_VERIFY(m_store->find(automatic)->state == RequestState::Signed);
    QCOMPARE(m_store->find(automatic)->approvedBy, QStringLiteral("automatic"));
    QVERIFY(m_store->find(waiting)->state == RequestState::Pending);

    // With automatic signing off, a verified request waits too.
    m_settings->setAutoSign(false);
    const QString manual = openRequest(verified(), {QByteArrayLiteral("one")});
    QTest::qWait(20);
    QVERIFY(m_store->find(manual)->state == RequestState::Pending);
}

void SigningTest::rejectionAndRefusalAreLogged() {
    const QString id = openRequest(unverified(), {QByteArrayLiteral("one")});
    QVERIFY(m_store->reject(id, QStringLiteral("wrong tag")));
    QList<LogEntry> entries = m_log->readAll();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().outcome, QStringLiteral("rejected"));
    QCOMPARE(entries.first().reason, QStringLiteral("wrong tag"));
    QVERIFY(!entries.first().by.isEmpty());

    const QJsonObject bad{
        {QStringLiteral("repository"), QStringLiteral("someone/neurotrader")},
        {QStringLiteral("ref"), QStringLiteral("refs/tags/v0.1.0")},
        {QStringLiteral("files"),
         QJsonArray{QJsonObject{{QStringLiteral("name"), QStringLiteral("x.msi")},
                                {QStringLiteral("size"), 3},
                                {QStringLiteral("sha256"), sha256(QByteArrayLiteral("one"))}}}},
    };
    QCOMPARE(m_store->create(bad, unverified()).status, 403);
    entries = m_log->readAll();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.last().outcome, QStringLiteral("refused"));
    QCOMPARE(entries.last().by, QStringLiteral("checks"));
    QCOMPARE(entries.last().file, QStringLiteral("x.msi"));

    QCOMPARE(m_store->cancel(openRequest(unverified(), {QByteArrayLiteral("one")})).status, 200);
    entries = m_log->readAll();
    QCOMPARE(entries.size(), 3);
    QCOMPARE(entries.last().outcome, QStringLiteral("expired"));
    QCOMPARE(entries.last().by, QStringLiteral("nobody"));
}

void SigningTest::logRoundTrips() {
    LogEntry entry;
    entry.time = QDateTime(QDate(2026, 12, 10), QTime(14, 9, 41, 250), QTimeZone::UTC);
    entry.requestId = QStringLiteral("id");
    entry.repository = QStringLiteral("sigmastone-io/neurotrader");
    entry.ref = QStringLiteral("refs/tags/v1");
    entry.commit = QStringLiteral("abc");
    entry.file = QStringLiteral("a \"quoted\" name.msi");
    entry.sha256 = QStringLiteral("ff");
    entry.outcome = QStringLiteral("signed");
    entry.by = QStringLiteral("automatic");
    entry.reason = QString();
    QSignalSpy appended(m_log.get(), &SignatureLog::appended);
    QVERIFY(m_log->append(entry));
    QCOMPARE(appended.count(), 1);
    const QList<LogEntry> entries = m_log->readAll();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().time, entry.time);
    QCOMPARE(entries.first().file, entry.file);
    QCOMPARE(entries.first().by, entry.by);
}

QTEST_MAIN(SigningTest)

#include "tst_signing.moc"
