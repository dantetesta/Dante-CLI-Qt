#include "ThemeRegistry.h"

#include <QColor>
#include <QVariantMap>

namespace dante::themes {

namespace {

// Helper: build a scheme without all the boilerplate. ansi order is the
// canonical 0..15 (normal black/red/green/yellow/blue/magenta/cyan/white,
// then the bright tier).
TerminalScheme make(const char* id,
                    const char* name,
                    const char* bg,
                    const char* fg,
                    const char* cursor,
                    const char* selectionBg,
                    std::array<const char*, 16> hex) {
    TerminalScheme s;
    s.id = QString::fromLatin1(id);
    s.name = QString::fromLatin1(name);
    s.bg = QColor(bg);
    s.fg = QColor(fg);
    s.cursor = QColor(cursor);
    s.selectionBg = QColor(selectionBg);
    for (size_t i = 0; i < 16; ++i) s.ansi[i] = QColor(hex[i]);
    return s;
}

const QVector<TerminalScheme>& palettes() {
    // Palettes copied verbatim from Swift's TerminalThemes.swift.
    // selectionBg is derived (foreground at 28% alpha would clip text);
    // we instead use a translucent variant of cursor for a Swift-parity look.
    static const QVector<TerminalScheme> kAll = [] {
        QVector<TerminalScheme> v;

        v.push_back(make("TokyoNight", "Tokyo Night",
            "#1a1b26", "#c0caf5", "#7aa2f7", "#283457", {{
                "#15161e", "#f7768e", "#9ece6a", "#e0af68",
                "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6",
                "#414868", "#f7768e", "#9ece6a", "#e0af68",
                "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5"
            }}));

        v.push_back(make("Dracula", "Dracula",
            "#282a36", "#f8f8f2", "#bd93f9", "#44475a", {{
                "#000000", "#ff5555", "#50fa7b", "#f1fa8c",
                "#bd93f9", "#ff79c6", "#8be9fd", "#bbbbbb",
                "#555555", "#ff5555", "#50fa7b", "#f1fa8c",
                "#bd93f9", "#ff79c6", "#8be9fd", "#ffffff"
            }}));

        v.push_back(make("OneDark", "One Dark",
            "#282c34", "#abb2bf", "#528bff", "#3e4451", {{
                "#000000", "#e06c75", "#98c379", "#e5c07b",
                "#61afef", "#c678dd", "#56b6c2", "#abb2bf",
                "#5c6370", "#e06c75", "#98c379", "#e5c07b",
                "#61afef", "#c678dd", "#56b6c2", "#ffffff"
            }}));

        v.push_back(make("SolarizedDark", "Solarized Dark",
            "#002b36", "#839496", "#93a1a1", "#073642", {{
                "#073642", "#dc322f", "#859900", "#b58900",
                "#268bd2", "#d33682", "#2aa198", "#eee8d5",
                "#002b36", "#cb4b16", "#586e75", "#657b83",
                "#839496", "#6c71c4", "#93a1a1", "#fdf6e3"
            }}));

        v.push_back(make("SolarizedLight", "Solarized Light",
            "#fdf6e3", "#657b83", "#586e75", "#eee8d5", {{
                "#073642", "#dc322f", "#859900", "#b58900",
                "#268bd2", "#d33682", "#2aa198", "#eee8d5",
                "#002b36", "#cb4b16", "#586e75", "#657b83",
                "#839496", "#6c71c4", "#93a1a1", "#fdf6e3"
            }}));

        v.push_back(make("GruvboxDark", "Gruvbox Dark",
            "#282828", "#ebdbb2", "#ebdbb2", "#3c3836", {{
                "#282828", "#cc241d", "#98971a", "#d79921",
                "#458588", "#b16286", "#689d6a", "#a89984",
                "#928374", "#fb4934", "#b8bb26", "#fabd2f",
                "#83a598", "#d3869b", "#8ec07c", "#ebdbb2"
            }}));

        v.push_back(make("CatppuccinMocha", "Catppuccin Mocha",
            "#1e1e2e", "#cdd6f4", "#f5e0dc", "#313244", {{
                "#45475a", "#f38ba8", "#a6e3a1", "#f9e2af",
                "#89b4fa", "#f5c2e7", "#94e2d5", "#bac2de",
                "#585b70", "#f38ba8", "#a6e3a1", "#f9e2af",
                "#89b4fa", "#f5c2e7", "#94e2d5", "#a6adc8"
            }}));

        v.push_back(make("Monokai", "Monokai",
            "#272822", "#f8f8f2", "#f8f8f0", "#49483e", {{
                "#272822", "#f92672", "#a6e22e", "#f4bf75",
                "#66d9ef", "#ae81ff", "#a1efe4", "#f8f8f2",
                "#75715e", "#f92672", "#a6e22e", "#f4bf75",
                "#66d9ef", "#ae81ff", "#a1efe4", "#f9f8f5"
            }}));

        v.push_back(make("Nord", "Nord",
            "#2e3440", "#d8dee9", "#88c0d0", "#434c5e", {{
                "#3b4252", "#bf616a", "#a3be8c", "#ebcb8b",
                "#81a1c1", "#b48ead", "#88c0d0", "#e5e9f0",
                "#4c566a", "#bf616a", "#a3be8c", "#ebcb8b",
                "#81a1c1", "#b48ead", "#8fbcbb", "#eceff4"
            }}));

        v.push_back(make("RosePine", "Rosé Pine",
            "#191724", "#e0def4", "#524f67", "#26233a", {{
                "#26233a", "#eb6f92", "#31748f", "#f6c177",
                "#9ccfd8", "#c4a7e7", "#ebbcba", "#e0def4",
                "#6e6a86", "#eb6f92", "#31748f", "#f6c177",
                "#9ccfd8", "#c4a7e7", "#ebbcba", "#e0def4"
            }}));

        v.push_back(make("Kanagawa", "Kanagawa",
            "#1f1f28", "#dcd7ba", "#c8c093", "#2d4f67", {{
                "#16161d", "#c34043", "#76946a", "#c0a36e",
                "#7e9cd8", "#957fb8", "#6a9589", "#c8c093",
                "#727169", "#e82424", "#98bb6c", "#e6c384",
                "#7fb4ca", "#938aa9", "#7aa89f", "#dcd7ba"
            }}));

        return v;
    }();
    return kAll;
}

QVariantMap toMap(const TerminalScheme& s) {
    QVariantList ansi;
    ansi.reserve(16);
    for (const auto& c : s.ansi) ansi.push_back(c);
    return QVariantMap{
        {"id", s.id},
        {"name", s.name},
        {"bg", s.bg},
        {"fg", s.fg},
        {"cursor", s.cursor},
        {"selectionBg", s.selectionBg},
        {"ansi", ansi},
    };
}

} // namespace

const QVector<TerminalScheme>& ThemeRegistry::all() {
    return palettes();
}

const TerminalScheme* ThemeRegistry::find(const QString& id) {
    for (const auto& s : palettes()) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

QVariantList ThemeRegistry::list() const {
    QVariantList out;
    out.reserve(palettes().size());
    for (const auto& s : palettes()) out.push_back(toMap(s));
    return out;
}

QVariantMap ThemeRegistry::findById(const QString& id) const {
    if (const auto* s = ThemeRegistry::find(id)) return toMap(*s);
    return QVariantMap{};
}

QVariantList ThemeRegistry::preview(const QString& id) const {
    const auto* s = ThemeRegistry::find(id);
    if (!s) return {};
    // bg + three accents drawn from the bright tier where available
    // (indices 1, 4, 5 — red/blue/magenta in ANSI convention).
    return QVariantList{
        s->bg,
        s->ansi[size_t(9)],
        s->ansi[size_t(12)],
        s->ansi[size_t(13)],
    };
}

} // namespace dante::themes
