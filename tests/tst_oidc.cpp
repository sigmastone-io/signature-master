// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "app/Authenticator.h"
#include "app/JwksCache.h"
#include "app/Jwt.h"
#include "app/Settings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <openssl/core_names.h>
#include <openssl/evp.h>

namespace {

// A key pair made for the test, offered to the authenticator as if it were GitHub's.
class TestKeys : public KeySource {
public:
    TestKeys() {
        m_key = EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", static_cast<size_t>(2048));
        BIGNUM* n = nullptr;
        BIGNUM* e = nullptr;
        EVP_PKEY_get_bn_param(m_key, OSSL_PKEY_PARAM_RSA_N, &n);
        EVP_PKEY_get_bn_param(m_key, OSSL_PKEY_PARAM_RSA_E, &e);
        m_public.kid = QStringLiteral("test-key-1");
        m_public.modulus = bytes(n);
        m_public.exponent = bytes(e);
        BN_free(n);
        BN_free(e);
    }
    ~TestKeys() override { EVP_PKEY_free(m_key); }

    int refreshes = 0;

    std::optional<RsaPublicKey> key(const QString& kid, bool allowRefresh) override {
        if (allowRefresh) {
            ++refreshes;
        }
        if (kid == m_public.kid) {
            return m_public;
        }
        return std::nullopt;
    }

    const RsaPublicKey& publicKey() const { return m_public; }

    QByteArray sign(const QJsonObject& header, const QJsonObject& payload) const {
        const QByteArray input =
            Jwt::base64UrlEncode(QJsonDocument(header).toJson(QJsonDocument::Compact)) + '.' +
            Jwt::base64UrlEncode(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        EVP_DigestSignInit(context, nullptr, EVP_sha256(), nullptr, m_key);
        size_t length = 0;
        EVP_DigestSign(context,
                       nullptr,
                       &length,
                       reinterpret_cast<const unsigned char*>(input.constData()),
                       static_cast<size_t>(input.size()));
        QByteArray signature(static_cast<int>(length), '\0');
        EVP_DigestSign(context,
                       reinterpret_cast<unsigned char*>(signature.data()),
                       &length,
                       reinterpret_cast<const unsigned char*>(input.constData()),
                       static_cast<size_t>(input.size()));
        EVP_MD_CTX_free(context);
        signature.resize(static_cast<int>(length));
        return input + '.' + Jwt::base64UrlEncode(signature);
    }

    QJsonObject jwksDocument() const {
        return {
            {QStringLiteral("keys"),
             QJsonArray{QJsonObject{
                 {QStringLiteral("kty"), QStringLiteral("RSA")},
                 {QStringLiteral("kid"), m_public.kid},
                 {QStringLiteral("n"), QString::fromLatin1(Jwt::base64UrlEncode(m_public.modulus))},
                 {QStringLiteral("e"),
                  QString::fromLatin1(Jwt::base64UrlEncode(m_public.exponent))}}}}};
    }

private:
    static QByteArray bytes(const BIGNUM* number) {
        QByteArray out(BN_num_bytes(number), '\0');
        BN_bn2bin(number, reinterpret_cast<unsigned char*>(out.data()));
        return out;
    }

    EVP_PKEY* m_key = nullptr;
    RsaPublicKey m_public;
};

} // namespace

class OidcTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void noTokenIsUnverified();
    void goodTokenPasses();
    void audienceArrayPasses();
    void tamperedSignatureFails();
    void wrongAudienceFails();
    void expiredTokenFails();
    void wrongIssuerFails();
    void wrongOwnerFails();
    void claimsMustMatchTheManifest();
    void unknownKeyAsksForARefresh();
    void garbageIsNotAJwt();
    void jwksParsesGitHubShape();

private:
    QJsonObject manifest() const;
    QJsonObject header() const;
    QJsonObject payload(const QByteArray& body) const;
    AuthResult run(const QByteArray& token, const QByteArray& body);

    QTemporaryDir m_dir;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<TestKeys> m_keys;
    std::unique_ptr<GitHubOidcAuthenticator> m_auth;
};

void OidcTest::initTestCase() {
    QVERIFY(m_dir.isValid());
    m_settings = std::make_unique<Settings>(m_dir.filePath(QStringLiteral("settings.ini")));
    m_settings->setAllowedOwner(QStringLiteral("example-org"));
    m_settings->setAllowedRepositories({QStringLiteral("example-app")});
    m_keys = std::make_unique<TestKeys>();
    m_auth = std::make_unique<GitHubOidcAuthenticator>(m_settings.get(), m_keys.get());
}

QJsonObject OidcTest::manifest() const {
    return {
        {QStringLiteral("repository"), QStringLiteral("example-org/example-app")},
        {QStringLiteral("ref"), QStringLiteral("refs/tags/v0.1.0")},
        {QStringLiteral("commit"), QStringLiteral("3b9d1e4f2c07")},
        {QStringLiteral("run_id"), 1287},
        {QStringLiteral("files"), QJsonArray{}},
    };
}

QJsonObject OidcTest::header() const {
    return {{QStringLiteral("alg"), QStringLiteral("RS256")},
            {QStringLiteral("typ"), QStringLiteral("JWT")},
            {QStringLiteral("kid"), QStringLiteral("test-key-1")}};
}

QJsonObject OidcTest::payload(const QByteArray& body) const {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    return {
        {QStringLiteral("iss"), QLatin1String(GitHubOidcAuthenticator::kIssuer)},
        {QStringLiteral("aud"), GitHubOidcAuthenticator::audienceFor(body)},
        {QStringLiteral("iat"), static_cast<double>(now - 10)},
        {QStringLiteral("nbf"), static_cast<double>(now - 10)},
        {QStringLiteral("exp"), static_cast<double>(now + 300)},
        {QStringLiteral("repository_owner"), QStringLiteral("example-org")},
        {QStringLiteral("repository"), QStringLiteral("example-org/example-app")},
        {QStringLiteral("ref"), QStringLiteral("refs/tags/v0.1.0")},
        {QStringLiteral("sha"), QStringLiteral("3b9d1e4f2c07")},
        {QStringLiteral("run_id"), QStringLiteral("1287")},
        {QStringLiteral("actor"), QStringLiteral("octocat")},
        {QStringLiteral("workflow_ref"),
         QStringLiteral("example-org/example-app/.github/workflows/rust.yml@refs/tags/v0.1.0")},
        {QStringLiteral("event_name"), QStringLiteral("push")},
    };
}

AuthResult OidcTest::run(const QByteArray& token, const QByteArray& body) {
    return m_auth->authenticate(
        token, body, QJsonDocument::fromJson(body).object(), QStringLiteral("127.0.0.1"));
}

void OidcTest::noTokenIsUnverified() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    const AuthResult result = run(QByteArray(), body);
    QVERIFY(result.outcome == CheckOutcome::Unverified);
    QVERIFY(result.identity.contains(QStringLiteral("127.0.0.1")));
}

void OidcTest::goodTokenPasses() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    const AuthResult result = run(m_keys->sign(header(), payload(body)), body);
    QVERIFY2(result.outcome == CheckOutcome::Passed, qPrintable(result.detail));
    QCOMPARE(result.identity,
             QStringLiteral("GitHub Actions, example-org/example-app run 1287 by octocat"));
    QVERIFY(result.detail.contains(QStringLiteral("rust.yml")));
}

void OidcTest::audienceArrayPasses() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QJsonObject claims = payload(body);
    claims.insert(QStringLiteral("aud"),
                  QJsonArray{QStringLiteral("other"), GitHubOidcAuthenticator::audienceFor(body)});
    QVERIFY(run(m_keys->sign(header(), claims), body).outcome == CheckOutcome::Passed);
}

void OidcTest::tamperedSignatureFails() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QByteArray token = m_keys->sign(header(), payload(body));
    // Flip a character inside the payload: the signature no longer covers it.
    const int dot = token.indexOf('.') + 5;
    token[dot] = token.at(dot) == 'A' ? 'B' : 'A';
    const AuthResult result = run(token, body);
    QVERIFY(result.outcome == CheckOutcome::Failed);
    QVERIFY(result.detail.contains(QStringLiteral("signature")) ||
            result.detail.contains(QStringLiteral("JWT")));
}

void OidcTest::wrongAudienceFails() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QByteArray other = body;
    other.replace("v0.1.0", "v0.1.1");
    // A token minted for other bytes, presented with this body.
    const AuthResult result = run(m_keys->sign(header(), payload(other)), body);
    QVERIFY(result.outcome == CheckOutcome::Failed);
    QVERIFY(result.detail.contains(QStringLiteral("audience")));
}

void OidcTest::expiredTokenFails() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QJsonObject claims = payload(body);
    claims.insert(QStringLiteral("exp"),
                  static_cast<double>(QDateTime::currentSecsSinceEpoch() - 600));
    const AuthResult result = run(m_keys->sign(header(), claims), body);
    QVERIFY(result.outcome == CheckOutcome::Failed);
    QVERIFY(result.detail.contains(QStringLiteral("expired")));
}

void OidcTest::wrongIssuerFails() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QJsonObject claims = payload(body);
    claims.insert(QStringLiteral("iss"), QStringLiteral("https://evil.example"));
    QVERIFY(run(m_keys->sign(header(), claims), body).outcome == CheckOutcome::Failed);
}

void OidcTest::wrongOwnerFails() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QJsonObject claims = payload(body);
    claims.insert(QStringLiteral("repository_owner"), QStringLiteral("someone-else"));
    const AuthResult result = run(m_keys->sign(header(), claims), body);
    QVERIFY(result.outcome == CheckOutcome::Failed);
    QVERIFY(result.detail.contains(QStringLiteral("someone-else")));
}

void OidcTest::claimsMustMatchTheManifest() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    for (const char* name : {"repository", "ref", "sha", "run_id"}) {
        QJsonObject claims = payload(body);
        claims.insert(QLatin1String(name), QStringLiteral("something-else"));
        const AuthResult result = run(m_keys->sign(header(), claims), body);
        QVERIFY2(result.outcome == CheckOutcome::Failed, name);
        QVERIFY2(result.detail.contains(QStringLiteral("manifest says")),
                 qPrintable(result.detail));
    }
}

void OidcTest::unknownKeyAsksForARefresh() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QJsonObject head = header();
    head.insert(QStringLiteral("kid"), QStringLiteral("rotated-away"));
    const int before = m_keys->refreshes;
    const AuthResult result = run(m_keys->sign(head, payload(body)), body);
    QVERIFY(result.outcome == CheckOutcome::Failed);
    QVERIFY(result.detail.contains(QStringLiteral("rotated-away")));
    QCOMPARE(m_keys->refreshes, before + 1);
}

void OidcTest::garbageIsNotAJwt() {
    const QByteArray body = QJsonDocument(manifest()).toJson(QJsonDocument::Compact);
    QVERIFY(run(QByteArrayLiteral("not.a.token"), body).outcome == CheckOutcome::Failed);
    QVERIFY(run(QByteArrayLiteral("ghp_somethingelse"), body).outcome == CheckOutcome::Failed);
}

void OidcTest::jwksParsesGitHubShape() {
    const QHash<QString, RsaPublicKey> keys =
        JwksCache::parse(QJsonDocument(m_keys->jwksDocument()).toJson(QJsonDocument::Compact));
    QCOMPARE(keys.size(), 1);
    QCOMPARE(keys.value(QStringLiteral("test-key-1")).modulus, m_keys->publicKey().modulus);
    QCOMPARE(keys.value(QStringLiteral("test-key-1")).exponent, m_keys->publicKey().exponent);
}

QTEST_MAIN(OidcTest)

#include "tst_oidc.moc"
