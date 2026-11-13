// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SigningRequest.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantMap>

class RequestStore;
class SigningService;

// The request list for QML, newest first, narrowed by a filter. Exposed as the Requests singleton.
class RequestModel : public QAbstractListModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(Requests)
    QML_SINGLETON

    // "all", "pending" (received and pending), "signed", "refused" (every negative end)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        RepositoryRole,
        NameRole,
        RefRole,
        RefShortRole,
        CommitRole,
        CommitShortRole,
        WorkflowRole,
        JobRole,
        RunIdRole,
        RunUrlRole,
        ActorRole,
        AuthenticatedAsRole,
        RequestedAtRole,
        UpdatedAtRole,
        StateRole,
        StateLabelRole,
        ReasonRole,
        ApprovedByRole,
        FileCountRole,
        FilesRole,
        ChecksRole,
        UnverifiedRole,
    };

    RequestModel(RequestStore* store, SigningService* signing, QObject* parent = nullptr);
    ~RequestModel() override;

    static RequestModel* instance();
    static RequestModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString& filter);
    int count() const { return static_cast<int>(m_ids.size()); }

    // Every role of one row as a map, for the detail pane.
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int rowOf(const QString& id) const;
    Q_INVOKABLE bool reject(const QString& id, const QString& reason);
    // The person at the desk approved it; signing starts at once.
    Q_INVOKABLE bool sign(const QString& id);

Q_SIGNALS:
    void filterChanged();
    void countChanged();
    // A visible row's data changed in place.
    void rowChanged(int row);

private:
    bool matches(const SigningRequest& request) const;
    void rebuild();
    QVariant roleValue(const SigningRequest& request, int role) const;

    RequestStore* m_store;
    SigningService* m_signing;
    QString m_filter = QStringLiteral("all");
    QStringList m_ids; // visible rows, newest first
};
