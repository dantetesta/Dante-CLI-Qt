// Entry-point — bootstraps Qt, registers QML types, loads Main.qml.
//
// Threading model (per skill spec, adapted for a terminal app):
//   • UI thread          → QGuiApplication + QML scene graph
//   • Shell I/O          → QProcess on a worker (handled by ShellAdapter)
//   • Persistence        → QTimer-debounced writes off the UI thread
//   • Network (Groq)     → QNetworkAccessManager (async, signal-driven)

#include "AppState.h"
#include "TabsModel.h"
#include "FavoritesModel.h"
#include "SnippetsModel.h"
#include "CredentialsModel.h"
#include "TerminalBridge.h"

#include "telemetry/Logger.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName("Dante Testa");
    QCoreApplication::setOrganizationDomain("dantetesta.com.br");
    QCoreApplication::setApplicationName("Dante CLI");
    QCoreApplication::setApplicationVersion("0.7.0");

    QGuiApplication app(argc, argv);
    QGuiApplication::setWindowIcon(QIcon(":/icons/app.png"));
    QQuickStyle::setStyle("Fusion");

    dante::telemetry::install();
    qInfo() << "Dante CLI starting" << QCoreApplication::applicationVersion();

    QQmlApplicationEngine engine;

    // Wire up the app state graph.
    auto* appState   = new dante::AppState(&app);
    auto* tabs       = new dante::TabsModel(appState, &app);
    auto* favorites  = new dante::FavoritesModel(&app);
    auto* snippets   = new dante::SnippetsModel(&app);
    auto* credentials= new dante::CredentialsModel(&app);
    auto* terminal   = new dante::TerminalBridge(&app);

    appState->hydrate();
    favorites->hydrate();
    snippets->hydrate();
    credentials->hydrate();

    // Make them available to QML by `appState`, `tabs`, `favorites`, etc.
    engine.rootContext()->setContextProperty("appState",   appState);
    engine.rootContext()->setContextProperty("tabs",       tabs);
    engine.rootContext()->setContextProperty("favorites",  favorites);
    engine.rootContext()->setContextProperty("snippets",   snippets);
    engine.rootContext()->setContextProperty("credentials",credentials);
    engine.rootContext()->setContextProperty("terminal",   terminal);

    engine.loadFromModule("dante.ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load Main.qml";
        return -1;
    }
    return app.exec();
}
