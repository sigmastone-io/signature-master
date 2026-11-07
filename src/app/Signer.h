#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

#include <memory>

class Settings;

// Signs one file at a time: `input` is the unsigned file, `output` where the signed copy goes.
// Asynchronous; `finished` carries the verdict. The signing service sequences the calls.
class Signer : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    virtual void sign(const QString& requestId,
                      const QString& fileName,
                      const QString& input,
                      const QString& output) = 0;

Q_SIGNALS:
    void progress(const QString& requestId, const QString& fileName, const QString& message);
    void finished(const QString& requestId, const QString& fileName, bool ok, const QString& error);
};

// signtool sign /sha1 <thumbprint> /fd sha256 /tr <timestamp> /td sha256, then a check that the
// file now carries that certificate's signature, then signtool verify /pa unless the
// certificate is self-signed (no chain can be built for one, and the thumbprint check already
// says who signed).
class SigntoolSigner : public Signer {
    Q_OBJECT

public:
    explicit SigntoolSigner(Settings* settings, QObject* parent = nullptr);

    void sign(const QString& requestId,
              const QString& fileName,
              const QString& input,
              const QString& output) override;

    static constexpr int kTimeoutMs = 10 * 60 * 1000; // a card may be waiting for its PIN

private:
    struct Job {
        QString requestId;
        QString fileName;
        QString output;
        QString thumbprint;
        bool selfSigned = false;
        QByteArray transcript;
    };

    void runSign(Job job);
    void runVerify(Job job);
    QProcess* startSigntool(const QStringList& arguments, Job job);
    void fail(const Job& job, const QString& error);
    static QString tail(const QByteArray& transcript);

    Settings* m_settings;
};
