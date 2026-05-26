// Value type describing a terminal color scheme. Mirrors the Swift
// sibling's `TerminalTheme` struct (Utilities/TerminalThemes.swift) so that
// the same palette names round-trip between mac and Windows builds.
//
// The 16-entry ANSI vector follows the conventional ordering:
//   [0..7]   black, red, green, yellow, blue, magenta, cyan, white
//   [8..15]  bright variants of the above
//
// This struct is Q_DECLARE_METATYPE'd so it can cross the QML boundary as
// a property value (and so QVariant<TerminalScheme> works in QSettings).
#pragma once

#include <QColor>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <array>

namespace dante::themes {

struct TerminalScheme {
    QString id;
    QString name;

    QColor bg;
    QColor fg;
    QColor cursor;
    QColor selectionBg;

    // 16 ANSI entries (see header comment for layout).
    std::array<QColor, 16> ansi{};

    Q_INVOKABLE QColor ansiAt(int i) const {
        if (i < 0 || i >= 16) return QColor();
        return ansi[size_t(i)];
    }
};

} // namespace dante::themes

Q_DECLARE_METATYPE(dante::themes::TerminalScheme)
