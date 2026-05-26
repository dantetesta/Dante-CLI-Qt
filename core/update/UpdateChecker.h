// HTTP client that polls the update manifest at dantetesta.com.br and reports
// whether a newer build is available. Pure C++ — no QML / UI knowledge here.
//
// Manifest format (https://dantetesta.com.br/dante-cli/winupdate.json):
//   {
//     "version":      "0.7.0-alpha.8",
//     "url":          "https://.../Dante-CLI-Setup-...-x64.exe",
//     "notes":        "Free-form changelog snippet",
//     "publishedAt":  "2026-05-26T10:00:00Z",
//     "mandatory":    false
//   }
//
// Version comparison is semver-lite: numeric components compared left to right,
// and `-alpha.N` / `-beta.N` / `-rc.N` are ordered alpha < beta < rc < release.
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>

class QNetworkAccessManager;

namespace dante::update {

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);

    void setManifestUrl(QUrl u) { manifestUrl_ = std::move(u); }
    void setCurrentVersion(QString v) { currentVersion_ = std::move(v); }

    /// Trigger a one-shot HTTP fetch. Emits `updateAvailable` or
    /// `noUpdate` when the response lands, `checkFailed` on error.
    Q_INVOKABLE void check();

signals:
    /// Newer version exists. `info` keys: version, url, notes, publishedAt,
    /// mandatory, sizeBytes (0 if unknown).
    void updateAvailable(QVariantMap info);

    /// Manifest fetched OK but we're already on the latest.
    void noUpdate(QString currentVersion);

    /// Network error, parse error, or no manifest URL configured.
    void checkFailed(QString reason);

private:
    /// Returns -1 if a < b, 0 if equal, 1 if a > b. Handles pre-release tags.
    static int compareVersions(const QString& a, const QString& b);

    QNetworkAccessManager* nam_;
    QUrl    manifestUrl_;
    QString currentVersion_;
};

} // namespace dante::update
