#include "RequestStore.h"

#include "FileVersion.h"
#include "Settings.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>

namespace {

constexpr auto kIdentity = "identity";
constexpr auto kOwner = "owner";
constexpr auto kRepository = "repository";
constexpr auto kRef = "ref";
constexpr auto kCertificate = "certificate";
constexpr auto kFiles = "files";

QString stringField(const QJsonObject& object, const char* key) {
    const QJsonValue value = object.value(QLatin1String(key));
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 0);
    }
    return value.toString().trimmed();
}

bool isSha256(const QString& text) {
    static const QRegularExpression hex(QStringLiteral("^[0-9a-f]{64}$"));
    return hex.match(text).hasMatch();
}

QString filesSummary(int count) {
    return count == 1 ? QStringLiteral("1 file") : QStringLiteral("%1 files").arg(count);
}

} // namespace

RequestStore::RequestStore(Settings* settings, QObject* parent)
    : QObject(parent), m_settings(settings) {
    m_certificateProbe = [this] {
        return CertificateStore::describe(m_settings->certificateThumbprint());
    };
    wipeLeftovers();
    m_housekeeping.setInterval(60 * 1000);
    connect(&m_housekeeping, &QTimer::timeout, this, &RequestStore::expireStale);
    m_housekeeping.start();
}

void RequestStore::setCertificateProbe(std::function<CertificateInfo()> probe) {
    m_certificateProbe = std::move(probe);
}

RequestStore::Reply RequestStore::error(int status, const QString& message) {
    return {status, QJsonObject{{QStringLiteral("error"), message}}};
}

bool RequestStore::safeFileName(const QString& name) {
    if (name.isEmpty() || name.size() > 200 || name == QStringLiteral(".") ||
        name == QStringLiteral("..") || name.startsWith(QLatin1Char('.'))) {
        return false;
    }
    static const QRegularExpression allowed(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._ +-]*$"));
    return allowed.match(name).hasMatch();
}

RequestStore::Reply RequestStore::create(const QJsonObject& manifest, const AuthResult& auth) {
    SigningRequest request;
    request.id = QUuid::createUuid().toString(QUuid::WithoutBraces).remove(QLatin1Char('-'));
    request.requestedAt = QDateTime::currentDateTimeUtc();
    request.updatedAt = request.requestedAt;
    request.repository = stringField(manifest, "repository");
    request.ref = stringField(manifest, "ref");
    request.commit = stringField(manifest, "commit");
    request.workflow = stringField(manifest, "workflow");
    request.job = stringField(manifest, "job");
    request.runId = stringField(manifest, "run_id");
    request.runUrl = stringField(manifest, "run_url");
    request.actor = stringField(manifest, "actor");
    request.authenticatedAs = auth.identity;

    // A malformed manifest is a bug in the caller, not a request to list.
    if (request.repository.count(QLatin1Char('/')) != 1 || request.owner().isEmpty() ||
        request.name().isEmpty()) {
        return error(400, QStringLiteral("repository must be owner/name"));
    }
    if (request.ref.isEmpty()) {
        return error(400, QStringLiteral("ref is required"));
    }
    const QJsonArray files = manifest.value(QLatin1String(kFiles)).toArray();
    if (files.isEmpty()) {
        return error(400, QStringLiteral("files must list at least one file"));
    }
    for (const QJsonValue& value : files) {
        const QJsonObject object = value.toObject();
        RequestFile file;
        file.name = stringField(object, "name");
        file.size = static_cast<qint64>(object.value(QStringLiteral("size")).toDouble());
        file.sha256 = stringField(object, "sha256").toLower();
        if (!safeFileName(file.name)) {
            return error(400, QStringLiteral("file name \"%1\" is not acceptable").arg(file.name));
        }
        if (file.size <= 0) {
            return error(400, QStringLiteral("file \"%1\" needs a positive size").arg(file.name));
        }
        if (!isSha256(file.sha256)) {
            return error(400, QStringLiteral("file \"%1\" needs a SHA-256 in hex").arg(file.name));
        }
        for (const RequestFile& other : request.files) {
            if (other.name == file.name) {
                return error(400, QStringLiteral("file \"%1\" is listed twice").arg(file.name));
            }
        }
        request.files.append(file);
    }

    // The checks, in the order the window shows them.
    request.checks.append({QLatin1String(kIdentity),
                           QStringLiteral("Caller identity verified through GitHub"),
                           auth.outcome,
                           auth.detail});

    const QString owner = m_settings->allowedOwner();
    request.checks.append({QLatin1String(kOwner),
                           QStringLiteral("Owner is %1").arg(owner),
                           request.owner() == owner ? CheckOutcome::Passed : CheckOutcome::Failed,
                           request.owner() == owner
                               ? QString()
                               : QStringLiteral("owner %1 is not %2").arg(request.owner(), owner)});

    const bool repositoryAllowed = m_settings->allowedRepositories().contains(request.name());
    request.checks.append(
        {QLatin1String(kRepository),
         QStringLiteral("Repository is on the allow list"),
         repositoryAllowed ? CheckOutcome::Passed : CheckOutcome::Failed,
         repositoryAllowed ? QString()
                           : QStringLiteral("%1 is not on the allow list").arg(request.name())});

    const QString pattern = m_settings->allowedRefs();
    const QRegularExpression refPattern(QRegularExpression::wildcardToRegularExpression(pattern));
    const bool refAllowed = refPattern.match(request.ref).hasMatch();
    request.checks.append(
        {QLatin1String(kRef),
         QStringLiteral("Ref matches %1").arg(pattern),
         refAllowed ? CheckOutcome::Passed : CheckOutcome::Failed,
         refAllowed ? QString() : QStringLiteral("ref is not a release tag (%1)").arg(pattern)});

    const CertificateInfo certificate = m_certificateProbe();
    const bool certificateUsable =
        certificate.found && certificate.notAfter > QDateTime::currentDateTimeUtc();
    request.checks.append({QLatin1String(kCertificate),
                           QStringLiteral("A signing certificate is in place"),
                           certificateUsable ? CheckOutcome::Passed : CheckOutcome::Failed,
                           certificateUsable   ? certificate.subject
                           : certificate.found ? QStringLiteral("the certificate has expired")
                                               : QStringLiteral("no certificate is chosen")});

    request.checks.append(
        {QLatin1String(kFiles),
         QStringLiteral("Every file matches its manifest hash"),
         CheckOutcome::Pending,
         QStringLiteral("waiting for %1").arg(filesSummary(request.files.size()))});

    m_lastRequestAt = request.requestedAt;

    if (request.anyCheckFailed()) {
        QString reason;
        for (const Check& item : request.checks) {
            if (item.outcome == CheckOutcome::Failed) {
                reason = item.detail;
                break;
            }
        }
        request.state = RequestState::Refused;
        request.reason = reason;
        m_requests.append(request);
        Q_EMIT added(request.id);
        Q_EMIT entered(request.id, RequestState::Refused);
        return {403, request.toJson()};
    }

    request.workDir = QDir(m_settings->workDirectory()).filePath(request.id);
    QDir dir(request.workDir);
    if (!dir.mkpath(QStringLiteral("files")) || !dir.mkpath(QStringLiteral("signed"))) {
        return error(500, QStringLiteral("the work directory could not be created"));
    }
    m_requests.append(request);
    Q_EMIT added(request.id);
    return {201, request.toJson()};
}

RequestStore::Reply
RequestStore::receiveFile(const QString& id, const QString& name, const QByteArray& bytes) {
    SigningRequest* request = mutableFind(id);
    if (request == nullptr) {
        return error(404, QStringLiteral("no such request"));
    }
    if (request->state != RequestState::Received) {
        return error(
            409,
            QStringLiteral("the request is %1 and takes no files").arg(toString(request->state)));
    }
    RequestFile* file = nullptr;
    for (RequestFile& candidate : request->files) {
        if (candidate.name == name) {
            file = &candidate;
        }
    }
    if (file == nullptr) {
        return error(404, QStringLiteral("the manifest does not list \"%1\"").arg(name));
    }
    if (file->state != FileState::Expected) {
        return error(409, QStringLiteral("\"%1\" was already received").arg(name));
    }

    auto refuse = [&](const QString& problem) {
        file->state = FileState::Failed;
        file->error = problem;
        if (Check* check = request->check(QLatin1String(kFiles))) {
            check->outcome = CheckOutcome::Failed;
            check->detail = problem;
        }
        transition(*request, RequestState::Refused, problem);
        return Reply{400, request->toJson()};
    };

    if (bytes.size() != file->size) {
        return refuse(QStringLiteral("%1: %2 bytes arrived, the manifest says %3")
                          .arg(name)
                          .arg(bytes.size())
                          .arg(file->size));
    }
    const QString digest =
        QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    if (digest != file->sha256) {
        return refuse(QStringLiteral("%1: the SHA-256 differs from the manifest").arg(name));
    }

    const QString path = QDir(request->workDir).filePath(QStringLiteral("files/") + name);
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(bytes) != bytes.size() || !out.commit()) {
        file->state = FileState::Failed;
        file->error = QStringLiteral("could not be written to the work directory");
        transition(
            *request, RequestState::Failed, QStringLiteral("%1 could not be written").arg(name));
        return {500, request->toJson()};
    }
    file->state = FileState::Received;
    file->path = path;
    file->version = FileVersion::of(path);

    if (request->allFilesReceived()) {
        if (Check* check = request->check(QLatin1String(kFiles))) {
            check->outcome = CheckOutcome::Passed;
            check->detail =
                QStringLiteral("%1 received and hashed").arg(filesSummary(request->files.size()));
        }
        transition(*request, RequestState::Pending, QString());
    } else {
        request->updatedAt = QDateTime::currentDateTimeUtc();
        Q_EMIT changed(request->id);
    }
    return {200, request->toJson()};
}

RequestStore::Reply RequestStore::describe(const QString& id) const {
    const std::optional<SigningRequest> request = find(id);
    if (!request.has_value()) {
        return error(404, QStringLiteral("no such request"));
    }
    return {200, request->toJson()};
}

RequestStore::Reply RequestStore::cancel(const QString& id) {
    SigningRequest* request = mutableFind(id);
    if (request == nullptr) {
        return error(404, QStringLiteral("no such request"));
    }
    if (request->state == RequestState::Signing) {
        return error(409, QStringLiteral("the request is being signed"));
    }
    if (!isTerminal(request->state)) {
        transition(*request, RequestState::Expired, QStringLiteral("the job gave up"));
    }
    return {200, request->toJson()};
}

QString RequestStore::signedFilePath(const QString& id, const QString& name) const {
    const std::optional<SigningRequest> request = find(id);
    if (!request.has_value() || request->state != RequestState::Signed) {
        return {};
    }
    for (const RequestFile& file : request->files) {
        if (file.name == name && file.state == FileState::Signed) {
            return file.signedPath;
        }
    }
    return {};
}

bool RequestStore::reject(const QString& id, const QString& reason) {
    SigningRequest* request = mutableFind(id);
    if (request == nullptr || request->state != RequestState::Pending) {
        return false;
    }
    transition(*request,
               RequestState::Rejected,
               reason.trimmed().isEmpty() ? QStringLiteral("rejected by the signer")
                                          : reason.trimmed());
    return true;
}

std::optional<SigningRequest> RequestStore::find(const QString& id) const {
    for (const SigningRequest& request : m_requests) {
        if (request.id == id) {
            return request;
        }
    }
    return std::nullopt;
}

int RequestStore::pendingCount() const {
    int count = 0;
    for (const SigningRequest& request : m_requests) {
        if (request.state == RequestState::Pending) {
            ++count;
        }
    }
    return count;
}

SigningRequest* RequestStore::mutableFind(const QString& id) {
    for (SigningRequest& request : m_requests) {
        if (request.id == id) {
            return &request;
        }
    }
    return nullptr;
}

void RequestStore::transition(SigningRequest& request, RequestState state, const QString& reason) {
    request.state = state;
    request.reason = reason;
    request.updatedAt = QDateTime::currentDateTimeUtc();
    if (isTerminal(state) && state != RequestState::Signed && !request.workDir.isEmpty()) {
        // Nothing left to sign or to fetch: the files go now, the entry stays for the list.
        wipe(request.workDir);
    }
    Q_EMIT changed(request.id);
    Q_EMIT entered(request.id, state);
}

void RequestStore::expireStale() {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QStringList dropped;
    for (SigningRequest& request : m_requests) {
        if ((request.state == RequestState::Pending || request.state == RequestState::Received) &&
            request.requestedAt.secsTo(now) > kApprovalTimeoutMinutes * 60) {
            transition(
                request,
                RequestState::Expired,
                QStringLiteral("no approval within %1 minutes").arg(kApprovalTimeoutMinutes));
        } else if (isTerminal(request.state) &&
                   request.updatedAt.secsTo(now) > kRetentionHours * 3600) {
            dropped.append(request.id);
        }
    }
    for (const QString& id : dropped) {
        for (qsizetype i = 0; i < m_requests.size(); ++i) {
            if (m_requests[i].id == id) {
                wipe(m_requests[i].workDir);
                m_requests.removeAt(i);
                Q_EMIT removed(id);
                break;
            }
        }
    }
}

void RequestStore::wipeLeftovers() {
    // Request directories are 32 hex characters; anything else in the work directory is not ours.
    static const QRegularExpression idShape(QStringLiteral("^[0-9a-f]{32}$"));
    const QDir work(m_settings->workDirectory());
    const QStringList entries = work.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (idShape.match(entry).hasMatch()) {
            wipe(work.filePath(entry));
        }
    }
}

void RequestStore::wipe(const QString& directory) {
    if (!directory.isEmpty()) {
        QDir(directory).removeRecursively();
    }
}
