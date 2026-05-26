#include "UpdateChecker.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringList>
#include <QUrlQuery>

namespace dante::update {

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

void UpdateChecker::check() {
    if (!manifestUrl_.isValid()) {
        emit checkFailed("manifest URL not configured");
        return;
    }
    QNetworkRequest req(manifestUrl_);
    req.setRawHeader("Accept", "application/json");
    // Cache-bust: HEAD-style query string forces CDN re-fetch.
    QUrl u = req.url();
    QUrlQuery q(u);
    q.addQueryItem("ts", QString::number(QDateTime::currentMSecsSinceEpoch()));
    u.setQuery(q);
    req.setUrl(u);
    req.setTransferTimeout(8000);

    auto* reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(QString("HTTP %1: %2")
                .arg(int(reply->error())).arg(reply->errorString()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto o = doc.object();
        const QString remoteVer = o.value("version").toString();
        if (remoteVer.isEmpty()) {
            emit checkFailed("manifest missing 'version' field");
            return;
        }
        if (compareVersions(currentVersion_, remoteVer) >= 0) {
            emit noUpdate(currentVersion_);
            return;
        }
        QVariantMap info;
        info.insert("version",     remoteVer);
        info.insert("url",         o.value("url").toString());
        info.insert("notes",       o.value("notes").toString());
        info.insert("publishedAt", o.value("publishedAt").toString());
        info.insert("mandatory",   o.value("mandatory").toBool(false));
        info.insert("sizeBytes",   o.value("sizeBytes").toInteger(0));
        emit updateAvailable(info);
    });
}

namespace {
    // Splits "0.7.0-alpha.8" into:
    //   numeric:  [0, 7, 0]
    //   suffix:   "alpha"        ("" if no pre-release)
    //   preNum:   8              (0 if no number)
    // Suffix ordering: "alpha" < "beta" < "rc" < ""  (release).
    int suffixRank(const QString& s) {
        if (s.isEmpty()) return 100;            // release wins
        if (s == "rc")    return 30;
        if (s == "beta")  return 20;
        if (s == "alpha") return 10;
        return 1;                                // unknown pre-release
    }

    struct Parsed { QVector<int> numeric; QString suffix; int preNum{0}; };

    Parsed parse(const QString& v) {
        Parsed p;
        QString s = v.trimmed();
        if (s.startsWith('v') || s.startsWith('V')) s.remove(0, 1);

        QString numPart = s;
        QString prePart;
        const int dash = s.indexOf('-');
        if (dash >= 0) { numPart = s.left(dash); prePart = s.mid(dash + 1); }

        const auto numTokens = numPart.split('.', Qt::SkipEmptyParts);
        for (const auto& t : numTokens) p.numeric.append(t.toInt());

        if (!prePart.isEmpty()) {
            const auto preTokens = prePart.split('.', Qt::SkipEmptyParts);
            p.suffix = preTokens.value(0);
            if (preTokens.size() > 1) p.preNum = preTokens.value(1).toInt();
        }
        return p;
    }
}

int UpdateChecker::compareVersions(const QString& a, const QString& b) {
    const auto pa = parse(a);
    const auto pb = parse(b);

    const int n = std::max(pa.numeric.size(), pb.numeric.size());
    for (int i = 0; i < n; ++i) {
        const int av = i < pa.numeric.size() ? pa.numeric[i] : 0;
        const int bv = i < pb.numeric.size() ? pb.numeric[i] : 0;
        if (av != bv) return av < bv ? -1 : 1;
    }
    const int sa = suffixRank(pa.suffix);
    const int sb = suffixRank(pb.suffix);
    if (sa != sb) return sa < sb ? -1 : 1;
    if (pa.preNum != pb.preNum) return pa.preNum < pb.preNum ? -1 : 1;
    return 0;
}

} // namespace dante::update
