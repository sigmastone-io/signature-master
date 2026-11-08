#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSettings>
#include <QString>
#include <QStringList>

// Every user-facing setting, persisted as it changes. Exposed to QML as the Settings singleton;
// C++ reaches it through Settings::instance(). Nothing secret is kept here: the certificate PIN
// goes to the credential store and never through this class.
class Settings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool autoSign READ autoSign WRITE setAutoSign NOTIFY autoSignChanged)
    Q_PROPERTY(bool rememberPin READ rememberPin WRITE setRememberPin NOTIFY rememberPinChanged)
    Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY
                   startWithWindowsChanged)

    Q_PROPERTY(QString certificateThumbprint READ certificateThumbprint WRITE
                   setCertificateThumbprint NOTIFY certificateThumbprintChanged)
    Q_PROPERTY(
        QString timestampUrl READ timestampUrl WRITE setTimestampUrl NOTIFY timestampUrlChanged)
    Q_PROPERTY(
        QString signtoolPath READ signtoolPath WRITE setSigntoolPath NOTIFY signtoolPathChanged)

    Q_PROPERTY(QString bindAddress READ bindAddress WRITE setBindAddress NOTIFY bindAddressChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool pauseListening READ pauseListening WRITE setPauseListening NOTIFY
                   pauseListeningChanged)
    Q_PROPERTY(
        QString workDirectory READ workDirectory WRITE setWorkDirectory NOTIFY workDirectoryChanged)

    Q_PROPERTY(
        QString allowedOwner READ allowedOwner WRITE setAllowedOwner NOTIFY allowedOwnerChanged)
    Q_PROPERTY(QStringList allowedRepositories READ allowedRepositories WRITE setAllowedRepositories
                   NOTIFY allowedRepositoriesChanged)
    Q_PROPERTY(QString allowedRefs READ allowedRefs WRITE setAllowedRefs NOTIFY allowedRefsChanged)

    // Notices the person has dismissed, by key; a notice whose key is here stays hidden until
    // the state it describes goes away and comes back.
    Q_PROPERTY(QStringList dismissedNotices READ dismissedNotices NOTIFY dismissedNoticesChanged)

    Q_PROPERTY(QString filePath READ filePath CONSTANT)

public:
    explicit Settings(QObject* parent = nullptr);
    // Settings kept in a file of the caller's choosing, for tests.
    explicit Settings(const QString& iniPath, QObject* parent = nullptr);
    ~Settings() override;

    static Settings* instance();
    static Settings* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QString filePath() const;

    bool autoSign() const { return m_autoSign; }
    void setAutoSign(bool on);
    bool rememberPin() const { return m_rememberPin; }
    void setRememberPin(bool on);
    bool startWithWindows() const;
    void setStartWithWindows(bool on);

    QString certificateThumbprint() const { return m_certificateThumbprint; }
    void setCertificateThumbprint(const QString& thumbprint);
    QString timestampUrl() const { return m_timestampUrl; }
    void setTimestampUrl(const QString& url);
    QString signtoolPath() const { return m_signtoolPath; }
    void setSigntoolPath(const QString& path);

    QStringList dismissedNotices() const { return m_dismissedNotices; }
    Q_INVOKABLE bool isDismissed(const QString& key) const;
    Q_INVOKABLE void dismissNotice(const QString& key);
    // Forgets every dismissed notice whose key starts with `prefix`, so that it shows again when
    // its state returns.
    void forgetNotices(const QString& prefix);

    QString bindAddress() const { return m_bindAddress; }
    void setBindAddress(const QString& address);
    int port() const { return m_port; }
    void setPort(int port);
    bool pauseListening() const { return m_pauseListening; }
    void setPauseListening(bool paused);
    QString workDirectory() const { return m_workDirectory; }
    void setWorkDirectory(const QString& directory);

    QString allowedOwner() const { return m_allowedOwner; }
    void setAllowedOwner(const QString& owner);
    QStringList allowedRepositories() const { return m_allowedRepositories; }
    void setAllowedRepositories(const QStringList& repositories);
    Q_INVOKABLE void addAllowedRepository(const QString& repository);
    Q_INVOKABLE void removeAllowedRepository(const QString& repository);
    QString allowedRefs() const { return m_allowedRefs; }
    void setAllowedRefs(const QString& pattern);

    // Where signtool.exe is on this machine, from the newest Windows SDK, or empty.
    static QString locateSigntool();

Q_SIGNALS:
    void autoSignChanged();
    void rememberPinChanged();
    void startWithWindowsChanged();
    void certificateThumbprintChanged();
    void timestampUrlChanged();
    void signtoolPathChanged();
    void bindAddressChanged();
    void portChanged();
    void pauseListeningChanged();
    void workDirectoryChanged();
    void allowedOwnerChanged();
    void allowedRepositoriesChanged();
    void allowedRefsChanged();
    void dismissedNoticesChanged();

private:
    void load();
    template <typename T>
    bool update(const char* key, T& field, const T& value);

    QSettings m_store;
    bool m_autoSign = false;
    bool m_rememberPin = false;
    QString m_certificateThumbprint;
    QString m_timestampUrl;
    QString m_signtoolPath;
    QString m_bindAddress;
    int m_port = 7443;
    bool m_pauseListening = false;
    QString m_workDirectory;
    QString m_allowedOwner;
    QStringList m_allowedRepositories;
    QString m_allowedRefs;
    QStringList m_dismissedNotices;
};
