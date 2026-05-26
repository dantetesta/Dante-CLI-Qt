#include "TrayController.h"
#include "AppState.h"
#include "UpdateController.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QWindow>

namespace dante {

TrayController::TrayController(AppState* appState,
                               UpdateController* updater,
                               QObject* parent)
    : QObject(parent)
    , appState_(appState)
    , updater_(updater)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qInfo() << "[tray] system tray unavailable — TrayController disabled";
        return;
    }

    tray_ = new QSystemTrayIcon(this);
    buildIcon();
    buildMenu();

    tray_->setToolTip("Dante CLI");
    tray_->setContextMenu(menu_);
    connect(tray_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason r) { onIconActivated(int(r)); });

    if (updater_) {
        connect(updater_, &UpdateController::availabilityChanged,
                this, &TrayController::onUpdateAvailable);
    }
    tray_->show();
}

TrayController::~TrayController() {
    if (tray_) tray_->hide();
}

void TrayController::attachWindow(QWindow* w) { window_ = w; }

void TrayController::buildIcon() {
    // Render a 32×32 accent-colored rounded square with a white "D" inside.
    // Crisp on retina because we paint at 2× scale.
    const int   sz  = 64;
    QPixmap     pm(sz, sz);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.setBrush(QColor("#7C82FF"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(2, 2, sz - 4, sz - 4), 12, 12);
        QFont f("Helvetica");
        f.setPixelSize(40);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, sz, sz), Qt::AlignCenter, "D");
    }
    pm.setDevicePixelRatio(2.0);
    tray_->setIcon(QIcon(pm));
}

void TrayController::buildMenu() {
    menu_ = new QMenu();

    auto* showAct = menu_->addAction(tr("Mostrar Dante CLI"));
    connect(showAct, &QAction::triggered, this, &TrayController::toggleWindow);

    auto* newTabAct = menu_->addAction(tr("Nova aba"));
    connect(newTabAct, &QAction::triggered, this, [this]() {
        if (appState_) appState_->addTab(QStringLiteral("Terminal"));
        toggleWindow();  // bring the window forward
    });

    menu_->addSeparator();

    if (updater_) {
        auto* updateAct = menu_->addAction(tr("Verificar atualizações"));
        connect(updateAct, &QAction::triggered,
                updater_, &UpdateController::checkNow);
    }

    menu_->addSeparator();

    auto* quitAct = menu_->addAction(tr("Sair"));
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void TrayController::onIconActivated(int reason) {
    // Trigger == left-click on Windows; on macOS only the context menu fires,
    // so this is largely a Windows convenience.
    if (reason == QSystemTrayIcon::Trigger ||
        reason == QSystemTrayIcon::DoubleClick) {
        toggleWindow();
    }
}

void TrayController::onUpdateAvailable() {
    if (!updater_ || !updater_->available()) return;
    const auto info = updater_->info();
    const QString version = info.value("version").toString();
    const QString notes   = info.value("notes").toString();
    tray_->showMessage(
        tr("Dante CLI %1 disponível").arg(version),
        notes.isEmpty() ? tr("Clique em Atualizar na janela principal.") : notes,
        QSystemTrayIcon::Information,
        8000);
}

void TrayController::toggleWindow() {
    if (!window_) return;
    if (window_->isVisible() && window_->isActive()) {
        window_->hide();
    } else {
        window_->show();
        window_->raise();
        window_->requestActivate();
    }
}

} // namespace dante
