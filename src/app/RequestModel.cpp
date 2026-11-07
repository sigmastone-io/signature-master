#include "RequestModel.h"

#include "RequestStore.h"
#include "SigningService.h"

#include <QLocale>
#include <QVariantList>

namespace {
RequestModel* g_instance = nullptr;

QString shortHash(const QString& hash, int groups) {
    if (hash.size() < 16) {
        return hash;
    }
    QString out;
    for (int i = 0; i < groups; ++i) {
        out += hash.mid(i * 4, 4) + QLatin1Char(' ');
    }
    out += QStringLiteral("… ");
    for (int i = groups; i > 0; --i) {
        out += hash.mid(hash.size() - i * 4, 4);
        if (i > 1) {
            out += QLatin1Char(' ');
        }
    }
    return out;
}
} // namespace

RequestModel::RequestModel(RequestStore* store, SigningService* signing, QObject* parent)
    : QAbstractListModel(parent), m_store(store), m_signing(signing) {
    Q_ASSERT(g_instance == nullptr);
    g_instance = this;

    connect(m_store, &RequestStore::added, this, [this] { rebuild(); });
    connect(m_store, &RequestStore::removed, this, [this] { rebuild(); });
    connect(m_store, &RequestStore::changed, this, [this](const QString& id) {
        const int row = rowOf(id);
        const std::optional<SigningRequest> request = m_store->find(id);
        if (row >= 0 && request.has_value() && matches(*request)) {
            const QModelIndex cell = index(row);
            Q_EMIT dataChanged(cell, cell);
            Q_EMIT rowChanged(row);
        } else {
            rebuild();
        }
    });
    rebuild();
}

RequestModel::~RequestModel() {
    g_instance = nullptr;
}

RequestModel* RequestModel::instance() {
    Q_ASSERT(g_instance != nullptr);
    return g_instance;
}

RequestModel* RequestModel::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    RequestModel* model = instance();
    QJSEngine::setObjectOwnership(model, QJSEngine::CppOwnership);
    return model;
}

int RequestModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : count();
}

QVariant RequestModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size()) {
        return {};
    }
    const std::optional<SigningRequest> request = m_store->find(m_ids.at(index.row()));
    if (!request.has_value()) {
        return {};
    }
    return roleValue(*request, role);
}

QHash<int, QByteArray> RequestModel::roleNames() const {
    return {
        {IdRole, "id"},
        {RepositoryRole, "repository"},
        {NameRole, "name"},
        {RefRole, "ref"},
        {RefShortRole, "refShort"},
        {CommitRole, "commit"},
        {CommitShortRole, "commitShort"},
        {WorkflowRole, "workflow"},
        {JobRole, "job"},
        {RunIdRole, "runId"},
        {RunUrlRole, "runUrl"},
        {ActorRole, "actor"},
        {AuthenticatedAsRole, "authenticatedAs"},
        {RequestedAtRole, "requestedAt"},
        {UpdatedAtRole, "updatedAt"},
        {StateRole, "state"},
        {StateLabelRole, "stateLabel"},
        {ReasonRole, "reason"},
        {ApprovedByRole, "approvedBy"},
        {FileCountRole, "fileCount"},
        {FilesRole, "files"},
        {ChecksRole, "checks"},
        {UnverifiedRole, "unverified"},
    };
}

void RequestModel::setFilter(const QString& filter) {
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    Q_EMIT filterChanged();
    rebuild();
}

QVariantMap RequestModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= m_ids.size()) {
        return map;
    }
    const std::optional<SigningRequest> request = m_store->find(m_ids.at(row));
    if (!request.has_value()) {
        return map;
    }
    const QHash<int, QByteArray> names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        map.insert(QString::fromLatin1(it.value()), roleValue(*request, it.key()));
    }
    return map;
}

int RequestModel::rowOf(const QString& id) const {
    return static_cast<int>(m_ids.indexOf(id));
}

bool RequestModel::reject(const QString& id, const QString& reason) {
    return m_store->reject(id, reason);
}

bool RequestModel::sign(const QString& id) {
    return m_signing->approve(id, qEnvironmentVariable("USERNAME", QStringLiteral("signer")));
}

bool RequestModel::matches(const SigningRequest& request) const {
    if (m_filter == QStringLiteral("pending")) {
        return request.state == RequestState::Pending || request.state == RequestState::Received ||
               request.state == RequestState::Signing;
    }
    if (m_filter == QStringLiteral("signed")) {
        return request.state == RequestState::Signed;
    }
    if (m_filter == QStringLiteral("refused")) {
        return request.state == RequestState::Refused || request.state == RequestState::Rejected ||
               request.state == RequestState::Failed || request.state == RequestState::Expired;
    }
    return true;
}

void RequestModel::rebuild() {
    beginResetModel();
    m_ids.clear();
    const QList<SigningRequest>& requests = m_store->requests();
    for (auto it = requests.crbegin(); it != requests.crend(); ++it) {
        if (matches(*it)) {
            m_ids.append(it->id);
        }
    }
    endResetModel();
    Q_EMIT countChanged();
}

QVariant RequestModel::roleValue(const SigningRequest& request, int role) const {
    switch (role) {
    case IdRole:
        return request.id;
    case RepositoryRole:
        return request.repository;
    case NameRole:
        return request.name();
    case RefRole:
        return request.ref;
    case RefShortRole:
        return request.refShort();
    case CommitRole:
        return request.commit;
    case CommitShortRole:
        return request.commit.left(12);
    case WorkflowRole:
        return request.workflow;
    case JobRole:
        return request.job;
    case RunIdRole:
        return request.runId;
    case RunUrlRole:
        return request.runUrl;
    case ActorRole:
        return request.actor;
    case AuthenticatedAsRole:
        return request.authenticatedAs;
    case RequestedAtRole:
        return request.requestedAt.toLocalTime();
    case UpdatedAtRole:
        return request.updatedAt.toLocalTime();
    case StateRole:
        return toString(request.state);
    case StateLabelRole:
        return toLabel(request.state);
    case ReasonRole:
        return request.reason;
    case ApprovedByRole:
        return request.approvedBy;
    case FileCountRole:
        return static_cast<int>(request.files.size());
    case UnverifiedRole:
        return request.anyCheckUnverified();
    case FilesRole: {
        QVariantList list;
        for (const RequestFile& file : request.files) {
            list.append(QVariantMap{
                {QStringLiteral("name"), file.name},
                {QStringLiteral("size"), file.size},
                {QStringLiteral("sizeText"), QLocale().formattedDataSize(file.size, 1)},
                {QStringLiteral("sha256"), file.sha256},
                {QStringLiteral("sha256Short"), shortHash(file.sha256, 2)},
                {QStringLiteral("version"), file.version},
                {QStringLiteral("state"), toString(file.state)},
                {QStringLiteral("error"), file.error},
            });
        }
        return list;
    }
    case ChecksRole: {
        QVariantList list;
        for (const Check& item : request.checks) {
            list.append(QVariantMap{
                {QStringLiteral("key"), item.key},
                {QStringLiteral("title"), item.title},
                {QStringLiteral("outcome"), toString(item.outcome)},
                {QStringLiteral("detail"), item.detail},
            });
        }
        return list;
    }
    default:
        return {};
    }
}
