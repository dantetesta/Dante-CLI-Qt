// PaletteController — see header for design notes.
#include "PaletteController.h"

#include "AppState.h"
#include "FavoritesModel.h"
#include "SnippetsModel.h"
#include "themes/ThemeRegistry.h"
#include "themes/TerminalScheme.h"

#include <QChar>
#include <QVariantMap>
#include <QStringList>

#include <algorithm>
#include <climits>

namespace dante {

namespace {

// Catalog row constants — labels in Portuguese to match the Swift sibling.
// hints kept short and action-y so the list reads like a menu.

constexpr int kMaxResults = 20;

} // namespace

PaletteController::PaletteController(AppState* appState,
                                     FavoritesModel* favorites,
                                     SnippetsModel* snippets,
                                     themes::ThemeRegistry* schemes,
                                     QObject* parent)
    : QObject(parent)
    , appState_(appState)
    , favorites_(favorites)
    , snippets_(snippets)
    , schemes_(schemes)
{
    rebuildCatalog();
    refilter();
}

void PaletteController::toggle() { setOpen(!open_); }
void PaletteController::show()   { setOpen(true);  }
void PaletteController::hide()   { setOpen(false); }

void PaletteController::setOpen(bool v) {
    if (open_ == v) return;
    open_ = v;
    if (open_) {
        // Reset query + selection each time we (re)open so the experience
        // matches Spotlight / VS Code Cmd+P (fresh slate).
        query_.clear();
        activeIndex_ = 0;
        refilter();
        emit queryChanged();
    }
    emit openChanged();
}

void PaletteController::setQuery(QString q) {
    if (query_ == q) return;
    query_ = std::move(q);
    activeIndex_ = 0;
    refilter();
    emit queryChanged();
    emit activeIndexChanged();
}

void PaletteController::setActiveIndex(int i) {
    if (filtered_.isEmpty()) { activeIndex_ = 0; emit activeIndexChanged(); return; }
    const int clamped = std::clamp(i, 0, int(filtered_.size()) - 1);
    if (clamped == activeIndex_) return;
    activeIndex_ = clamped;
    emit activeIndexChanged();
}

void PaletteController::selectNext() {
    if (filtered_.isEmpty()) return;
    activeIndex_ = (activeIndex_ + 1) % filtered_.size();
    emit activeIndexChanged();
}

void PaletteController::selectPrev() {
    if (filtered_.isEmpty()) return;
    activeIndex_ = (activeIndex_ - 1 + filtered_.size()) % filtered_.size();
    emit activeIndexChanged();
}

void PaletteController::execute() {
    if (filtered_.isEmpty()) return;
    const int idx = std::clamp(activeIndex_, 0, int(filtered_.size()) - 1);
    const int catIdx = filtered_.at(idx);
    if (catIdx < 0 || catIdx >= catalog_.size()) return;
    const auto cb = catalog_.at(catIdx).callback;
    setOpen(false);
    if (cb) cb();
}

/* ───────────────────────────── Catalog build ─────────────────────────── */

void PaletteController::rebuildCatalog() {
    catalog_.clear();
    buildStaticCatalog();

    // Dynamic: Favorites → "cd para «name»".
    if (favorites_) {
        const int n = favorites_->rowCount();
        for (int i = 0; i < n; ++i) {
            const auto idx = favorites_->index(i);
            const QString name = favorites_->data(idx, FavoritesModel::NameRole).toString();
            const QString path = favorites_->data(idx, FavoritesModel::PathRole).toString();
            const QString emoji= favorites_->data(idx, FavoritesModel::EmojiRole).toString();
            if (name.isEmpty() || path.isEmpty()) continue;
            Command c;
            c.id       = QStringLiteral("favorite.") + name;
            c.label    = QStringLiteral("cd para «%1»").arg(name);
            c.hint     = path;
            c.category = QStringLiteral("Favoritos");
            c.icon     = emoji.isEmpty() ? QStringLiteral("\xF0\x9F\x93\x82") /* 📂 */ : emoji;
            c.callback = [this, path]() {
                if (!appState_) return;
                const QString tabId = appState_->activeTabId();
                if (tabId.isEmpty()) return;
                const QString cmd = QStringLiteral("cd \"%1\"\r").arg(path);
                emit terminalWriteRequested(tabId, cmd);
            };
            catalog_.append(std::move(c));
        }
    }

    // Dynamic: Snippets.
    if (snippets_) {
        const int n = snippets_->rowCount();
        for (int i = 0; i < n; ++i) {
            const auto idx = snippets_->index(i);
            const QString name    = snippets_->data(idx, SnippetsModel::NameRole).toString();
            const QString command = snippets_->data(idx, SnippetsModel::CommandRole).toString();
            const QString emoji   = snippets_->data(idx, SnippetsModel::EmojiRole).toString();
            if (name.isEmpty() || command.isEmpty()) continue;
            Command c;
            c.id       = QStringLiteral("snippet.") + name;
            c.label    = QStringLiteral("Snippet: %1").arg(name);
            c.hint     = command;
            c.category = QStringLiteral("Snippets");
            c.icon     = emoji.isEmpty() ? QStringLiteral("\xE2\x9A\xA1") /* ⚡ */ : emoji;
            c.callback = [this, command]() {
                if (!appState_) return;
                const QString tabId = appState_->activeTabId();
                if (tabId.isEmpty()) return;
                emit terminalWriteRequested(tabId, command);
            };
            catalog_.append(std::move(c));
        }
    }

    // Dynamic: Terminal color schemes.
    if (schemes_) {
        for (const auto& s : themes::ThemeRegistry::all()) {
            const QString id   = s.id;
            const QString name = s.name;
            Command c;
            c.id       = QStringLiteral("theme.") + id;
            c.label    = QStringLiteral("Tema: %1").arg(name);
            c.hint     = id;
            c.category = QStringLiteral("Temas");
            c.icon     = QStringLiteral("\xF0\x9F\x8E\xA8"); // 🎨
            c.callback = [this, id]() {
                if (appState_) appState_->setTerminalScheme(id);
            };
            catalog_.append(std::move(c));
        }
    }

    refilter();
}

void PaletteController::buildStaticCatalog() {
    auto add = [&](QString id, QString label, QString hint, QString category,
                   QString shortcut, QString icon, std::function<void()> cb) {
        Command c;
        c.id = std::move(id);
        c.label = std::move(label);
        c.hint = std::move(hint);
        c.category = std::move(category);
        c.shortcut = std::move(shortcut);
        c.icon = std::move(icon);
        c.callback = std::move(cb);
        catalog_.append(std::move(c));
    };

    // ──────── Abas ────────
    add("tab.new", "Nova aba", "Cria uma nova aba de terminal", "Abas",
        "Ctrl+T", "\xE2\x9E\x95", /* ➕ */
        [this]() { if (appState_) appState_->addTab("Terminal"); });

    add("tab.close", "Fechar aba", "Fecha a aba ativa", "Abas",
        "Ctrl+W", "\xE2\x9C\x96", /* ✖ */
        [this]() { if (appState_) appState_->closeTab(appState_->activeTabId()); });

    add("tab.next", "Próxima aba", "Ir para a próxima aba", "Abas",
        "Ctrl+Tab", "\xE2\x86\x92", /* → */
        [this]() {
            if (!appState_) return;
            const auto& tabs = appState_->tabs();
            if (tabs.isEmpty()) return;
            int cur = -1;
            for (int i = 0; i < tabs.size(); ++i)
                if (tabs.at(i).id == appState_->activeTabId()) { cur = i; break; }
            const int next = (cur + 1 + tabs.size()) % tabs.size();
            appState_->selectTab(tabs.at(next).id);
        });

    add("tab.prev", "Aba anterior", "Ir para a aba anterior", "Abas",
        "Ctrl+Shift+Tab", "\xE2\x86\x90", /* ← */
        [this]() {
            if (!appState_) return;
            const auto& tabs = appState_->tabs();
            if (tabs.isEmpty()) return;
            int cur = -1;
            for (int i = 0; i < tabs.size(); ++i)
                if (tabs.at(i).id == appState_->activeTabId()) { cur = i; break; }
            const int prev = (cur - 1 + tabs.size()) % tabs.size();
            appState_->selectTab(tabs.at(prev).id);
        });

    add("split.vertical", "Dividir aba na vertical", "Cria um painel ao lado", "Abas",
        "Ctrl+D", "\xE2\x96\xAE", /* ▮ */
        [this]() { if (appState_) appState_->splitActive("vertical"); });

    add("split.horizontal", "Dividir aba na horizontal", "Cria um painel abaixo", "Abas",
        "Ctrl+Shift+D", "\xE2\x96\xAC", /* ▬ */
        [this]() { if (appState_) appState_->splitActive("horizontal"); });

    add("split.collapse", "Fechar divisão da aba", "Volta ao painel único", "Abas",
        "", "\xE2\x97\xBB", /* ◻ */
        [this]() { if (appState_) appState_->splitActive(""); });

    // ──────── AI ────────
    add("ai.toggle", "Abrir / fechar Dante AI", "Mostra o painel de IA", "IA",
        "Ctrl+Shift+A", "\xE2\x9C\xA8", /* ✨ */
        [this]() { emit aiToggleRequested(); });

    add("ai.voice", "Iniciar gravação de voz", "Captura áudio do microfone", "IA",
        "Ctrl+Ctrl+M", "\xF0\x9F\x8E\xA4", /* 🎤 */
        [this]() { emit voiceStartRequested(); });

    // ──────── Terminal ────────
    add("term.clear", "Limpar terminal", "Equivalente a `clear`", "Terminal",
        "Ctrl+L", "\xF0\x9F\xA7\xB9", /* 🧹 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty()) emit clearRequested(tabId);
        });

    add("term.focus", "Foco no terminal", "Devolve o foco para o terminal ativo",
        "Terminal", "Esc", "\xF0\x9F\x8E\xAF", /* 🎯 */
        [this]() { emit focusTerminalRequested(); });

    // ──────── Visualização ────────
    add("ui.toggleSidebar", "Alternar sidebar", "Cicla os modos da sidebar esquerda",
        "Interface", "Ctrl+B", "\xF0\x9F\x97\x82", /* 🗂 */
        [this]() {
            if (!appState_) return;
            // SidebarMode is an enum: Favorites=0, Snippets=1, Credentials=2, Files=3.
            // Cycle 0..3 and wrap.
            const int next = (appState_->sidebarMode() + 1) % 4;
            appState_->setSidebarMode(next);
        });

    add("ui.toggleRightSidebar", "Alternar sidebar direita",
        "Mostra/esconde o painel direito", "Interface",
        "Ctrl+Shift+B", "\xF0\x9F\xA7\xA9", /* 🧩 */
        [this]() {
            if (!appState_) return;
            appState_->setRightSidebarVisible(!appState_->rightSidebarVisible());
        });

    add("ui.fontInc", "Aumentar fonte", "Incrementa o tamanho do terminal",
        "Interface", "Ctrl++", "\xF0\x9F\x94\x8D", /* 🔍 */
        [this]() {
            if (!appState_) return;
            appState_->setFontSize(std::min(48, appState_->fontSize() + 1));
        });

    add("ui.fontDec", "Diminuir fonte", "Decrementa o tamanho do terminal",
        "Interface", "Ctrl+-", "\xF0\x9F\x94\x8E", /* 🔎 */
        [this]() {
            if (!appState_) return;
            appState_->setFontSize(std::max(8, appState_->fontSize() - 1));
        });

    // ──────── Paleta (meta — para discoverability) ────────
    add("palette.toggle", "Abrir paleta de comandos",
        "Mostra esta paleta", "Paleta",
        "Ctrl+K", "\xF0\x9F\x8E\xAF", /* 🎯 */
        [this]() { setOpen(true); });

    // ──────── Help / About ────────
    add("help.shortcuts", "Mostrar atalhos de teclado",
        "Lista todos os atalhos", "Ajuda",
        "Ctrl+Shift+/", "\xE2\x9D\x93", /* ❓ */
        [this]() {
            // Reuses ai overlay-style; let the host wire this if needed.
            // For now we simply (re)open the palette as a graceful fallback.
            setOpen(true);
        });

    add("help.about", "Sobre o Dante CLI", "Versão e créditos", "Ajuda",
        "", "\xE2\x84\xB9", /* ℹ */
        [this]() { setOpen(false); });

    // ──────── Sessão ────────
    add("session.save", "Salvar sessão agora", "Persiste abas/configurações",
        "Sessão", "", "\xF0\x9F\x92\xBE", /* 💾 */
        [this]() { if (appState_) appState_->saveOnClose(); });

    // ──────── Quick cd ────────
    add("cd.home", "cd para HOME", "Ir para a pasta do usuário", "Navegação",
        "", "\xF0\x9F\x8F\xA0", /* 🏠 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("cd ~\r"));
        });

    add("cd.desktop", "cd para Desktop", "Atalho rápido", "Navegação",
        "", "\xF0\x9F\x96\xA5", /* 🖥 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("cd ~/Desktop\r"));
        });

    add("cd.downloads", "cd para Downloads", "Atalho rápido", "Navegação",
        "", "\xE2\xAC\x87", /* ⬇ */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("cd ~/Downloads\r"));
        });

    add("git.status", "git status", "Executa no terminal ativo", "Git",
        "", "\xF0\x9F\x8C\xBF", /* 🌿 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("git status\r"));
        });

    add("git.log", "git log --oneline -20", "Histórico curto", "Git",
        "", "\xF0\x9F\x93\x9C", /* 📜 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("git log --oneline -20\r"));
        });

    add("git.diff", "git diff", "Mostra alterações não-comitadas", "Git",
        "", "\xF0\x9F\x94\x80", /* 🔀 */
        [this]() {
            if (!appState_) return;
            const QString tabId = appState_->activeTabId();
            if (!tabId.isEmpty())
                emit terminalWriteRequested(tabId, QStringLiteral("git diff\r"));
        });
}

/* ───────────────────────────── Fuzzy matcher ─────────────────────────── */

int PaletteController::score(const QString& query, const QString& label) {
    if (query.isEmpty()) return 0;       // everything matches with score 0
    if (label.isEmpty()) return INT_MIN;

    const QString q = query.toLower();
    const QString l = label.toLower();

    int score = 0;
    int li = 0;
    int lastMatchedAt = -1;

    for (int qi = 0; qi < q.size(); ++qi) {
        const QChar qc = q.at(qi);
        bool matched = false;
        while (li < l.size()) {
            const QChar lc = l.at(li);
            if (lc == qc) {
                int delta = 10;
                if (li == 0) {
                    delta += 50;
                } else {
                    const QChar prev = l.at(li - 1);
                    // Word boundary: previous char is non-alnum (space, '-', etc.)
                    if (!prev.isLetterOrNumber()) delta += 25;
                }
                if (lastMatchedAt >= 0) {
                    const int gap = li - lastMatchedAt - 1;
                    if (gap > 0) delta -= gap; // -1 per intervening char
                }
                score += delta;
                lastMatchedAt = li;
                ++li;
                matched = true;
                break;
            }
            ++li;
        }
        if (!matched) return INT_MIN;     // query char missing → no match
    }
    return score;
}

void PaletteController::refilter() {
    filtered_.clear();
    results_.clear();

    struct Hit { int idx; int score; };
    QVector<Hit> hits;
    hits.reserve(catalog_.size());

    const bool emptyQuery = query_.trimmed().isEmpty();

    for (int i = 0; i < catalog_.size(); ++i) {
        const auto& c = catalog_.at(i);
        int s;
        if (emptyQuery) {
            s = 0;
        } else {
            s = score(query_, c.label);
            // Also try category + hint as fallback alleys, with reduced weight.
            const int sCat = score(query_, c.category);
            const int sHnt = score(query_, c.hint);
            if (sCat != INT_MIN) s = std::max(s == INT_MIN ? INT_MIN : s, sCat / 2);
            if (sHnt != INT_MIN) s = std::max(s == INT_MIN ? INT_MIN : s, sHnt / 2);
            if (s == INT_MIN) continue;
            if (s < 0) continue;
        }
        hits.push_back({i, s});
    }

    std::stable_sort(hits.begin(), hits.end(),
                     [](const Hit& a, const Hit& b) { return a.score > b.score; });

    const int n = std::min<int>(kMaxResults, hits.size());
    for (int i = 0; i < n; ++i) {
        const auto& h = hits.at(i);
        const auto& c = catalog_.at(h.idx);
        filtered_.append(h.idx);
        QVariantMap row;
        row.insert("id",       c.id);
        row.insert("label",    c.label);
        row.insert("hint",     c.hint);
        row.insert("category", c.category);
        row.insert("shortcut", c.shortcut);
        row.insert("icon",     c.icon);
        row.insert("score",    h.score);
        results_.append(row);
    }

    if (activeIndex_ >= filtered_.size())
        activeIndex_ = filtered_.isEmpty() ? 0 : (filtered_.size() - 1);

    emit resultsChanged();
    emit activeIndexChanged();
}

} // namespace dante
