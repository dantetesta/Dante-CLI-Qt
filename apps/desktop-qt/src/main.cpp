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
#include "TerminalView.h"
#include "AIController.h"
#include "PaletteController.h"
#include "VoiceController.h"

#include "telemetry/Logger.h"
#include "themes/ThemeRegistry.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <qqml.h>

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

    // VT100 terminal widget — register before engine loads QML.
    qmlRegisterType<dante::TerminalView>("dante.ui", 1, 0, "TerminalView");

    QQmlApplicationEngine engine;

    // Wire up the app state graph.
    auto* appState   = new dante::AppState(&app);
    auto* tabs       = new dante::TabsModel(appState, &app);
    auto* favorites  = new dante::FavoritesModel(&app);
    auto* snippets   = new dante::SnippetsModel(&app);
    auto* credentials= new dante::CredentialsModel(&app);
    auto* terminal   = new dante::TerminalBridge(&app);
    auto* ai         = new dante::AIController(appState, &app);
    // Voice → Whisper. nullptr lets it spin its own GroqClient (independent
    // QNetworkAccessManager so the chat round-trip can't stall mic uploads).
    auto* voice      = new dante::VoiceController(appState, /*groq*/ nullptr, &app);
    auto* schemes    = new dante::themes::ThemeRegistry(&app);

    appState->hydrate();
    favorites->hydrate();
    snippets->hydrate();
    credentials->hydrate();

    // Command palette — depends on hydrated models so we build it after.
    auto* palette = new dante::PaletteController(appState, favorites, snippets, schemes, &app);

    // Rebuild the palette catalog whenever favorites / snippets change so the
    // "cd para «name»" / "Snippet: …" rows stay current. AppState's terminal
    // scheme list is static for the session, so no need to listen for those.
    QObject::connect(favorites, &QAbstractItemModel::rowsInserted,
                     palette, [palette]() { palette->rebuildCatalog(); });
    QObject::connect(favorites, &QAbstractItemModel::rowsRemoved,
                     palette, [palette]() { palette->rebuildCatalog(); });
    QObject::connect(favorites, &QAbstractItemModel::dataChanged,
                     palette, [palette]() { palette->rebuildCatalog(); });
    QObject::connect(snippets, &QAbstractItemModel::rowsInserted,
                     palette, [palette]() { palette->rebuildCatalog(); });
    QObject::connect(snippets, &QAbstractItemModel::rowsRemoved,
                     palette, [palette]() { palette->rebuildCatalog(); });
    QObject::connect(snippets, &QAbstractItemModel::dataChanged,
                     palette, [palette]() { palette->rebuildCatalog(); });

    // Palette side-effect signals → real controllers.
    // (terminalWriteRequested / clearRequested are also handled in QML so the
    //  active TerminalView gets the bytes through the same path AI uses.)
    QObject::connect(palette, &dante::PaletteController::terminalWriteRequested,
                     terminal, &dante::TerminalBridge::write);
    QObject::connect(palette, &dante::PaletteController::aiToggleRequested,
                     ai, &dante::AIController::toggle);
    // voiceStartRequested + focusTerminalRequested are QML-only side effects.
    QObject::connect(palette, &dante::PaletteController::voiceStartRequested,
                     voice, &dante::VoiceController::startRecording);

    // Voice transcription → drop the text into the active terminal session.
    QObject::connect(voice, &dante::VoiceController::transcribed,
                     [appState, terminal](const QString& text) {
                         if (!text.isEmpty()) terminal->write(appState->activeTabId(), text);
                     });

    // Make them available to QML by `appState`, `tabs`, `favorites`, etc.
    engine.rootContext()->setContextProperty("appState",   appState);
    engine.rootContext()->setContextProperty("tabs",       tabs);
    engine.rootContext()->setContextProperty("favorites",  favorites);
    engine.rootContext()->setContextProperty("snippets",   snippets);
    engine.rootContext()->setContextProperty("credentials",credentials);
    engine.rootContext()->setContextProperty("terminal",   terminal);
    engine.rootContext()->setContextProperty("ai",         ai);
    engine.rootContext()->setContextProperty("voice",      voice);
    engine.rootContext()->setContextProperty("schemes",    schemes);
    engine.rootContext()->setContextProperty("palette",    palette);

    engine.loadFromModule("dante.ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load Main.qml";
        return -1;
    }
    return app.exec();
}
