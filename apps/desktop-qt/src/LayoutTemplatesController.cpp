#include "LayoutTemplatesController.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace dante {

namespace {
    constexpr auto kFile = "layout-templates.json";

    QJsonObject spansToJson(const QVariantMap& spans) {
        // QVariant::toInt takes a `bool*`, not a default value, so we coerce
        // missing or zero entries manually.
        QJsonObject out;
        for (auto it = spans.constBegin(); it != spans.constEnd(); ++it) {
            const auto s = it.value().toMap();
            int sc = s.value("cols").toInt(); if (sc <= 0) sc = 1;
            int sr = s.value("rows").toInt(); if (sr <= 0) sr = 1;
            out.insert(it.key(), QJsonObject{ {"cols", sc}, {"rows", sr} });
        }
        return out;
    }

    QVariantMap spansFromJson(const QJsonObject& obj) {
        QVariantMap out;
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            const auto o = it.value().toObject();
            out.insert(it.key(), QVariantMap{
                {"cols", o.value("cols").toInt(1)},
                {"rows", o.value("rows").toInt(1)},
            });
        }
        return out;
    }
}

LayoutTemplatesController::LayoutTemplatesController(QObject* parent)
    : QObject(parent)
    , store_(std::make_unique<persistence::JsonStore>(kFile, 400, this))
{
    hydrate();
}

void LayoutTemplatesController::hydrate() {
    list_.clear();
    const auto arr = store_->read({}).object().value("templates").toArray();
    for (const auto& v : arr) {
        const auto o = v.toObject();
        QVariantMap m;
        m.insert("id",        o.value("id").toString());
        m.insert("name",      o.value("name").toString());
        m.insert("emoji",     o.value("emoji").toString());
        m.insert("cols",      o.value("cols").toInt(2));
        m.insert("rows",      o.value("rows").toInt(1));
        m.insert("spans",     spansFromJson(o.value("spans").toObject()));
        m.insert("createdAt", o.value("createdAt").toString());
        list_.append(m);
    }
    emit listChanged();
}

void LayoutTemplatesController::persist() {
    QJsonArray arr;
    for (const auto& v : list_) {
        const auto m = v.toMap();
        arr.append(QJsonObject{
            {"id",        m.value("id").toString()},
            {"name",      m.value("name").toString()},
            {"emoji",     m.value("emoji").toString()},
            {"cols",      m.value("cols").toInt()},
            {"rows",      m.value("rows").toInt()},
            {"spans",     spansToJson(m.value("spans").toMap())},
            {"createdAt", m.value("createdAt").toString()},
        });
    }
    store_->scheduleWrite(QJsonDocument(QJsonObject{{"templates", arr}}));
}

QString LayoutTemplatesController::save(const QString& id,
                                        const QString& name,
                                        const QString& emoji,
                                        int cols, int rows,
                                        const QVariantMap& spans) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return {};

    QVariantMap m;
    m.insert("id",        id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id);
    m.insert("name",      trimmed);
    m.insert("emoji",     emoji);
    m.insert("cols",      qBound(1, cols, 6));
    m.insert("rows",      qBound(1, rows, 6));
    m.insert("spans",     spans);
    m.insert("createdAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    // Update-in-place if id matches an existing row; otherwise prepend so
    // the most-recent templates show first.
    bool replaced = false;
    for (int i = 0; i < list_.size(); ++i) {
        const auto existing = list_[i].toMap();
        if (existing.value("id").toString() == m.value("id").toString()) {
            // Preserve original createdAt on update.
            m.insert("createdAt", existing.value("createdAt").toString());
            list_[i] = m;
            replaced = true;
            break;
        }
    }
    if (!replaced) list_.prepend(m);

    persist();
    emit listChanged();
    return m.value("id").toString();
}

void LayoutTemplatesController::remove(const QString& id) {
    for (int i = 0; i < list_.size(); ++i) {
        if (list_[i].toMap().value("id").toString() == id) {
            list_.removeAt(i);
            persist();
            emit listChanged();
            return;
        }
    }
}

QVariantMap LayoutTemplatesController::get(const QString& id) const {
    for (const auto& v : list_) {
        const auto m = v.toMap();
        if (m.value("id").toString() == id) return m;
    }
    return {};
}

int LayoutTemplatesController::visiblePanelCount(int cols, int rows,
                                                 const QVariantMap& spans) const {
    if (cols < 1 || rows < 1) return 0;
    int covered = 0;
    for (int i = 0; i < cols * rows; ++i) {
        const QString key = QString::number(i);
        if (!spans.contains(key)) continue;
        const auto s = spans.value(key).toMap();
        int sc = s.value("cols").toInt(); if (sc <= 0) sc = 1;
        int sr = s.value("rows").toInt(); if (sr <= 0) sr = 1;
        if (sc == 1 && sr == 1) continue;
        const int r = i / cols;
        const int c = i % cols;
        for (int rr = r; rr < qMin(rows, r + sr); ++rr) {
            for (int cc = c; cc < qMin(cols, c + sc); ++cc) {
                const int idx = rr * cols + cc;
                if (idx != i) ++covered;
            }
        }
    }
    return cols * rows - covered;
}

} // namespace dante
