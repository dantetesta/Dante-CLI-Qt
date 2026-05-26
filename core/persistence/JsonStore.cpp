#include "JsonStore.h"
#include "AppPaths.h"
#include <QFile>
#include <QSaveFile>
#include <QDebug>

namespace dante::persistence {

JsonStore::JsonStore(QString filename, int debounceMs, QObject* parent)
    : QObject(parent)
    , filename_(std::move(filename))
{
    timer_.setSingleShot(true);
    timer_.setInterval(debounceMs);
    connect(&timer_, &QTimer::timeout, this, &JsonStore::doWrite);
}

QJsonDocument JsonStore::read(const QJsonDocument& fallback) const {
    QFile f(file(filename_));
    if (!f.exists()) return fallback;
    if (!f.open(QIODevice::ReadOnly)) return fallback;
    const QByteArray bytes = f.readAll();
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JsonStore::read parse error" << filename_ << err.errorString();
        return fallback;
    }
    return doc;
}

void JsonStore::scheduleWrite(QJsonDocument doc) {
    pending_ = std::move(doc);
    hasPending_ = true;
    if (timer_.interval() == 0) {
        doWrite();
    } else {
        timer_.start();
    }
}

void JsonStore::flush() {
    if (hasPending_) {
        timer_.stop();
        doWrite();
    }
}

void JsonStore::doWrite() {
    if (!hasPending_) return;
    hasPending_ = false;

    QSaveFile f(file(filename_));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit writeFailed(QStringLiteral("open ") + filename_);
        return;
    }
    const QByteArray bytes = pending_.toJson(QJsonDocument::Indented);
    f.write(bytes);
    if (!f.commit()) {
        emit writeFailed(QStringLiteral("commit ") + filename_);
    }
}

} // namespace dante::persistence
