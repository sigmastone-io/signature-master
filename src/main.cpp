#include "app/DeskStatus.h"
#include "app/Settings.h"
#include "app/TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Sigmastone"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("sigmastone.io"));
    QCoreApplication::setApplicationName(QStringLiteral("SignatureMaster"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SM_VERSION));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Signature Master"));
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
    parser.process(app);

    QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

    Settings settings;
    DeskStatus desk(&settings);

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

    TrayController tray(&settings, &desk);
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
