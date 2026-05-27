#include "AutoFillController.h"

#include "autofill/AutoFillEngine.h"

#include <QMap>

namespace dante {

AutoFillController::AutoFillController(QObject* parent)
    : QObject(parent) {}

void AutoFillController::prepare(const QString& templateText) {
    const auto items = autofill::AutoFillEngine::scan(templateText);
    if (items.isEmpty()) {
        // Pass-through — no UI needed.
        template_.clear();
        if (!pending_.isEmpty()) {
            pending_.clear();
            emit pendingPlaceholdersChanged();
        }
        emit commandReady(templateText);
        return;
    }
    pending_.clear();
    pending_.reserve(items.size());
    for (const auto& p : items) {
        QVariantMap m;
        m.insert(QStringLiteral("name"),         p.name);
        m.insert(QStringLiteral("label"),        p.label);
        m.insert(QStringLiteral("kind"),         p.kind);
        m.insert(QStringLiteral("defaultValue"), p.defaultValue);
        m.insert(QStringLiteral("required"),     p.required);
        m.insert(QStringLiteral("choices"),      p.choices);
        pending_.append(m);
    }
    template_ = templateText;
    emit pendingPlaceholdersChanged();
}

void AutoFillController::submit(const QVariantMap& values) {
    if (template_.isEmpty()) return;
    QMap<QString, QString> typed;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        typed.insert(it.key(), it.value().toString());
    }
    const QString out = autofill::AutoFillEngine::render(template_, typed);
    template_.clear();
    pending_.clear();
    emit pendingPlaceholdersChanged();
    emit commandReady(out);
}

void AutoFillController::cancel() {
    if (template_.isEmpty() && pending_.isEmpty()) return;
    template_.clear();
    pending_.clear();
    emit pendingPlaceholdersChanged();
}

bool AutoFillController::hasPlaceholders(const QString& templateText) const {
    return !autofill::AutoFillEngine::scan(templateText).isEmpty();
}

} // namespace dante
