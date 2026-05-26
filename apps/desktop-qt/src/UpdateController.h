// QML-facing orchestrator for the autoupdate flow. Wraps UpdateChecker plus
// the actual download + silent-install kick-off. Exposed as `updater` context
// property; the banner UI reads `available`, `info`, `progress`, and
// `installing` Q_PROPERTYs.
//
// Flow:
//   1. main.cpp calls `checkNow()` on launch, then a QTimer triggers it every
//      4 hours while the app is running.
//   2. If newer manifest → `available` becomes true, `info` populated.
//   3. User clicks "Atualizar" in the banner → QML invokes `downloadAndInstall()`.
//   4. We download the .exe to %TEMP%, then on Windows run it with
//      `/SILENT /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS` and quit the app.
//      On macOS we just `QDesktopServices::openUrl` (no .exe to install).
//   5. `installing` stays true until process spawn returns control; if the
//      install fails we surface `lastError` and back off to `available=false`
//      (user can manually retry via the banner).
#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace dante::update { class UpdateChecker; }

namespace dante {

class UpdateController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool         available  READ available  NOTIFY availabilityChanged)
    Q_PROPERTY(QVariantMap  info       READ info       NOTIFY availabilityChanged)
    Q_PROPERTY(int          progress   READ progress   NOTIFY progressChanged)   // 0..100
    Q_PROPERTY(bool         installing READ installing NOTIFY installingChanged)
    Q_PROPERTY(QString      lastError  READ lastError  NOTIFY lastErrorChanged)
public:
    explicit UpdateController(QObject* parent = nullptr);

    bool        available()  const { return available_; }
    QVariantMap info()       const { return info_; }
    int         progress()   const { return progress_; }
    bool        installing() const { return installing_; }
    QString     lastError()  const { return lastError_; }

    /// Force a check now (also called every 4 h by an internal timer).
    Q_INVOKABLE void checkNow();

    /// Dismiss the banner without installing. The next check that finds a
    /// newer version will re-arm it; we don't permanently snooze.
    Q_INVOKABLE void dismiss();

    /// Begin the download + spawn-installer flow.
    Q_INVOKABLE void downloadAndInstall();

signals:
    void availabilityChanged();
    void progressChanged();
    void installingChanged();
    void lastErrorChanged();

private:
    void onUpdateAvailable(const QVariantMap& info);
    void onNoUpdate(const QString& currentVersion);
    void onCheckFailed(const QString& reason);
    void setProgress(int p);
    void setInstalling(bool v);
    void setLastError(const QString& s);

    update::UpdateChecker* checker_{nullptr};
    QNetworkAccessManager* nam_{nullptr};
    QTimer*                pollTimer_{nullptr};

    bool        available_{false};
    QVariantMap info_;
    int         progress_{0};
    bool        installing_{false};
    QString     lastError_;
};

} // namespace dante
