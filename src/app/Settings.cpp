#include "Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

Settings* g_instance = nullptr;

constexpr auto kAutoSign = "signing/autoSign";
constexpr auto kRememberPin = "signing/rememberPin";
constexpr auto kCertificateThumbprint = "certificate/thumbprint";
constexpr auto kTimestampUrl = "certificate/timestampUrl";
constexpr auto kSigntoolPath = "certificate/signtoolPath";
constexpr auto kBindAddress = "listening/bindAddress";
constexpr auto kPort = "listening/port";
constexpr auto kPauseListening = "listening/paused";
constexpr auto kWorkDirectory = "listening/workDirectory";
constexpr auto kAllowedOwner = "callers/owner";
constexpr auto kAllowedRepositories = "callers/repositories";
constexpr auto kAllowedRefs = "callers/refs";

constexpr auto kRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto kRunValue = "SignatureMaster";

QString defaultWorkDirectory() {
    return QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/work"));
}

} // namespace

Settings::Settings(QObject* parent)
    : QObject(parent), m_store(QSettings::IniFormat,
                               QSettings::UserScope,
                               QStringLiteral("Sigmastone"),
                               QStringLiteral("SignatureMaster")) {
    Q_ASSERT(g_instance == nullptr);
    g_instance = this;

    m_autoSign = m_store.value(QLatin1String(kAutoSign), false).toBool();
    m_rememberPin = m_store.value(QLatin1String(kRememberPin), false).toBool();
    m_certificateThumbprint = m_store.value(QLatin1String(kCertificateThumbprint)).toString();
    m_timestampUrl =
        m_store.value(QLatin1String(kTimestampUrl), QStringLiteral("http://time.certum.pl"))
            .toString();
    m_signtoolPath = m_store.value(QLatin1String(kSigntoolPath)).toString();
    if (m_signtoolPath.isEmpty()) {
        m_signtoolPath = locateSigntool();
    }
    m_bindAddress =
        m_store.value(QLatin1String(kBindAddress), QStringLiteral("0.0.0.0")).toString();
    m_port = m_store.value(QLatin1String(kPort), 7443).toInt();
    m_pauseListening = m_store.value(QLatin1String(kPauseListening), false).toBool();
    m_workDirectory =
        m_store.value(QLatin1String(kWorkDirectory), defaultWorkDirectory()).toString();
    m_allowedOwner =
        m_store.value(QLatin1String(kAllowedOwner), QStringLiteral("sigmastone-io")).toString();
    m_allowedRepositories = m_store
                                .value(QLatin1String(kAllowedRepositories),
                                       QStringList{QStringLiteral("neurotrader"),
                                                   QStringLiteral("neurotrader-gui"),
                                                   QStringLiteral("neurotrader-installer")})
                                .toStringList();
    m_allowedRefs =
        m_store.value(QLatin1String(kAllowedRefs), QStringLiteral("refs/tags/v*")).toString();
}

Settings::~Settings() {
    g_instance = nullptr;
}

Settings* Settings::instance() {
    Q_ASSERT(g_instance != nullptr);
    return g_instance;
}

Settings* Settings::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine);
    Settings* settings = instance();
    // The application owns the instance; QML must never delete it.
    QJSEngine::setObjectOwnership(settings, QJSEngine::CppOwnership);
    Q_UNUSED(jsEngine);
    return settings;
}

QString Settings::filePath() const {
    return QDir::toNativeSeparators(m_store.fileName());
}

template <typename T>
bool Settings::update(const char* key, T& field, const T& value) {
    if (field == value) {
        return false;
    }
    field = value;
    m_store.setValue(QLatin1String(key), QVariant::fromValue(value));
    m_store.sync();
    return true;
}

void Settings::setAutoSign(bool on) {
    if (update(kAutoSign, m_autoSign, on)) {
        Q_EMIT autoSignChanged();
    }
}

void Settings::setRememberPin(bool on) {
    if (update(kRememberPin, m_rememberPin, on)) {
        Q_EMIT rememberPinChanged();
    }
}

bool Settings::startWithWindows() const {
#ifdef Q_OS_WIN
    QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    return run.contains(QLatin1String(kRunValue));
#else
    return false;
#endif
}

void Settings::setStartWithWindows(bool on) {
#ifdef Q_OS_WIN
    if (on == startWithWindows()) {
        return;
    }
    QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
    if (on) {
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        run.setValue(QLatin1String(kRunValue), QStringLiteral("\"%1\" --hidden").arg(exe));
    } else {
        run.remove(QLatin1String(kRunValue));
    }
    run.sync();
    Q_EMIT startWithWindowsChanged();
#else
    Q_UNUSED(on);
#endif
}

void Settings::setCertificateThumbprint(const QString& thumbprint) {
    QString normalized = thumbprint;
    normalized.remove(QLatin1Char(' ')).remove(QLatin1Char(':'));
    normalized = normalized.toLower();
    if (update(kCertificateThumbprint, m_certificateThumbprint, normalized)) {
        Q_EMIT certificateThumbprintChanged();
    }
}

void Settings::setTimestampUrl(const QString& url) {
    if (update(kTimestampUrl, m_timestampUrl, url.trimmed())) {
        Q_EMIT timestampUrlChanged();
    }
}

void Settings::setSigntoolPath(const QString& path) {
    if (update(kSigntoolPath, m_signtoolPath, QDir::toNativeSeparators(path.trimmed()))) {
        Q_EMIT signtoolPathChanged();
    }
}

void Settings::setBindAddress(const QString& address) {
    if (update(kBindAddress, m_bindAddress, address.trimmed())) {
        Q_EMIT bindAddressChanged();
    }
}

void Settings::setPort(int port) {
    if (port < 1 || port > 65535) {
        // Leave the stored value alone; the field rebinds to it.
        Q_EMIT portChanged();
        return;
    }
    if (update(kPort, m_port, port)) {
        Q_EMIT portChanged();
    }
}

void Settings::setPauseListening(bool paused) {
    if (update(kPauseListening, m_pauseListening, paused)) {
        Q_EMIT pauseListeningChanged();
    }
}

void Settings::setWorkDirectory(const QString& directory) {
    if (update(kWorkDirectory, m_workDirectory, QDir::toNativeSeparators(directory.trimmed()))) {
        Q_EMIT workDirectoryChanged();
    }
}

void Settings::setAllowedOwner(const QString& owner) {
    if (update(kAllowedOwner, m_allowedOwner, owner.trimmed())) {
        Q_EMIT allowedOwnerChanged();
    }
}

void Settings::setAllowedRepositories(const QStringList& repositories) {
    if (update(kAllowedRepositories, m_allowedRepositories, repositories)) {
        Q_EMIT allowedRepositoriesChanged();
    }
}

void Settings::addAllowedRepository(const QString& repository) {
    const QString name = repository.trimmed();
    if (name.isEmpty() || m_allowedRepositories.contains(name)) {
        return;
    }
    QStringList list = m_allowedRepositories;
    list.append(name);
    setAllowedRepositories(list);
}

void Settings::removeAllowedRepository(const QString& repository) {
    QStringList list = m_allowedRepositories;
    if (list.removeAll(repository) > 0) {
        setAllowedRepositories(list);
    }
}

void Settings::setAllowedRefs(const QString& pattern) {
    if (update(kAllowedRefs, m_allowedRefs, pattern.trimmed())) {
        Q_EMIT allowedRefsChanged();
    }
}

QString Settings::locateSigntool() {
    const QString kits = QStringLiteral("C:/Program Files (x86)/Windows Kits/10/bin");
    QString best;
    const QDir dir(kits);
    // Version directories sort ascending by name; the last existing x64 signtool wins.
    const QStringList versions = dir.entryList({QStringLiteral("10.*")}, QDir::Dirs, QDir::Name);
    for (const QString& version : versions) {
        const QString candidate =
            kits + QLatin1Char('/') + version + QStringLiteral("/x64/signtool.exe");
        if (QFileInfo::exists(candidate)) {
            best = candidate;
        }
    }
    return QDir::toNativeSeparators(best);
}
