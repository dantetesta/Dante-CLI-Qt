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

        // ─── SPEC-100 expansion: full 47-scheme parity with the Swift
        // sibling's Utilities/TerminalThemes.swift. selectionBg derived
        // (slightly darker than bg) where the Swift code didn't ship one.

        v.push_back(make("DraculaPro", "Dracula Pro",
            "#22212c", "#f8f8f2", "#ff9580", "#3a3850", {{
                "#22212c", "#ff9580", "#8aff80", "#ffca80",
                "#9580ff", "#ff80bf", "#80ffea", "#f8f8f2",
                "#454158", "#ffaa99", "#a2ff99", "#ffd699",
                "#aa99ff", "#ff99cc", "#99ffee", "#ffffff"
            }}));

        v.push_back(make("TokyoNightStorm", "Tokyo Night Storm",
            "#24283b", "#a9b1d6", "#c0caf5", "#364a82", {{
                "#1d202f", "#f7768e", "#9ece6a", "#e0af68",
                "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6",
                "#414868", "#f7768e", "#9ece6a", "#e0af68",
                "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5"
            }}));

        v.push_back(make("AppleClassic", "Apple Classic",
            "#000000", "#c7c7c7", "#ffffff", "#3a3a3a", {{
                "#000000", "#c91b00", "#00c200", "#c7c400",
                "#0225c7", "#ca30c7", "#00c5c7", "#c7c7c7",
                "#686868", "#ff6e67", "#5ffa68", "#fffc67",
                "#6871ff", "#ff77ff", "#60fdff", "#ffffff"
            }}));

        v.push_back(make("MonokaiPro", "Monokai Pro",
            "#2d2a2e", "#fcfcfa", "#ffd866", "#403e41", {{
                "#403e41", "#ff6188", "#a9dc76", "#ffd866",
                "#fc9867", "#ab9df2", "#78dce8", "#fcfcfa",
                "#727072", "#ff6188", "#a9dc76", "#ffd866",
                "#fc9867", "#ab9df2", "#78dce8", "#fcfcfa"
            }}));

        v.push_back(make("GruvboxLight", "Gruvbox Light",
            "#fbf1c7", "#3c3836", "#3c3836", "#ebdbb2", {{
                "#fbf1c7", "#cc241d", "#98971a", "#d79921",
                "#458588", "#b16286", "#689d6a", "#7c6f64",
                "#928374", "#9d0006", "#79740e", "#b57614",
                "#076678", "#8f3f71", "#427b58", "#3c3836"
            }}));

        v.push_back(make("CatppuccinLatte", "Catppuccin Latte",
            "#eff1f5", "#4c4f69", "#dc8a78", "#ccd0da", {{
                "#5c5f77", "#d20f39", "#40a02b", "#df8e1d",
                "#1e66f5", "#ea76cb", "#179299", "#acb0be",
                "#6c6f85", "#d20f39", "#40a02b", "#df8e1d",
                "#1e66f5", "#ea76cb", "#179299", "#bcc0cc"
            }}));

        v.push_back(make("CatppuccinMacchiato", "Catppuccin Macchiato",
            "#24273a", "#cad3f5", "#f4dbd6", "#363a4f", {{
                "#494d64", "#ed8796", "#a6da95", "#eed49f",
                "#8aadf4", "#f5bde6", "#8bd5ca", "#b8c0e0",
                "#5b6078", "#ed8796", "#a6da95", "#eed49f",
                "#8aadf4", "#f5bde6", "#8bd5ca", "#a5adcb"
            }}));

        v.push_back(make("CatppuccinFrappe", "Catppuccin Frappé",
            "#303446", "#c6d0f5", "#f2d5cf", "#414559", {{
                "#51576d", "#e78284", "#a6d189", "#e5c890",
                "#8caaee", "#f4b8e4", "#81c8be", "#b5bfe2",
                "#626880", "#e78284", "#a6d189", "#e5c890",
                "#8caaee", "#f4b8e4", "#81c8be", "#a5adce"
            }}));

        v.push_back(make("GithubDark", "GitHub Dark",
            "#0d1117", "#c9d1d9", "#58a6ff", "#21262d", {{
                "#484f58", "#ff7b72", "#3fb950", "#d29922",
                "#58a6ff", "#bc8cff", "#39c5cf", "#b1bac4",
                "#6e7681", "#ffa198", "#56d364", "#e3b341",
                "#79c0ff", "#d2a8ff", "#56d4dd", "#f0f6fc"
            }}));

        v.push_back(make("GithubLight", "GitHub Light",
            "#ffffff", "#24292f", "#0969da", "#d0d7de", {{
                "#24292f", "#cf222e", "#1a7f37", "#9a6700",
                "#0969da", "#8250df", "#1b7c83", "#6e7781",
                "#57606a", "#a40e26", "#116329", "#7d4e00",
                "#0550ae", "#6639ba", "#3192aa", "#24292f"
            }}));

        v.push_back(make("MaterialDark", "Material Dark",
            "#263238", "#eeffff", "#ffcc02", "#37474f", {{
                "#000000", "#ff5370", "#c3e88d", "#ffcb6b",
                "#82aaff", "#c792ea", "#89ddff", "#ffffff",
                "#545454", "#ff5370", "#c3e88d", "#ffcb6b",
                "#82aaff", "#c792ea", "#89ddff", "#ffffff"
            }}));

        v.push_back(make("NightOwl", "Night Owl",
            "#011627", "#d6deeb", "#80a4c2", "#1d3b53", {{
                "#011627", "#ef5350", "#22da6e", "#c5e478",
                "#82aaff", "#c792ea", "#21c7a8", "#ffffff",
                "#575656", "#ef5350", "#22da6e", "#ffeb95",
                "#82aaff", "#c792ea", "#7fdbca", "#ffffff"
            }}));

        v.push_back(make("NightOwlLight", "Night Owl Light",
            "#fbfbfb", "#403f53", "#5ca7e4", "#e0e7ec", {{
                "#011627", "#d3423e", "#2aa298", "#daaa01",
                "#4876d6", "#403f53", "#08916a", "#7a8181",
                "#7a8181", "#f76e6e", "#49d0c5", "#dac26b",
                "#5ca7e4", "#697098", "#00c990", "#989fb1"
            }}));

        v.push_back(make("Synthwave84", "Synthwave 84",
            "#262335", "#f0eff1", "#f97e72", "#3b3257", {{
                "#000000", "#fe4450", "#72f1b8", "#fede5d",
                "#03edf9", "#ff7edb", "#03edf9", "#f0eff1",
                "#495495", "#fe4450", "#72f1b8", "#fede5d",
                "#36f9f6", "#ff7edb", "#36f9f6", "#ffffff"
            }}));

        v.push_back(make("Cobalt", "Cobalt",
            "#002240", "#ffffff", "#ffc600", "#003560", {{
                "#000000", "#ff2600", "#3ad900", "#ffc600",
                "#1e88e5", "#9b5cdc", "#00aaaa", "#ffffff",
                "#555555", "#ff5e5e", "#79ff5e", "#ffd866",
                "#5cb6f8", "#b893e6", "#5ce6e6", "#ffffff"
            }}));

        v.push_back(make("Cobalt2", "Cobalt 2",
            "#193549", "#ffffff", "#ffc600", "#0d2a40", {{
                "#000000", "#ff2c83", "#3ad900", "#ffc600",
                "#1278d8", "#9b5cdc", "#00cccc", "#ffffff",
                "#555555", "#ff5e5e", "#79ff5e", "#ffd866",
                "#5cb6f8", "#b893e6", "#5ce6e6", "#ffffff"
            }}));

        v.push_back(make("Palenight", "Palenight",
            "#292d3e", "#959dcb", "#ffcc00", "#3b3f51", {{
                "#292d3e", "#f07178", "#c3e88d", "#ffcb6b",
                "#82aaff", "#c792ea", "#89ddff", "#959dcb",
                "#676e95", "#ff8b92", "#ddffa7", "#ffe585",
                "#9cc4ff", "#e1acff", "#a3f7ff", "#ffffff"
            }}));

        v.push_back(make("AyuDark", "Ayu Dark",
            "#0a0e14", "#b3b1ad", "#e6b450", "#1a1f29", {{
                "#01060e", "#ea6c73", "#91b362", "#f9af4f",
                "#53bdfa", "#fae994", "#90e1c6", "#c7c7c7",
                "#686868", "#f07178", "#c2d94c", "#ffb454",
                "#59c2ff", "#ffee99", "#95e6cb", "#ffffff"
            }}));

        v.push_back(make("AyuMirage", "Ayu Mirage",
            "#1f2430", "#cbccc6", "#ffcc66", "#2a3140", {{
                "#191e2a", "#ed8274", "#a6cc70", "#fad07b",
                "#6dcbfa", "#cfbafa", "#90e1c6", "#c7c7c7",
                "#686868", "#f28779", "#bae67e", "#ffd580",
                "#73d0ff", "#d4bfff", "#95e6cb", "#ffffff"
            }}));

        v.push_back(make("AyuLight", "Ayu Light",
            "#fafafa", "#5c6773", "#ff6a00", "#e7eaed", {{
                "#000000", "#f07171", "#86b300", "#f2ae49",
                "#399ee6", "#a37acc", "#4cbf99", "#5c6773",
                "#828c99", "#f07171", "#86b300", "#f2ae49",
                "#399ee6", "#a37acc", "#4cbf99", "#000000"
            }}));

        v.push_back(make("VSCodeDark", "VS Code Dark",
            "#1e1e1e", "#cccccc", "#aeafad", "#264f78", {{
                "#000000", "#cd3131", "#0dbc79", "#e5e510",
                "#2472c8", "#bc3fbc", "#11a8cd", "#e5e5e5",
                "#666666", "#f14c4c", "#23d18b", "#f5f543",
                "#3b8eea", "#d670d6", "#29b8db", "#e5e5e5"
            }}));

        v.push_back(make("VSCodeLight", "VS Code Light",
            "#ffffff", "#333333", "#000000", "#add6ff", {{
                "#000000", "#cd3131", "#00bc00", "#949800",
                "#0451a5", "#bc05bc", "#0598bc", "#555555",
                "#666666", "#cd3131", "#14ce14", "#b5ba00",
                "#0451a5", "#bc05bc", "#0598bc", "#a5a5a5"
            }}));

        v.push_back(make("Ocean", "Ocean",
            "#2b303b", "#c0c5ce", "#c0c5ce", "#343d46", {{
                "#2b303b", "#bf616a", "#a3be8c", "#ebcb8b",
                "#8fa1b3", "#b48ead", "#96b5b4", "#c0c5ce",
                "#65737e", "#bf616a", "#a3be8c", "#ebcb8b",
                "#8fa1b3", "#b48ead", "#96b5b4", "#eff1f5"
            }}));

        v.push_back(make("OceanicNext", "Oceanic Next",
            "#1b2b34", "#d8dee9", "#c0c5ce", "#243741", {{
                "#29414f", "#ec5f67", "#99c794", "#fac863",
                "#6699cc", "#c594c5", "#5fb3b3", "#65737e",
                "#405860", "#ec5f67", "#99c794", "#fac863",
                "#6699cc", "#c594c5", "#5fb3b3", "#d8dee9"
            }}));

        v.push_back(make("Gotham", "Gotham",
            "#0a0f14", "#98d1ce", "#599cab", "#11151c", {{
                "#0a0f14", "#c33027", "#26a98b", "#edb443",
                "#195466", "#4e5165", "#33859a", "#98d1ce",
                "#10151b", "#d26939", "#081f2d", "#245361",
                "#093748", "#888987", "#599cab", "#d3ebe9"
            }}));

        v.push_back(make("Seti", "Seti",
            "#151718", "#cacecd", "#cacecd", "#282a2b", {{
                "#323232", "#c22832", "#8ec43d", "#e0c64f",
                "#43a5d5", "#8b57b5", "#8ec43d", "#eeeeee",
                "#323232", "#c22832", "#8ec43d", "#e0c64f",
                "#43a5d5", "#8b57b5", "#8ec43d", "#ffffff"
            }}));

        v.push_back(make("Snazzy", "Snazzy",
            "#1a1f29", "#eff0eb", "#97979b", "#2a2f3a", {{
                "#282a36", "#ff5c57", "#5af78e", "#f3f99d",
                "#57c7ff", "#ff6ac1", "#9aedfe", "#f1f1f0",
                "#686868", "#ff5c57", "#5af78e", "#f3f99d",
                "#57c7ff", "#ff6ac1", "#9aedfe", "#eff0eb"
            }}));

        v.push_back(make("Hopscotch", "Hopscotch",
            "#322931", "#b9b5b8", "#989498", "#433b42", {{
                "#322931", "#dd464c", "#8fc13e", "#fdcc59",
                "#1290bf", "#c85e7c", "#149b93", "#b9b5b8",
                "#797379", "#fd8b19", "#433b42", "#5c545b",
                "#989498", "#d5d3d5", "#b33508", "#ffffff"
            }}));

        v.push_back(make("HackerGreen", "Hacker Green",
            "#000000", "#00ff00", "#00ff00", "#003300", {{
                "#000000", "#008f11", "#00ff41", "#26a96c",
                "#004c00", "#003b00", "#00cc66", "#00ff00",
                "#005500", "#00d20f", "#00ff41", "#39ff14",
                "#33a532", "#00ff66", "#22ff66", "#aaffaa"
            }}));

        v.push_back(make("AmberOnBlack", "Amber on Black",
            "#000000", "#ffb000", "#ffb000", "#332200", {{
                "#000000", "#ff5500", "#ffaa00", "#ffd700",
                "#cc7700", "#aa5500", "#ff8800", "#ffb000",
                "#553300", "#ff7733", "#ffbf33", "#ffe066",
                "#dd9933", "#bb6633", "#ffaa44", "#ffdd99"
            }}));

        v.push_back(make("Novel", "Novel",
            "#dfdbc3", "#3b2322", "#73635a", "#cbc7af", {{
                "#000000", "#cc0000", "#009600", "#d06b00",
                "#0000cc", "#7300cc", "#00b6cc", "#cccccc",
                "#808080", "#cc0000", "#009600", "#d06b00",
                "#0000cc", "#7300cc", "#00b6cc", "#3b2322"
            }}));

        v.push_back(make("OneLight", "One Light",
            "#fafafa", "#383a42", "#526fff", "#dadada", {{
                "#fafafa", "#e45649", "#50a14f", "#c18401",
                "#4078f2", "#a626a4", "#0184bc", "#383a42",
                "#a0a1a7", "#e45649", "#50a14f", "#c18401",
                "#4078f2", "#a626a4", "#0184bc", "#383a42"
            }}));

        v.push_back(make("RosePineMoon", "Rosé Pine Moon",
            "#232136", "#e0def4", "#56526e", "#393552", {{
                "#393552", "#eb6f92", "#3e8fb0", "#f6c177",
                "#9ccfd8", "#c4a7e7", "#ea9a97", "#e0def4",
                "#6e6a86", "#eb6f92", "#3e8fb0", "#f6c177",
                "#9ccfd8", "#c4a7e7", "#ea9a97", "#e0def4"
            }}));

        v.push_back(make("RosePineDawn", "Rosé Pine Dawn",
            "#faf4ed", "#575279", "#9893a5", "#f2e9e1", {{
                "#f2e9e1", "#b4637a", "#286983", "#ea9d34",
                "#56949f", "#907aa9", "#d7827e", "#575279",
                "#9893a5", "#b4637a", "#286983", "#ea9d34",
                "#56949f", "#907aa9", "#d7827e", "#575279"
            }}));

        v.push_back(make("EverforestDark", "Everforest Dark",
            "#2d353b", "#d3c6aa", "#d3c6aa", "#414b50", {{
                "#475258", "#e67e80", "#a7c080", "#dbbc7f",
                "#7fbbb3", "#d699b6", "#83c092", "#d3c6aa",
                "#475258", "#e67e80", "#a7c080", "#dbbc7f",
                "#7fbbb3", "#d699b6", "#83c092", "#d3c6aa"
            }}));

        v.push_back(make("EverforestLight", "Everforest Light",
            "#fdf6e3", "#5c6a72", "#5c6a72", "#efebd4", {{
                "#5c6a72", "#f85552", "#8da101", "#dfa000",
                "#3a94c5", "#df69ba", "#35a77c", "#fdf6e3",
                "#939f91", "#f85552", "#8da101", "#dfa000",
                "#3a94c5", "#df69ba", "#35a77c", "#fdf6e3"
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
