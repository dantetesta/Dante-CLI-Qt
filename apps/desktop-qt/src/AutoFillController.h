// SPEC-080 — QML bridge for the AutoFill engine.
//
// Single instance lives for the duration of the app, exposed to QML as
// `autoFill`. Lifecycle:
//
//   1. PaletteController detects a snippet/favorite contains placeholders
//      and calls `autoFill.prepare(text)` (instead of writing to the
//      terminal directly).
//   2. QML observes `pendingPlaceholders` and shows AutoFillDialog.
//   3. The user fills the form and clicks Run → QML calls
//      `autoFill.submit(values)`.
//   4. The controller renders the template and emits `commandReady(text)`,
//      which main.cpp wires to the terminal write.
//
// Cancel resets `pendingPlaceholders` to an empty list so QML hides the
// dialog automatically.
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace dante {

class AutoFillController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList pendingPlaceholders READ pendingPlaceholders
               NOTIFY pendingPlaceholdersChanged)
    Q_PROPERTY(QString pendingTemplate READ pendingTemplate
               NOTIFY pendingPlaceholdersChanged)
public:
    explicit AutoFillController(QObject* parent = nullptr);

    QVariantList pendingPlaceholders() const { return pending_; }
    QString      pendingTemplate()     const { return template_; }

    // Parse `templateText`. When it has at least one placeholder, populate
    // `pendingPlaceholders` (QML side reacts). When there are none, the
    // controller emits `commandReady(templateText)` immediately — caller
    // can route directly through this without short-circuiting in palette
    // logic.
    Q_INVOKABLE void prepare(const QString& templateText);

    // Render template + emit commandReady. Values is name → string.
    Q_INVOKABLE void submit(const QVariantMap& values);

    // Clear pending state without emitting commandReady.
    Q_INVOKABLE void cancel();

    // Cheap pre-flight check exposed for the palette: returns true when the
    // text contains at least one placeholder. Lets callers decide whether
    // to bypass the dialog.
    Q_INVOKABLE bool hasPlaceholders(const QString& templateText) const;

signals:
    void pendingPlaceholdersChanged();
    void commandReady(const QString& text);

private:
    QVariantList pending_;
    QString      template_;
};

} // namespace dante
