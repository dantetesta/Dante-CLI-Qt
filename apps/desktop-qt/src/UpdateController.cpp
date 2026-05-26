#include "UpdateController.h"
#include "update/UpdateChecker.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace dante {

namespace {
    constexpr auto kManifestUrl = "https://dantetesta.com.br/dante-cli/winupdate.json";
    constexpr int  kPollIntervalMs = 4 * 60 * 60 * 1000;  // 4 hours
}

UpdateController::UpdateController(QObject* parent)
    : QObject(parent)
    , checker_(new update::UpdateChecker(this))
    , nam_(new QNetworkAccessManager(this))
    , pollTimer_(new QTimer(this))
{
    checker_->setManifestUrl(QUrl(kManifestUrl));
    checker_->setCurrentVersion(QCoreApplication::applicationVersion());

    connect(checker_, &update::UpdateChecker::updateAvailable,
            this, &UpdateController::onUpdateAvailable);
    connect(checker_, &update::UpdateChecker::noUpdate,
            this, &UpdateController::onNoUpdate);
    connect(checker_, &update::UpdateChecker::checkFailed,
            this, &UpdateController::onCheckFailed);

    pollTimer_->setInterval(kPollIntervalMs);
    pollTimer_->setSingleShot(false);
    connect(pollTimer_, &QTimer::timeout, this, &UpdateController::checkNow);
    pollTimer_->start();
}

void UpdateController::checkNow() {
    if (installing_) return;  // don't re-check mid-install
    checker_->check();
}

void UpdateController::dismiss() {
    if (!available_) return;
    available_ = false;
    info_.clear();
    emit availabilityChanged();
}

void UpdateController::downloadAndInstall() {
    if (installing_) return;
    const QString url = info_.value("url").toString();
    if (url.isEmpty()) {
        setLastError("manifest missing 'url'");
        return;
    }
#if !defined(Q_OS_WIN)
    // Mac / Linux: no .exe to silent-install — just open the release page.
    QDesktopServices::openUrl(QUrl(url));
    return;
#else
    setInstalling(true);
    setProgress(0);
    setLastError({});

    const QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString filename = QString("Dante-CLI-Setup-%1.exe")
        .arg(info_.value("version").toString());
    const QString localPath = QDir(tmp).filePath(filename);

    auto* reply = nam_->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        if (total > 0) setProgress(int((received * 100) / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, localPath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setLastError(QString("Download failed: %1").arg(reply->errorString()));
            setInstalling(false);
            return;
        }
        QFile out(localPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setLastError("Cannot write installer to temp");
            setInstalling(false);
            return;
        }
        out.write(reply->readAll());
        out.close();

        // Spawn Inno Setup silently. CLOSEAPPLICATIONS asks Inno to terminate
        // our own .exe (we're updating ourselves), and RESTARTAPPLICATIONS
        // launches it again after install completes.
        const QStringList args = {
            "/SILENT", "/SUPPRESSMSGBOXES", "/NORESTART",
            "/CLOSEAPPLICATIONS", "/RESTARTAPPLICATIONS",
        };
        const bool ok = QProcess::startDetached(localPath, args);
        if (!ok) {
            setLastError("Failed to launch installer (" + localPath + ")");
            setInstalling(false);
            return;
        }
        // Give Inno a beat to seize file handles, then exit ourselves so the
        // installer can replace our .exe in place.
        QTimer::singleShot(500, qApp, &QCoreApplication::quit);
    });
#endif
}

void UpdateController::onUpdateAvailable(const QVariantMap& info) {
    info_ = info;
    available_ = true;
    setLastError({});
    emit availabilityChanged();
}

void UpdateController::onNoUpdate(const QString& /*currentVersion*/) {
    if (!available_) return;
    available_ = false;
    info_.clear();
    emit availabilityChanged();
}

void UpdateController::onCheckFailed(const QString& reason) {
    // Silent — we don't surface check failures to the user. Just log and
    // wait for the next 4 h poll.
    qWarning().noquote() << "[update] check failed:" << reason;
    setLastError(reason);
}

void UpdateController::setProgress(int p) {
    if (progress_ == p) return;
    progress_ = p;
    emit progressChanged();
}

void UpdateController::setInstalling(bool v) {
    if (installing_ == v) return;
    installing_ = v;
    emit installingChanged();
}

void UpdateController::setLastError(const QString& s) {
    if (lastError_ == s) return;
    lastError_ = s;
    emit lastErrorChanged();
}

} // namespace dante
