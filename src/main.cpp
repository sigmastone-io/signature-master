#include "app/Authenticator.h"
#include "app/DeskStatus.h"
#include "app/JwksCache.h"
#include "app/LogModel.h"
#include "app/RequestModel.h"
#include "app/RequestStore.h"
#include "app/Settings.h"
#include "app/SignatureLog.h"
#include "app/Signer.h"
#include "app/SigningService.h"
#include "app/TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleHints>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Sigmastone"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("sigmastone.io"));
    QCoreApplication::setApplicationName(QStringLiteral("SignatureMaster"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SM_VERSION));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Signature Master"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/SignatureMaster/icons/app.svg")));
    // The window hides to the tray on close; only the tray menu quits.
    QGuiApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Signing desk for Sigmastone releases"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption hiddenOption(
        QStringLiteral("hidden"),
        QStringLiteral(
            "Start in the tray without showing the window (used by Start with Windows)."));
    parser.addOption(hiddenOption);
    const QCommandLineOption schemeOption(
        QStringLiteral("color-scheme"),
        QStringLiteral("Force \"light\" or \"dark\" instead of following Windows (for checking "
                       "the theme)."),
        QStringLiteral("scheme"));
    parser.addOption(schemeOption);
    parser.process(app);

    if (parser.isSet(schemeOption)) {
        const QString scheme = parser.value(schemeOption).toLower();
        if (scheme == QStringLiteral("light")) {
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
        } else if (scheme == QStringLiteral("dark")) {
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        }
    }

    QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

    Settings settings;
    SignatureLog log(SignatureLog::defaultPath());
    RequestStore store(&settings);
    store.setLog(&log);
    // GitHub's signing keys; SM_OIDC_JWKS_URL points a development desk at a local JWKS.
    const QString jwksOverride = qEnvironmentVariable("SM_OIDC_JWKS_URL");
    JwksCache keys(jwksOverride.isEmpty() ? JwksCache::gitHubActionsJwks() : QUrl(jwksOverride));
    keys.refreshInBackground();
    GitHubOidcAuthenticator authenticator(&settings, &keys);
    SigntoolSigner signer(&settings);
    SigningService signing(&settings, &store, &signer);
    DeskStatus desk(&settings, &store, &authenticator);
    RequestModel requests(&store, &signing);
    LogModel logEntries(&log);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    engine.loadFromModule("SignatureMaster", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (window == nullptr) {
        return 1;
    }

    TrayController tray(&settings, &desk, &store);
    QObject::connect(&tray, &TrayController::openRequested, window, [window] {
        window->show();
        window->raise();
        window->requestActivate();
    });
    QObject::connect(&tray, &TrayController::quitRequested, &app, &QCoreApplication::quit);

    if (!parser.isSet(hiddenOption)) {
        window->show();
    }
    return QCoreApplication::exec();
}
