// Singleton registry of terminal color schemes. Exposed to QML as the
// context property `schemes` (see main.cpp). Palettes are lifted verbatim
// from the Swift sibling's Utilities/TerminalThemes.swift — same hex
// values, same ordering — so a `terminalScheme` id is portable.
//
// QML usage:
//   import "."
//   property var scheme: schemes.find(appState.terminalScheme)
//   color: scheme ? scheme.bg : Theme.ink
//
// C++ usage:
//   const auto* s = ThemeRegistry::find("TokyoNight");
#pragma once

#include "TerminalScheme.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace dante::themes {

class ThemeRegistry : public QObject {
    Q_OBJECT
public:
    explicit ThemeRegistry(QObject* parent = nullptr) : QObject(parent) {}

    /// Returns all known schemes in display order.
    static const QVector<TerminalScheme>& all();

    /// Linear lookup by id (case-sensitive). Returns nullptr if not found.
    static const TerminalScheme* find(const QString& id);

    /// Default scheme id (always present in `all()`).
    static QString defaultId() { return QStringLiteral("TokyoNight"); }

    /* ─── QML-friendly invokables ─────────────────────────────────────── */

    /// QVariantList of {id,name,bg,fg,cursor,accent1,accent2,accent3,accent4}
    /// — perfect to feed a Repeater inside SchemePicker.qml.
    Q_INVOKABLE QVariantList list() const;

    /// Returns a QVariantMap for one scheme (empty map if unknown).
    /// Fields: id, name, bg, fg, cursor, selectionBg, ansi (QVariantList of QColor).
    /// Named distinctly from the static C++ `find()` so the two don't
    /// collide on return type when Q_INVOKABLE introspects the class.
    Q_INVOKABLE QVariantMap findById(const QString& id) const;

    /// Convenience: just the four accents (ansi 1..4 bright tier when
    /// available) for the mini preview stripe in SchemePicker.
    Q_INVOKABLE QVariantList preview(const QString& id) const;
};

} // namespace dante::themes
