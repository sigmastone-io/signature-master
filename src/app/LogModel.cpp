// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "LogModel.h"

#include "DisplayPath.h"

#include <QFile>
#include <QTextStream>

namespace {
LogModel* g_instance = nullptr;

QString refShort(const QString& ref) {
    const QString tags = QStringLiteral("refs/tags/");
    return ref.startsWith(tags) ? ref.mid(tags.size()) : ref;
}

QString csvField(const QString& value) {
    QString out = value;
    out.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}
} // namespace

LogModel::LogModel(SignatureLog* log, QObject* parent) : QAbstractListModel(parent), m_log(log) {
    Q_ASSERT(g_instance == nullptr);
    g_instance = this;
    const QList<LogEntry> entries = m_log->readAll();
    for (auto it = entries.crbegin(); it != entries.crend(); ++it) {
        m_entries.append(*it);
    }
    connect(m_log, &SignatureLog::appended, this, [this](const LogEntry& entry) {
        m_entries.prepend(entry);
        rebuild();
    });
    rebuild();
}

LogModel::~LogModel() {
    g_instance = nullptr;
}

LogModel* LogModel::instance() {
    Q_ASSERT(g_instance != nullptr);
    return g_instance;
}

LogModel* LogModel::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    LogModel* model = instance();
    QJSEngine::setObjectOwnership(model, QJSEngine::CppOwnership);
    return model;
}

int LogModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : count();
}

QVariant LogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return {};
    }
    const LogEntry& entry = m_entries.at(m_visible.at(index.row()));
    switch (role) {
    case TimeRole:
        return entry.time.toLocalTime();
    case RequestIdRole:
        return entry.requestId;
    case RepositoryRole:
        return entry.repository;
    case NameRole:
        return entry.repository.section(QLatin1Char('/'), 1, 1);
    case RefRole:
        return entry.ref;
    case RefShortRole:
        return refShort(entry.ref);
    case CommitRole:
        return entry.commit;
    case FileRole:
        return entry.file;
    case Sha256Role:
        return entry.sha256;
    case Sha256ShortRole:
        return entry.sha256.size() >= 16
                   ? entry.sha256.left(8) + QStringLiteral("…") + entry.sha256.right(8)
                   : entry.sha256;
    case OutcomeRole:
        return entry.outcome;
    case ByRole:
        return entry.by;
    case ReasonRole:
        return entry.reason;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const {
    return {
        {TimeRole, "time"},
        {RequestIdRole, "requestId"},
        {RepositoryRole, "repository"},
        {NameRole, "name"},
        {RefRole, "ref"},
        {RefShortRole, "refShort"},
        {CommitRole, "commit"},
        {FileRole, "file"},
        {Sha256Role, "sha256"},
        {Sha256ShortRole, "sha256Short"},
        {OutcomeRole, "outcome"},
        {ByRole, "by"},
        {ReasonRole, "reason"},
    };
}

void LogModel::setFilter(const QString& filter) {
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    Q_EMIT filterChanged();
    rebuild();
}

void LogModel::setSearch(const QString& search) {
    const QString trimmed = search.trimmed();
    if (m_search == trimmed) {
        return;
    }
    m_search = trimmed;
    Q_EMIT searchChanged();
    rebuild();
}

QString LogModel::path() const {
    return DisplayPath::of(m_log->path());
}

QString LogModel::exportCsv(const QUrl& file) const {
    QFile out(file.isLocalFile() ? file.toLocalFile() : file.toString());
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return out.errorString();
    }
    QTextStream stream(&out);
    stream << "time,request,repository,ref,commit,file,sha256,outcome,by,reason\n";
    for (const int row : m_visible) {
        const LogEntry& entry = m_entries.at(row);
        const QStringList fields{entry.time.toUTC().toString(Qt::ISODateWithMs),
                                 entry.requestId,
                                 entry.repository,
                                 entry.ref,
                                 entry.commit,
                                 entry.file,
                                 entry.sha256,
                                 entry.outcome,
                                 entry.by,
                                 entry.reason};
        QStringList quoted;
        for (const QString& field : fields) {
            quoted.append(csvField(field));
        }
        stream << quoted.join(QLatin1Char(',')) << '\n';
    }
    return {};
}

bool LogModel::matches(const LogEntry& entry) const {
    if (m_filter == QStringLiteral("automatic")) {
        if (entry.by != QStringLiteral("automatic")) {
            return false;
        }
    } else if (m_filter != QStringLiteral("all") && entry.outcome != m_filter) {
        return false;
    }
    if (m_search.isEmpty()) {
        return true;
    }
    for (const QString& field : {entry.repository,
                                 entry.ref,
                                 entry.file,
                                 entry.sha256,
                                 entry.commit,
                                 entry.by,
                                 entry.reason}) {
        if (field.contains(m_search, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

void LogModel::rebuild() {
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        if (matches(m_entries.at(i))) {
            m_visible.append(i);
        }
    }
    endResetModel();
    Q_EMIT countChanged();
}
