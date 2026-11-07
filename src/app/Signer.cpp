#include "Signer.h"

#include "Authenticode.h"
#include "CertificateStore.h"
#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

SigntoolSigner::SigntoolSigner(Settings* settings, QObject* parent)
    : Signer(parent), m_settings(settings) {}

void SigntoolSigner::sign(const QString& requestId,
                          const QString& fileName,
                          const QString& input,
                          const QString& output) {
    Job job;
    job.requestId = requestId;
    job.fileName = fileName;
    job.output = output;
    job.thumbprint = m_settings->certificateThumbprint();

    if (m_settings->signtoolPath().isEmpty() || !QFileInfo::exists(m_settings->signtoolPath())) {
        fail(job, QStringLiteral("signtool.exe was not found; set its path in Settings"));
        return;
    }
    const CertificateInfo certificate = CertificateStore::describe(job.thumbprint);
    if (!certificate.found) {
        fail(job, QStringLiteral("the signing certificate is no longer in the user's store"));
        return;
    }
    job.selfSigned = certificate.selfSigned;

    // Sign a copy; the unsigned original stays what the manifest described.
    QFile::remove(output);
    if (!QFileInfo(output).dir().mkpath(QStringLiteral(".")) || !QFile::copy(input, output)) {
        fail(job, QStringLiteral("the file could not be copied to the signed directory"));
        return;
    }
    runSign(job);
}

void SigntoolSigner::runSign(Job job) {
    QStringList arguments{QStringLiteral("sign"),
                          QStringLiteral("/v"),
                          QStringLiteral("/sha1"),
                          job.thumbprint,
                          QStringLiteral("/fd"),
                          QStringLiteral("sha256")};
    const QString timestamp = m_settings->timestampUrl();
    if (!timestamp.isEmpty()) {
        arguments << QStringLiteral("/tr") << timestamp << QStringLiteral("/td")
                  << QStringLiteral("sha256");
    }
    arguments << QDir::toNativeSeparators(job.output);

    Q_EMIT progress(job.requestId, job.fileName, QStringLiteral("signtool sign"));
    QProcess* process = startSigntool(arguments, job);
    connect(
        process,
        &QProcess::finished,
        this,
        [this, process, job](int exitCode, QProcess::ExitStatus status) mutable {
            job.transcript += process->readAll();
            process->deleteLater();
            if (status != QProcess::NormalExit || exitCode != 0) {
                fail(job, QStringLiteral("signtool sign failed: %1").arg(tail(job.transcript)));
                return;
            }
            const std::optional<QString> signer = Authenticode::signerThumbprint(job.output);
            if (!signer.has_value()) {
                fail(job, QStringLiteral("the file carries no signature after signing"));
                return;
            }
            if (*signer != job.thumbprint) {
                fail(
                    job,
                    QStringLiteral("the file was signed by another certificate (%1)").arg(*signer));
                return;
            }
            if (job.selfSigned) {
                // No chain to verify; the thumbprint check is the whole truth here.
                Q_EMIT finished(job.requestId, job.fileName, true, QString());
                return;
            }
            runVerify(job);
        });
}

void SigntoolSigner::runVerify(Job job) {
    Q_EMIT progress(job.requestId, job.fileName, QStringLiteral("signtool verify /pa"));
    QProcess* process = startSigntool({QStringLiteral("verify"),
                                       QStringLiteral("/pa"),
                                       QStringLiteral("/v"),
                                       QDir::toNativeSeparators(job.output)},
                                      job);
    connect(process,
            &QProcess::finished,
            this,
            [this, process, job](int exitCode, QProcess::ExitStatus status) mutable {
                job.transcript += process->readAll();
                process->deleteLater();
                if (status != QProcess::NormalExit || exitCode != 0) {
                    fail(job,
                         QStringLiteral("signtool verify failed: %1").arg(tail(job.transcript)));
                    return;
                }
                Q_EMIT finished(job.requestId, job.fileName, true, QString());
            });
}

QProcess* SigntoolSigner::startSigntool(const QStringList& arguments, Job job) {
    auto* process = new QProcess(this);
    process->setProgram(m_settings->signtoolPath());
    process->setArguments(arguments);
    process->setProcessChannelMode(QProcess::MergedChannels);
    // A card waiting for its PIN can take a while; a hung signtool must not take the desk with it.
    auto* timeout = new QTimer(process);
    timeout->setSingleShot(true);
    timeout->setInterval(kTimeoutMs);
    connect(timeout, &QTimer::timeout, process, [process] {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, job](QProcess::ProcessError) {
        if (process->state() == QProcess::NotRunning && process->exitCode() == 0 &&
            process->error() == QProcess::FailedToStart) {
            process->deleteLater();
            fail(job,
                 QStringLiteral("signtool could not be started: %1").arg(process->errorString()));
        }
    });
    timeout->start();
    process->start();
    return process;
}

void SigntoolSigner::fail(const Job& job, const QString& error) {
    QFile::remove(job.output);
    Q_EMIT finished(job.requestId, job.fileName, false, error);
}

QString SigntoolSigner::tail(const QByteArray& transcript) {
    const QString text = QString::fromLocal8Bit(transcript).trimmed();
    QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString& line : lines) {
        line = line.trimmed();
    }
    lines.removeAll(QString());
    // signtool prints its error at the end; keep the last few lines.
    while (lines.size() > 3) {
        lines.removeFirst();
    }
    return lines.join(QStringLiteral(" | "));
}
