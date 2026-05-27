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
#include "AIProvidersModel.h"
#include "PaletteController.h"
#include "VoiceController.h"
#include "UpdateController.h"
#include "TrayController.h"
#include "LayoutTemplatesController.h"
#include "FileTreeController.h"
#include "GitStatusProvider.h"
#include "ProcessStatsController.h"
#include "CalculatorController.h"
#include "ResourcesController.h"
#include "AutoFillController.h"
#include "GeneratorsModel.h"

#include "telemetry/Logger.h"
#include "themes/ThemeRegistry.h"

#include <QApplication>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QIcon>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QWindow>
#include <qqml.h>

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName("Dante Testa");
    QCoreApplication::setOrganizationDomain("dantetesta.com.br");
    QCoreApplication::setApplicationName("Dante CLI");
    QCoreApplication::setApplicationVersion("0.7.0-alpha.32");

    // QApplication (not QGuiApplication) is required because QSystemTrayIcon's
    // context menu uses QMenu, which is a QWidget. Linking Widgets is already
    // there for the same reason (Fusion style on QML controls).
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/icons/app.png"));
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
    auto* aiProviders = new dante::AIProvidersModel(&app);
    aiProviders->hydrate();
    // Voice → Whisper. nullptr lets it spin its own GroqClient (independent
    // QNetworkAccessManager so the chat round-trip can't stall mic uploads).
    auto* voice      = new dante::VoiceController(appState, /*groq*/ nullptr, &app);
    auto* schemes    = new dante::themes::ThemeRegistry(&app);
    auto* updater    = new dante::UpdateController(&app);
    auto* templates  = new dante::LayoutTemplatesController(&app);
    auto* fileTree   = new dante::FileTreeController(&app);
    auto* gitStatus  = new dante::GitStatusProvider(&app);
    fileTree->setGitProvider(gitStatus);
    auto* processStats = new dante::ProcessStatsController(&app);
    auto* calculator = new dante::CalculatorController(&app);
    calculator->hydrate();
    auto* resources = new dante::ResourcesController(&app);
    resources->hydrate();
    QObject::connect(resources, &dante::ResourcesController::requestTerminalWrite,
                     &app, [appState, terminal](const QString& text) {
                         if (!appState->activeTabId().isEmpty())
                             terminal->write(appState->activeTabId(), text);
                     });
    auto* autoFill   = new dante::AutoFillController(&app);
    auto* generators = new dante::GeneratorsModel(&app);
    QObject::connect(autoFill, &dante::AutoFillController::commandReady,
                     &app, [appState, terminal](const QString& text) {
                         if (!appState->activeTabId().isEmpty())
                             terminal->write(appState->activeTabId(), text);
                     });

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
    QObject::connect(palette, &dante::PaletteController::autoFillRequested,
                     autoFill, &dante::AutoFillController::prepare);
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
    engine.rootContext()->setContextProperty("aiProviders", aiProviders);
    engine.rootContext()->setContextProperty("resources",  resources);
    engine.rootContext()->setContextProperty("autoFill",   autoFill);
    engine.rootContext()->setContextProperty("generators", generators);
    engine.rootContext()->setContextProperty("voice",      voice);
    engine.rootContext()->setContextProperty("schemes",    schemes);
    engine.rootContext()->setContextProperty("palette",    palette);
    engine.rootContext()->setContextProperty("updater",    updater);
    engine.rootContext()->setContextProperty("templates",  templates);
    engine.rootContext()->setContextProperty("fileTree",   fileTree);
    engine.rootContext()->setContextProperty("gitStatus",  gitStatus);
    engine.rootContext()->setContextProperty("processStats", processStats);
    engine.rootContext()->setContextProperty("calculator", calculator);

    // Kick off a first update check after the UI is up (deferred so a slow
    // network probe doesn't delay window-show).
    QTimer::singleShot(2000, updater, &dante::UpdateController::checkNow);

    // Capture every QML parse / import error the engine reports so the
    // MessageBox can show the *real* reason instead of a generic "falhou".
    // alpha.13 only surfaced "Main.qml failed" with no context, which made
    // remote debugging impossible. Listening on the warnings signal hits
    // all imports + parsers (Component-level objectCreationFailed misses
    // import errors that happen before the type system even resolves).
    QStringList qmlErrors;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
        [&qmlErrors](const QList<QQmlError>& list) {
            for (const auto& e : list) {
                const QString s = e.toString();
                qCritical().noquote() << "[qml]" << s;
                qmlErrors.append(s);
            }
        });

    engine.loadFromModule("dante.ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load Main.qml";
        const QString logDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
        // First few errors are usually the cause; later ones are cascade
        // damage. Show the top 6 in the MessageBox; full list is in the log.
        QString first;
        if (!qmlErrors.isEmpty()) {
            first = "\n\nErros do Qt (primeiros 6):\n• "
                  + qmlErrors.mid(0, 6).join("\n• ");
        }
        QMessageBox::critical(
            nullptr,
            "Dante CLI",
            "Falha ao carregar a interface (Main.qml)." + first +
            "\n\nLog completo:\n" + logDir);
        return -1;
    }

    // System tray. Built after the root object exists so we can attach the
    // ApplicationWindow for the show/hide toggle.
    auto* tray = new dante::TrayController(appState, updater, &app);
    if (auto* win = qobject_cast<QWindow*>(engine.rootObjects().first())) {
        tray->attachWindow(win);
    }

    return app.exec();
}
