// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SignatureLog.h"

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

// The signature log for QML, newest first, narrowed by a filter and a search. Exposed as the
// LogEntries singleton. It reads the file once and then follows what the log appends.
class LogModel : public QAbstractListModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(LogEntries)
    QML_SINGLETON

    // "all", "signed", "refused", "rejected", "automatic"
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int total READ total NOTIFY countChanged)
    Q_PROPERTY(QString path READ path CONSTANT)

public:
    enum Role {
        TimeRole = Qt::UserRole + 1,
        RequestIdRole,
        RepositoryRole,
        NameRole,
        RefRole,
        RefShortRole,
        CommitRole,
        FileRole,
        Sha256Role,
        Sha256ShortRole,
        OutcomeRole,
        ByRole,
        ReasonRole,
    };

    explicit LogModel(SignatureLog* log, QObject* parent = nullptr);
    ~LogModel() override;

    static LogModel* instance();
    static LogModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString& filter);
    QString search() const { return m_search; }
    void setSearch(const QString& search);
    int count() const { return static_cast<int>(m_visible.size()); }
    int total() const { return static_cast<int>(m_entries.size()); }
    QString path() const;

    // Writes the visible rows as CSV. Returns an empty string, or the error.
    Q_INVOKABLE QString exportCsv(const QUrl& file) const;

Q_SIGNALS:
    void filterChanged();
    void searchChanged();
    void countChanged();

private:
    bool matches(const LogEntry& entry) const;
    void rebuild();

    SignatureLog* m_log;
    QList<LogEntry> m_entries; // newest first
    QList<int> m_visible;      // indexes into m_entries
    QString m_filter = QStringLiteral("all");
    QString m_search;
};
