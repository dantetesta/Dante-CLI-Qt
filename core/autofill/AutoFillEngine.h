// SPEC-080 — AutoFill engine.
//
// Parses a command-line template that may contain placeholders, returns the
// list of detected placeholders, and renders the final string once values are
// provided. Grammar (matches what snippets/favorites/generators may embed):
//
//   $VAR                   required variable, name follows POSIX identifier
//                          rules ([A-Za-z_][A-Za-z0-9_]*). Looked up in the
//                          supplied values map.
//   ${VAR:default}         variable with a default. If the value map is
//                          missing the key (or maps it to an empty string),
//                          the default is substituted.
//   $(label)               interactive prompt. The literal label is shown
//                          to the user; the placeholder name is derived
//                          from a slugified version of it so the same label
//                          twice in the same template collapses to ONE
//                          prompt (DRY — answer once, reuse).
//   $(label|kind)          typed prompt. Valid kinds:
//                            text        — single-line text (default).
//                            password    — masked input.
//                            path        — file picker.
//                            dir         — directory picker.
//                            choice:a,b  — dropdown with comma-separated
//                                          options.
//                            number      — numeric spinbox.
//
// The parser is pure C++/QtCore — no QObject, no QML deps — so it can be
// unit-tested in isolation and reused by any controller.
#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace dante::autofill {

struct Placeholder {
    QString     rawToken;     // exact substring as it appears in the template
    QString     name;         // resolved key used to look up values
    QString     label;        // human-readable prompt
    QString     defaultValue; // for ${VAR:default}; empty otherwise
    QString     kind;         // text|password|path|dir|choice|number
    QStringList choices;      // populated when kind == "choice"
    bool        required {true};
};

class AutoFillEngine {
public:
    // Scan a template, returning the deduped list of placeholders in
    // appearance order. Duplicates (same resolved name) are collapsed to a
    // single entry — the first occurrence wins for label/kind/default.
    static QVector<Placeholder> scan(const QString& templateText);

    // Substitute every placeholder with values[name]. Unknown/empty values
    // fall back to defaultValue when present, or to an empty string. The
    // raw token (literal characters) is replaced — no recursive expansion.
    static QString render(const QString& templateText,
                          const QMap<QString, QString>& values);

    // Returns true when every REQUIRED placeholder has a non-empty value
    // (or a default). When false, *missingName is set to the first offender.
    static bool isComplete(const QString& templateText,
                           const QMap<QString, QString>& values,
                           QString* missingName = nullptr);
};

} // namespace dante::autofill
