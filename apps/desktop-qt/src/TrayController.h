// System tray integration. On Windows shows up in the notification area;
// on macOS shows in the menu bar (NSStatusItem). Provides:
//   • Mostrar / Esconder janela (default-action on left-click)
//   • Context menu: Mostrar, Nova aba, Sair
//   • Toast notification when UpdateController flips `available` to true
//
// The icon is generated procedurally from a Unicode glyph so we don't depend
// on a packaged .ico/.icns file (we can swap later when proper artwork lands).
#pragma once

#include <QObject>
#include <QPointer>

class QSystemTrayIcon;
class QMenu;
class QWindow;

namespace dante {

class AppState;
class UpdateController;

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(AppState* appState,
                            UpdateController* updater,
                            QObject* parent = nullptr);
    ~TrayController() override;

    /// Attach to the main window so left-click can show/hide it. Call once
    /// after engine.loadFromModule returns.
    void attachWindow(QWindow* w);

private slots:
    void onIconActivated(int reason);
    void onUpdateAvailable();

private:
    void buildIcon();
    void buildMenu();
    void toggleWindow();

    AppState*          appState_{nullptr};
    UpdateController*  updater_{nullptr};
    QSystemTrayIcon*   tray_{nullptr};
    QMenu*             menu_{nullptr};
    QPointer<QWindow>  window_;
};

} // namespace dante
